//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2022 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file PhysicsList.cc
//---------------------------------------------------------------------------//
#include "PhysicsList.hh"

#include <G4ComptonScattering.hh>
#include <G4KleinNishinaModel.hh>
#include <G4LivermorePhotoElectricModel.hh>
#include <G4PhotoElectricEffect.hh>
#include <G4PhysicsListHelper.hh>
#include <G4ProcessManager.hh>
#include <G4Proton.hh>
#include <G4SystemOfUnits.hh>

#include "base/Assert.hh"
#include "comm/Logger.hh"

// Not from Geant4
#include <G4BetheHeitlerModel.hh>
#include <G4CoulombScattering.hh>
#include <G4GammaConversion.hh>
#include <G4LivermoreRayleighModel.hh>
#include <G4MollerBhabhaModel.hh>
#include <G4RayleighScattering.hh>
#include <G4UrbanMscModel.hh>
#include <G4WentzelVIModel.hh>
#include <G4eCoulombScatteringModel.hh>
#include <G4eIonisation.hh>
#include <G4eMultipleScattering.hh>
#include <G4eeToTwoGammaModel.hh>
#include <G4eplusAnnihilation.hh>

#include "BremsstrahlungProcess.hh"

namespace geant_exporter
{
//---------------------------------------------------------------------------//
/*!
 * Construct empty.
 */
PhysicsList::PhysicsList() : G4VUserPhysicsList()
{
    CELER_LOG(status) << "Loading physics list";

    // Manually set the physics table binning
    // G4EmParameters::Instance()->SetNumberOfBins(10);
}

//---------------------------------------------------------------------------//
/*!
 * Default destructor.
 */
PhysicsList::~PhysicsList() = default;

//---------------------------------------------------------------------------//
/*!
 * Build list of available particles.
 *
 * The minimal E.M. set can be built by using
 * \c G4EmBuilder::ConstructMinimalEmSet();
 * and includes gamma, e+, e-, mu+, mu-, pi+, pi-, K+, K-, p, pbar, deuteron,
 * triton, He3, alpha, and generic ion, along with Geant4's pseudo-particles
 * geantino and charged geantino.
 *
 * Currently only instantiating e+, e-, gamma, and proton (the latter is needed
 * for msc)
 */
void PhysicsList::ConstructParticle()
{
    G4Gamma::GammaDefinition();
    G4Electron::ElectronDefinition();
    G4Positron::PositronDefinition();
    G4Proton::ProtonDefinition();
}

//---------------------------------------------------------------------------//
/*!
 * Build list of available processes and models.
 */
void PhysicsList::ConstructProcess()
{
    // Applies to all constructed particles
    G4VUserPhysicsList::AddTransportation();

    // Add E.M. processes for photons, electrons, and positrons
    this->add_gamma_processes();
    this->add_e_processes();
}

//---------------------------------------------------------------------------//
// PRIVATE
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
/*!
 * Add E.M. processes for photons.
 *
 * | Processes            | Model classes                 |
 * | -------------------- | ----------------------------- |
 * | Compton scattering   | G4KleinNishinaModel           |
 * | Photoelectric effect | G4LivermorePhotoElectricModel |
 * | Rayleigh scattering  | G4LivermoreRayleighModel      |
 * | Gamma conversion     | G4BetheHeitlerModel           |
 */
void PhysicsList::add_gamma_processes()
{
    auto       physics_list = G4PhysicsListHelper::GetPhysicsListHelper();
    const auto gamma        = G4Gamma::Gamma();

    if (true)
    {
        // Compton Scattering: G4KleinNishinaModel
        auto compton_scattering = std::make_unique<G4ComptonScattering>();
        compton_scattering->SetEmModel(new G4KleinNishinaModel());
        physics_list->RegisterProcess(compton_scattering.release(), gamma);

        CELER_LOG(info) << "Loaded Compton scattering with "
                           "G4KleinNishinaModel";
    }

    if (true)
    {
        // Photoelectric effect: G4LivermorePhotoElectricModel
        auto photoelectrict_effect = std::make_unique<G4PhotoElectricEffect>();
        photoelectrict_effect->SetEmModel(new G4LivermorePhotoElectricModel());
        physics_list->RegisterProcess(photoelectrict_effect.release(), gamma);

        CELER_LOG(info) << "Loaded photoelectric effect with "
                           "G4LivermorePhotoElectricModel";
    }

    if (true)
    {
        // Rayleigh: G4LivermoreRayleighModel
        physics_list->RegisterProcess(new G4RayleighScattering(), gamma);

        CELER_LOG(info) << "Loaded Rayleigh scattering with "
                           "G4LivermoreRayleighModel";
    }

    if (true)
    {
        // Gamma conversion: G4BetheHeitlerModel
        auto gamma_conversion = std::make_unique<G4GammaConversion>();
        gamma_conversion->SetEmModel(new G4BetheHeitlerModel());
        physics_list->RegisterProcess(gamma_conversion.release(), gamma);

        CELER_LOG(info) << "Loaded gamma conversion with G4BetheHeitlerModel";
    }
}

//---------------------------------------------------------------------------//
/*!
 * Add E.M. processes for electrons and positrons.
 *
 * | Processes                    | Model classes             |
 * | ---------------------------- | --------------------------|
 * | Pair annihilation            | G4eeToTwoGammaModel       |
 * | Ionization                   | G4MollerBhabhaModel       |
 * | Bremsstrahlung (low E)       | G4SeltzerBergerModel      |
 * | Bremsstrahlung (high E)      | G4eBremsstrahlungRelModel |
 * | Coulomb scattering           | G4eCoulombScatteringModel |
 * | Multiple scattering (low E)  | G4UrbanMscModel           |
 * | Multiple scattering (high E) | G4WentzelVIModel          |
 *
 * \note
 * - Bremsstrahlung models are selected manually at compile time using
 *   \c BremsstrahlungProcess::ModelSelection and need to be updated
 *   accordingly.
 * - Coulomb scattering and multiple scattering (high E) are currently
 *   disabled.
 */
void PhysicsList::add_e_processes()
{
    auto       physics_list = G4PhysicsListHelper::GetPhysicsListHelper();
    const auto electron     = G4Electron::Electron();
    const auto positron     = G4Positron::Positron();

    if (true)
    {
        // e+e- annihilation: G4eeToTwoGammaModel
        physics_list->RegisterProcess(new G4eplusAnnihilation(), positron);

        CELER_LOG(info) << "Loaded pair annihilation with G4eplusAnnihilation";
    }

    if (true)
    {
        // e-e+ ionization: G4MollerBhabhaModel
        auto electron_ionization = std::make_unique<G4eIonisation>();
        auto positron_ionization = std::make_unique<G4eIonisation>();
        electron_ionization->SetEmModel(new G4MollerBhabhaModel());
        positron_ionization->SetEmModel(new G4MollerBhabhaModel());
        physics_list->RegisterProcess(electron_ionization.release(), electron);
        physics_list->RegisterProcess(positron_ionization.release(), positron);

        CELER_LOG(info) << "Loaded e-e+ ionization with G4MollerBhabhaModel";
    }

    if (true)
    {
        // Bremsstrahlung: G4SeltzerBergerModel + G4eBremsstrahlungRelModel
        // Currently only using Seltzer-Berger
        auto models = BremsstrahlungProcess::ModelSelection::seltzer_berger;

        auto electron_brems = std::make_unique<BremsstrahlungProcess>(models);
        auto positron_brems = std::make_unique<BremsstrahlungProcess>(models);
        physics_list->RegisterProcess(electron_brems.release(), electron);
        physics_list->RegisterProcess(positron_brems.release(), positron);

        switch (models)
        {
            case BremsstrahlungProcess::ModelSelection::seltzer_berger:
                CELER_LOG(info) << "Loaded Bremsstrahlung with "
                                   "G4SeltzerBergerModel";
                break;
            case BremsstrahlungProcess::ModelSelection::relativistic:
                CELER_LOG(info) << "Loaded Bremsstrahlung with "
                                   "G4eBremsstrahlungRelModel";
                break;
            case BremsstrahlungProcess::ModelSelection::all:
                CELER_LOG(info)
                    << "Loaded Bremsstrahlung with "
                       "G4SeltzerBergerModel and G4eBremsstrahlungRelModel";
                break;
            default:
                CELER_ASSERT_UNREACHABLE();
        }
    }

    // DISABLED
    if (false)
    {
        // Coulomb scattering: G4eCoulombScatteringModel
        double msc_energy_limit = G4EmParameters::Instance()->MscEnergyLimit();

        // Electron
        auto coulomb_scat_electron = std::make_unique<G4CoulombScattering>();
        auto coulomb_model_electron
            = std::make_unique<G4eCoulombScatteringModel>();
        coulomb_scat_electron->SetMinKinEnergy(msc_energy_limit);
        coulomb_model_electron->SetLowEnergyLimit(msc_energy_limit);
        coulomb_model_electron->SetActivationLowEnergyLimit(msc_energy_limit);
        coulomb_scat_electron->SetEmModel(coulomb_model_electron.release());
        physics_list->RegisterProcess(coulomb_scat_electron.release(),
                                      electron);

        // Positron
        auto coulomb_scat_positron = std::make_unique<G4CoulombScattering>();
        auto coulomb_model_positron
            = std::make_unique<G4eCoulombScatteringModel>();
        coulomb_scat_positron->SetMinKinEnergy(msc_energy_limit);
        coulomb_model_positron->SetLowEnergyLimit(msc_energy_limit);
        coulomb_model_positron->SetActivationLowEnergyLimit(msc_energy_limit);
        coulomb_scat_positron->SetEmModel(coulomb_model_positron.release());
        physics_list->RegisterProcess(coulomb_scat_positron.release(),
                                      positron);

        CELER_LOG(info) << "Loaded Coulomb scattering with "
                           "G4eCoulombScatteringModel";
    }

    if (true)
    {
        // Multiple scattering: Urban (low E) and WentzelVI (high E) models
        double msc_energy_limit = G4EmParameters::Instance()->MscEnergyLimit();
        auto   msc_electron     = std::make_unique<G4eMultipleScattering>();
        auto   msc_positron     = std::make_unique<G4eMultipleScattering>();

        if (true)
        {
            // Urban model
            // Electron
            auto urban_msc_electron = std::make_unique<G4UrbanMscModel>();
            urban_msc_electron->SetHighEnergyLimit(msc_energy_limit);
            msc_electron->SetEmModel(urban_msc_electron.release());

            // Positron
            auto urban_msc_positron = std::make_unique<G4UrbanMscModel>();
            urban_msc_positron->SetHighEnergyLimit(msc_energy_limit);
            msc_positron->SetEmModel(urban_msc_positron.release());

            CELER_LOG(info) << "Loaded multiple scattering with "
                               "G4UrbanMscModel";
        }

        // DISABLED
        if (false)
        {
            // WentzelVI model
            // Electron
            auto wentzelvi_msc_electron = std::make_unique<G4WentzelVIModel>();
            wentzelvi_msc_electron->SetLowEnergyLimit(msc_energy_limit);
            msc_electron->SetEmModel(wentzelvi_msc_electron.release());

            // Positron
            auto wentzelvi_msc_positron = std::make_unique<G4WentzelVIModel>();
            wentzelvi_msc_positron->SetLowEnergyLimit(msc_energy_limit);
            msc_positron->SetEmModel(wentzelvi_msc_positron.release());

            CELER_LOG(info) << "Loaded multiple scattering with "
                               "G4WentzelVIModel";
        }

        physics_list->RegisterProcess(msc_electron.release(), electron);
        physics_list->RegisterProcess(msc_positron.release(), positron);
    }
}

//---------------------------------------------------------------------------//
} // namespace geant_exporter
