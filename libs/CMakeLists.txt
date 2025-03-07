project(libs)

if(UNITTEST)
    add_subdirectory(gtest_framework)
    include(CTest)
endif()

