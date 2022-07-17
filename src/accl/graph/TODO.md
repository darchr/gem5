# TODO Items

* We might need to revisit the fact that we could insert something to a queue on
    the same cycle that another event is consuming something from the queue.
* Move checking for wl.degree == 0 to coalesce engine.
* Fix the retry system between memory queue and coalesce engine
* Update inheritance: There is not enough reason for PushEngine and
CoalesceEngine to be of the same type (i.e. delete BaseMemEngine).
