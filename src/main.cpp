#include <tuple>
#include <functional>

using std::tuple;
using std::function;

struct Void {};

template <typename T> struct Value {};

template <typename T, typename R0, typename R1, typename D0, typename D1, typename B0, typename B1> struct LayoutableImpl {
  tuple<R0, R1> dim;
  function<tuple<T, tuple<D0, D1>> (tuple<B0, B1>)> gen;
};

template <typename X> struct DimensionOf {};
template <typename X> struct BoundsOf {};

template <typename T, typename R0, typename R1> using Layoutable = LayoutableImpl<T, R0, R1,
         typename DimensionOf<R0>::type,
         typename DimensionOf<R1>::type,
         typename BoundsOf<R0>::type,
         typename BoundsOf<R1>::type>;

struct AtLeastLayoutAxis {
  AtLeastLayoutAxis(float min_): min(min_) {}
  float min;
};

template <> struct DimensionOf<AtLeastLayoutAxis> {using type = Void;};
template <> struct BoundsOf<AtLeastLayoutAxis> {using type = tuple<Value<float>, Value<float>>;};

template <typename T, typename... Tail>
Layoutable<T, AtLeastLayoutAxis, AtLeastLayoutAxis> vertical(
    Layoutable<T, AtLeastLayoutAxis, AtLeastLayoutAxis> head,
    Tail... tail) {
  float minW = minWidth(head, tail...);
  float minH = exactHeight(tail...);
  return Layoutable<T, AtLeastLayoutAxis, AtLeastLayoutAxis>(
      make_tuple(minW, minH),
      [=] (tuple<tuple<Value<float>, Value<float>>, tuple<Value<float>, Value<float>>> dim) {
        return asdf;
      });
}

int main() {
  return 0;
}
