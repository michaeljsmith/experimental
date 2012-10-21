#include <string>
#include <functional>
#include "SDL.h"
#include "SDL_Opengl.h"

#include "fontstash.h"

using std::string;
using std::function;

int viewportWidth = -1, viewportHeight = -1;

struct sth_stash* stash = 0;

class Object {
  public:
    Object(function<void (Object&)> cls): fn(nullImpl) {
      cls(*this);
    }

    Object(Object const&) = delete;
    Object& operator=(Object const&) = delete;

    template <typename S> function<S> getMethod(void* methodId) {
      return *static_cast<function<S>*>(fn(methodId));
    }

    template <typename F> void addMethod(void* methodId, F method) {
      function<void* (void*)> oldFn = fn;
      fn = [=] (void* givenMethodId) -> void const* {
        if (givenMethodId == methodId) {
          return &method;
        } else {
          return oldFn(givenMethodId);
        }
      };
    }

  private:
    static void* nullImpl(void const*) {
      ((void (*)())nullptr)();
      return nullptr;
    }

    function<void* (void const*)> fn;
};

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

namespace widgets {
  int render;
}

inline void app(Object& self) {
  self.addMethod(&widgets::render, [&] () mutable {
    float ascender;
    sth_vmetrics(stash, 0, 24.0f, &ascender, nullptr, nullptr);
    sth_draw_text(stash, 0, 24.0f, 0xFF0000FF, 10, viewportHeight - ascender, "Foo!", nullptr);
  });
}

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
          //handleClick(event.button.x, event.button.y, app);
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

Object _app(app);

inline void render() {
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
  _app.getMethod<void ()>(&widgets::render)();

  sth_end_draw(stash);

  glEnable(GL_DEPTH_TEST);

  SDL_GL_SwapBuffers();
}

int main(int /*argc*/, char* /*argv*/[])
{
  initPlatform();
  initGl();

  initializeFonts();

	while (!done)
	{
    handlePendingEvents();
    render();
	}

  shutdownPlatform();

	return 0;
}
