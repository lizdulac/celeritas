//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/ext/GeantImporter.test.cc
//---------------------------------------------------------------------------//
#include "celeritas/ext/GeantImporter.hh"

#include "celeritas_cmake_strings.h"
#include "celeritas_config.h"
#include "corecel/io/Repr.hh"
#include "corecel/io/StringUtils.hh"
#include "corecel/sys/Version.hh"
#include "celeritas/ext/GeantSetup.hh"
#include "celeritas/io/ImportData.hh"
#include "celeritas/phys/PDGNumber.hh"

#include "../GeantTestBase.hh"
#include "celeritas_test.hh"
#if CELERITAS_USE_JSON
#    include "celeritas/ext/GeantPhysicsOptionsIO.json.hh"
#endif

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
// Helper functions
namespace
{
template<class Iter>
std::vector<std::string> to_vec_string(Iter iter, Iter end)
{
    std::vector<std::string> result;
    for (; iter != end; ++iter)
    {
        result.push_back(to_cstring(*iter));
    }
    return result;
}

auto const geant4_version = Version::from_string(celeritas_geant4_version);
}  // namespace

//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class GeantImporterTest : public GeantTestBase
{
  protected:
    using DataSelection = GeantImportDataSelection;
    using VecModelMaterial = std::vector<ImportModelMaterial>;

    struct ImportSummary
    {
        std::vector<std::string> particles;
        std::vector<std::string> processes;
        std::vector<std::string> models;

        void print_expected() const;
    };

    struct ImportXsSummary
    {
        std::vector<size_type> size;  //!< Number of micro XS in each material
        std::vector<real_type> energy;
        std::vector<real_type> xs;

        void print_expected() const;
    };

    ImportSummary summarize(ImportData const& data) const;
    ImportXsSummary summarize(VecModelMaterial const& xs) const;

    // Import data potentially with different selection options
    GeantImportDataSelection build_import_data_selection() const final
    {
        return selection_;
    }

    ImportProcess const&
    find_process(PDGNumber pdg, ImportProcessClass ipc) const
    {
        CELER_EXPECT(this->imported_data());
        auto const& processes = this->imported_data().processes;
        auto result = std::find_if(processes.begin(),
                                   processes.end(),
                                   [pdg, ipc](ImportProcess const& proc) {
                                       return PDGNumber{proc.particle_pdg}
                                                  == pdg
                                              && proc.process_class == ipc;
                                   });
        CELER_VALIDATE(result != processes.end(),
                       << "missing process " << to_cstring(ipc)
                       << " for particle PDG=" << pdg.get());
        return *result;
    }

    ImportMscModel const&
    find_msc_model(PDGNumber pdg, ImportModelClass imc) const
    {
        CELER_EXPECT(this->imported_data());
        auto const& models = this->imported_data().msc_models;
        auto result = std::find_if(
            models.begin(), models.end(), [pdg, imc](ImportMscModel const& m) {
                return PDGNumber{m.particle_pdg} == pdg && m.model_class == imc;
            });
        CELER_VALIDATE(result != models.end(),
                       << "missing model " << to_cstring(imc)
                       << " for particle PDG=" << pdg.get());
        return *result;
    }
    real_type comparison_tolerance() const
    {
        // Some values change substantially between geant versions
        return geant4_version == Version(11, 0, 3) ? 1e-12 : 5e-3;
    }

  protected:
    GeantImportDataSelection selection_{};
};

//---------------------------------------------------------------------------//
auto GeantImporterTest::summarize(ImportData const& data) const -> ImportSummary
{
    ImportSummary s;
    for (auto const& p : data.particles)
    {
        s.particles.push_back(p.name);
    }

    // Create sorted unique set of process and model names inserted
    std::set<ImportProcessClass> pclass;
    std::set<ImportModelClass> mclass;
    for (auto const& p : data.processes)
    {
        pclass.insert(p.process_class);
        for (auto const& m : p.models)
        {
            mclass.insert(m.model_class);
        }
    }
    for (auto const& m : data.msc_models)
    {
        mclass.insert(m.model_class);
    }
    s.processes = to_vec_string(pclass.begin(), pclass.end());
    s.models = to_vec_string(mclass.begin(), mclass.end());
    return s;
}

void GeantImporterTest::ImportSummary::print_expected() const
{
    cout << "/*** ADD THE FOLLOWING UNIT TEST CODE ***/\n"
            "static const char* expected_particles[] = "
         << repr(this->particles) << ";\n"
         << "EXPECT_VEC_EQ(expected_particles, summary.particles);\n"
            "static const char* expected_processes[] = "
         << repr(this->processes) << ";\n"
         << "EXPECT_VEC_EQ(expected_processes, summary.processes);\n"
            "static const char* expected_models[] = "
         << repr(this->models) << ";\n"
         << "EXPECT_VEC_EQ(expected_models, summary.models);\n"
            "/*** END CODE ***/\n";
}

auto GeantImporterTest::summarize(VecModelMaterial const& materials) const
    -> ImportXsSummary
{
    ImportXsSummary result;
    for (auto const& mat : materials)
    {
        result.size.push_back(mat.energy.size());
        result.energy.push_back(mat.energy.front());
        result.energy.push_back(mat.energy.back());
    }

    // Skip export of first material, which is usually vacuum
    auto mat_iter = materials.begin();
    for (auto const& xs_vec : mat_iter->micro_xs)
    {
        EXPECT_EQ(mat_iter->energy.size(), xs_vec.size());
    }
    ++mat_iter;

    for (; mat_iter != materials.end(); ++mat_iter)
    {
        for (auto const& xs_vec : mat_iter->micro_xs)
        {
            EXPECT_EQ(mat_iter->energy.size(), xs_vec.size());
            result.xs.push_back(xs_vec.front() / units::barn);
            result.xs.push_back(xs_vec.back() / units::barn);
        }
    }
    return result;
}

void GeantImporterTest::ImportXsSummary::print_expected() const
{
    cout << "/*** ADD THE FOLLOWING UNIT TEST CODE ***/\n"
         << "static const real_type expected_size[] = " << repr(this->size)
         << ";\n"
         << "EXPECT_VEC_EQ(expected_size, result.size);\n"
         << "static const real_type expected_e[] = " << repr(this->energy)
         << ";\n"
         << "EXPECT_VEC_SOFT_EQ(expected_e, result.energy);\n"
         << "static const real_type expected_xs[] = " << repr(this->xs)
         << ";\n"
         << "EXPECT_VEC_SOFT_EQ(expected_xs, result.xs);\n"
         << "/*** END CODE ***/\n";
}

//---------------------------------------------------------------------------//
class FourSteelSlabsEmStandard : public GeantImporterTest
{
    char const* geometry_basename() const final { return "four-steel-slabs"; }

    GeantPhysicsOptions build_geant_options() const override
    {
        GeantPhysicsOptions opts;
        opts.relaxation = RelaxationSelection::all;
        opts.verbose = true;
#if CELERITAS_USE_JSON
        {
            nlohmann::json out = opts;
            static char const expected[]
                = R"json({"apply_cuts":false,"brems":"all","coulomb_scattering":false,"eloss_fluctuation":true,"em_bins_per_decade":7,"gamma_general":false,"integral_approach":true,"linear_loss_limit":0.01,"lowest_electron_energy":[0.001,"MeV"],"lpm":true,"max_energy":[100000000.0,"MeV"],"min_energy":[0.0001,"MeV"],"msc":"urban_extended","msc_lambda_limit":0.1,"msc_range_factor":0.04,"msc_safety_factor":0.6,"rayleigh_scattering":true,"relaxation":"all","verbose":true})json";
            EXPECT_EQ(std::string(expected), std::string(out.dump()))
                << "\n/*** REPLACE ***/\nR\"json(" << std::string(out.dump())
                << ")json\"\n/******/";
        }
#endif
        return opts;
    }
};

//---------------------------------------------------------------------------//
class TestEm3 : public GeantImporterTest
{
  protected:
    char const* geometry_basename() const final { return "testem3-flat"; }

    GeantPhysicsOptions build_geant_options() const override
    {
        GeantPhysicsOptions opts;
        opts.relaxation = RelaxationSelection::none;
        opts.rayleigh_scattering = false;
        opts.verbose = false;
        return opts;
    }
};

//---------------------------------------------------------------------------//
class OneSteelSphere : public GeantImporterTest
{
  protected:
    char const* geometry_basename() const final { return "one-steel-sphere"; }

    GeantPhysicsOptions build_geant_options() const override
    {
        GeantPhysicsOptions opts;
        opts.msc = MscModelSelection::urban;
        opts.relaxation = RelaxationSelection::none;
        opts.verbose = false;
        return opts;
    }
};

