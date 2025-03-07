project(helloworld_cli)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

project(${PROJECT_NAME} VERSION ${PROJECT_VESRION})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/version_api/version.h.in version.h)

file(GLOB_RECURSE SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} src/main.cpp)
add_library(version_lib STATIC src/version_api.cpp)

target_include_directories(version_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(version_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/version_api)
target_include_directories(version_lib PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(${PROJECT_NAME} PUBLIC version_lib)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME helloworld)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)

if(UNITTEST)
        include(unittest.cmake)
endif()

