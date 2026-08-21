if (STANDARD_BUILD)
cmake_minimum_required(VERSION 3.20)
project(libnatkit-core LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)

include(GNUInstallDirs)

if(MSVC)
  message("------------------ Compiler is MSVC ------------------")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D__msvc_cplusplus /std:c++11")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c20")
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(BUILD_SHARED_LIBS ON)
else()
  message("------------------ Compiler is Something Else ------------------")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0 -std=c++11 -DSERVER")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0")
  if (NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--disable-new-dtags")
  endif()
endif()

set(LIBNATKIT_CORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBNATKIT_CORE_THIRD_PARTY_DIR "${LIBNATKIT_CORE_ROOT}/third-party")
set(LIBNATKIT_CORE_INCLUDE "${LIBNATKIT_CORE_ROOT}/include")
set(LIBNATKIT_CORE_HEADER_FILES
  "${LIBNATKIT_CORE_ROOT}/include/libnatkit-core.hpp"
  "${LIBNATKIT_CORE_ROOT}/include/libnatkit-core-abi.h"
  "${LIBNATKIT_CORE_ROOT}/include/CircularBuffer.hpp"
  "${LIBNATKIT_CORE_ROOT}/include/SensorBuffer.hpp"
  )

set(LIBNATKIT_CORE_SRC_FILES
  "${LIBNATKIT_CORE_ROOT}/src/BasicMetaInfoSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/BasicTopicInformation.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/DataSchemaDescriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/ExgPillEmgDataSchemaV1.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/ExgPillEmgDataSchemaV1Descriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/ExgPillEmgTransformDataSchemaV1.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/ExgPillEmgTransformDataSchemaV1Descriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatSignalFrameDataSchemaV1.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatSignalFrameDataSchemaV1Descriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/JsonDecoder.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/JsonEncoder.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/MarkerEventV1.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/MetaRecord.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatImuDataSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatImuDataSchemaDescriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatImuBulkDataSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatKitNodeStatusV1Schema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatKitNodeStatusV1Descriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatKitPrimaryStatusV1Schema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatKitPrimaryStatusV1Descriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatImuBulkDataSchemaDescriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatMuseDataSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatMuseDataSchemaDescriptor.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatMuseBulkDataSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/RawStream.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/Registry.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/SessionMetadataRecord.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/SerializationType.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/Stream.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/StreamStitch.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/StreamType.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/TopicMessenger.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/TopicTranslator.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/TransformProvenanceRecord.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/main.cpp"
  )


add_library(libnatkit-core SHARED ${LIBNATKIT_CORE_SRC_FILES})
#add_executable(libnatkit-core ${LIBNATKIT_CORE_SRC_FILES})
set_target_properties(libnatkit-core PROPERTIES PUBLIC_HEADER "${LIBNATKIT_CORE_HEADER_FILES}")
target_include_directories(libnatkit-core
    PUBLIC $<BUILD_INTERFACE:${LIBNATKIT_CORE_INCLUDE}>
    PUBLIC $<BUILD_INTERFACE:${LIBNATKIT_CORE_THIRD_PARTY_DIR}/nlohmann/include>
)

add_library(nlohmann INTERFACE)
target_include_directories(nlohmann INTERFACE ${LIBNATKIT_CORE_THIRD_PARTY_DIR}/nlohmann/include)
target_link_libraries(libnatkit-core PUBLIC nlohmann)

install(TARGETS libnatkit-core
        CONFIGURATIONS Debug
        RUNTIME
          DESTINATION ${CMAKE_INSTALL_BINDIR}/Debug
        LIBRARY
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/Debug
        ARCHIVE
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/Debug
        INCLUDES
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Debug
        PUBLIC_HEADER
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Debug
)

install(TARGETS libnatkit-core
        CONFIGURATIONS Release RelWithDebInfo MinSizeRel
        RUNTIME
          DESTINATION ${CMAKE_INSTALL_BINDIR}/Release
        LIBRARY
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/Release
        ARCHIVE
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/Release
        INCLUDES
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Release
        PUBLIC_HEADER
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Release
)

# Off by default so the deployed Docker build (cmake --build) stays lean; CI and
# local dev turn it on with -DLIBNATKIT_CORE_BUILD_TESTS=ON.
option(LIBNATKIT_CORE_BUILD_TESTS "Build libnatkit-core ABI tests" OFF)
if(LIBNATKIT_CORE_BUILD_TESTS)
  enable_testing()
  add_executable(stream_id_abi_test "${LIBNATKIT_CORE_ROOT}/tests/stream_id_abi_test.cpp")
  target_link_libraries(stream_id_abi_test PRIVATE libnatkit-core nlohmann)
  target_compile_definitions(stream_id_abi_test PRIVATE
    NAT_GOLDEN_FIXTURE_PATH="${LIBNATKIT_CORE_ROOT}/tests/stream_id_golden.json")
  add_test(NAME stream_id_abi_test COMMAND stream_id_abi_test)

  add_executable(imu_frame_version_test "${LIBNATKIT_CORE_ROOT}/tests/imu_frame_version_test.cpp")
  target_link_libraries(imu_frame_version_test PRIVATE libnatkit-core nlohmann)
  add_test(NAME imu_frame_version_test COMMAND imu_frame_version_test)

  add_executable(device_status_schema_test "${LIBNATKIT_CORE_ROOT}/tests/device_status_schema_test.cpp")
  target_link_libraries(device_status_schema_test PRIVATE libnatkit-core nlohmann)
  target_compile_definitions(device_status_schema_test PRIVATE
    NAT_DEVICE_STATUS_FIXTURE_PATH="${LIBNATKIT_CORE_ROOT}/tests/device_status_golden.json")
  add_test(NAME device_status_schema_test COMMAND device_status_schema_test)
endif()

else()
idf_component_register(
  SRCS
  "src/BasicMetaInfoSchema.cpp"
  "src/BasicTopicInformation.cpp"
  "src/DataSchemaDescriptor.cpp"
  "src/ExgPillEmgDataSchemaV1.cpp"
  "src/ExgPillEmgDataSchemaV1Descriptor.cpp"
  "src/ExgPillEmgTransformDataSchemaV1.cpp"
  "src/ExgPillEmgTransformDataSchemaV1Descriptor.cpp"
  "src/NatSignalFrameDataSchemaV1.cpp"
  "src/NatSignalFrameDataSchemaV1Descriptor.cpp"
  "src/JsonDecoder.cpp"
  "src/JsonEncoder.cpp"
  "src/MarkerEventV1.cpp"
  "src/MetaRecord.cpp"
  "src/NatImuDataSchema.cpp"
  "src/NatImuDataSchemaDescriptor.cpp"
  "src/NatImuBulkDataSchema.cpp"
  "src/NatImuBulkDataSchemaDescriptor.cpp"
  "src/NatMuseDataSchema.cpp"
  "src/NatMuseDataSchemaDescriptor.cpp"
  "src/NatMuseBulkDataSchema.cpp"
  "src/RawStream.cpp"
  "src/Registry.cpp"
  "src/SessionMetadataRecord.cpp"
  "src/SerializationType.cpp"
  "src/Stream.cpp"
  "src/StreamStitch.cpp"
  "src/StreamType.cpp"
  "src/TopicMessenger.cpp"
  "src/TopicTranslator.cpp"
  "src/TransformProvenanceRecord.cpp"
  "src/main.cpp"

  INCLUDE_DIRS
  "include"

  REQUIRES
  json
  )

# Relax some warnings for ESP-IDF build - libnatkit-core was written for less strict compilers
target_compile_options(${COMPONENT_LIB} PRIVATE
  -Wno-error=switch
  -Wno-error=pessimizing-move
  -Wno-error=redundant-move
  -Wno-error=deprecated-copy
  -Wno-error=implicit-fallthrough
)
endif()
