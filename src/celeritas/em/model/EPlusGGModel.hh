//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/em/model/EPlusGGModel.hh
//---------------------------------------------------------------------------//
#pragma once

#include "celeritas/em/data/EPlusGGData.hh"
#include "celeritas/phys/Model.hh"
#include "celeritas/phys/ParticleParams.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Set up and launch two-gamma positron annihiliation.
 */
class EPlusGGModel final : public Model
{
  public:
    // Construct from model ID and other necessary data
    EPlusGGModel(ActionId id, ParticleParams const& particles);

    // Particle types and energy ranges that this model applies to
    SetApplicability applicability() const final;

    // Get the microscopic cross sections for the given particle and material
    MicroXsBuilders micro_xs(Applicability) const final;

    // Apply the interaction kernel on host
    void execute(ParamsHostCRef const&, StateHostRef&) const final;

    // Apply the interaction kernel on device
    void execute(ParamsDeviceCRef const&, StateDeviceRef&) const final;

    // ID of the model
    ActionId action_id() const final;

    //! Short name for the interaction kernel
    std::string label() const final { return "annihil-2-gamma"; }

    //! Name of the model, for user interaction
    std::string description() const final
    {
        return "Positron annihilation yielding two gammas";
    }

    // Access data on device
    EPlusGGData device_ref() const { return data_; }

  private:
    EPlusGGData data_;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas
