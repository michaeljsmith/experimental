#include <memory>
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

// TODO: Specialize Class for void type.
// TODO: Special version for non-returning expressions?
inline void nullRenderable(shared_ptr<Renderable>& self) {
  struct _NullRenderable : public Renderable {
    virtual void render() {
    }
  };

  self = make_shared<_NullRenderable>();
}

inline void nullTouchHandler(shared_ptr<TouchHandler>& self, function<void (shared_ptr<TouchHandler>&, Void)>) {
  struct NullTouchHandler : public TouchHandler {
    virtual void handleTouch(int /*x*/, int /*y*/) {
    }
  };

  self = make_shared<NullTouchHandler>();
}

inline function<void (shared_ptr<Widget>& self)> widget(
    function<void (shared_ptr<Renderable>&)> renderable,
    function<void (shared_ptr<TouchHandler>&)> touchHandler) {

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

inline function<void (shared_ptr<TouchHandler>& self)> onTouch(function<void ()> onClick) {
  struct TouchHandlerImpl : public TouchHandler {
    function<void ()> _onClick;

    TouchHandlerImpl(
        function<void ()> const& onClick2):
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

inline function<void (shared_ptr<Widget>& self)> button(function<void ()> onClick) {
  return widget(quad, onTouch(onClick));
}

int main() {
  return 0;
}
