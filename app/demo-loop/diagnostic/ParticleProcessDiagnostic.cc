//----------------------------------*-C++-*----------------------------------//
// Copyright 2021-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file demo-loop/diagnostic/ParticleProcessDiagnostic.cc
//---------------------------------------------------------------------------//
#include "ParticleProcessDiagnostic.hh"

using namespace celeritas;

namespace demo_loop
{
//---------------------------------------------------------------------------//
/*!
 * Tally the particle/process combinations that occur at each step.
 */
void count_particle_process(
    HostCRef<CoreParamsData> const& params,
    HostRef<CoreStateData> const& states,
    ParticleProcessLauncher<MemSpace::host>::ItemsRef counts)
{
    ParticleProcessLauncher<MemSpace::host> launch(params, states, counts);
    for (auto tid : range(TrackSlotId{states.size()}))
    {
        launch(tid);
    }
}
}  // namespace demo_loop
