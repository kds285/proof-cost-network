file(GLOB SRCS "*.cpp")
add_library(Games ${SRCS})
target_include_directories(Games INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Games ${TORCH_LIBRARIES})