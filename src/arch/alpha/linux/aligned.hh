/*
 * Copyright (c) 2004 The Regents of The University of Michigan
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
 * Authors: Ali Saidi
 *          Nathan Binkert
 */

#ifndef __ARCH_ALPHA_LINUX_ALIGNED_HH__
#define __ARCH_ALPHA_LINUX_ALIGNED_HH__


/* GCC 3.3.X has a bug in which attributes+typedefs don't work. 3.2.X is fine
 * as in 3.4.X, but the bug is marked will not fix in 3.3.X so here is
 * the work around.
 */
#if (__GNUC__ == 3 && __GNUC_MINOR__  != 3) || __GNUC__ > 3
typedef uint64_t uint64_ta __attribute__ ((aligned (8))) ;
typedef int64_t int64_ta __attribute__ ((aligned (8))) ;
typedef Addr Addr_a __attribute__ ((aligned (8))) ;
#else
#define uint64_ta uint64_t __attribute__ ((aligned (8)))
#define int64_ta int64_t __attribute__ ((aligned (8)))
#define Addr_a Addr __attribute__ ((aligned (8)))
#endif /* __GNUC__ __GNUC_MINOR__ */

#endif /* __ARCH_ALPHA_LINUX_ALIGNED_HH__ */
