# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

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
CMAKE_COMMAND = /opt/cmake-3.12.2/bin/cmake

# The command to remove a file.
RM = /opt/cmake-3.12.2/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ljl/mpServer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ljl/mpServer

# Include any dependencies generated for this target.
include src/CMakeFiles/tomdog.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/tomdog.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/tomdog.dir/flags.make

src/CMakeFiles/tomdog.dir/Log.cpp.o: src/CMakeFiles/tomdog.dir/flags.make
src/CMakeFiles/tomdog.dir/Log.cpp.o: src/Log.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ljl/mpServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/tomdog.dir/Log.cpp.o"
	cd /home/ljl/mpServer/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/tomdog.dir/Log.cpp.o -c /home/ljl/mpServer/src/Log.cpp

src/CMakeFiles/tomdog.dir/Log.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tomdog.dir/Log.cpp.i"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ljl/mpServer/src/Log.cpp > CMakeFiles/tomdog.dir/Log.cpp.i

src/CMakeFiles/tomdog.dir/Log.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tomdog.dir/Log.cpp.s"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ljl/mpServer/src/Log.cpp -o CMakeFiles/tomdog.dir/Log.cpp.s

src/CMakeFiles/tomdog.dir/main.cpp.o: src/CMakeFiles/tomdog.dir/flags.make
src/CMakeFiles/tomdog.dir/main.cpp.o: src/main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ljl/mpServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/CMakeFiles/tomdog.dir/main.cpp.o"
	cd /home/ljl/mpServer/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/tomdog.dir/main.cpp.o -c /home/ljl/mpServer/src/main.cpp

src/CMakeFiles/tomdog.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tomdog.dir/main.cpp.i"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ljl/mpServer/src/main.cpp > CMakeFiles/tomdog.dir/main.cpp.i

src/CMakeFiles/tomdog.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tomdog.dir/main.cpp.s"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ljl/mpServer/src/main.cpp -o CMakeFiles/tomdog.dir/main.cpp.s

src/CMakeFiles/tomdog.dir/mpServer.cpp.o: src/CMakeFiles/tomdog.dir/flags.make
src/CMakeFiles/tomdog.dir/mpServer.cpp.o: src/mpServer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ljl/mpServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/CMakeFiles/tomdog.dir/mpServer.cpp.o"
	cd /home/ljl/mpServer/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/tomdog.dir/mpServer.cpp.o -c /home/ljl/mpServer/src/mpServer.cpp

src/CMakeFiles/tomdog.dir/mpServer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tomdog.dir/mpServer.cpp.i"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ljl/mpServer/src/mpServer.cpp > CMakeFiles/tomdog.dir/mpServer.cpp.i

src/CMakeFiles/tomdog.dir/mpServer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tomdog.dir/mpServer.cpp.s"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ljl/mpServer/src/mpServer.cpp -o CMakeFiles/tomdog.dir/mpServer.cpp.s

src/CMakeFiles/tomdog.dir/mySemaphore.cpp.o: src/CMakeFiles/tomdog.dir/flags.make
src/CMakeFiles/tomdog.dir/mySemaphore.cpp.o: src/mySemaphore.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ljl/mpServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/CMakeFiles/tomdog.dir/mySemaphore.cpp.o"
	cd /home/ljl/mpServer/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/tomdog.dir/mySemaphore.cpp.o -c /home/ljl/mpServer/src/mySemaphore.cpp

src/CMakeFiles/tomdog.dir/mySemaphore.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tomdog.dir/mySemaphore.cpp.i"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ljl/mpServer/src/mySemaphore.cpp > CMakeFiles/tomdog.dir/mySemaphore.cpp.i

src/CMakeFiles/tomdog.dir/mySemaphore.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tomdog.dir/mySemaphore.cpp.s"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ljl/mpServer/src/mySemaphore.cpp -o CMakeFiles/tomdog.dir/mySemaphore.cpp.s

src/CMakeFiles/tomdog.dir/sqlConnpool.cpp.o: src/CMakeFiles/tomdog.dir/flags.make
src/CMakeFiles/tomdog.dir/sqlConnpool.cpp.o: src/sqlConnpool.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ljl/mpServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/CMakeFiles/tomdog.dir/sqlConnpool.cpp.o"
	cd /home/ljl/mpServer/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/tomdog.dir/sqlConnpool.cpp.o -c /home/ljl/mpServer/src/sqlConnpool.cpp

src/CMakeFiles/tomdog.dir/sqlConnpool.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tomdog.dir/sqlConnpool.cpp.i"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ljl/mpServer/src/sqlConnpool.cpp > CMakeFiles/tomdog.dir/sqlConnpool.cpp.i

src/CMakeFiles/tomdog.dir/sqlConnpool.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tomdog.dir/sqlConnpool.cpp.s"
	cd /home/ljl/mpServer/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ljl/mpServer/src/sqlConnpool.cpp -o CMakeFiles/tomdog.dir/sqlConnpool.cpp.s

# Object files for target tomdog
tomdog_OBJECTS = \
"CMakeFiles/tomdog.dir/Log.cpp.o" \
"CMakeFiles/tomdog.dir/main.cpp.o" \
"CMakeFiles/tomdog.dir/mpServer.cpp.o" \
"CMakeFiles/tomdog.dir/mySemaphore.cpp.o" \
"CMakeFiles/tomdog.dir/sqlConnpool.cpp.o"

# External object files for target tomdog
tomdog_EXTERNAL_OBJECTS =

bin/tomdog: src/CMakeFiles/tomdog.dir/Log.cpp.o
bin/tomdog: src/CMakeFiles/tomdog.dir/main.cpp.o
bin/tomdog: src/CMakeFiles/tomdog.dir/mpServer.cpp.o
bin/tomdog: src/CMakeFiles/tomdog.dir/mySemaphore.cpp.o
bin/tomdog: src/CMakeFiles/tomdog.dir/sqlConnpool.cpp.o
bin/tomdog: src/CMakeFiles/tomdog.dir/build.make
bin/tomdog: src/CMakeFiles/tomdog.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ljl/mpServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX executable ../bin/tomdog"
	cd /home/ljl/mpServer/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tomdog.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/tomdog.dir/build: bin/tomdog

.PHONY : src/CMakeFiles/tomdog.dir/build

src/CMakeFiles/tomdog.dir/clean:
	cd /home/ljl/mpServer/src && $(CMAKE_COMMAND) -P CMakeFiles/tomdog.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/tomdog.dir/clean

src/CMakeFiles/tomdog.dir/depend:
	cd /home/ljl/mpServer && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ljl/mpServer /home/ljl/mpServer/src /home/ljl/mpServer /home/ljl/mpServer/src /home/ljl/mpServer/src/CMakeFiles/tomdog.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/tomdog.dir/depend
