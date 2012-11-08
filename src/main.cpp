#include <memory>
#include <functional>
#include <iostream>

using std::shared_ptr;
using std::make_shared;
using std::function;
using std::cout;

struct TouchHandler {
  virtual ~TouchHandler();
  virtual void handleTouch(int x, int y) = 0;
};

TouchHandler::~TouchHandler() {
}

struct Renderable {
  virtual ~Renderable();
  virtual void render() = 0;
};

Renderable::~Renderable() {
}

struct Widget : public TouchHandler, public Renderable {
  virtual ~Widget();
  virtual void render() = 0;
  virtual void handleTouch(int x, int y) = 0;
};

Widget::~Widget() {
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
  struct _NullRenderable : public Renderable {
    virtual void render() {
    }
  };

  self = make_shared<_NullRenderable>();
}

inline void nullTouchHandler(shared_ptr<TouchHandler>& self) {
  struct NullTouchHandler : public TouchHandler {
    virtual void handleTouch(int /*x*/, int /*y*/) {
    }
  };

  self = make_shared<NullTouchHandler>();
}

inline Class<Widget> widget(
    Class<Renderable> renderable,
    Class<TouchHandler> touchHandler) {

  struct WidgetImpl : public Widget {
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
  struct TouchHandlerImpl : public TouchHandler {
    Action _onClick;

    TouchHandlerImpl(
        Action const& onClick2):
      _onClick(onClick2) {
    }

    virtual void handleTouch(int /*x*/, int /*y*/) {
      // ... Test if in area.

      _onClick();
    }
  };

  return [=] (shared_ptr<TouchHandler>& self) {
    self = make_shared<TouchHandlerImpl>(onClick);
  };
}

inline void quad(shared_ptr<Renderable>& self) {
  struct RenderableImpl : public Renderable {
    virtual void render() {
      // ... Render quad
      cout << "Rendering quad\n";
    }
  };

  self = make_shared<RenderableImpl>();
}

inline Class<Widget> button(Action onClick) {
  return widget(quad, onTouch(onClick));
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

// Program = function<Class<T> (function<Class<T> (V)>)>;

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

auto app = yield([] (ValueTarget<int> finish) -> Class<Widget> {
  return button(finish(literal(3)));
  });

int main() {
  shared_ptr<Widget> widget;
  runApp(app)(widget);

  widget->render();
  widget->render();
  widget->handleTouch(0, 0);

  return 0;
}
