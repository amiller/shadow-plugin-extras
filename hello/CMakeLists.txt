## hello world plug-in

## create and install a shared library that can plug into shadow
add_bitcode(shadow-plugin-hello-bitcode src/hello-plugin.c src/hello.c)
add_plugin(shadow-plugin-hello shadow-plugin-hello-bitcode)
target_link_libraries(shadow-plugin-hello)
install(TARGETS shadow-plugin-hello DESTINATION plugins)

## create and install an executable that can run outside of shadow
add_executable(hello src/hello-main.c src/hello.c)
target_link_libraries(hello)
install(TARGETS hello DESTINATION bin)
