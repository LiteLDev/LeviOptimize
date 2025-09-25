# LeviOptimize

![English](https://img.shields.io/badge/English-inactive?style=for-the-badge)
[![中文](https://img.shields.io/badge/简体中文-informational?style=for-the-badge)](README.zh.md)

A performance optimization mod for Minecraft Bedrock Dedicated Server (BDS), "LeviOptimize" is designed to enhance server performance by addressing issues related to low ticks per second (TPS). Operating within the LeviLamina framework, it modifies BDS code to achieve notable performance improvements.

## Table of Contents

- [Security](#security)
- [Installation](#installation)
  - [Using Lip](#using-lip)
  - [Manual Installation](#manual-installation)
- [Usage](#usage)
- [Commands](#commands)
- [Features](#features)
- [Contributing](#contributing)
- [License](#license)

## Security

LeviOptimize is an independent project not affiliated with Mojang or Microsoft, thus not officially supported by them. It is open-source and free to use. Users should note that LeviOptimize is not liable for any potential damage to servers or data. Please proceed with caution and use at your own risk.

## Installation

### Using Lip

```sh
lip install github.com/LiteLDev/LeviOptimize
```

### Manual Installation

1. Download the latest release from the [Releases page](https://github.com/LiteLDev/LeviOptimize/releases).
2. Unpack the `LeviOptimize-windows-x64.zip` file.
3. Copy the `LeviOptimize` folder to the `mods` folder of your BDS installation directory.

## Usage

Upon its first run, LeviOptimize will generate configuration files in `mod/LeviOptimize/config/`. These files are customizable to tailor the mod's behavior to your needs.

## Commands

- **`timing`**: Displays server performance statistics, focusing on TPS and detailed ECS performance metrics.

> **Note**: If your application crashes immediately upon using the `timing` command, with the exception pointing to a null pointer, please check the version of your msvcp140.dll. Consider updating or repairing the Visual C++ Redistributable, or simply copy a working msvcp140.dll file from a functioning device to your BDS root directory.

## Features

- **`optHopperItem`**: Enhances TPS by optimizing item transfers between hoppers and containers.
- **`optMovingBlock`**: Improves FPS and TPS by optimizing block entities' processing during piston movements.
- **`fixChunkLeak`**: Addresses the issue of chunks remaining loaded after players leave.
- **`optSeenPercent`**: Caches "SeenPercent" values to improve TPS, with a note of caution regarding potential hash collisions.
- **`optPushEntity`**: Improves server performance by reducing TPS drops from entity cramming with two key options:

## Contributing

We welcome questions, issues, and PRs. Feel free to contribute by:

- Creating an issue to ask questions or report bugs.
- Submitting PRs with improvements or new features.

## License

GPL-3.0-or-later © LiteLDev
