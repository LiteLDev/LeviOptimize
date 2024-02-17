# LeviOptimize

A performance optimization plugin for Minecraft Bedrock Server

LeviOptimize is a performance optimization plugin for Minecraft Bedrock Dedicated Server (BDS), designed to enhance server performance by addressing low ticks per second (TPS) and frames per second (FPS) issues. This plugin operates within the LeviLamina framework, modifying BDS code to achieve significant performance improvements.

## Security

LeviOptimize is not affiliated with Mojang or Microsoft. Therefore, it is not officially supported by Mojang or Microsoft. This plugin is open-source and free to use. However, it is important to note that LeviOptimize is not responsible for any potential damage to your server or data. Please use this plugin at your own risk.

## Install

```sh
lip install github.com/LiteLDev/LeviOptimize
```

## Usage

After first run, LeviOptimize will generate configuration files in `plugin/LeviOptimize/config/`. You can modify these files to customize the plugin's behavior.

- **fixHopperItem**: Optimizes item transfers between hoppers and containers to enhance server TPS, addressing performance issues associated with item duplication.

- **fixMovingBlock**: Boosts FPS and server TPS by optimizing the processing of block entities with extensive NBT data during piston movements, minimizing the frequency and load of data transmissions from the server to the client.

- **fixChunkLeak**: Fix the issue where chunks remain loaded after a player carrying a map leaves.

- **optPushEntity**: Aims to mitigate the TPS decrease caused by entity cramming.

- **optBlockLookup**: Replaces the original block query mechanism with hashmap instead of map for queries, generally enhancing query speed but increasing memory usage.

## Contributing

Ask questions by creating an issue.

PRs accepted.

## License

GPL-3.0-or-later © LiteLDev
