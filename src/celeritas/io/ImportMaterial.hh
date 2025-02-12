//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/io/ImportMaterial.hh
//---------------------------------------------------------------------------//
#pragma once

#include <map>
#include <string>
#include <vector>

#include "corecel/Macros.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Enum for storing G4State enumerators.
 */
enum class ImportMaterialState
{
    other,
    not_defined [[deprecated]] = other,
    solid,
    liquid,
    gas,
    size_
};

//---------------------------------------------------------------------------//
/*!
 * Store particle production cut.
 */
struct ImportProductionCut
{
    double energy{};  //!< [MeV]
    double range{};  //!< [cm]
};

//---------------------------------------------------------------------------//
/*!
 * Store elemental composition of a given material.
 */
struct ImportMatElemComponent
{
    unsigned int element_id{};  //!< Index of element in ImportElement
    double mass_fraction{};  //!< [g/cm^3]
    double number_fraction{};
};

//---------------------------------------------------------------------------//
/*!
 * Store material data.
 */
struct ImportMaterial
{
    //!@{
    //! \name Type aliases
    using MapIntCutoff = std::map<int, ImportProductionCut>;
    using VecComponent = std::vector<ImportMatElemComponent>;
    //!@}

    std::string name{};
    ImportMaterialState state{ImportMaterialState::size_};
    double temperature;  //!< [K]
    double density;  //!< [g/cm^3]
    double electron_density;  //!< [1/cm^3]
    double number_density;  //!< [1/cm^3]
    double radiation_length;  //!< [cm]
    double nuclear_int_length;  //!< [cm]
    MapIntCutoff pdg_cutoffs;  //!< Cutoff per PDG
    VecComponent elements;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas
