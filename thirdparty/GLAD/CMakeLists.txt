find_package(OpenGL REQUIRED)

add_library(glad STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/glad.c
)

target_link_libraries(glad ${OPENGL_LIBRARIES})

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)