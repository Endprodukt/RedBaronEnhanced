cmake_minimum_required(VERSION 3.21)

project(redbaron_enhanced VERSION 0.1.0 LANGUAGES C CXX)

# Builds: see BUILD.md.

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(RB_BUILD_GAME "Build the game itself (needs the VectorPie SDK and SDL2)" ON)

# --- third party ------------------------------------------------------------

# miniz (MIT): reads the MAME ROM zip.
add_library(miniz STATIC third_party/miniz/miniz.c)
target_include_directories(miniz PUBLIC third_party/miniz)
target_compile_options(miniz PRIVATE -w)

# --- the game core: no SDK, no SDL, testable on its own ----------------------

add_library(rbcore STATIC
    src/core/machine.c
    src/core/emu.c
    src/cpu/m6502.c
    src/mbox/mathbox.c
    src/avg/avg.c
    src/rom/romset.c
    src/capture/capture.c
    src/sound/pokey.c
    src/sound/sound.c
    src/sound/samples.c
    # the program, as tools/mac2c.py writes it from upstream/*.MAC
    src/game/main.c
    src/game/states.c
    src/game/objects.c
    src/game/ground.c
    src/game/display.c
    src/game/alpha.c
    src/game/collide.c
    src/game/math.c
    src/game/coin.c
    src/game/sound.c
    src/game/grnd.c
    src/game/vgut.c
    src/game/earom.c
    src/game/int.c
    # by hand
    src/game/frame.c
    # additions
    src/render/subframe.c
    src/render/solid.c
    src/render/color.c
    src/render/deco.c
    src/render/glyph.c
    src/render/screens.c
    src/render/cockpit.c
    src/render/demo.c
    src/render/fx.c
)
target_include_directories(rbcore PUBLIC src)
target_link_libraries(rbcore PUBLIC miniz)
target_compile_options(rbcore PRIVATE -Wall -Wextra)
if(UNIX)
    target_link_libraries(rbcore PUBLIC m)
endif()

# --- tools ---------------------------------------------------------------------

# Checks the game frame by frame against a MAME recording.
add_executable(rbcompare tools/rbcompare/main.c)
target_link_libraries(rbcompare PRIVATE rbcore)
target_compile_options(rbcompare PRIVATE -Wall -Wextra)

# Checks the interrupt routine in C against the original's on the 6502 core.
add_executable(rbnmi tools/rbnmi/main.c)
target_link_libraries(rbnmi PRIVATE rbcore)
target_compile_options(rbnmi PRIVATE -Wall -Wextra)

# Runs the game without a screen: its picture as SVG, its sound as WAV.
add_executable(rbshot tools/rbshot/main.c)
target_link_libraries(rbshot PRIVATE rbcore)
target_compile_options(rbshot PRIVATE -Wall -Wextra)
# (--samples: the recordings, read with the SDK's Ogg Vorbis decoder when the SDK is there)
set(RB_STB_VORBIS "${CMAKE_CURRENT_SOURCE_DIR}/external/VectorPie/sdk/src/third_party/stb/stb_vorbis.c")
if(EXISTS "${RB_STB_VORBIS}")
    add_library(rbvorbis STATIC "${RB_STB_VORBIS}")
    target_compile_options(rbvorbis PRIVATE -w)
    target_link_libraries(rbshot PRIVATE rbvorbis)
    target_compile_definitions(rbshot PRIVATE RB_HAVE_VORBIS)
    target_include_directories(rbshot PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/VectorPie/sdk/src/third_party/stb")
endif()

# --- the game: on the VectorPie SDK (a window on a PC, the vector board on the cabinet) ---

if(RB_BUILD_GAME)
    set(RB_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/VectorPie/sdk"
        CACHE PATH "VectorPie SDK source directory")
    if(NOT EXISTS "${RB_SDK_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "VectorPie SDK not found in ${RB_SDK_DIR}: see BUILD.md "
                            "(or configure with -DRB_BUILD_GAME=OFF for the tools only)")
    endif()
    if(RB_PI_BUILD)
        # cmake/toolchain-pi-clang.cmake (the Pi build from Windows, without
        # WSL): the cabinet's own SDL2, by its file.
        add_library(SDL2::SDL2 SHARED IMPORTED GLOBAL)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_LOCATION "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0"
            IMPORTED_SONAME "libSDL2-2.0.so.0"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SYSROOT}/usr/include/SDL2")
        set(THREADS_PREFER_PTHREAD_FLAG ON)
    endif()
    set(VP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(VP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${RB_SDK_DIR}" vectorpie EXCLUDE_FROM_ALL)

    add_library(rbout STATIC
        src/render/config.c
        src/render/draw.c
        src/platform/app.c
        src/platform/audio.c
        src/platform/controls.c
        src/platform/menu.c
    )
    target_link_libraries(rbout PUBLIC rbcore vectorpie)
    # (the SDK's Ogg Vorbis decoder, stb_vorbis: its declarations, for the recordings)
    target_include_directories(rbout PRIVATE "${RB_SDK_DIR}/src/third_party/stb")
    target_compile_options(rbout PRIVATE -Wall -Wextra)

    add_executable(redbaron src/main.c)
    target_link_libraries(redbaron PRIVATE rbout)
    vp_program(redbaron)

    # package files next to the program (vp_asset_path looks there)
    configure_file(package/redbaron.ini ${CMAKE_CURRENT_BINARY_DIR}/redbaron.ini COPYONLY)
    file(GLOB RB_SOUNDS CONFIGURE_DEPENDS package/sounds/*.ogg)
    foreach(f ${RB_SOUNDS})
        get_filename_component(n ${f} NAME)
        configure_file(${f} ${CMAKE_CURRENT_BINARY_DIR}/sounds/${n} COPYONLY)
    endforeach()
endif()
