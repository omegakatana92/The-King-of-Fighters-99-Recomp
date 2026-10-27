cmake_minimum_required(VERSION 3.21)
project(kof99 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The neogeorecomp toolkit: the in-tree copy (a submodule in a clean
# checkout), or a checkout next to this project.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp/CMakeLists.txt)
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp)
else()
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/../neogeorecomp)
endif()
set(NEOGEORECOMP_DIR ${ng_default} CACHE PATH "neogeorecomp checkout")
add_subdirectory(${NEOGEORECOMP_DIR} neogeorecomp_build)

# Your unzipped MAME "kof99" set and "neogeo" system ROMs.  The build
# decrypts them into ${CMAKE_BINARY_DIR}/roms; nothing derived from them is
# committed.  Without it the player builds interpreter-only.
set(KOF99_ROM_DIR "" CACHE PATH "Directory with your kof99 and Neo Geo BIOS ROM files")
# Optional: entry points a previous run found at runtime (--dump-misses).
set(KOF99_ENTRIES "${CMAKE_BINARY_DIR}/entries.txt" CACHE FILEPATH "Extra entry points for the recompiler")

if(KOF99_ROM_DIR)
    # Fail at configure time on missing files or obviously wrong sizes.
    # tools/decrypt.c checks the CRC32 of every input before using it.
    set(_kof99_inputs
        "ka.neo-sma|262144"
        "251-p1.p1|4194304"
        "251-p2.p2|4194304"
        "251-c1.c1|8388608"
        "251-c2.c2|8388608"
        "251-c3.c3|8388608"
        "251-c4.c4|8388608"
        "251-c5.c5|8388608"
        "251-c6.c6|8388608"
        "251-c7.c7|8388608"
        "251-c8.c8|8388608"
        "251-m1.m1|131072"
        "251-v1.v1|4194304"
        "251-v2.v2|4194304"
        "251-v3.v3|4194304"
        "251-v4.v4|2097152"
        "sp-s2.sp1|131072"
        "sfix.sfix|131072"
        "sm1.sm1|131072"
        "000-lo.lo|131072")
    foreach(_entry IN LISTS _kof99_inputs)
        string(REPLACE "|" ";" _pair "${_entry}")
        list(GET _pair 0 _name)
        list(GET _pair 1 _want)
        set(_path "${KOF99_ROM_DIR}/${_name}")
        if(NOT EXISTS "${_path}")
            message(FATAL_ERROR
                "Required ROM file is missing: ${_path}\n"
                "Unzip the MAME sets 'kof99' and 'neogeo' into one directory and "
                "pass -DKOF99_ROM_DIR=/path/to/roms.")
        endif()
        file(SIZE "${_path}" _have)
        if(NOT _have EQUAL _want)
            message(FATAL_ERROR
                "Wrong size for ${_path}: ${_have} bytes, expected ${_want}.\n"
                "The file is not the expected KOF '99 / Neo Geo ROM.")
        endif()
    endforeach()
endif()

# Stage 0: ROM decryption (NEO-SMA program + NEO-CMC42 graphics + S ROM).
add_executable(kof99_decrypt tools/decrypt.c)

# Stage 1: the generator.
add_executable(kof99_recomp src/recomp_main.c src/game.c)
target_link_libraries(kof99_recomp PRIVATE m68krecomp_core)
target_include_directories(kof99_recomp PRIVATE src)

# Stage 2: the player, with the generated C when ROMs are available.
set(GAME_SOURCES src/main.c src/rom_select.c)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GEN_FILES 32)
if(KOF99_ROM_DIR)
    set(DEC_DIR ${CMAKE_BINARY_DIR}/roms)
    set(DEC_STAMP ${DEC_DIR}/.decrypted)
    add_custom_command(
        OUTPUT ${DEC_STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEC_DIR}
        COMMAND kof99_decrypt --rom-path ${KOF99_ROM_DIR} --out ${DEC_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${DEC_STAMP}
        DEPENDS kof99_decrypt
        COMMENT "Decrypting KOF '99 ROMs (NEO-SMA + NEO-CMC42)")

    set(GEN_SOURCES ${GEN_DIR}/recomp_table.c)
    math(EXPR last "${GEN_FILES} - 1")
    foreach(i RANGE ${last})
        if(i LESS 10)
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_0${i}.c)
        else()
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_${i}.c)
        endif()
    endforeach()
    # The entry list starts empty; the `profile` target fills it from
    # scripted sessions and the next build regenerates with it.
    if(NOT EXISTS ${KOF99_ENTRIES})
        file(WRITE ${KOF99_ENTRIES} "")
    endif()
    file(MAKE_DIRECTORY ${GEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SOURCES} ${GEN_DIR}/recomp_common.h
        COMMAND kof99_recomp --rom-path ${DEC_DIR} --out ${GEN_DIR} --files ${GEN_FILES}
                --entries ${KOF99_ENTRIES}
        DEPENDS kof99_recomp ${KOF99_ENTRIES} ${DEC_STAMP}
        COMMENT "Recompiling KOF '99 from ${DEC_DIR}")
    list(APPEND GAME_SOURCES ${GEN_SOURCES})
