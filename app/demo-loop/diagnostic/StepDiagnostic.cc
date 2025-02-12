//----------------------------------*-C++-*----------------------------------//
// Copyright 2021-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file demo-loop/diagnostic/StepDiagnostic.cc
//---------------------------------------------------------------------------//
#include "StepDiagnostic.hh"

using namespace celeritas;

namespace demo_loop
{
//---------------------------------------------------------------------------//
/*!
 * Count the steps per track for each particle type.
 */
void count_steps(HostCRef<CoreParamsData> const& params,
                 HostRef<CoreStateData> const& states,
                 StepDiagnosticDataRef<MemSpace::host> data)
{
    StepLauncher<MemSpace::host> launch(params, states, data);
    for (auto tid : range(TrackSlotId{states.size()}))
    {
        launch(tid);
    }
}
}  // namespace demo_loop
