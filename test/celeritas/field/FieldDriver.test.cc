//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/field/FieldDriver.test.cc
//---------------------------------------------------------------------------//

#include "celeritas/field/FieldDriver.hh"

#include "corecel/Types.hh"
#include "corecel/cont/Range.hh"
#include "corecel/math/Algorithms.hh"
#include "celeritas/Constants.hh"
#include "celeritas/Quantities.hh"
#include "celeritas/field/DormandPrinceStepper.hh"
#include "celeritas/field/FieldDriverOptions.hh"
#include "celeritas/field/MagFieldEquation.hh"
#include "celeritas/field/MakeMagFieldPropagator.hh"
#include "celeritas/field/Types.hh"
#include "celeritas/field/UniformField.hh"
#include "celeritas/field/detail/FieldUtils.hh"

#include "DiagnosticStepper.hh"
#include "FieldTestParams.hh"
#include "celeritas_test.hh"

using celeritas::constants::sqrt_two;
using celeritas::units::ElementaryCharge;
using celeritas::units::MevEnergy;
using celeritas::units::MevMass;
using celeritas::units::MevMomentum;

template<class E>
using DiagnosticDPStepper
    = celeritas::test::DiagnosticStepper<celeritas::DormandPrinceStepper<E>>;

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class FieldDriverTest : public Test
{
  public:
    static constexpr MevMass electron_mass() { return MevMass{0.5109989461}; }
    static constexpr ElementaryCharge electron_charge()
    {
        return ElementaryCharge{-1};
    }

    // Calculate momentum assuming an electron
    MevMomentum calc_momentum(MevEnergy energy) const
    {
        real_type m = electron_mass().value();
        real_type e = energy.value();
        return MevMomentum{std::sqrt(e * e + 2 * m * e)};
    }

    // Get the momentum in units of MevMomentum
    Real3 calc_momentum(MevEnergy energy, Real3 const& dir)
    {
        CELER_EXPECT(is_soft_unit_vector(dir));
        return detail::ax(this->calc_momentum(energy).value(), dir);
    }

    // Calculate momentum assuming an electron
    real_type calc_curvature(MevEnergy energy, real_type field_strength) const
    {
        CELER_EXPECT(field_strength > 0);
        return native_value_from(calc_momentum(energy))
               / (std::fabs(native_value_from(electron_charge()))
                  * field_strength);
    }
};

class RevolutionFieldDriverTest : public FieldDriverTest
{
  protected:
    void SetUp() override
    {
        // Input parameters of an electron in a uniform magnetic field
        test_params.nsteps = 100;
        test_params.revolutions = 10;
        test_params.radius = 3.8085386036 * units::centimeter;
        test_params.delta_z = 6.7003310629 * units::centimeter;
        test_params.epsilon = 1.0e-5;
    }

  protected:
    // Field parameters
    FieldDriverOptions driver_options;

    // Test parameters
    FieldTestParams test_params;
};

//---------------------------------------------------------------------------//

template<template<class EquationT> class StepperT, class FieldT>
CELER_FUNCTION decltype(auto)
make_mag_field_driver(FieldT&& field,
                      FieldDriverOptions const& options,
                      units::ElementaryCharge charge)
{
    using Driver_t = FieldDriver<StepperT<MagFieldEquation<FieldT>>>;
    return Driver_t{options,
                    make_mag_field_stepper<StepperT>(
                        ::celeritas::forward<FieldT>(field), charge)};
}

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

TEST_F(FieldDriverTest, types)
{
    FieldDriverOptions driver_options;
    auto driver = make_mag_field_driver<DormandPrinceStepper>(
        UniformField({0, 0, 1}), driver_options, electron_charge());

    // Make sure object is holding things by value
    EXPECT_TRUE(
        (std::is_same<
            FieldDriver<DormandPrinceStepper<MagFieldEquation<UniformField>>>,
            decltype(driver)>::value));
    // Size: field vector, q / c, reference to options
    EXPECT_EQ(sizeof(Real3) + sizeof(real_type) + sizeof(FieldDriverOptions*),
              sizeof(driver));
}

