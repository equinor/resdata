#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <detail/resdata/cwrap_pybind.hpp>
#include <resdata/smspec_node.hpp>

namespace py = pybind11;

namespace {

int node_cmp(const rd::smspec_node &self, py::handle other) {
    if (!py::isinstance<rd::smspec_node>(other))
        throw py::type_error(
            "Other argument must be of type ResdataSMSPECNode");
    return self.cmp(other.cast<const rd::smspec_node &>());
}

} // namespace

PYBIND11_MODULE(rd_smspec_node, m) {
    m.doc() =
        "Small class with some meta information about a summary variable.\n"
        "The summary variables have different attributes, like if they\n"
        "represent a total quantity, a rate or a historical quantity. These\n"
        "quantities, in addition to the underlying values like WGNAMES,\n"
        "KEYWORD and NUMS taken from the the SMSPEC file are stored in this\n"
        "structure.\n";
    py::class_<rd::smspec_node>(m, "ResdataSMSPECNode")
        .def(py::init([]() -> rd::smspec_node * {
            py::set_error(PyExc_NotImplementedError,
                          "Class can not be instantiated directly!");
            throw py::error_already_set();
        }))
        .def("cmp", &node_cmp, py::arg("other"))
        .def("__lt__",
             [](const rd::smspec_node &self, py::handle other) {
                 return node_cmp(self, other) < 0;
             })
        .def("__gt__",
             [](const rd::smspec_node &self, py::handle other) {
                 return node_cmp(self, other) > 0;
             })
        .def("__eq__",
             [](const rd::smspec_node &self, py::handle other) {
                 return node_cmp(self, other) == 0;
             })
        .def("__hash__",
             [](const rd::smspec_node &self) {
                 return py::hash(py::cast(self.get_gen_key1()));
             })
        .def_property_readonly("unit", &rd::smspec_node::get_unit,
                               "Returns the unit of this node as a string.\n")
        .def_property_readonly(
            "wgname", &rd::smspec_node::get_wgname,
            "Returns the WGNAME property for this node.\n"
            "\n"
            "Many variables do not have the WGNAME property, i.e. the field\n"
            "related variables like FOPT and the block properties like\n"
            "BPR:10,10,10. For these variables the function will return\n"
            "None, and not the dummy value: \":+:+:+:+\".\n")
        .def_property_readonly(
            "keyword", &rd::smspec_node::get_keyword,
            "Returns the KEYWORD property for this node.\n"
            "\n"
            "The KEYWORD property is the main classification property in\n"
            "the SMSPEC file. The properties of a variable can be\n"
            "read from the KEYWORD value; see table 3.4 in the ECLIPSE "
            "file\n"
            "format reference manual.\n")
        .def_property_readonly("num",
                               [](const rd::smspec_node &self) -> py::object {
                                   if (self.need_nums())
                                       return py::cast(self.get_num());
                                   return py::none();
                               })
        .def_property_readonly(
            "default", &rd::smspec_node::get_default,
            "Will return the default value for this key.\n"
            "\n"
            "The default value is typically used when fetching values from "
            "a\n"
            "historical case, when the key is only present in the "
            "restarted case.\n"
            "The default value is also used to initialize the PARAMS "
            "vector when\n"
            "writing to file.\n")
        .def("get_key1", &rd::smspec_node::get_gen_key1,
             "Returns the primary composite key, i.e. like 'WOPR:OPX' for "
             "this\n"
             "node.\n")
        .def("get_key2", &rd::smspec_node::get_gen_key2,
             "Returns the secondary composite key for this node.\n"
             "\n"
             "Most variables have only one composite key, but in "
             "particular\n"
             "nodes which involve (i,j,k) coordinates will contain two\n"
             "forms:\n"
             "\n"
             "    get_key1()  =>  \"BPR:10,11,6\"\n"
             "    get_key2()  =>  \"BPR:52423\"\n"
             "\n"
             "Where the '52423' in get_key2() corresponds to i + j*nx +\n"
             "k*nx*ny.\n")
        .def("var_type",
             [](const rd::smspec_node &self) {
                 return SummaryVarType()(static_cast<int>(self.get_var_type()));
             })
        .def(
            "get_num",
            [](const rd::smspec_node &self) -> py::object {
                if (self.need_nums())
                    return py::cast(self.get_num());
                return py::none();
            },
            "Returns the NUMS value for this keyword; or None.\n"
            "\n"
            "Many of the summary keywords have an integer stored in the\n"
            "vector NUMS as an attribute, i.e. the block properties have\n"
            "the global index of the cell in the nums vector. If the\n"
            "variable in question makes use of the NUMS value this "
            "property\n"
            "will return the value, otherwise it will return None:\n"
            "\n"
            "   sum.smspec_node(\"FOPT\").num     => None\n"
            "   sum.smspec_node(\"BPR:1000\").num => 1000\n"
            "\n")
        .def("is_rate", &rd::smspec_node::is_rate,
             "Will check if the variable in question is a rate variable.\n"
             "\n"
             "The conecpt of rate variabel is important (internally) when\n"
             "interpolation values to arbitrary times.\n")
        .def("is_total", &rd::smspec_node::is_total,
             "Will check if the node corresponds to a total quantity.\n"
             "\n"
             "The question of whether a variable corresponds to a "
             "'total'\n"
             "quantity or not can be interesting for e.g. interpolation\n"
             "purposes. The actual question whether a quantity is total "
             "or\n"
             "not is based on a hardcoded list in smspec_node_set_flags() "
             "in\n"
             "smspec_node.c; this list again is based on the tables 2.7 "
             "-\n"
             "2.11 in the ECLIPSE fileformat documentation.\n")
        .def("is_historical", &rd::smspec_node::is_historical,
             "Checks if the key corresponds to a historical variable.\n"
             "\n"
             "The check is only based on the last character; all "
             "variables\n"
             "ending with 'H' are considered historical.\n");
}
