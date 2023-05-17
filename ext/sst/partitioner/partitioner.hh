#ifndef __GEM5_NODES_PARTITIONER_HH__
#define __GEM5_NODES_PARTITIONER_HH__

#include "sst/core/eli/elementinfo.h"
#include "sst/core/sstpart.h"

class gem5NodesPartitioner;

class gem5NodesPartitioner: public SST::Partition::SSTPartitioner
{
  public:
    SST_ELI_REGISTER_PARTITIONER(
        gem5NodesPartitioner,
        "gem5NodesPartitioner",
        "gem5NodesPartitioner",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Partition the components based on the name suffixes."
    )

  protected:
    SST::RankInfo nRanks;
    SST::Output* partOutput;

  public:
    gem5NodesPartitioner(SST::RankInfo rankCount, SST::RankInfo myRank, int verbosity);
    ~gem5NodesPartitioner();
    void performPartition(SST::ConfigGraph* graph) override;
    bool requiresConfigGraph() override;
    bool spawnOnAllRanks() override;
};

#endif
