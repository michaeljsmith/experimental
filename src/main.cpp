#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <functional>
#include "SDL.h"
#include "SDL_Opengl.h"

#include "fontstash.h"

using std::function;

template <int Line> struct SourceLocationTag {};
#define UNIQUE_TAG SourceLocationTag<__LINE__> // TODO: Make work across TUs!

template <typename Tag, typename T> struct TrivialSubClass {};

template <typename T> struct Object {};
template <typename T> using Class = function<T ()>;

template <typename T> struct Method : public function<T> {};

template <typename T> using Execute = TrivialSubClass<UNIQUE_TAG, Method<T ()>>;
typedef TrivialSubClass<UNIQUE_TAG, Method<void ()>> Render;

template <typename T> using Action = TrivialSubClass<UNIQUE_TAG, Object<Execute<T>>>;
typedef TrivialSubClass<UNIQUE_TAG, Object<Render>> Widget;

Widget command(string text, Action<void> const& action) {
}

inline Action<void>& newGame() {
  static Action<void> result;
  return result;
}

inline Widget mainMenu() {
  return menu(
      command("new game", newGame()),
      command("quit", quit()));
}

int main(int /*argc*/, char* /*argv*/[])
{
	int done;
	SDL_Event event;
	SDL_Surface* screen;
	const SDL_VideoInfo* vi;
	int width,height;
	struct sth_stash* stash = 0;
	float sx,sy,dx,dy,lh;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return -1;
	}

	// Init OpenGL
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	vi = SDL_GetVideoInfo();
	width = vi->current_w - 20;
	height = vi->current_h - 80;
	screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL);
	if (!screen)
	{
		printf("Could not initialise SDL opengl\n");
		return -1;
	}

	SDL_WM_SetCaption("FontStash Demo", 0);

//	fnt = sth_create("DroidSansJapanese.ttf", 512,512);
	stash = sth_create(512,512);
	if (!stash)
	{
		printf("Could not create stash.\n");
		return -1;
	}
	if (!sth_add_font(stash,0,"data/DroidSerif-Regular.ttf"))
	{
		printf("Could not add font.\n");
		return -1;
	}
	if (!sth_add_font(stash,1,"data/DroidSerif-Italic.ttf"))
	{
		printf("Could not add font.\n");
		return -1;
	}
	if (!sth_add_font(stash,2,"data/DroidSerif-Bold.ttf"))
	{
		printf("Could not add font.\n");
		return -1;
	}
	if (!sth_add_font(stash,3,"data/DroidSansJapanese.ttf"))
	{
		printf("Could not add font.\n");
		return -1;
	}

	done = 0;
	while (!done)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
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

		// Update and render
		glViewport(0, 0, width, height);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,width,0,height,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glColor4ub(255,255,255,255);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		sx = 100; sy = 250;

		sth_begin_draw(stash);

		dx = sx; dy = sy;
		sth_draw_text(stash, 0,24.0f, dx,dy,"The quick ",&dx);
		sth_draw_text(stash, 1,48.0f, dx,dy,"brown ",&dx);
		sth_draw_text(stash, 0,24.0f, dx,dy,"fox ",&dx);
		sth_vmetrics(stash, 1,24, NULL,NULL,&lh);
		dx = sx;
		dy -= lh*1.2f;
		sth_draw_text(stash, 1,24.0f, dx,dy,"jumps over ",&dx);
		sth_draw_text(stash, 2,24.0f, dx,dy,"the lazy ",&dx);
		sth_draw_text(stash, 0,24.0f, dx,dy,"dog.",&dx);
		dx = sx;
		dy -= lh*1.2f;
		sth_draw_text(stash, 0,12.0f, dx,dy,"Now is the time for all good men to come to the aid of the party.",&dx);
		sth_vmetrics(stash, 1,12, NULL,NULL,&lh);
		dx = sx;
		dy -= lh*1.2f*2;
		sth_draw_text(stash, 1,18.0f, dx,dy,"Ég get etið gler án þess að meiða mig.",&dx);
		sth_vmetrics(stash, 1,18, NULL,NULL,&lh);
		dx = sx;
		dy -= lh*1.2f;
		sth_draw_text(stash, 3,18.0f, dx,dy,"私はガラスを食べられます。それは私を傷つけません。",&dx);

		sth_end_draw(stash);

		glEnable(GL_DEPTH_TEST);
		SDL_GL_SwapBuffers();
	}

	SDL_Quit();
	return 0;
}
