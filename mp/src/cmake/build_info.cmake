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
    ERROR_QUIET
)

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
