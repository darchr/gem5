# RocketChip Processor Choice

Technically, Rocket has 1 big core as the DefaultConfig, found [here](https://github.com/chipsalliance/rocket-chip/blob/836be7a92e5bd368a8a49f408a44b71c738a9a68/src/main/scala/system/Configs.scala#L23C1-L23C1).

But, to maintain closeness with experimental setup that was used to generate SimPoints, we will use RocketChip with 4 big cores.
