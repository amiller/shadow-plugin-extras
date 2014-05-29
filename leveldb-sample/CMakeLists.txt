## leveldb sample plug-in

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

## create and install an executable that can run outside of shadow
add_executable(leveldb-sample leveldb-sample-main.c leveldb-sample.c)
target_link_libraries(leveldb-sample ${LEVELDB_LIBS})
install(TARGETS leveldb-sample DESTINATION bin)
