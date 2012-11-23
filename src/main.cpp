#include <boost/function.hpp>
#include <array>
#include <iostream>

using boost::function;
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
  function<void (int, int)> handleClick;
};

inline function<void (function<void (int)>)> literal(int value) {
  return [=] (function<void (int)> k) {
    k(value);
  };
}

inline function<void (function<void (int)>)> let(
    function<void (function<void (int)>)> expression,
    function<function<void (function<void (int)>)>(function<void (function<void (int)>)>)> body) {
  return [=] (function<void (int)> k) {
    expression([=] (int value) {
      body(literal(value))(k);
    });
  };
}

inline function<void (function<void (int)>)> sequence(
    function<void (function<void (int)>)> action0,
    function<void (function<void (int)>)> action1) {
  return [=] (function<void (int)> k) {
    action0([=] (int) {
      action1(k);
    });
  };
}

inline function<void (function<void (int)>)> sum(
    function<void (function<void (int)>)> x0,
    function<void (function<void (int)>)> x1) {
  return [=] (function<void (int)> k) {
    x0([=] (int _x0) {
      x1([=] (int _x1) {
        k(_x0 + _x1);
      });
    });
  };
}

inline function<void (function<void (int)>)> printAndReturn(function<void (function<void (int)>)> expression) {
  return [=] (function<void (int)> k) {
    expression([=] (int value) {
      cout << "printAndReturn " << value << "\n";
      k(value);
    });
  };
}

auto app = 
  let(printAndReturn(literal(5)), [=] (function<void (function<void (int)>)> value1) {
    return sequence(
        printAndReturn(sum(value1, literal(1))),
        printAndReturn(sum(value1, literal(2))));
  });

int main() {
  app([=] (int result) {
    exit(result);
  });

  return 0;
}
