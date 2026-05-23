cmake_minimum_required(VERSION 3.16)

project(PEA4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(aco_tsp
        src/main.cpp
        src/config.cpp
        src/reader.cpp
        src/utils.cpp
        src/writer.cpp
        src/pheromone_update.cpp
        src/ant_colony.cpp
)

target_include_directories(aco_tsp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MINGW)
    target_link_options(aco_tsp PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
    )
endif()