cmake_minimum_required(VERSION 3.24)

project(takane
    VERSION 0.9.0
    DESCRIPTION "ArtifactDB file validators"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(takane INTERFACE)
add_library(artifactdb::takane ALIAS takane)

include(GNUInstallDirs)
target_include_directories(takane INTERFACE 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/artifactdb_takane>")

option(TAKANE_FETCH_EXTERN "Automatically fetch takane's external dependencies." ON)
if(TAKANE_FETCH_EXTERN)
    add_subdirectory(extern)
else()
    find_package(artifactdb_ritsuko 0.6.0 CONFIG REQUIRED)
    find_package(artifactdb_uzuki2 2.0.0 CONFIG REQUIRED)
    find_package(artifactdb_chihaya 1.1.0 CONFIG REQUIRED)
endif()

target_link_libraries(takane INTERFACE artifactdb::ritsuko artifactdb::chihaya artifactdb::uzuki2)

option(TAKANE_FIND_HDF5 "Try to find and link to HDF5 for takane." ON)
if(TAKANE_FIND_HDF5)
    find_package(HDF5 COMPONENTS C CXX)
    if (HDF5_FOUND)
        target_link_libraries(takane INTERFACE hdf5::hdf5 hdf5::hdf5_cpp)
    endif()
endif()

# Building the test-related machinery, if we are compiling this library directly.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(TAKANE_TESTS "Build takane's test suite." ON)
else()
    option(TAKANE_TESTS "Build takane's test suite." OFF)
endif()

if(TAKANE_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif() 
endif()

# Setting up the installation commands.
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/artifactdb_takane)

install(TARGETS takane 
    EXPORT takaneTargets)

install(EXPORT takaneTargets
    FILE artifactdb_takaneTargets.cmake
    NAMESPACE artifactdb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/artifactdb_takane)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/artifactdb_takaneConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/artifactdb_takane)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/artifactdb_takaneConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/artifactdb_takaneConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/artifactdb_takaneConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/artifactdb_takane)
