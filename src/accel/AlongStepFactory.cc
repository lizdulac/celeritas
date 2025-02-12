//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file accel/AlongStepFactory.cc
//---------------------------------------------------------------------------//
#include "AlongStepFactory.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include "corecel/io/Logger.hh"
#include "corecel/math/ArrayUtils.hh"
#include "celeritas/em/UrbanMscParams.hh"
#include "celeritas/field/UniformFieldData.hh"
#include "celeritas/global/alongstep/AlongStepGeneralLinearAction.hh"
#include "celeritas/global/alongstep/AlongStepUniformMscAction.hh"
#include "celeritas/io/ImportData.hh"

#include "detail/Convert.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct with a function to return the field strength.
 *
 * The function is evaluated whenever Celeritas is set up (which is after
 * Geant4 physics is initialized).
 */
UniformAlongStepFactory::UniformAlongStepFactory(FieldFunction f)
    : get_field_(std::move(f))
{
    CELER_EXPECT(get_field_);
}

//---------------------------------------------------------------------------//
/*!
 * Emit an along-step action.
 *
 * The action will embed the linear propagator if the magnetic field strength
 * is zero (or the accessor is unset).
 */
auto UniformAlongStepFactory::operator()(AlongStepFactoryInput const& input) const
    -> result_type
{
    // Get the field strength in tesla (or zero if accessor is undefined)
    Real3 field = get_field_
                      ? detail::convert_from_geant(get_field_(), CLHEP::tesla)
                      : Real3{0, 0, 0};
    real_type magnitude_tesla = norm(field);

    if (magnitude_tesla > 0)
    {
        // Create a uniform field
        if (input.imported->em_params.energy_loss_fluct)
        {
            CELER_LOG(error)
                << "Magnetic field with energy loss fluctuations is "
                   "not currently supported: using mean energy loss";
        }

        // Convert field units from tesla to native celeritas units
        for (real_type& v : field)
        {
            v /= units::tesla;
        }

        UniformFieldParams field_params;
        field_params.field = field;
        CELER_LOG(info) << "Creating along-step action with field strength "
                        << magnitude_tesla << "T";
        return std::make_shared<AlongStepUniformMscAction>(
            input.action_id,
            field_params,
            UrbanMscParams::from_import(
                *input.particle, *input.material, *input.imported));
    }
    else
    {
        CELER_LOG(info) << "Creating along-step action with no field";
        return celeritas::AlongStepGeneralLinearAction::from_params(
            input.action_id,
            *input.material,
            *input.particle,
            celeritas::UrbanMscParams::from_import(
                *input.particle, *input.material, *input.imported),
            input.imported->em_params.energy_loss_fluct);
    }
}

//---------------------------------------------------------------------------//
}  // namespace celeritas
