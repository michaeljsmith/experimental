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

template <typename T> struct Wrapper {};

struct Layout {
  virtual ~Layout();

  virtual SpaceSize getSize() {return SpaceSize();}
};

Layout::~Layout() {
}

template <> struct Wrapper<Layout> : Layout {
  shared_ptr<Layout> _base;
  Wrapper(shared_ptr<Layout> base): _base(base) {}

  virtual SpaceSize getSize();
};

SpaceSize Wrapper<Layout>::getSize() {
  return _base->getSize();
}

struct TouchHandler {
  virtual ~TouchHandler();
  virtual void handleTouch(int x, int y) = 0;
};

TouchHandler::~TouchHandler() {
}

template <> struct Wrapper<TouchHandler> : TouchHandler {
  shared_ptr<TouchHandler> _base;
  Wrapper(shared_ptr<TouchHandler> base): _base(base) {}

  virtual void handleTouch(int x, int y);
};

void Wrapper<TouchHandler>::handleTouch(int x, int y) {
  _base->handleTouch(x, y);
}

struct Renderable {
  virtual ~Renderable();
  virtual void render() = 0;
};

Renderable::~Renderable() {
}

template <> struct Wrapper<Renderable> : Renderable {
  shared_ptr<Renderable> _base;
  Wrapper(shared_ptr<Renderable> base): _base(base) {}

  virtual void render();
};

void Wrapper<Renderable>::render() {
  _base->render();
}

struct Widget : Layout, TouchHandler, Renderable {
  virtual ~Widget();
};

Widget::~Widget() {
}

template <> struct Wrapper<Widget> : Widget, Wrapper<Layout>, Wrapper<TouchHandler>, Wrapper<Renderable> {
  Wrapper(shared_ptr<Widget> base):
    Wrapper<Layout>(base), Wrapper<TouchHandler>(base), Wrapper<Renderable>(base) {
  }

  virtual SpaceSize getSize();
  virtual void handleTouch(int x, int y) {Wrapper<TouchHandler>::handleTouch(x, y);}
  virtual void render() {Wrapper<Renderable>::render();}
};

SpaceSize Wrapper<Widget>::getSize() {
  return Wrapper<Layout>::getSize();
}

inline shared_ptr<Layout> makeLayout(function<SpaceSize ()> onGetSize) {
  struct Impl : Layout {
    function<SpaceSize ()> _onGetSize;

    Impl(function<SpaceSize ()> onGetSize2): _onGetSize(onGetSize2) {}

    virtual SpaceSize getSize() {
      return _onGetSize();
    }
  };

  return make_shared<Impl>(onGetSize);
}

inline shared_ptr<TouchHandler> makeTouchHandler(function<void (int, int)> onHandleTouch) {
  struct Impl : TouchHandler {
    function<void (int, int)> _onHandleTouch;

    Impl(function<void (int, int)> onHandleTouch2): _onHandleTouch(onHandleTouch2) {}

    virtual void handleTouch(int x, int y) {
      _onHandleTouch(x, y);
    }
  };

  return make_shared<Impl>(onHandleTouch);
}

inline shared_ptr<Renderable> makeRenderable(function<void ()> onRender) {
  struct Impl : Renderable {
    function<void ()> _onRender;

    Impl(function<void ()> onRender2): _onRender(onRender2) {}

    virtual void render() {
      _onRender();
    }
  };

  return make_shared<Impl>(onRender);
}

inline void initializeNullImpl(shared_ptr<Widget>& instance) {
  struct NullImpl : Widget {
    virtual SpaceSize getSize() {
      return SpaceSize();
    }

    virtual void handleTouch(int /*x*/, int /*y*/) {
    }

    virtual void render() {
    }
  };

  instance = make_shared<NullImpl>();
}

template <typename T> shared_ptr<T> makeNullImpl() {
  shared_ptr<T> instance;
  initializeNullImpl(instance);
  return instance;
}

inline shared_ptr<Widget> override(shared_ptr<Widget> base, shared_ptr<Layout> layout) {
  struct Override : Wrapper<Widget> {
    shared_ptr<Layout> _layout;

    Override(shared_ptr<Layout> layout2, shared_ptr<Widget> base2):
      Wrapper<Widget>(base2), _layout(layout2) {
    }

    virtual SpaceSize getSize() {
      return _layout->getSize();
    }
  };

  return make_shared<Override>(layout, base);
}

