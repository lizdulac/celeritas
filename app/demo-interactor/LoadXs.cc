//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2022 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file LoadXs.cc
//---------------------------------------------------------------------------//
#include "LoadXs.hh"

#include <iterator>
#include <utility>
#include <vector>

#include "base/Range.hh"
#include "base/Units.hh"

namespace demo_interactor
{
//---------------------------------------------------------------------------//
/*!
 * Construct particle parameters and send to GPU.
 */
std::shared_ptr<XsGridParams> load_xs()
{
    XsGridParams::Input input;
    // Energy is the concatenation of lambda (up to 1 MeV) and lambdaprim
    // (first point is the same as lambda's last point)
    input.energy
        = {1.38949549e-04, 1.93069773e-04, 2.68269580e-04, 3.72759372e-04,
           5.17947468e-04, 7.19685673e-04, 1.00000000e-03, 1.38949549e-03,
           1.93069773e-03, 2.68269580e-03, 3.72759372e-03, 5.17947468e-03,
           7.19685673e-03, 1.00000000e-02, 1.38949549e-02, 1.93069773e-02,
           2.68269580e-02, 3.72759372e-02, 5.17947468e-02, 7.19685673e-02,
           1.00000000e-01, 1.38949549e-01, 1.93069773e-01, 2.68269580e-01,
           3.72759372e-01, 5.17947468e-01, 7.19685673e-01, 1.00000000e+00,
           1.38949549e+00, 1.93069773e+00, 2.68269580e+00, 3.72759372e+00,
           5.17947468e+00, 7.19685673e+00, 1.00000000e+01, 1.38949549e+01,
           1.93069773e+01, 2.68269580e+01, 3.72759372e+01, 5.17947468e+01,
           7.19685673e+01, 1.00000000e+02, 1.38949549e+02, 1.93069773e+02,
           2.68269580e+02, 3.72759372e+02, 5.17947468e+02, 7.19685673e+02,
           1.00000000e+03, 1.38949549e+03, 1.93069773e+03, 2.68269580e+03,
           3.72759372e+03, 5.17947468e+03, 7.19685673e+03, 1.00000000e+04,
           1.38949549e+04, 1.93069773e+04, 2.68269580e+04, 3.72759372e+04,
           5.17947468e+04, 7.19685673e+04, 1.00000000e+05, 1.38949549e+05,
           1.93069773e+05, 2.68269580e+05, 3.72759372e+05, 5.17947468e+05,
           7.19685673e+05, 1.00000000e+06, 1.38949549e+06, 1.93069773e+06,
           2.68269580e+06, 3.72759372e+06, 5.17947468e+06, 7.19685673e+06,
           1.00000000e+07, 1.38949549e+07, 1.93069773e+07, 2.68269580e+07,
           3.72759372e+07, 5.17947468e+07, 7.19685673e+07, 1.00000000e+08};

    // Cross sections from geant (1/mm)
    static const double lambda[]
        = {7.67362267e-05, 1.64899126e-04, 3.36973428e-04, 6.54835694e-04,
           1.21011994e-03, 2.12659055e-03, 3.55384702e-03, 5.64771749e-03,
           8.53506016e-03, 1.22659050e-02, 1.67630095e-02, 2.17853115e-02,
           2.69237124e-02, 3.16421146e-02, 3.53635164e-02, 3.80692797e-02,
           4.01681611e-02, 4.12364840e-02, 4.10434539e-02, 3.96619528e-02,
           3.73966269e-02, 3.46104627e-02, 3.15873343e-02, 2.84933058e-02,
           2.54088040e-02, 2.23782905e-02, 1.94438346e-02, 1.66551478e-02,
           1.95417601e-02, 2.26153815e-02, 2.58391414e-02, 2.91801620e-02,
           3.26108436e-02, 3.61089916e-02, 3.96573010e-02, 4.32425767e-02,
           4.68549181e-02, 5.04869823e-02, 5.41333691e-02, 5.77901309e-02,
           6.14543945e-02, 6.51240753e-02, 6.87976644e-02, 7.24740710e-02,
           7.61525080e-02, 7.98324076e-02, 8.35133604e-02, 8.71950716e-02,
           9.08773287e-02, 9.45599789e-02, 9.82429120e-02, 1.01926049e-01,
           1.05609332e-01, 1.09292721e-01, 1.12976186e-01, 1.16659705e-01,
           1.20343264e-01, 1.24026851e-01, 1.27710458e-01, 1.31394080e-01,
           1.35077712e-01, 1.38761353e-01, 1.42444998e-01, 1.46128648e-01,
           1.49812300e-01, 1.53495955e-01, 1.57179610e-01, 1.60863267e-01,
           1.64546925e-01, 1.68230583e-01, 1.71914242e-01, 1.75597901e-01,
           1.79281560e-01, 1.82965219e-01, 1.86648879e-01, 1.90332538e-01,
           1.94016198e-01, 1.97699857e-01, 2.01383517e-01, 2.05067176e-01,
           2.08750836e-01, 2.12434495e-01, 2.16118155e-01, 2.19801815e-01};

    input.xs.resize(std::end(lambda) - std::begin(lambda));
    for (auto i : celeritas::range(input.xs.size()))
    {
        input.xs[i] = lambda[i] * (1 / celeritas::units::millimeter);
    }

    input.prime_energy = 1.0; // XS are scaled by a factor of E above 1 MeV

    return std::make_shared<XsGridParams>(std::move(input));
}

//---------------------------------------------------------------------------//
} // namespace demo_interactor
