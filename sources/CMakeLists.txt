cmake_minimum_required(VERSION 3.00.0)
project(opus)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c90 -pedantic")
add_library(opus
        # DATA
        data_structure/array.h data_structure/array.c
#        data_structure/deprecated/avl.h data_structure/deprecated/avl.c
#        data_structure/deprecated/avl_hash.h data_structure/deprecated/avl_hash.c
        data_structure/avl.h data_structure/avl.c
        data_structure/hashmap.h data_structure/hashmap.c
        data_structure/heap.h data_structure/heap.c
        data_structure/matrix.h data_structure/matrix.c
        data_structure/tree_printer.h data_structure/tree_printer.c
        data_structure/trie.h data_structure/trie.c

        # PHYSICS
        #        physics/matter/physics.h
        #        physics/matter/axes.c
        #        physics/matter/body.c
        #        physics/matter/algorithm/bvh.h physics/matter/algorithm/bvh.c
        #        physics/matter/algorithm/sat.c
        #        physics/matter/algorithm/resolver.c
        #        physics/matter/collision.c
        #        physics/matter/common.c
        #        physics/matter/composite.c
        #        physics/matter/constraint.c
        #        physics/matter/detector.c
        #        physics/matter/engine.c
        #        physics/matter/pair.c
        #        physics/matter/sleeping.c
        #        physics/matter/vertices.c

        physics/opus/physics.h physics/opus/physics_private.h
        physics/opus/collision/narrow_phase/SAT.c
        physics/opus/collision/narrow_phase/GJK.c
        physics/opus/collision/broad_phase/BVH.c
        physics/opus/collision/broad_phase/grid.c
        physics/opus/collision/broad_phase/SAP.c
        physics/opus/geometry/polygon.c
        physics/opus/geometry/circle.c
        physics/opus/geometry/shape.c
        physics/opus/collision/contact/v_clip.c
        physics/opus/collision/contact/contacts.c
        physics/opus/collision/contact/sleeping.c
        physics/opus/dynamics/body.c
        physics/opus/dynamics/joint.c
        physics/opus/dynamics/constraint.c
        physics/opus/factory.c
        physics/opus/world.c

        # PATHFINDING
#        pathfinding/finder/d_star_lite/map.h pathfinding/finder/d_star_lite/map.c
#        pathfinding/finder/d_star_lite/d_star_lite.c
#        pathfinding/finder/a_star.c
#        pathfinding/finder/d_star.c
#        pathfinding/finder/flow_field.c
#        pathfinding/finder/floyd_warshall.c
#        pathfinding/graph/adjacency_list.c
#        pathfinding/graph/adjacency_multilist.c
#        pathfinding/graph/forward_star.c
#        pathfinding/graph/graph.c
#        pathfinding/graph/grid.c
#        pathfinding/graph/nav_mesh.c
#        pathfinding/graph/orthogonal_list.c
#        pathfinding/utils/utils.h
#        pathfinding/graph.h
#        pathfinding/pathfinder.h

        # BRAIN
        brain/ann.h brain/ann.c
        brain/lstm.h brain/lstm.c

        # MATH
        math/math.h math/math.c
        math/autodiff.h math/autodiff.c
        math/geometry.h math/geometry.c
        math/bresenham.h math/bresenham.c
        math/polygon/delaunay.h math/polygon/delaunay.c
        math/polygon/polygon.h
        math/polygon/clip.c
        math/polygon/tessellate.c
        math/polygon/utils.c
        math/curve/dubins_curve.h math/curve/dubins_curve.c
        math/curve/spline.h math/curve/spline.c
        math/curve/reeds_shepp_curve.h math/curve/reeds_shepp_curve.c
        external/predicates.c

        # ?
        _/agents.h _/agents.c

        # UTILS
        utils/utils.h utils/utils.c
        utils/event.h utils/event.c
        utils/slre.h utils/slre.c

        # vg
        external/glad/glad.h external/glad/glad.c
        vg/vg_color.h vg/vg_color.c
        vg/vg.h vg/vg.c
        vg/vg_gl.h vg/vg_gl.c
        engine/engine.h engine/engine.c
        engine/input.h engine/input.c
        vg/vg_utils.h vg/vg_utils.c
        vg/vg_gui.h vg/vg_gui.c
        )
set_target_properties(opus PROPERTIES LINKER_LANGUAGE C)
target_include_directories(opus PUBLIC ./)
if (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_EMSCRIPTEN)
    target_include_directories(opus PUBLIC sources/external/emscripten)
endif ()
target_link_libraries(opus m)

add_subdirectory(vg/pluto)
target_link_libraries(opus plutovg)

# link opengl and glfw library
if (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_WINDOWS)
    target_link_libraries(opus gdi32 opengl32 glfw3)
elseif (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_LINUX)
    target_link_libraries(opus dl GL glfw)
elseif (CORE_BUILDING_PLATFORM STREQUAL PLATFORM_EMSCRIPTEN)
        target_link_libraries(opus glfw)
else ()
    message(ERROR "platform ${CMAKE_SYSTEM_NAME} is not supported")
endif ()