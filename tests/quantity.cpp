#include "../units.hpp"
#include <assert.h>

#ifndef BAD
#  define BAD
#endif

using namespace units;

class foot_tag_class {};
auto foot_tag = hana::type<foot_tag_class>{};

void takes_meter_float(quantity<si::meter, float>) {}

int main() {
  auto meter = make_quantity<si::meter>(1);
  auto second = make_quantity<si::second>(1);
  make_quantity<si::meter>(0) * make_quantity<si::second>(1);

  using feet = decltype(unit_from_tags(tag::length, foot_tag));
  auto foot = make_quantity<feet>(1);

  foot* second;

  meter.cast<float>() + make_quantity<si::meter>(0.0f);

  takes_meter_float(meter.cast<float>());

  BAD;

  assert((foot + foot).number == 2);
  assert(((meter + meter) * meter) == 2*meter * meter);

  printf("all runtime tests pass\n");
  return 0;
}
