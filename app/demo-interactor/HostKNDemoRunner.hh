//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file demo-interactor/HostKNDemoRunner.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>

#include "corecel/Types.hh"
#include "celeritas/em/data/KleinNishinaData.hh"
#include "celeritas/phys/ParticleData.hh"
#include "celeritas/phys/ParticleParams.hh"

#include "KNDemoIO.hh"
#include "XsGridParams.hh"

namespace demo_interactor
{
//---------------------------------------------------------------------------//
/*!
 * Run interactions on the host CPU.
 *
 * This is an analog to the demo_interactor::KNDemoRunner for device simulation
 * but does all the transport directly on the CPU side.
 */
class HostKNDemoRunner
{
  public:
    //!@{
    //! \name Type aliases
    using size_type = celeritas::size_type;
    using result_type = demo_interactor::KNDemoResult;
    using constSPParticleParams
        = std::shared_ptr<const celeritas::ParticleParams>;
    using constSPXsGridParams = std::shared_ptr<XsGridParams const>;
    //!@}

  public:
    // Construct with parameters
    HostKNDemoRunner(constSPParticleParams particles, constSPXsGridParams xs);

    // Run given number of particles
    result_type operator()(demo_interactor::KNDemoRunArgs args);

  private:
    constSPParticleParams pparams_;
    constSPXsGridParams xsparams_;
    celeritas::KleinNishinaData kn_data_;
};

//---------------------------------------------------------------------------//
}  // namespace demo_interactor
