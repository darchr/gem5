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

#include "mem/dram_cache/dirty_list.hh"

#include "base/random.hh"
#include "debug/DRAMCache.hh"
#include "mem/dram_cache/dram_cache_ctrl.hh"

InfiniteDirtyList::InfiniteDirtyList(InfiniteDirtyListParams* params) :
    AbstractDirtyList(params), sampleEvent(this)
{
}

bool
InfiniteDirtyList::checkSafe(Addr real_addr)
{
    auto it = dirtyLines.find(getCacheLineNo(real_addr));
    DPRINTF(DRAMCache, "Checking safe tag %#x to %#x. Curent %#x\n",
            getCacheLineNo(real_addr), real_addr, it->second);
    if (it == dirtyLines.end()) {
        // no entry in the dirty list for this cache line.
        // Either the line is clean or has never been written.
        DPRINTF(DRAMCache, "Safe! No entry for this line\n");
        miss++;
        return true;
    } else if (it->second == real_addr) {
        // The cache line is dirty, but the tag matches the current line
        // so we can overwrite it safely.
        DPRINTF(DRAMCache, "Safe! Tag match\n");
        tagMatch++;
        return true;
    } else {
        // There is an entry in the dirty list for this line, and it doesn't
        // match the tag of this address, so we have to write it back.
        hit++;
        DPRINTF(DRAMCache, "NOT Safe! Tag mis-match\n");
        return false;
    }
}

bool
InfiniteDirtyList::checkDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "Checking dirty tag %#x to %#x\n",
            getCacheLineNo(real_addr), real_addr);
    auto it = dirtyLines.find(getCacheLineNo(real_addr));
    if (it == dirtyLines.end()) {
        // no entry in the dirty list for this cache line.
        // Either the line is clean or has never been written.
        return false;
    } else if (it->second == real_addr) {
        // There is an entry in the dirty list for this line
        return true;
    } else {
        return false;
    }
}

void
InfiniteDirtyList::markDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "Setting dirty tag %#x to %#x\n",
            getCacheLineNo(real_addr), real_addr);

    auto it = dirtyLines.find(getCacheLineNo(real_addr));
    panic_if(it != dirtyLines.end() && it->second == real_addr,
             "Write to write Shouldn't have called markDirty!\n");
    panic_if(it != dirtyLines.end(), "Overwriting dirty!");
    dirtyLines[getCacheLineNo(real_addr)] = real_addr;
}

void
InfiniteDirtyList::removeDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "Removing dirty tag %#x to %#x\n",
            getCacheLineNo(real_addr), real_addr);

    auto it = dirtyLines.find(getCacheLineNo(real_addr));
    panic_if(it == dirtyLines.end(), "No match for remove");
    dirtyLines.erase(it);
}

void
InfiniteDirtyList::regStats()
{
    ClockedObject::regStats();

    miss
        .name(name() + ".miss")
        .desc("Check for dirty resulted in no matches found")
        ;
    tagMatch
        .name(name() + ".tagMatch")
        .desc("Check for dirty resulted in a tag match (ok to write)")
        ;
    hit
        .name(name() + ".hit")
        .desc("Check for dirty resulted in a hit to a different addr")
        ;
    writeToWrite
        .name(name() + ".writeToWrite")
        .desc("Times we had a hit in the dirty list when updating it")
        ;

    activeEntries
        .name(name() + ".activeEntries")
        .init(16)
        .desc("Size of dirty list sampled every 10us")
        ;
}

InfiniteDirtyList*
InfiniteDirtyListParams::create()
{
    return new InfiniteDirtyList(this);
}

/****************************************************************************/

FullSADirtyList::FullSADirtyList(FullSADirtyListParams* params) :
    AbstractDirtyList(params),
    regionSize(params->region_size),
    regionLines(regionSize / banks / blockSize),
    tagsPerEntry(regionLines),
    cacheRegions(bankLines / regionLines),
    entries(cacheRegions)
{
    panic_if(regionSize / banks < 64,
             "Region size to small for %d banks", banks);
    warn("This is as big as pure SRAM tags!");

    for (int i = 0; i < entries; i++) {
        dirtyListArray.emplace_back(tagsPerEntry, this);
    }
}

