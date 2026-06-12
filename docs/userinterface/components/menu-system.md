# Menu System

## Sources

- `srcs/plugins/user_interface/components/menu/MenuBar.hpp`
- `srcs/plugins/user_interface/components/menu/Menu.hpp`
- `srcs/plugins/user_interface/components/menu/MenuItem.hpp`

## Overview

The top-level menu stack is composed of:

- `MenuBar`: owns multiple `Menu` objects and active-menu state.
- `Menu`: header + dropdown panel + list of `MenuItem`.
- `MenuItem`: action or checkable entry with click/toggle callbacks.

## MenuItem Types

- `Action`: triggers `onClick()`.
- `Checkable`: toggles `checked` and triggers `onToggle(bool)`.

## Behavior

- Hovering menu headers switches active menu.
- Clicking an item executes callback and closes menu.
- Menu bar auto-closes when pointer leaves header/panel area.
