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

using Expression = function<void (function<void (int)>)>;

//callCC f k = f (\a _ -> k a) k
inline Expression callCC(function<Expression (function<Expression (Expression)>)> body) {
  return [=] (function<void (int)> k) {
    body([=] (Expression result) -> Expression {
      return [=] (function<void (int)> /*ignoredContinuation*/) {
        result(k);
      };
    })(k);
  };
}

inline Expression literal(int value) {
  return [=] (function<void (int)> k) {
    k(value);
  };
}

inline Expression let(
    Expression expression,
    function<Expression (Expression)> body) {
  return [=] (function<void (int)> k) {
    expression([=] (int value) {
      body(literal(value))(k);
    });
  };
}

inline Expression sequence(
    Expression action0,
    Expression action1) {
  return [=] (function<void (int)> k) {
    action0([=] (int) {
      action1(k);
    });
  };
}

inline Expression sum(
    Expression x0,
    Expression x1) {
  return [=] (function<void (int)> k) {
    x0([=] (int _x0) {
      x1([=] (int _x1) {
        k(_x0 + _x1);
      });
    });
  };
}

inline Expression printAndReturn(Expression expression) {
  return [=] (function<void (int)> k) {
    expression([=] (int value) {
      cout << "printAndReturn " << value << "\n";
      k(value);
    });
  };
}

auto app = 
  let(printAndReturn(literal(5)), [=] (Expression value1) {
    return sequence(
      callCC([=] (function<Expression (Expression)> exit) {
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
