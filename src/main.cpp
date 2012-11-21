#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <array>

using std::function;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::cout;
using std::array;

using Bound = int;
using AxisBounds = array<Bound, 2>;
using Bounds = array<AxisBounds, 2>;

using Position = int;
using Point = array<Position, 2>;

inline bool positionInAxisBounds(Position position, AxisBounds axisBounds) {
  return position >= axisBounds[0] &&
    position < axisBounds[1];
}

inline bool pointInBounds(Point point, Bounds bounds) {
  for (size_t axis = 0; axis < 2; ++axis) {
    if (!positionInAxisBounds(point[axis], bounds[axis])) {
      return false;
    }
  }
  return true;
}

struct Widget {
  Widget() {}
  Widget(function<void (int, int)> _onClick): onClick(_onClick) {}
  function<void (int, int)> onClick;
};

inline void app(function<void (Widget)> yield, function<void (int)> finish) {
  function<void (function<void (Widget)>)> menu2 = [=] (function<void (Widget)> k) {
    k(Widget([=] (int, int) {
        finish(3);
    }));
  };

  function<void (function<void (Widget)>)> menu1 = [=] (function<void (Widget)> k) {
    k(Widget([=] (int, int) {
      menu2(yield);
    }));
  };

  menu1(yield);
}

int main() {
  Widget _app;
  app([&_app] (Widget widget) {
      _app = widget;
    }, [] (int result) {
      exit(result);
    });

  _app.onClick(5, 5);
  _app.onClick(5, 5);

  return 0;
}
