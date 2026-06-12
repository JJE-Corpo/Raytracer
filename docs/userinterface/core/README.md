# UI Core

## Scope

This section documents the base UI infrastructure in `srcs/plugins/user_interface/`:

- `UserInterface.hpp/.cpp`
- `Component.hpp`
- `Theme.hpp`
- `CursorType.hpp`
- `VerticalLayout.hpp`
- `ViewportHelper.hpp`

## Main Concepts

- `UserInterface` is the plugin entry point (`PluginType::USER_INTERFACE`).
- `Component` is the base abstraction for all widgets.
- `Theme` centralizes color constants.
- `CursorType` standardizes cursor intents.
- `VerticalLayout` is a tiny helper used by many panel/window layouts.
- `ViewportHelper` contains viewport projection/picking and material-slider key filtering helpers.

## `Component` Contract

All components share this lifecycle:

1. `setFont(...)`
2. `layout(...)` (for widgets that own geometry)
3. `update(mouse)`
4. `handleEvent(event, mouse)`
5. `draw(window)`

State fields available in the base struct:

- `enabled`
- `hovered`
- `visible`

## `UserInterface` Responsibilities

- Creates and owns the SFML window loop.
- Builds menus, side panels, and render viewport widgets.
- Forwards user actions to `ICoreAccess`.
- Synchronizes viewport selection with panel selection.
- Displays toasts and auxiliary windows.

## `ViewportHelper` Highlights

- `isMaterialFloatSlider(name, model)` selects which material keys become sliders.
- `projectToPixel(...)` projects world points to render pixels.
- `pickViewportLight(...)` picks nearby lights in the viewport using projected gizmo centers.
