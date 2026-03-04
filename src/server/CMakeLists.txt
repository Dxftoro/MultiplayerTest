# SERVER SUBDIR

file (GLOB_RECURSE SOURCES "*.cpp")
file (GLOB_RECURSE HEADERS "*.h")

add_executable (server ${SOURCES} ${HEADERS})

set_target_properties(server PROPERTIES CXX_STANDARD 23)

target_link_libraries(server PRIVATE
	enet
	glm
)

target_include_directories(server PRIVATE
	${CMAKE_SOURCE_DIR}/src/shared
	${CMAKE_SOURCE_DIR}/thirdparty/enet/include
	${CMAKE_SOURCE_DIR}/thirdparty/GLM
	${CMAKE_SOURCE_DIR}/thirdparty/EnTT
)