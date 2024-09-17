cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    CppLox
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    CppLox_lib OBJECT
    source/chunk.cpp
    source/compiler.cpp
    source/debug.cpp
    source/memory.cpp
    source/object.cpp
    source/scanner.cpp
    source/table.cpp
    source/value.cpp
    source/dispatcher.cpp
    source/vm.cpp
)

target_include_directories(
    CppLox_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(CppLox_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(CppLox_exe source/main.cpp)
add_executable(CppLox::exe ALIAS CppLox_exe)

set_property(TARGET CppLox_exe PROPERTY OUTPUT_NAME CppLox)

target_compile_features(CppLox_exe PRIVATE cxx_std_17)

target_link_libraries(CppLox_exe PRIVATE CppLox_lib)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(CppLox_exe PRIVATE OpenMP::OpenMP_CXX)
endif()

# ---- Install rules ----



if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT CppLox_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of CppLox"
  )
endif()

include(cmake/dev-mode.cmake)
