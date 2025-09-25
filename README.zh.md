# LeviOptimize

[![English](https://img.shields.io/badge/English-informational?style=for-the-badge)](README.md)
![中文](https://img.shields.io/badge/简体中文-inactive?style=for-the-badge)

Minecraft基岩版服务器（BDS）的性能优化插件，旨在通过解决每秒传输帧数（TPS）低下的问题来提升服务器性能。该插件在LeviLamina框架内运作，通过修改BDS代码以实现显著的性能改进。

## 目录

- [安全性](#安全性)
- [安装](#安装)
  - [使用Lip](#使用lip)
  - [手动安装](#手动安装)
- [使用](#使用)
- [命令](#命令)
- [特性](#特性)
- [贡献](#贡献)
- [许可证](#许可证)

## 安全性

LeviOptimize是一个独立项目，不隶属于Mojang或Microsoft，因此没有得到官方支持。它是开源且免费使用的。用户应注意，LeviOptimize对服务器或数据可能造成的任何潜在损害不承担责任。请谨慎使用，并自行承担风险。

## 安装

### 使用Lip

```sh
lip install github.com/LiteLDev/LeviOptimize
```

### 手动安装

1. 从[发布页面](https://github.com/LiteLDev/LeviOptimize/releases)下载最新版本。
2. 解压`LeviOptimize-windows-x64.zip`文件。
3. 将`LeviOptimize`文件夹复制到BDS安装目录下的`mods`文件夹中。

## 使用

首次运行后，LeviOptimize将在`mod/LeviOptimize/config/`中生成配置文件。您可以修改这些文件以定制插件的行为。

## 命令

- **`timing`**：显示服务器性能统计信息，重点关注TPS和详细的ECS性能指标。

> **注意**：如果您在使用`timing`命令后应用程序立即崩溃，并且异常原因是空指针，请检查您的msvcp140.dll版本。考虑更新或修复Visual
> C++
> Redistributable，或者简单地从一个正常工作的设备复制一个msvcp140.dll文件到您的BDS根目录。

## 功能

- **`optHopperItem`**：通过优化漏斗和容器之间的物品传输来提高TPS。
- **`optMovingBlock`**：通过优化活塞运动过程中的方块实体处理来提高FPS和TPS。
- **`fixChunkLeak`**：修复玩家离开后区块未卸载的问题。
- **`optSeenPercent`**：缓存特定坐标及其相应边界框内的“SeenPercent”值，以提高TPS。由于存在哈希冲突可能导致性能下降的可能性，实际效果不确定。
- **`optPushEntity`**:
  通过减少实体拥挤导致的TPS下降，提升服务器性能，主要包括两个配置选项：

## 贡献

欢迎通过创建问题或提交PR来提问和贡献。

## 许可证

GPL-3.0-or-later © LiteLDev
