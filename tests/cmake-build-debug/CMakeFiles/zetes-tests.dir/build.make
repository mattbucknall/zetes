# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/matt/usr/clion-2019.3.5/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /home/matt/usr/clion-2019.3.5/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/matt/Projects/zetes/tests

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/matt/Projects/zetes/tests/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/zetes-tests.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/zetes-tests.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/zetes-tests.dir/flags.make

CMakeFiles/zetes-tests.dir/zetes-tests.c.o: CMakeFiles/zetes-tests.dir/flags.make
CMakeFiles/zetes-tests.dir/zetes-tests.c.o: ../zetes-tests.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matt/Projects/zetes/tests/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/zetes-tests.dir/zetes-tests.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/zetes-tests.dir/zetes-tests.c.o   -c /home/matt/Projects/zetes/tests/zetes-tests.c

CMakeFiles/zetes-tests.dir/zetes-tests.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zetes-tests.dir/zetes-tests.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matt/Projects/zetes/tests/zetes-tests.c > CMakeFiles/zetes-tests.dir/zetes-tests.c.i

CMakeFiles/zetes-tests.dir/zetes-tests.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zetes-tests.dir/zetes-tests.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matt/Projects/zetes/tests/zetes-tests.c -o CMakeFiles/zetes-tests.dir/zetes-tests.c.s

CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.o: CMakeFiles/zetes-tests.dir/flags.make
CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.o: /home/matt/Projects/zetes/zetes.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matt/Projects/zetes/tests/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.o   -c /home/matt/Projects/zetes/zetes.c

CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matt/Projects/zetes/zetes.c > CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.i

CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matt/Projects/zetes/zetes.c -o CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.s

# Object files for target zetes-tests
zetes__tests_OBJECTS = \
"CMakeFiles/zetes-tests.dir/zetes-tests.c.o" \
"CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.o"

# External object files for target zetes-tests
zetes__tests_EXTERNAL_OBJECTS =

zetes-tests: CMakeFiles/zetes-tests.dir/zetes-tests.c.o
zetes-tests: CMakeFiles/zetes-tests.dir/home/matt/Projects/zetes/zetes.c.o
zetes-tests: CMakeFiles/zetes-tests.dir/build.make
zetes-tests: CMakeFiles/zetes-tests.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/matt/Projects/zetes/tests/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable zetes-tests"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/zetes-tests.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/zetes-tests.dir/build: zetes-tests

.PHONY : CMakeFiles/zetes-tests.dir/build

CMakeFiles/zetes-tests.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/zetes-tests.dir/cmake_clean.cmake
.PHONY : CMakeFiles/zetes-tests.dir/clean

CMakeFiles/zetes-tests.dir/depend:
	cd /home/matt/Projects/zetes/tests/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/matt/Projects/zetes/tests /home/matt/Projects/zetes/tests /home/matt/Projects/zetes/tests/cmake-build-debug /home/matt/Projects/zetes/tests/cmake-build-debug /home/matt/Projects/zetes/tests/cmake-build-debug/CMakeFiles/zetes-tests.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/zetes-tests.dir/depend
