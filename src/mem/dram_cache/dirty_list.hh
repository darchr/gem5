/* Copyright (c) 2016 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Jason Lowe-Power
 */

#ifndef __MEM_DRAM_CACHE_DIRTY_LIST_HH__
#define __MEM_DRAM_CACHE_DIRTY_LIST_HH__

#include <map>
#include <set>
#include <unordered_map>

#include "base/callback.hh"
#include "base/statistics.hh"
#include "params/AbstractDirtyList.hh"
#include "params/FullSADirtyList.hh"
#include "params/InfiniteDirtyList.hh"
#include "params/LimitedSADirtyList.hh"
#include "params/SplitDirtyList.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"
#include "sim/system.hh"

class DRAMCacheCtrl;

class AbstractDirtyList : public ClockedObject
{
  protected:
    unsigned blockSize;
    uint64_t size;
    uint64_t bankLines;
    unsigned banks;
    unsigned bankNumber;

    DRAMCacheCtrl *dramCacheCtrl;
    System *system;

    /**
     * Return the "cache line number" for this block across the whole cache.
     */
    Addr getCacheLineNo(Addr block_addr) {
        // Global cache line number
        Addr line_no = block_addr / blockSize / banks;
        return line_no % bankLines;
    }

  public:
    AbstractDirtyList(const AbstractDirtyListParams* p) :
        ClockedObject(p), blockSize(p->block_size), size(p->cache_size),
        bankLines(size / blockSize), banks(p->banks),
        dramCacheCtrl(nullptr), system(p->system)
        { }

    void registerDirtyList(DRAMCacheCtrl *controller, int bank_number) {
        dramCacheCtrl = controller;
        bankNumber = bank_number;
    }

    /**
     * Checks to see if the line is dirty in atomic mode. Used to check for
     * whether or not a block that is in the cache is dirty or not
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if dirty, false otherwise
     */
    virtual bool checkDirty(Addr real_addr) = 0;

    /**
     * Checks to see if it's safe to write this address into the cache.
     * It's safe if either the line is clean in the cache, or, if the line
     * is dirty, then the tags much match.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if safe, false otherwise
     */
    virtual bool checkSafe(Addr real_addr) = 0;

    /**
     * Mark that this address is dirty in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is dirty in the cache
     */
    virtual void markDirty(Addr real_addr) = 0;

    /**
     * Mark that this address now clean in the cache or not in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is now *not* dirty in the cache
     */
    virtual void removeDirty(Addr real_addr) = 0;

    /**
     * Check to see if there are any entries left in the dirty list
     * The address is needed in case of set associativity
     *
     * @param addr the address of what we would like to store in the DL
     * @return true if the dirty list if full
     */
    virtual bool checkFull(Addr addr) = 0;

    /**
     * Return the actual address (tag) of a block to victimize
     *
     * @param addr the address of what we would like to store in the DL
     * @return address of block to victimize
     */
    virtual Addr getVictim(Addr addr) = 0;

    /**
     * Check to see if we can insert this address into the dirty list
     *
     * @param addr the address of what we would like to store in the DL
     * @return true if there is a free tag or a superblock tag match
     */
    virtual bool canInsert(Addr addr) = 0;
};



class InfiniteDirtyList : public AbstractDirtyList
{
  private:
    // Map from cache line number to the actual address that is stored there
    std::map<Addr,Addr> dirtyLines;

    Stats::Scalar miss;
    Stats::Scalar tagMatch;
    Stats::Scalar hit;
    Stats::Scalar writeToWrite;

    Stats::Histogram activeEntries;

    /**
     * Sample the number of active entries.
     * Reschedules another sample in 10us
     */
    void sampleActiveEntries() {
        activeEntries.sample(dirtyLines.size());
        // Sample every 10us
        schedule(&sampleEvent, curTick() + 10000000);
    }

    EventWrapper<InfiniteDirtyList, &InfiniteDirtyList::sampleActiveEntries>
        sampleEvent;

  public:

    InfiniteDirtyList(InfiniteDirtyListParams* params);

    void regStats() override;

    void init() override { schedule(&sampleEvent, 10000000); }

    /**
     * Checks to see if the line is dirty in atomic mode. Used to check for
     * whether or not a block that is in the cache is dirty or not
     * This returns true only if the specific address is dirty. It does not
     * return true if there is a dirty block in the cache with a different tag
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if dirty, false otherwise
     */
    bool checkDirty(Addr real_addr) override;

