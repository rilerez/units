#include <assert.h>
#include <boost/hana.hpp>
#include <functional>
#include <type_traits>

namespace units {
namespace hana = boost::hana;
using namespace hana::literals;

#define FN(...) [&](auto _) { return __VA_ARGS__; }

#define MAKE_TAG(name)                                                         \
  namespace types {                                                            \
  class name {};                                                               \
  }                                                                            \
  constexpr auto name = hana::type<types::name>{};

namespace dimension {
namespace tag {
MAKE_TAG(length)
MAKE_TAG(time)
MAKE_TAG(mass)
MAKE_TAG(current)
MAKE_TAG(temperature)
MAKE_TAG(amount_of_substance)
MAKE_TAG(luminous_intensity)
}  // namespace tag

template <class map_t>
struct dimension {
  const map_t map = {};
  constexpr dimension() = default;
  explicit constexpr dimension(map_t map) : map{map} {}
  constexpr static auto is_dimension = true;
};
template <class map1, class map2>
constexpr auto operator==(const dimension<map1>, const dimension<map2>) {
  return map1{} == map2{};
}

constexpr auto make_dimension = [](const auto map) {
  return dimension{hana::to_map(
      hana::remove_if(hana::to_tuple(map), FN(hana::second(_) == 0_c)))};
};

constexpr auto none = make_dimension(hana::make_map());

template <class map1, class map2>
constexpr auto operator+(const dimension<map1> x, const dimension<map2>) {
  static_assert(map1{} == map2{}, "You cannot add different dimensions.");
  return x;
}
template <class map1, class map2>
constexpr auto operator-(const dimension<map1> x, const dimension<map2>) {
  static_assert(map1{} == map2{}, "You cannot subtract different dimensions");
  return x;
}

constexpr auto merge = [](const auto merger, const auto x, const auto y) {
  auto shared_keys = hana::keys(hana::intersection(x, y));
  auto merged_on_common = hana::to_map(
      hana::transform(shared_keys, FN(hana::make_pair(_, merger(x[_], y[_])))));
  return hana::union_(hana::union_(x, y), merged_on_common);
};  // namespace dimension

template <class map1, class map2>
constexpr auto operator*(const dimension<map1> x, const dimension<map2> y) {
  return make_dimension(merge(std::plus{}, x.map, y.map));
}
template <class map1, class map2>
constexpr auto operator/(const dimension<map1> x, const dimension<map2> y) {
  return make_dimension(merge(std::minus{}, x.map, y.map));
}
}  // namespace dimension

namespace unit {

namespace tag {
MAKE_TAG(second)
MAKE_TAG(meter)
MAKE_TAG(kilogram)
MAKE_TAG(ampere)
MAKE_TAG(kelvin)
MAKE_TAG(mole)
MAKE_TAG(candela)
}  // namespace tag
#undef MAKE_TAG

template <class map_t>
struct unitmap {
  map_t map;
  constexpr unitmap() = default;
  constexpr explicit unitmap(const map_t map) : map{map} {}
  static auto constexpr is_unitmap = true;
};
template <class map1, class map2>
constexpr auto operator==(const unitmap<map1>, const unitmap<map2>) {
  return map1{} == map2{};
}
constexpr auto make_unitmap = [](const auto map) { return unitmap{map}; };
constexpr auto union_when_common = [](const auto map1, const auto map2) {
  static_assert(
      hana::intersection(map1, map2) == hana::intersection(map2, map1),
      "Dimensions must be measured in common units.");
  return hana::union_(map1, map2);
};

#define DEFALLOP DEFOP(+) DEFOP(-) DEFOP(*) DEFOP(/)

#define DEFOP(op)                                                              \
  template <class map1, class map2>                                            \
  constexpr auto operator op(const unitmap<map1> x, const unitmap<map2> y) {   \
    return unitmap{union_when_common(x.map, y.map)};                           \
  }
DEFALLOP;
#undef DEFOP

template <class dimension_t, class unitmap_t>
struct unit {
  static_assert(dimension_t::is_dimension);
  static_assert(unitmap_t::is_unitmap);
  dimension_t dimension = {};
  unitmap_t unitmap = {};
  constexpr unit() = default;
  constexpr explicit unit(const dimension_t, const unitmap_t) {}
  constexpr static auto is_unit = true;
};
template <class dimension_t, class unitmap_t>
unit(dimension_t, unitmap_t)->unit<dimension_t, unitmap_t>;

template <class dim1, class dim2, class umap1, class umap2>
constexpr auto operator==(const unit<dim1, umap1> x,
                          const unit<dim2, umap2> y) {
  return (x.dimension == y.dimension) && (x.unitmap == y.unitmap);
}

constexpr auto make_unit = [](auto dimension, auto unitmap) {
  auto cleaned_unitmap =
      // left arg overwrites common values
      make_unitmap(hana::intersection(unitmap.map, dimension.map));
  return unit{dimension, cleaned_unitmap};
};

constexpr auto none =
    make_unit(dimension::none, make_unitmap(hana::make_map()));
}  // namespace unit

#define DEFOP(op)                                                              \
  template <class dim1, class unitmap1, class dim2, class unitmap2>            \
  constexpr auto operator op(const unit::unit<dim1, unitmap1> unit1,           \
                             const unit::unit<dim2, unitmap2> unit2) {         \
    return unit::make_unit(unit1.dimension op unit2.dimension,                 \
                           unit1.unitmap op unit2.unitmap);                    \
  }
DEFALLOP;
#undef DEFOP

template <class unit_t, class number_t>
struct quantity {
  static_assert(unit_t::is_unit);
  number_t number;
  constexpr unit_t unit() const { return {}; }
  constexpr explicit quantity(number_t number) : number{number} {}
  constexpr static auto is_quantity = true;
  template <class unit_t2>
  constexpr quantity(const quantity<unit_t2, number_t>&) {
    static_assert(unit_t2{} == unit_t{}, "Unit mismatch.");
  }
  constexpr explicit quantity(unit_t, number_t number) : quantity(number) {}

