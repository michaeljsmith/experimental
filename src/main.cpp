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

template <typename T> using Expr = function<void (function<void (T)>)>;

//callCC f k = f (\a _ -> k a) k
inline Expr<int> callCC(function<Expr<int> (function<Expr<int> (Expr<int>)>)> body) {
  return [=] (function<void (int)> k) {
    body([=] (Expr<int> result) -> Expr<int> {
      return [=] (function<void (int)> /*ignoredContinuation*/) {
        result(k);
      };
    })(k);
  };
}

auto metaContinuation = function<Expr<int> (Expr<int>)>([] (Expr<int> /*value*/) -> Expr<int> {
  cout << __FILE__ << "(" <<  __LINE__ << "): Missing top-level reset\n";
  exit(1);
  });

inline Expr<int> abort(Expr<int> thunk) {
  return metaContinuation(thunk);
}

inline Expr<int> literal(int value) {
  return [=] (function<void (int)> k) {
    k(value);
  };
}

template <typename T, typename F> inline Expr<T> let(
    Expr<T> expression,
    F body) {
  return [=] (function<void (T)> k) {
    expression([=] (T value) {
      body(literal(value))(k);
    });
  };
}

inline Expr<int> sequence(
    Expr<int> action0,
    Expr<int> action1) {
  return [=] (function<void (int)> k) {
    action0([=] (int) {
      action1(k);
    });
  };
}

inline Expr<int> sum(
    Expr<int> x0,
    Expr<int> x1) {
  return [=] (function<void (int)> k) {
    x0([=] (int _x0) {
      x1([=] (int _x1) {
        k(_x0 + _x1);
      });
    });
  };
}

inline Expr<int> printAndReturn(Expr<int> expression) {
  return [=] (function<void (int)> k) {
    expression([=] (int value) {
      cout << "printAndReturn " << value << "\n";
      k(value);
    });
  };
}

auto app = 
  let(printAndReturn(literal(5)), [=] (Expr<int> value1) {
    return sequence(
      callCC([=] (function<Expr<int> (Expr<int>)> exit) {
        return sequence(
          exit(printAndReturn(sum(value1, literal(1)))),
          printAndReturn(sum(value1, literal(2))));
      }),
      printAndReturn(sum(value1, literal(3))));
  });

int main() {
  app([=] (int result) {
    exit(result);
  });

  return 0;
}