bool
FullSADirtyList::checkSafe(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr));
    Set& set = dirtyListArray[getSetIndex(real_addr)];

    Tag* tag = set.getTagMatch(real_addr);
    if (tag == nullptr) {
        if (set.allDirty) {
            // It's not safe as we need to assume all lines that map to this
            // set are dirty.
            DPRINTF(DRAMCache, "Saturated hit. Unsafe\n");
            saturatedHit++;
            return false;
        } else {
            // In this case, we checked each way and didn't find a match, means
            // that the block this address maps to is NOT dirty, so it's safe
            DPRINTF(DRAMCache, "No tag found. Safe\n");
            miss++;
            return true;
        }
    } else {
        // Valid tag means the line in the cache *is dirty*
        if (tag->tag == real_addr) {
            // If the whole thing matches then it is safe to write it
            // into the cache since we know the line in the cache has
            // the same tag as this line.
            DPRINTF(DRAMCache, "Tag match. Safe\n");
            tagMatch++;
            return true;
        } else {
            DPRINTF(DRAMCache, "Tag mismatch. Unsafe\n");
            hit++;
            return false;
        }
    }
}

bool
FullSADirtyList::checkDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr));
    auto& set = dirtyListArray[getSetIndex(real_addr)];
    Tag* tag = set.getTagMatch(real_addr);
    if (tag && tag->tag == real_addr) {
        return true;
    }

    // Definitely not dirty
    return false;
}

void
FullSADirtyList::markDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x.\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr));

    if (checkDirty(real_addr)) {
        DPRINTF(DRAMCache, "No need to mark dirty... already dirty");
        return;
    }

    auto& set = dirtyListArray[getSetIndex(real_addr)];
    DPRINTF(DRAMCache, "Num dirty in set: %d\n", set.numDirty);

    Tag *tag = set.getTagMatch(real_addr);
    panic_if(tag && tag->tag != real_addr, "Tag missmatch in mark dirty");
    if (!tag) {
        // If there was no match
        set.numDirty++;
        assert(set.numDirty <= regionLines);
        // Try to find an empty location
        Tag* empty = nullptr;
        for (auto& tag: set.tags) {
            if (!tag.valid) {
                empty = &tag;
                break;
            } else {
                assert(tag.tag != 0);
            }
        }

        if (empty) {
            DPRINTF(DRAMCache, "Adding tag\n");
            empty->tag = real_addr;
            empty->valid = true;
        } else {
            DPRINTF(DRAMCache, "All were dirty!\n");
            // There were no entries available so, mark as all dirty
            if (!set.allDirty) {
                saturations++;
            }
            set.allDirty = true;

            // Choose random tag to replace. This will help when lots of
            // tags match
            int way = random_mt.random<int>(0, tagsPerEntry - 1);
            Tag& tag = set.tags[way];
            DPRINTF(DRAMCache, "Replacing tag %#x\n", tag.tag);
            assert(tag.valid && tag.tag != real_addr);
            tag.tag = real_addr;
        }
    }
    DPRINTF(DRAMCache, "Num dirty in set: %d\n", set.numDirty);
}

void
FullSADirtyList::removeDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr));

    auto& set = dirtyListArray[getSetIndex(real_addr)];
    DPRINTF(DRAMCache, "Num dirty in set: %d\n", set.numDirty);

    Tag *tag = set.getTagMatch(real_addr);
    if (tag) {
        tag->valid = false;
        tag->tag = 0;
    }

    set.numDirty--;
    assert(set.numDirty >= 0);

    DPRINTF(DRAMCache, "Num dirty in set: %d\n", set.numDirty);

    // Can we clear allDirty? This will happen if the only dirty things
    // are full tags.
    if (set.allDirty && set.numDirty < set.ways) {
        int valid = 0;
        for (auto& tag: set.tags) {
            if (tag.valid) valid++;
        }
        if (valid == set.numDirty) {
            DPRINTF(DRAMCache, "Clearing all dirty!\n");
            set.allDirty = false;
            unSaturations++;
        }
    }
}

bool
FullSADirtyList::checkFull(Addr real_addr)
{
    return false;
}

Addr
FullSADirtyList::getVictim(Addr real_addr)
{
    panic("Should never be called");
}

