/*
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 *
 * Authors: Gabe Black
 *          Ali Saidi
 */

#ifndef __ARCH_SPARC_ISA_TRAITS_HH__
#define __ARCH_SPARC_ISA_TRAITS_HH__

#include "arch/sparc/types.hh"
#include "arch/sparc/sparc_traits.hh"
#include "base/types.hh"
#include "config/full_system.hh"

class StaticInstPtr;

namespace BigEndianGuest {}

namespace SparcISA
{
const int MachineBytes = 8;

// This makes sure the big endian versions of certain functions are used.
using namespace BigEndianGuest;

// SPARC has a delay slot
#define ISA_HAS_DELAY_SLOT 1

// SPARC NOP (sethi %(hi(0), g0)
const MachInst NoopMachInst = 0x01000000;

// 8K. This value is implmentation specific; and should probably
// be somewhere else.
const int LogVMPageSize = 13;
const int VMPageSize = (1 << LogVMPageSize);

// real address virtual mapping
// sort of like alpha super page, but less frequently used
const Addr SegKPMEnd  = ULL(0xfffffffc00000000);
const Addr SegKPMBase = ULL(0xfffffac000000000);

// Why does both the previous set of constants and this one exist?
const int PageShift = 13;
const int PageBytes = 1ULL << PageShift;

const int BranchPredAddrShiftAmt = 2;

StaticInstPtr decodeInst(ExtMachInst);

/////////// TLB Stuff ////////////
const Addr StartVAddrHole = ULL(0x0000800000000000);
const Addr EndVAddrHole = ULL(0xFFFF7FFFFFFFFFFF);
const Addr VAddrAMask = ULL(0xFFFFFFFF);
const Addr PAddrImplMask = ULL(0x000000FFFFFFFFFF);
const Addr BytesInPageMask = ULL(0x1FFF);

#if FULL_SYSTEM
enum InterruptTypes
{
    IT_TRAP_LEVEL_ZERO,
    IT_HINTP,
    IT_INT_VEC,
    IT_CPU_MONDO,
    IT_DEV_MONDO,
    IT_RES_ERROR,
    IT_SOFT_INT,
    NumInterruptTypes
};

#endif

// Memory accesses cannot be unaligned
const bool HasUnalignedMemAcc = false;
}

#endif // __ARCH_SPARC_ISA_TRAITS_HH__
