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

//callCC f k = f (\a _ -> k a) k
template <typename T> inline Expr<T> callCC(
    function<Expr<T> (function<Expr<shared_ptr<Widget>> (Expr<T>)>)> body) {
  return [=] (function<void (T)> k) {
    body([=] (Expr<T> result) -> Expr<shared_ptr<Widget>> {
      return [=] (function<void (shared_ptr<Widget>)> /*ignoredContinuation*/) {
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

function<Expr<shared_ptr<Widget>> (Expr<shared_ptr<Widget>>)> metaContinuation = [] (Expr<shared_ptr<Widget>> /*value*/) {
  return [=] (function<void (shared_ptr<Widget>)> /*k*/) {
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
template <typename T> inline Expr<T> abort(Expr<shared_ptr<Widget>> expression) {
  return [=] (function<void (T)> /*k*/) {
    expression([=] (shared_ptr<Widget> value) {
      metaContinuation(literal(value))([] (shared_ptr<Widget>) {
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
inline Expr<shared_ptr<Widget>> reset(Expr<shared_ptr<Widget>> expression) {
  return let(get(metaContinuation),
      function<Expr<shared_ptr<Widget>> (Expr<function<Expr<shared_ptr<Widget>> (Expr<shared_ptr<Widget>>)>>)>(
        [=] (Expr<function<Expr<shared_ptr<Widget>> (Expr<shared_ptr<Widget>>)>> mc) {
          return callCC<shared_ptr<Widget>>([=] (function<Expr<shared_ptr<Widget>> (Expr<shared_ptr<Widget>>)> k) {
            return sequence(
              set(metaContinuation, literal([=] (Expr<shared_ptr<Widget>> v) -> Expr<shared_ptr<Widget>> {
                return sequence(
                  set(metaContinuation, mc),
                  k(v));
              })),
              abort<shared_ptr<Widget>>(expression));
          });
  }));
}

//(define (*shift f)
// (call-with-current-continuation
//  (lambda (k)
//   (*abort (lambda ()
//            (f (lambda (v)
//                (reset (k v)))))))))
inline Expr<int> shift(function<Expr<shared_ptr<Widget>> (function<Expr<shared_ptr<Widget>> (Expr<int>)>)> body) {
  return callCC<int>([=] (function<Expr<shared_ptr<Widget>> (Expr<int>)> k) {
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

shared_ptr<Widget> nextWidget;

auto app = 
  let(printAndReturn(literal(5)), function<Expr<shared_ptr<Widget>> (Expr<int>)>([=] (Expr<int> value1) {
    return reset(
      let(
        shift([=] (function<Expr<shared_ptr<Widget>> (Expr<int>)> k) {
          return literal(make_shared<Widget>(function<void (int, int)>([=] (int, int) {
            cout << "handleClick 1\n";
            k(literal(1))([=] (shared_ptr<Widget> widget) {
              nextWidget = widget;
            });
          })));
        }),
        function<Expr<shared_ptr<Widget>> (Expr<int>)>([=] (Expr<int> value2) {
          return sequence(
            printAndReturn(sum(value2, literal(1))),
            printAndReturn(sum(value1, literal(2))),
            printAndReturn(sum(value1, literal(3))),
            literal(make_shared<Widget>(function<void (int, int)>([=] (int, int) {
              cout << "final handleClick\n";
            }))));
        })));
  }));

int main() {
  app([=] (shared_ptr<Widget> widget) {
    widget->handleClick(5, 5);
    nextWidget->handleClick(10, 10);
    exit(0);
  });

  return 0;
}
