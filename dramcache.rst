=================================
Documentation on DRAM cache model
=================================

Patch description
-----------------

Two main patches/changesets for the DRAM cache.
 # dramcache: This patch contains all of the source code for the DRAM cache
 # configs-all: This patch contains all of the configs for my paper including the configs for the DRAM cache

All of the other patches are just supporting patches. Some for getting KVM to work on my computer and some for doing sampling.


Different DRAM cache designs
----------------------------

There are four DRAM cache designs included in the code.
 # DRAMCacheFOM: A fill-on-miss design that closely follows the Intel Knight's Landing cache design. On a cache miss there is an extra request to the DRAM controller to reserve the location (no MSHRs). This is dumb, but it's what KNL does.
 # DRAMCacheCtrl with writeback policy: This is a victim cache design. On a miss nothing is inserted into the cache. Inserts occur only on LLC writebacks.
 # DRAMCacheCtrl with writethrough policy: This a the same victim cache described above, but writes all dirty data through to main-memory. Thus, this cache is fully clean. This policy improves performance for some applications and hurts performance in others.
 # DRAMCacheCtrl with adaptive policy: This is a victim cache that adaptively moves between the victim clean and victim dirty designs depending on the address stream. This is the main contribution of my paper.

Overall code design
-------------------
There are two main pieces of code. The DRAMCacheCtrl is the cache control logic. It has a DRAMCacheStorage object. The DRAMCacheStorage object is a DRAM controller. Thus, the DRAM controller and the cache controller are totally separate logic. These communicate over a "wire" interface (i.e. a function), not over ports.

The DRAM cache is assumed to be banked based on the low-order bits of the address for the most parallelism and bandwidth. The banks are totally separate and do not communicate at all.

You can ignore the DirtyList code for all of the cache designs except the adaptive policy.

All of the code should be well documented in comments.

gem5 configs
------------

The configs I used in my paper can be found in configs/myconfigs/mysys. dram_cache.py is a wrapper around the DRAMCacheCtrl to configure it properly. It's a pretty messy file. Sorry.
It should be pretty straightforward to remove the dirty list (adaptive) code. You should use "no_dirty_list=True" for the FOM, writethrough, and writeback designs.

main_systems.py gives the exact systems I used in the paper. This is a good place to start for the baseline designs.

The rest of the code in myconfigs is for running and for sampling. It's fairly well docuemented in comments.

Random specifics
----------------

Victim cache oddities
~~~~~~~~~~~~~~~~~~~~~
To reduce the overhead of writing back clean data when it is already in the cache, we assume that the LLC tracks whether the data is in the DRAM cache or not. This adds an extra bit to the LLC tag. However, we do not implement it this way. Instead we add a filter at the LLC crossbar. This is functionally the same thing, but much easier to implement in gem5.
