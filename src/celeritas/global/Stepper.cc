//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/global/Stepper.cc
//---------------------------------------------------------------------------//
#include "Stepper.hh"

#include <type_traits>
#include <utility>

#include "corecel/cont/Range.hh"
#include "corecel/data/Ref.hh"
#include "orange/OrangeData.hh"
#include "celeritas/Types.hh"
#include "celeritas/random/XorwowRngData.hh"
#include "celeritas/track/TrackInitData.hh"
#include "celeritas/track/TrackInitUtils.hh"
#include "celeritas/track/TrackInitParams.hh"

#include "CoreParams.hh"
#include "detail/ActionSequence.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct with problem parameters and setup options.
 */
template<MemSpace M>
Stepper<M>::Stepper(Input input) : params_(std::move(input.params))
{
    CELER_EXPECT(params_);
    CELER_VALIDATE(input.stream_id, << "stream ID is not set");
    CELER_VALIDATE(input.num_track_slots > 0,
                   << "number of track slots is not set");
    states_ = CollectionStateStore<CoreStateData, M>(
        params_->host_ref(), input.stream_id, input.num_track_slots);

    // Create action sequence
    {
        ActionSequence::Options opts;
        opts.sync = input.sync;
        actions_
            = std::make_shared<ActionSequence>(*params_->action_reg(), opts);
    }

    core_ref_.params = get_ref<M>(*params_);
    core_ref_.states = states_.ref();

    CELER_ENSURE(actions_ && *actions_);
}

//---------------------------------------------------------------------------//
//! Default destructor
template<MemSpace M>
Stepper<M>::~Stepper() = default;

//---------------------------------------------------------------------------//
/*!
 * Transport already-initialized states.
 *
 * A single transport step is simply a loop over a toplogically sorted DAG
 * of kernels.
 */
template<MemSpace M>
auto Stepper<M>::operator()() -> result_type
{
    CELER_EXPECT(*this);

    actions_->execute(core_ref_.params, core_ref_.states);

    // Get the number of track initializers and active tracks
    result_type result;
    result.active = core_ref_.states.init.num_active;
    result.alive = states_.size() - core_ref_.states.init.vacancies.size();
    result.queued = core_ref_.states.init.initializers.size();

    return result;
}

//---------------------------------------------------------------------------//
/*!
 * Initialize new primaries and transport them for a single step.
 */
template<MemSpace M>
auto Stepper<M>::operator()(SpanConstPrimary primaries) -> result_type
{
    CELER_EXPECT(*this);
    CELER_EXPECT(!primaries.empty());

    CELER_VALIDATE(primaries.size() + core_ref_.states.init.initializers.size()
                       <= core_ref_.states.init.initializers.capacity(),
                   << "insufficient initializer capacity ("
                   << core_ref_.states.init.initializers.capacity()
                   << ") with size ("
                   << core_ref_.states.init.initializers.size()
                   << ") for primaries (" << primaries.size() << ")");
    auto max_id
        = std::max_element(primaries.begin(),
                           primaries.end(),
                           [](Primary const& left, Primary const& right) {
                               return left.event_id < right.event_id;
                           });
    CELER_VALIDATE(max_id->event_id < params_->init()->max_events(),
                   << "event number " << max_id->event_id.unchecked_get()
                   << " exceeds max_events=" << params_->init()->max_events());

    // Create track initializers
    extend_from_primaries(core_ref_.params, core_ref_.states, primaries);

    return (*this)();
}

//---------------------------------------------------------------------------//
// EXPLICIT INSTANTIATION
//---------------------------------------------------------------------------//

template class Stepper<MemSpace::host>;
template class Stepper<MemSpace::device>;

//---------------------------------------------------------------------------//
}  // namespace celeritas
