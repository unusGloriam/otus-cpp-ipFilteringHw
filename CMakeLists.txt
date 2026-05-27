cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

project(ip_filter VERSION ${PROJECT_VESRION})

find_package(GTest REQUIRED)

option(WITH_BOOST_TEST "Whether to build Boost test" ON)

configure_file(version.h.in version.h)

add_executable(ip_filter_cli ip_filter.cpp)
add_library(ip_filter lib.cpp ip_filter_utils.cpp)

set_target_properties(ip_filter_cli ip_filter PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(ip_filter
    PRIVATE "${CMAKE_BINARY_DIR}"
)

target_link_libraries(ip_filter_cli PRIVATE
    ip_filter
)

if(WITH_BOOST_TEST)
    find_package(Boost COMPONENTS unit_test_framework REQUIRED)
    add_executable(test_version test_version.cpp)

    set_target_properties(test_version PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
    )

    set_target_properties(test_version PROPERTIES
        COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
        INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR}
    )

    target_link_libraries(test_version
        ${Boost_LIBRARIES}
        ip_filter
    )
endif()

if (MSVC)
    target_compile_options(ip_filter_cli PRIVATE
        /W4
    )
    target_compile_options(ip_filter PRIVATE
        /W4
    )
    if(WITH_BOOST_TEST)
        target_compile_options(test_version PRIVATE
            /W4
        )
    endif()
else ()
    target_compile_options(ip_filter_cli PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    target_compile_options(ip_filter PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    if(WITH_BOOST_TEST)
        target_compile_options(test_version PRIVATE
            -Wall -Wextra -pedantic -Werror
        )
    endif()
endif()

install(TARGETS ip_filter_cli RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)

if(WITH_BOOST_TEST)
    enable_testing()
    add_test(test_version test_version)
endif()

#
# Тесты
#

# Рекурсивно ищем все используемые в тестах .cpp файлы
file(GLOB TEST_SRC_FILES "${CMAKE_SOURCE_DIR}/tests/*.cpp")

add_executable(unit_tests "${TEST_SRC_FILES}")
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unit_tests
        PRIVATE
        ip_filter
        GTest::GTest
        GTest::Main)

# Включаем тестирование
enable_testing()
add_test(NAME Tests COMMAND unit_tests)