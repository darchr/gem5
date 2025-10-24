// This is the file needed for enabling caches for the permissions
#ifndef __PERMISSIONS_PERMISSION_CACHE_HH__
#define __PERMISSIONS_PERMISSION_CACHE_HH__

#include <cmath>
#include <queue>
#include <string>
#include <map>

#include "sim/eventq.hh"

namespace gem5 {
// make sure that the typedef is set correctly.
typedef struct permission_handler phandler_t;
typedef struct cache_entry_vector entry_t;

// This class will be inherited by the simobjects
class PermissionCache {
    private:
        // For the permission checks. These methods need to return two things:
        // was the cache successful, how long did the entire lookup took
        struct permission_handler {
            // If this was a cache hit, then we do not queue the missLatency.
            // We replace missLatency with an actual memory request, which will
            // be ideal.
            bool is_cached;
            // The standard how long did it take to do the lookup.
            Tick latency;

        };
        
        // Each entry in the MMP permission will have these values. There are
        // implementational details.
        struct cache_entry_vector {
            // first we need to figure out the domain of this entry. Who sets
            // up the domain entries? The operating system but not even the
            // authors implemented this in the evaluation. This is done in the
            // followup paper.
            // XXX: Keeping the domain_id as a field for future usage.
            int domain_id;
            // There needs to be a monotonic ID incrementor that gives the
            // location of this address' permission. Ideally this shouldn't be
            // monotonic as the OS will periodically clear permissions but in
            // our research we only see results for a single program.
            // TODO: This will be left unimplemented!
            uint64_t id;
            // is_cached will be true if any packet within 64 Bytes is true.
            bool is_cached;
            // We need to maintain the size as a variable. This is the segment
            // size. The lookup will be longer but it is critical to implement
            // this.
            size_t size;
            // These are needed for LRU and MRU policies.
            Tick last_accessed;
            int access_count;
        };

    protected:
        // A structure that maintains the structure of the cache.
        std::unordered_map<gem5::Addr, cache_entry_vector*> permission_cache;

        struct permission_handler isCachedRequest(gem5::Addr addr);
        struct permission_handler simpleLRU(gem5::Addr addr);
        struct permission_handler simpleMRU(gem5::Addr addr);
        struct permission_handler simpleRandom(gem5::Addr addr);

    public:
        PermissionCache();
        // setup the cache
        PermissionCache(size_t cache_size);
        PermissionCache(size_t cache_size, std::string policy);
        PermissionCache(
                    size_t cache_size, std::string policy, size_t cache_line);

        // these variables need to be set by the forward class
        size_t total_cache_size;
        size_t permission_cache_line_size;

        gem5::Tick hit_latency;
        gem5::Tick miss_latency;
        gem5::Tick creation_latency;
        // there needs to be total number of entries and then the total number
        // of occupied entries.
        unsigned int number_of_entries;
        // the cache is direct-mapped.
        unsigned int number_of_occupied_entries;

        std::string cache_policy;

        uint64_t cache_mask;
};

}   // namespace gem5

#endif  // _if_def