    /**
     * Checks to see if it's safe to write this address into the cache.
     * It's safe if either the line is clean in the cache, or, if the line
     * is dirty, then the tags much match.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if safe, false otherwise
     */
    bool checkSafe(Addr real_addr) override;

    /**
     * Mark that this address is dirty in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is dirty in the cache
     */
    void markDirty(Addr real_addr) override;

    /**
     * Mark that this address now clean in the cache or not in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is now *not* dirty in the cache
     */
    void removeDirty(Addr real_addr) override;

    /**
     * Check to see if there are any entries left in the dirty list
     * The address is needed in case of set associativity
     * Always return false since this is an infinite cach
     * @param addr the address of what we would like to store in the DLe
     *
     * @return true if the dirty list if full
     */
    bool checkFull(Addr addr) override { return false; };

    /**
     * Return the actual address (tag) of a block to victimize
     * This is an inifinite cache, so it should never victimiz
     * @param addr the address of what we would like to store in the DL
     *
     * @return address of block to victimize
     */
    Addr getVictim(Addr addr) override { panic("Should never be full"); }


    /**
     * Check to see if we can insert this address into the dirty list
     * Always return true since it's inifinite
     * @param addr the address of what we would like to store in the DL
     * @return true if there is a free tag or a superblock tag match
     */
    bool canInsert(Addr addr) override { return true; }
};

class FullSADirtyList : public AbstractDirtyList
{
  private:
    struct Tag {
        bool valid;
        Addr tag;
        Tag(): valid(false), tag(0) { }
    };

    struct Set {
        int ways;
        FullSADirtyList *dirtyList;
        // Assume that all lines are dirty, not just the ones with tags
        bool allDirty;
        // Number of dirty lines that map to this set
        int numDirty;
        // The few tags that we're tracking
        std::vector<Tag> tags;
        Set(int ways, FullSADirtyList* dl)
            : ways(ways), dirtyList(dl), allDirty(false), numDirty(0)
        {
            tags.resize(ways);
        }
        /// @return the tag for the currently cached line, or null
        Tag* getTagMatch(Addr real_addr) {
            for (auto& tag: tags) {
                if (tag.valid && (dirtyList->getCacheLineNo(tag.tag) ==
                                  dirtyList->getCacheLineNo(real_addr))) {
                    return &tag;
                }
            }
            return nullptr;
        }
    };

    std::vector<Set> dirtyListArray;

    // Size of each region (in bytes)
    int regionSize;
    // Number of cache lines in each region
    int regionLines;
    // Number of tags to use per entry. If this is less than the number of
    // lines per region, then you may see some "saturatedHits"
    int tagsPerEntry;
    // Number of regions in the cache
    int cacheRegions;
    // Number of cache region entries in the dirty list
    // If this is the same as the number of cache regions, it's a complete DL
    int entries;

    unsigned getSetIndex(Addr addr) {
        Addr lineno = getCacheLineNo(addr);
        // This is not the normal set index calculation. Normally, it would be
        // mod to use the lower bits. But we actually want to use the upper
        // bits so that we can take advantage of region behavior
        return lineno / regionLines;
    }

    Stats::Scalar miss;
    Stats::Scalar tagMatch;
    Stats::Scalar hit;
    Stats::Scalar saturatedHit;
    Stats::Scalar saturations;
    Stats::Scalar unSaturations;

    Stats::Scalar zeroDirtyLinesSets;
    Stats::Histogram dirtyLinesPerSet;
    Stats::Histogram dirtyRegionsPerSet4;
    Stats::Histogram dirtyRegionsPerSet16;
    Stats::Histogram dirtyRegionsPerSet64;

    /**
     * Sample the number of active entries.
     * Reschedules another sample in 10us
     */
    void sampleActiveEntries() {
        for (auto& set : dirtyListArray) {
            dirtyLinesPerSet.sample(set.numDirty);
            if (set.numDirty == 0) {
                zeroDirtyLinesSets++;
                continue;
            }
            std::set<Addr> regions4;
            std::set<Addr> regions16;
            std::set<Addr> regions64;
            for (auto& tag : set.tags) {
                if (!tag.valid) continue;
                // Note: Add 6 for block size and 4 for 16 banks
                regions4.insert(tag.tag >> 12);
                regions16.insert(tag.tag >> 14);
                regions64.insert(tag.tag >> 16);
            }
            dirtyRegionsPerSet4.sample(regions4.size());
            dirtyRegionsPerSet16.sample(regions16.size());
            dirtyRegionsPerSet64.sample(regions64.size());
        }
    }

