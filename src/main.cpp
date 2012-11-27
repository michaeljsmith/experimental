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
  template <typename F> Expr(F f): fn(f) {}

  void operator()(function<void (T)> k) const {
    fn(k);
  }

  function<void (function<void (T)>)> fn;
};

using Object = function<shared_ptr<Widget> (function<void (shared_ptr<Widget>)>)>;

//callCC f k = f (\a _ -> k a) k
template <typename T> inline Expr<T> callCC(
    function<Expr<T> (function<Expr<Object> (Expr<T>)>)> body) {
  return [=] (function<void (T)> k) {
    body([=] (Expr<T> result) -> Expr<Object> {
      return [=] (function<void (Object)> /*ignoredContinuation*/) {
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

template <typename T, typename V> inline Expr<T> let(
    Expr<V> expression,
    function<Expr<T> (Expr<V>)> body) {
  return [=] (function<void (T)> k) {
    expression([=] (V value) {
      body(literal(value))(k);
    });
  };
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

inline Expr<int> print(char const* text) {
  return [=] (function<void (int)> k) {
    cout << text;
    k(0);
  };
}

function<Expr<Object> (Expr<Object>)> metaContinuation = [] (Expr<Object> /*value*/) {
  return [=] (function<void (Object)> /*k*/) {
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
template <typename T> inline Expr<T> abort(Expr<Object> expression) {
  return [=] (function<void (T)> /*k*/) {
    expression([=] (Object value) {
      metaContinuation(literal(value))([] (Object) {
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
inline Expr<Object> reset(Expr<Object> expression) {
  return let(get(metaContinuation),
      function<Expr<Object> (Expr<function<Expr<Object> (Expr<Object>)>>)>(
        [=] (Expr<function<Expr<Object> (Expr<Object>)>> mc) {
          return callCC<Object>([=] (function<Expr<Object> (Expr<Object>)> k) {
            return sequence(
              set(metaContinuation, literal([=] (Expr<Object> v) -> Expr<Object> {
                return sequence(
                  set(metaContinuation, mc),
                  k(v));
              })),
              abort<Object>(expression));
          });
  }));
}

//(define (*shift f)
// (call-with-current-continuation
//  (lambda (k)
//   (*abort (lambda ()
//            (f (lambda (v)
//                (reset (k v)))))))))
inline Expr<int> shift(function<Expr<Object> (function<Expr<Object> (Expr<int>)>)> body) {
  return callCC<int>([=] (function<Expr<Object> (Expr<int>)> k) {
    return abort<int>(
      body([=] (Expr<int> value) {
        return reset(k(value));
      }));
  });
}

inline Expr<int> printAndReturn(Expr<int> expression) {
  return [=] (function<void (int)> k) {
    expression([=] (int value) {
      cout << "printAndReturn " << value << "\n";
      k(value);
    });
  };
}

inline Expr<int> object(function<shared_ptr<Widget> (function<void (Expr<int>)>)> body) {
  return shift([=] (function<Expr<Object> (Expr<int>)> k) {
    return literal([=] (function<void (shared_ptr<Widget>)> yield) -> shared_ptr<Widget> {
      auto continue_ = [=] (Expr<int> value) {
        k(value)([=] (Object widget) {
          yield(widget([=] (shared_ptr<Widget> newWidget) {
            yield(newWidget);
          }));
        });
      };
      return body(continue_);
    });
  });
}

auto yieldWidget = object([] (function<void (Expr<int>)> continue_) {
  return make_shared<Widget>([=] (int, int) {
    cout << "handleClick 1\n";

    continue_(literal(1));
  });
});

auto app = 
  let(printAndReturn(literal(5)), function<Expr<Object> (Expr<int>)>([=] (Expr<int> value1) {
    return reset(
      let(
        yieldWidget,
        function<Expr<Object> (Expr<int>)>([=] (Expr<int> value2) {
          return sequence(
            printAndReturn(sum(value2, literal(1))),
            printAndReturn(sum(value1, literal(2))),
            printAndReturn(sum(value1, literal(3))),
            literal([=] (function<void (shared_ptr<Widget>)> /*yield*/) {
              return make_shared<Widget>([=] (int, int) {
                cout << "final handleClick\n";
              });
            }));
        })));
  }));

int main() {
  app([=] (Object widget) {
    shared_ptr<Widget> _widget = widget([=, &_widget] (shared_ptr<Widget> newWidget) {
      _widget = newWidget;
    });
    _widget->handleClick(5, 5);
    _widget->handleClick(10, 10);
  });

  return 0;
}
