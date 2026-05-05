# target_link_libraries(LabyrinthGame PRIVATE sfml-audio)

cmake_minimum_required(VERSION 3.16)

project(Labyrinth-of-Distorted-Reality LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(LabyrinthGame
    main.cpp
    Menuscene.cpp
    LabyrinthGame.cpp
    LabyrinthGameRender.cpp
    LabyrinthGameWorld.cpp
)

# SFML 3.x CMake package. If CMake cannot find SFML automatically,
# configure with: -DSFML_DIR=C:/SFML-3.0.2/lib/cmake/SFML
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

target_link_libraries(LabyrinthGame PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)