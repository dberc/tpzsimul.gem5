# Copyright (c) 2006 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
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
# Authors: Steve Reinhardt

import os, sys

# single "path" arg encodes everything we need to know about test
(category, name, isa, opsys, config) = sys.argv[1].split('/')

# find path to directory containing this file
tests_root = os.path.dirname(__file__)
test_progs = os.path.join(tests_root, 'test-progs')

# generate path to binary file
def binpath(app, file=None):
    # executable has same name as app unless specified otherwise
    if not file:
        file = app
    return os.path.join(test_progs, app, 'bin', isa, opsys, file)

# generate path to input file
def inputpath(app, file=None):
    # input file has same name as app unless specified otherwise
    if not file:
        file = app
    return os.path.join(test_progs, app, 'input', file)

# build configuration
execfile(os.path.join(tests_root, 'configs', config + '.py'))

# set default maxtick... script can override
# -1 means run forever
from m5 import MaxTick
maxtick = MaxTick

# tweak configuration for specific test

execfile(os.path.join(tests_root, category, name, 'test.py'))

# instantiate configuration
m5.instantiate(root)

# simulate until program terminates
exit_event = m5.simulate(maxtick)

print 'Exiting @ tick', m5.curTick(), 'because', exit_event.getCause()
