#include "pth-toy.h"

#include <shd-library.h>

static void _mylog(ShadowLogLevel level, const char* functionName, const char* format, ...) {
	va_list variableArguments;
	va_start(variableArguments, format);
	vprintf(format, variableArguments);
	va_end(variableArguments);
	printf("%s", "\n");
}

void pluginpreload_setPluginContext(PluginName plg) {
}
void pluginpreload_setShadowContext() {
}
void pluginpreload_setPthContext() {
}

extern int plugin_main(int argc, char *argv[], ShadowLogFunc slogf);

int main(int argc, char *argv[]) { 
  plugin_main(argc, argv, _mylog);
}
