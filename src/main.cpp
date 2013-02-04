#include <iostream>
#include <functional>
#include <memory>
#include <vector>

using namespace std;

template <typename T> struct Expression {
  using Continuation = function<void (T)>;

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

template <typename DX, typename DY, typename T> struct Layoutable {
};

template <typename T> struct Layout {};

template <typename DX, typename DY, typename T> Layoutable<DX, DY, T> layoutable(
    function<Layout<T> (typename DX::Type dimX, typename DY::Type dimY)> fn) {
  return Layoutable<DX, DY, T>(fn);
}

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

template <typename T> Layout<T> layout(
    layoutDimensions::Dimension dimX,
    layoutDimensions::Dimension dimY,
    shared_ptr<T> uiElement) {
  return Layout<T>(dimX, dimY, uiElement);
}

template <typename T> Layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed, T> overlay(
    Layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed, T> fixed,
    Layoutable<layoutDimensions::Expandable, layoutDimensions::Expandable, T> expandable) {
  struct OverlayObject : T {
    shared_ptr<T> fixed;
    shared_ptr<T> expandable;
  };

  return layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed>(
      function<Layout<shared_ptr<T>> (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY)>(
          [=] (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY) -> Layout<shared_ptr<T>> {
            auto fixedWidget = performLayout(fixed, dimX, dimY);
            auto expandableWidget = performLayout(expandable, layoutDimX(fixedWidget), layoutDimY(fixedWidget));

            return layout(
                layoutDimX(fixedWidget), layoutDimY(fixedWidget),
                shared_ptr<T>(make_shared<OverlayObject>(fixedWidget, expandableWidget)));
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

struct Widget_ {
  virtual ~Widget_() {}
};

template <typename DX, typename DY> using Widget = Layoutable<DX, DY, Widget_>;

struct UiManager {
  struct Element : enable_shared_from_this<Element> {
    UiManager& manager;
    Value<float> x;
    Value<float> y;

    Element(UiManager& manager_, Value<float> x_, Value<float> y_):
      manager(manager_), x(x_), y(y_) {}

    virtual ~Element() {
      manager.removeElement(shared_from_this());
    }

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
    Value<float> x;
    Value<float> w;
    Value<float> y;
    Value<float> h;

    Element(TouchManager& manager_, Value<float> x_, Value<float> w_, Value<float> y_, Value<float> h_):
      manager(manager_), x(x_), w(w_), y(y_), h(h_) {}

    virtual ~Element() {
      manager.removeElement(shared_from_this());
    }

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

TouchManager touchManager;

using TouchElement = TouchManager::Element;
using TouchElementHandle = TouchManager::ElementHandle;

//auto makeTouchTextElement = bind(&TouchManager::makeTextElement, uiManager);
inline TouchElementHandle makeTouchElement(Expression<void> action, Value<float> posX, Value<float> width, Value<float> posY, Value<float> height) {
  return touchManager.makeElement(action, posX, width, posY, height);
}

inline Widget<layoutDimensions::Fixed, layoutDimensions::Fixed> label(string caption) {
  struct LabelWidget : Widget_ {
    UiElementHandle uiElement;
  };

  return layoutable<layoutDimensions::Fixed, layoutDimensions::Fixed>(
      function<Layout<Widget_> (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY)>(
          [=] (layoutDimensions::Fixed::Type dimX, layoutDimensions::Fixed::Type dimY) -> Layout<Widget_> {
            auto uiElement = makeUiTextElement(caption, dimX, dimY);
            return layout(
                layoutDimensions::dimension(dimX, sizeX(uiElement)),
                layoutDimensions::dimension(dimY, sizeY(uiElement)),
                shared_ptr<Widget_>(make_shared<LabelWidget>(uiElement)));
          }));
}

inline Widget<layoutDimensions::Expandable, layoutDimensions::Expandable> hotRegion(Expression<void> action) {
  struct HotRegionWidget : Widget_ {
    TouchElement touchElement;
  };

  return layoutable<layoutDimensions::Expandable, layoutDimensions::Expandable>(
      function<Layout<Widget_> (layoutDimensions::Expandable::Type dimX, layoutDimensions::Expandable::Type dimY)>(
          [=] (layoutDimensions::Expandable::Type dimX, layoutDimensions::Expandable::Type dimY) -> Layout<Widget_> {
            auto touchElement = makeTouchElement(action, pos(dimX), size(dimX), pos(dimY), size(dimY));
            return layout(
                dimX, dimY, shared_ptr<Widget_>(make_shared<HotRegionWidget>(touchElement)));
          }));
}

inline Widget<layoutDimensions::Fixed, layoutDimensions::Fixed> button(string caption, Expression<void> action) {
  return overlay(label(caption), hotRegion(action));
}

auto newGame = print("Starting new game.\n"_x);
auto foo = print("Setting foo.\n"_x);

inline Expression<void> optionsMenu(Expression<void> back) {
  return menu([=] (function<Expression<void> (Expression<void>)> exitMenu) {
    return vertical(
      button("Back", exitMenu(back)),
      button("Foo", foo));
  });
}

auto mainMenu = menu([] (function<Expression<void> (Expression<void>)> exitMenu) {
    return vertical(
      button("New Game", exitMenu(newGame)),
      button("Options", exitMenu(optionsMenu)),
      button("Exit", exitMenu(exitApp)));
 });

auto app = mainMenu;

int main() {
  return 0;
}
