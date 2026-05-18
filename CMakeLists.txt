cmake_minimum_required(VERSION 3.20)

project(RobotVacuumCleanerAI VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(FetchContent)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(asio)

add_library(asio_standalone INTERFACE)
add_library(asio::asio ALIAS asio_standalone)
target_include_directories(asio_standalone
    INTERFACE
        ${asio_SOURCE_DIR}/asio/include
)

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

add_library(rvc_app_support
    src/net/TcpServer.cpp
    src/protocol/CommandHandler.cpp
    src/protocol/CommandParser.cpp
    src/protocol/StateFormatter.cpp
    src/sim/ControllerStateSnapshot.cpp
    src/sim/RecordingDevices.cpp
    src/sim/RobotVacuumApplication.cpp
)

target_link_libraries(rvc_app_support
    PUBLIC
        rvc_controller
    PRIVATE
        asio::asio
        Threads::Threads
)

target_compile_definitions(rvc_app_support
    PRIVATE
        ASIO_STANDALONE
)

if(WIN32)
    target_compile_definitions(rvc_app_support
        PRIVATE
            _WIN32_WINNT=0x0601
    )
    target_link_libraries(rvc_app_support
        PRIVATE
            ws2_32
            mswsock
    )
endif()

add_executable(rvc_app
    app/rvc_app/main.cpp
)

target_link_libraries(rvc_app
    PRIVATE
        rvc_app_support
)

if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

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
