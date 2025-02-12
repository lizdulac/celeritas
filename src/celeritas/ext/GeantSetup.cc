//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/ext/GeantSetup.cc
//---------------------------------------------------------------------------//
#include "GeantSetup.hh"

#include <memory>
#include <utility>
#include <G4ParticleTable.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VUserDetectorConstruction.hh>
#include <G4Version.hh>

#if G4VERSION_NUMBER >= 1070
#    include <G4Backtrace.hh>
#else
#    include <G4MTRunManager.hh>
#endif
#if G4VERSION_NUMBER >= 1100
#    include <G4RunManagerFactory.hh>
#else
#    include <G4RunManager.hh>
#endif

#include "corecel/io/Logger.hh"
#include "corecel/io/ScopedTimeAndRedirect.hh"
#include "corecel/io/ScopedTimeLog.hh"
#include "corecel/sys/ScopedMem.hh"

#include "LoadGdml.hh"
#include "detail/GeantExceptionHandler.hh"
#include "detail/GeantLoggerAdapter.hh"
#include "detail/GeantPhysicsList.hh"

namespace celeritas
{
namespace
{
//---------------------------------------------------------------------------//
/*!
 * Load the detector geometry from a GDML input file.
 */
class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    explicit DetectorConstruction(UPG4PhysicalVolume world)
        : world_{std::move(world)}
    {
        CELER_ENSURE(world_);
    }

    G4VPhysicalVolume* Construct() override
    {
        CELER_EXPECT(world_);
        return world_.release();
    }

  private:
    UPG4PhysicalVolume world_;
};

//---------------------------------------------------------------------------//
}  // namespace

//---------------------------------------------------------------------------//
/*!
 * Get the number of threads in a version-portable way.
 *
 * G4RunManager::GetNumberOfThreads isn't virtual before Geant4 v10.7.0 so we
 * need to explicitly dynamic cast to G4MTRunManager to get the number of
 * threads.
 */
int get_num_threads(G4RunManager const& runman)
{
#if G4VERSION_NUMBER >= 1070
    return runman.GetNumberOfThreads();
#else
    if (auto const* runman_mt = dynamic_cast<G4MTRunManager const*>(&runman))
    {
        return runman_mt->GetNumberOfThreads();
    }
    // Not multithreaded
    return 1;
#endif
}

//---------------------------------------------------------------------------//
/*!
 * Construct from a GDML file and physics options.
 */
GeantSetup::GeantSetup(std::string const& gdml_filename, Options options)
{
    CELER_LOG(status) << "Initializing Geant4 run manager";
    ScopedMem record_setup_mem("GeantSetup.construct");

    {
        // Run manager writes output that cannot be redirected...
        ScopedTimeAndRedirect scoped_time("G4RunManager");
        detail::GeantExceptionHandler scoped_exception_handler;
        // Access the particle table before creating the run manager, so that
        // missing environment variables like G4ENSDFSTATEDATA get caught
        // cleanly rather than segfaulting
        G4ParticleTable::GetParticleTable();

        // Guard against segfaults due to bad Geant4 global cleanup
        static int geant_launch_count = 0;
        CELER_VALIDATE(geant_launch_count == 0,
                       << "Geant4 cannot be 'run' more than once per "
                          "execution");
        ++geant_launch_count;

        // Disable geant4 signal interception
#if G4VERSION_NUMBER >= 1070
        G4Backtrace::DefaultSignals() = {};
#endif

#if G4VERSION_NUMBER >= 1100
        run_manager_.reset(
            G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial));
#else
        // Note: custom deleter means `make_unique` won't work
        run_manager_.reset(new G4RunManager);
#endif
        CELER_ASSERT(run_manager_);
    }

    detail::GeantLoggerAdapter scoped_logger;
    detail::GeantExceptionHandler scoped_exception_handler;

    {
        CELER_LOG(status) << "Initializing Geant4 geometry and physics";
        ScopedTimeLog scoped_time;

        // Load GDML and save a copy of the pointer
        auto world = load_gdml(gdml_filename);
        CELER_ASSERT(world);
        world_ = world.get();

        // Construct the geometry
        auto detector
            = std::make_unique<DetectorConstruction>(std::move(world));
        run_manager_->SetUserInitialization(detector.release());

        // Construct the physics
        auto physics_list = std::make_unique<detail::GeantPhysicsList>(options);
        run_manager_->SetUserInitialization(physics_list.release());
    }

    {
        CELER_LOG(status) << "Initializing Geant4 physics tables";
        ScopedMem record_mem("GeantSetup.initialize");
        ScopedTimeLog scoped_time;

        run_manager_->Initialize();
        run_manager_->RunInitialization();
    }

    CELER_ENSURE(world_);
    CELER_ENSURE(*this);
}

//---------------------------------------------------------------------------//
/*!
 * Terminate the run manager on destruction.
 */
GeantSetup::~GeantSetup()
{
    if (run_manager_)
    {
        run_manager_->RunTermination();
    }
}

//---------------------------------------------------------------------------//
/*!
 * Delete a geant4 run manager.
 */
void GeantSetup::RMDeleter::operator()(G4RunManager* rm) const
{
    CELER_LOG(debug) << "Clearing Geant4 state";
    delete rm;
}

//---------------------------------------------------------------------------//
}  // namespace celeritas
