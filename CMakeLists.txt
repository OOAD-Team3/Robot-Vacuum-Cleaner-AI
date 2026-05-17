cmake_minimum_required(VERSION 3.20)

project(RobotVacuumCleanerAI VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rvc_controller
    src/AutomaticCleaning.cpp
    src/CleaningPolicy.cpp
    src/Commands.cpp
    src/DustResponse.cpp
    src/RVCSWController.cpp
    src/SensorState.cpp
)

target_include_directories(rvc_controller
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_TESTING)
    find_package(GTest QUIET)

    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(rvc_controller_tests
        tests/RVCSWControllerTest.cpp
    )

    target_link_libraries(rvc_controller_tests
        PRIVATE
            rvc_controller
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(rvc_controller_tests)
endif()
