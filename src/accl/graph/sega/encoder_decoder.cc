#include "accl/graph/sega/encoder_decoder.hh"

#include "debug/EncoderDecoder.hh"

namespace gem5
{
    static unsigned
    defaultCyclesFor(unsigned bitw, bool is_encoder, bool dual_output)
    {
        // Defaults derived from your 5 GHz timing:
        //  - 3-bit encoder: 2 cycles
        //  - 3-bit decoder: 3 cycles (worst-case of 2–3)
        //  - 6-bit dual-output: 6 cycles (encoder/decoder)
        //  - 6-bit single-output: 96 cycles (encoder/decoder)
        fatal_if(bitw != 3 && bitw != 6,
                "EncoderDecoder: bit_width must be 3 or 6, got %u", bitw);

        if (bitw == 3)
            return is_encoder ? 2u : 3u;

        // bitw == 6
        return dual_output ? 6u : 96u;
    }

    EncoderDecoder::EncoderDecoder(const EncoderDecoderParams &p)
        : ClockedObject(p),
        _bitWidth(p.bit_width),
        _isEncoder(p.is_encoder),
        _dualOutput(p.dual_output),
        _cyclesPerOp(p.cycles_per_op ? p.cycles_per_op
                                        : defaultCyclesFor(p.bit_width,
                                                        p.is_encoder,
                                                        p.dual_output)),
        processEvent([this]{}, name() + ".processEvent")
    {
        fatal_if(_bitWidth != 3 && _bitWidth != 6,
                "%s: bit_width must be 3 or 6 (got %u)", name(), _bitWidth);
        // If user supplied cycles_per_op, keep it;
        // otherwise we computed a default.
    }

    Tick
    EncoderDecoder::getDelay() const
    {
        Tick delay = clockPeriod() * _cyclesPerOp;
        DPRINTF(EncoderDecoder, "%s: getDelay() = clockPeriod(%lu)"
                "* cyclesPerOp(%u) = %lu ticks\n",
                name(), clockPeriod(), _cyclesPerOp, delay);
        return delay;
    }

    void
    EncoderDecoder::scheduleOperation()
    {
        DPRINTF(EncoderDecoder, "%s: Scheduling operation for tick %lu\n",
                name(), curTick() + getDelay());

        if (!processEvent.scheduled()) {
            schedule(processEvent, curTick() + getDelay());
        }
    }
}