    class StatsCallback : public Callback {
        FullSADirtyList* dl;
      public:
        StatsCallback(FullSADirtyList* dl) : dl(dl) { }
        void process() override { dl->sampleActiveEntries(); }
    };

  public:

    FullSADirtyList(FullSADirtyListParams* params);

    void regStats() override;

    /**
     * Checks to see if the line is dirty in atomic mode. Used to check for
     * whether or not a block that is in the cache is dirty or not
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if dirty, false otherwise
     */
    bool checkDirty(Addr real_addr) override;

    /**
     * Checks to see if it's safe to write this address into the cache.
     * It's safe if either the line is clean in the cache, or, if the line
     * is dirty, then the tags much match.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if safe, false otherwise
     */
    bool checkSafe(Addr real_addr) override;

    /**
     * Mark that this address is dirty in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is dirty in the cache
     */
    void markDirty(Addr real_addr) override;

    /**
     * Mark that this address now clean in the cache or not in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is now *not* dirty in the cache
     */
    void removeDirty(Addr real_addr) override;

    /**
     * Check to see if there are any entries left in the dirty list
     * The address is needed in case of set associativity
     *
     * @param addr the address of what we would like to store in the DL
     * @return true if the dirty list if full
     */
    bool checkFull(Addr addr) override;

    /**
     * Return the actual address (tag) of a block to victimize
     * Random victim for now
     *
     * @param addr the address of what we would like to store in the DL
     * @return address of block to victimize
     */
    Addr getVictim(Addr addr) override;

    /**
     * Check to see if we can insert this address into the dirty list
     * Always return true since it's inifinite
     * @param addr the address of what we would like to store in the DL
     * @return true if there is a free tag or a superblock tag match
     */
    bool canInsert(Addr addr) override { return true; }
};

class LimitedSADirtyList : public AbstractDirtyList
{
  private:
    struct Tag {
        // Which lines are valid (in the current superblock)
        std::vector<bool> validLines;
        // Which lines are dirty in cache region
        std::vector<bool> dirtyLines;
        bool valid;
        Addr superBlockTag;
        Addr superFrameTag;
        // Number of times we checked safe for a different superblock (and it
        // was not safe)
        int hits;
        Tag(): valid(false), superBlockTag(0), superFrameTag(0), hits(0) { }
        int dirtyCount() {
            return std::count(dirtyLines.begin(), dirtyLines.end(), true);
        }
        int validCount() {
            return std::count(validLines.begin(), validLines.end(), true);
        }
    };

    struct Set {
        LimitedSADirtyList *dirtyList;

        std::vector<Tag> tags;

        int regionLines;

        Set(LimitedSADirtyList* dl, int ways, int region_lines)
            : dirtyList(dl), regionLines(region_lines)
        {
            tags.resize(ways);
        }
        /// @return the tag for the currently cached line, or null
        Tag* getTagMatch(Addr real_addr) {
            for (auto& tag: tags) {
                if (tag.valid && (tag.superFrameTag ==
                                  dirtyList->getSuperFrameTag(real_addr))) {
                    return &tag;
                }
            }
            return nullptr;
        }
    };

    std::vector<Set> dirtyListArray;

    // Number of cache region entries in the dirty list
    // If this is the same as the number of cache regions, it's a complete DL
    int entries;
    // Size of each region (in bytes)
    int regionSize;
    // Number of cache lines in each region
    int regionLines;
    // Number of regions in the cache
    int cacheRegions;
    // Number of ways in each set
    int ways;
    // Number of sets in the storage array
    int sets;

    // Number of hits to a set with a different superblock before we force a
    // clean.
    int hitCleanThreshold;

    // If true, use a less conservative (higher area) way of detecting safety.
    // This assumes that each tag has 2 full bitvectors.
    bool fullBitvector;

    std::set<Addr> currentlyCleaning;

    unsigned getSetIndex(Addr addr) {
        Addr lineno = getCacheLineNo(addr);
        // This is not the normal set index calculation. Normally, it would be
        // mod to use the lower bits. But we actually want to use the upper
        // bits so that we can take advantage of region behavior
        return (lineno / regionLines) % sets;
    }

