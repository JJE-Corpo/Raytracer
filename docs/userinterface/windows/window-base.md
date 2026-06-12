# Window Base

## Source

- `srcs/plugins/user_interface/windows/Window.hpp`

## Role

Abstract base for threaded SFML sub-windows.

## Contract

Derived windows must implement:

- `handleEvent(...)`
- `drawUi()`
- `updateUi()`

## Lifecycle

- `loop()` creates window and runs poll/update/draw cycle.
- `destroy()` stops running flag and joins (or detaches) thread safely.
