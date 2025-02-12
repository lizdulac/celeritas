//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/mat/MaterialParamsOutput.cc
//---------------------------------------------------------------------------//
#include "MaterialParamsOutput.hh"

#include <utility>

#include "celeritas_config.h"
#include "corecel/Assert.hh"
#include "corecel/cont/Range.hh"
#include "corecel/io/JsonPimpl.hh"
#include "corecel/math/Quantity.hh"
#include "celeritas/Types.hh"

#include "MaterialParams.hh"  // IWYU pragma: keep

#if CELERITAS_USE_JSON
#    include <nlohmann/json.hpp>

#    include "corecel/cont/LabelIO.json.hh"
#endif

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct from shared geometry data.
 */
MaterialParamsOutput::MaterialParamsOutput(SPConstMaterialParams geo)
    : material_(std::move(geo))
{
    CELER_EXPECT(material_);
}

//---------------------------------------------------------------------------//
/*!
 * Write output to the given JSON object.
 */
void MaterialParamsOutput::output(JsonPimpl* j) const
{
#if CELERITAS_USE_JSON
    using json = nlohmann::json;

    auto obj = json::object();
    auto units = json::object();

    // Unfold elements
    {
        auto label = json::array();
        auto atomic_number = json::array();
        auto atomic_mass = json::array();
        auto coulomb_correction = json::array();
        auto mass_radiation_coeff = json::array();

        for (auto id : range(ElementId{material_->num_elements()}))
        {
            ElementView const el_view = material_->get(id);
            label.push_back(material_->id_to_label(id));
            atomic_number.push_back(el_view.atomic_number().unchecked_get());
            atomic_mass.push_back(el_view.atomic_mass().value());
            coulomb_correction.push_back(el_view.coulomb_correction());
            mass_radiation_coeff.push_back(el_view.mass_radiation_coeff());
        }
        obj["elements"] = {
            {"label", std::move(label)},
            {"atomic_number", std::move(atomic_number)},
            {"atomic_mass", std::move(atomic_mass)},
            {"coulomb_correction", std::move(coulomb_correction)},
            {"mass_radiation_coeff", std::move(mass_radiation_coeff)},
        };
        units["atomic_mass"]
            = accessor_unit_label<decltype(&ElementView::atomic_mass)>();
    }

    // Unfold materials
    {
        auto label = json::array();
        auto number_density = json::array();
        auto temperature = json::array();
        auto matter_state = json::array();

        auto zeff = json::array();
        auto density = json::array();
        auto electron_density = json::array();
        auto radiation_length = json::array();
        auto mean_excitation_energy = json::array();

        auto element_id = json::array();
        auto element_frac = json::array();

        for (auto id : range(MaterialId{material_->num_materials()}))
        {
            MaterialView const mat_view = material_->get(id);
            label.push_back(material_->id_to_label(id));
            number_density.push_back(mat_view.number_density());
            temperature.push_back(mat_view.temperature());
            matter_state.push_back(to_cstring(mat_view.matter_state()));

            // Save derivative data
            zeff.push_back(mat_view.zeff());
            density.push_back(mat_view.density());
            electron_density.push_back(mat_view.electron_density());
            radiation_length.push_back(mat_view.radiation_length());
            mean_excitation_energy.push_back(
                mat_view.mean_excitation_energy().value());

            // Save element ids and fractions
            auto elids = json::array();
            auto elfrac = json::array();
            for (MatElementComponent const& el : mat_view.elements())
            {
                elids.push_back(el.element.unchecked_get());
                elfrac.push_back(el.fraction);
            }
            element_id.push_back(std::move(elids));
            element_frac.push_back(std::move(elfrac));
        }
        obj["materials"] = {
            {"label", std::move(label)},
            {"number_density", std::move(number_density)},
            {"temperature", std::move(temperature)},
            {"matter_state", std::move(matter_state)},
            {"element_id", std::move(element_id)},
            {"element_frac", std::move(element_frac)},
            {"zeff", std::move(zeff)},
            {"density", std::move(density)},
            {"electron_density", std::move(electron_density)},
            {"radiation_length", std::move(radiation_length)},
            {"mean_excitation_energy", std::move(mean_excitation_energy)},
        };
        {
            units["mean_excitation_energy"] = accessor_unit_label<
                decltype(&MaterialView::mean_excitation_energy)>();
        }
    }

    obj["_units"] = std::move(units);
    j->obj = std::move(obj);
#else
    (void)sizeof(j);
#endif
}

//---------------------------------------------------------------------------//
}  // namespace celeritas
