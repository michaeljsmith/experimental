struct TouchHandler {
  virtual ~TouchHandler() {}
  virtual void handleTouch(int x, int y) = 0;
};

struct Renderable {
  virtual ~Renderable() {}
  virtual void render() = 0;
};

struct Widget : public TouchHandler, public Renderable {
  virtual void render() = 0;
  virtual void handleTouch(int x, int y) = 0;
};

struct _WidgetImpl : public Widget {
  shared_ptr<Renderable> renderable;
  shared_ptr<TouchHandler> touchHandler;

  _WidgetImpl(shared_ptr<Renderable> renderable, shared_ptr<TouchHandler> touchHandler):
    renderable(renderable), touchHandler(touchHandler) {}

  virtual void render() {
    renderable->render();
  }

  virtual void handleTouch(int x, int y) {
    touchHandler->handleTouch(x, y);
  }
};

struct Void {};

// TODO: Specialize Class for void type.
// TODO: Special version for non-returning expressions?
void nullRenderable(shared_ptr<Renderable>& self) {
  struct _NullRenderable : public Renderable {
    virtual void render() {
    }
  };

  self = make_shared<_NullRenderable>();
}

void nullTouchHandler(shared_ptr<TouchHandler>& self, function<void (shared_ptr<TouchHandler>&, Void)>) {
  struct _NullTouchHandler : public TouchHandler {
    virtual void handleTouch(int x, int y) {
    }
  };

  self = make_shared<_NullRenderable>();
}

void waitForTouch(shared_ptr<TouchHandler>& self, function<void (shared_ptr<TouchHandler>&, Void)> cont) {
  struct TouchHandlerImpl : public TouchHandler {
    virtual void handleTouch(int x, int y) {
      // ... Test if in area.

      cont(self, Void());
    }
  };

  self = make_shared<TouchHandlerImpl>();
}

void quad(shared_ptr<TouchHandler>& self) {
  struct RenderableImpl : public Renderable {
    virtual void render() {
      // ... Render quad
    }
  };

  self = make_shared<RenderableImpl>();
}

void button(shared_ptr<Widget>& self, function<void (shared_ptr<Widget>&, Void)> cont) {
}

int main() {
  return 0;
}
