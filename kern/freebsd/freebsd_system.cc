/*
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Modifications for the FreeBSD kernel.  Based off of kern/linux/linux_system.cc.
 * Currently only used to skip DELAY function.
 *
 */

#include "base/loader/symtab.hh"
#include "base/trace.hh"
#include "cpu/exec_context.hh"
#include "cpu/base.hh"
#include "kern/freebsd/freebsd_system.hh"
#include "kern/system_events.hh"
#include "mem/functional/memory_control.hh"
#include "mem/functional/physical.hh"
#include "sim/builder.hh"
#include "dev/platform.hh"
#include "targetarch/isa_traits.hh"
#include "targetarch/vtophys.hh"
#include "sim/debug.hh"

using namespace std;

FreebsdSystem::FreebsdSystem(Params *p)
    : System(p)
{
    Addr addr = 0;

    /**
     * Any time ide_delay_50ms, calibarte_delay or
     * determine_cpu_caches is called just skip the
     * function. Currently determine_cpu_caches only is used put
     * information in proc, however if that changes in the future we
     * will have to fill in the cache size variables appropriately.
     */
    skipDelayEvent = new SkipFuncEvent(&pcEventQueue, "DELAY");
    if (kernelSymtab->findAddress("DELAY", addr))
        skipDelayEvent->schedule(addr+sizeof(MachInst));

    skipOROMEvent = new SkipFuncEvent(&pcEventQueue, "orm_identify");
    if (kernelSymtab->findAddress("orm_identify", addr))
        skipOROMEvent->schedule(addr+sizeof(MachInst));

    skipAicEvent = new SkipFuncEvent(&pcEventQueue, "ahc_isa_identify");
    if (kernelSymtab->findAddress("ahc_isa_identify", addr))
        skipAicEvent->schedule(addr+sizeof(MachInst));

    skipPNPEvent = new SkipFuncEvent(&pcEventQueue, "pnp_identify");
    if (kernelSymtab->findAddress("pnp_identify", addr))
        skipPNPEvent->schedule(addr+sizeof(MachInst));

    skipATAEvent = new SkipFuncEvent(&pcEventQueue, "ata_attach");
    if (kernelSymtab->findAddress("ata_attach", addr))
        skipATAEvent->schedule(addr+sizeof(MachInst));

    skipCalibrateClocks = new FreebsdSkipCalibrateClocksEvent(&pcEventQueue, "calibrate_clocks");
    if (kernelSymtab->findAddress("calibrate_clocks", addr))
        skipCalibrateClocks->schedule(addr + sizeof(MachInst) * 2);

}


FreebsdSystem::~FreebsdSystem()
{
    delete skipDelayEvent;
    delete skipOROMEvent;
    delete skipAicEvent;
    delete skipATAEvent;
    delete skipPNPEvent;
    delete skipCalibrateClocks;
}


void
FreebsdSystem::doCalibrateClocks(ExecContext *xc)
{
    Addr ppc_vaddr = 0;
    Addr timer_vaddr = 0;
    Addr ppc_paddr = 0;
    Addr timer_paddr = 0;

    ppc_vaddr = (Addr)xc->regs.intRegFile[ArgumentReg1];
    timer_vaddr = (Addr)xc->regs.intRegFile[ArgumentReg2];

    ppc_paddr = vtophys(physmem, ppc_vaddr);
    timer_paddr = vtophys(physmem, timer_vaddr);

    uint8_t *ppc = physmem->dma_addr(ppc_paddr, sizeof(uint32_t));
    uint8_t *timer = physmem->dma_addr(timer_paddr, sizeof(uint32_t));

    *(uint32_t *)ppc = 2000000000;
    *(uint32_t *)timer = 1193180;
}


BEGIN_DECLARE_SIM_OBJECT_PARAMS(FreebsdSystem)

    Param<Tick> boot_cpu_frequency;
    SimObjectParam<MemoryController *> memctrl;
    SimObjectParam<PhysicalMemory *> physmem;

    Param<string> kernel;
    Param<string> console;
    Param<string> pal;

    Param<string> boot_osflags;
    Param<string> readfile;
    Param<unsigned int> init_param;

    Param<uint64_t> system_type;
    Param<uint64_t> system_rev;

    Param<bool> bin;
    VectorParam<string> binned_fns;
    Param<bool> bin_int;

END_DECLARE_SIM_OBJECT_PARAMS(FreebsdSystem)

BEGIN_INIT_SIM_OBJECT_PARAMS(FreebsdSystem)

    INIT_PARAM(boot_cpu_frequency, "Frequency of the boot CPU"),
    INIT_PARAM(memctrl, "memory controller"),
    INIT_PARAM(physmem, "phsyical memory"),
    INIT_PARAM(kernel, "file that contains the kernel code"),
    INIT_PARAM(console, "file that contains the console code"),
    INIT_PARAM(pal, "file that contains palcode"),
    INIT_PARAM_DFLT(boot_osflags, "flags to pass to the kernel during boot",
                    "a"),
    INIT_PARAM_DFLT(readfile, "file to read startup script from", ""),
    INIT_PARAM_DFLT(init_param, "numerical value to pass into simulator", 0),
    INIT_PARAM_DFLT(system_type, "Type of system we are emulating", 34),
    INIT_PARAM_DFLT(system_rev, "Revision of system we are emulating", 1<<10),
    INIT_PARAM_DFLT(bin, "is this system to be binned", false),
    INIT_PARAM(binned_fns, "functions to be broken down and binned"),
    INIT_PARAM_DFLT(bin_int, "is interrupt code binned seperately?", true)

END_INIT_SIM_OBJECT_PARAMS(FreebsdSystem)

CREATE_SIM_OBJECT(FreebsdSystem)
{
    System::Params *p = new System::Params;
    p->name = getInstanceName();
    p->boot_cpu_frequency = boot_cpu_frequency;
    p->memctrl = memctrl;
    p->physmem = physmem;
    p->kernel_path = kernel;
    p->console_path = console;
    p->palcode = pal;
    p->boot_osflags = boot_osflags;
    p->init_param = init_param;
    p->readfile = readfile;
    p->system_type = system_type;
    p->system_rev = system_rev;
    p->bin = bin;
    p->binned_fns = binned_fns;
    p->bin_int = bin_int;
    return new FreebsdSystem(p);
}

REGISTER_SIM_OBJECT("FreebsdSystem", FreebsdSystem)

