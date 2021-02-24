#include "arch/riscv/pmp.hh"

#include "arch/generic/tlb.hh"
#include "base/addr_range.hh"
#include "base/types.hh"
#include "math.h"
#include "mem/request.hh"
#include "params/PMP.hh"
#include "sim/sim_object.hh"

PMP::PMP(const Params &params) :
SimObject(params), Max_Entries(params.max_pmp)
{

 for (int i=0; i<Max_Entries; i++){

    pmp_entry* entry = new pmp_entry;
    pmp_table.push_back(entry);
 }

}

// check if this region can be
// accessed (else raise an exception)
bool
PMP::pmp_check(const RequestPtr &req, BaseTLB::Mode mode)
{

// all pmp entries need to be looked from the lowest to
// the highest number

    bool addr_match = false;

    int i;
    for (i = 0; i < pmp_table.size(); i++) {
        if (pmp_table[i]->pmp_addr.contains(req->getPaddr()))
           //address matched
            addr_match = true;

    }

    if (!addr_match)
        return false; //no pmp entry has a matching address


    // else check the RWX permissions from the pmp entry

    uint8_t allowed_privs = PMP_READ | PMP_WRITE | PMP_EXEC;

    // i is the index of pmp table which matched
    allowed_privs &= pmp_table[i]->pmp_cfg;


    // How to determine if the request is for an inst cache
    if ((mode == BaseTLB::Mode::Read) &&
            ((PMP_READ & allowed_privs) == PMP_READ))
        return true;
    else if ((mode == BaseTLB::Mode::Write) &&
            ((PMP_WRITE & allowed_privs) == PMP_WRITE))
        return true;
    else if ((mode == BaseTLB::Mode::Execute) &&
            ((PMP_WRITE & allowed_privs) == PMP_EXEC))
        return true;
    else
        return false;
}

// to get a field from pmpcfg register
inline uint8_t
PMP::pmp_get_a_field(uint8_t cfg)
{
    uint8_t a = cfg >> 3;
    return a & 0x3;
}


void
PMP::pmp_update_cfg(uint32_t pmp_index, uint8_t this_cfg)
{
    pmp_table[pmp_index]->pmp_cfg = this_cfg;

    Addr this_addr = pmp_table[pmp_index]->raw_addr;

    Addr prev_addr = 0;

    if (pmp_index >= 1) {

        prev_addr = pmp_table[pmp_index - 1]->pmp_addr.end();
    }

    switch (pmp_get_a_field(this_cfg)) {
    case PMP_OFF:
        {
        AddrRange this_range(0, -1);
        pmp_table[pmp_index]->pmp_addr = this_range;
        break;
        }
    case PMP_TOR:
        {
        AddrRange this_range(prev_addr << 2, (this_addr << 2) - 1);
        pmp_table[pmp_index]->pmp_addr = this_range;
        break;
        }
    case PMP_NA4:
        {
        AddrRange this_range(this_addr << 2, (this_addr + 4) - 1);
        pmp_table[pmp_index]->pmp_addr = this_range;
        break;
        }
    case PMP_NAPOT:
        {
        AddrRange this_range = pmp_decode_napot(this_addr);
        pmp_table[pmp_index]->pmp_addr = this_range;
        break;
        }
    default:
        {
        AddrRange this_range(0,0);
        pmp_table[pmp_index]->pmp_addr = this_range;
        break;
        }
    }


}

void
PMP::pmp_update_addr(uint32_t pmp_index, Addr this_addr)
{
    // just writing the raw addr in the pmp table
    // will convert it into a range, once cfg
    // reg is written

    pmp_table[pmp_index]->raw_addr = this_addr;

}


AddrRange
PMP::pmp_decode_napot(Addr a)
{
    if (a == -1) {
      AddrRange this_range(0u, -1);
      return this_range;
    } else {
        uint64_t t1 = ctz64(~a);
        uint64_t range = std::pow(2,t1+3);
        uint64_t base = bits(a, 63, t1+1);
        AddrRange this_range(base, base+range);
        return this_range;
    }

}
