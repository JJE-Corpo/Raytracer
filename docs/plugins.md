# Plugin System

## Overview

The project uses a runtime plugin loader for shared libraries (`.so`) found in `./plugins`.

At startup:

1. The build step creates plugin shared objects (renderer and user interface).
2. `Core::loadPlugins()` scans `./plugins`.
3. `PluginLoader::load(path)` calls `dlopen` and resolves:
   - `create_plugin`
   - `destroy_plugin`
4. Instances are stored in loader handles and queried by `PluginType`.

## Interfaces

- Base plugin interface: `IPlugin`
- Type system: `PluginType`
- Loader contract: `IPluginLoader`

The currently active plugin types are:

- `RENDERER`
- `USER_INTERFACE`

## C ABI Contract

A plugin must export both symbols:

```text
extern "C" IPlugin *create_plugin();
extern "C" void destroy_plugin(IPlugin *plugin);
```

Without both functions, the loader rejects the plugin.

## Renderer Plugins

Built in this project:

- `renderer_default.so`
- `renderer_clustered.so`
- `renderer_viewport.so`

`Core::loadRenderers()` inspects loaded renderer instances and selects pointers by renderer name.

## UI Plugin

- `user_interface.so`

If present and valid, `Core::loadUserInterface()` calls `create(*this)` on the plugin.

## Lifecycle

```mermaid
flowchart TD
    Start[Core startup] --> Scan[Scan ./plugins]
    Scan --> Open[dlopen each .so]
    Open --> Resolve[Resolve create_plugin/destroy_plugin]
    Resolve --> Create[create plugin symbol]
    Create --> Register[Store plugin handle]
    Register --> Use[Core queries by PluginType]
    Use --> Shutdown[Core shutdown]
    Shutdown --> Destroy[destroy plugin instance]
    Destroy --> Close[dlclose(handle)]
```

## Error Handling

- `dlopen` failure: plugin skipped.
- Missing exported symbols: plugin handle closed and skipped.
- Wrong runtime type after cast: plugin ignored for that subsystem.

## Notes

- Primitive and light classes are located in `srcs/plugins/...` but currently built into the main executable rather than loaded as runtime `.so` plugins.
- This architecture still keeps those implementations isolated and compatible with future dynamic-loading extensions.
