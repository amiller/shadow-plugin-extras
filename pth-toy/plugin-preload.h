/*
 * See LICENSE for licensing information
 */

#ifndef PLUGIN_PRELOAD_H_
#define PLUGIN_PRELOAD_H_

typedef enum _ExecutionContext ExecutionContext;
enum _ExecutionContext {
  EXECTX_NONE, EXECTX_PLUGIN, EXECTX_PTH, EXECTX_SHADOW
};

typedef enum _PluginName PluginName;
enum _PluginName {
  PLUGIN_PTH_TOY
};

void pluginpreload_setPluginContext(PluginName plg);
void pluginpreload_setShadowContext();
void pluginpreload_setPthContext();
void pluginpreload_init(GModule *module, int nLocks);

#endif /* PLUGIN_PRELOAD_H_ */
