//----------------------------------*-C++-*----------------------------------//
// Copyright 2021-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/em/generated/SeltzerBergerInteract.hh
//! \note Auto-generated by gen-interactor.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"
#include "corecel/Types.hh"
#include "celeritas/em/data/SeltzerBergerData.hh" // IWYU pragma: associated
#include "celeritas/global/CoreTrackDataFwd.hh"

namespace celeritas
{
namespace generated
{
void seltzer_berger_interact(
    celeritas::SeltzerBergerHostRef const&,
    celeritas::HostCRef<celeritas::CoreParamsData> const&,
    celeritas::HostRef<celeritas::CoreStateData>&);

void seltzer_berger_interact(
    celeritas::SeltzerBergerDeviceRef const&,
    celeritas::DeviceCRef<celeritas::CoreParamsData> const&,
    celeritas::DeviceRef<celeritas::CoreStateData>&);

#if !CELER_USE_DEVICE
inline void seltzer_berger_interact(
    celeritas::SeltzerBergerDeviceRef const&,
    celeritas::DeviceCRef<celeritas::CoreParamsData> const&,
    celeritas::DeviceRef<celeritas::CoreStateData>&)
{
    CELER_ASSERT_UNREACHABLE();
}
#endif

}  // namespace generated
}  // namespace celeritas
