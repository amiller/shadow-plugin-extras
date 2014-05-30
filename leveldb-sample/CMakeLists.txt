## leveldb sample plug-in

find_package(Boost COMPONENTS thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/" ${CMAKE_MODULE_PATH} )
find_package(LEVELDB REQUIRED)

include_directories(${LEVELDB_INCLUDE})

## C Version
## create and install a shared library that can plug into shadow
add_bitcode(shadow-plugin-leveldb-sample-bitcode leveldb-sample-plugin.c leveldb-sample.c)
add_plugin(shadow-plugin-leveldb-sample shadow-plugin-leveldb-sample-bitcode)
target_link_libraries(shadow-plugin-leveldb-sample ${LEVELDB_LIBS})
install(TARGETS shadow-plugin-leveldb-sample DESTINATION plugins)

## CPP Version
## create and install a shared library that can plug into shadow
add_bitcode(shadow-plugin-leveldb-sample-cpp-bitcode leveldb-sample-plugin.c leveldb-sample-cpp.cpp)
add_plugin(shadow-plugin-leveldb-sample-cpp shadow-plugin-leveldb-sample-cpp-bitcode)
target_link_libraries(shadow-plugin-leveldb-sample-cpp ${LEVELDB_LIBS})
install(TARGETS shadow-plugin-leveldb-sample-cpp DESTINATION plugins)

## Multithread Version
## create and install a shared library that can plug into shadow
add_bitcode(shadow-plugin-leveldb-sample-mt-bitcode leveldb-sample-plugin.c leveldb-sample-mt.cpp thread_toy.cpp)
add_plugin(shadow-plugin-leveldb-sample-mt shadow-plugin-leveldb-sample-mt-bitcode)
target_link_libraries(shadow-plugin-leveldb-sample-mt ${LEVELDB_LIBS})
install(TARGETS shadow-plugin-leveldb-sample-mt DESTINATION plugins)

add_executable(thread_toy thread_toy.cpp thread_toy_main.cpp)
target_link_libraries(thread_toy ${Boost_LIBRARIES})

add_executable(leveldb-sample-mt leveldb-sample-main.c leveldb-sample.c)
target_link_libraries(leveldb-sample-mt ${LEVELDB_LIBS})

## create and install an executable that can run outside of shadow
add_executable(leveldb-sample leveldb-sample-main.c leveldb-sample.c)
target_link_libraries(leveldb-sample ${LEVELDB_LIBS})
install(TARGETS leveldb-sample DESTINATION bin)
