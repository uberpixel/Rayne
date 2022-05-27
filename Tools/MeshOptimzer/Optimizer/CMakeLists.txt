project(Optimizer)

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Vendor/meshoptimizer)

set(OPTIMIZER_SOURCE
		main.cpp
	)

add_executable(optimizer ${OPTIMIZER_SOURCE})
target_link_libraries(optimizer meshoptimizer)
target_include_directories(optimizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/meshoptimizer/src)

