find_package(SDL2 REQUIRED)

add_definitions(-DBUILD_SDL)

# Only compile main.c, sdl-events.c, sdl-audio.c etc. are no longer needed
set(MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

add_executable(${BINARY_NAME}-sdl WIN32 ${MAIN_SRC})

target_compile_definitions(${BINARY_NAME}-sdl PRIVATE 
    BUILD_SDL 
    M_CORE_GBA
    MINIMAL_CORE=2
    ENABLE_VFS_FD
    ${OS_DEFINES}
)

target_link_libraries(${BINARY_NAME}-sdl 
    ${BINARY_NAME} 
    ${SDL2_LIBRARIES}
    m 
    pthread
)

set_target_properties(${BINARY_NAME}-sdl PROPERTIES 
    OUTPUT_NAME ${BINARY_NAME}-sdl
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
