if (STANDARD_BUILD)
cmake_minimum_required(VERSION 3.20)
project(libnatkit-core LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(MSVC)
  message("------------------ Compiler is MSVC ------------------")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D__msvc_cplusplus /std:c++20")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c20")
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(BUILD_SHARED_LIBS ON)
else()
  message("------------------ Compiler is Something Else ------------------")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0 -std=c++20")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0")
  if (NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--disable-new-dtags")
  endif()
endif()

set(LIBNATKIT_CORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBNATKIT_CORE_INCLUDE "${LIBNATKIT_CORE_ROOT}/include")
set(LIBNATKIT_CORE_HEADER_FILES
  "${LIBNATKIT_CORE_ROOT}/include/libnatkit-core.hpp"
  )

set(LIBNATKIT_CORE_SRC_FILES
  "${LIBNATKIT_CORE_ROOT}/src/BasicMetaInfoSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/BasicTopicInformation.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/JsonDecoder.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/JsonEncoder.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/NatImuDataSchema.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/RawStream.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/Registry.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/SerializationType.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/Stream.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/StreamType.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/TopicMessenger.cpp"
  "${LIBNATKIT_CORE_ROOT}/src/TopicTranslator.cpp"
  )


add_subdirectory(third-party/cJSON)

add_library(libnatkit-core SHARED ${LIBNATKIT_CORE_SRC_FILES} ${LIBNATKIT_CORE_HEADER_FILES})
target_include_directories(libnatkit-core PRIVATE ${LIBNATKIT_CORE_INCLUDE})
target_include_directories(libnatkit-core PRIVATE third-party/cJSON)
target_link_libraries(libnatkit-core cjson)

install(TARGETS libnatkit-core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES ${LIBNATKIT_CORE_HEADER_FILES} DESTINATION include)

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

  INCLUDE_DIRS
  "include"

  REQUIRES
  json
  )
endif()
