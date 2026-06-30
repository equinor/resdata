#include <cstdio>
#include <optional>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/well/well_info.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_subsidence.hpp>
#include <resdata/rd_region.hpp>
#include <resdata/rd_grav.hpp>
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/smspec_node.hpp>
#include <resdata/layer.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/fault_block_layer.hpp>
#include <resdata/well/well_state.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_ts.hpp>

#include <ert/util/double_vector.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/lookup_table.hpp>
#include <ert/util/rng.hpp>
#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>
#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_region.hpp>
#include <ert/geometry/geo_surface.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

using smspec_node_type = rd::smspec_node;

template <typename T> T *cast_cwrap(py::handle obj) {
    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());
    return reinterpret_cast<T *>(pointer);
}

py::object ResdataKW() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.resfile").attr("ResdataKW");
    }
    return cls;
}

template <> rd_kw_type *from_cwrap<rd_kw_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataKW()))
        throw py::type_error("Expected ResdataKW, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_kw_type>(obj);
}

template <> rd_kw_type *from_cwrap(std::optional<py::handle> obj) {
    if (!obj)
        return nullptr;

    if (!py::isinstance(*obj, ResdataKW()))
        throw py::type_error("Expected ResdataKW, got " +
                             static_cast<std::string>(py::repr(*obj)));
    return cast_cwrap<rd_kw_type>(*obj);
}

py::object Grid() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.grid").attr("Grid");
    }
    return cls;
}

template <> rd_grid_type *from_cwrap<rd_grid_type>(py::handle obj) {
    if (!py::isinstance(obj, Grid()))
        throw py::type_error("Expected Grid, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_grid_type>(obj);
}

py::object Summary() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.summary").attr("Summary");
    }
    return cls;
}

template <> rd_sum_type *from_cwrap<rd_sum_type>(py::handle obj) {
    if (!py::isinstance(obj, Summary()))
        throw py::type_error("Expected Summary, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_sum_type>(obj);
}

py::object StringList() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("StringList");
    }
    return cls;
}

template <> stringlist_type *from_cwrap<stringlist_type>(py::handle obj) {
    if (!py::isinstance(obj, StringList()))
        throw py::type_error("Expected StringList, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<stringlist_type>(obj);
}

py::object TimeVector() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("TimeVector");
    }
    return cls;
}

template <> time_t_vector_type *from_cwrap<time_t_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, TimeVector()))
        throw py::type_error("Expected TimeVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<time_t_vector_type>(obj);
}

py::object SummaryKeyWordVector() {
    static py::object cls;
    if (!cls) {
        cls =
            py::module_::import("resdata.summary").attr("SummaryKeyWordVector");
    }
    return cls;
}

template <> rd_sum_vector_type *from_cwrap<rd_sum_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, SummaryKeyWordVector()))
        throw py::type_error("Expected SummaryKeyWordVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_sum_vector_type>(obj);
}

py::object DoubleVector() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("DoubleVector");
    }
    return cls;
}

template <> double_vector_type *from_cwrap<double_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, DoubleVector()))
        throw py::type_error("Expected DoubleVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<double_vector_type>(obj);
}

py::object CFILE() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("cwrap.cfile").attr("CWrapFile");
    }
    return cls;
}

template <> FILE *from_cwrap<FILE>(py::handle obj) {
    if (!py::isinstance(obj, CFILE()))
        throw py::type_error("Expected CFILE, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<FILE>(obj);
}

py::object ResDataType() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata").attr("ResDataType");
    }
    return cls;
}

template <>::rd_data_type *from_cwrap<::rd_data_type>(py::handle obj) {
    if (!py::isinstance(obj, ResDataType()))
        throw py::type_error("Expected ResDataType, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<::rd_data_type>(obj);
}

py::object IntVector() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("IntVector");
    }
    return cls;
}

template <> int_vector_type *from_cwrap<int_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, IntVector()))
        throw py::type_error("Expected IntVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<int_vector_type>(obj);
}

py::object ResdataFile() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.resfile").attr("ResdataFile");
    }
    return cls;
}

template <> rd_file_type *from_cwrap<rd_file_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataFile()))
        throw py::type_error("Expected ResdataFile, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_file_type>(obj);
}

py::object WellInfo() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.well").attr("WellInfo");
    }
    return cls;
}

template <> well_info_type *from_cwrap<well_info_type>(py::handle obj) {
    if (!py::isinstance(obj, WellInfo()))
        throw py::type_error("Expected WellInfo, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<well_info_type>(obj);
}

py::object ResdataFileView() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.resfile").attr("ResdataFileView");
    }
    return cls;
}

template <> rd_file_view_type *from_cwrap<rd_file_view_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataFileView()))
        throw py::type_error("Expected ResdataFileView, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_file_view_type>(obj);
}
py::object ResdataSubsidence() {
    static py::object cls;
    if (!cls) {
        cls =
            py::module_::import("resdata.gravimetry").attr("ResdataSubsidence");
    }
    return cls;
}

template <> rd_subsidence_type *from_cwrap<rd_subsidence_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataSubsidence()))
        throw py::type_error("Expected ResdataSubsidence, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<rd_subsidence_type>(obj);
}

py::object ResdataRegion() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.grid").attr("ResdataRegion");
    }
    return cls;
}

template <> rd_region_type *from_cwrap<rd_region_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataRegion()))
        throw py::type_error("Expected ResdataRegion, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<rd_region_type>(obj);
}

py::object WellState() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.well").attr("WellState");
    }
    return cls;
}

