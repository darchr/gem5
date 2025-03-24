#include "network/Layer.hh"

#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

namespace gem5
{
    Layer::Layer(const LayerParams& params) :
        ClockedObject(params),
        rangeSize(params.range_size)
    {
    }

    uint64_t
    Layer::getRangeSize() const
    {
        return rangeSize;
    }
} // namespace gem5
