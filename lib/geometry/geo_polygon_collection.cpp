#include <cstdlib>

#include <map>
#include <string>

#include <ert/util/util.hpp>
#include <ert/util/type_macros.hpp>
#include <ert/util/vector.hpp>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>

#define GEO_POLYGON_COLLECTION_TYPE_ID 95721327

struct geo_polygon_collection_struct {
    UTIL_TYPE_ID_DECLARATION;
    vector_ptr polygon_list = new_vector();
    std::map<std::string, rd::Polygon *> polygon_map;
};

UTIL_IS_INSTANCE_FUNCTION(geo_polygon_collection,
                          GEO_POLYGON_COLLECTION_TYPE_ID)

geo_polygon_collection_type *geo_polygon_collection_alloc() {
    geo_polygon_collection_type *polygons = new geo_polygon_collection_type();
    UTIL_TYPE_ID_INIT(polygons, GEO_POLYGON_COLLECTION_TYPE_ID);
    return polygons;
}

int geo_polygon_collection_size(const geo_polygon_collection_type *polygons) {
    return vector_get_size(polygons->polygon_list.get());
}

rd::Polygon *
geo_polygon_collection_create_polygon(geo_polygon_collection_type *polygons,
                                      const char *name) {
    rd::Polygon *polygon{nullptr};
    bool create_polygon = true;

    if (name && geo_polygon_collection_has_polygon(polygons, name))
        create_polygon = false;

    if (create_polygon) {
        polygon = new rd::Polygon(name);
        geo_polygon_collection_add_polygon(polygons, polygon, true);
    }

    return polygon;
}

bool geo_polygon_collection_add_polygon(geo_polygon_collection_type *polygons,
                                        rd::Polygon *polygon,
                                        bool polygon_owner) {
    const char *name = geo_polygon_get_name(polygon);
    if (geo_polygon_collection_has_polygon(polygons, name))
        return false;
    else {
        if (polygon_owner)
            vector_append_owned_ref(polygons->polygon_list.get(), polygon,
                                    [](void *arg) {delete static_cast<rd::Polygon *>(arg);});
        else
            vector_append_ref(polygons->polygon_list.get(), polygon);

        if (name)
            polygons->polygon_map[name] = polygon;

        return true;
    }
}

bool geo_polygon_collection_has_polygon(
    const geo_polygon_collection_type *polygons, const char *name) {
    if (name)
        return (polygons->polygon_map.count(name) > 0);
    else
        return false;
}

void geo_polygon_collection_free(geo_polygon_collection_type *polygons) {
    delete polygons;
}

rd::Polygon *
geo_polygon_collection_iget_polygon(const geo_polygon_collection_type *polygons,
                                    int index) {
    return (rd::Polygon *)vector_iget(polygons->polygon_list.get(), index);
}

rd::Polygon *
geo_polygon_collection_get_polygon(const geo_polygon_collection_type *polygons,
                                   const char *polygon_name) {
    return polygons->polygon_map.at(polygon_name);
}
