# Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
# Copyright (c) 2009 The Hewlett-Packard Development Company
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

from m5.util import code_formatter, orderdict

from slicc.util import PairContainer
from slicc.symbols.Symbol import Symbol

class DataMember(PairContainer):
    def __init__(self, ident, type, pairs, init_code):
        super(DataMember, self).__init__(pairs)
        self.ident = ident
        self.type = type
        self.init_code = init_code

class Enumeration(PairContainer):
    def __init__(self, ident, pairs):
        super(Enumeration, self).__init__(pairs)
        self.ident = ident

class Method(object):
    def __init__(self, return_type, param_types):
        self.return_type = return_type
        self.param_types = param_types

class Type(Symbol):
    def __init__(self, table, ident, location, pairs, machine=None):
        super(Type, self).__init__(table, ident, location, pairs)
        self.c_ident = ident
        self.abstract_ident = ""
        if machine:
            if self.isExternal or self.isPrimitive:
                if "external_name" in self:
                    self.c_ident = self["external_name"]
            else:
                # Append with machine name
                self.c_ident = "%s_%s" % (machine, ident)

        self.pairs.setdefault("desc", "No description avaliable")

        # check for interface that this Type implements
        if "interface" in self:
            interface = self["interface"]
            if interface in ("Message", "NetworkMessage"):
                self["message"] = "yes"
            if interface == "NetworkMessage":
                self["networkmessage"] = "yes"

        # FIXME - all of the following id comparisons are fragile hacks
        if self.ident in ("CacheMemory", "NewCacheMemory",
                          "TLCCacheMemory", "DNUCACacheMemory",
                          "DNUCABankCacheMemory", "L2BankCacheMemory",
                          "CompressedCacheMemory", "PrefetchCacheMemory"):
            self["cache"] = "yes"

        if self.ident in ("TBETable", "DNUCATBETable", "DNUCAStopTable"):
            self["tbe"] = "yes"

        if self.ident == "NewTBETable":
            self["newtbe"] = "yes"

        if self.ident == "TimerTable":
            self["timer"] = "yes"

        if self.ident == "DirectoryMemory":
            self["dir"] = "yes"

        if self.ident == "PersistentTable":
            self["persistent"] = "yes"

        if self.ident == "Prefetcher":
            self["prefetcher"] = "yes"

        if self.ident == "DNUCA_Movement":
            self["mover"] = "yes"

        self.isMachineType = (ident == "MachineType")

        self.data_members = orderdict()

        # Methods
        self.methods = {}

        # Enums
        self.enums = orderdict()

    @property
    def isPrimitive(self):
        return "primitive" in self
    @property
    def isNetworkMessage(self):
        return "networkmessage" in self
    @property
    def isMessage(self):
        return "message" in self
    @property
    def isBuffer(self):
        return "buffer" in self
    @property
    def isInPort(self):
        return "inport" in self
    @property
    def isOutPort(self):
        return "outport" in self
    @property
    def isEnumeration(self):
        return "enumeration" in self
    @property
    def isExternal(self):
        return "external" in self
    @property
    def isGlobal(self):
        return "global" in self
    @property
    def isInterface(self):
        return "interface" in self

    # Return false on error
    def dataMemberAdd(self, ident, type, pairs, init_code):
        if ident in self.data_members:
            return False

        member = DataMember(ident, type, pairs, init_code)
        self.data_members[ident] = member

        return True

    def dataMemberType(self, ident):
        return self.data_members[ident].type

    def methodId(self, name, param_type_vec):
        return '_'.join([name] + [ pt.c_ident for pt in param_type_vec ])

    def methodIdAbstract(self, name, param_type_vec):
        return '_'.join([name] + [ pt.abstract_ident for pt in param_type_vec ])

    def methodAdd(self, name, return_type, param_type_vec):
        ident = self.methodId(name, param_type_vec)
        if ident in self.methods:
            return False

        self.methods[ident] = Method(return_type, param_type_vec)
        return True

    def enumAdd(self, ident, pairs):
        if ident in self.enums:
            return False

        self.enums[ident] = Enumeration(ident, pairs)

        # Add default
        if "default" not in self:
            self["default"] = "%s_NUM" % self.c_ident

        return True

    def writeCodeFiles(self, path):
        if self.isExternal:
            # Do nothing
            pass
        elif self.isEnumeration:
            self.printEnumHH(path)
            self.printEnumCC(path)
        else:
            # User defined structs and messages
            self.printTypeHH(path)
            self.printTypeCC(path)

    def printTypeHH(self, path):
        code = code_formatter()
        code('''
/** \\file ${{self.c_ident}}.hh
 *
 *
 * Auto generated C++ code started by $__file__:$__line__
 */

#ifndef ${{self.c_ident}}_H
#define ${{self.c_ident}}_H

#include "mem/ruby/common/Global.hh"
#include "mem/gems_common/Allocator.hh"
''')

        for dm in self.data_members.values():
            if not dm.type.isPrimitive:
                code('#include "mem/protocol/$0.hh"', dm.type.c_ident)

        parent = ""
        if "interface" in self:
            code('#include "mem/protocol/$0.hh"', self["interface"])
            parent = " :  public %s" % self["interface"]

        code('''
$klass ${{self.c_ident}}$parent {
  public:
    ${{self.c_ident}}()
''', klass="class")

        # Call superclass constructor
        if "interface" in self:
            code('        : ${{self["interface"]}}()')

        code.indent()
        code("{")
        if not self.isGlobal:
            code.indent()
            for dm in self.data_members.values():
                ident = dm.ident
                if "default" in dm:
                    # look for default value
                    code('m_$ident = ${{dm["default"]}}; // default for this field')
                elif "default" in dm.type:
                    # Look for the type default
                    tid = dm.type.c_ident
                    code('m_$ident = ${{dm.type["default"]}}; // default value of $tid')
                else:
                    code('// m_$ident has no default')
            code.dedent()
        code('}')

        # ******** Default destructor ********
        code('~${{self.c_ident}}() { };')

        # ******** Full init constructor ********
        if not self.isGlobal:
            params = [ 'const %s& local_%s' % (dm.type.c_ident, dm.ident) \
                       for dm in self.data_members.itervalues() ]

            if self.isMessage:
                params.append('const unsigned local_proc_id')
            
            params = ', '.join(params)
            code('${{self.c_ident}}($params)')

            # Call superclass constructor
            if "interface" in self:
                code('    : ${{self["interface"]}}()')

            code('{')
            code.indent()
            for dm in self.data_members.values():
                code('m_${{dm.ident}} = local_${{dm.ident}};')
                if "nextLineCallHack" in dm:
                    code('m_${{dm.ident}}${{dm["nextLineCallHack"]}};')

            if self.isMessage:
                code('proc_id = local_proc_id;')
            
            code.dedent()
            code('}')

        # create a static factory method
        if "interface" in self:
            code('''
static ${{self["interface"]}}* create() {
    return new ${{self.c_ident}}();
}
''')

        # ******** Message member functions ********
        # FIXME: those should be moved into slicc file, slicc should
        # support more of the c++ class inheritance

        if self.isMessage:
            code('''
Message* clone() const { checkAllocator(); return s_allocator_ptr->allocate(*this); }
void destroy() { checkAllocator(); s_allocator_ptr->deallocate(this); }
static Allocator<${{self.c_ident}}>* s_allocator_ptr;
static void checkAllocator() { if (s_allocator_ptr == NULL) { s_allocator_ptr = new Allocator<${{self.c_ident}}>; }}
''')

        if not self.isGlobal:
            # const Get methods for each field
            code('// Const accessors methods for each field')
            for dm in self.data_members.values():
                code('''
/** \\brief Const accessor method for ${{dm.ident}} field.
 *  \\return ${{dm.ident}} field
 */
const ${{dm.type.c_ident}}& get${{dm.ident}}() const { return m_${{dm.ident}}; }
''')

            # Non-const Get methods for each field
            code('// Non const Accessors methods for each field')
            for dm in self.data_members.values():
                code('''
/** \\brief Non-const accessor method for ${{dm.ident}} field.
 *  \\return ${{dm.ident}} field
 */
${{dm.type.c_ident}}& get${{dm.ident}}() { return m_${{dm.ident}}; }
''')

            #Set methods for each field
            code('// Mutator methods for each field')
            for dm in self.data_members.values():
                code('''
/** \\brief Mutator method for ${{dm.ident}} field */
void set${{dm.ident}}(const ${{dm.type.c_ident}}& local_${{dm.ident}}) { m_${{dm.ident}} = local_${{dm.ident}}; }
''')

        code('void print(ostream& out) const;')
        code.dedent()
        code('  //private:')
        code.indent()

        # Data members for each field
        for dm in self.data_members.values():
            if "abstract" not in dm:
                const = ""
                init = ""

                # global structure
                if self.isGlobal:
                    const = "static const "

                # init value
                if dm.init_code:
                    # only global structure can have init value here
                    assert self.isGlobal
                    init = " = %s" % (dm.init_code)

                desc = ""
                if "desc" in dm:
                    desc = '/**< %s */' % dm["desc"]

                code('$const${{dm.type.c_ident}} m_${{dm.ident}}$init; $desc')

        if self.isMessage:
            code('unsigned proc_id;')

        code.dedent()
        code('};')

        code('''
// Output operator declaration
ostream& operator<<(ostream& out, const ${{self.c_ident}}& obj);

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const ${{self.c_ident}}& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // ${{self.c_ident}}_H
''')

        code.write(path, "%s.hh" % self.c_ident)

    def printTypeCC(self, path):
        code = code_formatter()

        code('''
/** \\file ${{self.c_ident}}.cc
 *
 * Auto generated C++ code started by $__file__:$__line__
 */

#include "mem/protocol/${{self.c_ident}}.hh"
''')

        if self.isMessage:
            code('Allocator<${{self.c_ident}}>* ${{self.c_ident}}::s_allocator_ptr = NULL;')
        code('''
/** \\brief Print the state of this object */
void ${{self.c_ident}}::print(ostream& out) const
{
    out << "[${{self.c_ident}}: ";
''')

        # For each field
        code.indent()
        for dm in self.data_members.values():
            code('out << "${{dm.ident}} = " << m_${{dm.ident}} << " ";''')

        if self.isMessage:
            code('out << "Time = " << getTime() << " ";')
        code.dedent()

        # Trailer
        code('''
    out << "]";
}''')

        code.write(path, "%s.cc" % self.c_ident)

    def printEnumHH(self, path):
        code = code_formatter()
        code('''
/** \\file ${{self.c_ident}}.hh
 *
 * Auto generated C++ code started by $__file__:$__line__
 */
#ifndef ${{self.c_ident}}_H
#define ${{self.c_ident}}_H

#include "mem/ruby/common/Global.hh"

/** \\enum ${{self.c_ident}}
 *  \\brief ${{self.desc}}
 */
enum ${{self.c_ident}} {
    ${{self.c_ident}}_FIRST,
''')

        code.indent()
        # For each field
        for i,(ident,enum) in enumerate(self.enums.iteritems()):
            desc = enum.get("desc", "No description avaliable")
            if i == 0: 
                init = ' = %s_FIRST' % self.c_ident 
            else:
                init = ''
            code('${{self.c_ident}}_${{enum.ident}}$init, /**< $desc */')
        code.dedent()
        code('''
    ${{self.c_ident}}_NUM
};
${{self.c_ident}} string_to_${{self.c_ident}}(const string& str);
string ${{self.c_ident}}_to_string(const ${{self.c_ident}}& obj);
${{self.c_ident}} &operator++(${{self.c_ident}} &e);
''')

        # MachineType hack used to set the base component id for each Machine
        if self.isMachineType:
            code('''
int ${{self.c_ident}}_base_level(const ${{self.c_ident}}& obj);
MachineType ${{self.c_ident}}_from_base_level(int);
int ${{self.c_ident}}_base_number(const ${{self.c_ident}}& obj);
int ${{self.c_ident}}_base_count(const ${{self.c_ident}}& obj);
''')

            for enum in self.enums.itervalues():
                code('#define MACHINETYPE_${{enum.ident}} 1')

        # Trailer
        code('''
ostream& operator<<(ostream& out, const ${{self.c_ident}}& obj);

#endif // ${{self.c_ident}}_H
''')

        code.write(path, "%s.hh" % self.c_ident)

    def printEnumCC(self, path):
        code = code_formatter()
        code('''
/** \\file ${{self.c_ident}}.hh
 *
 * Auto generated C++ code started by $__file__:$__line__
 */

#include "mem/protocol/${{self.c_ident}}.hh"

''')

        if self.isMachineType:
            for enum in self.enums.itervalues():
                code('#include "mem/protocol/${{enum.ident}}_Controller.hh"')

        code('''
ostream& operator<<(ostream& out, const ${{self.c_ident}}& obj)
{
    out << ${{self.c_ident}}_to_string(obj);
    out << flush;
    return out;
}

string ${{self.c_ident}}_to_string(const ${{self.c_ident}}& obj)
{
    switch(obj) {
''')

        # For each field
        code.indent()
        for enum in self.enums.itervalues():
            code('  case ${{self.c_ident}}_${{enum.ident}}:')
            code('    return "${{enum.ident}}";')
        code.dedent()

        # Trailer
        code('''
      default:
        ERROR_MSG("Invalid range for type ${{self.c_ident}}");
        return "";
    }
}

${{self.c_ident}} string_to_${{self.c_ident}}(const string& str)
{
''')

        # For each field
        code.indent()
        code("if (false) {")
        start = "} else "
        for enum in self.enums.itervalues():
            code('${start}if (str == "${{enum.ident}}") {')
            code('    return ${{self.c_ident}}_${{enum.ident}};')
        code.dedent()

        code('''
    } else {
        WARN_EXPR(str);
        ERROR_MSG("Invalid string conversion for type ${{self.c_ident}}");
    }
}

${{self.c_ident}}& operator++(${{self.c_ident}}& e) {
    assert(e < ${{self.c_ident}}_NUM);
    return e = ${{self.c_ident}}(e+1);
}
''')

        # MachineType hack used to set the base level and number of
        # components for each Machine
        if self.isMachineType:
            code('''
/** \\brief returns the base vector index for each machine type to be used by NetDest
  *
  * \\return the base vector index for each machine type to be used by NetDest
  * \\see NetDest.hh
  */
int ${{self.c_ident}}_base_level(const ${{self.c_ident}}& obj)
{
    switch(obj) {
''')

            # For each field
            code.indent()
            for i,enum in enumerate(self.enums.itervalues()):
                code('  case ${{self.c_ident}}_${{enum.ident}}:')
                code('    return $i;')
            code.dedent()

            # total num
            code('''
      case ${{self.c_ident}}_NUM:
        return ${{len(self.enums)}};

      default:
        ERROR_MSG("Invalid range for type ${{self.c_ident}}");
        return -1;
    }
}

/** \\brief returns the machine type for each base vector index used by NetDest
 *
 * \\return the MachineTYpe
 */
MachineType ${{self.c_ident}}_from_base_level(int type)
{
    switch(type) {
''')

            # For each field
            code.indent()
            for i,enum in enumerate(self.enums.itervalues()):
                code('  case $i:')
                code('    return ${{self.c_ident}}_${{enum.ident}};')
            code.dedent()

            # Trailer
            code('''
      default:
        ERROR_MSG("Invalid range for type ${{self.c_ident}}");
        return MachineType_NUM;
    }
}

/** \\brief The return value indicates the number of components created
 * before a particular machine\'s components
 *
 * \\return the base number of components for each machine
 */
int ${{self.c_ident}}_base_number(const ${{self.c_ident}}& obj)
{
    int base = 0;
    switch(obj) {
''')

            # For each field
            code.indent()
            code('  case ${{self.c_ident}}_NUM:')
            for enum in reversed(self.enums.values()):
                code('    base += ${{enum.ident}}_Controller::getNumControllers();')
                code('  case ${{self.c_ident}}_${{enum.ident}}:')
            code('    break;')
            code.dedent()

            code('''
      default:
        ERROR_MSG("Invalid range for type ${{self.c_ident}}");
        return -1;
    }

    return base;
}

/** \\brief returns the total number of components for each machine
 * \\return the total number of components for each machine
 */
int ${{self.c_ident}}_base_count(const ${{self.c_ident}}& obj)
{
    switch(obj) {
''')

            # For each field
            for enum in self.enums.itervalues():
                code('''
      case ${{self.c_ident}}_${{enum.ident}}:
        return ${{enum.ident}}_Controller::getNumControllers();
''')

            # total num
            code('''
      case ${{self.c_ident}}_NUM:
      default:
        ERROR_MSG("Invalid range for type ${{self.c_ident}}");
        return -1;
    }
}
''')

        # Write the file
        code.write(path, "%s.cc" % self.c_ident)

__all__ = [ "Type" ]
