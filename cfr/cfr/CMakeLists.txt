cmake_minimum_required(VERSION 2.8.12)
project(mct)
set (CMAKE_CXX_STANDARD 14)

find_package( PythonInterp 3.8 REQUIRED )
find_package( PythonLibs 3.8 REQUIRED )

find_package(pybind11 REQUIRED)
pybind11_add_module(mct interface.cpp mctree.cpp player.cpp env.cpp card.cpp Pluribus.cpp Node.cpp InfoNode.cpp State.cpp)
if(MSVC)
  target_compile_options(env PRIVATE /utf-8)
endif()

