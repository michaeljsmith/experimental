#include <cassert>
#include <functional>

template <typename S> using F = std::function<S>;

template <typename T> struct Ex {};

template <> struct Ex<void> {
  using Cont = F<void ()>;
  using Fn = F<void (Cont)>;
  Fn fn;
  Ex(Fn const& fn_): fn(fn_) {}
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

struct Rect_ {
  Float l;
  Float t;
  Float w;
  Float h;
};
using Rect = Ex<Rect_>;

inline Rect rect(Float /*l*/, Float /*t*/, Float /*w*/, Float /*h*/) {
  assert(0);
}

inline Void display(Rect rect, F<Rect (Void)> genUi) {
  return callCc([=] (Void exit) {
    let(genUi(exit), [=] (Rect ui) {
      return bind(ui, rect,
        waitForever);
    });
  });

  //return [=] (F<void ()> k) {
  //  auto exit = Void([=] () {
  //    cleanup();
  //    k();
  //  });

  //  auto ui = genUi(exit);
  //  auto ui_ = 
  //};
}

inline Rect mainUi(Void /*exit*/) {
  assert(0);
}

auto app = display(rect(lit(0.0f), lit(0.0f), lit(500.0f), lit(500.0f)), mainUi(exit));

int main() {

  evalSync(app);

  return 0;
}
