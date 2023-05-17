#include "partitioner.hh"

#include "../util.hh"

#include <string>
#include <vector>

#include "sst/core/configGraph.h"

gem5NodesPartitioner::gem5NodesPartitioner(SST::RankInfo nRanks, SST::RankInfo myRank, int verbosity)
{
    this->nRanks = nRanks;
    this->partOutput = new SST::Output("gem5NodesPartition ", verbosity, 0, SST::Output::STDOUT);
}

gem5NodesPartitioner::~gem5NodesPartitioner()
{
    delete this->partOutput;
}

void
gem5NodesPartitioner::performPartition(SST::ConfigGraph* graph)
{
    this->partOutput->verbose(CALL_INFO, 1, 0, "Performing PARTITIONING ***************************************\n");
    SST::ConfigComponentMap_t& componentsMap = graph->getComponentMap();
    for (auto component: componentsMap)
    {
        std::vector<std::string> tokens = tokenizeString(component->name, { '_' });
        this->partOutput->verbose(CALL_INFO, 1, 0, "%s\n", component->name.c_str());
        for (auto token: tokens)
        {
            this->partOutput->verbose(CALL_INFO, 1, 0, "   %s\n", token.c_str());
        }
        component->rank = SST::RankInfo(std::stoi(tokens.back()), 0);
    }
}

bool
gem5NodesPartitioner::requiresConfigGraph()
{
    return true;
}

bool gem5NodesPartitioner::spawnOnAllRanks()
{
    return false;
}
