#include "permissions/permission_cache.hh"

// For dubugging

#include "debug/PermissionCaching.hh"

namespace gem5 {
// all the methods should be defined here and here only.
PermissionCache::PermissionCache() {
    // haven't decided how to use this method
    fatal("The permission cache needs to know the size of the cache.");
}

PermissionCache::PermissionCache(size_t cache_size) {
    // the cache size must be a multiple of 64
    panic_if(cache_size % 64 != 0, "The cache must be standard cache-line"
                                    "aligned!");
    this->total_cache_size = cache_size;
    // This is standard
    this->permission_cache_line_size = 64;

    this->number_of_entries = total_cache_size / permission_cache_line_size;

}

PermissionCache::PermissionCache(size_t cache_size,
                                    std::string policy,
                                    size_t cache_line) {
    // the cache size must be a multiple of 64
    panic_if(cache_size % 64 != 0, "The cache must be standard cache-line"
                                    "aligned!");
    this->total_cache_size = cache_size;
    this->cache_policy = policy;
    // This is standard
    this->permission_cache_line_size = 64;
    this->number_of_entries = total_cache_size / permission_cache_line_size;
}

PermissionCache::PermissionCache(size_t cache_size,
                                    std::string policy,
                                    size_t cache_line) {
    // the cache size must be a multiple of 64
    panic_if(cache_size % 64 != 0, "The cache must be standard cache-line"
                                    "aligned!");
    this->total_cache_size = cache_size;
    this->cache_policy = policy;
    // This is standard.
    assert(cache_line == 64);
    this->permission_cache_line_size = cache_line;
    this->number_of_entries = total_cache_size / permission_cache_line_size;
}


// make sure to implement the caching methods here to quickly copy paste them,
// if needed.
PermissionCache::permission_handler
PermissionCache::isCachedRequest(gem5::Addr addr) {
    /*
    Simple caching function that determines the caching variable from the
    class contructor and then makes sure to return where the given address
    has the values in the cache.

    @params
    addr: address to check inside the cache

    :returns:
        A struct with cache hit status and the latency value.
    */

    // TODO: A cached request needs to fetch 64 bytes of data. Each entry in
    // the permission table is 2 bytes. So an aligned entry should have 32
    // cached entries!

    if (cache_policy == "lru") {
        return simpleLRU(addr);
    }
    else if (cache_policy == "mru") {
        return simpleMRU(addr);
    }
    else if (cache_policy == "random") {
        return simpleRandom(addr);
    }
    else {
        // unknown caching policy
        panic("Unknown caching policy!");
        // uncrachable code.
    }
}

PermissionCache::permission_handler
PermissionCache::simpleLRU(gem5::Addr addr) {
    // ideally see if there is an entry (MMP). The address needs to ignore the
    // last 8 bits as each entry can have.
    gem5::Addr addr_key = addr | 0xFFFFFFFF; // cache_mask;
    auto lookup = permission_cache.find(addr_key);

    // create a return structure
    struct permission_handler return_struct;
    return_struct.is_cached = false;

    // TODO:
    // Permissions are per segment. So a binary search needs to be made to
    // figure out the exact delay of MMP. The simplest implementation is when
    // the segment size is the same as the page size (i.e. 4KiB)

    // There can be a variable latency added for this lookup in the cache of
    // MMP. the lateny of a hit is actually a variable latency. Since this
    // is a binary lookup, the latency is log2 N where N is the number
    // of entries. Make sure that the latency is never 0.
    Tick latency = this->number_of_entries > 0 ?
                                    std::log2(this->number_of_entries) : 1;

    if (lookup != permission_cache.end()) {
        // found the entry in the permission table. see if this is cached.
        DPRINTF(PermissionCaching, "PLB hit for addr %#x\n", addr);

        if (lookup->second->is_cached == true) {
            // Hit latency must be very small!
            latency = hit_latency;
            // since this is LRU, increment the count by 1
            lookup->second->access_count++;
            // need to be smarter to return this to the caller.
            // ++stats.numPermissionTableCacheHits;
            return_struct.is_cached = true;
        }
        else {
            DPRINTF(PermissionCaching, "PLB miss for addr %#x\n", addr);
            // this entry is not cached.
            latency = miss_latency;
            // find a suitable location to cache this entry.
            // see what is the cache occupancy until now.
            if (number_of_occupied_entries < number_of_entries) {
                // there is space in the cache. just create a new entry
                lookup->second->is_cached = true;
                // lru ignores last access.
                lookup->second->access_count = 1;
                number_of_occupied_entries++;
            }
            else {
                // need to replace something :(
                int min_count = INT_MAX;
                gem5::Addr key;
                for (auto it = permission_cache.begin();
                        it !=  permission_cache.end(); it++) {
                    if (min_count < it->second->access_count &&
                                            it->second->is_cached == true) {
                        min_count = it->second->access_count;
                        key = it->first;
                    }
                }
                // delete the min_count entry!
                permission_cache[key]->is_cached = false;
                permission_cache[key]->access_count = 0;

                // make sure to update the current lookup
                lookup->second->is_cached = true;
                lookup->second->access_count = 1;

            }
        }
    }
    else {
        // the entry doesnt exist in the permission table. The mmp needs to
        // create this first and no matter what it does, this will be a
        // cache miss.
        // ++stats.numPermissionTableEntries;

        DPRINTF(PermissionCaching, "Creating permission entry for addr %#x\n",
                                                                        addr);
        latency = creation_latency + miss_latency;

        // create an entry first
        struct cache_entry_vector *cve = new struct cache_entry_vector;
        cve->is_cached = false;
        cve->access_count = 1;

        // insert this entry to the table.
        permission_cache.insert({addr_key, cve});

        // see if there is space in the cache for us to cache it.
        if (number_of_occupied_entries < number_of_entries) {
            permission_cache[addr_key]->is_cached = true;
            number_of_occupied_entries++;
        }
        else {
            // this entry must be cached and there is no more space in the
            // cache.
            int min_count = INT_MAX;
            gem5::Addr key;
            for (auto it = permission_cache.begin();
                            it !=  permission_cache.end(); it++) {
                if (min_count < it->second->access_count &&
                                            it->second->is_cached == true) {
                    min_count = it->second->access_count;
                    key = it->first;
                }
            }
            // delete the min_count entry!
            permission_cache[key]->is_cached = false;
            permission_cache[key]->access_count = 0;
            permission_cache[addr_key]->is_cached = true;
            permission_cache[addr_key]->access_count = 1;
        }
    }

    // check if this address is in the cache
    return_struct.latency = latency;
    return return_struct;

}

PermissionCache::permission_handler
PermissionCache::simpleMRU(gem5::Addr addr) {
    // ideally see if there is an entry (MMP)
    auto lookup = permission_cache.find(addr);
    // create a return structure
    struct permission_handler return_struct;
    return_struct.is_cached = false;

    // There can be a variable latency added for this lookup in the cache of
    // MMP.
    Tick latency = 0;

    if (lookup != permission_cache.end()) {
        // found the entry in the permission table. see if this is cached.
        if (lookup->second->is_cached == true) {
            latency = hit_latency;
            lookup->second->last_accessed = gem5::curTick();
            return_struct.is_cached = true;
        }
        else {
            // this entry is not cached.
            latency = miss_latency;
            // find a suitable location to cache this entry.
            // see what is the cache occupancy until now.
            if (number_of_occupied_entries < number_of_entries) {
                // there is space in the cache. just create a new entry
                lookup->second->is_cached = true;
                // lru ignores last access.
                lookup->second->last_accessed = gem5::curTick();
                number_of_occupied_entries++;
            }
            else {
                // need to replace something :(
                gem5::Tick max_count = 0;
                gem5::Addr key;
                for (auto it = permission_cache.begin();
                        it !=  permission_cache.end(); it++) {
                    if (max_count > it->second->last_accessed &&
                                            it->second->is_cached == true) {
                        max_count = it->second->last_accessed;
                        key = it->first;
                    }
                }
                // delete the min_count entry!
                permission_cache[key]->is_cached = false;

                // make sure to update the current lookup
                lookup->second->is_cached = true;
                lookup->second->last_accessed = gem5::curTick();
            }
        }
    }
    else {
        // the entry doesnt exist in the permission table. The mmp needs to
        // create this first and no matter what it does, this will be a
        // cache miss.
        // ++stats.numPermissionTableEntries;
        latency = creation_latency + miss_latency;

        // create an entry first
        struct cache_entry_vector *cve = new struct cache_entry_vector;
        cve->is_cached = false;
        cve->last_accessed = gem5::curTick();

        // insert this entry to the table.
        permission_cache.insert({addr, cve});

        // see if there is space in the cache for us to cache it.
        if (number_of_occupied_entries < number_of_entries) {
            permission_cache[addr]->is_cached = true;
            number_of_occupied_entries++;
        }
        else {
            // this entry must be cached and there is no more space in the
            // cache.
            gem5::Tick max_count = 0;
            gem5::Addr key;
            for (auto it = permission_cache.begin();
                            it !=  permission_cache.end(); it++) {
                if (max_count > it->second->last_accessed &&
                                            it->second->is_cached == true) {
                    max_count = it->second->last_accessed;
                    key = it->first;
                }
            }
            // delete the min_count entry!
            permission_cache[key]->is_cached = false;
            permission_cache[addr]->is_cached = true;
            permission_cache[addr]->last_accessed = gem5::curTick();
        }
    }

    // check if this address is in the cache
    return_struct.latency = latency;
    return return_struct;
}

PermissionCache::permission_handler
PermissionCache::simpleRandom(gem5::Addr addr) {
    // ideally see if  is an entry (MMP)
    auto lookup = permission_cache.find(addr);
    // create a return structure
    struct permission_handler return_struct;
    return_struct.is_cached = false;

    // There can be a variable latency added for this lookup in the cache of
    // MMP.
    Tick latency = 0;

    if (lookup != permission_cache.end()) {
        // found the entry in the permission table. see if this is cached.
        if (lookup->second->is_cached == true) {
            latency = hit_latency;
            return_struct.is_cached = true;
        }
        else {
            // this entry is not cached.
            latency = miss_latency;
            // find a suitable location to cache this entry.
            // see what is the cache occupancy until now.
            if (number_of_occupied_entries < number_of_entries) {
                // there is space in the cache. just create a new entry
                lookup->second->is_cached = true;
                number_of_occupied_entries++;
            }
            else {
                // need to replace something :(
                srand(time(NULL));
                // randomly choose an index to replace
                int index = (rand() % (permission_cache.size() + 1));

                // FIXME:
                // need to travel to that index.
                int i = 0;
                for (auto it = permission_cache.begin();
                        it !=  permission_cache.end(); it++, i++) {
                    if (i == index) {
                        it->second->is_cached = false;
                        lookup->second->is_cached = true;
                        break;
                    }
                }
            }
        }
    }
    else {
        // the entry doesnt exist in the permission table. The mmp needs to
        // create this first and no matter what it does, this will be a
        // cache miss.
        // ++stats.numPermissionTableEntries;
        latency = creation_latency + miss_latency;

        // create an entry first
        struct cache_entry_vector *cve = new struct cache_entry_vector;
        cve->is_cached = false;
        cve->last_accessed = gem5::curTick();

        // insert this entry to the table.
        permission_cache.insert({addr, cve});

        // see if there is space in the cache for us to cache it.
        if (number_of_occupied_entries < number_of_entries) {
            permission_cache[addr]->is_cached = true;
            number_of_occupied_entries++;
        }
        else {
            // need to replace something :(
            srand(time(NULL));
            // randomly choose an index to replace
            int index = (rand() % (permission_cache.size() + 1));

            // FIXME:
            // need to travel to that index.
            int i = 0;
            for (auto it = permission_cache.begin();
                    it !=  permission_cache.end(); it++, i++) {
                if (i == index) {
                    it->second->is_cached = false;
                    lookup->second->is_cached = true;
                    break;
                }
            }
        }
    }

    // check if this address is in the cache
    return_struct.latency = latency;
    return return_struct;
}

}   // end namespace