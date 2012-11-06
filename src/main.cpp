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

struct WidgetImpl : public Widget {
  shared_ptr<Renderable> _renderable;
  shared_ptr<TouchHandler> _touchHandler;

  WidgetImpl(shared_ptr<Renderable> renderable, shared_ptr<TouchHandler> touchHandler):
    _renderable(renderable), _touchHandler(touchHandler) {}

  virtual void render();
  virtual void handleTouch(int x, int y);
};

void WidgetImpl::render() {
  _renderable->render();
}

void WidgetImpl::handleTouch(int x, int y) {
  _touchHandler->handleTouch(x, y);
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

inline void waitForTouch(shared_ptr<TouchHandler>& self, function<void (shared_ptr<TouchHandler>&, Void)> cont) {
  struct TouchHandlerImpl : public TouchHandler {
    shared_ptr<TouchHandler>& _self;
    function<void (shared_ptr<TouchHandler>&, Void)> _cont;

    TouchHandlerImpl(
        shared_ptr<TouchHandler>& self2,
        function<void (shared_ptr<TouchHandler>&, Void)> const& cont2):
      _self(self2),
      _cont(cont2) {
    }

    virtual void handleTouch(int /*x*/, int /*y*/) {
      // ... Test if in area.

      _cont(_self, Void());
    }
  };

  self = make_shared<TouchHandlerImpl>(self, cont);
}

inline void quad(shared_ptr<Renderable>& self) {
  struct RenderableImpl : public Renderable {
    virtual void render() {
      // ... Render quad
    }
  };

  self = make_shared<RenderableImpl>();
}

inline void button(shared_ptr<Widget>& /*self*/, function<void (shared_ptr<Widget>&, Void)> /*cont*/) {
}

int main() {
  return 0;
}
