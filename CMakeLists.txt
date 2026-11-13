cmake_minimum_required(VERSION 3.21)

# brokkr (pybrokkr): build helpers, found through scikit-build-core in wheel builds, else asked of the Python that has it.
find_package(brokkr QUIET)
if(NOT brokkr_FOUND)
    if(Python_EXECUTABLE)
        set(_brokkr_python "${Python_EXECUTABLE}")
    else()
        find_program(_brokkr_python NAMES python3 python REQUIRED)
    endif()
    execute_process(COMMAND "${_brokkr_python}" -m pybrokkr --cmake-dir
                    OUTPUT_VARIABLE _brokkr_dir OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    set(brokkr_DIR "${_brokkr_dir}" CACHE PATH "The directory holding brokkrConfig.cmake" FORCE)
    find_package(brokkr REQUIRED)
endif()
brokkr_read_project_version(VEFR_PROJECT_VERSION "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml")

project(vefr VERSION "${VEFR_PROJECT_VERSION}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

option(VEFR_BUILD_PYVEFR "Build the pyvefr Python bindings." OFF)
option(VEFR_BUILD_TESTS "Build the vefr tests." OFF)
option(VEFR_STDLIB_ASSERTIONS "Check bounds and preconditions of the standard library in Debug builds." ON)

if(VEFR_STDLIB_ASSERTIONS)
    add_compile_definitions(
        $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>
        "$<$<CONFIG:Debug>:_LIBCPP_HARDENING_MODE=(_LIBCPP_HARDENING_MODE_EXTENSIVE)>"
    )
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if(NOT MSVC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    # Shadowed names hide the variable a statement meant; CI turns every warning into an error.
    add_compile_options(-Wall -Wextra -Wshadow)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC 13+ flags views returned from temporary views; views never refer to the temporary.
        add_compile_options(-Wno-dangling-reference)
    endif()
endif()

# Dependencies: urd and, from urd's native prefix, nauty.

brokkr_find_python_native_prefix(pyurd URD_PREFIX)
brokkr_find_python_native_prefix(pyurd_native URD_NATIVE_PREFIX)
brokkr_register_python_native_runtime_prefix("pyurd_native" "${URD_NATIVE_PREFIX}")

find_package(urd CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
message(STATUS "Found urd: ${urd_DIR} (found version ${urd_VERSION})")

if(VEFR_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(src)

if(VEFR_BUILD_PYVEFR)
    add_subdirectory(python/src)
endif()

if(VEFR_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Installation and package export

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/vefr" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT vefr)

brokkr_install_locator(DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vefr/cmake" COMPONENT vefr)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/vefrConfigVersion.cmake"
    VERSION ${vefr_VERSION}
    COMPATIBILITY SameMinorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vefrConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vefr"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/vefrConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/vefrConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vefr"
    COMPONENT vefr
)
