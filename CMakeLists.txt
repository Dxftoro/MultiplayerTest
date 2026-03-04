cmake_minimum_required (VERSION 3.8)
project (MultiplayerTest)

add_subdirectory ("thirdparty/enet")
add_subdirectory ("thirdparty/GLAD")
add_subdirectory ("thirdparty/GLM/glm")
add_subdirectory ("src/client")
add_subdirectory ("src/server")