void
FullSADirtyList::regStats()
{
    ClockedObject::regStats();

    miss
        .name(name() + ".miss")
        .desc("Check for dirty resulted in no matches found")
        ;
    tagMatch
        .name(name() + ".tagMatch")
        .desc("Check for dirty resulted in a tag match (ok to write)")
        ;
    hit
        .name(name() + ".hit")
        .desc("Check for dirty resulted in a hit to a different addr")
        ;
    saturatedHit
        .name(name() + ".saturatedHit")
        .desc("Check for dirty resulted in a set that's all dirty")
        ;
    saturations
        .name(name() + ".saturations")
        .desc("Number of times a set became saturated: more dirty than tags")
        ;
    unSaturations
        .name(name() + ".unSaturations")
        .desc("Number of times a set became unsaturated: less dirty than tags")
        ;

    zeroDirtyLinesSets
        .name(name() + ".zeroDirtyLinesSets")
        .desc("Sets which have no dirty lines.")
        ;

    dirtyLinesPerSet
        .name(name() + ".dirtyLinesPerSet")
        .init(16)
        .desc("Size of dirty list")
        ;

    dirtyRegionsPerSet4
        .name(name() + ".dirtyRegionsPerSet4")
        .init(16)
        .desc("Size of dirty list")
        ;
    dirtyRegionsPerSet16
        .name(name() + ".dirtyRegionsPerSet16")
        .init(16)
        .desc("Size of dirty list")
        ;
    dirtyRegionsPerSet64
        .name(name() + ".dirtyRegionsPerSet64")
        .init(16)
        .desc("Size of dirty list")
        ;

    Stats::registerDumpCallback(new StatsCallback(this));

}

FullSADirtyList*
FullSADirtyListParams::create()
{
    return new FullSADirtyList(this);
}

/****************************************************************************/

LimitedSADirtyList::LimitedSADirtyList(LimitedSADirtyListParams* params) :
    AbstractDirtyList(params), entries(params->entries),
    regionSize(params->region_size),
    regionLines(regionSize / banks / blockSize),
    cacheRegions(bankLines / regionLines), ways(params->ways),
    sets(entries / ways), hitCleanThreshold(params->clean_threshold),
    fullBitvector(params->full_bitvector)
{
    panic_if(regionSize / banks < 64,
             "Region size to small for %d banks", banks);

    inform("Size is %d B", entries * 5);

    for (int i = 0; i < sets; i++) {
        dirtyListArray.emplace_back(this, ways, regionLines);
    }
}

bool
LimitedSADirtyList::checkSafe(Addr real_addr)
{
    if (fullBitvector) {
        return checkSafeBitvector(real_addr);
    } else {
        return checkSafeSingleTag(real_addr);
    }
}

bool
LimitedSADirtyList::checkSafeBitvector(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x, tag: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr), getSuperBlockTag(real_addr));
    Set& set = dirtyListArray[getSetIndex(real_addr)];
    Tag *tag = set.getTagMatch(real_addr);

    if (!tag) {
        // This super frame is not in the dirty list "cache" so it's safe
        miss++;
        return true;
    }

    DPRINTF(DRAMCache, "This region is dirty\n");
    dirtyRegionHit++;

    if (tag->superBlockTag == getSuperBlockTag(real_addr)) {
        // This tag is tracking the same superblock as the request.

        // Set the hit counter to 0 since we have a match.
        // I know, this is a very confusing counter. It's counting the number
        // of times that the superblock tag in this set is wrong, ina
        tag->hits = 0;

        if (!tag->dirtyLines[getSuperBlockOffset(real_addr)]) {
            // This line isn't dirty in the cache.
            lineNotDirtyMiss++;
            return true;
        } else if (tag->validLines[getSuperBlockOffset(real_addr)]) {
            // This is dirty in the cache, but the tag matches.
            DPRINTF(DRAMCache, "Tag match. Safe\n");
            tagMatch++;
            return true;
        } else {
            // The other tag must have this line valid, so it's not safe.
            DPRINTF(DRAMCache, "Found a tag match, but line isn't valid\n");
            lineDirtyLineTagMiss++;
            return false;
        }
    } else {
        // This is tracking a different superblock.. could still be safe.
        // If none of the tags match, then check to see if this block is dirty
        if (!tag->dirtyLines[getSuperBlockOffset(real_addr)]) {
            // This line isn't dirty in the cache.
            tagMissButNotDirty++;
            return true;
        } else {
            // No tag matches, and the line is dirty. It's not safe to write.
            DPRINTF(DRAMCache, "Tag mismatch. Unsafe\n");
            hit++;
            tag->hits++;
            if (hitCleanThreshold && tag->hits > hitCleanThreshold) {
                Addr region = tag->superBlockTag;
                Addr start = region * regionSize + blockSize * bankNumber;
                if (system->getMemoryMode() != Enums::timing) {
                    dramCacheCtrl->atomicClean(start, regionSize);
                } else {
                    DPRINTF(DRAMCache, "Beginning to clean region %#x\n",
                            real_addr / regionSize);
                    if (!currentlyCleaning.count(tag->superFrameTag)) {
                        linesToClean.sample(tag->dirtyCount());
                        currentlyCleaning.insert(tag->superFrameTag);
                        schedule(new CleanSetEvent(dramCacheCtrl, this,region),
                                 nextCycle());
                    }
                }
            }
            return false;
        }
    }
}