inline shared_ptr<Widget> override(shared_ptr<Widget> base, shared_ptr<TouchHandler> touchHandler) {
  struct Override : Wrapper<Widget> {
    shared_ptr<TouchHandler> _touchHandler;

    Override(shared_ptr<TouchHandler> touchHandler2, shared_ptr<Widget> base2):
      Wrapper<Widget>(base2), _touchHandler(touchHandler2) {
    }

    virtual void handleTouch(int x, int y) {
      _touchHandler->handleTouch(x, y);
    }
  };

  return make_shared<Override>(touchHandler, base);
}

inline shared_ptr<Widget> override(shared_ptr<Widget> base, shared_ptr<Renderable> renderable) {
  struct Override : Wrapper<Widget> {
    shared_ptr<Renderable> _renderable;

    Override(shared_ptr<Renderable> renderable2, shared_ptr<Widget> base2):
      Wrapper<Widget>(base2), _renderable(renderable2) {
    }

    virtual void render() {
      _renderable->render();
    }
  };

  return make_shared<Override>(renderable, base);
}

inline shared_ptr<Widget> makeUniform(function<void (function<void (shared_ptr<Widget>)> message)> handleMessage) {
  return override(
      override(makeNullImpl<Widget>(),
        makeTouchHandler(
          [=] (int x, int y) {
            handleMessage(bind(&TouchHandler::handleTouch, std::placeholders::_1, x, y));
          })),
      makeRenderable(
        [=] () {
          handleMessage(bind(&Renderable::render, std::placeholders::_1));
        }));
}

struct Void {};

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

inline void nullRenderable(shared_ptr<Renderable>& self) {
  struct _NullRenderable : Renderable {
    virtual void render() {
    }
  };

  self = make_shared<_NullRenderable>();
}

inline void nullTouchHandler(shared_ptr<TouchHandler>& self) {
  struct NullTouchHandler : TouchHandler {
    virtual void handleTouch(int /*x*/, int /*y*/) {
    }
  };

  self = make_shared<NullTouchHandler>();
}

inline Class<Widget> widget(
    Class<Renderable> renderable,
    Class<TouchHandler> touchHandler) {

  struct WidgetImpl : Widget {
    shared_ptr<Renderable> _renderable;
    shared_ptr<TouchHandler> _touchHandler;

    virtual void render() {
      _renderable->render();
    }

    virtual void handleTouch(int x, int y) {
      _touchHandler->handleTouch(x, y);
    }
  };

  return [=] (shared_ptr<Widget>& self) {
    shared_ptr<Renderable> renderableInstance;
    renderable(renderableInstance);

    shared_ptr<TouchHandler> touchHandlerInstance;
    touchHandler(touchHandlerInstance);

    auto widget = make_shared<WidgetImpl>();
    widget->_renderable = renderableInstance;
    widget->_touchHandler = touchHandlerInstance;
    self = widget;
  };
}

inline Class<TouchHandler> onTouch(Action onClick) {
  struct TouchHandlerImpl : TouchHandler {
    Action _onClick;

    TouchHandlerImpl(
        Action const& onClick2):
      _onClick(onClick2) {
    }

    virtual void handleTouch(int x, int y) {
      if (pointInSpaceBounds({{x, y}}, parameters::bounds)) {
        _onClick();
      }
    }
  };

  return [=] (shared_ptr<TouchHandler>& self) {
    self = make_shared<TouchHandlerImpl>(onClick);
  };
}

inline void quad(shared_ptr<Renderable>& self) {
  struct RenderableImpl : Renderable {
    virtual void render() {
      // ... Render quad
      cout << "Rendering quad\n";
    }
  };

  self = make_shared<RenderableImpl>();
}

inline Class<Widget> withSizeFn(function<SpaceSize ()> sizeFn, Class<Widget> base) {
  return [=] (shared_ptr<Widget>& self) {
    shared_ptr<Widget> _base;
    base(_base);

    self = override(_base, makeLayout(sizeFn));
  };
}

inline Class<Widget> withSize(SpaceSize size, Class<Widget> base) {
  return withSizeFn([=] () {
      return size;
    }, base);
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

    auto widget = makeUniform([=] (function<void (shared_ptr<Widget>)> message) {
      // TODO: Cache.
      array<SpaceSize, 2> sizes;
      std::transform(items.begin(), items.end(), sizes.begin(), std::bind(&Layout::getSize, std::placeholders::_1));

      SpaceBounds parentBounds = parameters::bounds;

      int currentPosition = 0;
      for (size_t i = 0; i < 2; ++i) {
        parameters::bounds[0] = boundsConstrainedToSize(parentBounds[0], sizes[i][0]);
        parameters::bounds[1] = boundsFromPositionAndSize(currentPosition, sizes[i][1]);
        currentPosition += sizes[i][1];

        message(items[i]);
      }

      parameters::bounds = parentBounds;
    });
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
