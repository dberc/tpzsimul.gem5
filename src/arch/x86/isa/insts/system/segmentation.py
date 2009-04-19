# Copyright (c) 2007 The Hewlett-Packard Development Company
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms,
# with or without modification, are permitted provided that the
# following conditions are met:
#
# The software must be used only for Non-Commercial Use which means any
# use which is NOT directed to receiving any direct monetary
# compensation for, or commercial advantage from such use.  Illustrative
# examples of non-commercial use are academic research, personal study,
# teaching, education and corporate research & development.
# Illustrative examples of commercial use are distributing products for
# commercial advantage and providing services using the software for
# commercial advantage.
#
# If you wish to use this software or functionality therein that may be
# covered by patents for commercial use, please contact:
#     Director of Intellectual Property Licensing
#     Office of Strategy and Technology
#     Hewlett-Packard Company
#     1501 Page Mill Road
#     Palo Alto, California  94304
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.  Redistributions
# in binary form must reproduce the above copyright notice, this list of
# conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.  Neither the name of
# the COPYRIGHT HOLDER(s), HEWLETT-PACKARD COMPANY, nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.  No right of
# sublicense is granted herewith.  Derivatives of the software and
# output created using the software may be prepared, but only for
# Non-Commercial Uses.  Derivatives of the software may be shared with
# others provided: (i) the others agree to abide by the list of
# conditions herein which includes the Non-Commercial Use restrictions;
# and (ii) such Derivatives of the software include the above copyright
# notice to acknowledge the contribution from this software where
# applicable, this list of conditions and the disclaimer below.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Gabe Black

microcode = '''
def macroop LGDT_M
{
    .adjust_env maxOsz

    # Get the limit
    ld t1, seg, sib, disp, dataSize=2
    # Get the base
    ld t2, seg, sib, 'adjustedDisp + 2'
    wrbase tsg, t2
    wrlimit tsg, t1
};

def macroop LGDT_P
{
    .adjust_env maxOsz

    rdip t7
    # Get the limit
    ld t1, seg, riprel, disp, dataSize=2
    # Get the base
    ld t2, seg, riprel, 'adjustedDisp + 2'
    wrbase tsg, t2
    wrlimit tsg, t1
};

#
# These versions are for when the original data size was 16 bits. The base is
# still 32 bits, but the top byte is zeroed before being used.
#

def macroop LGDT_16_M
{
    .adjust_env maxOsz

    # Get the limit
    ld t1, seg, sib, disp, dataSize=2
    # Get the base
    ld t2, seg, sib, 'adjustedDisp + 2', dataSize=4
    zexti t2, t2, 23, dataSize=8
    wrbase tsg, t2
    wrlimit tsg, t1
};

def macroop LGDT_16_P
{
    .adjust_env maxOsz

    rdip t7
    # Get the limit
    ld t1, seg, riprel, disp, dataSize=2
    # Get the base
    ld t2, seg, riprel, 'adjustedDisp + 2', dataSize=4
    zexti t2, t2, 23, dataSize=8
    wrbase tsg, t2
    wrlimit tsg, t1
};

def macroop LIDT_M
{
    .adjust_env maxOsz

    # Get the limit
    ld t1, seg, sib, disp, dataSize=2
    # Get the base
    ld t2, seg, sib, 'adjustedDisp + 2'
    wrbase idtr, t2
    wrlimit idtr, t1
};

def macroop LIDT_P
{
    .adjust_env maxOsz

    rdip t7
    # Get the limit
    ld t1, seg, riprel, disp, dataSize=2
    # Get the base
    ld t2, seg, riprel, 'adjustedDisp + 2'
    wrbase idtr, t2
    wrlimit idtr, t1
};

#
# These versions are for when the original data size was 16 bits. The base is
# still 32 bits, but the top byte is zeroed before being used.
#

def macroop LIDT_16_M
{
    .adjust_env maxOsz

    # Get the limit
    ld t1, seg, sib, disp, dataSize=2
    # Get the base
    ld t2, seg, sib, 'adjustedDisp + 2', dataSize=4
    zexti t2, t2, 23, dataSize=8
    wrbase idtr, t2
    wrlimit idtr, t1
};

def macroop LIDT_16_P
{
    .adjust_env maxOsz

    rdip t7
    # Get the limit
    ld t1, seg, riprel, disp, dataSize=2
    # Get the base
    ld t2, seg, riprel, 'adjustedDisp + 2', dataSize=4
    zexti t2, t2, 23, dataSize=8
    wrbase idtr, t2
    wrlimit idtr, t1
};

def macroop LTR_R
{
    chks reg, t0, TRCheck
    limm t4, 0, dataSize=8
    srli t4, reg, 3, dataSize=2
    ldst t1, tsg, [8, t4, t0], dataSize=8
    ld t2, tsg, [8, t4, t0], 8, dataSize=8
    chks reg, t1, TSSCheck
    wrdh t3, t1, t2
    wrdl tr, t1, reg
    wrbase tr, t3, dataSize=8
    ori t1, t1, (1 << 9)
    st t1, tsg, [8, t4, t0], dataSize=8
};

def macroop LTR_M
{
    ld t5, seg, sib, disp, dataSize=2
    chks t5, t0, TRCheck
    limm t4, 0, dataSize=8
    srli t4, t5, 3, dataSize=2
    ldst t1, tsg, [8, t4, t0], dataSize=8
    ld t2, tsg, [8, t4, t0], 8, dataSize=8
    chks t5, t1, TSSCheck
    wrdh t3, t1, t2
    wrdl tr, t1, t5
    wrbase tr, t3, dataSize=8
    ori t1, t1, (1 << 9)
    st t1, tsg, [8, t4, t0], dataSize=8
};

def macroop LTR_P
{
    rdip t7
    ld t5, seg, riprel, disp, dataSize=2
    chks t5, t0, TRCheck
    limm t4, 0, dataSize=8
    srli t4, t5, 3, dataSize=2
    ldst t1, tsg, [8, t4, t0], dataSize=8
    ld t2, tsg, [8, t4, t0], 8, dataSize=8
    chks t5, t1, TSSCheck
    wrdh t3, t1, t2
    wrdl tr, t1, t5
    wrbase tr, t3, dataSize=8
    ori t1, t1, (1 << 9)
    st t1, tsg, [8, t4, t0], dataSize=8
};

def macroop LLDT_R
{
    chks reg, t0, InGDTCheck, flags=(EZF,)
    br label("end"), flags=(CEZF,)
    limm t4, 0, dataSize=8
    srli t4, reg, 3, dataSize=2
    ldst t1, tsg, [8, t4, t0], dataSize=8
    ld t2, tsg, [8, t4, t0], 8, dataSize=8
    chks reg, t1, LDTCheck
    wrdh t3, t1, t2
    wrdl tr, t1, reg
    wrbase tr, t3, dataSize=8
end:
    fault "NoFault"
};

def macroop LLDT_M
{
    ld t5, seg, sib, disp, dataSize=2
    chks t5, t0, InGDTCheck, flags=(EZF,)
    br label("end"), flags=(CEZF,)
    limm t4, 0, dataSize=8
    srli t4, t5, 3, dataSize=2
    ldst t1, tsg, [8, t4, t0], dataSize=8
    ld t2, tsg, [8, t4, t0], 8, dataSize=8
    chks t5, t1, LDTCheck
    wrdh t3, t1, t2
    wrdl tr, t1, t5
    wrbase tr, t3, dataSize=8
end:
    fault "NoFault"
};

def macroop LLDT_P
{
    rdip t7
    ld t5, seg, riprel, disp, dataSize=2
    chks t5, t0, InGDTCheck, flags=(EZF,)
    br label("end"), flags=(CEZF,)
    limm t4, 0, dataSize=8
    srli t4, t5, 3, dataSize=2
    ldst t1, tsg, [8, t4, t0], dataSize=8
    ld t2, tsg, [8, t4, t0], 8, dataSize=8
    chks t5, t1, LDTCheck
    wrdh t3, t1, t2
    wrdl tr, t1, t5
    wrbase tr, t3, dataSize=8
end:
    fault "NoFault"
};

def macroop SWAPGS
{
    rdval t1, kernel_gs_base, dataSize=8
    rdbase t2, gs, dataSize=8
    wrbase gs, t1, dataSize=8
    wrval kernel_gs_base, t2, dataSize=8
};
'''
