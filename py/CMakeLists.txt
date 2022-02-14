file(GLOB SRCS "*.cpp" "${CMAKE_SOURCE_DIR}/MiniZero/Configure.cpp" "${CMAKE_SOURCE_DIR}/MiniZero/ConfigureLoader.cpp")

find_package(pybind11 REQUIRED)
pybind11_add_module(miniZeroPy ${SRCS})

target_link_libraries(miniZeroPy PUBLIC Games)
set_target_properties(miniZeroPy PROPERTIES COMPILE_DEFINITIONS ${GAME_TYPE}=true)
