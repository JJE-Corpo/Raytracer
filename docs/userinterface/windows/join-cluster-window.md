# JoinClusterWindow

## Source

- `srcs/plugins/user_interface/windows/JoinClusterWindow.hpp`

## Role

Prompt window for cluster connection parameters.

## Inputs

- IP text field
- Port text field (numeric validation)

## Callback

- `windowCallback(std::string ip, size_t port)`

Triggered on Enter validation or JOIN button click.
