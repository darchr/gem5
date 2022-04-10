# TODO Items

* use setLE/setBE inside createUpdatePacket and createWritePacket
* parameterize cache size, associativity, maybe latencies,
and memory atom size in the coalesce engine
* look at all the simobjects and come up with a general architecture. Make
sure all the simobjects follow that architecture.
* implement all the communications between simobjects as req/retry.
* get rid of maps with RequestPtr as keys


Advice from Jason:
* use tryEnqueueMemReq that returns a boolean that shows if it has succeeded to enqueue the request.
* if it
* scratch all of these