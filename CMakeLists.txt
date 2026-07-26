cmake_minimum_required(VERSION 3.10)
project(fashion-mnist VERSION 6.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги компиляции
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
endif()

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/logistic_regression.cpp
    src/utils.cpp
)

set(HEADERS
    src/logistic_regression.hpp
    src/utils.hpp
)

# Исполняемый файл
add_executable(fashion-mnist ${SOURCES} ${HEADERS})

target_include_directories(fashion-mnist PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Установка
install(TARGETS fashion-mnist RUNTIME DESTINATION bin)