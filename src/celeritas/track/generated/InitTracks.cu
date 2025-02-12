//---------------------------------*-CUDA-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/track/generated/InitTracks.cu
//! \note Auto-generated by gen-trackinit.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "celeritas/track/detail/InitTracksLauncher.hh"
#include "corecel/device_runtime_api.h"
#include "corecel/sys/KernelParamCalculator.device.hh"
#include "corecel/sys/Device.hh"
#include "corecel/Types.hh"

namespace celeritas
{
namespace generated
{
namespace
{
__global__ void
#if CELERITAS_LAUNCH_BOUNDS
#if CELERITAS_USE_CUDA && (__CUDA_ARCH__ == 700) // Tesla V100-SXM2-16GB
__launch_bounds__(1024, 6)
#endif
#if CELERITAS_USE_HIP && defined(__gfx90a__)
__launch_bounds__(1024, 8)
#endif
#endif // CELERITAS_LAUNCH_BOUNDS
init_tracks_kernel(
    DeviceCRef<CoreParamsData> const core_params,
    DeviceRef<CoreStateData> const core_states,
    size_type const num_vacancies)
{
    auto tid = KernelParamCalculator::thread_id();
    if (!(tid < num_vacancies))
        return;

    detail::InitTracksLauncher<MemSpace::device> launch(core_params, core_states, num_vacancies);
    launch(tid);
}
}  // namespace

void init_tracks(
    DeviceCRef<CoreParamsData> const& core_params,
    DeviceRef<CoreStateData> const& core_states,
    size_type const num_vacancies)
{
    CELER_LAUNCH_KERNEL(
        init_tracks,
        celeritas::device().default_block_size(),
        num_vacancies,
        core_params, core_states, num_vacancies);
}

}  // namespace generated
}  // namespace celeritas
