# README

This README details how to build a RISCV full system that utilizes the
lupIO devices. The LupIO devices were created by Professor Joël Porquet-Lupine,
and more information about the device can be found here (https://luplab.cs.ucdavis.edu/assets/lupio/wcae21-porquet-lupio-paper.pdf).  The specs for each individual device can
be found here (https://gitlab.com/luplab/lupio/lupio-specs), and the Linux drivers
for each of these devices can be found here (https://gitlab.com/luplab/lupio/linux).

In order to run this system, you need the following:

* `run_lupv.py`
* a built version of the lupIO gem5 repo
* bbl  
* disk image  

Both the bbl and a working disk image can be found on the LupIO+gem5 Google
Drive. In addition, instructions to build a version of Linux with the lupIO
drivers are there, but should not be needed.

In order for `run_lupv.py` to work in its current state, move it within the
gem5 directory.

Once you have everything set up, you can run the RISCV full system by using
the following command line prompt. Note that for the cpu type, you can choose
between `atomic` and `simple` modes.

``` bash
[gem5 binary] gem5/run_lupv.py [path to bbl] [path to the disk image] [cpu type] [num cpus]
```

example:

```bash
gem5/build/RISCV/gem5.opt gem5/run_lupv.py bbl rootfs.img atomic 1
```

Once you've started the gem5 simulation, you can open up a separate terminal
and use m5term to connect to the simulated console. The port number will be
specified in the gem5 simulation.

example:

```bash
m5term localhost 3456
```

This should allow you to run busybox, in which you can see the LupIO device at
work!
