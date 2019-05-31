#include <boost/hana.hpp>
#include <functional>
#include <type_traits>

namespace units {
namespace hana = boost::hana;
using namespace hana::literals;

/**
 * Defines a lambda that takes _ and returns the wrapped expression
 */
#define FN(...) [&](auto _) { return __VA_ARGS__; }

namespace impl {
/**
 * A Dimension is a product of powers of _base dimensions_. For example,
 * meters per second squared is (meter^1 * second^{-2}). Here meter and
 * second are base dimensions
 */
template <class map_t>
struct dimension {
  const map_t powers = {};
  constexpr dimension() = default;
  explicit constexpr dimension(map_t powers) : powers{powers} {}
  constexpr static auto is_dimension = true;
};
template <class map1, class map2>
constexpr auto operator==(const dimension<map1>, const dimension<map2>) {
  return map1{} == map2{};
}

/**
 * Constructs dimensions performing type inference given a map
 */
constexpr auto make_dimension = [](const auto map) {
  return dimension{hana::to_map(
      hana::remove_if(hana::to_tuple(map), FN(hana::second(_) == 0_c)))};
};

template <class dim_powers1, class dim_powers2>
constexpr auto operator+(const dimension<dim_powers1> x, const dimension<dim_powers2>) {
  static_assert(dim_powers1{} == dim_powers2{}, "You cannot add different dimensions.");
  return x;
}
template <class dim_powers1, class dim_powers2>
constexpr auto operator-(const dimension<dim_powers1> x, const dimension<dim_powers2>) {
  static_assert(dim_powers1{} == dim_powers2{}, "You cannot subtract different dimensions");
  return x;
}

/**
 * Merges hana maps by calling merger on values of common keys of x and y
 */
constexpr auto merge = [](const auto merger, const auto x, const auto y) {
  auto shared_keys = hana::keys(hana::intersection(x, y));
  auto merged_on_common = hana::to_map(
      hana::transform(shared_keys, FN(hana::make_pair(_, merger(x[_], y[_])))));
  // right argument overrides keys for union
  return hana::union_(hana::union_(x, y), merged_on_common);
};  // namespace dimension

template <class dim_powers1, class dim_powers2>
constexpr auto operator*(const dimension<dim_powers1> x, const dimension<dim_powers2> y) {
  return make_dimension(merge(std::plus{}, x.powers, y.powers));
}
template <class dim_powers1, class dim_powers2>
constexpr auto operator/(const dimension<dim_powers1> x, const dimension<dim_powers2> y) {
  return make_dimension(merge(std::minus{}, x.powers, y.powers));
}

template <class map_t>
struct dim2unit {
  map_t map;
  constexpr dim2unit() = default;
  constexpr explicit dim2unit(const map_t map) : map{map} {}
  static auto constexpr is_dim2unit = true;
};
template <class map1, class map2>
constexpr auto operator==(const dim2unit<map1>, const dim2unit<map2>) {
  return map1{} == map2{};
}
constexpr auto make_dim2unit = FN(dim2unit{_});
constexpr auto union_when_common = [](const auto map1, const auto map2) {
  static_assert(
      hana::intersection(map1, map2) == hana::intersection(map2, map1),
      "Dimensions must be measured in common units.");
  return hana::union_(map1, map2);
};

#define DEFALLOP DEFOP(+) DEFOP(-) DEFOP(*) DEFOP(/)

#define DEFOP(op)                                                              \
  template <class map1, class map2>                                            \
  constexpr auto operator op(const dim2unit<map1> x, const dim2unit<map2> y) { \
    return impl::dim2unit{union_when_common(x.map, y.map)};                    \
  }
DEFALLOP;
#undef DEFOP

template <class dimension_t, class dim2unit_t>
struct unit {
  static_assert(dimension_t::is_dimension);
  static_assert(dim2unit_t::is_dim2unit);
  dimension_t dimension = {};
  dim2unit_t dim2unit = {};
  constexpr unit() = default;
  constexpr explicit unit(const dimension_t, const dim2unit_t) {}
  constexpr static auto is_unit = true;
};
template <class dimension_t, class dim2unit_t>
unit(dimension_t, dim2unit_t)->unit<dimension_t, dim2unit_t>;

template <class dim1, class dim2, class umap1, class umap2>
constexpr auto operator==(const unit<dim1, umap1> x,
                          const unit<dim2, umap2> y) {
  return (x.dimension == y.dimension) && (x.dim2unit == y.dim2unit);
}

constexpr auto make_unit = [](auto dimension, auto dim2unit) {
  auto cleaned_dim2unit =
      // left arg overwrites common values
      make_dim2unit(hana::intersection(dim2unit.map, dimension.powers));
  return unit{dimension, cleaned_dim2unit};
};

#define DEFOP(op)                                                              \
  template <class dim1, class dim2unit1, class dim2, class dim2unit2>          \
  constexpr auto operator op(const unit<dim1, dim2unit1> unit1,                \
                             const unit<dim2, dim2unit2> unit2) {              \
    return make_unit(unit1.dimension op unit2.dimension,                       \
                     unit1.dim2unit op unit2.dim2unit);                        \
  }
DEFALLOP;
#undef DEFOP
};  // namespace impl

/**
 * A quantity wraps a numeric type with strongly typed information about its
 * dimension. Operators +,-,*,/ are overloaded to ensure dimensions and numeric
 * types match.
 */
template <class unit_t, class number_t>
struct quantity {
  static_assert(unit_t::is_unit);
  number_t number;
  constexpr unit_t unit() const { return {}; }
  constexpr explicit quantity(number_t number) : number{number} {}
  constexpr static auto is_quantity = true;

