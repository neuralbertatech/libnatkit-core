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
  "${LIBNATKIT_CORE_ROOT}/include/CircularBuffer.hpp"
  "${LIBNATKIT_CORE_ROOT}/include/SensorBuffer.hpp"
  )

set(LIBNATKIT_CORE_SRC_FILES
  "${LIBNATKIT_CORE_ROOT}/src/BasicMetaInfoSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/BasicTopicInformation.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/JsonDecoder.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/JsonEncoder.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatImuDataSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatImuBulkDataSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/RawStream.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/Registry.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/SerializationType.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/Stream.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/StreamType.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/TopicMessenger.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/TopicTranslator.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/main.cpp"
  )


#add_library(libnatkit-core SHARED ${LIBNATKIT_CORE_SRC_FILES})
add_executable(libnatkit-core ${LIBNATKIT_CORE_SRC_FILES})
set_target_properties(libnatkit-core PROPERTIES PUBLIC_HEADER ${LIBNATKIT_CORE_HEADER_FILES})
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

else()
idf_component_register(
  SRCS 
  "src/BasicMetaInfoSchema.cpp"
  "src/BasicTopicInformation.cpp"
  "src/JsonDecoder.cpp"
  "src/JsonEncoder.cpp"
  "src/RawStream.cpp"
  "src/Registry.cpp"
  "src/SerializationType.cpp"
  "src/Stream.cpp"
  "src/StreamType.cpp"
  "src/TopicMessenger.cpp"
  "src/TopicTranslator.cpp"
  "src/main.cpp"

  INCLUDE_DIRS
  "include"

  REQUIRES
  json
  )
endif()
