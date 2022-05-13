//---------------------------------*-CUDA-*----------------------------------//
// Copyright 2021-2022 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/em/generated/RelativisticBremInteract.cu
//! \note Auto-generated by gen-interactor.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "RelativisticBremInteract.hh"

#include "corecel/device_runtime_api.h"
#include "corecel/Assert.hh"
#include "corecel/Types.hh"
#include "corecel/sys/KernelParamCalculator.device.hh"
#include "corecel/sys/Device.hh"
#include "celeritas/em/launcher/RelativisticBremLauncher.hh"
#include "celeritas/phys/InteractionLauncher.hh"

using celeritas::MemSpace;

namespace celeritas
{
namespace generated
{
namespace
{
__global__ void relativistic_brem_interact_kernel(
    const celeritas::RelativisticBremDeviceRef model_data,
    const celeritas::CoreRef<MemSpace::device> core_data)
{
    auto tid = celeritas::KernelParamCalculator::thread_id();
    if (!(tid < core_data.states.size()))
        return;

    auto launch = celeritas::make_interaction_launcher(
        core_data,
        model_data,
        celeritas::relativistic_brem_interact_track);
    launch(tid);
}
} // namespace

void relativistic_brem_interact(
    const celeritas::RelativisticBremDeviceRef& model_data,
    const celeritas::CoreRef<MemSpace::device>& core_data)
{
    CELER_EXPECT(core_data);
    CELER_EXPECT(model_data);

    CELER_LAUNCH_KERNEL(relativistic_brem_interact,
                        celeritas::device().default_block_size(),
                        core_data.states.size(),
                        model_data, core_data);
}

} // namespace generated
} // namespace celeritas
