//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/em/SeltzerBerger.test.cc
//---------------------------------------------------------------------------//
#include "corecel/cont/Range.hh"
#include "corecel/math/Algorithms.hh"
#include "corecel/math/ArrayUtils.hh"
#include "celeritas/Quantities.hh"
#include "celeritas/em/distribution/SBEnergyDistribution.hh"
#include "celeritas/em/interactor/SeltzerBergerInteractor.hh"
#include "celeritas/em/interactor/detail/SBPositronXsCorrector.hh"
#include "celeritas/em/model/SeltzerBergerModel.hh"
#include "celeritas/em/process/BremsstrahlungProcess.hh"
#include "celeritas/io/SeltzerBergerReader.hh"
#include "celeritas/mat/MaterialTrackView.hh"
#include "celeritas/mat/MaterialView.hh"
#include "celeritas/phys/CutoffView.hh"
#include "celeritas/phys/InteractionIO.hh"
#include "celeritas/phys/InteractorHostTestBase.hh"

#include "celeritas_test.hh"

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class SeltzerBergerTest : public InteractorHostTestBase
{
    using Base = InteractorHostTestBase;

  protected:
    using Energy = units::MevEnergy;
    using EnergySq = SBEnergyDistHelper::EnergySq;

    void SetUp() override
    {
        auto const& particles = *this->particle_params();
        data_.ids.electron = particles.find(pdg::electron());
        data_.ids.positron = particles.find(pdg::positron());
        data_.ids.gamma = particles.find(pdg::gamma());
        data_.electron_mass = particles.get(data_.ids.electron).mass();

        // Set up shared material data
        MaterialParams::Input mat_inp;
        mat_inp.elements = {{AtomicNumber{29}, units::AmuMass{63.546}, "Cu"}};
        mat_inp.materials = {
            {0.141 * constants::na_avogadro,
             293.0,
             MatterState::solid,
             {{ElementId{0}, 1.0}},
             "Cu"},
        };
        this->set_material_params(mat_inp);

        // Set up Seltzer-Berger cross section data
        std::string data_path = this->test_data_path("celeritas", "");
        SeltzerBergerReader read_element_data(data_path.c_str());

        // Create mock import data
        {
            ImportProcess ip_electron = this->make_import_process(
                pdg::electron(),
                pdg::gamma(),
                ImportProcessClass::e_brems,
                {ImportModelClass::e_brems_sb, ImportModelClass::e_brems_lpm});
            ImportProcess ip_positron = ip_electron;
            ip_positron.particle_pdg = pdg::positron().get();
            this->set_imported_processes(
                {std::move(ip_electron), std::move(ip_positron)});
        }

        // Construct SeltzerBergerModel and set host data
        model_
            = std::make_shared<SeltzerBergerModel>(ActionId{0},
                                                   *this->particle_params(),
                                                   *this->material_params(),
                                                   this->imported_processes(),
                                                   read_element_data);
        data_ = model_->host_ref();

        // Set cutoffs
        CutoffParams::Input input;
        CutoffParams::MaterialCutoffs material_cutoffs;
        material_cutoffs.push_back({MevEnergy{0.02064384}, 0.07});
        input.materials = this->material_params();
        input.particles = this->particle_params();
        input.cutoffs.insert({pdg::gamma(), material_cutoffs});
        this->set_cutoff_params(input);

        // Set default particle to incident 1 MeV photon in copper
        this->set_inc_particle(pdg::electron(), MevEnergy{1.0});
        this->set_inc_direction({0, 0, 1});
        this->set_material("Cu");
    }

    EnergySq density_correction(MaterialId matid, Energy e) const
    {
        CELER_EXPECT(matid);
        CELER_EXPECT(e > zero_quantity());
        using namespace constants;

        auto mat = this->material_params()->get(matid);
        constexpr auto migdal = 4 * pi * r_electron
                                * ipow<2>(lambdabar_electron);

        real_type density_factor = mat.electron_density() * migdal;
        return EnergySq{density_factor * ipow<2>(e.value())};
    }

    void sanity_check(Interaction const& interaction) const
    {
        if (interaction.action == Action::unchanged)
        {
            // If incident energy is below cutoff, the interaction is rejected
            return;
        }

        EXPECT_EQ(Action::scattered, interaction.action);
        EXPECT_SOFT_EQ(1.0, norm(interaction.direction));
    }

  protected:
    std::shared_ptr<SeltzerBergerModel> model_;
    SeltzerBergerRef data_;
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

TEST_F(SeltzerBergerTest, sb_tables)
{
    auto const& xs = model_->host_ref().differential_xs;

    // The tables should just have the one element (copper). The values of the
    // arguments have been calculated from the g4emlow@7.13 dataset.
    ASSERT_EQ(1, xs.elements.size());

    auto argmax = xs.sizes[xs.elements[ElementId{0}].argmax];
    unsigned int const expected_argmax[]
        = {31, 31, 31, 30, 30, 7, 7, 6, 5, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0,
           0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_VEC_EQ(argmax, expected_argmax);
}

TEST_F(SeltzerBergerTest, sb_positron_xs_scaling)
{
    ParticleParams const& pp = *this->particle_params();
    const units::MevMass positron_mass
        = pp.get(pp.find(pdg::positron())).mass();
    const MevEnergy gamma_cutoff{0.01};
    const ElementView el = this->material_params()->get(ElementId{0});

    std::vector<real_type> scaling_frac;

    for (real_type inc_energy : {1.0, 10.0, 100., 1000.})
    {
        SBPositronXsCorrector scale_xs(
            positron_mass, el, gamma_cutoff, MevEnergy{inc_energy});
        for (real_type sampled_efrac : {.10001, .5, .9, .9999})
        {
            real_type exit_energy = sampled_efrac * inc_energy;
            scaling_frac.push_back(scale_xs(MevEnergy{exit_energy}));
        }
    }
    // clang-format off
    static const double expected_scaling_frac[] = {
        0.98771267862086, 0.88085886234621, 0.36375147691123, 2.6341925633236e-29,
        0.99965385757708, 0.99583269657665, 0.92157316225919, 2.1585790781929e-09,
        0.99999599590292, 0.99994914123134, 0.99844428624414, 0.0041293798201,
        0.99999995934326, 0.99999948043882, 0.99998298916928, 0.33428689072689};
    // clang-format on
    EXPECT_VEC_NEAR(expected_scaling_frac, scaling_frac, 1e-11);
}

TEST_F(SeltzerBergerTest, sb_energy_dist)
{
    const MevEnergy gamma_cutoff{0.0009};

    int const num_samples = 8192;
    std::vector<double> max_xs;
    std::vector<double> xs_zero;
    std::vector<double> avg_exit_frac;
    std::vector<double> avg_engine_samples;

    auto sample_many = [&](real_type inc_energy, auto& sample_energy) {
        double total_exit_energy = 0;
        RandomEngine& rng_engine = this->rng();
        for (int i = 0; i < num_samples; ++i)
        {
            Energy exit_gamma = sample_energy(rng_engine);
            EXPECT_GT(exit_gamma.value(), gamma_cutoff.value());
            EXPECT_LT(exit_gamma.value(), inc_energy);
            total_exit_energy += exit_gamma.value();
        }

        avg_exit_frac.push_back(total_exit_energy / (num_samples * inc_energy));
        avg_engine_samples.push_back(double(rng_engine.count()) / num_samples);
    };

    // Note: the first point has a very low cross section compared to
    // ionization so won't be encountered in practice. The differential cross
    // section distribution is much flatter there, so there should be lower
    // rejection. The second point is where the maximum of the differential SB
    // data switches between a high-exit-energy peak and a low-exit-energy
    // peak, which should result in a higher rejection rate. The remaining
    // points are arbitrary.
    for (real_type inc_energy : {0.001, 0.0045, 0.567, 7.89, 89.0, 901.})
    {
        SBEnergyDistHelper edist_helper(
            model_->host_ref().differential_xs,
            Energy{inc_energy},
            ElementId{0},
            this->density_correction(MaterialId{0}, Energy{inc_energy}),
            gamma_cutoff);
        max_xs.push_back(edist_helper.max_xs().value());
        xs_zero.push_back(edist_helper.xs_zero().value());

        SBEnergyDistribution<SBElectronXsCorrector> sample_energy(edist_helper,
                                                                  {});
        // Loop over many particles
        sample_many(inc_energy, sample_energy);
    }

    {
        real_type inc_energy = 7.89;

        SBEnergyDistHelper edist_helper(
            model_->host_ref().differential_xs,
            Energy{inc_energy},
            ElementId{0},
            this->density_correction(MaterialId{0}, Energy{inc_energy}),
            gamma_cutoff);

        // Sample with a "correction" that's constant, which shouldn't change
        // sampling efficiency or expected value correction
        {
            struct ScaleXs
            {
                using Xs = Quantity<units::Millibarn>;

                real_type operator()(Energy) const { return 0.5; }

                Xs max_xs(SBEnergyDistHelper const& helper) const
                {
                    return helper.calc_xs(MevEnergy{0.0009});
                }
            };

            SBEnergyDistribution<ScaleXs> sample_energy(edist_helper,
                                                        ScaleXs{});

            // Loop over many particles
            sample_many(inc_energy, sample_energy);
        }

        // Sample with the positron XS correction
        {
            ParticleParams const& pp = *this->particle_params();

            SBEnergyDistribution<SBPositronXsCorrector> sample_energy(
                edist_helper,
                {pp.get(pp.find(pdg::positron())).mass(),
                 this->material_params()->get(ElementId{0}),
                 gamma_cutoff,
                 Energy{inc_energy}});

            // Loop over many particles
            sample_many(inc_energy, sample_energy);
        }
    }

    // clang-format off
    const double expected_max_xs[] = {2.866525852195, 4.72696244794,
        12.18911946078, 13.93366489719, 13.85758694967, 13.3353235437};
    const double expected_xs_zero[] = {1.98829818915769, 4.40320232447369,
        12.18911946078, 13.93366489719, 13.85758694967, 13.3353235437};
    const double expected_avg_exit_frac[] = {0.949115932248866,
        0.497486662164049, 0.082127972143285, 0.0645177016233406,
        0.0774717918229646, 0.0891340819129683, 0.0639090949553034,
        0.0642877319142647};
    const double expected_avg_engine_samples[] = {4.0791015625, 4.06005859375,
	5.134765625, 4.65625, 4.43017578125, 4.35693359375, 9.3681640625,
        4.65478515625};
    // clang-format on

    EXPECT_VEC_SOFT_EQ(expected_max_xs, max_xs);
    EXPECT_VEC_SOFT_EQ(expected_xs_zero, xs_zero);
    EXPECT_VEC_SOFT_EQ(expected_avg_exit_frac, avg_exit_frac);
    EXPECT_VEC_SOFT_EQ(expected_avg_engine_samples, avg_engine_samples);
}

TEST_F(SeltzerBergerTest, basic)
{
    // Reserve 4 secondaries, one for each sample
    int const num_samples = 4;
    this->resize_secondaries(num_samples);

    // Production cuts
    auto material_view = this->material_track().make_material_view();
    auto cutoffs = this->cutoff_params()->get(MaterialId{0});

    // Create the interactor
    SeltzerBergerInteractor interact(model_->host_ref(),
                                     this->particle_track(),
                                     this->direction(),
                                     cutoffs,
                                     this->secondary_allocator(),
                                     material_view,
                                     ElementComponentId{0});
    RandomEngine& rng_engine = this->rng();

    // Produce two samples from the original/incident photon
    std::vector<double> angle;
    std::vector<double> energy;

    // Loop number of samples
    for (int i : range(num_samples))
    {
        Interaction result = interact(rng_engine);
        SCOPED_TRACE(result);
        this->sanity_check(result);

        EXPECT_EQ(result.secondaries.data(),
                  this->secondary_allocator().get().data()
                      + result.secondaries.size() * i);

        energy.push_back(result.secondaries[0].energy.value());
        angle.push_back(dot_product(result.direction,
                                    result.secondaries.front().direction));
    }

    EXPECT_EQ(num_samples, this->secondary_allocator().get().size());

    // Note: these are "gold" values based on the host RNG.
    double const expected_angle[] = {0.959441513277674,
                                     0.994350429950924,
                                     0.968866136008621,
                                     0.961582855967571};
    double const expected_energy[] = {0.0349225070114679,
                                      0.0316182310804369,
                                      0.0838794010486177,
                                      0.106195186929141};

    EXPECT_VEC_SOFT_EQ(expected_energy, energy);
    EXPECT_VEC_SOFT_EQ(expected_angle, angle);

    // Next sample should fail because we're out of secondary buffer space
    {
        Interaction result = interact(rng_engine);
        EXPECT_EQ(0, result.secondaries.size());
        EXPECT_EQ(Action::failed, result.action);
    }
}

TEST_F(SeltzerBergerTest, stress_test)
{
    int const num_samples = 1e4;
    std::vector<double> avg_engine_samples;

    // Views
    auto cutoffs = this->cutoff_params()->get(MaterialId{0});
    auto material_view = this->material_track().make_material_view();

    // Loop over a set of incident gamma energies
    for (auto particle : {pdg::electron(), pdg::positron()})
    {
        for (double inc_e : {1.5, 5.0, 10.0, 50.0, 100.0})
        {
            SCOPED_TRACE("Incident energy: " + std::to_string(inc_e));
            this->set_inc_particle(pdg::gamma(), MevEnergy{inc_e});

            RandomEngine& rng_engine = this->rng();
            RandomEngine::size_type num_particles_sampled = 0;

            // Loop over several incident directions
            for (Real3 const& inc_dir : {Real3{0, 0, 1},
                                         Real3{1, 0, 0},
                                         Real3{1e-9, 0, 1},
                                         Real3{1, 1, 1}})
            {
                this->set_inc_direction(inc_dir);
                this->resize_secondaries(num_samples);

                // Create interactor
                this->set_inc_particle(particle, MevEnergy{inc_e});
                SeltzerBergerInteractor interact(model_->host_ref(),
                                                 this->particle_track(),
                                                 this->direction(),
                                                 cutoffs,
                                                 this->secondary_allocator(),
                                                 material_view,
                                                 ElementComponentId{0});

                // Loop over many particles
                for (unsigned int i = 0; i < num_samples; ++i)
                {
                    Interaction result = interact(rng_engine);
                    this->sanity_check(result);
                }
                EXPECT_EQ(num_samples,
                          this->secondary_allocator().get().size());
                num_particles_sampled += num_samples;
            }
            avg_engine_samples.push_back(double(rng_engine.count())
                                         / double(num_particles_sampled));
        }
    }

    // Gold values for average number of calls to RNG
    static double const expected_avg_engine_samples[] = {14.088,
                                                         13.2402,
                                                         12.9641,
                                                         12.5832,
                                                         12.4988,
                                                         14.2108,
                                                         13.254,
                                                         12.9431,
                                                         12.5952,
                                                         12.4888};

    EXPECT_VEC_SOFT_EQ(expected_avg_engine_samples, avg_engine_samples);
}

TEST_F(SeltzerBergerTest, positron_xs_corrector_edge_case)
{
    // See https://github.com/celeritas-project/celeritas/issues/617

    // Set up material data (only value used in this test is the atomic number)
    MaterialParams::Input mat_inp;
    mat_inp.elements = {{AtomicNumber{26}, units::AmuMass{55.845}, "Fe"}};
    mat_inp.materials = {{0.128 * constants::na_avogadro,
                          293.0,
                          MatterState::solid,
                          {{ElementId{0}, 1.0}},
                          "Fe"}};

    auto const material_params
        = std::make_shared<MaterialParams>(std::move(mat_inp));

    units::MevMass const positron_mass{0.51099890999999997};
    units::MevEnergy const min_gamma_energy{0.020822442086622296};
    units::MevEnergy const inc_energy{241.06427050865221};
    units::MevEnergy const sampled_energy{0.020822442732819097};
    SBPositronXsCorrector xs_corrector(positron_mass,
                                       material_params->get(ElementId{0}),
                                       min_gamma_energy,
                                       inc_energy);

    auto const result = xs_corrector(sampled_energy);
    EXPECT_EQ(1, result);
}
//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas
