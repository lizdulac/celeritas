//----------------------------------*-C++-*----------------------------------//
// Copyright 2021-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file demo-loop/diagnostic/StepDiagnostic.hh
//---------------------------------------------------------------------------//
#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"
#include "corecel/cont/Range.hh"
#include "corecel/data/Collection.hh"
#include "corecel/data/CollectionBuilder.hh"
#include "corecel/math/Atomics.hh"
#include "celeritas/Types.hh"
#include "celeritas/global/CoreTrackData.hh"
#include "celeritas/phys/ParticleParams.hh"
#include "celeritas/phys/ParticleTrackView.hh"
#include "celeritas/track/SimTrackView.hh"

#include "../Transporter.hh"
#include "Diagnostic.hh"

namespace demo_loop
{
//---------------------------------------------------------------------------//
/*!
 * Storage for collecting information on steps per track.
 */
template<Ownership W, MemSpace M>
struct StepDiagnosticData
{
    //// TYPES ////

    using size_type = celeritas::size_type;
    template<class T>
    using Items = celeritas::Collection<T, W, M>;

    //// DATA ////

    Items<size_type> counts;  //!< Bin tracks by particle and step count
    size_type num_bins;
    size_type num_particles;

    //// METHODS ////

    //! Whether the data is initialized
    explicit CELER_FUNCTION operator bool() const
    {
        return num_bins > 0 && num_particles > 0
               && counts.size() == num_bins * num_particles;
    }

    //! Assign from another set of data
    template<Ownership W2, MemSpace M2>
    StepDiagnosticData& operator=(StepDiagnosticData<W2, M2>& other)
    {
        CELER_EXPECT(other);
        counts = other.counts;
        num_bins = other.num_bins;
        num_particles = other.num_particles;
        return *this;
    }
};

template<MemSpace M>
using StepDiagnosticDataRef = StepDiagnosticData<Ownership::reference, M>;

//---------------------------------------------------------------------------//
/*!
 * Diagnostic class for getting the distribution of the number of steps per
 * track for each particle type.
 */
template<MemSpace M>
class StepDiagnostic : public Diagnostic<M>
{
  public:
    //!@{
    //! \name Type aliases
    using size_type = celeritas::size_type;
    using SPConstParticles = std::shared_ptr<const celeritas::ParticleParams>;
    using ParamsRef = celeritas::CoreParamsData<Ownership::const_reference, M>;
    using StateRef = celeritas::CoreStateData<Ownership::reference, M>;
    //!@}

  public:
    // Construct with shared problem data and upper bound on steps per track
    StepDiagnostic(ParamsRef const& params,
                   SPConstParticles particles,
                   size_type num_tracks,
                   size_type max_steps);

    // Number of steps per track, tallied before post-processing
    void mid_step(ParamsRef const&, StateRef const& states) final;

    // Collect diagnostic results
    void get_result(TransporterResult* result) final;

    // Get distribution of steps per track for each particle type
    std::unordered_map<std::string, std::vector<size_type>> steps();

  private:
    // Shared problem data
    ParamsRef const& params_;
    // Shared particle data for getting particle name from particle ID
    SPConstParticles particles_;
    // Data for finding distribution of steps per track
    StepDiagnosticData<Ownership::value, M> data_;
};

//---------------------------------------------------------------------------//
// KERNEL LAUNCHER(S)
//---------------------------------------------------------------------------//
/*!
 * Step diagnostic kernel launcher.
 */
template<MemSpace M>
class StepLauncher
{
  public:
    //!@{
    //! \name Type aliases
    using size_type = celeritas::size_type;
    using TrackSlotId = celeritas::TrackSlotId;
    using ParamsRef = celeritas::CoreParamsData<Ownership::const_reference, M>;
    using StateRef = celeritas::CoreStateData<Ownership::reference, M>;
    //!@}

  public:
    // Construct with shared and state data
    CELER_FUNCTION StepLauncher(ParamsRef const& params,
                                StateRef const& states,
                                StepDiagnosticDataRef<M> data);

    // Create track views and tally steps per track
    inline CELER_FUNCTION void operator()(TrackSlotId tid) const;

