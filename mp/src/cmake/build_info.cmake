string(TIMESTAMP BUILD_DATE "%Y%m%d")
string(TIMESTAMP BUILD_DATETIME UTC)

find_package(Git REQUIRED)
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_LONGHASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
string(SUBSTRING "${GIT_HASH}" 0 7 GIT_HASH)

configure_file(
    ${CMAKE_SOURCE_DIR}/../game/neo/resource/GameMenu.preproc.res
    ${CMAKE_SOURCE_DIR}/../game/neo/resource/GameMenu.res
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_version_info.preproc.h
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_version_info.h
    @ONLY
)
