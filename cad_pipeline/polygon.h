#include "boost/geometry.hpp"
#include "boost/geometry/geometries/geometries.hpp"
#include "boost/geometry/geometries/point_xy.hpp"

namespace internal {
using point_t = boost::geometry::model::d2::point_xy<float>;
using polygon_t = boost::geometry::model::polygon<point_t>;
}  // namespace internal

using Polygon = internal::polygon_t;