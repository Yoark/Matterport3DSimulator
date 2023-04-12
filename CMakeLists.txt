
# use vcpkg cmake configs
set(CMAKE_TOOLCHAIN_FILE "./vcpkg/scripts/buildsystems/vcpkg.cmake")
project(Matterport_Simulator CXX)
cmake_minimum_required(VERSION 2.8)

# to find bugs in find
set(CMAKE_FIND_DEBUG_MODE ON)
# need to find opencv ,glm and jsoncpp
set(CMAKE_PREFIX_PATH "./vcpkg/installed/x64-linux")
option(OSMESA_RENDERING "Offscreen CPU rendering with OSMesa" OFF)
option(EGL_RENDERING "Offscreen GPU rendering with EGL" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# Make custom find-modules available
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories("${PROJECT_SOURCE_DIR}/include")

# -------------------------------
# OpenCV: find in vcpkg if exists
# -------------------------------
find_package(OpenCV CONFIG REQUIRED)
if (OpenCV_FOUND)
  message("OpenCV found at ${OpenCV_DIR}")
  message("OpenCV_LIBS found at ${OpenCV_LIBS}")
  message("OpenCV_INCLUDE_DIRS found at ${OpenCV_INCLUDE_DIRS}")
  include_directories( ${OpenCV_INCLUDE_DIRS} )
else()
  message(SEND_ERROR "OpenCV not found.")
endif()
find_package(PkgConfig REQUIRED)
find_package(OpenMP)
if (OPENMP_CXX_FOUND)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()


if(EGL_RENDERING)
  add_definitions(-DEGL_RENDERING)
  find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
  # try to find libepoxy
  find_library(LIBEPOXY_LIBRARY NAMES epoxy libepoxy)
  if(NOT LIBEPOXY_LIBRARY)
      message(FATAL_ERROR "libepoxy not found")
  endif()
  
  find_path(LIBEPOXY_INCLUDE_DIR NAMES epoxy/gl.h)
  if(NOT LIBEPOXY_INCLUDE_DIR)
      message(FATAL_ERROR "libepoxy include directory not found")
  endif()
  # pkg_check_modules(EPOXY REQUIRED epoxy)
  set(GL_LIBS OpenGL::OpenGL OpenGL::EGL ${EPOXY_LIBRARIES})
# current build do
elseif(OSMESA_RENDERING)
  add_definitions(-DOSMESA_RENDERING)
  pkg_check_modules(OSMESA REQUIRED osmesa)
  set(GL_LIBS ${OSMESA_LIBRARIES})
else()
  cmake_policy(SET CMP0072 OLD)
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  set(GL_LIBS ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
endif()

add_library(MatterSim SHARED src/lib/MatterSim.cpp src/lib/NavGraph.cpp src/lib/Benchmark.cpp src/lib/cbf.cpp)
if(OSMESA_RENDERING)
  target_compile_definitions(MatterSim PUBLIC "-DOSMESA_RENDERING")
endif()

# find jsoncp
find_package(jsoncpp CONFIG REQUIRED)
# target_include_directories(MatterSim PRIVATE ${jsoncpp_INCLUDE_DIRS})
# target_link_libraries(MatterSim ${jsoncpp_LIBRARIES} ${OpenCV_LIBS} ${GL_LIBS})
target_link_libraries(MatterSim JsonCpp::JsonCpp ${OpenCV_LIBS} ${GL_LIBS})

add_executable(tests src/test/main.cpp)
# target_include_directories(tests PRIVATE ${jsoncpp_INCLUDE_DIRS})
# target_link_libraries(tests MatterSim ${jsoncpp_LIBRARIES} ${OpenCV_LIBS})
target_link_libraries(tests MatterSim JsonCpp::JsonCpp ${OpenCV_LIBS})

add_executable(mattersim_main src/driver/mattersim_main.cpp)
target_link_libraries(mattersim_main MatterSim)

# add_subdirectory(pybind11)

find_package(PythonInterp 3)
message(${PYTHON_EXECUTABLE})

# Need to search for python executable again to pick up an activated
# virtualenv python, if any.
unset(PYTHON_EXECUTABLE CACHE)
find_program(PYTHON_EXECUTABLE python
      PATHS ENV PATH         # look in the PATH environment variable
      NO_DEFAULT_PATH        # do not look anywhere else...
      )

find_package(NumPy REQUIRED)
# find this, installed with conda
find_package(pybind11 REQUIRED)
pybind11_add_module(MatterSimPython src/lib_python/MatterSimPython.cpp)
target_include_directories(MatterSimPython PRIVATE ${NUMPY_INCLUDES})
target_link_libraries(MatterSimPython PRIVATE MatterSim)
set_target_properties(MatterSimPython
  PROPERTIES
  OUTPUT_NAME MatterSim)