TEST_F(FieldDriverTest, step_counts)
{
    FieldDriverOptions driver_options;
    driver_options.max_nsteps = std::numeric_limits<short int>::max();

    real_type field_strength = 1.0 * units::tesla;
    auto stepper = make_mag_field_stepper<DiagnosticDPStepper>(
        UniformField({0, 0, field_strength}), units::ElementaryCharge{-1});
    FieldDriver<decltype(stepper)&> driver{driver_options, stepper};

    std::vector<real_type> energy;
    std::vector<real_type> radii;
    std::vector<unsigned int> counts;
    std::vector<real_type> lengths;

    // Test the number of field equation evaluations that have to be done to
    // travel a step length of 10 cm, for electrons from 0.1 eV to 10 TeV.
    for (int loge : range(-7, 7).step(1))
    {
        MevEnergy e{std::pow(10, loge)};
        real_type radius = this->calc_curvature(e, field_strength);

        OdeState state;
        state.pos = {radius, 0, 0};
        state.mom = this->calc_momentum(e, {0, sqrt_two / 2, sqrt_two / 2});

        stepper.reset_count();
        auto end = driver.advance(10 * units::centimeter, state);

        energy.push_back(e.value());
        radii.push_back(radius);
        counts.push_back(stepper.count());
        lengths.push_back(end.step);
    }

    // clang-format off
    static double const expected_radii[] = {0.00010663611598835,
        0.00033721315583664, 0.0010663663247419, 0.0033722948818996,
        0.010668826843187, 0.033885874824232, 0.11173141982667,
        0.47431804394274, 3.5019461121752, 33.526427131057, 333.73450257138,
        3335.8113985278, 33356.579970281, 333564.26564901};
    static unsigned int const expected_counts[] = {782u, 247u, 92u, 45u, 31u,
        13u, 10u, 8u, 6u, 4u, 2u, 1u, 1u, 1u};
    static double const expected_lengths[] = {0.077562380895466,
        0.077251971561029, 0.076209747456898, 0.072434474486632,
        0.063064404446822, 0.085308004478855, 0.15625, 0.36823861947329,
        0.99606722344324, 3.0796706094122, 9.7157674620814, 10, 10, 10};
    // clang-format on

    EXPECT_VEC_SOFT_EQ(expected_radii, radii);
    EXPECT_VEC_EQ(expected_counts, counts);
    EXPECT_VEC_SOFT_EQ(expected_lengths, lengths);
}

//---------------------------------------------------------------------------//

TEST_F(RevolutionFieldDriverTest, advance)
{
    auto driver = make_mag_field_driver<DormandPrinceStepper>(
        UniformField({0, 0, 1.0 * units::tesla}),
        driver_options,
        electron_charge());

    // Test parameters and the sub-step size
    real_type circumference = 2 * constants::pi * test_params.radius;
    real_type hstep = circumference / test_params.nsteps;

    // Initial state and the epected state after revolutions
    OdeState y;
    y.pos = {test_params.radius, 0, 0};
    y.mom = this->calc_momentum(MevEnergy{10.9181415106}, {0, 0.96, 0.28});

    OdeState y_expected = y;

    real_type total_step_length{0};

    // Try the stepper by hstep for (num_revolutions * num_steps) times
    real_type delta = driver_options.errcon;
    for (int nr = 0; nr < test_params.revolutions; ++nr)
    {
        y_expected.pos
            = {test_params.radius, 0, (nr + 1) * test_params.delta_z};

        // Travel hstep for num_steps times in the field
        for ([[maybe_unused]] int j : range(test_params.nsteps))
        {
            auto end = driver.advance(hstep, y);
            total_step_length += end.step;
            y = end.state;
        }

        // Check the total error and the state (position, momentum)
        EXPECT_VEC_NEAR(y_expected.pos, y.pos, delta);
    }

    // Check the total error, step/curve length
    EXPECT_SOFT_NEAR(
        total_step_length, circumference * test_params.revolutions, delta);
}

TEST_F(RevolutionFieldDriverTest, accurate_advance)
{
    auto driver = make_mag_field_driver<DormandPrinceStepper>(
        UniformField({0, 0, 1.0 * units::tesla}),
        driver_options,
        electron_charge());

    // Test parameters and the sub-step size
    real_type circumference = 2 * constants::pi * test_params.radius;
    real_type hstep = circumference / test_params.nsteps;

    // Initial state and the epected state after revolutions
    OdeState y;
    y.pos = {test_params.radius, 0, 0};
    y.mom = this->calc_momentum(MevEnergy{10.9181415106}, {0, 0.96, 0.28});

    OdeState y_expected = y;

    // Try the stepper by hstep for (num_revolutions * num_steps) times
    real_type total_curved_length{0};
    real_type delta = driver_options.errcon;

    for (int nr = 0; nr < test_params.revolutions; ++nr)
    {
        // test one_good_step
        OdeState y_accurate = y;

        // Travel hstep for num_steps times in the field
        for ([[maybe_unused]] int j : range(test_params.nsteps))
        {
            auto end = driver.accurate_advance(hstep, y_accurate, .001);

            total_curved_length += end.step;
            y_accurate = end.state;
        }
        // Check the total error and the state (position, momentum)
        EXPECT_VEC_NEAR(y_expected.pos, y.pos, delta);
    }

    // Check the total error, step/curve length
    EXPECT_LT(total_curved_length - circumference * test_params.revolutions,
              delta);
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas
