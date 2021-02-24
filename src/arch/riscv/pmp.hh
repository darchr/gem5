#ifndef __ARCH_RISCV_PMP_HH__
#define __ARCH_RISCV_PMP_HH__

#include "arch/generic/tlb.hh"
#include "base/addr_range.hh"
#include "base/types.hh"
#include "mem/packet.hh"
#include "params/PMP.hh"
#include "sim/sim_object.hh"

class PMP : public SimObject
{
  public:

    typedef PMPParams Params;

    const Params &
    params() const
    {
        return dynamic_cast<const Params &>(_params);
    }

    int Max_Entries;

    PMP(const Params &params);

    // For encoding of address matching
    // mode of PMP address register
    // A (3-4) bits of pmpcfg register
    typedef enum {
        PMP_OFF,
        PMP_TOR,
        PMP_NA4,
        PMP_NAPOT
    } pmp_amatch;


    // PMP range permissions
    const uint8_t PMP_READ  =  1 << 0;
    const uint8_t PMP_WRITE = 1 << 1;
    const uint8_t PMP_EXEC  = 1 << 2;
    const uint8_t PMP_LOCK  = 1 << 7;

    // struct corresponding to a single
    // PMP entry
    typedef struct {
        AddrRange pmp_addr;
        Addr raw_addr;
        uint8_t  pmp_cfg;
    } pmp_entry;

    std::vector<pmp_entry*> pmp_table;

    bool pmp_check(const RequestPtr &req, BaseTLB::Mode mode);
    static inline uint8_t pmp_get_a_field(uint8_t cfg);
    void pmp_update_cfg(uint32_t pmp_index, uint8_t this_cfg);
    void pmp_update_addr(uint32_t pmp_index, Addr this_addr);
    AddrRange pmp_decode_napot(Addr a);

};

#endif // __ARCH_RISCV_PMP_HH__
