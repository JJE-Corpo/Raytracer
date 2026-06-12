# Toast System

## Sources

- `srcs/plugins/user_interface/toast/Toast.hpp`
- `srcs/plugins/user_interface/toast/ToastManager.hpp`

## Role

Transient non-blocking notifications displayed in the UI (bottom-right stack).

## Supported Types

- `INFO`
- `SUCCESS`
- `WARNING`
- `ERROR`

## Behavior

- `push(title, content, type)` creates a toast entry.
- `update()` applies fade-in/fade-out and removes expired items.
- `draw(window)` renders stacked animated toasts.
