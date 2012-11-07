;cc
#include <functional>

using std::shared_ptr;
using std::make_shared;
using std::function;

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

template <typename T> using ValueContinuation = function<void (T)>;
template <typename T> using ValueExpression = function<void (ValueContinuation<T>)>;
template <typename T> using ValueTarget = function<Action (ValueExpression<T>)>;

inline ValueTarget<int> valueTarget(ValueContinuation<int> fn) {
  return [&] (ValueExpression<int> expr) -> Action {
    Action action = [&] () {
      expr(fn);
    };
    return action;
  };
}

template <typename T> ValueExpression<T> literal(T value) {
  return [] (ValueContinuation<T> cont) {
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
    }
  };

  self = make_shared<RenderableImpl>();
}

inline Class<Widget> button(Action onClick) {
  return widget(quad, onTouch(onClick));
}

// TODO: break this down.
inline Class<Widget> interrupt(
    function<Class<Widget> (ValueTarget<int> escape)> body,
    function<Class<Widget> (int)> cont) {

  body(valueTarget([=] (int value) {
    auto nextClass = cont(value);
    nextClass(self);
  }));

  //return [=] (shared_ptr<Widget>& self) {
  //  body(valueTarget([=, &self] (int value) {
  //    auto nextClass = cont(value);
  //    nextClass(self);
  //  }));
  //};
}

//auto foo = interrupt([] (ValueTarget<int> finish) {
//  button(finish(literal(3)));
//  });

int main() {
  return 0;
}