bool
LimitedSADirtyList::checkSafeSingleTag(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x, tag: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr), getSuperBlockTag(real_addr));
    Set& set = dirtyListArray[getSetIndex(real_addr)];
    Tag *tag = set.getTagMatch(real_addr);

    if (!tag) {
        // This super frame is not in the dirty list "cache" so it's safe
        miss++;
        return true;
    }

    DPRINTF(DRAMCache, "This region is dirty\n");
    dirtyRegionHit++;

    if (tag->superBlockTag == getSuperBlockTag(real_addr)) {
        // This tag is tracking the same superblock as the request.

        // Set the hit counter to 0 since we have a match.
        // I know, this is a very confusing counter. It's counting the number
        // of times that the superblock tag in this set is wrong, ina
        tag->hits = 0;

        tagMatch++;
        return true;
    } else {
        hit++;
        tag->hits++;
        if (hitCleanThreshold && tag->hits > hitCleanThreshold) {
            Addr region = tag->superBlockTag;
            Addr start = region * regionSize + blockSize * bankNumber;
            if (system->getMemoryMode() != Enums::timing) {
                dramCacheCtrl->atomicClean(start, regionSize);
            } else {
                DPRINTF(DRAMCache, "Beginning to clean region %#x\n",
                        real_addr / regionSize);
                if (!currentlyCleaning.count(tag->superFrameTag)) {
                    linesToClean.sample(tag->dirtyCount());
                    currentlyCleaning.insert(tag->superFrameTag);
                    schedule(new CleanSetEvent(dramCacheCtrl, this,region),
                             nextCycle());
                }
            }
        }
        return false;
    }
}

bool
LimitedSADirtyList::checkDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x, tag: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr), getSuperBlockTag(real_addr));

    Set& set = dirtyListArray[getSetIndex(real_addr)];
    Tag *tag = set.getTagMatch(real_addr);

    if (!tag) {
        return false;
    }

    if (tag->superBlockTag == getSuperBlockTag(real_addr)) {
        // This tag is tracking the same superblock as the request.
        if (tag->validLines[getSuperBlockOffset(real_addr)]) {
            // Definitely dirty!
            assert(tag->dirtyLines[getSuperBlockOffset(real_addr)]);
            return true;
        }
    }

    // Don't know
    return false;
}

void
LimitedSADirtyList::markDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x, of:%d, tag: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr), getSuperBlockOffset(real_addr),
            getSuperBlockTag(real_addr));

    if (checkDirty(real_addr)) {
        DPRINTF(DRAMCache, "No need to mark dirty... already dirty");
        markDirtyTagHit++;
        return;
    }

    auto& set = dirtyListArray[getSetIndex(real_addr)];
    Tag *tag = set.getTagMatch(real_addr);

    if (!tag) {
        DPRINTF(DRAMCache, "No tag. Allocating\n");
        // no matches, there better be an invalid tag!
        for (auto& it: set.tags) {
            if (!it.valid) {
                tag = &it;
            }
        }
        assert(tag);
        tag->valid = true;
        tag->superBlockTag = getSuperBlockTag(real_addr);
        tag->superFrameTag = getSuperFrameTag(real_addr);
        tag->validLines.resize(regionLines);
        tag->dirtyLines.resize(regionLines);
        assert(tag->dirtyCount() == 0 && tag->validCount() == 0);
    }
    DPRINTF(DRAMCache, "Num dirty in superframe: %d\n", tag->dirtyCount());

    panic_if(tag->dirtyLines[getSuperBlockOffset(real_addr)], "Already dirty");
    panic_if(tag->validLines[getSuperBlockOffset(real_addr)], "Already valid");

    if (tag->superBlockTag == getSuperBlockTag(real_addr)) {
        DPRINTF(DRAMCache, "Updating tag %#x\n", getSuperBlockTag(real_addr));
        assert(!tag->validLines[getSuperBlockOffset(real_addr)]);
        tag->validLines[getSuperBlockOffset(real_addr)] = true;
        markDirtyTagHit++;
    } else {
        panic_if(!fullBitvector, "Cannot mark line in other superblock dirty");
        DPRINTF(DRAMCache, "All were dirty!\n");
        // There were no entries available so, mark as all dirty
        saturations++;
    }

    tag->dirtyLines[getSuperBlockOffset(real_addr)] = true;

    if (!fullBitvector) {
        assert(tag->dirtyCount() == tag->validCount());
    }

    DPRINTF(DRAMCache, "Num dirty in superframe: %d\n", tag->dirtyCount());
}

