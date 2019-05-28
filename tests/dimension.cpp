#include <boost/preprocessor/cat.hpp>
#include "../units.hpp"

#define GENSYM BOOST_PP_CAT(gensym,  __COUNTER__)
namespace hana = boost::hana;
namespace testDimension {
namespace testSum {
  using namespace units::dimension;

auto make_dim_from_tag = [](auto tag) {
  return units::dimension::dimension{hana::make_map(hana::make_pair(tag, 1_c))};
};

  //  (clang++ --std=c++2a dimension.cpp 2>&1) | grep -i "static_assert(m" | grep -i "you cannot add different dimensions"  | wc -l
  //  ===
  // (cat dimension.cpp) | tr "\n" " " | sed "s/.*begin static assert//g"  | sed "s//ok.*//g" | tr ";" "\n" | wc -l
  auto length = make_dim_from_tag(base_dimension::tag::length);
  auto area = length* length;

  auto time = make_dim_from_tag(base_dimension::tag::time);

  auto GENSYM = length + length;
  auto GENSYM = area + area;

  auto GENSYM = (length/length) +(area/area);

  BAD
}  // namespace testSum
}  // namespace testDimension

int main() { return 0; }
