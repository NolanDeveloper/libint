cmake_minimum_required(VERSION 3.2)

project(libint)

include(CTest)
include(FetchContent)

set(CMAKE_C_STANDARD 11)

add_subdirectory(include)
add_subdirectory(src)

if(BUILD_TESTING)
    FetchContent_Declare(
            acutest
            GIT_REPOSITORY https://github.com/mity/acutest
            GIT_TAG        cce300734bfe5c3879b0449ac283a872633e615c
    )
    FetchContent_MakeAvailable(acutest)

    add_subdirectory(test-unit)
endif()