void
LimitedSADirtyList::removeDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x, tag: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSetIndex(real_addr), getSuperBlockTag(real_addr));

    auto& set = dirtyListArray[getSetIndex(real_addr)];
    Tag *tag = set.getTagMatch(real_addr);

    panic_if(!tag, "How did we think this was dirty??");

    panic_if(!tag->dirtyLines[getSuperBlockOffset(real_addr)],
             "trying to clean a line that's already clean");
    tag->dirtyLines[getSuperBlockOffset(real_addr)] = false;

    if (tag->superBlockTag == getSuperBlockTag(real_addr)) {
        assert(tag->validLines[getSuperBlockOffset(real_addr)]);
        tag->validLines[getSuperBlockOffset(real_addr)] = false;
        if (tag->validCount() == 0) {
            tag->superBlockTag = 0;
            clearTag++;
        }
    }

    DPRINTF(DRAMCache, "Num dirty in superframe: %d\n", tag->dirtyCount());

    if (tag->dirtyCount() == 0) {
        assert(tag->validCount() == 0);
        tag->valid = false;
        tag->hits = 0;
        tag->superFrameTag = 0;
        clearSet++;
    }

    if (tag->validCount() == 0 && tag->dirtyCount() > 0) {
        // There are dirty lines in the cache frame, but the superblock tag
        // is invalid. Let's go ahead and clean it.
        Addr region = real_addr / regionSize;
        Addr start = region * regionSize + blockSize * bankNumber;
        assert(real_addr % regionSize == start % regionSize);
        if (system->getMemoryMode() != Enums::timing) {
            dramCacheCtrl->atomicClean(start, regionSize);
        } else {
            cleanForTagFree++;
            DPRINTF(DRAMCache, "Beginning to clean region %#x\n", region);
            if (!currentlyCleaning.count(region)) {
                linesToClean.sample(tag->dirtyCount());
                currentlyCleaning.insert(region);
                schedule(new CleanSetEvent(dramCacheCtrl, this, region),
                         nextCycle());
            }
        }
    }
}

bool
LimitedSADirtyList::checkFull(Addr real_addr)
{
    return false;
}

bool
LimitedSADirtyList::canInsert(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x, of:%d, tag: %#x\n",
        __func__, real_addr, getCacheLineNo(real_addr),
        getSetIndex(real_addr), getSuperBlockOffset(real_addr),
        getSuperBlockTag(real_addr));

    if (checkDirty(real_addr)) {
        DPRINTF(DRAMCache, "Can insert (it's dirty)\n");
        return true;
    }

    auto& set = dirtyListArray[getSetIndex(real_addr)];
    Tag *tag = set.getTagMatch(real_addr);

    if (!tag) {
        DPRINTF(DRAMCache, "No tag match\n");
        for (auto& tag: set.tags) {
            if (!tag.valid) {
                // There is a tag to use.
                DPRINTF(DRAMCache, "Can insert. Found empty tag\n");
                return true;
            }
        }
        DPRINTF(DRAMCache, "No empty tags :(\n");
        return false;
    } else if (fullBitvector &&
               tag->superFrameTag == getSuperFrameTag(real_addr)) {
        // If using a full bitvector, then it's safe as long as the frame
        // matches
        DPRINTF(DRAMCache, "Can insert (super frame tag match)\n");
        return true;
    } else if (!fullBitvector &&
               tag->superBlockTag == getSuperBlockTag(real_addr)) {
        // If we don't have full bitvectors, then we must only have one super
        // block in each frame
        DPRINTF(DRAMCache, "Can insert (super block tag match)\n");
        return true;
    } else {
        DPRINTF(DRAMCache, "Can't insert!\n");
        return false;
    }
}

