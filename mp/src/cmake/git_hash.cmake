set(GIT_HASH "unknown")
string(TIMESTAMP BUILD_DATE "%Y%m%d")

find_package(Git QUIET)
if (GIT_FOUND)
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/../game/neo/resource/GameMenu.preproc.res
    ${CMAKE_SOURCE_DIR}/../game/neo/resource/GameMenu.res
    @ONLY
)
