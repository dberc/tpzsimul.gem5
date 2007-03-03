from m5.SimObject import SimObject
from m5.params import *
from m5.proxy import *
class IntrControl(SimObject):
    type = 'IntrControl'
    sys = Param.System(Parent.any, "the system we are part of")
