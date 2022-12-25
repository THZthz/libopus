cmake_minimum_required(VERSION 3.00.0)
project(opus)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c90 -pedantic")
add_library(opus
        # DATA
        data_structure/array.h
        data_structure/avl.h data_structure/avl.c
        data_structure/avl_hash.h data_structure/avl_hash.c
        data_structure/hashmap.h data_structure/hashmap.c
        data_structure/heap.h data_structure/heap.c
        data_structure/matrix.h data_structure/matrix.c
        data_structure/tree_printer.h data_structure/tree_printer.c
        data_structure/trie.h data_structure/trie.c

        # PHYSICS
        physics/physics.h
        physics/axes.c
        physics/body.c
        physics/algorithm/bvh.h physics/algorithm/bvh.c
        physics/algorithm/sat.c
        physics/algorithm/resolve.c
        physics/collision.c
        physics/common.c
        physics/composite.c
        physics/constraint.c
        physics/detector.c
        physics/engine.c
        physics/pair.c
        physics/sleeping.c
        physics/vertices.c

        # PATHFINDING
        pathfinding/finder/d_star_lite/map.h pathfinding/finder/d_star_lite/map.c
        pathfinding/finder/d_star_lite/d_star_lite.c
        pathfinding/finder/a_star.c
        pathfinding/finder/d_star.c
        pathfinding/finder/flow_field.c
        pathfinding/finder/floyd_warshall.c
        pathfinding/graph/adjacency_list.c
        pathfinding/graph/adjacency_multilist.c
        pathfinding/graph/forward_star.c
        pathfinding/graph/graph.c
        pathfinding/graph/grid.c
        pathfinding/graph/nav_mesh.c
        pathfinding/graph/orthogonal_list.c
        pathfinding/utils/utils.h
        pathfinding/graph.h
        pathfinding/pathfinder.h

        # BRAIN
        brain/ann.h brain/ann.c

        # MATH
        math/math.h math/math.c
        math/geometry.h math/geometry.c
        math/bresenham.h math/bresenham.c
        math/delaunay.h math/delaunay.c
        math/dubins_curve.h math/dubins_curve.c
        math/interpolate.h math/interpolate.c
        math/polynomial.h math/polynomial.c
        math/reeds_shepp_curve.h math/reeds_shepp_curve.c
        external/predicates.c

        # RENDER
        render/glad/glad.h render/glad/glad.c
        render/color.h
        render/render.h
        render/render_funcs.h render/render_funcs.c
        render/render_utils.h render/render_utils.c

        # ENGINE
        engine/plugins/input_manager.c
        engine/plugins/vector_graphics.c
        engine/engine.h engine/engine.c
        engine/gui.h engine/gui.c

        # GAME
        game/agents.h game/agents.c

        # UTILS
        utils/utils.h utils/utils.c
        utils/event.h utils/event.c
        utils/slre.h utils/slre.c
        )
set_target_properties(opus PROPERTIES LINKER_LANGUAGE C)
target_include_directories(opus PUBLIC ./)
#if (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_EMSCRIPTEN)
#    target_include_directories(opus PUBLIC sources/core/external/emscripten)
#endif ()
target_link_libraries(opus m)
#if (NOT CORE_BUILDING_PLATFORM STREQUAL PLATFORM_EMSCRIPTEN)
#    target_link_libraries(opus core_render)
#endif ()

add_subdirectory(render/pluto)
target_link_libraries(opus plutovg)

# link opengl and glfw library
if (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_WINDOWS)
    target_link_libraries(opus gdi32 opengl32 glfw3)
elseif (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_LINUX)
    target_link_libraries(core_render dl GL glfw)
elseif (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_EMSCRIPTEN)
    #    target_link_libraries(core_render glfw)
else ()
    message(ERROR "platform ${CMAKE_SYSTEM_NAME} is not supported")
endif ()