//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/global/AlongStep.test.cc
//---------------------------------------------------------------------------//
#include "celeritas/SimpleCmsTestBase.hh"
#include "celeritas/TestEm3Base.hh"
#include "celeritas/em/UrbanMscParams.hh"
#include "celeritas/ext/GeantPhysicsOptions.hh"
#include "celeritas/field/UniformFieldData.hh"
#include "celeritas/global/ActionRegistry.hh"
#include "celeritas/global/alongstep/AlongStepUniformMscAction.hh"
#include "celeritas/phys/PDGNumber.hh"
#include "celeritas/phys/ParticleParams.hh"

#include "../MockTestBase.hh"
#include "../SimpleTestBase.hh"
#include "AlongStepTestBase.hh"
#include "celeritas_test.hh"

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class KnAlongStepTest : public SimpleTestBase, public AlongStepTestBase
{
};

class MockAlongStepTest : public MockTestBase, public AlongStepTestBase
{
};

#define Em3AlongStepTest TEST_IF_CELERITAS_GEANT(Em3AlongStepTest)
class Em3AlongStepTest : public TestEm3Base, public AlongStepTestBase
{
  public:
    GeantPhysicsOptions build_geant_options() const override
    {
        auto opts = TestEm3Base::build_geant_options();
        opts.em_bins_per_decade = bpd_;
        opts.eloss_fluctuation = fluct_;
        opts.msc = msc_ ? MscModelSelection::urban : MscModelSelection::none;
        return opts;
    }

    size_type bpd_{14};
    bool msc_{false};
    bool fluct_{true};
};

