//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/mat/MaterialData.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Types.hh"
#include "corecel/cont/Range.hh"
#include "corecel/data/Collection.hh"
#include "corecel/data/CollectionBuilder.hh"
#include "celeritas/Quantities.hh"
#include "celeritas/Types.hh"
#include "celeritas/phys/AtomicNumber.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
// PARAMS
//---------------------------------------------------------------------------//
/*!
 * Fundamental, invariant properties of an element.
 *
 * Add elemental properties as needed if they apply to more than one physics
 * model. (When nuclear physics is implemented, add isotopes.)
 *
 * Note that more than one "element def" can exist for a single atomic number:
 * there might be different enrichments of an element in the problem.
 */
struct ElementRecord
{
    AtomicNumber atomic_number;  //!< Z number
    units::AmuMass atomic_mass;  //!< Isotope-weighted average atomic mass

    // COMPUTED PROPERTIES

    real_type cbrt_z = 0;  //!< Z^{1/3}
    real_type cbrt_zzp = 0;  //!< (Z (Z + 1))^{1/3}
    real_type log_z = 0;  //!< log Z

    real_type coulomb_correction = 0;  //!< f(Z)
    real_type mass_radiation_coeff = 0;  //!< 1/X_0 (bremsstrahlung)
};

//---------------------------------------------------------------------------//
/*!
 * Fractional element component of a material.
 *
 * This represents, e.g., the fraction of hydrogen in water.
 */
struct MatElementComponent
{
    ElementId element;  //!< Index in MaterialParams elements
    real_type fraction;  //!< Fraction of number density
};

//---------------------------------------------------------------------------//
/*!
 * Fundamental (static) properties of a material.
 *
 * Multiple material definitions are allowed to reuse a single element
 * definition vector (memory management from the params store should handle
 * this). Derivative properties such as electron_density are calculated from
 * the elemental components.
 */
struct MaterialRecord
{
    real_type number_density;  //!< Atomic number density [1/cm^3]
    real_type temperature;  //!< Temperature [K]
    MatterState matter_state;  //!< Solid, liquid, gas
    ItemRange<MatElementComponent> elements;  //!< Element components

    // COMPUTED PROPERTIES

    real_type zeff;  //!< Weighted atomic number
    real_type density;  //!< Density [g/cm^3]
    real_type electron_density;  //!< Electron number density [1/cm^3]
    real_type rad_length;  //!< Radiation length [cm]
    units::MevEnergy mean_exc_energy;  //!< Mean excitation energy [MeV]
    units::LogMevEnergy log_mean_exc_energy;  //!< Log mean excitation energy
};

//---------------------------------------------------------------------------//
/*!
 * Access material properties on the device.
 *
 * This view is created from \c MaterialParams.
 *
 * \sa MaterialParams (owns the pointed-to data)
 * \sa ElementView (uses the pointed-to element data in a kernel)
 * \sa MaterialView (uses the pointed-to material data in a kernel)
 */
template<Ownership W, MemSpace M>
struct MaterialParamsData
{
    template<class T>
    using Items = celeritas::Collection<T, W, M>;

    Items<ElementRecord> elements;
    Items<MatElementComponent> elcomponents;
    Items<MaterialRecord> materials;
    ElementComponentId::size_type max_element_components{};

    //// MEMBER FUNCTIONS ////

    //! Whether the data is assigned
    explicit CELER_FUNCTION operator bool() const
    {
        return !materials.empty();
    }

    //! Assign from another set of data
    template<Ownership W2, MemSpace M2>
    MaterialParamsData& operator=(MaterialParamsData<W2, M2> const& other)
    {
        CELER_EXPECT(other);
        elements = other.elements;
        elcomponents = other.elcomponents;
        materials = other.materials;
        max_element_components = other.max_element_components;
        return *this;
    }
};

//---------------------------------------------------------------------------//
// STATE
//---------------------------------------------------------------------------//
/*!
 * Dynamic material state of a particle track.
 */
struct MaterialTrackState
{
    MaterialId material_id;  //!< Current material being tracked
};

//---------------------------------------------------------------------------//
/*!
 * Store dynamic states of multiple physical particles.
 *
 * The "element scratch space" is a 2D array of reals, indexed with
 * [trackslot_id][el_component_id], where the fast-moving dimension has the
 * greatest number of element components of any material in the problem. This
 * can be used for the physics to calculate microscopic cross sections.
 *
 * \sa MaterialStateStore (owns the pointed-to data)
 * \sa MaterialTrackView (uses the pointed-to data in a kernel)
 */
template<Ownership W, MemSpace M>
struct MaterialStateData
{
    template<class T>
    using Items = celeritas::StateCollection<T, W, M>;

    Items<MaterialTrackState> state;
    Items<real_type> element_scratch;  // 2D array: [num states][max
                                       // components]

    //! Whether the interface is assigned
    explicit CELER_FUNCTION operator bool() const { return !state.empty(); }

    //! State size
    CELER_FUNCTION size_type size() const { return state.size(); }

    //! Assign from another set of data
    template<Ownership W2, MemSpace M2>
    MaterialStateData& operator=(MaterialStateData<W2, M2>& other)
    {
        CELER_EXPECT(other);
        state = other.state;
        element_scratch = other.element_scratch;
        return *this;
    }
};

//---------------------------------------------------------------------------//
/*!
 * Resize a material state in host code.
 */
template<MemSpace M>
inline void resize(MaterialStateData<Ownership::value, M>* data,
                   HostCRef<MaterialParamsData> const& params,
                   size_type size)
{
    CELER_EXPECT(size > 0);
    resize(&data->state, size);
    resize(&data->element_scratch, size * params.max_element_components);
}

//---------------------------------------------------------------------------//
}  // namespace celeritas
