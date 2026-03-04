# SHARED SUBDIR

file (GLOB_RECURSE SOURCES "*.cpp")
file (GLOB_RECURSE HEADERS "*.h")

add_library(shared ${SOURCES} ${HEADERS})

set_target_properties(shared PROPERTIES CXX_STANDARD 23)

#target_link_libraries(shared PRIVATE
#	enet
#	glm
#)

target_include_directories(shared PUBLIC
	${CMAKE_SOURCE_DIR}/thirdparty/enet/include
	${CMAKE_SOURCE_DIR}/thirdparty/GLM
	${CMAKE_SOURCE_DIR}/thirdparty/EnTT
)