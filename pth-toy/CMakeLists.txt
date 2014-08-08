## pth-toy sample plug-in

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/" ${CMAKE_MODULE_PATH} )

find_package(RT REQUIRED)
find_package(DL REQUIRED)
find_package(GLIB REQUIRED)

# threads - set options BEFORE find_package()
set(CMAKE_THREAD_PREFER_PTHREAD ON)  
find_package(Threads REQUIRED)
if(NOT Threads_FOUND)
    message(FATAL_ERROR "Could not locate Threads.")
endif()
message(STATUS "CMAKE_USE_PTHREADS_INIT = ${CMAKE_USE_PTHREADS_INIT}")
message(STATUS "CMAKE_THREAD_LIBS_INIT = ${CMAKE_THREAD_LIBS_INIT}")

# openssl
find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "Could not locate OpenSSL.")
endif()
message(STATUS "OPENSSL_VERSION = ${OPENSSL_VERSION}")
message(STATUS "OPENSSL_INCLUDE_DIR = ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_LIBRARIES = ${OPENSSL_LIBRARIES}")

#=============
# Pth building
#=============

set(PTH_ROOT_DIR ${CMAKE_BINARY_DIR}/gnu-pth)
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
add_bitcode(shadow-plugin-pth-bitcode ${PTH_SOURCES})
add_dependencies(shadow-plugin-pth-bitcode generate_pth)



#### Targets


## preload library for intercepting functions
add_library(shadow-preload-pth-toy SHARED plugin-preload.c)
target_link_libraries(shadow-preload-pth-toy ${GLIB_LIBRARIES} ${OPENSSL_LIBRARIES} ${RT_LIBRARIES} ${DL_LIBRARIES})
install(TARGETS shadow-preload-pth-toy DESTINATION lib)

## C Version
## create and install a shared library that can plug into shadow
add_bitcode(shadow-plugin-pth-toy-bitcode pth-toy-plugin.c pth-toy.c toy-pthread.c)
add_plugin(shadow-plugin-pth-toy shadow-plugin-pth-toy-bitcode shadow-plugin-pth-bitcode)
target_link_libraries(shadow-plugin-pth-toy ${OPENSSL_LIBRARIES})
install(TARGETS shadow-plugin-pth-toy DESTINATION plugins)

## Standalone executable 
## uses pthread
add_executable(pth-toy pth-toy.c pth-toy-main.c toy-pthread.c)
target_link_libraries(pth-toy ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS pth-toy DESTINATION bin)

