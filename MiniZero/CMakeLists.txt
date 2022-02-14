file(GLOB SRCS "*.cpp")
add_executable(MiniZero ${SRCS})

find_package(Boost COMPONENTS system thread)
set(LIBS ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${TORCH_LIBRARIES})

set_target_properties(MiniZero Games PROPERTIES COMPILE_DEFINITIONS ${GAME_TYPE}=true)
target_link_libraries(MiniZero Games ${LIBS})
