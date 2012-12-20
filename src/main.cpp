#include <boost/function.hpp>
#include <memory>
#include <array>
#include <iostream>

using boost::function;
using std::shared_ptr;
using std::make_shared;
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
  Widget(function<void (int, int)> _handleClick): handleClick(_handleClick) {}
  function<void (int, int)> handleClick;
};

template <typename T> struct Expr {
  typedef T Raw;
  typedef function<void (Raw)> Continuation;

  template <typename F> Expr(F f): fn(f) {}

  void operator()(function<void (T)> k) const {
    fn(k);
  }

  function<void (function<void (T)>)> fn;
};

typedef Expr<int> Int;

using Object = Expr<function<shared_ptr<Widget> (function<void (shared_ptr<Widget>)>)>>;

//callCC f k = f (\a _ -> k a) k
template <typename T> inline T callCC(function<T (function<Object (T)>)> body) {
  return [=] (typename T::Continuation k) {
    body([=] (T result) -> Object {
      return [=] (Object::Continuation /*ignoredContinuation*/) {
        result(k);
      };
    })(k);
  };
}

template <typename T> inline Expr<T> literal(T value) {
  return [=] (function<void (T)> k) {
    k(value);
  };
}

namespace detail {
  template <typename T, typename V> inline T letHelper(
      V expression,
      function<T (V)> body) {
    return [=] (typename T::Continuation k) {
      expression([=] (typename V::Raw value) {
        body(literal(value))(k);
      });
    };
  }
}

template <typename X0, typename X1> inline auto let(X0 expression, X1 body) -> decltype(body(expression)) {
  return detail::letHelper(expression, function<decltype(body(expression)) (X0)>(body));
}

template <typename T> inline T valueNull(Expr<T>) {exit(1);}

template <typename T> Expr<T> sequence(Expr<T> expr) {
  return expr;
}

template <typename T, typename... Rest> auto sequence(Expr<T> head, Rest... rest) -> decltype(sequence(rest...)) {
  return [=] (function<void (decltype(valueNull(sequence(rest...))))> k) {
    head([=] (T) {
      sequence(rest...)(k);
    });
  };
}

inline Int sum(
    Int x0,
    Int x1) {
  return [=] (Int::Continuation k) {
    x0([=] (int _x0) {
      x1([=] (int _x1) {
        k(_x0 + _x1);
      });
    });
  };
}

inline Int print(char const* text) {
  return [=] (Int::Continuation k) {
    cout << text;
    k(0);
  };
}

function<Object (Object)> metaContinuation = [] (Object /*value*/) {
  return [=] (Object::Continuation /*k*/) {
    cout << __FILE__ << "(" <<  __LINE__ << "): Missing top-level reset\n";
    exit(1);
  };
  };

template <typename T> inline Expr<T> get(T const& var) {
  return [&var] (function<void (T)> k) {
    k(var);
  };
}

template <typename T, typename V> inline Expr<T> set(T& var, Expr<V> value) {
  return [=, &var] (function<void (T)> k) {
    value([=, &var] (V _value) {
      var = _value;
      k(var);
    });
  };
}

//(define (*abort thunk)
//  (let ((v (thunk)))
//    (*meta-continuation* v)))
template <typename T> inline Expr<T> abort(Object expression) {
  return [=] (function<void (T)> /*k*/) {
    expression([=] (Object::Raw value) {
      metaContinuation(literal(value))([] (Object::Raw) {
        cout << __FILE__ << "(" <<  __LINE__ <<
          "): metaContinuation returned via original continuation.\n";
      });
    });
  };
}

//(define (*reset thunk)
//  (let ((mc *meta-continuation*))
//    (call-with-current-continuation
//      (lambda (k)
//        (begin
//          (set! *meta-continuation*
//            (lambda (v)
//              (set! *meta-continuation* mc)
//              (k v)))
//          (*abort thunk))))))
inline Object reset(Object expression) {
  return let(get(metaContinuation), [=] (Expr<function<Object (Object)>> mc) {
    return callCC<Object>([=] (function<Object (Object)> k) {
      return sequence(
          set(metaContinuation, literal([=] (Object v) -> Object {
            return sequence(
                set(metaContinuation, mc),
                k(v));
          })),
          abort<Object::Raw>(expression));
    });
  });
}

//(define (*shift f)
// (call-with-current-continuation
//  (lambda (k)
//   (*abort (lambda ()
//            (f (lambda (v)
//                (reset (k v)))))))))
inline Int shift(function<Object (function<Object (Int)>)> body) {
  return callCC<Int>([=] (function<Object (Int)> k) {
    return abort<int>(
      body([=] (Int value) {
        return reset(k(value));
      }));
  });
}

inline Int printAndReturn(Int expression) {
  return [=] (Int::Continuation k) {
    expression([=] (int value) {
      cout << "printAndReturn " << value << "\n";
      k(value);
    });
  };
}

inline Int object(function<Expr<shared_ptr<Widget>> (function<Int (Int)>)> body) {
  return shift([=] (function<Object (Int)> k) {
    return literal([=] (function<void (shared_ptr<Widget>)> yield) -> shared_ptr<Widget> {
      auto continue_ = [=] (Int value) {
        return Int([=] (Int::Continuation /*neverExecuted*/) {
          k(value)([=] (Object::Raw widget) {
            yield(widget([=] (shared_ptr<Widget> newWidget) {
              yield(newWidget);
            }));
          });
        });
      };

      shared_ptr<Widget> result;
      body(continue_)([&result] (shared_ptr<Widget> widget) {
        result = widget;
      });

      return result;
    });
  });
}

inline Expr<shared_ptr<Widget>> makeSharedWidget(Expr<function<void (int, int)>> handleClick) {
  return [=] (function<void (shared_ptr<Widget>)> k) {
    handleClick([=] (function<void (int, int)> _handleClick) {
      k(make_shared<Widget>(_handleClick));
    });
  };
}

inline Expr<function<void (int, int)>> lambda(function<Int (Int x0, Int x1)> body) {
  return [=] (function<void (function<void (int, int)>)> k) {
    k([=] (int _x0, int _x1) {
      body(literal(_x0), literal(_x1))([] (int) {});
    });
  };
}

auto yieldWidget = object([] (function<Int (Int)> continue_) {
  return makeSharedWidget(
    lambda([=] (Int, Int) {
      return sequence(
        print("handleClick 1\n"),
        continue_(literal(1)));
    }));
 });

auto app =
  let(printAndReturn(literal(5)), [=] (Int value1) {
    return reset(
      let(yieldWidget, [=] (Int value2) {
        return sequence(
            printAndReturn(sum(value2, literal(1))),
            printAndReturn(sum(value1, literal(2))),
            printAndReturn(sum(value1, literal(3))),
            literal([=] (function<void (shared_ptr<Widget>)> /*yield*/) {
              return make_shared<Widget>([=] (int, int) {
                cout << "final handleClick\n";
              });
            }));
      }));
  });

int main() {
  app([=] (Object::Raw widget) {
    shared_ptr<Widget> _widget = widget([=, &_widget] (shared_ptr<Widget> newWidget) {
      _widget = newWidget;
    });
    _widget->handleClick(5, 5);
    _widget->handleClick(10, 10);
  });

  return 0;
}