Addr
LimitedSADirtyList::getVictim(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SI: %#x, of:%d, tag: %#x\n",
        __func__, real_addr, getCacheLineNo(real_addr),
        getSetIndex(real_addr), getSuperBlockOffset(real_addr),
        getSuperBlockTag(real_addr));

    assert(system->getMemoryMode() != Enums::timing);

    auto& set = dirtyListArray[getSetIndex(real_addr)];
    Tag *tag = set.getTagMatch(real_addr);

    // Should never find a tag match!
    assert(!tag || tag->superBlockTag != getSuperBlockTag(real_addr));

    if (!tag) {
        int way = random_mt.random<int>(0, ways);
        DPRINTF(DRAMCache, "no valid super frame tag match, using %d\n", way);
        tag = &set.tags[way];
    }

    assert(tag->valid);

    DPRINTF(DRAMCache, "Replacing frame tag: %#x, superblock tag: %#x\n",
            tag->superFrameTag, tag->superBlockTag);

    // Do I need to add the bank number * blockSize here?
    Addr region = tag->superBlockTag * regionSize;
    Addr start = region + blockSize * bankNumber;
    dramCacheCtrl->atomicClean(start, regionSize);

    DPRINTF(DRAMCache, "done cleaning region %#x\n", region);

    return 0;
}

void
LimitedSADirtyList::CleanSetEvent::process()
{
    DPRINTFS(DRAMCache, dl, "Trying to clean %#x. %d left\n", currentAddr,
             addrLeft);
    if (ctrl->startCleaningReplace(currentAddr)) {
        currentAddr += dl->blockSize * dl->banks;
        addrLeft--;
    }
    if (addrLeft > 0) {
        dl->schedule(this, dl->nextCycle());
    } else {
        DPRINTFS(DRAMCache, dl, "Done with region %#x\n", region);
        dl->currentlyCleaning.erase(region);
        delete this;
    }
}

void
LimitedSADirtyList::regStats()
{
    ClockedObject::regStats();

    tagMatch
        .name(name() + ".tagMatch")
        .desc("The line is dirty, the superblock tag matches, and the line is"
              " valid")
        ;
    hit
        .name(name() + ".hit")
        .desc("The line is dirty, but the superblock tag doesn't match")
        ;
    miss
        .name(name() + ".miss")
        .desc("The set this line maps to is totally clean")
        ;
    lineNotDirtyMiss
        .name(name() + ".lineNotDirtyMiss")
        .desc("The set is dirty, but this particular line is clean")
        ;
    tagMissButNotDirty
        .name(name() + ".tagMissButNotDirty")
        .desc("The superblock doesn't match, but the line isn't dirty (miss)")
        ;
    lineDirtyLineTagMiss
        .name(name() + ".lineDirtyLineTagMiss")
        .desc("Line is dirty, the superblock tag matches, but the line isn't"
              " valid")
        ;
    dirtyRegionHit
        .name(name() + ".dirtyRegionHit")
        .desc("Hit to region (super block) tag")
        ;
    markDirtyTagHit
        .name(name() + ".markDirtyTagHit")
        .desc("Hit tag when marking dirty (already dirty or superblock hit)")
        ;
    saturations
        .name(name() + ".saturations")
        .desc("Couldn't add a super block tag when marking dirty")
        ;
    clearTag
        .name(name() + ".clearTag")
        .desc("Freed a super block tag")
        ;
    clearSet
        .name(name() + ".clearSet")
        .desc("The whole set (cache region) went from dirty to clean")
        ;

    zeroDirtyLinesSets
        .name(name() + ".zeroDirtyLinesSets")
        .desc("Sets which have no dirty lines.")
        ;

    dirtyLinesPerSet
        .name(name() + ".dirtyLinesPerSet")
        .init(16)
        .desc("Number of blocks in the set that are dirty")
        ;
    dirtyLinesInSuperBlock
        .name(name() + ".dirtyLinesInSuperBlock")
        .init(16)
        .desc("Number of blocks in dirty in the superblock")
        ;
    dirtyLinesNotInSuperBlock
        .name(name() + ".dirtyLinesNotInSuperBlock")
        .init(16)
        .desc("Number of blocks that are dirty in a different superblock")
        ;
    linesToClean
        .name(name() + ".linesToClean")
        .init(16)
        .desc("Number of dirty lines when we start a clean region request")
        ;

    Stats::registerDumpCallback(new StatsCallback(this));

}

