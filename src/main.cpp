#include <iostream>
#include <functional>
#include <memory>
#include <vector>

using namespace std;

namespace detail {
  template <typename... Args> struct Pack {private: Pack();};

  template <> struct Pack<> {
  };

  template <typename T, typename... Rest> struct Pack<T, Rest...> {
    Pack(T const& head_, Pack<Rest...> const& tail_): head(head_), tail(tail_) {}
    T head;
    Pack<Rest...> tail;
  };

  template <typename F, typename... Args>
  void unpackRecurse(F functor, Pack<> const& /*emptyPack*/, Args... args) {
    functor(args...);
  }

  template <typename F, typename T, typename... Rest, typename... Args>
  void unpackRecurse(F functor, Pack<T, Rest...> const& pack, Args... args) {
    detail::unpackRecurse(functor, pack.tail, args..., pack.head);
  }
}

inline detail::Pack<> pack() {
  return detail::Pack<>();
}

template <typename T, typename... Rest> detail::Pack<T, Rest...> pack(T const& head, Rest... tail) {
  return detail::Pack<T, Rest...>(head, pack(tail...));
}

template <typename F, typename... Args>
void unpack(F functor, detail::Pack<Args...> const& pack) {
  return detail::unpackRecurse(functor, pack);
}

template <typename T> struct Expression {
  using Continuation = function<void (T)>;
  using Function = function<void (Continuation)>;

  Function fn;

  template <typename F> Expression(F const& fn_): fn(fn_) {}

  void operator()(Continuation k) const {
    fn(k);
  }
};

template <> struct Expression<void> {
  using Continuation = function<void ()>;
  using Function = function<void (Continuation)>;

  Function fn;

  template <typename F> Expression(F const& fn_): fn(fn_) {}

  void operator()(Continuation k) {
    fn(k);
  }
};

template <typename T> Expression<T> literal(T const& value) {
  return [=] (typename Expression<T>::Continuation k) {
    k(value);
  };
}

inline Expression<string> operator"" _x(const char* str, size_t /*sz*/) {
  return literal(string(str));
}

inline Expression<void> print(Expression<string> value) {
  return [=] (Expression<void>::Continuation k) {
    auto cont = [] (string x) {
      cout << x;
    };
    value(cont);
    k();
  };
}

template <typename T> struct Value {
};

namespace layoutDimensions {
  struct Fixed {
    using Type = Value<float>;
  };

  struct Dimension {
    Dimension(Value<float> pos_, Value<float> size_): pos(pos_), size(size_) {}

    Value<float> pos;
    Value<float> size;
  };

  inline Dimension dimension(Value<float> pos, Value<float> size) {
    return Dimension(pos, size);
  }

  struct Expandable {
    using Type = Dimension;
  };

  inline Value<float>& pos(Dimension& dim) {
    return dim.pos;
  }

  inline Value<float>& size(Dimension& dim) {
    return dim.size;
  }
}

template <typename T> struct Layout {
  layoutDimensions::Dimension dimX;
  layoutDimensions::Dimension dimY;
  T uiElement;

  Layout(layoutDimensions::Dimension dimX_, layoutDimensions::Dimension dimY_, T uiElement_):
    dimX(dimX_), dimY(dimY_), uiElement(uiElement_) {}
};

template <typename T> layoutDimensions::Dimension layoutDimX(Layout<T> const& layout) {
  return layout.dimX;
}

template <typename T> layoutDimensions::Dimension layoutDimY(Layout<T> const& layout) {
  return layout.dimY;
}

template <typename T> T layoutElement(Layout<T> const& layout) {
  return layout.uiElement;
}

template <typename T> Layout<T> layout(
    layoutDimensions::Dimension dimX,
    layoutDimensions::Dimension dimY,
    T uiElement) {
  return Layout<T>(dimX, dimY, uiElement);
}

template <typename DX, typename DY, typename T> struct Layoutable {
  Layoutable(function<Layout<T> (typename DX::Type dimX, typename DY::Type dimY)> const& fn_): fn(fn_) {}
  function<Layout<T> (typename DX::Type dimX, typename DY::Type dimY)> fn;
};

template <typename DX, typename DY, typename T> Layoutable<DX, DY, T> layoutable(
    function<Layout<T> (typename DX::Type dimX, typename DY::Type dimY)> fn) {
  return Layoutable<DX, DY, T>(fn);
}