  constexpr quantity(const quantity<unit_t2, number_t>&) {
    static_assert(unit_t2{} == unit_t{}, "Unit mismatch.");
  }
  constexpr explicit quantity(unit_t, number_t number) : quantity(number) {}

  /**
   * Casts the numeric part of the quantity:
   * (make_quantity<si::meter>(3)).cast<float>() ====
   * make_quantity<si::meter>(3.0f)
   */
  template <class num_t2>
  auto cast() const {
    return quantity<unit_t, num_t2>{static_cast<num_t2>(number)};
  }

  constexpr auto operator==(quantity other) { return number == other.number; }
};
template <class unit_t, class number_t>
quantity(unit_t, number_t)->quantity<unit_t, number_t>;

/**
 * Constructs a quantity given its unit as a template argument and the number
 * as its function argument.
 */
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

/**
 * Makes an instance of a unit given a dimension tag and a unit tag. Tags must
 * be hashable by boost::hana
 */
constexpr auto unit_from_tags = [](const auto dim_tag, const auto unit_tag) {
  return impl::make_unit(
      impl::make_dimension(hana::make_map(hana::make_pair(dim_tag, 1_c))),
      impl::make_dim2unit(hana::make_map(hana::make_pair(dim_tag, unit_tag))));
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

/**
 * Tags are a way to encode a symbol in a way boost::hana can understand and
 * hash. We define a class, and then take the hana::type of it. This way we
 * can ensure two different tags are not equal and we can use them as keys
 *
 * This macro defines tags given their name.
 * WARNING - it pollutes a nested namespace called types.
 */
#define MAKE_TAG(name)                                                         \
  namespace types {                                                            \
  class name {};                                                               \
  }                                                                            \
  constexpr auto name = hana::type<types::name>{};

namespace tag {
// namespace types { class length {}; }
// constexpr auto length = hana::type<types::length>{};
MAKE_TAG(length)
MAKE_TAG(time)
MAKE_TAG(mass)
MAKE_TAG(current)
MAKE_TAG(temperature)
MAKE_TAG(amount_of_substance)
MAKE_TAG(luminous_intensity)
}  // namespace tag

namespace si {
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

#define DEF_BASE_UNIT(dim_, unit_)                                             \
  using unit_ = decltype(unit_from_tags(units::tag::dim_, si::tag::unit_));

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
#undef MAKE_TAG
}  // namespace units
