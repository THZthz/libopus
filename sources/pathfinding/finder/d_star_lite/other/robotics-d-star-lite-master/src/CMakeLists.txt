project(fltk-d_star_lite)

add_executable(d_star main.cpp map.cpp math.cpp planner.cpp simulator.cpp widgets/widget_base.cpp widgets/widget_real.cpp widgets/widget_robot.cpp)
target_link_libraries(d_star  fltk comctl32 wsock32 ole32 uuid fltk_images)