template <typename DX, typename DY, typename T>
Layout<T> performLayout(Layoutable<DX, DY, T> layoutable, typename DX::Type dimX, typename DX::Type dimY) {
  return layoutable.fn(dimX, dimY);
}

struct Widget_ {
  virtual ~Widget_();
};

template <typename T> Layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed, T> overlay(
    Layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed, T> fixed,
    Layoutable<layoutDimensions::Expandable, layoutDimensions::Expandable, T> expandable) {
  struct OverlayObject : Widget_ { // TODO: Not generic.
    OverlayObject(T fixed_, T expandable_): fixed(fixed_), expandable(expandable_) {}

    T fixed;
    T expandable;
  };

  return layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed>(
      function<Layout<T> (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY)>(
          [=] (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY) -> Layout<T> {
            auto fixedWidget = performLayout(fixed, dimX, dimY);
            auto expandableWidget = performLayout(expandable, layoutDimX(fixedWidget), layoutDimY(fixedWidget));

            // Removing this causes compile error for some reason...
            OverlayObject(layoutElement(fixedWidget), layoutElement(expandableWidget));

            return layout(
                layoutDimX(fixedWidget), layoutDimY(fixedWidget),
                T(make_shared<OverlayObject>(layoutElement(fixedWidget), layoutElement(expandableWidget))));
          }));
}

template <typename T> Layout<T> verticalRecurse(
    layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY,
    Layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed, T> last) {
  return performLayout(last, dimX, dimY);
}

template <typename T, typename... Tail> Layout<T> verticalRecurse(
    layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY,
    Layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed, T> head,
    Tail... tail) {
  struct Pair : T {
    shared_ptr<T> head;
  };

  auto headWidget = performLayout(head, dimX, dimY);
  auto tailWidget = verticalRecurse(dimX, dimY + sizeY(headWidget), tail...);

  return layout(
      layoutDimX(headWidget), // TODO: Max.
      layoutDimensions::dimension(dimY, sizeY(headWidget), sizeY(tailWidget)),
      shared_ptr<T>(make_shared<Pair>(headWidget, tailWidget)));
}

template <typename T, typename... Args> Layoutable<T, layoutDimensions::Fixed, layoutDimensions::Fixed>
  vertical(Layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed, T> head, Args... args) {
  return layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed>(
      function<Layout<shared_ptr<T>> (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY)>(
          [=] (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY) -> Layout<shared_ptr<T>> {
            return verticalRecurse(dimX, dimY, head, args...);
          }));
}

Widget_::~Widget_() {}

template <typename DX, typename DY> using Widget = Layoutable<DX, DY, shared_ptr<Widget_>>;

struct UiManager {
  struct Element : enable_shared_from_this<Element> {
    UiManager& manager;
    Value<float> x;
    Value<float> y;

    Element(UiManager& manager_, Value<float> x_, Value<float> y_):
      manager(manager_), x(x_), y(y_) {}

    virtual ~Element();

    Value<float> getX() {
      return x;
    }

    Value<float> getY() {
      return y;
    }
  };

  using ElementHandle = shared_ptr<Element>;

  vector<ElementHandle> elements;

  struct TextElement : Element {
    string text;
    TextElement(UiManager& manager_, string text_, Value<float> x_, Value<float> y_)
        : Element(manager_, x_, y_), text(text_) {
    }

    virtual ~TextElement();
  };

  ElementHandle makeTextElement(string text, Value<float> posX, Value<float> posY) {
    auto element = make_shared<TextElement>(*this, text, posX, posY);
    elements.push_back(element);
    return element;
  }

 private:
  void removeElement(ElementHandle /*element*/) {
    ((void (*)())0)();
  }
};

UiManager::Element::~Element() {
  manager.removeElement(shared_from_this());
}

UiManager::TextElement::~TextElement() {
}

UiManager uiManager;

using UiElement = UiManager::Element;
using UiElementHandle = UiManager::ElementHandle;

inline Value<float> sizeX(UiElementHandle element) {
  return element->getX();
}

inline Value<float> sizeY(UiElementHandle element) {
  return element->getY();
}

//auto makeUiTextElement = bind(&UiManager::makeTextElement, uiManager);
inline UiElementHandle makeUiTextElement(string text, Value<float> posX, Value<float> posY) {
  return uiManager.makeTextElement(text, posX, posY);
}

