find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module OPTIONAL_COMPONENTS Development.SABIModule)

find_package(nanobind CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
message(STATUS "Found nanobind: ${nanobind_DIR} (found version ${nanobind_VERSION})")

brokkr_python_package_init(PYVEFR pyvefr)

nanobind_add_module(_pyvefr NB_SUPPRESS_WARNINGS module.cpp pyvefr/bindings.cpp BACKEND_MODULE nanobind_backend)
target_link_libraries(_pyvefr PRIVATE vefr::vefr)
target_include_directories(_pyvefr PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

brokkr_python_module_rpaths(PYVEFR _pyvefr)

install(TARGETS _pyvefr DESTINATION "pyvefr" COMPONENT pyvefr)

set_property(TARGET vefr PROPERTY INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>")

brokkr_install_native_export(
    PKG_UPPER PYVEFR
    PACKAGE pyvefr
    CONFIG_NAME vefr
    NAMESPACE vefr::
    EXPORT_NAME pyvefrVefrTargets
    EXPORT_FILE vefrTargets.cmake
    TARGETS vefr
    HEADER_DIRS "${CMAKE_SOURCE_DIR}/include/vefr"
    CONFIG_TEMPLATE "${CMAKE_SOURCE_DIR}/Config.cmake.in"
    VERSION_FILE "${CMAKE_BINARY_DIR}/vefrConfigVersion.cmake"
)

brokkr_install_runtime_path_fixup(PYVEFR pyvefr)

set(PYVEFR_GENERATED_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/pyvefr/_version.py")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pyvefr")
file(WRITE "${PYVEFR_GENERATED_VERSION_FILE}" "__version__ = \"${PROJECT_VERSION}\"\n")
install(FILES "${PYVEFR_GENERATED_VERSION_FILE}" DESTINATION "pyvefr" COMPONENT pyvefr)

brokkr_install_python_files(pyvefr "${CMAKE_CURRENT_SOURCE_DIR}" FILES pyvefr/__init__.py pyvefr/py.typed)

brokkr_install_provider_env(PYVEFR pyvefr PREFIXES "${URD_NATIVE_PREFIX}")

brokkr_install_python_stubs(PYVEFR pyvefr MODULE pyvefr._pyvefr PRIVATE_MODULE _pyvefr RENAME_PACKAGES pyvefr)

add_custom_target(pyvefr_wheel DEPENDS _pyvefr)

# A downstream module, as a library built on vefr writes one (python/tests/test_downstream.py imports it from here).
if(VEFR_BUILD_TESTS)
    nanobind_add_module(_pyvefr_downstream_test NB_SUPPRESS_WARNINGS "${PROJECT_SOURCE_DIR}/python/tests/downstream/module.cpp" BACKEND_MODULE nanobind_backend)
    target_link_libraries(_pyvefr_downstream_test PRIVATE vefr::vefr)
endif()
