cmake_minimum_required(VERSION 3.21)

# brokkr (pybrokkr): build helpers, found as every dependency is: its prefix asked of the configuring Python (the
# venv's, -DPython_EXECUTABLE; scikit-build-core sets it in wheel builds), put on CMAKE_PREFIX_PATH, nowhere else searched.
if(NOT Python_EXECUTABLE)
    message(FATAL_ERROR "brokkr: no Python_EXECUTABLE: configure with -DPython_EXECUTABLE=<venv>/bin/python of a venv that has pybrokkr.")
endif()
execute_process(COMMAND "${Python_EXECUTABLE}" -m pybrokkr --cmake-dir
                OUTPUT_VARIABLE _brokkr_dir OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _brokkr_result)
if(NOT _brokkr_result EQUAL 0)
    message(FATAL_ERROR "brokkr: ${Python_EXECUTABLE} has no pybrokkr: configure with -DPython_EXECUTABLE=<venv>/bin/python of a venv that has it.")
endif()
cmake_path(GET _brokkr_dir PARENT_PATH _brokkr_prefix)
list(PREPEND CMAKE_PREFIX_PATH "${_brokkr_prefix}")
find_package(brokkr CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
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
