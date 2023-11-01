#include <string>

struct __attribute__ ((packed)) Edge
{
    uint16_t weight : 16;
    uint64_t neighbor : 48;

    Edge(uint64_t weight, uint64_t neighbor):
        weight(weight),
        neighbor(neighbor)
    {}

    std::string to_string() {
      std::string ret = "";
      ret += "Edge{weight: " + std::to_string(weight) + ", neighbord: " +
              std::to_string(neighbor) + "}";
      return ret;
    }
};

struct __attribute__ ((packed)) Vertex
{
    uint16_t dist : 16;
    uint64_t id : 48;
    uint64_t prev_edge : 48;

    Vertex(uint64_t dist, uint64_t id):
        dist(dist),
        id(id)
    {} 

    std::string to_string() {
      std::string ret = "";
      ret += "Vertex{dist: " + std::to_string(dist) + ", id: " +
              std::to_string(id) + "}";
      return ret;
    }
};

struct __attribute__ ((packed)) Update
{
    // uint16_t weight : 16;
    // uint64_t dst_id : 48;
    //uint64_t src_id : 48;

    uint16_t weight : 16;
    uint64_t dst_id : 48;

    // std::string to_string()
    // {
    //     //return csprintf("Update{weight: %u, dst_id: %lu}", weight, dst_id);
    // }

    Update(): weight(0), dst_id(0) {}

    Update(uint16_t weight, uint64_t dst_id):
        weight(weight),
        dst_id(dst_id)
    {}
};