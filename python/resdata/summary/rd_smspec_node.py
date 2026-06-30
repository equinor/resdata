import sys

from cwrap import BaseCClass

import resdata.summary._rd_smspec_node as _rd_smspec_node
from resdata.util.util import monkey_the_camel

from .rd_sum_var_type import SummaryVarType


class ResdataSMSPECNode(BaseCClass):
    """
    Small class with some meta information about a summary variable.

    The summary variables have different attributes, like if they
    represent a total quantity, a rate or a historical quantity. These
    quantities, in addition to the underlying values like WGNAMES,
    KEYWORD and NUMS taken from the the SMSPEC file are stored in this
    structure.
    """

    TYPE_NAME = "rd_smspec_node"

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly!")

    def cmp(self, other):
        if isinstance(other, ResdataSMSPECNode):
            return _rd_smspec_node._cmp(self, other)
        else:
            raise TypeError("Other argument must be of type ResdataSMSPECNode")

    def __lt__(self, other):
        return self.cmp(other) < 0

    def __gt__(self, other):
        return self.cmp(other) > 0

    def __eq__(self, other):
        return self.cmp(other) == 0

    def __hash__(self, other):
        return hash(_rd_smspec_node._gen_key1(self))

    @property
    def unit(self):
        """
        Returns the unit of this node as a string.
        """
        return _rd_smspec_node._node_unit(self)

    @property
    def wgname(self):
        """
        Returns the WGNAME property for this node.

        Many variables do not have the WGNAME property, i.e. the field
        related variables like FOPT and the block properties like
        BPR:10,10,10. For these variables the function will return
        None, and not the dummy value: ":+:+:+:+".
        """
        return _rd_smspec_node._node_wgname(self)

    @property
    def keyword(self):
        """
        Returns the KEYWORD property for this node.

        The KEYWORD property is the main classification property in
        the SMSPEC file. The properties of a variable can be
        read from the KEWYORD value; see table 3.4 in the ECLIPSE file
        format reference manual.
        """
        return _rd_smspec_node._node_keyword(self)

    @property
    def num(self):
        return self.get_num()

    @property
    def default(self):
        """Will return the default value for this key.

        The default value is typically used when fetching values from a
        historical case, when the key is only present in the restarted case.
        The default value is also used to initialize the PARAMS vector when
        writing to file.
        """
        return _rd_smspec_node._get_default(self)

    def get_key1(self):
        """
        Returns the primary composite key, i.e. like 'WOPR:OPX' for this
        node.
        """
        return _rd_smspec_node._gen_key1(self)

    def get_key2(self):
        """Returns the secondary composite key for this node.

        Most variables have only one composite key, but in particular
        nodes which involve (i,j,k) coordinates will contain two
        forms:

            get_key1()  =>  "BPR:10,11,6"
            get_key2()  =>  "BPR:52423"

        Where the '52423' in get_key2() corresponds to i + j*nx +
        k*nx*ny.
        """
        return _rd_smspec_node._gen_key2(self)

    def var_type(self) -> SummaryVarType:
        return SummaryVarType(_rd_smspec_node._var_type(self))

    def get_num(self):
        """
        Returns the NUMS value for this keyword; or None.

        Many of the summary keywords have an integer stored in the
        vector NUMS as an attribute, i.e. the block properties have
        the global index of the cell in the nums vector. If the
        variable in question makes use of the NUMS value this property
        will return the value, otherwise it will return None:

           sum.smspec_node("FOPT").num     => None
           sum.smspec_node("BPR:1000").num => 1000

        """
        if _rd_smspec_node._node_need_num(self):
            return _rd_smspec_node._node_num(self)
        else:
            return None

    def is_rate(self):
        """
        Will check if the variable in question is a rate variable.

        The conecpt of rate variabel is important (internally) when
        interpolation values to arbitrary times.
        """
        return _rd_smspec_node._node_is_rate(self)

    def is_total(self):
        """
        Will check if the node corresponds to a total quantity.

        The question of whether a variable corresponds to a 'total'
        quantity or not can be interesting for e.g. interpolation
        purposes. The actual question whether a quantity is total or
        not is based on a hardcoded list in smspec_node_set_flags() in
        smspec_node.c; this list again is based on the tables 2.7 -
        2.11 in the ECLIPSE fileformat documentation.
        """
        return _rd_smspec_node._node_is_total(self)

    def is_historical(self):
        """
        Checks if the key corresponds to a historical variable.

        The check is only based on the last character; all variables
        ending with 'H' are considered historical.
        """
        return _rd_smspec_node._node_is_historical(self)


sys.modules[__package__].ResdataSMSPECNode = ResdataSMSPECNode

monkey_the_camel(ResdataSMSPECNode, "getKey1", ResdataSMSPECNode.get_key1)
monkey_the_camel(ResdataSMSPECNode, "getKey2", ResdataSMSPECNode.get_key2)
monkey_the_camel(ResdataSMSPECNode, "varType", ResdataSMSPECNode.var_type)
monkey_the_camel(ResdataSMSPECNode, "getNum", ResdataSMSPECNode.get_num)
monkey_the_camel(ResdataSMSPECNode, "isRate", ResdataSMSPECNode.is_rate)
monkey_the_camel(ResdataSMSPECNode, "isTotal", ResdataSMSPECNode.is_total)
monkey_the_camel(ResdataSMSPECNode, "isHistorical", ResdataSMSPECNode.is_historical)
