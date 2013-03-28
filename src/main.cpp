#include <cassert>
#include <functional>

template <typename S> using F = std::function<S>;

template <typename T> struct ContOf {typedef void Type(T);};
template <> struct ContOf<void> {typedef void Type();};

template <typename T> struct Ex {
  using Type = T;
  using Cont = typename ContOf<T>::Type;
  using Fn = F<void (Cont)>;
  Fn fn;
  Ex(Fn fn_): fn(fn_) {}
};

using Void = Ex<void>;
using Float = Ex<float>;

template <typename T> T evalSync(Ex<T> /*expr*/) {
  assert(0);
}

auto waitForever = Void([] (F<void ()> /*k*/) {
  // Do nothing.
 });

template <typename T> Ex<T> lit(T const& /*val*/) {
  assert(0);
}

template <typename E> struct Val {
  using T = typename E::Type;
  Ex<T> get;
};

template <typename T> Void track(Var<T>, Val<T>) {
}

struct Rect_ {
  Val<Float> l;
  Val<Float> t;
  Val<Float> w;
  Val<Float> h;
};
using Rect = Ex<Rect_>;

inline Rect fixedRect(Float /*l*/, Float /*t*/, Float /*w*/, Float /*h*/) {
  assert(0);
}

inline Void display(Rect rect, F<Rect (Void)> genUi) {
  return callCc([=] (Void exit) {
    return let(genUi(exit), [=] (Rect ui) {
      return track(ui, rect,
                  waitForever);
    });
  });
}

inline Rect mainUi(Void /*exit*/) {
  assert(0);
}

auto app = display(fixedRect(lit(0.0f), lit(0.0f), lit(500.0f), lit(500.0f)), mainUi(exit));

int main() {

  evalSync(app);

  return 0;
}
