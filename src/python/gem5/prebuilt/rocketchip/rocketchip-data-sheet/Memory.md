# RocketChip Memory

According to the [FireSim paper](https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8416816), a RocketChip uses the memory system of the FPGA.

So, according to the information in that paper, the memory for the AWS EC2 F1 instance is a **DDR3** with a size of **16GiB**. Since we are also going to use the same (or similar) instance, we should go with the value given in the paper for consistency.

So the gem5 way to do it would be:

``` python
def RocketMemory():
    """
    Memory for the RocketChip.
    DDR3 Subsystem with 16GiB of memory.

    return: ChanneledMemory
    """
    memory = SingleChannelDDR3_1600("16GiB")
    return memory
```
