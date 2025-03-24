#ifndef __NETWORK_LAYER_HH__
#define __NETWORK_LAYER_HH__

#include "params/Layer.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class Layer : public ClockedObject
{
  private:
    uint64_t rangeSize;

  public:
    Layer(const LayerParams& params);

    uint64_t getRangeSize() const;
};

} // namespace gem5

#endif // __NETWORK_LAYER_HH__
