#ifndef __ENCODER_DECODER_HH__
#define __ENCODER_DECODER_HH__

#include "base/types.hh"
#include "params/EncoderDecoder.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

namespace gem5
{

class EncoderDecoder : public ClockedObject
{
  public:
    EncoderDecoder(const EncoderDecoderParams &p);
    // Delay for one conversion op (in Tick)
    Tick getDelay() const;

    // Schedule an operation to complete after delay
    void scheduleOperation();

    // Introspection
    unsigned bitWidth()   const { return _bitWidth; }
    bool     isEncoder()  const { return _isEncoder; }
    bool     dualOutput() const { return _dualOutput; }
    unsigned cyclesPerOp() const { return _cyclesPerOp; }

  private:
    unsigned _bitWidth;     // 3 or 6
    bool     _isEncoder;    // true = encoder, false = decoder
    bool     _dualOutput;   // relevant when bitWidth == 6
    unsigned _cyclesPerOp;  // derived default or user override

    // Event for processing operations
    EventFunctionWrapper processEvent;
};

} // namespace gem5

#endif // __ENCODER_DECODER_HH__
