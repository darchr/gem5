# TODO Items

* implement all the communications between simobjects as req/retry.
* get rid of maps with RequestPtr as keys
* add UpdateWL as a MemCmd
* Replace std::floor with roundDown from intmath.hh in src
* We might need to revisit the fact that we could insert something to a queue on
    the same cycle that another event is consuming something from the queue.
