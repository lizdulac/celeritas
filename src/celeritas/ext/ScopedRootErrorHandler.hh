//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/ext/ScopedRootErrorHandler.hh
//---------------------------------------------------------------------------//
#pragma once

#include "celeritas_config.h"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Install a ROOT Error Handler to redirect the message toward the
 * Celeritas Logger.
 */
class ScopedRootErrorHandler
{
  public:
    // Install the error handler
    ScopedRootErrorHandler();

    // Return to the previous error handler.
    ~ScopedRootErrorHandler();

  private:
    using ErrorHandlerFuncPtr = void (*)(int, bool, char const*, char const*);

    ErrorHandlerFuncPtr previous_;
};

#if !CELERITAS_USE_ROOT
//!@{
//! Do nothing if ROOT is disabled (source file will not be compiled)
inline ScopedRootErrorHandler::ScopedRootErrorHandler()
{
    (void)sizeof(previous_);
}
inline ScopedRootErrorHandler::~ScopedRootErrorHandler() {}
//!@}
#endif

//---------------------------------------------------------------------------//
}  // namespace celeritas
