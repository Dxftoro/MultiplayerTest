# CLIENT SUBDIR

file (GLOB_RECURSE SOURCES "*.cpp")
file (GLOB_RECURSE HEADERS "*.h")

message(====================== ${SOURCES} ${HEADERS})
add_executable (client ${SOURCES} ${HEADERS})

set_target_properties(client PROPERTIES CXX_STANDARD 23)

target_link_libraries(client PRIVATE
	enet
	glm
)

target_include_directories(client PRIVATE
	${CMAKE_SOURCE_DIR}/src/shared
	${CMAKE_SOURCE_DIR}/thirdparty/enet/include
	${CMAKE_SOURCE_DIR}/thirdparty/GLM
	${CMAKE_SOURCE_DIR}/thirdparty/EnTT
)

find_package(OpenGL REQUIRED)
add_library(glfw STATIC IMPORTED)
set(GLFW_DIR "D:/Documents/libraries/glfw-3.4.bin.WIN64")

set_target_properties(
	glfw PROPERTIES
	IMPORTED_LOCATION "${GLFW_DIR}/lib-vc2022/glfw3.lib"
)

set_target_properties(
	glfw PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${GLFW_DIR}/include"
)

target_link_libraries(client PRIVATE ${OPENGL_LIBRARIES})
target_include_directories(client PRIVATE ${OPENGL_LIBRARIES})

target_link_libraries(client PRIVATE
	glad
	glfw
)