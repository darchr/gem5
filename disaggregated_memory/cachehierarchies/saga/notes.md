# My notes from digging CHI source code.

* What is sequencer line write (RequestType::StoreLine)?
* I think *alloc_on_seq_acc* and  *alloc_on_seq_line_write* are specific to L1 caches.
They basically refer to sequencer accesses.
I do not understand why they would ever be set to false.
