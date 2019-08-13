cmake_minimum_required(VERSION 3.14)
project(lotr_backend)
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE CXX STANDARD REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
            "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
            FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_COMPILER /usr/local/gcc-9.1/bin/g++-9.1)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(TOP_DIR ${CMAKE_SOURCE_DIR})
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# only support linux/g++ for now
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRAPIDJSON_SSE42 -DSPDLOG_COMPILED_LIB -DCATCH_CONFIG_FAST_COMPILE -Wall -Wextra -Wno-unused-variable -Wno-long-long -Wno-unused-parameter -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -pedantic -std=c++2a ")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -ggdb -Og -mavx -mavx2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -mavx -mavx2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g3 -ggdb -O2 -mavx -mavx2")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -mavx -mavx2")

file(GLOB_RECURSE PROJECT_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB SPDLOG_SOURCE ${EXTERNAL_DIR}/spdlog/src/spdlog.cpp)
file(GLOB_RECURSE USOCKETS_SOURCE ${EXTERNAL_DIR}/uWebSockets/uSockets/src/*.c)
add_executable(lotr_backend ${SPDLOG_SOURCE} ${USOCKETS_SOURCE} ${PROJECT_SOURCES})

file(GLOB_RECURSE PROJECT_SOURCES_WITHOUT_MAIN ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM PROJECT_SOURCES_WITHOUT_MAIN "${PROJECT_SOURCE_DIR}/src/main.cpp")
file(GLOB PROJECT_TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp ${PROJECT_SOURCE_DIR}/test/test_helpers/*.cpp)
add_executable(lotr_test ${SPDLOG_SOURCE} ${USOCKETS_SOURCE} ${PROJECT_SOURCES_WITHOUT_MAIN} ${PROJECT_TEST_SOURCES})
add_test(NAME MyTest COMMAND lotr_test)

find_library(PQXX_LIBRARY pqxx HINTS ${EXTERNAL_DIR}/libpqxx/src/.libs)

if(NOT PQXX_LIBRARY)
    message(FATAL_ERROR "libpqxx not found")
endif()

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories("${TOP_DIR}/src")
include_directories("${EXTERNAL_DIR}/spdlog/include")
include_directories("${EXTERNAL_DIR}/rapidjson/include")
include_directories("${EXTERNAL_DIR}/uWebSockets/src")
include_directories("${EXTERNAL_DIR}/uWebSockets/uSockets/src")
include_directories("${EXTERNAL_DIR}/libpqxx/include")
include_directories("${EXTERNAL_DIR}/range-v3/include")
include_directories("${EXTERNAL_DIR}/Catch2/single_include")
include_directories("${ZLIB_INCLUDE_DIRS}")



target_link_libraries(lotr_backend PUBLIC ${PQXX_LIBRARY} -lpq)
target_link_libraries(lotr_backend PUBLIC -lpthread)
target_link_libraries(lotr_backend PUBLIC ${ZLIB_LIBRARIES} )
target_link_libraries(lotr_backend PUBLIC ${OPENSSL_LIBRARIES})

target_link_libraries(lotr_test PUBLIC ${PQXX_LIBRARY} -lpq)
target_link_libraries(lotr_test PUBLIC -lpthread)
target_link_libraries(lotr_test PUBLIC ${ZLIB_LIBRARIES} )
target_link_libraries(lotr_test PUBLIC ${OPENSSL_LIBRARIES})