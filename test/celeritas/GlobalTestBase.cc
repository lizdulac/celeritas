//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/GlobalTestBase.cc
//---------------------------------------------------------------------------//
#include "GlobalTestBase.hh"

#include <iostream>
#include <fstream>

#include "celeritas_config.h"
#include "corecel/io/JsonPimpl.hh"
#include "corecel/io/Logger.hh"
#include "corecel/io/OutputRegistry.hh"
#include "celeritas/geo/GeoParams.hh"
#include "celeritas/global/ActionRegistry.hh"
#include "celeritas/global/ActionRegistryOutput.hh"
#include "celeritas/global/CoreParams.hh"
#include "celeritas/phys/PhysicsParamsOutput.hh"
#include "celeritas/random/RngParams.hh"
#if CELERITAS_USE_JSON
#    include <nlohmann/json.hpp>
#endif

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
GlobalTestBase::GlobalTestBase()
{
    output_reg_ = std::make_shared<OutputRegistry>();
}

//---------------------------------------------------------------------------//
GlobalTestBase::~GlobalTestBase()
{
    if (this->HasFailure() && !this->output_reg()->empty())
    {
        std::cerr << "Writing diagnostic output to screen because test failed:\n";
        this->write_output(std::cout);
        std::cout << std::endl;
    }
}

//---------------------------------------------------------------------------//
auto GlobalTestBase::build_rng() const -> SPConstRng
{
    return std::make_shared<RngParams>(20220511);
}

//---------------------------------------------------------------------------//
auto GlobalTestBase::build_action_reg() const -> SPActionRegistry
{
    return std::make_shared<ActionRegistry>();
}

//---------------------------------------------------------------------------//
auto GlobalTestBase::build_core() -> SPConstCore
{
    CoreParams::Input inp;
    inp.geometry = this->geometry();
    inp.material = this->material();
    inp.geomaterial = this->geomaterial();
    inp.particle = this->particle();
    inp.cutoff = this->cutoff();
    inp.physics = this->physics();
    inp.rng = this->rng();
    inp.sim = this->sim();
    inp.init = this->init();
    inp.action_reg = this->action_reg();
    inp.output_reg = this->output_reg();
    CELER_ASSERT(inp);

    // Build along-step action to add to the stepping loop
    auto&& along_step = this->along_step();
    CELER_ASSERT(along_step);

    return std::make_shared<CoreParams>(std::move(inp));
}

//---------------------------------------------------------------------------//
void GlobalTestBase::write_output()
{
    if (!CELERITAS_USE_JSON)
    {
        CELER_LOG(error) << "JSON unavailable: cannot write output";
        return;
    }
    std::string filename = this->make_unique_filename(".json");
    std::ofstream of(filename);
    this->write_output(of);
    CELER_LOG(info) << "Wrote output to " << filename;
}

//---------------------------------------------------------------------------//
void GlobalTestBase::write_output(std::ostream& os) const
{
#if CELERITAS_USE_JSON
    JsonPimpl json_wrap;
    this->output_reg()->output(&json_wrap);

    // Print with pretty indentation
    os << json_wrap.obj.dump(1) << '\n';
#else
    os << "\"output unavailable\"";
#endif
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas
