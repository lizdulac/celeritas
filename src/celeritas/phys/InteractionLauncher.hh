//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/phys/InteractionLauncher.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/math/Algorithms.hh"
#include "celeritas/global/CoreTrackDataFwd.hh"

#include "detail/InteractionLauncherImpl.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Return a function-like class to launch a "Track"-dependent function.
 *
 * This function should be used exclusively by generated interaction functions.
 * The author of a new model needs to define (in a header file) an inline
 * function that accepts a CoreTrackView and a const reference to "native"
 * model data, and returns an Interaction. It is a given that the interaction
 * *does* apply to the given track.
 *
 * \code
inline CELER_FUNCTION Interaction foo_interact(
    NativeCRef<FooModelData> const& data,
    celeritas::CoreTrackView const& track)
{
    // ...
}
   \endcode
 *
 * \note The model data *must* have a member data `ActionId action_id;` for
 * filtering the tracks. We could improve this interface later.
 */
template<class D, class F>
CELER_FUNCTION detail::InteractionLauncherImpl<D, F>
make_interaction_launcher(NativeCRef<CoreParamsData> const& params,
                          NativeRef<CoreStateData> const& states,
                          D const& model_data,
                          F&& call_with_track)
{
    return {
        params, states, model_data, ::celeritas::forward<F>(call_with_track)};
}

//---------------------------------------------------------------------------//
}  // namespace celeritas