template <> well_state_type *from_cwrap<well_state_type>(py::handle obj) {
    if (!py::isinstance(obj, WellState()))
        throw py::type_error("Expected WellState, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<well_state_type>(obj);
}

py::object WellConnection() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.well").attr("WellConnection");
    }
    return cls;
}

py::object WellSegment() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.well").attr("WellSegment");
    }
    return cls;
}

// -----------------------------------------------------------------
// Class-import helpers and from_cwrap specializations consolidated
// from the per-class *_pybind.cpp modules.
// -----------------------------------------------------------------

template <> well_conn_type *from_cwrap<well_conn_type>(py::handle obj) {
    if (!py::isinstance(obj, WellConnection()))
        throw py::type_error("Expected WellConnection, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<well_conn_type>(obj);
}

template <> well_segment_type *from_cwrap<well_segment_type>(py::handle obj) {
    if (!py::isinstance(obj, WellSegment()))
        throw py::type_error("Expected WellSegment, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<well_segment_type>(obj);
}

py::object WellTimeLine() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.well").attr("WellTimeLine");
    return cls;
}

template <> well_ts_type *from_cwrap<well_ts_type>(py::handle obj) {
    if (!py::isinstance(obj, WellTimeLine()))
        throw py::type_error("Expected WellTimeLine, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<well_ts_type>(obj);
}

py::object RandomNumberGenerator() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.util.util")
                  .attr("RandomNumberGenerator");
    return cls;
}

template <> rng_type *from_cwrap<rng_type>(py::handle obj) {
    if (!py::isinstance(obj, RandomNumberGenerator()))
        throw py::type_error("Expected RandomNumberGenerator, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rng_type>(obj);
}

py::object LookupTable() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.util.util").attr("LookupTable");
    return cls;
}

template <> lookup_table_type *from_cwrap<lookup_table_type>(py::handle obj) {
    if (!py::isinstance(obj, LookupTable()))
        throw py::type_error("Expected LookupTable, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<lookup_table_type>(obj);
}

py::object CPolyline() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.geometry").attr("CPolyline");
    return cls;
}

template <> geo_polygon_type *from_cwrap<geo_polygon_type>(py::handle obj) {
    if (!py::isinstance(obj, CPolyline()))
        throw py::type_error("Expected CPolyline, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<geo_polygon_type>(obj);
}

py::object CPolylineCollection() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.geometry")
                  .attr("CPolylineCollection");
    return cls;
}

template <>
geo_polygon_collection_type *
from_cwrap<geo_polygon_collection_type>(py::handle obj) {
    if (!py::isinstance(obj, CPolylineCollection()))
        throw py::type_error("Expected CPolylineCollection, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<geo_polygon_collection_type>(obj);
}

py::object GeoPointset() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.geometry").attr("GeoPointset");
    return cls;
}

template <> geo_pointset_type *from_cwrap<geo_pointset_type>(py::handle obj) {
    if (!py::isinstance(obj, GeoPointset()))
        throw py::type_error("Expected GeoPointset, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<geo_pointset_type>(obj);
}

py::object GeoRegion() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.geometry").attr("GeoRegion");
    return cls;
}

template <> geo_region_type *from_cwrap<geo_region_type>(py::handle obj) {
    if (!py::isinstance(obj, GeoRegion()))
        throw py::type_error("Expected GeoRegion, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<geo_region_type>(obj);
}

py::object Surface() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.geometry").attr("Surface");
    return cls;
}

template <> geo_surface_type *from_cwrap<geo_surface_type>(py::handle obj) {
    if (!py::isinstance(obj, Surface()))
        throw py::type_error("Expected Surface, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<geo_surface_type>(obj);
}

py::object Layer() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.grid.faults.layer").attr("Layer");
    return cls;
}

template <> layer_type *from_cwrap<layer_type>(py::handle obj) {
    if (!py::isinstance(obj, Layer()))
        throw py::type_error("Expected Layer, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<layer_type>(obj);
}

py::object FaultBlock() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.grid.faults.fault_block")
                  .attr("FaultBlock");
    return cls;
}

template <> fault_block_type *from_cwrap<fault_block_type>(py::handle obj) {
    if (!py::isinstance(obj, FaultBlock()))
        throw py::type_error("Expected FaultBlock, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<fault_block_type>(obj);
}

py::object FaultBlockLayer() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.grid.faults.fault_block_layer")
                  .attr("FaultBlockLayer");
    return cls;
}

template <>
fault_block_layer_type *from_cwrap<fault_block_layer_type>(py::handle obj) {
    if (!py::isinstance(obj, FaultBlockLayer()))
        throw py::type_error("Expected FaultBlockLayer, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<fault_block_layer_type>(obj);
}

py::object ResdataSMSPECNode() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.summary").attr("ResdataSMSPECNode");
    return cls;
}

template <> smspec_node_type *from_cwrap<smspec_node_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataSMSPECNode()))
        throw py::type_error("Expected ResdataSMSPECNode, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<smspec_node_type>(obj);
}

py::object SummaryTStep() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.summary").attr("SummaryTStep");
    return cls;
}

template <> rd_sum_tstep_type *from_cwrap<rd_sum_tstep_type>(py::handle obj) {
    if (!py::isinstance(obj, SummaryTStep()))
        throw py::type_error("Expected SummaryTStep, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_sum_tstep_type>(obj);
}

py::object ResdataGrav() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.gravimetry").attr("ResdataGrav");
    return cls;
}

template <> rd_grav_type *from_cwrap<rd_grav_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataGrav()))
        throw py::type_error("Expected ResdataGrav, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_grav_type>(obj);
}
