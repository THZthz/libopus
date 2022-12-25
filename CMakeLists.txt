cmake_minimum_required(VERSION 3.20)
project(libopus C)

set(CMAKE_C_STANDARD 99)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
            FORCE)
endif (NOT CMAKE_BUILD_TYPE)

option(USE_EMSCRIPTEN "ON if you are using Emscripten to compile this" OFF)
option(EMSCRIPTEN_ON_MOBILE "ON if you use emscripten to compile this to run on mobile" OFF)
option(USE_SANITIZER "Use Sanitizer if ON" OFF)
option(DO_NOT_OPTIMIZE ON)

set(CORE_BUILDING_PLATFORM "unknown")
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CORE_BUILDING_PLATFORM "windows")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CORE_BUILDING_PLATFORM "linux")
elseif (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(CORE_BUILDING_PLATFORM "emscripten")
endif ()
set(PLATFORM_WINDOWS "windows")
set(PLATFORM_LINUX "linux")
set(PLATFORM_EMSCRIPTEN "emscripten")

if (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_EMSCRIPTEN)
    message("When compiled using Emscripten, USE_EMSCRIPTEN is automatically turned ON.")
    set(USE_EMSCRIPTEN ON)
endif ()
if (USE_EMSCRIPTEN)
    message("Add Definitions __EMSCRIPTEN__")
    add_definitions(-D__EMSCRIPTEN__)
endif ()
# turn EMSCRIPTEN_ON_MOBILE OFF if you are not using emscripten to compile
if (NOT USE_EMSCRIPTEN)
    if (EMSCRIPTEN_ON_MOBILE)
        message("You should not turn on EMSCRIPTEN_ON_MOBILE if USE_EMSCRIPTEN is not ON. Turn it OFF automatically now.")
        set(EMSCRIPTEN_ON_MOBILE OFF)
    endif ()
endif ()
if (EMSCRIPTEN_ON_MOBILE)
    message("Add Definitions __EMSCRIPTEN_ON_MOBILE__")
    add_definitions(-D__EMSCRIPTEN_ON_MOBILE__)
endif ()

if (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_LINUX AND USE_SANITIZER)
    message("On Linux, I normally Enable the SANITIZER and notify the compiler not to optimize code")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak,undefined -pg -Os")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak,undefined -pg -Os")
endif ()

if (DO_NOT_OPTIMIZE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
endif ()

#function(generate_executable file)
#    if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
#        include_directories(sources/core/emsdk/upstream/emscripten/system/include)
#        file(GLOB CORE_RENDER_SRC sources/core/render/*.c)
#        add_executable(executable ${file} ${CORE_RENDER_SRC} sources/core/external/glad/glad.c)
#        target_link_libraries(executable core plutovg)
#        set_target_properties(executable PROPERTIES SUFFIX ".html")
#        set_target_properties(executable PROPERTIES LINK_FLAGS "\
#                -o index.html\
#                -sALLOW_MEMORY_GROWTH=1 -sUSE_GLFW=3 -sUSE_WEBGL2 -sFULL_ES3\
#                --shell-file ../sources/core/external/emscripten_html_template.html\
#                --preload-file ../assets/fonts/consola.ttf@consola.ttf")
#    else ()
#        add_executable(executable ${file})
#        target_link_libraries(executable opus)
#    endif ()
#endfunction()


add_subdirectory(sources)

add_executable(m main.c)
add_executable(brain_example examples/brain_demo.c)
add_executable(physics_example examples/physics_engine_demo.c)
add_executable(delaunay_example examples/delaunay_demo.c)
add_executable(agents_example examples/agents_demo.c)

target_link_libraries(brain_example opus)
target_link_libraries(physics_example opus)
target_link_libraries(delaunay_example opus)
target_link_libraries(agents_example opus)
