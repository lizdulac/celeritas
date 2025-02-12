//---------------------------------*-CUDA-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/track/generated/ProcessSecondaries.cu
//! \note Auto-generated by gen-trackinit.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "celeritas/track/detail/ProcessSecondariesLauncher.hh"
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
__launch_bounds__(1024, 5)
#endif
#if CELERITAS_USE_HIP && defined(__gfx90a__)
__launch_bounds__(1024, 2)
#endif
#endif // CELERITAS_LAUNCH_BOUNDS
process_secondaries_kernel(
    DeviceCRef<CoreParamsData> const core_params,
    DeviceRef<CoreStateData> const core_states)
{
    auto tid = KernelParamCalculator::thread_id();
    if (!(tid < core_states.size()))
        return;

    detail::ProcessSecondariesLauncher<MemSpace::device> launch(core_params, core_states);
    launch(tid);
}
}  // namespace

void process_secondaries(
    DeviceCRef<CoreParamsData> const& core_params,
    DeviceRef<CoreStateData> const& core_states)
{
    CELER_LAUNCH_KERNEL(
        process_secondaries,
        celeritas::device().default_block_size(),
        core_states.size(),
        core_params, core_states);
}

}  // namespace generated
}  // namespace celeritas
