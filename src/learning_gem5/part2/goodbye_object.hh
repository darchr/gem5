#ifndef __LEARNING_GEM5_GOODBYE_OBJECT_HH__
#define __LEARNING_GEM5_GOODBYE_OBJECT_HH__

#include <string>

#include "params/GoodbyeObject.hh"
#include "sim/sim_object.hh"

class GoodbyeObject:public SimObject
{
    private:
        void processEvent();

        /**
         * Fills the buffer for one iteration. If the buffer isn't full, this
         * function will enqueue antother event to continue filling
         */
        void fillBuffer();

        EventWrapper<GoodbyeObject, &GoodbyeObject::processEvent> event;

        // the bytes processed per tick
        float bandwidth;
        // the size of the buffer we are going to fill
        int bufferSize;
        // the buffer we are putting our message in
        char *buffer;
        // the message to put into the buffer.
        std::string message;
        // the amount of the buffer we've used so far
        int bufferUsed;

    public:
        GoodbyeObject(GoodbyeObjectParams *p);
        ~GoodbyeObject();
        /**
         * called by an ourside object. Starts off the ecints to
         * fill the buffer with a goodbye message
         * @param name the name of the ofject we are saying
         * goodbye to
         */
        void sayGoodbye(std::string name);

};

#endif // __LEARNING_GEM5_GOODBYE_OBJECT_HH__
