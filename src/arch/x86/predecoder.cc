/*
 * Copyright (c) 2007 The Hewlett-Packard Development Company
 * All rights reserved.
 *
 * Redistribution and use of this software in source and binary forms,
 * with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * The software must be used only for Non-Commercial Use which means any
 * use which is NOT directed to receiving any direct monetary
 * compensation for, or commercial advantage from such use.  Illustrative
 * examples of non-commercial use are academic research, personal study,
 * teaching, education and corporate research & development.
 * Illustrative examples of commercial use are distributing products for
 * commercial advantage and providing services using the software for
 * commercial advantage.
 *
 * If you wish to use this software or functionality therein that may be
 * covered by patents for commercial use, please contact:
 *     Director of Intellectual Property Licensing
 *     Office of Strategy and Technology
 *     Hewlett-Packard Company
 *     1501 Page Mill Road
 *     Palo Alto, California  94304
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.  Redistributions
 * in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.  Neither the name of
 * the COPYRIGHT HOLDER(s), HEWLETT-PACKARD COMPANY, nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.  No right of
 * sublicense is granted herewith.  Derivatives of the software and
 * output created using the software may be prepared, but only for
 * Non-Commercial Uses.  Derivatives of the software may be shared with
 * others provided: (i) the others agree to abide by the list of
 * conditions herein which includes the Non-Commercial Use restrictions;
 * and (ii) such Derivatives of the software include the above copyright
 * notice to acknowledge the contribution from this software where
 * applicable, this list of conditions and the disclaimer below.
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

#include "arch/x86/predecoder.hh"
#include "base/misc.hh"
#include "base/trace.hh"
#include "sim/host.hh"

namespace X86ISA
{
    void Predecoder::doReset()
    {
        origPC = basePC + offset;
        DPRINTF(Predecoder, "Setting origPC to %#x\n", origPC);
        emi.rex = 0;
        emi.legacy = 0;
        emi.opcode.num = 0;
        emi.opcode.op = 0;
        emi.opcode.prefixA = emi.opcode.prefixB = 0;

        immediateCollected = 0;
        emi.immediate = 0;
        emi.displacement = 0;

        emi.modRM = 0;
        emi.sib = 0;
        emi.mode = 0;
    }

    void Predecoder::process()
    {
        //This function drives the predecoder state machine.

        //Some sanity checks. You shouldn't try to process more bytes if
        //there aren't any, and you shouldn't overwrite an already
        //predecoder ExtMachInst.
        assert(!outOfBytes);
        assert(!emiIsReady);

        //While there's still something to do...
        while(!emiIsReady && !outOfBytes)
        {
            uint8_t nextByte = getNextByte();
            switch(state)
            {
              case ResetState:
                doReset();
                state = PrefixState;
              case PrefixState:
                state = doPrefixState(nextByte);
                break;
              case OpcodeState:
                state = doOpcodeState(nextByte);
                break;
              case ModRMState:
                state = doModRMState(nextByte);
                break;
              case SIBState:
                state = doSIBState(nextByte);
                break;
              case DisplacementState:
                state = doDisplacementState();
                break;
              case ImmediateState:
                state = doImmediateState();
                break;
              case ErrorState:
                panic("Went to the error state in the predecoder.\n");
              default:
                panic("Unrecognized state! %d\n", state);
            }
        }
    }

    //Either get a prefix and record it in the ExtMachInst, or send the
    //state machine on to get the opcode(s).
    Predecoder::State Predecoder::doPrefixState(uint8_t nextByte)
    {
        uint8_t prefix = Prefixes[nextByte];
        State nextState = PrefixState;
        if(prefix)
            consumeByte();
        switch(prefix)
        {
            //Operand size override prefixes
          case OperandSizeOverride:
            DPRINTF(Predecoder, "Found operand size override prefix.\n");
            emi.legacy.op = true;
            break;
          case AddressSizeOverride:
            DPRINTF(Predecoder, "Found address size override prefix.\n");
            emi.legacy.addr = true;
            break;
            //Segment override prefixes
          case CSOverride:
          case DSOverride:
          case ESOverride:
          case FSOverride:
          case GSOverride:
          case SSOverride:
            DPRINTF(Predecoder, "Found segment override.\n");
            emi.legacy.seg = prefix;
            break;
          case Lock:
            DPRINTF(Predecoder, "Found lock prefix.\n");
            emi.legacy.lock = true;
            break;
          case Rep:
            DPRINTF(Predecoder, "Found rep prefix.\n");
            emi.legacy.rep = true;
            break;
          case Repne:
            DPRINTF(Predecoder, "Found repne prefix.\n");
            emi.legacy.repne = true;
            break;
          case RexPrefix:
            DPRINTF(Predecoder, "Found Rex prefix %#x.\n", nextByte);
            emi.rex = nextByte;
            break;
          case 0:
            nextState = OpcodeState;
            break;
          default:
            panic("Unrecognized prefix %#x\n", nextByte);
        }
        return nextState;
    }

    //Load all the opcodes (currently up to 2) and then figure out
    //what immediate and/or ModRM is needed.
    Predecoder::State Predecoder::doOpcodeState(uint8_t nextByte)
    {
        State nextState = ErrorState;
        emi.opcode.num++;
        //We can't handle 3+ byte opcodes right now
        assert(emi.opcode.num < 3);
        consumeByte();
        if(emi.opcode.num == 1 && nextByte == 0x0f)
        {
            nextState = OpcodeState;
            DPRINTF(Predecoder, "Found two byte opcode.\n");
            emi.opcode.prefixA = nextByte;
        }
        else if(emi.opcode.num == 2 &&
                (nextByte == 0x0f ||
                 (nextByte & 0xf8) == 0x38))
        {
            panic("Three byte opcodes aren't yet supported!\n");
            nextState = OpcodeState;
            DPRINTF(Predecoder, "Found three byte opcode.\n");
            emi.opcode.prefixB = nextByte;
        }
        else
        {
            DPRINTF(Predecoder, "Found opcode %#x.\n", nextByte);
            emi.opcode.op = nextByte;

            //Figure out the effective operand size. This can be overriden to
            //a fixed value at the decoder level.
            int logOpSize;
            if(/*FIXME long mode*/1)
            {
                if(emi.rex.w)
                    logOpSize = 3; // 64 bit operand size
                else if(emi.legacy.op)
                    logOpSize = 1; // 16 bit operand size
                else
                    logOpSize = 2; // 32 bit operand size
            }
            else if(/*FIXME default 32*/1)
            {
                if(emi.legacy.op)
                    logOpSize = 1; // 16 bit operand size
                else
                    logOpSize = 2; // 32 bit operand size
            }
            else // 16 bit default operand size
            {
                if(emi.legacy.op)
                    logOpSize = 2; // 32 bit operand size
                else
                    logOpSize = 1; // 16 bit operand size
            }

            //Set the actual op size
            emi.opSize = 1 << logOpSize;

            //Figure out the effective address size. This can be overriden to
            //a fixed value at the decoder level.
            int logAddrSize;
            if(/*FIXME 64-bit mode*/1)
            {
                if(emi.legacy.addr)
                    logAddrSize = 2; // 32 bit address size
                else
                    logAddrSize = 3; // 64 bit address size
            }
            else if(/*FIXME default 32*/1)
            {
                if(emi.legacy.addr)
                    logAddrSize = 1; // 16 bit address size
                else
                    logAddrSize = 2; // 32 bit address size
            }
            else // 16 bit default operand size
            {
                if(emi.legacy.addr)
                    logAddrSize = 2; // 32 bit address size
                else
                    logAddrSize = 1; // 16 bit address size
            }

            //Set the actual address size
            emi.addrSize = 1 << logAddrSize;

            //Figure out how big of an immediate we'll retreive based
            //on the opcode.
            int immType = ImmediateType[emi.opcode.num - 1][nextByte];
            immediateSize = SizeTypeToSize[logOpSize - 1][immType];

            //Determine what to expect next
            if (UsesModRM[emi.opcode.num - 1][nextByte]) {
                nextState = ModRMState;
            } else {
                if(immediateSize) {
                    nextState = ImmediateState;
                } else {
                    emiIsReady = true;
                    nextState = ResetState;
                }
            }
        }
        return nextState;
    }

    //Get the ModRM byte and determine what displacement, if any, there is.
    //Also determine whether or not to get the SIB byte, displacement, or
    //immediate next.
    Predecoder::State Predecoder::doModRMState(uint8_t nextByte)
    {
        State nextState = ErrorState;
        ModRM modRM;
        modRM = nextByte;
        DPRINTF(Predecoder, "Found modrm byte %#x.\n", nextByte);
        if (0) {//FIXME in 16 bit mode
            //figure out 16 bit displacement size
            if(modRM.mod == 0 && modRM.rm == 6 || modRM.mod == 2)
                displacementSize = 2;
            else if(modRM.mod == 1)
                displacementSize = 1;
            else
                displacementSize = 0;
        } else {
            //figure out 32/64 bit displacement size
            if(modRM.mod == 0 && modRM.rm == 5 || modRM.mod == 2)
                displacementSize = 4;
            else if(modRM.mod == 1)
                displacementSize = 1;
            else
                displacementSize = 0;
        }

        // The "test" instruction in group 3 needs an immediate, even though
        // the other instructions with the same actual opcode don't.
        if (emi.opcode.num == 1 && (modRM.reg & 0x6) == 0) {
           if (emi.opcode.op == 0xF6)
               immediateSize = 1;
           else if (emi.opcode.op == 0xF7)
               immediateSize = (emi.opSize == 8) ? 4 : emi.opSize;
        }

        //If there's an SIB, get that next.
        //There is no SIB in 16 bit mode.
        if (modRM.rm == 4 && modRM.mod != 3) {
                // && in 32/64 bit mode)
            nextState = SIBState;
        } else if(displacementSize) {
            nextState = DisplacementState;
        } else if(immediateSize) {
            nextState = ImmediateState;
        } else {
            emiIsReady = true;
            nextState = ResetState;
        }
        //The ModRM byte is consumed no matter what
        consumeByte();
        emi.modRM = modRM;
        return nextState;
    }

    //Get the SIB byte. We don't do anything with it at this point, other
    //than storing it in the ExtMachInst. Determine if we need to get a
    //displacement or immediate next.
    Predecoder::State Predecoder::doSIBState(uint8_t nextByte)
    {
        State nextState = ErrorState;
        emi.sib = nextByte;
        DPRINTF(Predecoder, "Found SIB byte %#x.\n", nextByte);
        consumeByte();
        if (emi.modRM.mod == 0 && emi.sib.base == 5)
            displacementSize = 4;
        if (displacementSize) {
            nextState = DisplacementState;
        } else if(immediateSize) {
            nextState = ImmediateState;
        } else {
            emiIsReady = true;
            nextState = ResetState;
        }
        return nextState;
    }

    //Gather up the displacement, or at least as much of it
    //as we can get.
    Predecoder::State Predecoder::doDisplacementState()
    {
        State nextState = ErrorState;

        getImmediate(immediateCollected,
                emi.displacement,
                displacementSize);

        DPRINTF(Predecoder, "Collecting %d byte displacement, got %d bytes.\n",
                displacementSize, immediateCollected);

        if(displacementSize == immediateCollected) {
            //Reset this for other immediates.
            immediateCollected = 0;
            //Sign extend the displacement
            switch(displacementSize)
            {
              case 1:
                emi.displacement = sext<8>(emi.displacement);
                break;
              case 2:
                emi.displacement = sext<16>(emi.displacement);
                break;
              case 4:
                emi.displacement = sext<32>(emi.displacement);
                break;
              default:
                panic("Undefined displacement size!\n");
            }
            DPRINTF(Predecoder, "Collected displacement %#x.\n",
                    emi.displacement);
            if(immediateSize) {
                nextState = ImmediateState;
            } else {
                emiIsReady = true;
                nextState = ResetState;
            }
        }
        else
            nextState = DisplacementState;
        return nextState;
    }

    //Gather up the immediate, or at least as much of it
    //as we can get
    Predecoder::State Predecoder::doImmediateState()
    {
        State nextState = ErrorState;

        getImmediate(immediateCollected,
                emi.immediate,
                immediateSize);

        DPRINTF(Predecoder, "Collecting %d byte immediate, got %d bytes.\n",
                immediateSize, immediateCollected);

        if(immediateSize == immediateCollected)
        {
            //Reset this for other immediates.
            immediateCollected = 0;

            //XXX Warning! The following is an observed pattern and might
            //not always be true!

            //Instructions which use 64 bit operands but 32 bit immediates
            //need to have the immediate sign extended to 64 bits.
            //Instructions which use true 64 bit immediates won't be
            //affected, and instructions that use true 32 bit immediates
            //won't notice.
            switch(immediateSize)
            {
              case 4:
                emi.immediate = sext<32>(emi.immediate);
                break;
              case 1:
                emi.immediate = sext<8>(emi.immediate);
            }

            DPRINTF(Predecoder, "Collected immediate %#x.\n",
                    emi.immediate);
            emiIsReady = true;
            nextState = ResetState;
        }
        else
            nextState = ImmediateState;
        return nextState;
    }
}