    Addr getSuperBlockTag(Addr addr) {
        return addr / regionSize;
    }

    Addr getSuperFrameTag(Addr addr) {
        return getCacheLineNo(addr) / regionLines;
    }

    Addr getSuperBlockOffset(Addr addr) {
        return (addr % regionSize) / blockSize / banks;
    }

    /**
     * Same as checkSafe, but assumes that there are full bitvectors in DL
     */
    bool checkSafeBitvector(Addr real_addr);

    /**
     * Same as checkSafe, but assumes only a counter for the current tag in DL
     */
    bool checkSafeSingleTag(Addr real_addr);

    /**
     * Sample the number of active entries.
     * Reschedules another sample in 10us
     */
    void sampleActiveEntries() {
        for (auto& set : dirtyListArray) {
            for (auto &tag : set.tags) {
                if (tag.valid) {
                    int dirty = tag.dirtyCount();
                    int dirty_in_tag = tag.validCount();
                    dirtyLinesPerSet.sample(dirty);
                    dirtyLinesInSuperBlock.sample(dirty_in_tag);
                    dirtyLinesNotInSuperBlock.sample(dirty - dirty_in_tag);
                } else {
                    zeroDirtyLinesSets++;
                }
            }
        }
    }

    class StatsCallback : public Callback {
        LimitedSADirtyList* dl;
      public:
        StatsCallback(LimitedSADirtyList* dl) : dl(dl) { }
        void process() override { dl->sampleActiveEntries(); }
    };

    class CleanSetEvent : public Event {
      private:
        DRAMCacheCtrl *ctrl;
        LimitedSADirtyList *dl;
        Addr currentAddr;
        int addrLeft;
        Addr region;
      public:
        CleanSetEvent(DRAMCacheCtrl* ctrl, LimitedSADirtyList *dl,
                      Addr region) :
            ctrl(ctrl), dl(dl),
            currentAddr(region*dl->regionSize + dl->bankNumber*dl->blockSize),
            addrLeft(dl->regionLines), region(region)
        {}
        void process();
    };

    Stats::Scalar tagMatch;
    Stats::Scalar hit;
    Stats::Scalar miss;
    Stats::Scalar lineNotDirtyMiss;
    Stats::Scalar tagMissButNotDirty;
    Stats::Scalar lineDirtyLineTagMiss;
    Stats::Scalar dirtyRegionHit;
    Stats::Scalar markDirtyTagHit;
    Stats::Scalar saturations;
    Stats::Scalar clearTag;
    Stats::Scalar clearSet;
    Stats::Scalar cleanForTagFree;

    Stats::Scalar zeroDirtyLinesSets;
    Stats::Histogram dirtyLinesPerSet;
    Stats::Histogram dirtyLinesInSuperBlock;
    Stats::Histogram dirtyLinesNotInSuperBlock;
    Stats::Histogram linesToClean;

  public:

    LimitedSADirtyList(LimitedSADirtyListParams* params);

    void regStats() override;

    /**
     * Checks to see if the line is dirty in atomic mode. Used to check for
     * whether or not a block that is in the cache is dirty or not
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if dirty, false otherwise
     */
    bool checkDirty(Addr real_addr) override;

    /**
     * Checks to see if it's safe to write this address into the cache.
     * It's safe if either the line is clean in the cache, or, if the line
     * is dirty, then the tags much match.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if safe, false otherwise
     */
    bool checkSafe(Addr real_addr) override;

    /**
     * Mark that this address is dirty in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is dirty in the cache
     */
    void markDirty(Addr real_addr) override;

    /**
     * Mark that this address now clean in the cache or not in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is now *not* dirty in the cache
     */
    void removeDirty(Addr real_addr) override;

    /**
     * Check to see if there are any entries left in the dirty list
     * The address is needed in case of set associativity
     *
     * @param addr the address of what we would like to store in the DL
     * @return true if the dirty list if full
     */
    bool checkFull(Addr addr) override;

    /**
     * Return the actual address (tag) of a block to victimize
     * Random victim for now
     *
     * @param addr the address of what we would like to store in the DL
     * @return address of block to victimize
     */
    Addr getVictim(Addr addr) override;


