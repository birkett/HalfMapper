# Compiling with CMake

The CMake Build System allows you to generate project files for your native environment.

CMake can generate Visual Studio solutions, XCode projects and Makefiles, using one, cross-platform, build definition file (CMakeLists.txt).

## General Notes
Find scripts for SDL2 and GLEW are included in /CMakeModules. These allow CMake to search for headers and link libraries on the system.

On Windows, we use the environment variables, SDL2 and GLEW, to point to the downloaded development libraries.

CMake is designed to allow out-of-tree builds. Always create a new folder, outside of the tree to build in. Run CMake from your build folder, and point it to the source tree.

The commands below assume two directories. The source tree (halfmapper-src), and the build directory (halfmapper-build).

On Linux and Mac OSX, CMake will obey CC and CCX env vars. You can force CMake to use gcc or clang. For example:
```shell
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
```

Before starting, you will need to clone the repository, and prepare the submodules:
```shell
git clone https://github.com/gzalo/halfmapper halfmapper-src
cd halfmapper-src
git submodule init
git submodule update
```

## Windows Visual Studio
**Prerequisites**
* [SDL2 2.0.x Development Libraries For Visual Studio](http://libsdl.org/release/SDL2-devel-2.0.4-VC.zip)
* [GLEW Development Libraries For Windows](https://sourceforge.net/projects/glew/files/glew/1.13.0/glew-1.13.0-win32.zip/download)
* Windows SDK (Installed automatically with Visual Studio 2010 and newer)

**General build process**
* Create environment variables pointing to SDL2 and GLEW
* Create a new build directory outside the source tree
* Change to the build directory
* Generate a Visual Studio solution by invoking CMake
* Compile and debug inside Visual Studio

**Memory Debugging**
On debug builds, memory debugging is supported, using two methods.
The first uses the MSVCRT leak detection (_CrtDumpMemoryLeaks). This works, but lists false-positives in std::map.
The second method uses [Visual Leak Detector](https://vld.codeplex.com)

To enable memory debugging, tell CMake to define the following preprocessor symbols:
```
USE_VLD
USE_CRT_LD
```

```shell
set SDL2="C:\Users\Someone\Downloads\SDL2-2.0.4\"
set GLEW="C:\Users\Someone\Downloads\GLEW-1.13.0\"
mkdir ../halfmapper-build
cd ../half-mapper-build

#For a normal build.
cmake ../halfmapper-src

#For a build with memory debugging.
cmake -DUSE_CRT_LD=1 -DUSE_VLD=1 ../halfmapper-src

#VS Solution is now in "halfmapper-build"
```


## Linux Makefiles
(Assuming Debian or Ubuntu - package names may be different!)

**Prerequisites**
* libsdl2-dev (For SDL2)
* mesa-common-dev (For OpenGL)
* libglew-dev (For GLEW)

**General build process**
* Create a new build directory outside the source tree
* Change to the build directory
* Generate Makefiles
* Compile with make

```shell
mkdir ../halfmapper-build
cd ../halfmapper-build
cmake ../halfmapper-src
make

#Binary is now in the build folder
```


Additional compiler warnings can be enabled by telling CMake to use them:
```
cmake -DEXTRA_WARNINGS=1 ../halfmapper-src
```


## OSX, *BSD, Solaris
Completely untested. Should be similar to the Linux process.
XCode projects will be generated on OSX.

## Windows MinGW
Untested. Should work using Makefiles.
