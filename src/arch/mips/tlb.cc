/*
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
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
 */

#include <cstring>

#include "arch/mips/tlb.hh"

namespace MipsISA {
    Fault
    TLB::translate(RequestPtr req, ThreadContext *tc, bool)
    {
        Fault fault = GenericTLB::translate(req, tc);
        if (fault != NoFault)
            return fault;

        typeof(req->getSize()) size = req->getSize();
        Addr paddr = req->getPaddr();

        if (!isPowerOf2(size))
            panic("Invalid request size!\n");
        if ((size - 1) & paddr)
            return new GenericAlignmentFault(paddr);

        return NoFault;
    }

    void
    TlbEntry::serialize(std::ostream &os)
    {
        SERIALIZE_SCALAR(_pageStart);
    }

    void
    TlbEntry::unserialize(Checkpoint *cp, const std::string &section)
    {
        UNSERIALIZE_SCALAR(_pageStart);
    }
};

MipsISA::ITB *
MipsITBParams::create()
{
    return new MipsISA::ITB(this);
}

MipsISA::DTB *
MipsDTBParams::create()
{
    return new MipsISA::DTB(this);
}
