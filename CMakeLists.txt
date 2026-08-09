cmake_minimum_required(VERSION 3.21)
project(platform_game C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(GAME_VENDOR_SDL "Build pinned SDL2/SDL2_ttf from source instead of using system packages" ON)

set(GAME_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Forráskód")

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${GAME_SRC}/*.c")

add_executable(game ${GAME_SOURCES})

target_include_directories(game PRIVATE "${GAME_SRC}")
target_compile_options(game PRIVATE -Wall)

if(EMSCRIPTEN)
    set(GAME_EM_FLAGS "SHELL:-sUSE_SDL=2" "SHELL:-sUSE_SDL_TTF=2")
    set(GAME_WEB_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
    set(GAME_WEB_PREJS "${CMAKE_CURRENT_SOURCE_DIR}/web/pre.js")

    target_compile_options(game PRIVATE ${GAME_EM_FLAGS})
    target_link_options(game PRIVATE
        ${GAME_EM_FLAGS}
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sEXIT_RUNTIME=0"
        "SHELL:-sFORCE_FILESYSTEM=1"
        "SHELL:-lidbfs.js"
        "SHELL:--preload-file ${GAME_SRC}/resources@/resources"
        "SHELL:--shell-file ${GAME_WEB_SHELL}"
        "SHELL:--pre-js ${GAME_WEB_PREJS}"
    )
    set_target_properties(game PROPERTIES
        OUTPUT_NAME index
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web"
        LINK_DEPENDS "${GAME_WEB_SHELL};${GAME_WEB_PREJS}"
    )
elseif(GAME_VENDOR_SDL)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    # As a subproject both libraries already default to no samples and no install
    # rules, and BUILD_SHARED_LIBS=OFF is what turns SDL2 static-only, so these
    # three are the only knobs that still differ from the defaults.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)  # build freetype instead of finding it

    FetchContent_Declare(SDL2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.4/SDL2-2.32.4.tar.gz
        URL_HASH SHA256=f15b478253e1ff6dac62257ded225ff4e7d0c5230204ac3450f1144ee806f934
    )
    FetchContent_Declare(SDL2_ttf
        URL https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.24.0/SDL2_ttf-2.24.0.tar.gz
        URL_HASH SHA256=0b2bf1e7b6568adbdbc9bb924643f79d9dedafe061fa1ed687d1d9ac4e453bfd
    )
    FetchContent_MakeAvailable(SDL2 SDL2_ttf)

    target_link_libraries(game PRIVATE SDL2::SDL2-static SDL2::SDL2main SDL2_ttf::SDL2_ttf-static)
else()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    target_link_libraries(game PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf)
endif()

if(NOT EMSCRIPTEN)
    target_link_libraries(game PRIVATE m)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${GAME_SRC}/resources" "$<TARGET_FILE_DIR:game>/resources"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:game>/saves"
    )
endif()