//---------------------------------------------------------------------------//
class OneSteelSphereGG : public OneSteelSphere
{
  protected:
    GeantPhysicsOptions build_geant_options() const override
    {
        auto opts = OneSteelSphere::build_geant_options();
        opts.gamma_general = true;
        opts.msc = MscModelSelection::urban_extended;
        return opts;
    }
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

TEST_F(FourSteelSlabsEmStandard, em_particles)
{
    selection_.particles = DataSelection::em;

    auto&& imported = this->imported_data();
    auto summary = this->summarize(imported);

    static char const* expected_particles[] = {"e+", "e-", "gamma"};
    EXPECT_VEC_EQ(expected_particles, summary.particles);
    static char const* expected_processes[] = {"e_ioni",
                                               "e_brems",
                                               "photoelectric",
                                               "compton",
                                               "conversion",
                                               "rayleigh",
                                               "annihilation"};
    EXPECT_VEC_EQ(expected_processes, summary.processes);
    static char const* expected_models[] = {"urban_msc",
                                            "moller_bhabha",
                                            "e_brems_sb",
                                            "e_brems_lpm",
                                            "e_plus_to_gg",
                                            "livermore_photoelectric",
                                            "klein_nishina",
                                            "bethe_heitler_lpm",
                                            "livermore_rayleigh"};
    EXPECT_VEC_EQ(expected_models, summary.models);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, em_hadronic)
{
    selection_.particles = DataSelection::em | DataSelection::hadron;
    selection_.processes = DataSelection::em;

    auto&& imported = this->imported_data();
    auto summary = this->summarize(imported);

    static char const* expected_particles[] = {"e+", "e-", "gamma", "proton"};
    EXPECT_VEC_EQ(expected_particles, summary.particles);
    static char const* expected_processes[] = {"e_ioni",
                                               "e_brems",
                                               "photoelectric",
                                               "compton",
                                               "conversion",
                                               "rayleigh",
                                               "annihilation"};
    EXPECT_VEC_EQ(expected_processes, summary.processes);
    static char const* expected_models[] = {"urban_msc",
                                            "moller_bhabha",
                                            "e_brems_sb",
                                            "e_brems_lpm",
                                            "e_plus_to_gg",
                                            "livermore_photoelectric",
                                            "klein_nishina",
                                            "bethe_heitler_lpm",
                                            "livermore_rayleigh"};
    EXPECT_VEC_EQ(expected_models, summary.models);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, elements)
{
    auto&& import_data = this->imported_data();

    auto const& elements = import_data.elements;
    EXPECT_EQ(4, elements.size());

    std::vector<std::string> names;
    std::vector<int> atomic_numbers;
    std::vector<double> atomic_masses, inv_rad_lengths_tsai, coulomb_factors;

    for (auto const& element : elements)
    {
        names.push_back(element.name);
        atomic_masses.push_back(element.atomic_mass);
        atomic_numbers.push_back(element.atomic_number);
        coulomb_factors.push_back(element.coulomb_factor);
        inv_rad_lengths_tsai.push_back(1 / element.radiation_length_tsai);
    }

    static char const* expected_names[] = {"Fe", "Cr", "Ni", "H"};
    static int const expected_atomic_numbers[] = {26, 24, 28, 1};
    static double const expected_atomic_masses[] = {
        55.845110798, 51.996130137, 58.6933251009, 1.007940752665};  // [AMU]
    static double const expected_coulomb_factors[] = {0.04197339849163,
                                                      0.03592322294658,
                                                      0.04844802666907,
                                                      6.400838295295e-05};
    // Check inverse radiation length since soft equal comparison is
    // useless for extremely small values
    static double const expected_inv_rad_lengths_tsai[] = {9.3141768784882e+40,
                                                           1.0803147822537e+41,
                                                           8.1192652842163e+40,
                                                           2.3509634762707e+43};

    EXPECT_VEC_EQ(expected_names, names);
    EXPECT_VEC_EQ(expected_atomic_numbers, atomic_numbers);
    EXPECT_VEC_SOFT_EQ(expected_atomic_masses, atomic_masses);
    EXPECT_VEC_SOFT_EQ(expected_coulomb_factors, coulomb_factors);
    EXPECT_VEC_SOFT_EQ(expected_inv_rad_lengths_tsai, inv_rad_lengths_tsai);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, materials)
{
    auto&& import_data = this->imported_data();

    auto const& materials = import_data.materials;
    EXPECT_EQ(2, materials.size());

    std::vector<std::string> names;
    std::vector<int> states;
    std::vector<int> pdgs;
    std::vector<double> cutoff_energies, cutoff_ranges;
    std::vector<double> el_comps_ids, el_comps_mass_frac, el_comps_num_fracs;
    std::vector<double> densities, num_densities, e_densities, temperatures,
        rad_lengths, nuc_int_lengths;

    for (auto const& material : materials)
    {
        names.push_back(material.name);
        states.push_back((int)material.state);
        densities.push_back(material.density);
        e_densities.push_back(material.electron_density);
        num_densities.push_back(material.number_density);
        nuc_int_lengths.push_back(material.nuclear_int_length);
        rad_lengths.push_back(material.radiation_length);
        temperatures.push_back(material.temperature);

        for (auto const& key : material.pdg_cutoffs)
        {
            pdgs.push_back(key.first);
            cutoff_energies.push_back(key.second.energy);
            cutoff_ranges.push_back(key.second.range);
        }

        for (auto const& el_comp : material.elements)
        {
            el_comps_ids.push_back(el_comp.element_id);
            el_comps_mass_frac.push_back(el_comp.mass_fraction);
            el_comps_num_fracs.push_back(el_comp.number_fraction);
        }
    }

    static char const* expected_names[] = {"G4_Galactic", "G4_STAINLESS-STEEL"};
    EXPECT_VEC_EQ(expected_names, names);
    static int const expected_states[] = {3, 1};
    EXPECT_VEC_EQ(expected_states, states);
    static int const expected_pdgs[] = {-11, 11, 22, -11, 11, 22};
    EXPECT_VEC_EQ(expected_pdgs, pdgs);
    static double const expected_cutoff_energies[] = {0.00099,
                                                      0.00099,
                                                      0.00099,
                                                      1.22808845964606,
                                                      1.31345289979559,
                                                      0.0209231725658313};
    EXPECT_VEC_NEAR(expected_cutoff_energies,
                    cutoff_energies,
                    geant4_version.major() == 10 ? 1e-12 : 0.02);
    static double const expected_cutoff_ranges[]
        = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
    EXPECT_VEC_SOFT_EQ(expected_cutoff_ranges, cutoff_ranges);
    static double const expected_densities[] = {1e-25, 8};
    EXPECT_VEC_SOFT_EQ(expected_densities, densities);
    static double const expected_e_densities[]
        = {0.05974697167543, 2.244432022882e+24};
    EXPECT_VEC_SOFT_EQ(expected_e_densities, e_densities);
    static double const expected_num_densities[]
        = {0.05974697167543, 8.699348925899e+22};
    EXPECT_VEC_SOFT_EQ(expected_num_densities, num_densities);
    static double const expected_nuc_int_lengths[]
        = {3.500000280825e+26, 16.67805709739};
    EXPECT_VEC_SOFT_EQ(expected_nuc_int_lengths, nuc_int_lengths);
    static double const expected_rad_lengths[]
        = {6.304350904227e+26, 1.738067064483};
    EXPECT_VEC_SOFT_EQ(expected_rad_lengths, rad_lengths);
    static double const expected_temperatures[] = {2.73, 293.15};
    EXPECT_VEC_SOFT_EQ(expected_temperatures, temperatures);
    static double const expected_el_comps_ids[] = {3, 0, 1, 2};
    EXPECT_VEC_SOFT_EQ(expected_el_comps_ids, el_comps_ids);
    static double const expected_el_comps_mass_frac[]
        = {1, 0.7462128746215, 0.1690010443115, 0.08478608106695};
    EXPECT_VEC_SOFT_EQ(expected_el_comps_mass_frac, el_comps_mass_frac);
    static double const expected_el_comps_num_fracs[] = {1, 0.74, 0.18, 0.08};
    EXPECT_VEC_SOFT_EQ(expected_el_comps_num_fracs, el_comps_num_fracs);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, eioni)
{
    real_type const tol = this->comparison_tolerance();

    ImportProcess const& proc = this->find_process(celeritas::pdg::electron(),
                                                   ImportProcessClass::e_ioni);
    EXPECT_EQ(ImportProcessType::electromagnetic, proc.process_type);
    EXPECT_EQ(celeritas::pdg::electron().get(), proc.secondary_pdg);

    // Test model
    ASSERT_EQ(1, proc.models.size());
    {
        auto const& model = proc.models[0];
        EXPECT_EQ(ImportModelClass::moller_bhabha, model.model_class);
        for (auto const& m : model.materials)
        {
            EXPECT_EQ(2, m.energy.size());
            EXPECT_EQ(0, m.micro_xs.size());
        }
    }

    auto const& tables = proc.tables;
    ASSERT_EQ(3, tables.size());
    {
        // Test energy loss table
        ImportPhysicsTable const& dedx = tables[0];
        ASSERT_EQ(ImportTableType::dedx, dedx.table_type);
        EXPECT_EQ(ImportUnits::mev, dedx.x_units);
        EXPECT_EQ(ImportUnits::mev_per_cm, dedx.y_units);
        ASSERT_EQ(2, dedx.physics_vectors.size());

        ImportPhysicsVector const& steel = dedx.physics_vectors.back();
        EXPECT_EQ(ImportPhysicsVectorType::log, steel.vector_type);
        ASSERT_EQ(steel.x.size(), steel.y.size());
        ASSERT_EQ(85, steel.x.size());
        EXPECT_SOFT_EQ(1e-4, steel.x.front());
        EXPECT_SOFT_EQ(1e8, steel.x.back());
        EXPECT_SOFT_NEAR(839.66835335480653, steel.y.front(), tol);
        EXPECT_SOFT_NEAR(11.378226755591747, steel.y.back(), tol);
    }
    {
        // Test range table
        ImportPhysicsTable const& range = tables[1];
        ASSERT_EQ(ImportTableType::range, range.table_type);
        EXPECT_EQ(ImportUnits::mev, range.x_units);
        EXPECT_EQ(ImportUnits::cm, range.y_units);
        ASSERT_EQ(2, range.physics_vectors.size());

        ImportPhysicsVector const& steel = range.physics_vectors.back();
        EXPECT_EQ(ImportPhysicsVectorType::log, steel.vector_type);
        ASSERT_EQ(steel.x.size(), steel.y.size());
        ASSERT_EQ(85, steel.x.size());
        EXPECT_SOFT_EQ(1e-4, steel.x.front());
        EXPECT_SOFT_EQ(1e8, steel.x.back());
        EXPECT_SOFT_NEAR(2.3818927937550707e-07, steel.y.front(), tol);
        EXPECT_SOFT_NEAR(8788715.7877501156, steel.y.back(), tol);
    }
    {
        // Test cross-section table
        ImportPhysicsTable const& xs = tables[2];
        ASSERT_EQ(ImportTableType::lambda, xs.table_type);
        EXPECT_EQ(ImportUnits::mev, xs.x_units);
        EXPECT_EQ(ImportUnits::cm_inv, xs.y_units);
        ASSERT_EQ(2, xs.physics_vectors.size());

        ImportPhysicsVector const& steel = xs.physics_vectors.back();
        EXPECT_EQ(ImportPhysicsVectorType::log, steel.vector_type);
        ASSERT_EQ(steel.x.size(), steel.y.size());
        ASSERT_EQ(54, steel.x.size());
        EXPECT_SOFT_NEAR(2.616556310615175, steel.x.front(), tol);
        EXPECT_SOFT_EQ(1e8, steel.x.back());
        EXPECT_SOFT_EQ(0, steel.y.front());
        EXPECT_SOFT_NEAR(0.1905939505829807, steel.y[1], tol);
        EXPECT_SOFT_NEAR(0.4373910150880348, steel.y.back(), tol);
    }
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, ebrems)
{
    ImportProcess const& proc = this->find_process(
        celeritas::pdg::electron(), ImportProcessClass::e_brems);
    EXPECT_EQ(celeritas::pdg::gamma().get(), proc.secondary_pdg);
    ASSERT_EQ(2, proc.models.size());
    {
        // Check Seltzer-Berger electron micro xs
        auto const& model = proc.models[0];
        EXPECT_EQ(ImportModelClass::e_brems_sb, model.model_class);
        EXPECT_EQ(2, model.materials.size());

        auto result = summarize(model.materials);
        static const real_type expected_size[] = {7ul, 5ul};
        EXPECT_VEC_EQ(expected_size, result.size);
        static const real_type expected_e[]
            = {0.001, 1000, 0.020822442086622, 1000};
        EXPECT_VEC_SOFT_EQ(expected_e, result.energy);
        static const real_type expected_xs[] = {19.90859573288,
                                                77.272184544415,
                                                16.869369978465,
                                                66.694254412524,
                                                23.221614672926,
                                                88.397283181803};
        EXPECT_VEC_SOFT_EQ(expected_xs, result.xs);
    }
    {
        // Check relativistic brems electron micro xs
        auto const& model = proc.models[1];
        EXPECT_EQ(ImportModelClass::e_brems_lpm, model.model_class);
        EXPECT_EQ(2, model.materials.size());

        auto result = summarize(model.materials);
        static const real_type expected_size[] = {6ul, 6ul};
        EXPECT_VEC_EQ(expected_size, result.size);
        static const real_type expected_e[]
            = {1000, 100000000, 1000, 100000000};
        EXPECT_VEC_SOFT_EQ(expected_e, result.energy);
        static const real_type expected_xs[] = {77.086886023111,
                                                14.346968386977,
                                                66.448046061979,
                                                12.347652116819,
                                                88.449439286966,
                                                16.486040161073};
        EXPECT_VEC_SOFT_EQ(expected_xs, result.xs);
    }
}

TEST_F(FourSteelSlabsEmStandard, conv)
{
    ImportProcess const& proc = this->find_process(
        celeritas::pdg::gamma(), ImportProcessClass::conversion);
    EXPECT_EQ(celeritas::pdg::electron().get(), proc.secondary_pdg);
    ASSERT_EQ(1, proc.models.size());

    {
        // Check Bethe-Heitler micro xs
        auto const& model = proc.models[0];
        EXPECT_EQ(ImportModelClass::bethe_heitler_lpm, model.model_class);

        EXPECT_EQ(2, model.materials.size());

        auto result = summarize(model.materials);

        static const real_type expected_size[] = {9ul, 9ul};
        EXPECT_VEC_EQ(expected_size, result.size);
        static const real_type expected_e[]
            = {1.02199782, 100000000, 1.02199782, 100000000};
        EXPECT_VEC_SOFT_EQ(expected_e, result.energy);
        static const real_type expected_xs[] = {1.4603666285612,
                                                4.4976609946794,
                                                1.250617083013,
                                                3.8760336885145,
                                                1.6856988385825,
                                                5.1617257552977};
        EXPECT_VEC_SOFT_EQ(expected_xs, result.xs);
    }
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, volumes)
{
    auto&& import_data = this->imported_data();

    auto const& volumes = import_data.volumes;
    EXPECT_EQ(5, volumes.size());

    std::vector<unsigned int> material_ids;
    std::vector<std::string> names, solids;

    for (auto const& volume : volumes)
    {
        material_ids.push_back(volume.material_id);
        names.push_back(volume.name);
        solids.push_back(volume.solid_name);
    }

    unsigned int const expected_material_ids[] = {1, 1, 1, 1, 0};

    static char const* expected_names[] = {"box0x125555be0",
                                           "box0x125556d20",
                                           "box0x125557160",
                                           "box0x1255575a0",
                                           "World0x125555f10"};

    static char const* expected_solids[] = {"box0x125555b70",
                                            "box0x125556c70",
                                            "box0x1255570a0",
                                            "box0x125557500",
                                            "World0x125555ea0"};

    EXPECT_VEC_EQ(expected_material_ids, material_ids);
    EXPECT_VEC_EQ(expected_names, names);
    EXPECT_VEC_EQ(expected_solids, solids);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, em_parameters)
{
    auto&& import_data = this->imported_data();

    auto const& em_params = import_data.em_params;
    EXPECT_EQ(true, em_params.energy_loss_fluct);
    EXPECT_EQ(true, em_params.lpm);
    EXPECT_EQ(true, em_params.integral_approach);
    EXPECT_DOUBLE_EQ(0.01, em_params.linear_loss_limit);
    EXPECT_DOUBLE_EQ(0.001, em_params.lowest_electron_energy);
    EXPECT_EQ(true, em_params.auger);
    EXPECT_EQ(0.04, em_params.msc_range_factor);
    EXPECT_EQ(0.6, em_params.msc_safety_factor);
    EXPECT_EQ(0.1, em_params.msc_lambda_limit);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, trans_parameters)
{
    auto&& import_data = this->imported_data();

    EXPECT_EQ(1000, import_data.trans_params.max_substeps);
    EXPECT_EQ(3, import_data.trans_params.looping.size());
    for (auto const& kv : import_data.trans_params.looping)
    {
        EXPECT_EQ(10, kv.second.threshold_trials);
        EXPECT_EQ(250, kv.second.important_energy);
    }
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, sb_data)
{
    auto&& import_data = this->imported_data();

    auto const& sb_map = import_data.sb_data;
    EXPECT_EQ(4, sb_map.size());

    std::vector<int> atomic_numbers;
    std::vector<double> sb_table_x;
    std::vector<double> sb_table_y;
    std::vector<double> sb_table_value;

    for (auto const& key : sb_map)
    {
        atomic_numbers.push_back(key.first);

        auto const& sb_table = key.second;
        sb_table_x.push_back(sb_table.x.front());
        sb_table_y.push_back(sb_table.y.front());
        sb_table_value.push_back(sb_table.value.front());
        sb_table_x.push_back(sb_table.x.back());
        sb_table_y.push_back(sb_table.y.back());
        sb_table_value.push_back(sb_table.value.back());
    }

    int const expected_atomic_numbers[] = {1, 24, 26, 28};
    double const expected_sb_table_x[]
        = {-6.9078, 9.2103, -6.9078, 9.2103, -6.9078, 9.2103, -6.9078, 9.2103};
    double const expected_sb_table_y[]
        = {1e-12, 1, 1e-12, 1, 1e-12, 1, 1e-12, 1};
    double const expected_sb_table_value[] = {7.85327,
                                              0.046875,
                                              2.33528,
                                              0.717773,
                                              2.18202,
                                              0.748535,
                                              2.05115,
                                              0.776611};

    EXPECT_VEC_EQ(expected_atomic_numbers, atomic_numbers);
    EXPECT_VEC_EQ(expected_sb_table_x, sb_table_x);
    EXPECT_VEC_EQ(expected_sb_table_y, sb_table_y);
    EXPECT_VEC_EQ(expected_sb_table_value, sb_table_value);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, livermore_pe_data)
{
    auto&& import_data = this->imported_data();

    auto const& lpe_map = import_data.livermore_pe_data;
    EXPECT_EQ(4, lpe_map.size());

    std::vector<int> atomic_numbers;
    std::vector<size_t> shell_sizes;
    std::vector<double> thresh_lo;
    std::vector<double> thresh_hi;

    std::vector<double> shell_binding_energy;
    std::vector<double> shell_xs;
    std::vector<double> shell_energy;

    for (auto const& key : lpe_map)
    {
        atomic_numbers.push_back(key.first);

        auto const& ilpe = key.second;

        shell_sizes.push_back(ilpe.shells.size());

        auto const& shells_front = ilpe.shells.front();
        auto const& shells_back = ilpe.shells.back();

        thresh_lo.push_back(ilpe.thresh_lo);
        thresh_hi.push_back(ilpe.thresh_hi);

        shell_binding_energy.push_back(shells_front.binding_energy);
        shell_binding_energy.push_back(shells_back.binding_energy);

        shell_xs.push_back(shells_front.xs.front());
        shell_xs.push_back(shells_front.xs.back());
        shell_energy.push_back(shells_front.energy.front());
        shell_energy.push_back(shells_front.energy.back());

        shell_xs.push_back(shells_back.xs.front());
        shell_xs.push_back(shells_back.xs.back());
        shell_energy.push_back(shells_back.energy.front());
        shell_energy.push_back(shells_back.energy.back());
    }

    int const expected_atomic_numbers[] = {1, 24, 26, 28};
    unsigned long const expected_shell_sizes[] = {1ul, 10ul, 10ul, 10ul};
    double const expected_thresh_lo[]
        = {0.00537032, 0.00615, 0.0070834, 0.0083028};
    double const expected_thresh_hi[]
        = {0.0609537, 0.0616595, 0.0616595, 0.0595662};

    double const expected_shell_binding_energy[] = {1.361e-05,
                                                    1.361e-05,
                                                    0.0059576,
                                                    5.96e-06,
                                                    0.0070834,
                                                    7.53e-06,
                                                    0.0083028,
                                                    8.09e-06};

    double const expected_shell_xs[] = {1.58971e-08,
                                        1.6898e-09,
                                        1.58971e-08,
                                        1.6898e-09,
                                        0.00839767,
                                        0.0122729,
                                        1.39553e-10,
                                        4.05087e-06,
                                        0.0119194,
                                        0.0173188,
                                        7.35358e-10,
                                        1.46397e-05,
                                        0.0162052,
                                        0.0237477,
                                        1.20169e-09,
                                        1.91543e-05};

    double const expected_shell_energy[] = {1.361e-05,
                                            0.0933254,
                                            1.361e-05,
                                            0.0933254,
                                            0.0059576,
                                            0.0831764,
                                            5.96e-06,
                                            0.0630957,
                                            0.0070834,
                                            0.081283,
                                            7.53e-06,
                                            0.0653131,
                                            0.0083028,
                                            0.0776247,
                                            8.09e-06,
                                            0.0676083};

    EXPECT_VEC_EQ(expected_atomic_numbers, atomic_numbers);
    EXPECT_VEC_EQ(expected_shell_sizes, shell_sizes);
    EXPECT_VEC_SOFT_EQ(expected_thresh_lo, thresh_lo);
    EXPECT_VEC_SOFT_EQ(expected_thresh_hi, thresh_hi);
    EXPECT_VEC_SOFT_EQ(expected_shell_binding_energy, shell_binding_energy);
    EXPECT_VEC_SOFT_EQ(expected_shell_xs, shell_xs);
    EXPECT_VEC_SOFT_EQ(expected_shell_energy, shell_energy);
}

//---------------------------------------------------------------------------//
TEST_F(FourSteelSlabsEmStandard, atomic_relaxation_data)
{
    auto&& import_data = this->imported_data();

    auto const& ar_map = import_data.atomic_relaxation_data;
    EXPECT_EQ(4, ar_map.size());

    std::vector<int> atomic_numbers;
    std::vector<size_t> shell_sizes;
    std::vector<int> designator;
    std::vector<double> auger_probability;
    std::vector<double> auger_energy;
    std::vector<double> fluor_probability;
    std::vector<double> fluor_energy;

    for (auto const& key : ar_map)
    {
        atomic_numbers.push_back(key.first);

        auto const& shells = key.second.shells;
        shell_sizes.push_back(shells.size());

        if (shells.empty())
        {
            continue;
        }

        auto const& shells_front = shells.front();
        auto const& shells_back = shells.back();

        designator.push_back(shells_front.designator);
        designator.push_back(shells_back.designator);

        auger_probability.push_back(shells_front.auger.front().probability);
        auger_probability.push_back(shells_front.auger.back().probability);
        auger_probability.push_back(shells_back.auger.front().probability);
        auger_probability.push_back(shells_back.auger.back().probability);
        auger_energy.push_back(shells_front.auger.front().energy);
        auger_energy.push_back(shells_front.auger.back().energy);
        auger_energy.push_back(shells_back.auger.front().energy);
        auger_energy.push_back(shells_back.auger.back().energy);

        fluor_probability.push_back(shells_front.fluor.front().probability);
        fluor_probability.push_back(shells_front.fluor.back().probability);
        fluor_probability.push_back(shells_back.fluor.front().probability);
        fluor_probability.push_back(shells_back.fluor.back().probability);
        fluor_energy.push_back(shells_front.fluor.front().energy);
        fluor_energy.push_back(shells_front.fluor.back().energy);
        fluor_energy.push_back(shells_back.fluor.front().energy);
        fluor_energy.push_back(shells_back.fluor.back().energy);
    }

    int const expected_atomic_numbers[] = {1, 24, 26, 28};
    unsigned long const expected_shell_sizes[] = {0ul, 7ul, 7ul, 7ul};
    int const expected_designator[] = {1, 11, 1, 11, 1, 11};

    double const expected_auger_probability[] = {0.048963695828293,
                                                 2.787499762505e-06,
                                                 0.015819909422702,
                                                 0.047183428103535,
                                                 0.044703908588515,
                                                 3.5127206748639e-06,
                                                 0.018361911975474,
                                                 0.076360349801533,
                                                 0.040678795307701,
                                                 3.1360396382578e-06,
                                                 0.021880812772728,
                                                 0.057510033570965};

    double const expected_auger_energy[] = {0.00458292,
                                            0.00594477,
                                            3.728e-05,
                                            3.787e-05,
                                            0.00539748,
                                            0.00706313,
                                            4.063e-05,
                                            4.618e-05,
                                            0.0062898,
                                            0.00828005,
                                            4.837e-05,
                                            5.546e-05};

    double const expected_fluor_probability[] = {0.082575892964534,
                                                 3.6954996851434e-06,
                                                 6.8993041093842e-08,
                                                 1.9834011813594e-08,
                                                 0.10139101947924,
                                                 8.7722616853269e-06,
                                                 3.4925922778373e-07,
                                                 1.158600755629e-07,
                                                 0.12105998603573,
                                                 1.8444997872369e-05,
                                                 1.0946006389633e-06,
                                                 5.1065929809277e-07};

    double const expected_fluor_energy[] = {0.00536786,
                                            0.00595123,
                                            4.374e-05,
                                            4.424e-05,
                                            0.00634985,
                                            0.00707066,
                                            5.354e-05,
                                            5.892e-05,
                                            0.00741782,
                                            0.00828814,
                                            6.329e-05,
                                            7.012e-05};

    EXPECT_VEC_EQ(expected_atomic_numbers, atomic_numbers);
    EXPECT_VEC_EQ(expected_shell_sizes, shell_sizes);
    EXPECT_VEC_EQ(expected_designator, designator);
    EXPECT_VEC_SOFT_EQ(expected_auger_probability, auger_probability);
    EXPECT_VEC_SOFT_EQ(expected_auger_energy, auger_energy);
    EXPECT_VEC_SOFT_EQ(expected_fluor_probability, fluor_probability);
    EXPECT_VEC_SOFT_EQ(expected_fluor_energy, fluor_energy);
}

//---------------------------------------------------------------------------//

TEST_F(TestEm3, volume_names)
{
    selection_.reader_data = false;
    auto const& volumes = this->imported_data().volumes;

    std::vector<std::string> names;
    for (auto const& volume : volumes)
    {
        names.push_back(volume.name);
    }

    // clang-format off
    static std::string const expected_names[] = {"gap_lv_0", "absorber_lv_0", "gap_lv_1", "absorber_lv_1", "gap_lv_2", "absorber_lv_2", "gap_lv_3", "absorber_lv_3", "gap_lv_4", "absorber_lv_4", "gap_lv_5", "absorber_lv_5", "gap_lv_6", "absorber_lv_6", "gap_lv_7", "absorber_lv_7", "gap_lv_8", "absorber_lv_8", "gap_lv_9", "absorber_lv_9", "gap_lv_10", "absorber_lv_10", "gap_lv_11", "absorber_lv_11", "gap_lv_12", "absorber_lv_12", "gap_lv_13", "absorber_lv_13", "gap_lv_14", "absorber_lv_14", "gap_lv_15", "absorber_lv_15", "gap_lv_16", "absorber_lv_16", "gap_lv_17", "absorber_lv_17", "gap_lv_18", "absorber_lv_18", "gap_lv_19", "absorber_lv_19", "gap_lv_20", "absorber_lv_20", "gap_lv_21", "absorber_lv_21", "gap_lv_22", "absorber_lv_22", "gap_lv_23", "absorber_lv_23", "gap_lv_24", "absorber_lv_24", "gap_lv_25", "absorber_lv_25", "gap_lv_26", "absorber_lv_26", "gap_lv_27", "absorber_lv_27", "gap_lv_28", "absorber_lv_28", "gap_lv_29", "absorber_lv_29", "gap_lv_30", "absorber_lv_30", "gap_lv_31", "absorber_lv_31", "gap_lv_32", "absorber_lv_32", "gap_lv_33", "absorber_lv_33", "gap_lv_34", "absorber_lv_34", "gap_lv_35", "absorber_lv_35", "gap_lv_36", "absorber_lv_36", "gap_lv_37", "absorber_lv_37", "gap_lv_38", "absorber_lv_38", "gap_lv_39", "absorber_lv_39", "gap_lv_40", "absorber_lv_40", "gap_lv_41", "absorber_lv_41", "gap_lv_42", "absorber_lv_42", "gap_lv_43", "absorber_lv_43", "gap_lv_44", "absorber_lv_44", "gap_lv_45", "absorber_lv_45", "gap_lv_46", "absorber_lv_46", "gap_lv_47", "absorber_lv_47", "gap_lv_48", "absorber_lv_48", "gap_lv_49", "absorber_lv_49", "world_lv"};
    // clang-format on
    EXPECT_VEC_EQ(expected_names, names);
}

//---------------------------------------------------------------------------//

TEST_F(TestEm3, unique_volumes)
{
    selection_.reader_data = false;
    selection_.unique_volumes = true;

    auto const& volumes = this->imported_data().volumes;

    EXPECT_EQ(101, volumes.size());
    EXPECT_TRUE(starts_with(volumes.front().name, "gap_lv_00x"));
}

//---------------------------------------------------------------------------//

TEST_F(OneSteelSphere, cutoffs)
{
    auto&& import_data = this->imported_data();

    EXPECT_EQ(2, import_data.volumes.size());
    EXPECT_EQ(2, import_data.materials.size());

    // Check secondary production cuts
    std::vector<int> pdg;
    std::vector<double> range_cut, energy_cut;
    for (auto const& mat : import_data.materials)
    {
        for (auto const& cut : mat.pdg_cutoffs)
        {
            pdg.push_back(cut.first);
            range_cut.push_back(cut.second.range);
            energy_cut.push_back(cut.second.energy);
        }
    }
    static int const expected_pdg[] = {-11, 11, 22, -11, 11, 22};
    EXPECT_VEC_EQ(expected_pdg, pdg);
    // 1 mm range cut in vacuum, 50 m range cut in steel
    static double const expected_range_cut[]
        = {0.1, 0.1, 0.1, 5000, 5000, 5000};
    EXPECT_VEC_SOFT_EQ(expected_range_cut, range_cut);
    static double const expected_energy_cut[] = {0.00099,
                                                 0.00099,
                                                 0.00099,
                                                 9549.6516356879,
                                                 9549.6516356879,
                                                 9549.6516356879};
    EXPECT_VEC_SOFT_EQ(expected_energy_cut, energy_cut);
}

TEST_F(OneSteelSphere, physics)
{
    // Check the bremsstrahlung cross sections
    ImportProcess const& brems = this->find_process(
        celeritas::pdg::electron(), ImportProcessClass::e_brems);
    ASSERT_EQ(1, brems.tables.size());
    ASSERT_EQ(2, brems.models.size());
    {
        // Check Seltzer-Berger electron micro xs
        auto const& model = brems.models[0];
        EXPECT_EQ(ImportModelClass::e_brems_sb, model.model_class);
        EXPECT_EQ(2, model.materials.size());

        auto result = summarize(model.materials);
        static unsigned int const expected_size[] = {7u, 2u};
        EXPECT_VEC_EQ(expected_size, result.size);
        static double const expected_energy[]
            = {0.001, 1000, 9549.6516356879, 1000};
        EXPECT_VEC_SOFT_EQ(expected_energy, result.energy);
        // Gamma production cut in steel is higher than the SB model upper
        // energy limit, so there will be no micro xs
        EXPECT_TRUE(result.xs.empty());
    }
    {
        // Check relativistic brems electron micro xs
        auto const& model = brems.models[1];
        EXPECT_EQ(ImportModelClass::e_brems_lpm, model.model_class);
        EXPECT_EQ(2, model.materials.size());

        auto result = summarize(model.materials);
        static const real_type expected_size[] = {6u, 5u};
        EXPECT_VEC_EQ(expected_size, result.size);
        static double const expected_energy[]
            = {1000, 100000000, 9549.6516356879, 100000000};
        EXPECT_VEC_SOFT_EQ(expected_energy, result.energy);
        static double const expected_xs[] = {16.197663688566,
                                             14.176435287746,
                                             13.963271396942,
                                             12.201090525228,
                                             18.583905773638,
                                             16.289792829097};
        EXPECT_VEC_SOFT_EQ(expected_xs, result.xs);
    }
    {
        // Check the bremsstrahlung macro xs
        ImportPhysicsTable const& xs = brems.tables[0];
        ASSERT_EQ(2, xs.physics_vectors.size());
        ImportPhysicsVector const& steel = xs.physics_vectors.back();
        ASSERT_EQ(29, steel.x.size());
        EXPECT_SOFT_EQ(9549.651635687942, steel.x.front());
        EXPECT_SOFT_EQ(1e8, steel.x.back());
    }
    {
        // Check the ionization electron macro xs
        ImportProcess const& ioni = this->find_process(
            celeritas::pdg::electron(), ImportProcessClass::e_ioni);
        ASSERT_EQ(3, ioni.tables.size());

        // Lambda table for steel
        ImportPhysicsTable const& xs = ioni.tables[2];
        ASSERT_EQ(2, xs.physics_vectors.size());
        ImportPhysicsVector const& steel = xs.physics_vectors.back();
        ASSERT_EQ(27, steel.x.size());
        // Starts at min primary energy = 2 * electron production cut for
        // primary electrons
        EXPECT_SOFT_EQ(19099.303271375884, steel.x.front());
        EXPECT_SOFT_EQ(1e8, steel.x.back());
    }
    {
        // Check the ionization positron macro xs
        ImportProcess const& ioni = this->find_process(
            celeritas::pdg::positron(), ImportProcessClass::e_ioni);
        ASSERT_EQ(3, ioni.tables.size());

        // Lambda table for steel
        ImportPhysicsTable const& xs = ioni.tables[2];
        ASSERT_EQ(2, xs.physics_vectors.size());
        ImportPhysicsVector const& steel = xs.physics_vectors.back();
        ASSERT_EQ(29, steel.x.size());
        // Start at min primary energy = electron production cut for primary
        // positrons
        EXPECT_SOFT_EQ(9549.651635687942, steel.x.front());
        EXPECT_SOFT_EQ(1e8, steel.x.back());
    }
    // Check MSC bounds
    {
        // Check the ionization electron macro xs
        ImportMscModel const& msc = this->find_msc_model(
            celeritas::pdg::electron(), ImportModelClass::urban_msc);
        EXPECT_TRUE(msc);
        for (ImportPhysicsVector const& pv : msc.xs_table.physics_vectors)
        {
            ASSERT_FALSE(pv.x.empty());
            EXPECT_SOFT_EQ(1e-4, pv.x.front());
            EXPECT_SOFT_EQ(1e2, pv.x.back());
        }
    }
}

TEST_F(OneSteelSphereGG, physics)
{
    auto&& imported = this->imported_data();
    auto summary = this->summarize(imported);

    static char const* expected_particles[] = {"e+", "e-", "gamma"};
    EXPECT_VEC_EQ(expected_particles, summary.particles);
    static char const* expected_processes[] = {"e_ioni",
                                               "e_brems",
                                               "photoelectric",
                                               "compton",
                                               "conversion",
                                               "rayleigh",
                                               "annihilation"};
    EXPECT_VEC_EQ(expected_processes, summary.processes);
    static char const* expected_models[] = {"urban_msc",
                                            "moller_bhabha",
                                            "e_brems_sb",
                                            "e_brems_lpm",
                                            "e_plus_to_gg",
                                            "livermore_photoelectric",
                                            "klein_nishina",
                                            "bethe_heitler_lpm",
                                            "livermore_rayleigh"};
    EXPECT_VEC_EQ(expected_models, summary.models);

    // Check MSC bounds
    {
        // Check the ionization electron macro xs
        ImportMscModel const& msc = this->find_msc_model(
            celeritas::pdg::electron(), ImportModelClass::urban_msc);
        EXPECT_TRUE(msc);
        for (ImportPhysicsVector const& pv : msc.xs_table.physics_vectors)
        {
            ASSERT_FALSE(pv.x.empty());
            EXPECT_SOFT_EQ(1e-4, pv.x.front());
            EXPECT_SOFT_EQ(1e8, pv.x.back());
        }
    }
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas
