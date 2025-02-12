#----------------------------------*-CMake-*----------------------------------#
# Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
# See the top-level COPYRIGHT file for details.
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#-----------------------------------------------------------------------------#

include(CeleritasAddTest)

#-----------------------------------------------------------------------------#
# TEST HARNESS
#-----------------------------------------------------------------------------#

set(CELERITAS_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
configure_file(testdetail/TestConfig.cc.in TestConfig.cc @ONLY)

celeritas_add_library(testcel_harness
  Test.cc
  testdetail/TestMacrosImpl.cc
  testdetail/NonMasterResultPrinter.cc
  testdetail/ParallelHandler.cc
  testdetail/TestMainImpl.cc
  "${CMAKE_CURRENT_BINARY_DIR}/TestConfig.cc"
)
target_compile_features(testcel_harness PUBLIC cxx_std_14)
celeritas_target_link_libraries(testcel_harness
  PUBLIC Celeritas::corecel GTest::GTest
)
celeritas_target_include_directories(testcel_harness
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

#-----------------------------------------------------------------------------#
# HIP SUPPORT
#-----------------------------------------------------------------------------#

if(CELERITAS_USE_HIP)
  set_source_files_properties(
    celeritas/field/FieldDriver.test.cu
    celeritas/field/FieldPropagator.test.cu
    celeritas/field/Steppers.test.cu
    celeritas/field/UserMapField.test.cu
    celeritas/field/UserParamField.test.cu
    celeritas/geo/HeuristicGeoTestBase.cu
    celeritas/mat/Material.test.cu
    celeritas/phys/Particle.test.cu
    celeritas/phys/Physics.test.cu
    celeritas/random/RngEngine.test.cu
    celeritas/random/curand/CurandPerformance.test.cu
    celeritas/track/TrackInit.test.cu
    corecel/cont/Range.test.cu
    corecel/data/Collection.test.cu
    corecel/data/StackAllocator.test.cu
    corecel/math/NumericLimits.test.cu
    orange/surf/SurfaceAction.test.cu
    orange/univ/SimpleUnitTracker.test.cu
    PROPERTIES LANGUAGE HIP
  )
endif()

#-----------------------------------------------------------------------------#
# TEST OPTIONS
#-----------------------------------------------------------------------------#

function(celeritas_add_device_test base)
  if(CELERITAS_USE_CUDA OR CELERITAS_USE_HIP)
    set(_cuda_args GPU SOURCES "${base}.test.cu")
  endif()
  celeritas_add_test("${base}.test.cc" ${_cuda_args} ${ARGN})
endfunction()

if(NOT (CELERITAS_USE_CUDA OR CELERITAS_USE_HIP))
  set(_needs_device DISABLE)
endif()
if(NOT CELERITAS_USE_HepMC3)
  set(_needs_hepmc DISABLE)
endif()
if(CELERITAS_USE_JSON)
  set(_optional_json_link nlohmann_json::nlohmann_json)
endif()
if(NOT CELERITAS_USE_ROOT)
  set(_needs_root DISABLE)
endif()
if(CELERITAS_DEBUG)
  set(_disable_if_debug DISABLE)
endif()
if(NOT CELERITAS_USE_VecGeom AND NOT CELERITAS_USE_JSON)
  set(_needs_geo DISABLE)
endif()

if(NOT CELERITAS_USE_Geant4)
  set(_needs_geant4 DISABLE)
else()
  # Optional dependence on low-energy EM data
  set(_geant4_test_env)
  foreach(_ds G4EMLOW G4ENSDFSTATE)
    list(APPEND _geant4_test_env
      "${Geant4_DATASET_${_ds}_ENVVAR}=${Geant4_DATASET_${_ds}_PATH}"
    )
  endforeach()
  set(_optional_geant4_env ENVIRONMENT "${_geant4_test_env}")
  set(_optional_geant4_link ${Geant4_LIBRARIES})
  set(_needs_geant4 ${_optional_geant4_env})
endif()

#-----------------------------------------------------------------------------#
# GOOGLETEST EXTENSION TESTS
#-----------------------------------------------------------------------------#

celeritas_setup_tests(SERIAL PREFIX testdetail)

celeritas_add_test(TestMacros.test.cc)

#-----------------------------------------------------------------------------#
# CORECEL TESTS
#-----------------------------------------------------------------------------#

celeritas_setup_tests(SERIAL PREFIX corecel)

celeritas_add_test(corecel/OpaqueId.test.cc)

# Cont
set(CELERITASTEST_PREFIX corecel/cont)
celeritas_add_test(corecel/cont/Array.test.cc)
celeritas_add_test(corecel/cont/InitializedValue.test.cc)
celeritas_add_test(corecel/cont/Span.test.cc)
celeritas_add_test(corecel/cont/LabelIdMultiMap.test.cc)
celeritas_add_device_test(corecel/cont/Range)

# Data
set(CELERITASTEST_PREFIX corecel/data)
celeritas_add_device_test(corecel/data/Collection)
celeritas_add_test(corecel/data/Copier.test.cc GPU)
celeritas_add_test(corecel/data/DeviceAllocation.test.cc GPU)
celeritas_add_test(corecel/data/DeviceVector.test.cc GPU)
celeritas_add_device_test(corecel/data/StackAllocator)

# Grid
set(CELERITASTEST_PREFIX corecel/grid)
celeritas_add_test(corecel/grid/Interpolator.test.cc)
celeritas_add_test(corecel/grid/NonuniformGrid.test.cc)
celeritas_add_test(corecel/grid/TwodGridCalculator.test.cc)
celeritas_add_test(corecel/grid/UniformGrid.test.cc)

# IO
set(CELERITASTEST_PREFIX corecel/io)
celeritas_add_test(corecel/io/EnumStringMapper.test.cc)
celeritas_add_test(corecel/io/Join.test.cc)
celeritas_add_test(corecel/io/Logger.test.cc)
celeritas_add_test(corecel/io/OutputRegistry.test.cc
  LINK_LIBRARIES ${_optional_json_link})
celeritas_add_test(corecel/io/Repr.test.cc)
celeritas_add_test(corecel/io/StringEnumMapper.test.cc)
celeritas_add_test(corecel/io/StringUtils.test.cc)

# Math
set(CELERITASTEST_PREFIX corecel/math)
celeritas_add_test(corecel/math/Algorithms.test.cc)
celeritas_add_test(corecel/math/ArrayUtils.test.cc)
celeritas_add_test(corecel/math/HashUtils.test.cc)
celeritas_add_device_test(corecel/math/NumericLimits)
celeritas_add_test(corecel/math/Quantity.test.cc
  LINK_LIBRARIES ${_optional_json_link})
celeritas_add_test(corecel/math/SoftEqual.test.cc)

# Sys
set(CELERITASTEST_PREFIX corecel/sys)
celeritas_add_test(corecel/sys/Environment.test.cc
  ENVIRONMENT "ENVTEST_ONE=1;ENVTEST_ZERO=0;ENVTEST_EMPTY="
  LINK_LIBRARIES ${_optional_json_link}
)
celeritas_add_test(corecel/sys/MpiCommunicator.test.cc
  NP ${CELERITASTEST_NP_DEFAULT})
celeritas_add_test(corecel/sys/MultiExceptionHandler.test.cc)
celeritas_add_test(corecel/sys/TypeDemangler.test.cc)
celeritas_add_test(corecel/sys/ScopedSignalHandler.test.cc)
celeritas_add_test(corecel/sys/ScopedStreamRedirect.test.cc)
celeritas_add_test(corecel/sys/Stopwatch.test.cc ADDED_TESTS _stopwatch)
set_tests_properties(${_stopwatch} PROPERTIES LABELS "nomemcheck")
celeritas_add_test(corecel/sys/Version.test.cc)

#-----------------------------------------------------------------------------#
# ORANGE TESTS
#-----------------------------------------------------------------------------#

celeritas_add_library(testcel_orange
  orange/OrangeGeoTestBase.cc
)
celeritas_target_link_libraries(testcel_orange
  PRIVATE Celeritas::testcel_harness Celeritas::orange
)

celeritas_setup_tests(SERIAL PREFIX orange
  LINK_LIBRARIES testcel_orange Celeritas::orange
)

#-----------------------------------------------------------------------------#
# Base
celeritas_add_test(orange/BoundingBox.test.cc)
celeritas_add_test(orange/Orange.test.cc)
celeritas_add_test(orange/Translator.test.cc)

# Base detail
celeritas_add_test(orange/detail/UnitIndexer.test.cc)

#-------------------------------------#
# Surfaces
set(CELERITASTEST_PREFIX orange/surf)
celeritas_add_test(orange/surf/detail/QuadraticSolver.test.cc)
celeritas_add_test(orange/surf/CylCentered.test.cc)
celeritas_add_test(orange/surf/GeneralQuadric.test.cc)
celeritas_add_test(orange/surf/PlaneAligned.test.cc)
celeritas_add_test(orange/surf/Sphere.test.cc)
celeritas_add_test(orange/surf/SphereCentered.test.cc)
celeritas_add_device_test(orange/surf/SurfaceAction)


#-------------------------------------#
# Universe details
set(CELERITASTEST_PREFIX orange/univ)
celeritas_add_test(orange/univ/detail/LogicEvaluator.test.cc)
celeritas_add_test(orange/univ/detail/LogicStack.test.cc)
celeritas_add_test(orange/univ/detail/SurfaceFunctors.test.cc)
celeritas_add_test(orange/univ/detail/SenseCalculator.test.cc)
celeritas_add_test(orange/univ/VolumeView.test.cc)
celeritas_add_device_test(orange/univ/SimpleUnitTracker)

#-----------------------------------------------------------------------------#
# CELERITAS TESTS
#-----------------------------------------------------------------------------#

celeritas_add_library(testcel_celeritas
  celeritas/grid/CalculatorTestBase.cc
  celeritas/phys/InteractionIO.cc
  celeritas/phys/InteractorHostTestBase.cc
  celeritas/phys/SecondaryIO.cc
  celeritas/phys/MockModel.cc
  celeritas/phys/MockProcess.cc
  celeritas/GeantTestBase.cc
  celeritas/GlobalTestBase.cc
  celeritas/GlobalGeoTestBase.cc
  celeritas/ImportedDataTestBase.cc
  celeritas/MockTestBase.cc
  celeritas/RootTestBase.cc
  celeritas/SimpleTestBase.cc
  celeritas/global/AlongStepTestBase.cc
  celeritas/global/StepperTestBase.cc
  celeritas/user/ExampleCalorimeters.cc
  celeritas/user/ExampleMctruth.cc
  celeritas/user/CaloTestBase.cc
  celeritas/user/MctruthTestBase.cc
)
celeritas_target_link_libraries(testcel_celeritas
  PRIVATE Celeritas::celeritas Celeritas::testcel_harness ${_optional_json_link}
)

celeritas_setup_tests(SERIAL PREFIX celeritas
  LINK_LIBRARIES testcel_celeritas Celeritas::celeritas
)

#-----------------------------------------------------------------------------#
# (Root level tests)
celeritas_add_test(celeritas/Constants.test.cc
  LINK_LIBRARIES ${_optional_geant4_link})

#-------------------------------------#
# EM
set(CELERITASTEST_PREFIX celeritas/em)
celeritas_add_test(celeritas/em/BetheHeitler.test.cc)
celeritas_add_test(celeritas/em/CombinedBrem.test.cc)
celeritas_add_test(celeritas/em/EPlusGG.test.cc)
celeritas_add_test(celeritas/em/Fluctuation.test.cc)
celeritas_add_test(celeritas/em/KleinNishina.test.cc)
celeritas_add_test(celeritas/em/LivermorePE.test.cc)
celeritas_add_test(celeritas/em/MollerBhabha.test.cc)
celeritas_add_test(celeritas/em/MuBremsstrahlung.test.cc)
celeritas_add_test(celeritas/em/Rayleigh.test.cc)
celeritas_add_test(celeritas/em/RelativisticBrem.test.cc)
celeritas_add_test(celeritas/em/SeltzerBerger.test.cc)
celeritas_add_test(celeritas/em/TsaiUrbanDistribution.test.cc)
celeritas_add_test(celeritas/em/UrbanMsc.test.cc ${_needs_root}
  ${_optional_geant4_env})

#-------------------------------------#
# External
set(CELERITASTEST_PREFIX celeritas/ext)

if(CELERITAS_USE_VecGeom)
  set(_vg_geant)
  if(CELERITAS_USE_Geant4)
    set(_vg_geant
      ${_needs_geant4}
      FILTER
        "-*Geant*"
        "FourLevelsGeantTest.*"
        "SolidsGeantTest.*"
    )
  else()
    set(_vg_geant ${_optional_geant4_env})
  endif()

  celeritas_add_device_test(celeritas/ext/Vecgeom
    LINK_LIBRARIES VecGeom::vecgeom
    ${_vg_geant}
  )
endif()

celeritas_add_test(celeritas/ext/EventReader.test.cc ${_needs_hepmc})
celeritas_add_test(celeritas/ext/GeantImporter.test.cc
  ${_needs_geant4} LINK_LIBRARIES ${_optional_json_link}
  FILTER
    "FourSteelSlabs*"
    "TestEm3*"
    "OneSteelSphere.*"
    "OneSteelSphereGG.*"
  )
celeritas_add_test(celeritas/ext/GeantVolumeMapper.test.cc ${_needs_geant4}
  LINK_LIBRARIES ${Geant4_LIBRARIES} Celeritas::orange)
celeritas_add_test(celeritas/ext/RootImporter.test.cc ${_needs_root})

#-------------------------------------#
# Field
set(CELERITASTEST_PREFIX celeritas/field)

celeritas_add_test(celeritas/field/Fields.test.cc
SOURCES
  celeritas/field/MagFieldMap.cc
  celeritas/field/CMSFieldMapReader.cc
)
celeritas_add_test(celeritas/field/Steppers.test.cc)
celeritas_add_test(celeritas/field/FieldDriver.test.cc)
celeritas_add_test(celeritas/field/FieldPropagator.test.cc ${_needs_geo})
celeritas_add_test(celeritas/field/LinearPropagator.test.cc ${_needs_geo})
celeritas_add_test(celeritas/field/MagFieldEquation.test.cc)

#-------------------------------------#
# Geo
set(CELERITASTEST_PREFIX celeritas/geo)

set(_geo_args GPU ${_needs_geo}
  SOURCES celeritas/geo/HeuristicGeoTestBase.cc)
if(CELERITAS_USE_CUDA OR CELERITAS_USE_HIP)
  list(APPEND _geo_args celeritas/geo/HeuristicGeoTestBase.cu)
endif()
if(CELERITAS_USE_VecGeom)
  list(APPEND _geo_args LINK_LIBRARIES VecGeom::vecgeom)
endif()
celeritas_add_test(celeritas/geo/Geometry.test.cc ${_geo_args})

celeritas_add_test(celeritas/geo/GeoMaterial.test.cc
  ${_needs_root} ${_needs_geo})

#-------------------------------------#
# Global
set(CELERITASTEST_PREFIX celeritas/global)
celeritas_add_test(celeritas/global/ActionRegistry.test.cc)

if(CELERITAS_USE_Geant4)
  set(_filter
    FILTER
      "-Em3*:SimpleCms*"
      "Em3AlongStepTest.nofluct_nomsc"
      "Em3AlongStepTest.msc_nofluct"
      "Em3AlongStepTest.msc_nofluct_finegrid"
      "Em3AlongStepTest.fluct_nomsc"
      "SimpleCmsAlongStepTest.msc_field"
      "SimpleCmsAlongStepTest.msc_field_finegrid"
  )
else()
  set(_filter)
endif()
celeritas_add_test(celeritas/global/AlongStep.test.cc
  NT 1 ${_optional_geant4_env} ${_filter}
)
celeritas_add_test(celeritas/global/KernelContextException.test.cc NT 1
  LINK_LIBRARIES ${_optional_json_link}
)
celeritas_add_test(celeritas/global/Stepper.test.cc
  GPU NT 4 ${_needs_geant4}
  FILTER
    # NOTE: these can be run in the same invocation once Geant4 reload works
    "TestEm3NoMsc.*"
    "TestEm3Msc.*"
    "TestEm3MscNofluct.*"
    "TestEm15MscField.*"
    "OneSteelSphere.*"
)

#-------------------------------------#
# Grid
set(CELERITASTEST_PREFIX celeritas/grid)
celeritas_add_test(celeritas/grid/GenericXsCalculator.test.cc)
celeritas_add_test(celeritas/grid/GridIdFinder.test.cc)
celeritas_add_test(celeritas/grid/InverseRangeCalculator.test.cc)
celeritas_add_test(celeritas/grid/PolyEvaluator.test.cc)
celeritas_add_test(celeritas/grid/RangeCalculator.test.cc)
celeritas_add_test(celeritas/grid/ValueGridBuilder.test.cc)
celeritas_add_test(celeritas/grid/ValueGridInserter.test.cc)
celeritas_add_test(celeritas/grid/VectorUtils.test.cc)
celeritas_add_test(celeritas/grid/XsCalculator.test.cc)

#-------------------------------------#
# IO
set(CELERITASTEST_PREFIX celeritas/io)
celeritas_add_test(celeritas/io/SeltzerBergerReader.test.cc ${_needs_geant4})

#-------------------------------------#
# Mat
set(CELERITASTEST_PREFIX celeritas/mat)
celeritas_add_test(celeritas/mat/ElementSelector.test.cc)
celeritas_add_device_test(celeritas/mat/Material)

#-------------------------------------#
# Phys
set(CELERITASTEST_PREFIX celeritas/phys)
celeritas_add_test(celeritas/phys/CutoffParams.test.cc)
celeritas_add_device_test(celeritas/phys/Particle)
celeritas_add_device_test(celeritas/phys/Physics)
celeritas_add_test(celeritas/phys/PhysicsStepUtils.test.cc)
celeritas_add_test(celeritas/phys/PrimaryGenerator.test.cc
  LINK_LIBRARIES ${_optional_json_link})
celeritas_add_test(celeritas/phys/ProcessBuilder.test.cc ${_needs_root}
  ${_optional_geant4_env})

#-----------------------------------------------------------------------------#
# Random
set(CELERITASTEST_PREFIX celeritas/random)

celeritas_add_device_test(celeritas/random/RngEngine)
celeritas_add_test(celeritas/random/Selector.test.cc)
celeritas_add_test(celeritas/random/XorwowRngEngine.test.cc GPU)

celeritas_add_test(celeritas/random/distribution/BernoulliDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/ExponentialDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/GammaDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/IsotropicDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/NormalDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/PoissonDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/RadialDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/ReciprocalDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/UniformBoxDistribution.test.cc)
celeritas_add_test(celeritas/random/distribution/UniformRealDistribution.test.cc)

if(CELERITAS_USE_CUDA)
  celeritas_add_test(celeritas/random/curand/CurandPerformance.test.cc GPU
    SOURCES celeritas/random/curand/CurandPerformance.test.cu
    LINK_LIBRARIES Celeritas::DeviceToolkit
    ${_disable_if_debug}
  )
endif()

#-------------------------------------#
# Track
set(CELERITASTEST_PREFIX celeritas/track)
celeritas_add_test(celeritas/track/Sim.test.cc ${_needs_geant4})
celeritas_add_device_test(celeritas/track/TrackInit ${_needs_device})

#-------------------------------------#
# User
set(CELERITASTEST_PREFIX celeritas/user)
celeritas_add_test(celeritas/user/DetectorSteps.test.cc GPU)
celeritas_add_test(celeritas/user/StepCollector.test.cc ${_optional_geant4_env})

#-----------------------------------------------------------------------------#
# ACCELERITAS TESTS
#-----------------------------------------------------------------------------#

if(CELERITAS_USE_Geant4)
  celeritas_setup_tests(SERIAL PREFIX accel
    LINK_LIBRARIES Celeritas::accel testcel_celeritas
  )

  celeritas_add_test(accel/ExceptionConverter.test.cc)
  celeritas_add_test(accel/detail/HitProcessor.test.cc
    ENVIRONMENT "${_geant4_test_env}")
endif()

#-----------------------------------------------------------------------------#
# DATA UPDATE
#-----------------------------------------------------------------------------#

set(_fourslabs "${CMAKE_CURRENT_SOURCE_DIR}/celeritas/data/four-steel-slabs")
set(_cmake_env)
if(CELERITAS_USE_Geant4)
  set(_cmake_env
    "${CMAKE_COMMAND}" "-E" "env" "${_geant4_test_env}"
  )
endif()
add_custom_target(update-four-slabs
  COMMAND
    ${_cmake_env}
    "$<TARGET_FILE:celer-export-geant>"
    "${_fourslabs}.gdml"
    "${_fourslabs}.geant.json"
    "${_fourslabs}.root"
  VERBATIM
)
add_dependencies(update-four-slabs celer-export-geant)

#-----------------------------------------------------------------------------#
