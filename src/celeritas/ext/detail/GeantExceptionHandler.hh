//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/ext/detail/GeantExceptionHandler.hh
//---------------------------------------------------------------------------//
#pragma once

#include <G4ExceptionSeverity.hh>
#include <G4Types.hh>
#include <G4VExceptionHandler.hh>

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Process Geant4 exceptions with Celeritas.
 *
 * The Geant exception handler base class changes global state in its
 * constructor (assigning "this") so this class instance must stay in scope
 * once created. There is no way to save or restore the previous handler.
 * Furthermore, creating a G4RunManagerKernel also resets the exception
 * handler, so errors thrown during setup *CANNOT* be caught by celeritas, and
 * this class can only be used after creating the G4RunManager.
 */
class GeantExceptionHandler final : public G4VExceptionHandler
{
  public:
    // Accept error codes from geant4
    G4bool Notify(char const* originOfException,
                  char const* exceptionCode,
                  G4ExceptionSeverity severity,
                  char const* description) final;
};

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas
