#include <memory>
#include <functional>
#include <iostream>
#include <array>

using std::shared_ptr;
using std::make_shared;
using std::function;
using std::cout;
using std::array;
using std::bind;

using Bounds = array<int, 2>;

using SpaceBounds = array<Bounds, 2>;
namespace parameters {
  SpaceBounds bounds;
}

using Size = int;
using SpaceSize = array<Size, 2>;

using Position = int;
using Point = array<Position, 2>;

inline Bounds boundsConstrainedToSize(Bounds oldBounds, int size) {
  auto upper = oldBounds[0] + std::min(oldBounds[1] - oldBounds[0], size);
  return {{oldBounds[0], upper}};
}

inline Bounds boundsFromPositionAndSize(int position, int size) {
  return {{position, position + size}};
}

inline bool positionInBounds(int position, Bounds bounds) {
  return position >= bounds[0] && position <= bounds[1];
}

inline bool pointInSpaceBounds(Point point, SpaceBounds bounds) {
  bool inBounds = true;
  for (size_t i = 0; i < 2; ++i) {
    inBounds = inBounds && positionInBounds(point[i], bounds[i]);
  }
  return inBounds;
}

template <typename T> T makeUniform(
    function<void (function<void (shared_ptr<T>)>)> fn) {
  return makeUniformDispatch(static_cast<T*>(nullptr), fn);
}

struct Layout {
  Layout(): getSize([] () {return SpaceSize();}) {}
  Layout(function<SpaceSize ()> _getSize): getSize(_getSize) {}

  function<SpaceSize ()> getSize;
};

inline Layout makeUniformDispatch(Layout*,
    function<void (function<void (shared_ptr<Layout>)>)> /*fn*/) {
  return Layout();
}

struct TouchHandler {
  TouchHandler(): handleTouch([] (int, int) {}) {}
  TouchHandler(function<void (int, int)> _handleTouch): handleTouch(_handleTouch) {}

  function<void (int x, int y)> handleTouch;
};

inline TouchHandler makeUniformDispatch(TouchHandler*,
    function<void (function<void (shared_ptr<TouchHandler>)>)> fn) {
  return TouchHandler([=] (int x, int y) {
    fn(bind(&TouchHandler::handleTouch, std::placeholders::_1, x, y));
  });
}

struct Renderable {
  Renderable(): render([] () {}) {}
  Renderable(function<void ()> _render): render(_render) {}

  function<void ()> render;
};

inline Renderable makeUniformDispatch(Renderable*,
    function<void (function<void (shared_ptr<Renderable>)>)> fn) {
  return Renderable([=] () {
    fn([=] (shared_ptr<Renderable> renderable) {renderable->render();});
  });
}

struct Widget : Layout, TouchHandler, Renderable {
  Widget() {}

  Widget(Layout layout, TouchHandler touchHandler, Renderable renderable):
    Layout(layout), TouchHandler(touchHandler), Renderable(renderable) {
  }
};

struct Void {};

inline Widget makeUniformDispatch(Widget*,
    function<void (function<void (shared_ptr<Widget>)>)> fn) {
  return Widget(
      makeUniform<Layout>(fn),
      makeUniform<TouchHandler>(fn),
      makeUniform<Renderable>(fn));
}

using Action = function<void ()>;
template <typename T> using Class = function<void (shared_ptr<T>& self)>;
template <typename T, typename V> using Program = function<Class<T> (function<Class<T> (V)>)>;

template <typename T> using ValueContinuation = function<void (T)>;
template <typename T> using ValueExpression = function<void (ValueContinuation<T>)>;
template <typename T> using ValueTarget = function<Action (ValueExpression<T>)>;

inline ValueTarget<int> valueTarget(ValueContinuation<int> fn) {
  return [=] (ValueExpression<int> expr) -> Action {
    Action action = [=] () {
      expr(fn);
    };
    return action;
  };
}

template <typename T> ValueExpression<T> literal(T value) {
  return [=] (ValueContinuation<T> cont) {
    cont(value);
  };
}

inline Class<TouchHandler> onTouch(Action onClick) {
  return [=] (shared_ptr<TouchHandler>& self) {
    self = make_shared<TouchHandler>([=] (int x, int y) {
      if (pointInSpaceBounds({{x, y}}, parameters::bounds)) {
        onClick();
      }
    });
  };
}

inline void quad(shared_ptr<Renderable>& self) {
  self = make_shared<Renderable>([] () {
     // ... Render quad
     cout << "Rendering quad\n";
   });
}

inline Class<Widget> withSizeFn(function<SpaceSize ()> sizeFn, Class<Widget> base) {
  return [=] (shared_ptr<Widget>& self) {
    shared_ptr<Widget> _self;
    base(_self);

    _self->getSize = sizeFn;
    self = _self;
  };
}

inline Class<Widget> withSize(SpaceSize size, Class<Widget> base) {
  return withSizeFn([=] () {
      return size;
    }, base);
}

inline Class<Widget> widget(Class<Renderable> renderable, Class<TouchHandler> touchHandler) {
  return ([=] (shared_ptr<Widget>& self) {
    shared_ptr<Widget> _self;
    _self = make_shared<Widget>();

    shared_ptr<Renderable> _renderable;
    renderable(_renderable);
    shared_ptr<TouchHandler> _touchHandler;
    touchHandler(_touchHandler);

    _self->render = _renderable->render;
    _self->handleTouch = _touchHandler->handleTouch;

    self = _self;
  });
}

inline Class<Widget> button(Action onClick) {
  return withSize({{20, 20}},
      widget(quad, onTouch(onClick)));
}

inline Program<Widget, int> yield(
    function<Class<Widget> (ValueTarget<int> escape)> body) {

  return [=] (function<Class<Widget> (int)> cont) -> Class<Widget> {
    return [=] (shared_ptr<Widget>& self) {
      auto target = valueTarget([=, &self] (int value) {
        auto nextClass = cont(value);
        nextClass(self);
      });

      auto cls = body(target);
      cls(self);
    };
  };
}

inline Program<Widget, int> interrupt(
    function<Program<Widget, int> (ValueTarget<int> escape)> body) {

  return [=] (function<Class<Widget> (int)> cont) -> Class<Widget> {
    return [=] (shared_ptr<Widget>& self) {
      auto target = valueTarget([=, &self] (int value) {
        auto nextClass = cont(value);
        nextClass(self);
      });

      auto program = body(target);
      auto cls = program(cont);
      cls(self);
    };
  };
}

inline Class<Widget> runApp(Program<Widget, int> program) {
  return program([] (int value) -> Class<Widget> {
    return [=] (shared_ptr<Widget>& /*self*/) -> void {
      exit(value);
    };
  });
}

inline Class<Widget> layout(Class<Widget> cls) {
  return cls;
}

template <typename... Parameters>
inline Class<Widget> layout(Class<Widget> head, Parameters... tail) {
  return [=] (shared_ptr<Widget>& self) {
    Class<Widget> classes[] {head, layout(tail...)};
    array<shared_ptr<Widget>, 2> items;
    for (size_t i = 0; i < 2; ++i) {
      classes[i](items[i]);
    }

    auto widget = make_shared<Widget>(makeUniform<Widget>([=] (function<void (shared_ptr<Widget>)> message) {
      // TODO: Cache.
      array<SpaceSize, 2> sizes;
      for (size_t i = 0; i < 2; ++i) {
        sizes[i] = items[i]->getSize();
      }

      SpaceBounds parentBounds = parameters::bounds;

      int currentPosition = 0;
      for (size_t i = 0; i < 2; ++i) {
        parameters::bounds[0] = boundsConstrainedToSize(parentBounds[0], sizes[i][0]);
        parameters::bounds[1] = boundsFromPositionAndSize(currentPosition, sizes[i][1]);
        currentPosition += sizes[i][1];

        message(items[i]);
      }

      parameters::bounds = parentBounds;
    }));
    self = widget;
  };
}

auto app = yield([] (ValueTarget<int> finish) {
  return layout(
    button(finish(literal(5))),
    button(finish(literal(3))));});

int main() {
  shared_ptr<Widget> widget;
  runApp(app)(widget);

  parameters::bounds = {{{{0, 100}}, {{0, 100}}}};
  widget->render();
  widget->handleTouch(10, 10);

  return 0;
}