LimitedSADirtyList*
LimitedSADirtyListParams::create()
{
    return new LimitedSADirtyList(this);
}

SplitDirtyList::SplitDirtyList(SplitDirtyListParams* params) :
    AbstractDirtyList(params), entries(params->entries),
    regionSize(params->region_size),
    regionLines(regionSize / banks / blockSize),
    cacheRegions(bankLines / regionLines)
{
    panic_if(regionSize / banks < 64,
             "Region size to small for %d banks", banks);

    // May need a for loop here
    dirtyListArray.resize(cacheRegions, regionLines);
}

bool
SplitDirtyList::checkSafe(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SFT: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSuperFrameTag(real_addr));

    // This is the entry in the tag array
    auto it = tagsArray.find(getSuperFrameTag(real_addr));
    // This is the entry in the dirty line tracker
    int num_dirty = dirtyListArray[getSuperFrameTag(real_addr)].dirty;

    if (it == tagsArray.end()) {
        // No tag match, as long as there are no dirty lines, it's safe.
        if (num_dirty == 0) {
            miss++;
            DPRINTF(DRAMCache, "Safe! No entry for this line\n");
            return true;
        } else {
            // There are some dirty lines in the super frame, but we aren't
            // tracking the tags for the dirty lines.
            DPRINTF(DRAMCache, "UnSafe! Some dirty, no tag\n");
            if (checkDirty(real_addr)) {
                tagCouldMatch++;
            }
            hitNoTag++;
            return false;
        }
    } else if (it->second.addr == getSuperBlockTag(real_addr)) {
        // Tag match! This is safe.
        DPRINTF(DRAMCache, "Safe! Tag match\n");
        tagMatch++;
        return true;
    } else {
        // There is an entry in the dirty list for this line, and it doesn't
        // match the tag of this address, so we have to write it back.
        assert(num_dirty > 0);
        hit++;
        DPRINTF(DRAMCache, "NOT Safe! Tag mis-match\n");
        return false;
    }
}

bool
SplitDirtyList::checkDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SFT: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSuperFrameTag(real_addr));

    return (dirtyBlockTags.count(real_addr) == 1);
}

void
SplitDirtyList::markDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SFT: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSuperFrameTag(real_addr));

    if (checkDirty(real_addr)) {
        DPRINTF(DRAMCache, "No need to mark dirty... already dirty\n");
        return;
    }

    dirtyBlockTags.insert(real_addr);

    // This is the entry in the tag array
    auto it = tagsArray.find(getSuperFrameTag(real_addr));
    // This is the entry in the dirty line tracker
    Entry& e = dirtyListArray[getSuperFrameTag(real_addr)];

    panic_if(e.dirtyLines[getSuperBlockOffset(real_addr)], "Already dirty?");

    if (it == tagsArray.end() && e.dirty == 0) {
        // Can only insert into the tag array if we know this is the only
        // super block in the super frame
        if (tagsArray.size() < entries) {
            DPRINTF(DRAMCache, "Adding tag entry.\n");
            TagEntry& tag = tagsArray[getSuperFrameTag(real_addr)];
            tag.addr = getSuperBlockTag(real_addr);
            tag.entry = &e;
        } else {
            tagArrayEviction++;
            // Evict an entry and insert this one.
            int min_dirty = regionLines;
            Addr the_one = 0;
            for (auto& tag : tagsArray) {
                if (tag.second.entry->dirty < min_dirty) {
                    min_dirty = tag.second.entry->dirty;
                    the_one = tag.first;
                }
            }
            TagEntry& tag = tagsArray[the_one];
            DPRINTF(DRAMCache, "Evicting tag. SFT: %#x. SBT: %#x. %d dirty\n",
                    the_one, tag.addr, tag.entry->dirty);
            tag.addr = getSuperBlockTag(real_addr);
            tag.entry = &e;
        }
    } else if (it != tagsArray.end()) {
        if (it->second.addr == getSuperBlockTag(real_addr)) {
            // This is a tag match. We don't have to do anything but mark the
            // line as dirty below
            markDirtyTagHit++;
        } else {
            DPRINTF(DRAMCache, "Entry in dirty tags is a superblock miss\n");
            // We must remove the entry in the tags to make sure that we don't
            // think it's safe when it's not.
            tagsArray.erase(it);
            superblockAliasErase++;
        }
    } else {
        // No matching tag in tag array, but the frame is dirty.
        superblockAlias++;
    }

    e.dirtyLines[getSuperBlockOffset(real_addr)] = true;
    e.dirty++;
}