  template <class num_t2>
  auto cast() const {
    return quantity<unit_t, num_t2>{static_cast<num_t2>(number)};
  }

  constexpr auto operator==(quantity other) { return number == other.number; }
};
template <class unit_t, class number_t>
quantity(unit_t, number_t)->quantity<unit_t, number_t>;

template <class unit_t, class num_t>
constexpr auto make_quantity(num_t num) {
  return quantity<unit_t, num_t>{num};
}

#define DEFOP(op)                                                              \
  template <class num, class unit1, class unit2>                               \
  auto operator op(const quantity<unit1, num> x,                               \
                   const quantity<unit2, num> y) {                             \
    return make_quantity<decltype(x.unit() op y.unit())>(                      \
        x.number op y.number);                                                 \
  }
DEFALLOP;
#undef DEFOP
#undef DEFALLOP

constexpr auto unit_from_tags = [](const auto dim_tag, const auto unit_tag) {
  return unit::make_unit(
      dimension::make_dimension(hana::make_map(hana::make_pair(dim_tag, 1_c))),
      unit::make_unitmap(hana::make_map(hana::make_pair(dim_tag, unit_tag))));
};

namespace eqns {
auto square = [](const auto x) { return x * x; };
auto force = [](const auto mass, const auto acceleration) {
  return mass * acceleration;
};
auto charge = [](const auto current, const auto time) {
  return current * time;
};
}  // namespace eqns

namespace si {
#define DEF_BASE_UNIT(dim_, unit_)                                             \
  using unit_ =                                                                \
      decltype(unit_from_tags(dimension::tag::dim_, unit::tag::unit_));

DEF_BASE_UNIT(time, second);
DEF_BASE_UNIT(length, meter);
DEF_BASE_UNIT(mass, kilogram);
DEF_BASE_UNIT(current, ampere);
DEF_BASE_UNIT(temperature, kelvin);
DEF_BASE_UNIT(amount_of_substance, mole);
DEF_BASE_UNIT(luminous_intensity, candela);
#undef DEF_BASE_UNIT

using newton =
    decltype(eqns::force(kilogram{}, meter{} / eqns::square(second{})));
using columb = decltype(eqns::charge(ampere{}, second{}));
using none = decltype(second{} / second{});
}  // namespace si

#define DEF_SCALAR_OP(op)                                                      \
  template <class num_t, class unit_t>                                         \
  auto operator op(const quantity<unit_t, num_t> x, const num_t k) {           \
    return x op make_quantity<si::none>(k);                                    \
  }                                                                            \
  template <class num_t, class unit_t>                                         \
  auto operator op(const num_t k, const quantity<unit_t, num_t> x) {           \
    return x op k;                                                             \
  }
DEF_SCALAR_OP(*)
DEF_SCALAR_OP(/)
#undef DEF_SCALAR_OP
}  // namespace units
