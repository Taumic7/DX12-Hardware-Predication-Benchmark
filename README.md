# DirectX 12 Hardware Predication #

![](https://i.imgur.com/C0y6hgx.png)
__Example rendering of circa 11360 primitives.__

This is a project intended to benchmark the performance impact of using the DX12 implementation of hardware predication to cull primitives per draw call rather than excluding them from the command lists.

The full report on this analysis can be accessed [here.](https://drive.google.com/open?id=1TK4qI9mJaJVkGguMgqVOuKlMGCoHqWCD)

The findings of these benchmarks show large performance overheads when used for a large amount of rather small draw calls, however promising results when applied to larger instanced draw calls, which may have use cases in LOD-esque application areas.


## Instanced Draw Call performance
![](https://i.imgur.com/rfZwncT.png)

## Unique Draw Call performance
![](https://i.imgur.com/lEKzaKV.png)