    /**
     * Check to see if we can insert this address into the dirty list
     *
     * @param addr the address of what we would like to store in the DL
     * @return true if there is a free tag or a superblock tag match
     */
    bool canInsert(Addr addr) override;
};

class SplitDirtyList : public AbstractDirtyList
{
  private:

    struct Entry {
        int dirty;
        std::vector<bool> dirtyLines;
        Entry(int region_lines) : dirty(0) { dirtyLines.resize(region_lines); }
    };
    std::vector<Entry> dirtyListArray;

    struct TagEntry {
        Addr addr;
        Entry* entry;
    };

    std::map<Addr,TagEntry> tagsArray;

    // For functionally tracking the dirty blocks in the cache.
    std::set<Addr> dirtyBlockTags;

    // Number of cache region entries in the dirty list
    // If this is the same as the number of cache regions, it's a complete DL
    int entries;
    // Size of each region (in bytes)
    int regionSize;
    // Number of cache lines in each region
    int regionLines;
    // Number of regions in the cache
    int cacheRegions;
    // Number of tags in the tag array
    int tags;

    std::set<Addr> currentlyCleaning;

    Addr getSuperBlockTag(Addr addr) {
        return addr / regionSize;
    }

    Addr getSuperFrameTag(Addr addr) {
        return getCacheLineNo(addr) / regionLines;
    }

    Addr getSuperBlockOffset(Addr addr) {
        return (addr % regionSize) / blockSize / banks;
    }

    Stats::Scalar miss;
    Stats::Scalar tagMatch;
    Stats::Scalar tagCouldMatch;
    Stats::Scalar hit;
    Stats::Scalar hitNoTag;
    Stats::Scalar clearTag;
    Stats::Scalar clearSet;
    Stats::Scalar markDirtyTagHit;
    Stats::Scalar tagArrayEviction;
    Stats::Scalar superblockAlias;
    Stats::Scalar superblockAliasErase;

    Stats::Scalar zeroDirtyLinesSets;
    Stats::Scalar tagsInUse;
    Stats::Histogram dirtyLinesPerSet;


    class StatsCallback : public Callback {
        SplitDirtyList* dl;
      public:
        StatsCallback(SplitDirtyList* dl) : dl(dl) { }
        void process() override { dl->sampleActiveEntries(); }
    };

    /**
     * Sample the number of active entries.
     * Reschedules another sample in 10us
     */
    void sampleActiveEntries() {
        for (auto& entry : dirtyListArray) {
            if (entry.dirty > 0) {
                dirtyLinesPerSet.sample(entry.dirty);
            } else {
                zeroDirtyLinesSets++;
            }
        }
        tagsInUse = tagsArray.size();
    }

  public:

    SplitDirtyList(SplitDirtyListParams* params);

    void regStats() override;

    /**
     * Checks to see if the line is dirty in atomic mode. Used to check for
     * whether or not a block that is in the cache is dirty or not
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if dirty, false otherwise
     */
    bool checkDirty(Addr real_addr) override;

    /**
     * Checks to see if it's safe to write this address into the cache.
     * It's safe if either the line is clean in the cache, or, if the line
     * is dirty, then the tags much match.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  we should check
     *
     * @return true if safe, false otherwise
     */
    bool checkSafe(Addr real_addr) override;

    /**
     * Mark that this address is dirty in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is dirty in the cache
     */
    void markDirty(Addr real_addr) override;

    /**
     * Mark that this address now clean in the cache or not in the cache.
     *
     * @param real_addr The physical memory address (aligned to a block) that
     *                  is now *not* dirty in the cache
     */
    void removeDirty(Addr real_addr) override;

    /**
     * Check to see if there are any entries left in the dirty list
     * The address is needed in case of set associativity
     *
     * @param addr the address of what we would like to store in the DL
     * @return true if the dirty list if full
     */
    bool checkFull(Addr addr) override;

    /**
     * Return the actual address (tag) of a block to victimize
     * Random victim for now
     *
     * @param addr the address of what we would like to store in the DL
     * @return address of block to victimize
     */
    Addr getVictim(Addr addr) override;

    /**
     * Check to see if we can insert this address into the dirty list
     * Always return true since it's inifinite
     * @param addr the address of what we would like to store in the DL
     * @return true if there is a free tag or a superblock tag match
     */
    bool canInsert(Addr addr) override { return true; }
};

#endif