endif()

add_executable(kof99 ${GAME_SOURCES} src/game.c)
if(WIN32)
    set_target_properties(kof99 PROPERTIES OUTPUT_NAME "KOF 99")
else()
    set_target_properties(kof99 PROPERTIES OUTPUT_NAME "kof99")
endif()
target_link_libraries(kof99 PRIVATE neogeorecomp)
target_include_directories(kof99 PRIVATE src ${GEN_DIR})
if(WIN32)
    # Native folder picker in rom_select.c (IFileOpenDialog).
    target_link_libraries(kof99 PRIVATE ole32 shell32)
endif()
if(KOF99_ROM_DIR)
    # Play scripted sessions and record the entry points that only show up at
    # runtime (computed jumps into unrolled code), then rebuild.
    add_custom_target(profile
        COMMAND $<TARGET_FILE:kof99> --headless --rom-path ${DEC_DIR}
                --frames 6000 --dump-misses ${KOF99_ENTRIES}
        COMMAND $<TARGET_FILE:kof99> --headless --rom-path ${DEC_DIR}
                --frames 12000 --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/attract.txt
                --dump-misses ${KOF99_ENTRIES}
        COMMAND $<TARGET_FILE:kof99> --headless --rom-path ${DEC_DIR}
                --frames 24000 --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/play_long.txt
                --dump-misses ${KOF99_ENTRIES}
        DEPENDS kof99
        COMMENT "Profiling scripted sessions for runtime-only entry points")
    target_compile_definitions(kof99 PRIVATE KOF99_HAVE_RECOMP)
    if(MSVC)
        # Generated routines are large; /bigobj lifts the section limit.
        # /O1 avoids MSVC's superlinear optimizer blow-up on these giant
        # dispatch functions and still runs the 68000 code at full speed.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "/bigobj;/O1;/MP4")
    else()
        # GCC/Clang at -O3 spend a very long time on these giant switch
        # functions. -O1 keeps the build practical; the generated code is
        # already one C block per instruction.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "-O1")
    endif()

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_custom_target(audio-check
            COMMAND $<TARGET_FILE:kof99> --headless --rom-path ${DEC_DIR}
                    --frames 24000 --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/play_long.txt
                    --wav ${CMAKE_BINARY_DIR}/audio-check.wav
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_audio.py
                    ${CMAKE_BINARY_DIR}/audio-check.wav --from 10 --max-silence 4
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_bgm.py
                    ${CMAKE_BINARY_DIR}/audio-check.wav --from 10 --max-silence 2
                    --min-median-rms 1500
            DEPENDS kof99
            COMMENT "Audio regression: fail on silent gaps or missing BGM")
    endif()
endif()
if(MSVC)
    target_compile_definitions(kof99 PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(kof99_recomp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(kof99_decrypt PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Ship the SDL2 DLL (when the window is built) next to the executable.
if(WIN32 AND TARGET SDL2::SDL2)
    add_custom_command(TARGET kof99 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:kof99> $<TARGET_FILE_DIR:kof99>
        COMMAND_EXPAND_LISTS)
endif()

# Smoke tests: prove that the linked executable actually dispatches
# recompiled blocks, not just that a table was generated.
include(CTest)
if(BUILD_TESTING AND KOF99_ROM_DIR)
    add_test(NAME native-entry-points
             COMMAND kof99 --headless --rom-path ${DEC_DIR} --frames 1)
    set_tests_properties(native-entry-points PROPERTIES
        PASS_REGULAR_EXPRESSION "[1-9][0-9]* recompiled entry points")
    add_test(NAME native-blocks
             COMMAND kof99 --headless --rom-path ${DEC_DIR} --frames 300)
    set_tests_properties(native-blocks PROPERTIES
        PASS_REGULAR_EXPRESSION "native blocks [1-9][0-9]*")
endif()
