cmake_minimum_required(VERSION 3.10)
project(MicroMayhem LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


add_executable(MicroMayhem
    src/main.c
    src/game_scene.c
    src/custom_fonts.c
    src/cJSON.c
    src/moveset_loader.c
    src/combat_system.c
)

target_include_directories(MicroMayhem PUBLIC ${PROJECT_SOURCE_DIR}/raylib/include)
target_link_directories(MicroMayhem PUBLIC ${PROJECT_SOURCE_DIR}/raylib/lib)
target_link_libraries(MicroMayhem PRIVATE raylib m)

if (WIN32)
    target_link_libraries(MicroMayhem PUBLIC opengl32 gdi32 winmm shell32 user32 kernel32)
endif()


set(ASSETS_SOURCE_PATH ${PROJECT_SOURCE_DIR}/assets)
set(ASSETS_DEST_PATH ${CMAKE_BINARY_DIR}/assets)
file(COPY ${ASSETS_SOURCE_PATH}/ DESTINATION ${ASSETS_DEST_PATH})