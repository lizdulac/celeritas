//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/track/generated/InitTracks.cc
//! \note Auto-generated by gen-trackinit.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include <utility>

#include "corecel/sys/MultiExceptionHandler.hh"
#include "corecel/sys/ThreadId.hh"
#include "corecel/Types.hh"
#include "celeritas/global/KernelContextException.hh"
#include "celeritas/track/detail/InitTracksLauncher.hh" // IWYU pragma: associated

namespace celeritas
{
namespace generated
{
void init_tracks(
    HostCRef<CoreParamsData> const& core_params,
    HostRef<CoreStateData> const& core_states,
    size_type const num_vacancies)
{
    MultiExceptionHandler capture_exception;
    detail::InitTracksLauncher<MemSpace::host> launch(core_params, core_states, num_vacancies);
    #pragma omp parallel for
    for (ThreadId::size_type i = 0; i < num_vacancies; ++i)
    {
        CELER_TRY_HANDLE_CONTEXT(
            launch(ThreadId{i}),
            capture_exception,
            KernelContextException(core_params, core_states, ThreadId{i}, "init_tracks"));
    }
    log_and_rethrow(std::move(capture_exception));
}

}  // namespace generated
}  // namespace celeritas
