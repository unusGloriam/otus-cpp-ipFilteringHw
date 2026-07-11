cmake_minimum_required(VERSION 3.20)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION_VALUE 0.0.${PATCH_VERSION})

project(matrix VERSION ${PROJECT_VERSION_VALUE})

find_package(GTest REQUIRED)

add_library(matrix_core
        src/matrix.cpp
)

target_include_directories(matrix_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(matrix_core PUBLIC cxx_std_23)

add_executable(matrix main.cpp)
target_link_libraries(matrix PRIVATE matrix_core)
target_compile_features(matrix PRIVATE cxx_std_23)

function(enable_project_warnings target_name)
    if (MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -pedantic -Werror)
    endif()
endfunction()

enable_project_warnings(matrix_core)
enable_project_warnings(matrix)

install(TARGETS matrix RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)

#
# Тесты
#

add_executable(unit_tests
    tests/main.cpp
        tests/matrix_test.cpp
)
target_link_libraries(unit_tests
    PRIVATE
        matrix_core
        GTest::GTest
)
target_compile_features(unit_tests PRIVATE cxx_std_23)
enable_project_warnings(unit_tests)

# Включаем тестирование
enable_testing()
add_test(NAME Tests COMMAND unit_tests)
