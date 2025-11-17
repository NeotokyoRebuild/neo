find_package(Git REQUIRED)

execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_LONGHASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
string(SUBSTRING "${GIT_LONGHASH}" 0 7 GIT_HASH)

execute_process(
    COMMAND git describe --tags --abbrev=0 --match v*
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_LATESTTAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get version number from GIT_LATESTTAG
# EX: v20.0-alpha -> 20 0
message(STATUS "GIT_LATESTTAG: ${GIT_LATESTTAG}")
if ("${GIT_LATESTTAG}" STREQUAL "")
    message(FATAL_ERROR "Failed to get git tag")
endif ()
string(REPLACE "-" ";" GIT_LATESTTAG_SPLIT "${GIT_LATESTTAG}")
list(GET GIT_LATESTTAG_SPLIT 0 GIT_LATESTTAG_0)
string(REPLACE "v" "" GIT_LATESTTAG_VERSION "${GIT_LATESTTAG_0}")
string(REPLACE "." ";" GIT_LATESTTAG_VERSION_SPLIT "${GIT_LATESTTAG_VERSION}")
list(GET GIT_LATESTTAG_VERSION_SPLIT 0 NEO_VERSION_MAJOR)
list(GET GIT_LATESTTAG_VERSION_SPLIT 1 NEO_VERSION_MINOR)
if ("${NEO_VERSION_MAJOR}" STREQUAL "")
    message(FATAL_ERROR "Failed to get NEO_VERSION_MAJOR, check if git tag formatted correctly")
endif ()
if ("${NEO_VERSION_MINOR}" STREQUAL "")
    message(FATAL_ERROR "Failed to get NEO_VERSION_MINOR, check if git tag formatted correctly")
endif ()
message(STATUS "NEO_VERSION_MAJOR: ${NEO_VERSION_MAJOR}")
message(STATUS "NEO_VERSION_MINOR: ${NEO_VERSION_MINOR}")

string(TIMESTAMP BUILD_DATE_SHORT "%Y%m%d")
string(TIMESTAMP BUILD_DATE_LONG "%Y-%m-%d")

set(OS_BUILD "${CMAKE_SYSTEM_NAME}")
set(COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
set(COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")

configure_file(
    ${CMAKE_SOURCE_DIR}/../game/neo/resource/GameMenu.res.in
    ${CMAKE_SOURCE_DIR}/../game/neo/resource/GameMenu.res
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_version_info.cpp.in
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_version_info.cpp
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_version_number.h.in
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_version_number.h
    @ONLY
)
