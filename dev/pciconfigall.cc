/* $Id$ */

/* @file
 * PCI Configspace implementation
 */

#include <deque>
#include <string>
#include <vector>

#include "base/trace.hh"
#include "cpu/exec_context.hh"
#include "dev/scsi_ctrl.hh"
#include "dev/pciconfigall.hh"
#include "dev/pcidev.hh"
#include "dev/tsunamireg.h"
#include "dev/tsunami.hh"
#include "mem/functional_mem/memory_control.hh"
#include "sim/builder.hh"
#include "sim/system.hh"

using namespace std;

PCIConfigAll::PCIConfigAll(const string &name, Tsunami *t,
                           Addr addr, Addr mask, MemoryController *mmu)
    : MmapDevice(name, addr, mask, mmu), tsunami(t)
{
    //Put back pointer in tsunami
    tsunami->pciconfig = this;

    // Make all the pointers to devices null
    for(int x=0; x < MAX_PCI_DEV; x++)
        for(int y=0; y < MAX_PCI_FUNC; y++)
          devices[x][y] = NULL;
}

Fault
PCIConfigAll::read(MemReqPtr &req, uint8_t *data)
{
    DPRINTF(PCIConfigAll, "read  va=%#x size=%d\n",
            req->vaddr, req->size);

    Addr daddr = (req->paddr & addr_mask);

    int device = (daddr >> 11) & 0x1F;
    int func = (daddr >> 8) & 0x7;
    int reg = daddr & 0xFF;

    if (devices[device][func] == NULL) {
        switch (req->size) {
           // case sizeof(uint64_t):
           //     *(uint64_t*)data = 0xFFFFFFFFFFFFFFFF;
           //     return No_Fault;
            case sizeof(uint32_t):
                *(uint32_t*)data = 0xFFFFFFFF;
                return No_Fault;
            case sizeof(uint16_t):
                *(uint16_t*)data = 0xFFFF;
                return No_Fault;
            case sizeof(uint8_t):
                *(uint8_t*)data = 0xFF;
                return No_Fault;
            default:
                panic("invalid access size(?) for PCI configspace!\n");
        }
    } else {
        switch (req->size) {
            case sizeof(uint32_t):
            case sizeof(uint16_t):
            case sizeof(uint8_t):
                devices[device][func]->ReadConfig(reg, req->size, data);
                return No_Fault;
            default:
                panic("invalid access size(?) for PCI configspace!\n");
        }
    }

    DPRINTFN("Tsunami PCI Configspace  ERROR: read  daddr=%#x size=%d\n", daddr, req->size);

    return No_Fault;
}

Fault
PCIConfigAll::write(MemReqPtr &req, const uint8_t *data)
{

    Addr daddr = (req->paddr & addr_mask);

    int device = (daddr >> 11) & 0x1F;
    int func = (daddr >> 8) & 0x7;
    int reg = daddr & 0xFF;

    union {
        uint8_t byte_value;
        uint16_t half_value;
        uint32_t word_value;
    };


    if (devices[device][func] == NULL)
        panic("Attempting to write to config space on non-existant device\n");
    else {
            switch (req->size) {
            case sizeof(uint8_t):
                byte_value = *(uint8_t*)data;
                break;
            case sizeof(uint16_t):
                half_value = *(uint16_t*)data;
                break;
            case sizeof(uint32_t):
                word_value = *(uint32_t*)data;
                break;
            default:
                panic("invalid access size(?) for PCI configspace!\n");
            }
    }
    DPRINTF(PCIConfigAll, "write - va=%#x size=%d data=%#x\n",
            req->vaddr, req->size, word_value);
    devices[device][func]->WriteConfig(reg, req->size, word_value);


    return No_Fault;
}


void
PCIConfigAll::serialize(std::ostream &os)
{
    // code should be written
}

void
PCIConfigAll::unserialize(Checkpoint *cp, const std::string &section)
{
    //code should be written
}

BEGIN_DECLARE_SIM_OBJECT_PARAMS(PCIConfigAll)

    SimObjectParam<Tsunami *> tsunami;
    SimObjectParam<MemoryController *> mmu;
    Param<Addr> addr;
    Param<Addr> mask;

END_DECLARE_SIM_OBJECT_PARAMS(PCIConfigAll)

BEGIN_INIT_SIM_OBJECT_PARAMS(PCIConfigAll)

    INIT_PARAM(tsunami, "Tsunami"),
    INIT_PARAM(mmu, "Memory Controller"),
    INIT_PARAM(addr, "Device Address"),
    INIT_PARAM(mask, "Address Mask")

END_INIT_SIM_OBJECT_PARAMS(PCIConfigAll)

CREATE_SIM_OBJECT(PCIConfigAll)
{
    return new PCIConfigAll(getInstanceName(), tsunami, addr, mask, mmu);
}

REGISTER_SIM_OBJECT("PCIConfigAll", PCIConfigAll)
