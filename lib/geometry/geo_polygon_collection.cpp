#include <cstdlib>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ert/util/util.hpp>
#include <ert/util/type_macros.hpp>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>

#define GEO_POLYGON_COLLECTION_TYPE_ID 95721327

struct geo_polygon_collection_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::vector<std::shared_ptr<rd::Polygon>> polygon_list;
    std::map<std::string, std::shared_ptr<rd::Polygon>> polygon_map;
};

UTIL_IS_INSTANCE_FUNCTION(geo_polygon_collection,
                          GEO_POLYGON_COLLECTION_TYPE_ID)

geo_polygon_collection_type *geo_polygon_collection_alloc() {
    geo_polygon_collection_type *polygons = new geo_polygon_collection_type();
    UTIL_TYPE_ID_INIT(polygons, GEO_POLYGON_COLLECTION_TYPE_ID);
    return polygons;
}

size_t
geo_polygon_collection_size(const geo_polygon_collection_type *polygons) {
    return polygons->polygon_list.size();
}

std::shared_ptr<rd::Polygon>
geo_polygon_collection_create_polygon(geo_polygon_collection_type *polygons,
                                      const char *name) {
    std::shared_ptr<rd::Polygon> polygon;

    if (name && geo_polygon_collection_has_polygon(polygons, name))
        return polygon;

    polygon = std::make_shared<rd::Polygon>(
        name ? std::optional<std::string>(name) : std::nullopt);
    geo_polygon_collection_add_polygon(polygons, polygon);

    return polygon;
}

bool geo_polygon_collection_add_polygon(geo_polygon_collection_type *polygons,
                                        std::shared_ptr<rd::Polygon> polygon) {
    auto name = polygon->get_name();
    if (geo_polygon_collection_has_polygon(
            polygons, name.has_value() ? name->c_str() : nullptr))
        return false;
    else {
        polygons->polygon_list.push_back(polygon);

        if (name.has_value())
            polygons->polygon_map[*name] = polygon;

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

std::shared_ptr<rd::Polygon>
geo_polygon_collection_iget_polygon(const geo_polygon_collection_type *polygons,
                                    size_t index) {
    return polygons->polygon_list.at(index);
}

std::shared_ptr<rd::Polygon>
geo_polygon_collection_get_polygon(const geo_polygon_collection_type *polygons,
                                   const std::string &polygon_name) {
    return polygons->polygon_map.at(polygon_name);
}