  private:
    ParamsRef const& params_;
    StateRef const& states_;
    StepDiagnosticDataRef<M> data_;
};

void count_steps(celeritas::HostCRef<celeritas::CoreParamsData> const& params,
                 celeritas::HostRef<celeritas::CoreStateData> const& states,
                 StepDiagnosticDataRef<MemSpace::host> data);

void count_steps(celeritas::DeviceCRef<celeritas::CoreParamsData> const& params,
                 celeritas::DeviceRef<celeritas::CoreStateData> const& states,
                 StepDiagnosticDataRef<MemSpace::device> data);

#if !CELER_USE_DEVICE
inline void count_steps(celeritas::DeviceCRef<celeritas::CoreParamsData> const&,
                        celeritas::DeviceRef<celeritas::CoreStateData> const&,
                        StepDiagnosticDataRef<MemSpace::device>)
{
    CELER_ASSERT_UNREACHABLE();
}
#endif

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Construct from shared data.
 */
template<MemSpace M>
StepDiagnostic<M>::StepDiagnostic(ParamsRef const& params,
                                  SPConstParticles particles,
                                  size_type num_tracks,
                                  size_type max_steps)
    : params_(params), particles_(particles)
{
    CELER_EXPECT(params_);
    CELER_EXPECT(particles_);
    CELER_EXPECT(num_tracks > 0);
    CELER_EXPECT(max_steps > 0);

    ::celeritas::HostVal<StepDiagnosticData> host_data;

    // Add two extra bins for underflow and overflow
    host_data.num_bins = max_steps + 2;
    host_data.num_particles = particles_->size();

    // Tracks binned by number of steps and particle type (indexed as
    // particle_id * num_bins + num_steps). The final bin is for overflow.
    std::vector<size_type> zeros(host_data.num_bins * host_data.num_particles);
    celeritas::make_builder(&host_data.counts)
        .insert_back(zeros.begin(), zeros.end());

    data_ = host_data;
    CELER_ENSURE(data_);
}

//---------------------------------------------------------------------------//
/*!
 * Get the distribution of steps per track.
 *
 * This must be called after the interactions have been processed (when the
 * step count is incremented) and before extend_from_secondaries, which can
 * initialize new tracks immediately in the parent track's slot and overwrite
 * the original track data.
 */
template<MemSpace M>
void StepDiagnostic<M>::mid_step(ParamsRef const&, StateRef const& states)
{
    StepDiagnosticDataRef<M> data_ref;
    data_ref = data_;
    count_steps(params_, states, data_ref);
}

//---------------------------------------------------------------------------//
/*!
 * Collect the diagnostic results.
 */
template<MemSpace M>
void StepDiagnostic<M>::get_result(TransporterResult* result)
{
    result->steps = this->steps();
}

//---------------------------------------------------------------------------//
/*!
 * Get distribution of steps per track for each particle type.
 *
 * For i in [0, \c max_steps + 1], steps[particle][i] is the number of tracks
 * of the given particle type that took i steps. The final bin stores the
 * number of tracks that took greater than \c max_steps steps.
 */
template<MemSpace M>
std::unordered_map<std::string, std::vector<celeritas::size_type>>
StepDiagnostic<M>::steps()
{
    using BinId = celeritas::ItemId<size_type>;

    // Copy result to host if necessary
    ::celeritas::HostVal<StepDiagnosticData> data;
    data = data_;

    // Map particle ID to particle name and store steps per track distribution
    std::unordered_map<std::string, std::vector<size_type>> result;
    for (auto particle_id : range(celeritas::ParticleId{particles_->size()}))
    {
        auto start = BinId{particle_id.get() * data.num_bins};
        auto end = BinId{start.get() + data.num_bins};
        CELER_ASSERT(end.get() <= data.counts.size());
        auto counts = data.counts[celeritas::ItemRange<size_type>{start, end}];

        // Export non-trivial particle's counts
        if (std::any_of(counts.begin(), counts.end(), [](size_type x) {
                return x > 0;
            }))
        {
            result[particles_->id_to_label(particle_id)]
                = {counts.begin(), counts.end()};
        }
    }
    return result;
}

//---------------------------------------------------------------------------//
/*!
 * Construct with shared and state data.
 */
template<MemSpace M>
CELER_FUNCTION StepLauncher<M>::StepLauncher(ParamsRef const& params,
                                             StateRef const& states,
                                             StepDiagnosticDataRef<M> data)
    : params_(params), states_(states), data_(data)
{
    CELER_EXPECT(params_);
    CELER_EXPECT(states_);
    CELER_EXPECT(data_);
}

//---------------------------------------------------------------------------//
/*!
 * Increment step count and tally number of steps for tracks that were killed.
 */
template<MemSpace M>
CELER_FUNCTION void StepLauncher<M>::operator()(TrackSlotId tid) const
{
    using BinId = celeritas::ItemId<size_type>;

    celeritas::ParticleTrackView particle(
        params_.particles, states_.particles, tid);
    celeritas::SimTrackView sim(params_.sim, states_.sim, tid);

    // Tally the number of steps if the track was killed
    if (sim.status() == celeritas::TrackStatus::killed)
    {
        // TODO: Add an ndarray-type class?
        auto get = [this](size_type i, size_type j) -> size_type& {
            size_type index = i * data_.num_bins + j;
            CELER_ENSURE(index < data_.counts.size());
            return data_.counts[BinId(index)];
        };

        size_type num_steps = sim.num_steps() < data_.num_bins - 1
                                  ? sim.num_steps()
                                  : data_.num_bins - 1;

        // Increment the bin corresponding to the given particle and step count
        auto& bin = get(particle.particle_id().get(), num_steps);
        celeritas::atomic_add(&bin, size_type{1});
    }
}

}  // namespace demo_loop
