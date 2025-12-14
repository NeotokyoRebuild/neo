find_package(Git REQUIRED)

execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_LONGHASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
string(SUBSTRING "${GIT_LONGHASH}" 0 7 GIT_HASH)

set(UPSTREAM_URL "https://github.com/NeotokyoRebuild/neo")
execute_process(
    COMMAND git remote get-url origin
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE THIS_REMOTE_URL
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Must fetch tags from upstream in forked repos to guarantee the "git describe --tags (...)" command works
if (NOT "${THIS_REMOTE_URL}" STREQUAL "${UPSTREAM_URL}")
execute_process(
    COMMAND git fetch ${UPSTREAM_URL} --force --tags
)
endif()
execute_process(
    COMMAND git describe --tags --abbrev=0 --match v*
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_LATESTTAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "GIT_LATESTTAG: ${GIT_LATESTTAG}")
if ("${GIT_LATESTTAG}" STREQUAL "")
    message(FATAL_ERROR "Failed to get git tag")
endif ()

# Get version number from GIT_LATESTTAG
# EX: v20.0-alpha -> 20 0
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

# For tagged releases ("v<major>.<minor>..."), never treat warnings as errors to guarantee build generation.
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    execute_process(
        COMMAND git tag --points-at
        OUTPUT_VARIABLE GIT_HEADTAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if("${GIT_HEADTAG}" STREQUAL "${GIT_LATESTTAG}")
        set(DEFAULT_WARN_AS_ERR OFF)
    endif()
endif()
if(NOT DEFINED DEFAULT_WARN_AS_ERR)
    # If the branch name or the latest commit message contains the phrase "nwae"
    # (short for: no-warnings-as-errors), allow CI to skip warnings-as-errors for the build.
    # This is inteded as means for devs to temporarily dodge the warning rules in exceptional
    # situations (breakage from compiler version changes etc), but the preferred solution almost
    # always is to actually fix the warning.
    execute_process(
        COMMAND git log --oneline -n 1
        OUTPUT_VARIABLE GIT_LATESTCOMMIT_MSG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if("${GIT_LATESTCOMMIT_MSG}" MATCHES ".*[Nn][Ww][Aa][Ee].*")
        set(DEFAULT_WARN_AS_ERR OFF)
    else()
        set(DEFAULT_WARN_AS_ERR ON)
    endif()
endif()

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
