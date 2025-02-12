//----------------------------------*-C++-*----------------------------------//
// Copyright 2021-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file demo-loop/LDemoIO.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>
#include <vector>
#include <celeritas/Types.hh>
#include <nlohmann/json.hpp>

#include "celeritas_config.h"
#include "corecel/Assert.hh"
#include "corecel/Types.hh"
#include "corecel/math/NumericLimits.hh"
#include "celeritas/ext/GeantPhysicsOptions.hh"
#include "celeritas/ext/GeantSetup.hh"
#include "celeritas/field/FieldDriverOptions.hh"
#include "celeritas/io/RootFileManager.hh"
#include "celeritas/phys/Model.hh"
#include "celeritas/phys/PrimaryGeneratorOptions.hh"

#include "Transporter.hh"

namespace celeritas
{
class OutputRegistry;
class CoreParams;

NLOHMANN_JSON_SERIALIZE_ENUM(TrackOrder,
                             {{TrackOrder::unsorted, "unsorted"},
                              {TrackOrder::shuffled, "shuffled"}})
}

namespace demo_loop
{
//---------------------------------------------------------------------------//
/*!
 * Write when event ID matches and either track ID or parent ID matches, or
 * when action ID matches.
 */
struct MCTruthFilter
{
    using size_type = celeritas::size_type;

    static constexpr size_type unspecified()
    {
        return static_cast<size_type>(-1);
    }

    std::vector<size_type> track_id;
    size_type event_id = unspecified();
    size_type parent_id = unspecified();
    size_type action_id = unspecified();

    explicit operator bool() const
    {
        return !track_id.empty() || event_id != unspecified()
               || parent_id != unspecified() || action_id != unspecified();
    }
};

//---------------------------------------------------------------------------//
/*!
 * Input for a single run.
 */
struct LDemoArgs
{
    using real_type = celeritas::real_type;
    using Real3 = celeritas::Real3;
    using size_type = celeritas::size_type;

    static constexpr Real3 no_field() { return Real3{0, 0, 0}; }

    // Problem definition
    std::string geometry_filename;  //!< Path to GDML file
    std::string physics_filename;  //!< Path to ROOT exported Geant4 data
    std::string hepmc3_filename;  //!< Path to HepMC3 event data
    std::string mctruth_filename;  //!< Path to ROOT MC truth event data

    // Optional filter for ROOT MC truth data
    MCTruthFilter mctruth_filter;

    // Optional setup options for generating primaries programmatically
    celeritas::PrimaryGeneratorOptions primary_gen_options;

    // Control
    unsigned int seed{};
    size_type max_num_tracks{};
    size_type max_steps = TransporterInput::no_max_steps();
    size_type initializer_capacity{};
    size_type max_events{};
    real_type secondary_stack_factor{};
    bool enable_diagnostics{};
    bool use_device{};
    bool sync{};

    // Magnetic field vector [* 1/Tesla] and associated field options
    Real3 mag_field{no_field()};
    celeritas::FieldDriverOptions field_options;

    // Optional fixed-size step limiter for charged particles
    // (non-positive for unused)
    real_type step_limiter{};

    // Options for physics
    bool brem_combined{true};

    // Diagnostic input
    EnergyDiagInput energy_diag;

    // Track init options
    celeritas::TrackOrder track_order{celeritas::TrackOrder::unsorted};

    // Optional setup options if loading directly from Geant4
    celeritas::GeantPhysicsOptions geant_options;

    //! Whether the run arguments are valid
    explicit operator bool() const
    {
        return !geometry_filename.empty() && !physics_filename.empty()
               && (primary_gen_options || !hepmc3_filename.empty())
               && max_num_tracks > 0 && max_steps > 0
               && initializer_capacity > 0 && max_events > 0
               && secondary_stack_factor > 0
               && (mag_field == no_field() || field_options);
    }
};

// Build transporter from input arguments
std::unique_ptr<TransporterBase>
build_transporter(LDemoArgs const& args,
                  std::shared_ptr<celeritas::OutputRegistry> const& outreg);

void to_json(nlohmann::json& j, LDemoArgs const& value);
void from_json(nlohmann::json const& j, LDemoArgs& value);

// Store LDemoArgs to ROOT file when ROOT is available
void to_root(celeritas::RootFileManager& root_manager, LDemoArgs const& cargs);

// Store CoreParams to ROOT file when ROOT is available
void to_root(celeritas::RootFileManager& root_manager,
             celeritas::CoreParams const& core_params);

#if !CELERITAS_USE_ROOT
inline void to_root(celeritas::RootFileManager&, LDemoArgs const&)
{
    CELER_NOT_CONFIGURED("ROOT");
}

inline void to_root(celeritas::RootFileManager&, celeritas::CoreParams const&)
{
    CELER_NOT_CONFIGURED("ROOT");
}
#endif

//---------------------------------------------------------------------------//
}  // namespace demo_loop