struct TouchManager {
  struct Element : enable_shared_from_this<Element> {
    TouchManager& manager;
    Expression<void> action;
    Value<float> x;
    Value<float> w;
    Value<float> y;
    Value<float> h;

    Element(TouchManager& manager_, Expression<void> action_, Value<float> x_, Value<float> w_, Value<float> y_, Value<float> h_):
      manager(manager_), action(action_), x(x_), w(w_), y(y_), h(h_) {}

    virtual ~Element();

    Value<float> getX() {
      return x;
    }

    Value<float> getY() {
      return y;
    }
  };

  using ElementHandle = shared_ptr<Element>;

  vector<ElementHandle> elements;

  ElementHandle makeElement(Expression<void> action, Value<float> posX, Value<float> width, Value<float> posY, Value<float> height) {
    auto element = make_shared<Element>(*this, action, posX, width, posY, height);
    elements.push_back(element);
    return element;
  }

 private:
  void removeElement(ElementHandle /*element*/) {
    ((void (*)())0)();
  }
};

TouchManager::Element::~Element() {
  manager.removeElement(shared_from_this());
}

TouchManager touchManager;

using TouchElement = TouchManager::Element;
using TouchElementHandle = TouchManager::ElementHandle;

//auto makeTouchTextElement = bind(&TouchManager::makeTextElement, uiManager);
inline TouchElementHandle makeTouchElement(Expression<void> action, Value<float> posX, Value<float> width, Value<float> posY, Value<float> height) {
  return touchManager.makeElement(action, posX, width, posY, height);
}

inline Widget<layoutDimensions::Fixed, layoutDimensions::Fixed> label(string caption) {
  struct LabelWidget : Widget_ {
    LabelWidget(UiElementHandle uiElement_): uiElement(uiElement_) {}
    UiElementHandle uiElement;
  };

  return layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed>(
      function<Layout<shared_ptr<Widget_>> (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY)>(
          [=] (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY) -> Layout<shared_ptr<Widget_>> {
            auto uiElement = makeUiTextElement(caption, dimX, dimY);
            return layout(
                layoutDimensions::dimension(dimX, sizeX(uiElement)),
                layoutDimensions::dimension(dimY, sizeY(uiElement)),
                shared_ptr<Widget_>(make_shared<LabelWidget>(uiElement)));
          }));
}

inline Widget<layoutDimensions::Expandable, layoutDimensions::Expandable> hotRegion(Expression<void> action) {
  struct HotRegionWidget : Widget_ {
    HotRegionWidget(TouchElementHandle touchElement_): touchElement(touchElement_) {}
    TouchElementHandle touchElement;
  };

  return layoutable<layoutDimensions::Expandable, layoutDimensions::Expandable>(
      function<Layout<shared_ptr<Widget_>> (layoutDimensions::Expandable::Type dimX, layoutDimensions::Expandable::Type dimY)>(
          [=] (layoutDimensions::Expandable::Type dimX, layoutDimensions::Expandable::Type dimY) -> Layout<shared_ptr<Widget_>> {
            auto touchElement = makeTouchElement(action, pos(dimX), size(dimX), pos(dimY), size(dimY));
            return layout(
                dimX, dimY, shared_ptr<Widget_>(make_shared<HotRegionWidget>(touchElement)));
          }));
}

inline Widget<layoutDimensions::Fixed, layoutDimensions::Fixed> button(string caption, Expression<void> action) {
  return overlay(label(caption), hotRegion(action));
}

auto startNewGame = print("Starting new game.\n"_x);
auto foo = print("Setting foo.\n"_x);

//inline Expression<void> runOptionsMenu(Expression<void> back) {
//  return slidePrompt([=] (function<Expression<void> (Expression<void>)> exitMenu) {
//    return items(
//      button("Back", exitMenu(back)),
//      button("Foo", foo));
//  });
//}
//
//auto runMainMenu = slidePrompt([] (function<Expression<void> (Expression<void>)> exitMenu) {
//    return items(
//      button("New Game", exitMenu(startNewGame)),
//      button("Options", exitMenu(runOptionsMenu)),
//      button("Exit", exitMenu(exitApp)));
// });
//
//auto app = runMainMenu;

int main() {

  return 0;
}
