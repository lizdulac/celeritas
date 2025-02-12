//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/io/Logger.hh
//---------------------------------------------------------------------------//
#pragma once

#include <string>
#include <utility>

#include "corecel/io/Logger.hh"

#include "LoggerTypes.hh"
#include "detail/LoggerMessage.hh"  // IWYU pragma: export

//---------------------------------------------------------------------------//
// MACROS
//---------------------------------------------------------------------------//
//! Inject the source code provenance (current file and line)
#define CELER_CODE_PROVENANCE \
    ::celeritas::Provenance   \
    {                         \
        __FILE__, __LINE__    \
    }

/*!
 * \def CELER_LOG
 *
 * Return a LogMessage object for streaming into at the given level. The
 * regular \c CELER_LOG call is for code paths that happen uniformly in
 * parallel.
 *
 * The logger will only format and print messages. It is not responsible
 * for cleaning up the state or exiting an app.
 *
 * \code
 CELER_LOG(debug) << "Don't print this in general";
 CELER_LOG(warning) << "You may want to reconsider your life choices";
 CELER_LOG(critical) << "Caught a fatal exception: " << e.what();
 * \endcode
 */
#define CELER_LOG(LEVEL)                               \
    ::celeritas::world_logger()(CELER_CODE_PROVENANCE, \
                                ::celeritas::LogLevel::LEVEL)

//---------------------------------------------------------------------------//
/*!
 * \def CELER_LOG_LOCAL
 *
 * Like \c CELER_LOG but for code paths that may only happen on a single
 * process. Use sparingly.
 */
#define CELER_LOG_LOCAL(LEVEL)                        \
    ::celeritas::self_logger()(CELER_CODE_PROVENANCE, \
                               ::celeritas::LogLevel::LEVEL)

namespace celeritas
{
class MpiCommunicator;

//---------------------------------------------------------------------------//
/*!
 * Manage logging in serial and parallel.
 *
 * This should generally be called by the \c world_logger and \c
 * self_logger functions below. The call \c operator() returns an object that
 * should be streamed into in order to create a log message.
 *
 * This object \em is assignable, so to replace the default log handler with a
 * different one, you can call \code
   world_logger = Logger(MpiCommunicator::comm_world(), my_handler);
 * \endcode
 */
class Logger
{
  public:
    //!@{
    //! \name Type aliases
    using Message = detail::LoggerMessage;
    //!@}

  public:
    // Construct with communicator (only rank zero is active) and handler
    Logger(MpiCommunicator const& comm,
           LogHandler handle,
           char const* level_env = nullptr);

    // Create a logger that flushes its contents when it destructs
    inline Message operator()(Provenance prov, LogLevel lev);

    //! Set the minimum logging verbosity
    void level(LogLevel lev) { min_level_ = lev; }

    //! Get the current logging verbosity
    LogLevel level() const { return min_level_; }

  private:
    LogHandler handle_;
    LogLevel min_level_ = LogLevel::status;
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
//! Create a logger that flushes its contents when it destructs
auto Logger::operator()(Provenance prov, LogLevel lev) -> Message
{
    LogHandler* handle = nullptr;
    if (handle_ && lev >= min_level_)
    {
        handle = &handle_;
    }
    return {handle, std::move(prov), lev};
}

//---------------------------------------------------------------------------//
// FREE FUNCTIONS
//---------------------------------------------------------------------------//
// Create loggers with reasonable default behaviors.
Logger make_default_world_logger();
Logger make_default_self_logger();

// Parallel logger (print only on "main" process)
Logger& world_logger();

// Serial logger (print on *every* process)
Logger& self_logger();

//---------------------------------------------------------------------------//
}  // namespace celeritas