#define SimpleCmsAlongStepTest TEST_IF_CELERITAS_GEANT(SimpleCmsAlongStepTest)
class SimpleCmsAlongStepTest : public SimpleCmsTestBase,
                               public AlongStepTestBase
{
  public:
    GeantPhysicsOptions build_geant_options() const override
    {
        auto opts = SimpleCmsTestBase::build_geant_options();
        opts.em_bins_per_decade = bpd_;
        opts.eloss_fluctuation = fluct_;
        opts.msc = msc_ ? MscModelSelection::urban : MscModelSelection::none;
        return opts;
    }

    SPConstAction build_along_step() override
    {
        auto& action_reg = *this->action_reg();
        UniformFieldParams field_params;
        field_params.field = {0, 0, 1 * units::tesla};

        auto msc = UrbanMscParams::from_import(
            *this->particle(), *this->material(), this->imported_data());
        CELER_ASSERT(msc);

        auto result = std::make_shared<AlongStepUniformMscAction>(
            action_reg.next_id(), field_params, msc);
        action_reg.insert(result);
        return result;
    }

    size_type bpd_{14};
    bool msc_{true};
    bool fluct_{false};
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

TEST_F(KnAlongStepTest, basic)
{
    size_type num_tracks = 10;
    Input inp;
    inp.particle_id = this->particle()->find(pdg::gamma());
    {
        inp.energy = MevEnergy{1};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(0, result.eloss);
        EXPECT_SOFT_EQ(1, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_EQ(3.3356409519815202e-11, result.time);
        EXPECT_SOFT_EQ(1, result.step);
        EXPECT_EQ("physics-discrete-select", result.action);
    }
    {
        inp.energy = MevEnergy{10};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(0, result.eloss);
        EXPECT_SOFT_EQ(5, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_EQ(1.6678204759908e-10, result.time);
        EXPECT_SOFT_EQ(5, result.step);
        EXPECT_EQ("geo-boundary", result.action);
    }
    {
        inp.energy = MevEnergy{10};
        inp.phys_mfp = 1e-4;
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(0, result.eloss);
        EXPECT_SOFT_EQ(0.0010008918838569024, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_EQ(3.3386159562990149e-14, result.time);
        EXPECT_SOFT_EQ(0.0010008918838569024, result.step);
        EXPECT_EQ("physics-discrete-select", result.action);
    }
}

TEST_F(MockAlongStepTest, basic)
{
    size_type num_tracks = 10;
    Input inp;
    inp.particle_id = this->particle()->find("celeriton");
    {
        inp.energy = MevEnergy{1};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(0.29312, result.eloss);
        EXPECT_SOFT_EQ(0.48853333333333, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_EQ(1.881667426791e-11, result.time);
        EXPECT_SOFT_EQ(0.48853333333333, result.step);
        EXPECT_EQ("eloss-range", result.action);
    }
    {
        inp.energy = MevEnergy{1e-6};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(1e-06, result.eloss);
        EXPECT_SOFT_EQ(5.2704627669473e-05, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_NEAR(1.2431209185653e-12, result.time, 1e-11);
        EXPECT_SOFT_EQ(5.2704627669473e-05, result.step);
        EXPECT_EQ("physics-discrete-select", result.action);
    }
    {
        inp.energy = MevEnergy{1e-12};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(1e-12, result.eloss);
        EXPECT_SOFT_EQ(5.2704627669473e-08, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_EQ(1.2430647328325e-12, result.time);
        EXPECT_SOFT_EQ(5.2704627669473e-08, result.step);
        EXPECT_EQ("physics-discrete-select", result.action);
    }
    {
        inp.energy = MevEnergy{1e-18};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(5.2704627669473e-11, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_EQ(0, result.time);
        EXPECT_SOFT_EQ(5.2704627669473e-11, result.step);
        EXPECT_EQ("physics-discrete-select", result.action);
    }
}

TEST_F(Em3AlongStepTest, nofluct_nomsc)
{
    msc_ = false;
    fluct_ = false;

    size_type num_tracks = 128;
    Input inp;
    {
        SCOPED_TRACE("low energy electron");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{1};

        {
            SCOPED_TRACE("far from boundary");
            inp.position = {0.0 - 0.25};
            inp.direction = {0, 1, 0};
            auto result = this->run(inp, num_tracks);
            EXPECT_SOFT_NEAR(0.44074534601915, result.eloss, 5e-4);
            EXPECT_SOFT_NEAR(0.22820529792233, result.displacement, 5e-4);
            EXPECT_SOFT_EQ(1, result.angle);
            EXPECT_SOFT_NEAR(8.0887018802006e-12, result.time, 5e-4);
            EXPECT_SOFT_NEAR(0.22820529792233, result.step, 5e-4);
            EXPECT_EQ("eloss-range", result.action);
        }
        {
            SCOPED_TRACE("very near (1um) boundary");
            inp.position = {0.0 - 1e-4};
            inp.direction = {1, 0, 0};
            auto result = this->run(inp, num_tracks);
            EXPECT_SOFT_NEAR(0.00018631642554801, result.eloss, 5e-4);
            EXPECT_SOFT_EQ(0.0001, result.displacement);
            EXPECT_SOFT_EQ(1, result.angle);
            EXPECT_SOFT_NEAR(3.5444847047126e-15, result.time, 5e-4);
            EXPECT_SOFT_EQ(0.0001, result.step);
            EXPECT_EQ("geo-boundary", result.action);
        }
    }
    {
        SCOPED_TRACE("very low energy electron");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{0.01};
        real_type const macro_xs{0.49509299806197};
        real_type const range_limit{0.00028364048015169};

        {
            SCOPED_TRACE("far from boundary");
            inp.position = {0.0 - 0.25};
            inp.direction = {0, 1, 0};

            // Using the calculated macro XS, stop *just* before we hit the
            // range
            inp.phys_mfp = range_limit * macro_xs * (1 - 1e-5);

            auto result = this->run(inp, num_tracks);
            EXPECT_SOFT_EQ(0.01, result.eloss);
            EXPECT_SOFT_EQ(0.00028363764374689, result.displacement);
            EXPECT_SOFT_EQ(1, result.angle);
            EXPECT_SOFT_EQ(4.8522211972805e-14, result.time);
            EXPECT_SOFT_EQ(0.00028363764374689, result.step);
            EXPECT_EQ("physics-discrete-select", result.action);
        }
        {
            SCOPED_TRACE("near boundary");
            inp.particle_id = this->particle()->find(pdg::electron());
            inp.energy = MevEnergy{0.01};

            real_type step = range_limit * (1 - 1e-5);
            inp.position = {0.0 - step};
            inp.direction = {1, 0, 0};
            inp.phys_mfp = 100;

            auto result = this->run(inp, num_tracks);
            EXPECT_SOFT_EQ(0.0099999992401263, result.eloss);
            EXPECT_SOFT_EQ(0.00028363764374689, result.displacement);
            EXPECT_SOFT_EQ(1, result.angle);
            EXPECT_SOFT_EQ(4.8522211972805e-14, result.time);
            EXPECT_SOFT_EQ(step, result.step);
        }
    }
}

TEST_F(Em3AlongStepTest, msc_nofluct)
{
    msc_ = true;
    fluct_ = false;

    size_type num_tracks = 1024;
    Input inp;
    {
        SCOPED_TRACE("electron far from boundary");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{10};
        inp.position = {0.0 - 0.25};
        inp.direction = {0, 1, 0};
        inp.phys_mfp = 100;
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_NEAR(2.2870403276278, result.eloss, 5e-4);
        EXPECT_SOFT_NEAR(1.1622519442871, result.displacement, 5e-4);
        EXPECT_SOFT_NEAR(0.85325942256503251, result.angle, 1e-3);
        EXPECT_SOFT_NEAR(4.083585865972e-11, result.time, 1e-5);
        EXPECT_SOFT_NEAR(1.222780668781, result.step, 5e-4);
        EXPECT_EQ("eloss-range", result.action);
    }
    {
        SCOPED_TRACE("low energy electron far from boundary");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{1};
        inp.position = {0.0 - 0.25};
        inp.direction = {1, 0, 0};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_NEAR(0.28579817262705, result.eloss, 5e-4);
        EXPECT_SOFT_NEAR(0.13028709259427, result.displacement, 5e-4);
        EXPECT_SOFT_NEAR(0.42060290539404, result.angle, 1e-3);
        EXPECT_SOFT_EQ(5.3240431819014e-12, result.time);
        EXPECT_SOFT_EQ(0.1502064087009, result.step);
        EXPECT_EQ("msc-range", result.action);
    }
    {
        SCOPED_TRACE("electron very near (1um) boundary");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{10};
        inp.position = {0.0 - 1e-4};
        inp.direction = {1, 0, 0};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_NEAR(0.00018784630366397, result.eloss, 5e-4);
        EXPECT_SOFT_EQ(0.0001, result.displacement);
        EXPECT_SOFT_NEAR(0.9999807140391257, result.angle, 1e-3);
        EXPECT_SOFT_EQ(3.3396076266578e-15, result.time);
        EXPECT_SOFT_NEAR(0.00010000053338476, result.step, 1e-8);
        EXPECT_EQ("geo-boundary", result.action);
    }
}

TEST_F(Em3AlongStepTest, msc_nofluct_finegrid)
{
    msc_ = true;
    fluct_ = false;
    bpd_ = 56;

    size_type num_tracks = 1024;
    Input inp;
    {
        // Even though the MSC cross section decreases with increasing energy,
        // on a finer energy grid the discontinuity in the positron cross
        // section means the cross section could have a *positive* slope just
        // above 10 MeV.
        SCOPED_TRACE("positron wth MSC cross section near discontinuity");
        inp.particle_id = this->particle()->find(pdg::positron());
        inp.energy = MevEnergy{10.6026777729432};
        inp.position
            = {-3.81588975039638, 0.0396989319776775, -0.0362911231520308};
        inp.direction
            = {0.995881993983801, -0.0107323420361051, 0.0900215023939723};
        inp.phys_mfp = 0.469519866261640;
        auto result = this->run(inp, num_tracks);
        // Distance to interaction = 0.0499189990540797
        EXPECT_SOFT_NEAR(0.049721747266950993, result.step, 1e-8);
        EXPECT_EQ("geo-boundary", result.action);
    }
}

TEST_F(Em3AlongStepTest, fluct_nomsc)
{
    msc_ = false;
    fluct_ = true;

    size_type num_tracks = 4096;
    Input inp;
    {
        SCOPED_TRACE("electron parallel to boundary");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{10};
        inp.position = {0.0 - 0.25};
        inp.direction = {0, 1, 0};
        auto result = this->run(inp, num_tracks);

        EXPECT_SOFT_NEAR(2.0631083076865, result.eloss, 1e-2);
        EXPECT_SOFT_NEAR(1.1026770872455, result.displacement, 1e-2);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_NEAR(3.6824891684752e-11, result.time, 1e-2);
        EXPECT_SOFT_NEAR(1.1026770872455, result.step, 1e-2);
        EXPECT_EQ("physics-discrete-select", result.action);
    }
    {
        SCOPED_TRACE("electron very near (1um) boundary");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{10};
        inp.position = {0.0 - 1e-4};
        inp.direction = {1, 0, 0};
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_NEAR(0.00019264335626186, result.eloss, 0.1);
        EXPECT_SOFT_EQ(9.9999999999993e-05, result.displacement);
        EXPECT_SOFT_EQ(1, result.angle);
        EXPECT_SOFT_EQ(3.3395898137995e-15, result.time);
        EXPECT_SOFT_EQ(9.9999999999993e-05, result.step);
        EXPECT_EQ("geo-boundary", result.action);
    }
}

TEST_F(SimpleCmsAlongStepTest, msc_field)
{
    size_type num_tracks = 128;
    Input inp;
    {
        // This track takes ~150k substeps in the field propagator before
        // reaching a boundary.
        SCOPED_TRACE("electron taking large step in vacuum");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{0.697421113579829943};
        inp.phys_mfp = 0.0493641564748481393;
        inp.position = {-33.3599681684743388, 1.43414625226707426, -700.000001};
        inp.direction = {-0.680265923322200705,
                         0.731921125057842015,
                         -0.0391118941072485030};
        // Step limited by distance to interaction = 2.49798914193346685e21
        auto result = this->run(inp, num_tracks);
        EXPECT_SOFT_EQ(27.208980085333259, result.step);
        EXPECT_EQ(0, result.eloss);
        EXPECT_EQ(0, result.mfp);
        EXPECT_EQ("geo-propagation-limit", result.action);
        EXPECT_DOUBLE_EQ(1, result.alive);
    }
}

TEST_F(SimpleCmsAlongStepTest, msc_field_finegrid)
{
    bpd_ = 56;

    size_type num_tracks = 1024;
    Input inp;
    {
        SCOPED_TRACE("range-limited electron in field near boundary");
        inp.particle_id = this->particle()->find(pdg::electron());
        inp.energy = MevEnergy{1.76660104663773580e-3};
        // The track is taking its second step in the EM calorimeter, so uses
        // the cached MSC range values from the previous step
        inp.msc_range = {8.43525996595540601e-4, 0.04, 1.34976131122020193e-5};
        inp.position = {
            59.3935490766840459, -109.988210668881749, -81.7228237502843484};
        inp.direction = {
            -0.333769826820287552, 0.641464235110772663, -0.690739703345700562};
        auto result = this->run(inp, num_tracks);
        // Range = 6.41578930992857482e-06
        EXPECT_SOFT_EQ(6.41578930992857482e-6, result.step);
        EXPECT_SOFT_EQ(inp.energy.value(), result.eloss);
        EXPECT_EQ("eloss-range", result.action);
        EXPECT_DOUBLE_EQ(0, result.alive);
    }
}
//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas
