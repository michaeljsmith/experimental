// [[[ Prelude
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <functional>
#include <string>
#include <assert.h>
#include "SDL.h"
#include "SDL_Opengl.h"

#include "fontstash.h"

using std::function;
using std::string;
// ]]]

// [[[ Cleanup
typedef function<void ()> CleanupHandler;

struct Cleanup {
  CleanupHandler _handler;
  Cleanup(CleanupHandler handler): _handler(handler) {}
  ~Cleanup() {_handler();}
};

#define CLEANUP_NAME_DETAIL(a, b) a##b
#define CLEANUP_NAME(a, b) CLEANUP_NAME_DETAIL(a, b)
#define CLEANUP(expr) Cleanup CLEANUP_NAME(cleanup, __LINE__) = (expr)
// ]]]

// [[[ UNIQUE_TAG
template <int Line> struct SourceLocationTag {};
#define UNIQUE_TAG SourceLocationTag<__LINE__> // TODO: Make work across TUs!
// ]]]

// [[[ TrivialSubClass
template <typename Tag, typename T> struct TrivialSubClass : public T {
  TrivialSubClass(): T() {}
  template <typename X0> TrivialSubClass(X0 x0): T(x0) {}
};
// ]]]

// [[[ Vars
template <typename T> CleanupHandler setVar(T& var, T value) {
  T oldValue = var;
  var = value;
  return [=,&var] () {
    var = oldValue;
  };
}
// ]]]

// [[[ Message
int messageFrameVersion = 0;

template <typename S> struct Message : public function<S> {
  Message(): Message(function<S>()) {}
  Message(function<S> fn): function<S>(fn), frameVersion(messageFrameVersion) {}

  template <typename... A> void operator()(A const &... args) {
    if (frameVersion == messageFrameVersion) {
      if ((*this) == nullptr) {
        ((void(*)())0)();
      }
      function<S>::operator()(args...);
    }
  }

  int frameVersion;
};

inline CleanupHandler clearMessageHandlers() {
  auto oldMessageFrameVersion = messageFrameVersion;
  ++messageFrameVersion;

  return [=] () {
    messageFrameVersion = oldMessageFrameVersion;
  };
}

template <typename S> inline CleanupHandler setMessage(Message<S>& msg, function<S> fn) {
  return setVar(msg, Message<S>(fn));
}
// ]]]

// [[[ Size
struct Size {
  Size() {}
  Size(float _w, float _h): w(_w), h(_h) {}
  float w, h;
};

struct BoundingRect {
  BoundingRect():
    x(0), y(0),
    w(std::numeric_limits<float>::max()),
    h(std::numeric_limits<float>::max()) {}
  BoundingRect(float _x, float _y, float _w, float _h): x(_x), y(_y), w(_w), h(_h) {}
  float x, y, w, h;
};

struct Position {
  Position() {}
  Position(float _x, float _y): x(_x), y(_y) {}
  float x, y;
};

inline bool positionInBounds(Position pos, BoundingRect bounds) {
  return (pos.x >= bounds.x &&
          pos.y >= bounds.y &&
          pos.x < bounds.x + bounds.w &&
          pos.y < bounds.y + bounds.h);
}
// ]]]

// [[[ Viewport
int viewportWidth = -1, viewportHeight = -1;
// ]]]

// [[[ Fonts
struct sth_stash* stash = 0;

inline void addFont(int idx, string path) {
  if (!sth_add_font(stash, idx, path.c_str()))
  {
    printf("Could not add font.\n");
    exit(1);
  }
}

inline void initializeFonts() {
	stash = sth_create(512,512);
	if (!stash)
	{
		printf("Could not create stash.\n");
    exit(1);
	}

  addFont(0, "data/DroidSerif-Regular.ttf");
	addFont(1, "data/DroidSerif-Italic.ttf");
  addFont(2, "data/DroidSerif-Bold.ttf");
  addFont(3, "data/DroidSansJapanese.ttf");
}
// ]]]

// [[[ old::Action
namespace old {
  typedef TrivialSubClass<UNIQUE_TAG, function<void ()>> Action;
}
// ]]]

// [[[ old::Expression
namespace old {
  template <typename T> using Expression = TrivialSubClass<UNIQUE_TAG, function<T ()>>;
}
// ]]]

// [[[ old::Widgets
namespace old {
  namespace messages {Message<void (Size)> widgetSize;}

  namespace parameters {Size widgetSize;}
  namespace parameters {BoundingRect widgetBounds;}

  namespace parameters {unsigned colour;}

  inline void renderText(string _text) {
    float ascender;
    sth_vmetrics(stash, 0, 24.0f, &ascender, nullptr, nullptr);
    sth_draw_text(stash, 0, 24.0f, parameters::colour, parameters::widgetBounds.x, viewportHeight - parameters::widgetBounds.y - ascender, _text.c_str(), nullptr);
  }

  inline Size getTextSize(string _text) {
    float minx, miny, maxx, maxy;
    sth_dim_text(stash, 0, 24.0f, _text.c_str(), &minx, &miny, &maxx, &maxy);
    float lineHeight;
    sth_vmetrics(stash, 0, 24.0f, nullptr, nullptr, &lineHeight);
    return Size(maxx - minx, lineHeight);
  }

  namespace parameters {Position clickPos;}

  inline void testHotRegion(Action action) {
    if (positionInBounds(parameters::clickPos, parameters::widgetBounds)) {
      action();
    }
  }

  namespace messages {Message<void (string)> text;}

  inline CleanupHandler widgetRoot() {
    auto h0 = clearMessageHandlers();
    auto h1 = setVar(parameters::widgetBounds, BoundingRect());
    return [=] () {
      h0();
      h1();
    };
  }

  typedef TrivialSubClass<UNIQUE_TAG, function<void ()>> Widget;

  inline Widget text(string _text)
  {
    return [=] () {
      messages::widgetSize(getTextSize(_text));
      messages::text(_text);
    };
  }

  namespace messages {Message<void (Action)> hotRegion;}

  inline Widget inHotRegion(Action _action, Widget _widget) {
    return Widget([=] () {
      messages::hotRegion(_action);
      _widget();
    });
  }

  inline Widget withColour(Expression<unsigned> _colour, Widget _target) {
    return Widget([=] () {
      CLEANUP(setVar(parameters::colour, _colour()));
      _target();
    });
  }

  namespace parameters {bool selected;}

  template <typename T> Expression<T> ifSelected(T trueCase, T falseCase) {
    return [=] () {
      return parameters::selected ? trueCase : falseCase;
    };
  }

  inline Widget button(string _text, Action _action) {
    return inHotRegion(_action,
        withColour(
          ifSelected(0xFFFF00FFu, 0xFFFFFFFFu),
          text(_text)));
  }

  inline Size widgetSize(Widget widget) {
    CLEANUP(clearMessageHandlers());
    CLEANUP(setMessage(messages::widgetSize, function<void (Size)>([] (Size size) {
      parameters::widgetSize = size;
    })));

    widget();
    return parameters::widgetSize;
  }

  inline void handleClick(float x, float y, Widget widget) {
    CLEANUP(widgetRoot());

    CLEANUP(setMessage(messages::hotRegion, function<void (Action)>(testHotRegion)));
    CLEANUP(setVar(parameters::clickPos, Position(x, y)));

    widget();
  }

  inline CleanupHandler restrictWidget(BoundingRect rect) {
    auto x = parameters::widgetBounds.x + std::max(0.0f, rect.x);
    auto y = parameters::widgetBounds.y + std::max(0.0f, rect.y);
    auto w = std::min(parameters::widgetBounds.w, rect.w);
    auto h = std::min(parameters::widgetBounds.h, rect.h);
    return setVar(parameters::widgetBounds, BoundingRect(x, y, w, h));
  }

  inline void widgetVertical(Widget widget) {
    auto size = widgetSize(widget);
    {
      CLEANUP(restrictWidget(BoundingRect(0.0f, 0.0f, size.w, size.h)));
      widget();
    }
    restrictWidget(BoundingRect(0, size.h,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()));
  }

  inline function<void (int)> menuCons(Widget head, function<void (int)> tail) {
    return [=] (int selectionIdx) -> void {
      {
        CLEANUP(setVar(parameters::selected, selectionIdx == 0));
        widgetVertical(head);
      }

      tail(selectionIdx - 1);
    };
  }

  inline function<void (int)> menuRecurse() {
    return [] (int) {};
  }

  template <typename... A> inline function<void (int)> menuRecurse(Widget head, A const&... tail) {
    return menuCons(head, menuRecurse(tail...));
  }

  template <typename... A> inline Widget menu(A const&... args) {
    auto _menu = menuRecurse(args...);

    return [=] () {
      _menu(0);
    };
  }
}
// ]]]

// [[[ old::Game
namespace old {
  Widget app;

  inline Widget optionsMenu(Action back) {
    return menu(
      button("foo", [] () {((void (*)())0)();}),
      button("back", back));
  }

  Action quit = [] () {((void (*)())0)();};

  inline Widget mainMenu() {

    // What to do when options is selected.
    Action _optionsMenuAction = [] () {
      app = optionsMenu([=] () {
        app = mainMenu();
      });
    };

    return menu(
      button("options", _optionsMenuAction),
      button("quit", quit));
  }
}
// ]]]

// [[[ instance
template <typename T>
inline function<T ()> instance(function<T ()> cls) {
  return cls;
}
// ]]]

// [[[ Action
typedef TrivialSubClass<UNIQUE_TAG, function<void()>> Action;

inline function<Action()> action(function<void()> _fn) {
  return [=] () -> Action {
    return Action(_fn);
  };
}
// ]]]

// [[[ Expression
template <typename T> using Expression = TrivialSubClass<UNIQUE_TAG, function<T ()>>;
// ]]]

// [[[ Widgets
struct Widget {};

namespace parameters {bool selected;}

namespace messages {Message<void (Size)> widgetSize;}
namespace parameters {Size widgetSize;}

namespace messages {Message<void (string)> text;}

namespace parameters {BoundingRect widgetBounds;}

namespace parameters {unsigned colour;}

namespace messages {Message<void (Action)> hotRegion;}

namespace parameters {Position clickPos;}

inline void renderText(string _text) {
  float ascender;
  sth_vmetrics(stash, 0, 24.0f, &ascender, nullptr, nullptr);
  sth_draw_text(stash, 0, 24.0f, parameters::colour, parameters::widgetBounds.x, viewportHeight - parameters::widgetBounds.y - ascender, _text.c_str(), nullptr);
}

inline void renderWidget(function<Widget ()> widget) {
  glViewport(0, 0, viewportWidth, viewportHeight);
  glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0,viewportWidth,0,viewportHeight,-1,1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_DEPTH_TEST);
  glColor4ub(255,255,255,255);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  sth_begin_draw(stash);

  {
    CLEANUP(clearMessageHandlers());
    CLEANUP(setVar(parameters::widgetBounds, BoundingRect()));
    CLEANUP(setMessage(messages::text, function<void (string)>(renderText)));
    widget();
  }

  sth_end_draw(stash);

  glEnable(GL_DEPTH_TEST);
}

inline void testHotRegion(Action action) {
  if (positionInBounds(parameters::clickPos, parameters::widgetBounds)) {
    action();
  }
}

inline void handleClick(float x, float y, function<Widget()> widget) {
  CLEANUP(clearMessageHandlers());
  CLEANUP(setVar(parameters::widgetBounds, BoundingRect()));
  CLEANUP(setMessage(messages::hotRegion, function<void(Action)>(testHotRegion)));
  CLEANUP(setVar(parameters::clickPos, Position(x, y)));

  widget();
}

template <typename T> function<Expression<T>()> varExpr(T const& var) {
  return [&] () -> Expression<T> {
    return [&] () -> T {
      return var;
    };
  };
}

template <typename T> function<Expression<T>()> ifExpr(function<Expression<bool>()> test, T trueCase, T falseCase) {
  return [=] () -> Expression<T> {
    auto _test = test();
    return [=] () -> T {
      return _test() ? trueCase : falseCase;
    };
  };
}

template <typename T> function<Expression<T>()> ifSelected(T trueCase, T falseCase) {
  return ifExpr(varExpr(parameters::selected), trueCase, falseCase);
}

inline Size getTextSize(string _text) {
  float minx, miny, maxx, maxy;
  sth_dim_text(stash, 0, 24.0f, _text.c_str(), &minx, &miny, &maxx, &maxy);
  float lineHeight;
  sth_vmetrics(stash, 0, 24.0f, nullptr, nullptr, &lineHeight);
  return Size(maxx - minx, lineHeight);
}

inline function<Widget ()> text(string _text)
{
  return [=] () -> Widget {
    messages::widgetSize(getTextSize(_text)); // TODO: Call only if widgetSize valid.
    messages::text(_text);
    return Widget();
  };
}

inline function<Widget()> withColour(function<Expression<unsigned> ()>  _colour, function<Widget ()> _target) {
  return [=] () -> Widget {
    CLEANUP(setVar(parameters::colour, _colour()()));
    return _target();
  };
}

inline function<Widget()> inHotRegion(function<Action()> _action, function<Widget()> _widget) {
  return [=] () -> Widget {
    auto action = _action();
    messages::hotRegion(action);
    return _widget();
  };
}

inline function<Widget ()> button(string _text, function<Action ()> _action) {
  return inHotRegion(_action,
      withColour(
        ifSelected(0xFFFF00FFu, 0xFFFFFFFFu),
        text(_text)));
}

inline Size widgetSize(function<Widget ()> widget) {
  CLEANUP(clearMessageHandlers());
  CLEANUP(setMessage(messages::widgetSize, function<void (Size)>([] (Size size) {
    parameters::widgetSize = size;
  })));

  widget();
  return parameters::widgetSize;
}

inline CleanupHandler restrictWidget(BoundingRect rect) {
  auto x = parameters::widgetBounds.x + std::max(0.0f, rect.x);
  auto y = parameters::widgetBounds.y + std::max(0.0f, rect.y);
  auto w = std::min(parameters::widgetBounds.w, rect.w);
  auto h = std::min(parameters::widgetBounds.h, rect.h);
  return setVar(parameters::widgetBounds, BoundingRect(x, y, w, h));
}

inline Widget widgetVertical(function<Widget ()> widget) {
  Widget result;
  auto size = widgetSize(widget);
  {
    CLEANUP(restrictWidget(BoundingRect(0.0f, 0.0f, size.w, size.h)));
    result = widget();
  }
  restrictWidget(BoundingRect(0, size.h,
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max()));

  return result;
}

inline function<Widget ()> menuItem(function<Widget ()> head, bool selected) {
  return [=] () -> Widget {
    CLEANUP(setVar(parameters::selected, selected));
    return widgetVertical(head);
  };
}

inline function<Widget ()> menuCons(function<Widget ()> head, function<Widget ()> tail) {
  return [=] () -> Widget {
    head();
    return tail();
  };
}

inline function<Widget ()> menuRecurse(int /*selectionIdx*/) {
  return [] () {return Widget();};
}

template <typename... A>
inline function<Widget ()> menuRecurse(int selectionIdx, function<Widget ()> head, A const&... tail) {
  return menuCons(menuItem(head, selectionIdx == 0), menuRecurse(selectionIdx - 1, tail...));
}

template <typename... A>
inline function<Widget ()> menu(A const&... args) {
  return menuRecurse(0, args...);
}
// ]]]

// [[[ Game
function<Widget ()> app;

inline function<Widget ()> optionsMenu(function<Action ()> back) {
  return menu(
    button("foo", action([](){((void (*)())0)();})),
    button("back", back));
}

function<Action ()> quit = action([](){((void (*)())0)();});

inline Widget mainMenu() {

  //// What to do when options is selected.
  //Action _optionsMenuAction = [] () {
  //  app = optionsMenu([=] () {
  //    app = mainMenu();
  //  });
  //};

  return menu(
    button("options", action([] () {((void(*)())0)();})/*_optionsMenuAction*/),
    button("quit", quit))();
}
// ]]]

// [[[ Platform
int done = 0;

inline void initPlatform() {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  const SDL_VideoInfo* vi = SDL_GetVideoInfo();
  viewportWidth = vi->current_w - 20;
  viewportHeight = vi->current_h - 80;

	SDL_WM_SetCaption("Curio", 0);
}

inline void shutdownPlatform() {
	SDL_Quit();
}

inline void initGl() {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

  SDL_Surface* screen = SDL_SetVideoMode(viewportWidth, viewportHeight, 0, SDL_OPENGL);
  if (!screen)
  {
    printf("Could not initialise SDL opengl\n");
    exit(1);
  }
}

inline void handlePendingEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_MOUSEMOTION:
        break;
      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button) {
          handleClick(event.button.x, event.button.y, app);
        }
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE)
          done = 1;
        break;
      case SDL_QUIT:
        done = 1;
        break;
      default:
        break;
    }
  }
}

inline void render() {
  renderWidget(app);

  SDL_GL_SwapBuffers();
}

int main(int /*argc*/, char* /*argv*/[])
{
  initPlatform();
  initGl();

  initializeFonts();

  app = instance(function<Widget ()>(mainMenu));

	while (!done)
	{
    handlePendingEvents();
    render();
	}

  shutdownPlatform();

	return 0;
}
// ]]]
