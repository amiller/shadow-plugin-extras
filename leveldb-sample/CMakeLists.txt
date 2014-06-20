## leveldb sample plug-in

find_package(Boost COMPONENTS thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/" ${CMAKE_MODULE_PATH} )
find_package(LEVELDB REQUIRED)

include_directories(${LEVELDB_INCLUDE})


#=============
# Pth building
#=============

set(PTH_ROOT_DIR ${CMAKE_SOURCE_DIR}/gnu-pth)
include_directories(${PTH_ROOT_DIR})

set(PTH_SOURCES
   ${PTH_ROOT_DIR}/pth_compat.c
   ${PTH_ROOT_DIR}/pth_debug.c
   ${PTH_ROOT_DIR}/pth_syscall.c
   ${PTH_ROOT_DIR}/pth_errno.c
   ${PTH_ROOT_DIR}/pth_ring.c
   ${PTH_ROOT_DIR}/pth_mctx.c
   ${PTH_ROOT_DIR}/pth_uctx.c
   ${PTH_ROOT_DIR}/pth_clean.c
   ${PTH_ROOT_DIR}/pth_time.c
   ${PTH_ROOT_DIR}/pth_tcb.c
   ${PTH_ROOT_DIR}/pth_util.c
   ${PTH_ROOT_DIR}/pth_pqueue.c
   ${PTH_ROOT_DIR}/pth_event.c
   ${PTH_ROOT_DIR}/pth_sched.c
   ${PTH_ROOT_DIR}/pth_data.c
   ${PTH_ROOT_DIR}/pth_msg.c
   ${PTH_ROOT_DIR}/pth_cancel.c
   ${PTH_ROOT_DIR}/pth_sync.c
   ${PTH_ROOT_DIR}/pth_attr.c
   ${PTH_ROOT_DIR}/pth_lib.c
   ${PTH_ROOT_DIR}/pth_fork.c
   ${PTH_ROOT_DIR}/pth_high.c
   ${PTH_ROOT_DIR}/pth_ext.c
   ${PTH_ROOT_DIR}/pth_string.c
   #${PTH_ROOT_DIR}/pthread.c
)

ADD_CUSTOM_COMMAND (
   OUTPUT ${PTH_ROOT_DIR}/pth_p.h
   COMMAND ${PTH_ROOT_DIR}/shtool 
   ARGS scpp -o ${PTH_ROOT_DIR}/pth_p.h -t ${PTH_ROOT_DIR}/pth_p.h.in -Dcpp -Cintern -M '==\#==' ${PTH_SOURCES} ${PTH_ROOT_DIR}/pthread.c
   DEPENDS ${PTH_ROOT_DIR}/pth_p.h.in ${PTH_SOURCES} ${PTH_ROOT_DIR}/pthread.c
)
ADD_CUSTOM_TARGET(generate_pth DEPENDS ${PTH_ROOT_DIR}/pth_p.h)

#set_property(SOURCE ${PTH_SOURCES} APPEND PROPERTY OBJECT_DEPENDS pth_p.h)



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
target_link_libraries(shadow-plugin-leveldb-sample-mt ${Boost_LIBRARIES})
install(TARGETS shadow-plugin-leveldb-sample-mt DESTINATION plugins)




add_bitcode(shadow-plugin-pth-bitcode ${PTH_SOURCES})

## Pth Version
## create and install a shared library that can plug into shadow
#add_bitcode(shadow-plugin-leveldb-sample-pth-bitcode leveldb-sample-plugin-pth.c leveldb-sample-pth.cpp thread_toy_#pth.cpp ${})
#add_plugin(shadow-plugin-leveldb-sample-pth 
#    shadow-plugin-leveldb-sample-pth-bitcode
#    shadow-plugin-pth-bitcode)
#target_link_libraries(shadow-plugin-leveldb-sample-pth)
#install(TARGETS shadow-plugin-leveldb-sample-pth DESTINATION plugins)


add_executable(thread_toy thread_toy.cpp thread_toy_main.cpp)
target_link_libraries(thread_toy ${Boost_LIBRARIES})
install(TARGETS thread_toy DESTINATION bin)

#add_executable(thread_toy_pth thread_toy_pth.cpp thread_toy_main.cpp ${PTH_SOURCES})
#target_link_libraries(thread_toy_pth)
#install(TARGETS thread_toy_pth DESTINATION bin)

add_executable(leveldb-sample-mt leveldb-sample-main.c leveldb-sample-mt.cpp thread_toy.cpp)
target_link_libraries(leveldb-sample-mt ${Boost_LIBRARIES})
install(TARGETS leveldb-sample-mt DESTINATION bin)

#add_executable(leveldb-sample-pth leveldb-sample-main-pth.c leveldb-sample-pth.cpp thread_toy_pth.cpp ${PTH_SOURCES})
#target_link_libraries(leveldb-sample-pth)
#install(TARGETS leveldb-sample-pth DESTINATION bin)

## create and install an executable that can run outside of shadow
add_executable(leveldb-sample leveldb-sample-main.c leveldb-sample.c)
target_link_libraries(leveldb-sample ${LEVELDB_LIBS})
install(TARGETS leveldb-sample DESTINATION bin)


## Yank version

add_bitcode(shadow-plugin-leveldb-sample-yank-bitcode leveldb-sample-plugin-yank.c leveldb-sample-yank.cpp thread_toy.cpp)
add_plugin(shadow-plugin-leveldb-sample-yank
    shadow-plugin-leveldb-sample-yank-bitcode
    shadow-plugin-pth-bitcode)
target_link_libraries(shadow-plugin-leveldb-sample-yank ${Boost_LIBRARIES})
install(TARGETS shadow-plugin-leveldb-sample-yank DESTINATION plugins)

## preload library for intercepting functions
add_library(shadow-preload-yank SHARED leveldb-sample-yank-preload.c)
target_link_libraries(shadow-preload-yank ${GLIB_LIBRARIES} ${DL_LIBRARIES})
install(TARGETS shadow-preload-yank DESTINATION lib)
