cmake_minimum_required(VERSION 3.30)
project(AetherRouter LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)

################################################################################
# Versioning
################################################################################
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git status --porcelain
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_STATUS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(GIT_STATUS)
    set(GIT_DIRTY "Dirty")
else()
    set(GIT_DIRTY "Clean")
endif()

################################################################################
# Aether
################################################################################
add_subdirectory(${EXTERNAL_DIR}/aether)

################################################################################
# AetherRouter
################################################################################
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(SRCS
    ${SRC_DIR}/aether_router.cc
    ${SRC_DIR}/tcp_server.cc
)

configure_file(
    ${INCLUDE_DIR}/version.h.in
    ${INCLUDE_DIR}/version.h
)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
        ${SRCS}
)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${EXTERNAL_DIR}/asio/asio/include
        ${EXTERNAL_DIR}/tomlplusplus/include
        ${INCLUDE_DIR}
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall -Wextra -Werror -Wpedantic -fdiagnostics-color=always -fPIE
    )
# Add flags for other compilers here
endif()
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Aether::Aether
)

################################################################################
# CI and tools
################################################################################
set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/tools)
set(CPPCHECK_SOURCES ${SRCS})
set(UNCRUSTIFY_SOURCES ${CPPCHECK_SOURCES})
set(UNCRUSTIFY_INC_DIRS ${INCLUDE_DIR})
if (CPPCHECK_SOURCES)
    include(${TOOLS_DIR}/cppcheck/cppcheck.cmake)
endif()
if (CPPCHECK_SOURCES AND UNCRUSTIFY_INC_DIRS)
    set(UNCRUSTIFY_SOURCES ${CPPCHECK_SOURCES})
    include(${TOOLS_DIR}/uncrustify/uncrustify.cmake)
endif()
