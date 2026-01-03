# Super Mario 64 Brasil Discord Bot
* [Join here!](https://discord.gg/ukJch4D)

## Features
* Messages
* SM64BR Awards submissions tracking
* Streaming messages and roles
* The Run integration for pacepals pings

## Supported Systems
* Linux x64
* Raspbery Pi 5 (Linux cross-compile)
* macOS AArch64

## Dependencies
Linux (Arch):
```
sudo pacman -Sy clang cmake git lld ninja unzip zip
```

macOS:
```
xcode-select --install
brew install cmake lld ninja
```

## Building
```
cmake --preset PRESET
cmake --build --preset PRESET
```

Where PRESET can be one of:
```
linux-debug
linux-release
rpi5-debug
rpi5-release
mac-debug
mac-release
```