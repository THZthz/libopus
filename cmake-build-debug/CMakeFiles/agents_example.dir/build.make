# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.20

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "C:\Environment\JetBrains\CLion 2021.2.2\bin\cmake\win\bin\cmake.exe"

# The command to remove a file.
RM = "C:\Environment\JetBrains\CLion 2021.2.2\bin\cmake\win\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\CLionProjects\libopus

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\CLionProjects\libopus\cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/agents_example.dir/depend.make
# Include the progress variables for this target.
include CMakeFiles/agents_example.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/agents_example.dir/flags.make

CMakeFiles/agents_example.dir/examples/agents_demo.c.obj: CMakeFiles/agents_example.dir/flags.make
CMakeFiles/agents_example.dir/examples/agents_demo.c.obj: CMakeFiles/agents_example.dir/includes_C.rsp
CMakeFiles/agents_example.dir/examples/agents_demo.c.obj: ../examples/agents_demo.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=C:\CLionProjects\libopus\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/agents_example.dir/examples/agents_demo.c.obj"
	C:\Environment\msys64\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles\agents_example.dir\examples\agents_demo.c.obj -c C:\CLionProjects\libopus\examples\agents_demo.c

CMakeFiles/agents_example.dir/examples/agents_demo.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/agents_example.dir/examples/agents_demo.c.i"
	C:\Environment\msys64\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E C:\CLionProjects\libopus\examples\agents_demo.c > CMakeFiles\agents_example.dir\examples\agents_demo.c.i

CMakeFiles/agents_example.dir/examples/agents_demo.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/agents_example.dir/examples/agents_demo.c.s"
	C:\Environment\msys64\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S C:\CLionProjects\libopus\examples\agents_demo.c -o CMakeFiles\agents_example.dir\examples\agents_demo.c.s

# Object files for target agents_example
agents_example_OBJECTS = \
"CMakeFiles/agents_example.dir/examples/agents_demo.c.obj"

# External object files for target agents_example
agents_example_EXTERNAL_OBJECTS =

agents_example.exe: CMakeFiles/agents_example.dir/examples/agents_demo.c.obj
agents_example.exe: CMakeFiles/agents_example.dir/build.make
agents_example.exe: sources/libopus.a
agents_example.exe: sources/render/pluto/libplutovg.a
agents_example.exe: CMakeFiles/agents_example.dir/linklibs.rsp
agents_example.exe: CMakeFiles/agents_example.dir/objects1.rsp
agents_example.exe: CMakeFiles/agents_example.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=C:\CLionProjects\libopus\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable agents_example.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\agents_example.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/agents_example.dir/build: agents_example.exe
.PHONY : CMakeFiles/agents_example.dir/build

CMakeFiles/agents_example.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\agents_example.dir\cmake_clean.cmake
.PHONY : CMakeFiles/agents_example.dir/clean

CMakeFiles/agents_example.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\CLionProjects\libopus C:\CLionProjects\libopus C:\CLionProjects\libopus\cmake-build-debug C:\CLionProjects\libopus\cmake-build-debug C:\CLionProjects\libopus\cmake-build-debug\CMakeFiles\agents_example.dir\DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/agents_example.dir/depend