void
SplitDirtyList::removeDirty(Addr real_addr)
{
    DPRINTF(DRAMCache, "%s Addr: %#x, LineNo: %#x, SFT: %#x\n",
            __func__, real_addr, getCacheLineNo(real_addr),
            getSuperFrameTag(real_addr));

    panic_if(dirtyBlockTags.count(real_addr) != 1, "Should be dirty!");
    dirtyBlockTags.erase(real_addr);

    // This is the entry in the tag array
    auto it = tagsArray.find(getSuperFrameTag(real_addr));
    // This is the entry in the dirty line tracker
    Entry& e = dirtyListArray[getSuperFrameTag(real_addr)];

    panic_if(!e.dirtyLines[getSuperBlockOffset(real_addr)], "not dirty!?");

    e.dirty--;
    e.dirtyLines[getSuperBlockOffset(real_addr)] = false;

    if (e.dirty == 0) {
        clearSet++;
    }

    if (it != tagsArray.end()) {
        assert(it->second.addr == getSuperBlockTag(real_addr));
        if (e.dirty == 0) {
            tagsArray.erase(it);
            clearTag++;
        }
    }
}

bool
SplitDirtyList::checkFull(Addr real_addr)
{
    return false;
}

Addr
SplitDirtyList::getVictim(Addr real_addr)
{
    panic("Should never be called.");
    return 0;
}

void
SplitDirtyList::regStats()
{
    ClockedObject::regStats();

    tagMatch
        .name(name() + ".tagMatch")
        .desc("The line is dirty, the superblock tag matches, and the line is"
              " valid")
        ;

    tagCouldMatch
        .name(name() + ".tagCouldMatch")
        .desc("The line is dirty in the cache, it could have matched")
        ;
    hit
        .name(name() + ".hit")
        .desc("The line is dirty, but the superblock tag doesn't match")
        ;
    hitNoTag
        .name(name() + ".hitNoTag")
        .desc("The frame is dirty, and we aren't tracking the tag")
        ;
    miss
        .name(name() + ".miss")
        .desc("The set this line maps to is totally clean")
        ;

    clearTag
        .name(name() + ".clearTag")
        .desc("Remove the tag as well as the dirty list entry")
        ;
    clearSet
        .name(name() + ".clearSet")
        .desc("This entry is now completely clean.")
        ;
    markDirtyTagHit
        .name(name() + ".markDirtyTagHit")
        .desc("This entry is now completely clean.")
        ;
    tagArrayEviction
        .name(name() + ".tagArrayEviction")
        .desc("Times we needed to evict an entry")
        ;
    superblockAlias
        .name(name() + ".superblockAlias")
        .desc("Times an super frame was already dirty but no superblock tag")
        ;
    superblockAliasErase
        .name(name() + ".superblockAliasErase")
        .desc("Times an entry in tagArray was erased for alias")
        ;

    zeroDirtyLinesSets
        .name(name() + ".zeroDirtyLinesSets")
        .desc("Sets which have no dirty lines.")
        ;

    tagsInUse
        .name(name() + ".tagsInUse")
        .desc("Number of tags in the tag array that are used")
        ;

    dirtyLinesPerSet
        .name(name() + ".dirtyLinesPerSet")
        .init(16)
        .desc("Number of blocks in the set that are dirty")
        ;
    Stats::registerDumpCallback(new StatsCallback(this));
}

SplitDirtyList*
SplitDirtyListParams::create()
{
    return new SplitDirtyList(this);
}
