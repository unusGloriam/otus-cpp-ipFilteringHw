cmake_minimum_required(VERSION 3.10)
project(kafkaordersender VERSION 8.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# ПУТИ К БИБЛИОТЕКАМ (из vcpkg)
# ============================================================

set(VCPKG_ROOT "C:/vcpkg/installed/x64-windows")

# ============================================================
# ИСХОДНЫЕ ФАЙЛЫ
# ============================================================

set(SOURCES
    src/main.cpp
    src/config/config.cpp
    src/kafka/producer.cpp
    src/kafka/consumer.cpp
    src/parser/json_parser.cpp
    src/cache/sqlite_cache.cpp
    src/database/postgresql.cpp
	src/processor/order_processor.cpp
    src/utils/logger.cpp
)

set(HEADERS
    src/config/config.hpp
    src/kafka/producer.hpp
    src/kafka/consumer.hpp
    src/parser/json_parser.hpp
    src/cache/sqlite_cache.hpp
    src/database/postgresql.hpp
	src/processor/order_processor.hpp
    src/utils/logger.hpp
    src/utils/uuid.hpp
)

# ============================================================
# ИСПОЛНЯЕМЫЙ ФАЙЛ
# ============================================================

add_executable(kafka_order_sender ${SOURCES} ${HEADERS})

# Пути для заголовочных файлов
target_include_directories(kafka_order_sender PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${VCPKG_ROOT}/include
)

# Пути для библиотек
target_link_directories(kafka_order_sender PRIVATE
    ${VCPKG_ROOT}/lib
)

# Линковка библиотек
target_link_libraries(kafka_order_sender
    PRIVATE
    rdkafka++          # C++ библиотека для Kafka
    rdkafka            # C библиотека для Kafka  
    sqlite3            # SQLite3
    pqxx               # PostgreSQL C++ библиотека
)

# Для MSVC
if(MSVC)
    target_compile_definitions(kafka_order_sender PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ============================================================
# УСТАНОВКА
# ============================================================

install(TARGETS kafka_order_sender RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION config)
install(DIRECTORY input/ DESTINATION input)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/input" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
# Копируем только .json файлы из корня input (без подпапок)
file(GLOB INPUT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/input/*.json")
foreach(FILE ${INPUT_FILES})
    get_filename_component(FILENAME ${FILE} NAME)
    file(COPY ${FILE} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release/input/")
endforeach()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Release/cache")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/input" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
# Копируем только .json файлы из корня input (без подпапок)
file(GLOB INPUT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/input/*.json")
foreach(FILE ${INPUT_FILES})
    get_filename_component(FILENAME ${FILE} NAME)
    file(COPY ${FILE} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug/input/")
endforeach()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Debug/cache")