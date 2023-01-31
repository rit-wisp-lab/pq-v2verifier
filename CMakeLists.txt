cmake_minimum_required(VERSION 3.20)
project(pq-v2verifier)

set(SOURCE_FILES
    src/main.cpp
    src/Vehicle.cpp
    src/VehicleUtil.cpp
    src/v2vcrypto.cpp
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/include/oqs)

target_link_libraries(${PROJECT_NAME} crypto oqs)
