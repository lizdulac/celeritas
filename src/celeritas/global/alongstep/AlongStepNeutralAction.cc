//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/global/alongstep/AlongStepNeutralAction.cc
//---------------------------------------------------------------------------//
#include "AlongStepNeutralAction.hh"

#include <utility>

#include "corecel/Assert.hh"
#include "corecel/sys/MultiExceptionHandler.hh"
#include "celeritas/Types.hh"
#include "celeritas/global/CoreTrackData.hh"
#include "celeritas/global/KernelContextException.hh"

#include "AlongStepLauncher.hh"
#include "detail/AlongStepNeutral.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct with next action ID.
 */
AlongStepNeutralAction::AlongStepNeutralAction(ActionId id) : id_(id)
{
    CELER_EXPECT(id_);
}

//---------------------------------------------------------------------------//
/*!
 * Launch the along-step action on host.
 */
void AlongStepNeutralAction::execute(ParamsHostCRef const& params,
                                     StateHostRef& state) const
{
    CELER_EXPECT(params && state);

    MultiExceptionHandler capture_exception;
    auto launch = make_along_step_launcher(
        params, state, NoData{}, NoData{}, NoData{}, detail::along_step_neutral);
#pragma omp parallel for
    for (size_type i = 0; i < state.size(); ++i)
    {
        CELER_TRY_HANDLE_CONTEXT(
            launch(ThreadId{i}),
            capture_exception,
            KernelContextException(params, state, ThreadId{i}, this->label()));
    }
    log_and_rethrow(std::move(capture_exception));
}

//---------------------------------------------------------------------------//
#if !CELER_USE_DEVICE
void AlongStepNeutralAction::execute(ParamsDeviceCRef const&,
                                     StateDeviceRef&) const
{
    CELER_NOT_CONFIGURED("CUDA OR HIP");
}
#endif

//---------------------------------------------------------------------------//
}  // namespace celeritas
