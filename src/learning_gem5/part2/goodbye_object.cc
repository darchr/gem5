#include "learning_gem5/part2/goodbye_object.hh"

#include "debug/Hello.hh"
#include "sim/sim_exit.hh"

GoodbyeObject::GoodbyeObject(GoodbyeObjectParams *params) :
    SimObject(params), event(*this), bandwidth(params->write_bandwidth),
    bufferSize(params->buffer_size), buffer(nullptr), bufferUsed(0)
{
    buffer = new char[bufferSize];
    DPRINTF(Hello, "Created the goodbye object\n");
}

GoodbyeObject::~GoodbyeObject()
{
    delete[] buffer;
}

void
GoodbyeObject::processEvent()
{
    DPRINTF(Hello, "Processing the event!\n");
    fillBuffer();
}
void
GoodbyeObject::sayGoodbye(std::string other_name)
{
    DPRINTF(Hello, "Saying goodbye to %s\n", other_name);

    message = "Goodbye " + other_name + "!! ";

    fillBuffer();
}

void
GoodbyeObject::fillBuffer()
{
    // There better be a message
    assert(message.length() > 0);

    // Copy form the message to the buffer per byte
    int bytes_copied = 0;
    for (auto it = message.begin();
         it < message.end() && bufferUsed < bufferSize -1;
         it ++, bufferUsed++, bytes_copied++) {
        // Copy the character into the buffer
        buffer[bufferUsed] = *it;
    }

    if (bufferUsed < bufferSize - 1) {
        // Wait for the next copy for as long as it would have taken
        DPRINTF(Hello, "Scheduling another fillBuffer in %d ticks\n",
                bandwidth * bytes_copied);
        schedule(event, curTick() + bandwidth * bytes_copied);
    } else {
        DPRINTF(Hello, "Goodbye done copying!\n");
        // Be sure to take into account the time for the last bytes
        exitSimLoop(buffer, 0, curTick() + bandwidth * bytes_copied);
    }
}

GoodbyeObject*
GoodbyeObjectParams::create()
{
    return new GoodbyeObject(this);
}
