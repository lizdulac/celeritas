#----------------------------------*-CMake-*----------------------------------#
# Copyright 2022-2023 UT-Battelle, LLC, and other Celeritas developers.
# See the top-level COPYRIGHT file for details.
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#-----------------------------------------------------------------------------#

set(SOURCES)
set(PRIVATE_DEPS Celeritas::DeviceToolkit)
set(PUBLIC_DEPS Celeritas::corecel)


# Add an object library to limit the propagation of includes to the rest of the
# library.
macro(celeritas_add_object_library target)
  add_library(${target} OBJECT ${ARGN})
  install(TARGETS ${target}
    EXPORT celeritas-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT runtime
  )
endmacro()

#-----------------------------------------------------------------------------#
# Main code
#-----------------------------------------------------------------------------#

list(APPEND SOURCES
  Types.cc
  em/AtomicRelaxationParams.cc
  em/FluctuationParams.cc
  em/UrbanMscParams.cc
  em/detail/Utils.cc
  em/model/BetheHeitlerModel.cc
  em/model/CombinedBremModel.cc
  em/model/EPlusGGModel.cc
  em/model/KleinNishinaModel.cc
  em/model/LivermorePEModel.cc
  em/model/MollerBhabhaModel.cc
  em/model/MuBremsstrahlungModel.cc
  em/model/RayleighModel.cc
  em/model/RelativisticBremModel.cc
  em/model/SeltzerBergerModel.cc
  em/process/BremsstrahlungProcess.cc
  em/process/ComptonProcess.cc
  em/process/EIonizationProcess.cc
  em/process/EPlusAnnihilationProcess.cc
  em/process/GammaConversionProcess.cc
  em/process/PhotoelectricProcess.cc
  em/process/RayleighProcess.cc
  ext/GeantPhysicsOptions.cc
  geo/GeoMaterialParams.cc
  geo/GeoParamsOutput.cc
  global/ActionInterface.cc
  global/ActionRegistry.cc
  global/ActionRegistryOutput.cc
  global/CoreParams.cc
  global/CoreTrackData.cc
  global/KernelContextException.cc
  global/Stepper.cc
  global/detail/ActionSequence.cc
  grid/ValueGridBuilder.cc
  grid/ValueGridData.cc
  grid/ValueGridInserter.cc
  grid/VectorUtils.cc
  io/AtomicRelaxationReader.cc
  io/ImportModel.cc
  io/ImportPhysicsTable.cc
  io/ImportPhysicsVector.cc
  io/ImportProcess.cc
  io/LivermorePEReader.cc
  io/SeltzerBergerReader.cc
  mat/MaterialParams.cc
  mat/MaterialParamsOutput.cc
  mat/detail/Utils.cc
  phys/CutoffParams.cc
  phys/ImportedModelAdapter.cc
  phys/ImportedProcessAdapter.cc
  phys/ParticleParams.cc
  phys/ParticleParamsOutput.cc
  phys/PhysicsParams.cc
  phys/PhysicsParamsOutput.cc
  phys/Process.cc
  phys/ProcessBuilder.cc
  random/CuHipRngData.cc
  random/XorwowRngData.cc
  random/XorwowRngParams.cc
  track/ExtendFromSecondariesAction.cc
  track/InitializeTracksAction.cc
  track/SimParams.cc
  track/TrackInitParams.cc
  user/DetectorSteps.cc
  user/StepCollector.cc
)

#-----------------------------------------------------------------------------#
# Configuration-dependent code
#-----------------------------------------------------------------------------#

if(CELERITAS_RNG STREQUAL "CURAND")
  list(APPEND PUBLIC_DEPS Celeritas::DeviceToolkit)
endif()

if(CELERITAS_USE_CUDA OR CELERITAS_USE_HIP)
  list(APPEND SOURCES
    track/detail/Filler.cu
  )
endif()

if(CELERITAS_USE_Geant4)
  set(_cg4_sources
    ext/LoadGdml.cc
    ext/GeantImporter.cc
    ext/GeantSetup.cc
    ext/GeantVolumeMapper.cc
    ext/detail/GeantBremsstrahlungProcess.cc
    ext/detail/GeantExceptionHandler.cc
    ext/detail/GeantLoggerAdapter.cc
    ext/detail/GeantMicroXsCalculator.cc
    ext/detail/GeantModelImporter.cc
    ext/detail/GeantPhysicsList.cc
    ext/detail/GeantProcessImporter.cc
    ext/detail/GeantVolumeVisitor.cc
  )
  set(_cg4_libs Celeritas::corecel XercesC::XercesC ${Geant4_LIBRARIES})

  if(CELERITAS_USE_VecGeom)
    list(APPEND _cg4_sources
      ext/detail/GeantGeoConverter.cc
    )
    list(APPEND _cg4_libs VecGeom::vecgeom)
  else()
    list(APPEND _cg4_libs Celeritas::orange)
  endif()

  celeritas_add_object_library(celeritas_geant4 ${_cg4_sources})
  target_link_libraries(celeritas_geant4 PRIVATE ${_cg4_libs})

  list(APPEND SOURCES $<TARGET_OBJECTS:celeritas_geant4>)
  list(APPEND PRIVATE_DEPS celeritas_geant4)
endif()

if(CELERITAS_USE_HepMC3)
  celeritas_add_object_library(celeritas_hepmc
    io/EventReader.cc
  )
  target_link_libraries(celeritas_hepmc
    PRIVATE Celeritas::corecel HepMC3::HepMC3
  )
  list(APPEND SOURCES $<TARGET_OBJECTS:celeritas_hepmc>)
  list(APPEND PRIVATE_DEPS celeritas_hepmc)
else()
  list(APPEND SOURCES
    io/EventReader.nohepmc.cc
  )
endif()

if(CELERITAS_USE_JSON)
  list(APPEND SOURCES
    ext/GeantPhysicsOptionsIO.json.cc
    field/FieldDriverOptionsIO.json.cc
    phys/PrimaryGeneratorOptionsIO.json.cc
  )
  list(APPEND PRIVATE_DEPS nlohmann_json::nlohmann_json)
endif()

if(CELERITAS_USE_OpenMP)
  list(APPEND PRIVATE_DEPS OpenMP::OpenMP_CXX)
endif()

if(CELERITAS_USE_ROOT)
  # Use directory includes because ROOT has trouble with build/install
  # interface dependencies propagated through corecel.
  # This is safe here as it is only adding project-local paths that are
  # identical to those set in corecel's usage requirments.
  include_directories(
    "${PROJECT_SOURCE_DIR}/src"
    "${CELERITAS_HEADER_CONFIG_DIRECTORY}"
  )

  # Set the CMAKE output directory locally to inform ROOT where we put our
  # libs. Safe against overriding project settings as the celeritas_add_...
  # functions set this to the same value for our targets.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CELERITAS_LIBRARY_OUTPUT_DIRECTORY})

  # Generate the dictionary source file
  root_generate_dictionary(CeleritasRootInterface
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportData.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportElement.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportMaterial.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportParticle.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportPhysicsTable.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportPhysicsVector.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportProcess.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ImportVolume.hh"
    NOINSTALL
    MODULE celeritas
    LINKDEF "${CMAKE_CURRENT_SOURCE_DIR}/ext/RootInterfaceLinkDef.h"
  )
  celeritas_add_object_library(celeritas_root
    ext/RootExporter.cc
    ext/RootImporter.cc
    ext/ScopedRootErrorHandler.cc
    ext/RootUniquePtr.root.cc
    io/RootFileManager.cc
    io/RootStepWriter.cc
    "${CMAKE_CURRENT_BINARY_DIR}/CeleritasRootInterface.cxx"
  )

  target_link_libraries(celeritas_root
    PRIVATE Celeritas::corecel ROOT::Core ROOT::Tree
  )

  # Install the rootmap/pcm files needed for users or downstream apps to use
  # Celeritas ROOT interfaces
  set(_lib_prefix
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_SHARED_LIBRARY_PREFIX}celeritas"
  )
  install(FILES
    "${_lib_prefix}.rootmap"
    "${_lib_prefix}_rdict.pcm"
    COMPONENT runtime
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )

  list(APPEND SOURCES $<TARGET_OBJECTS:celeritas_root>)
  list(APPEND PRIVATE_DEPS celeritas_root)
endif()

if(CELERITAS_USE_VecGeom)
  list(APPEND SOURCES
    ext/VecgeomParams.cc
    ext/detail/VecgeomNavCollection.cc
  )
  list(APPEND PRIVATE_DEPS VecGeom::vgdml)
  # This needs to be public because its might be needed
  # to resolve the symbols generate by the `nvcc -dlink` of
  # one of the executable.
  list(APPEND PUBLIC_DEPS VecGeom::vecgeom)
else()
  list(APPEND PUBLIC_DEPS Celeritas::orange)
endif()

macro(celeritas_polysource filename_we)
  list(APPEND SOURCES "${filename_we}.cc")
  if(CELERITAS_USE_CUDA OR CELERITAS_USE_HIP)
    list(APPEND SOURCES "${filename_we}.cu")
  endif()
endmacro()

#-----------------------------------------------------------------------------#
# Optional CUDA code
#-----------------------------------------------------------------------------#

celeritas_polysource(user/DetectorSteps)
celeritas_polysource(user/detail/StepGatherAction)
celeritas_polysource(global/alongstep/AlongStepGeneralLinearAction)
celeritas_polysource(global/alongstep/AlongStepNeutralAction)
celeritas_polysource(global/alongstep/AlongStepUniformMscAction)
celeritas_polysource(random/detail/CuHipRngStateInit)
celeritas_polysource(track/detail/TrackInitAlgorithms)
celeritas_polysource(track/detail/TrackSortUtils)

#-----------------------------------------------------------------------------#
# Auto-generated code
#-----------------------------------------------------------------------------#

# Generate model launcher and track initialization code for host/device
macro(celeritas_gen_interactor class func)
  celeritas_gen(SOURCES
    "gen-interactor.py" "em/generated/${class}Interact"
    --class ${class}  --func ${func}
  )
endmacro()

macro(celeritas_gen_action basedir class func order)
  celeritas_gen(SOURCES
    "gen-action.py" "${basedir}/generated/${class}"
    --class ${class} --func ${func} --actionorder ${order}
  )
endmacro()

macro(celeritas_gen_trackinit class)
  celeritas_gen(SOURCES
    "gen-trackinit.py" "track/generated/${class}"
    --class ${class}
  )
endmacro()

celeritas_gen_interactor("BetheHeitler" "bethe_heitler")
celeritas_gen_interactor("CombinedBrem" "combined_brem")
celeritas_gen_interactor("EPlusGG" "eplusgg")
celeritas_gen_interactor("KleinNishina" "klein_nishina")
celeritas_gen_interactor("LivermorePE" "livermore_pe")
celeritas_gen_interactor("MollerBhabha" "moller_bhabha")
celeritas_gen_interactor("MuBremsstrahlung" "mu_bremsstrahlung")
celeritas_gen_interactor("Rayleigh" "rayleigh")
celeritas_gen_interactor("RelativisticBrem" "relativistic_brem")
celeritas_gen_interactor("SeltzerBerger" "seltzer_berger")

celeritas_gen_action("phys" "DiscreteSelectAction" "discrete_select" "pre_post")
celeritas_gen_action("phys" "PreStepAction" "pre_step" "pre")
celeritas_gen_action("geo" "BoundaryAction" "boundary" "post")

celeritas_gen_trackinit("InitTracks")
celeritas_gen_trackinit("LocateAlive")
celeritas_gen_trackinit("ProcessPrimaries")
celeritas_gen_trackinit("ProcessSecondaries")

#-----------------------------------------------------------------------------#
# Create library
#-----------------------------------------------------------------------------#

celeritas_add_library(celeritas ${SOURCES})

if(NOT CELERITAS_USE_OpenMP
    AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
         OR CMAKE_CXX_COMPILER_ID MATCHES "Clang$"))
  celeritas_target_compile_options(celeritas
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-unknown-pragmas>
  )
endif()

celeritas_target_link_libraries(celeritas
  PRIVATE ${PRIVATE_DEPS}
  PUBLIC ${PUBLIC_DEPS}
)

#-----------------------------------------------------------------------------#
