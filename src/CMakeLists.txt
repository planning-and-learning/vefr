add_library(vefr SHARED nauty.cpp)
add_library(vefr::vefr ALIAS vefr)

target_compile_features(vefr PUBLIC cxx_std_20)
target_include_directories(vefr
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(vefr PUBLIC urd::urd)

find_path(VEFR_NAUTY_INCLUDE_DIR NAMES nauty.h nausparse.h REQUIRED)
find_library(VEFR_NAUTY_LIBRARY NAMES nauty REQUIRED)
message(STATUS "Found nauty: ${VEFR_NAUTY_LIBRARY}")
target_include_directories(vefr PRIVATE "${VEFR_NAUTY_INCLUDE_DIR}")
target_link_libraries(vefr PRIVATE "${VEFR_NAUTY_LIBRARY}")

if(APPLE)
    set_target_properties(vefr PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(vefr PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

install(
    TARGETS vefr
    EXPORT vefrTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT vefr
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT vefr
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT vefr
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT vefrTargets NAMESPACE vefr:: FILE "vefrTargets.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vefr" COMPONENT vefr)
export(EXPORT vefrTargets NAMESPACE vefr:: FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/vefrTargets.cmake")
