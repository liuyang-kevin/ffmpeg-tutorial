/**
 * show_window.cpp
 */
#include <SDL/SDL.h>

#include <iostream>
#include <string>

SDL_Surface* screen;
static const std::string SCREEN_CAPTION = "SDL window test";
static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 640;
static const int SCREEN_BPP = 32;

bool init();
bool finalize();
bool pollingEvent();

int main(int argc, char* argv[]) {
  // initialize
  if (!init()) {
    //std::cerr << "ERROR: failed to initialize SDL" << std::endl;
    exit(1);
  }

  // main loop
  while (true) {
    if (!pollingEvent()) break;
  }

  // finalize
  finalize();
  
  return 0;
}


bool init() {
  // initialize SDL
  if( SDL_Init(SDL_INIT_VIDEO) < 0 ) return false;

  // set caption of window
  SDL_WM_SetCaption( SCREEN_CAPTION.c_str(), NULL );

  // initialize window
  screen = SDL_SetVideoMode(
                            SCREEN_WIDTH,
                            SCREEN_HEIGHT,
                            SCREEN_BPP,
                            SDL_SWSURFACE // | SDL_FULLSCREEN
                            );

  // vanish mouce cursor
  // SDL_ShowCursor(SDL_DISABLE);

  return true;
}

bool finalize() {
  // finalize SDL
  SDL_Quit();
}


// polling event and execute actions
bool pollingEvent()
{
  SDL_Event ev;
  SDLKey *key;
  while ( SDL_PollEvent(&ev) )
    {
      switch(ev.type){
      case SDL_QUIT:
        // raise when exit event is occur
        return false;
        break;
      case SDL_KEYDOWN:
        // raise when key down
        {
          key=&(ev.key.keysym.sym);
          // ESC
          if(*key==27){
            return false;
          }
        }
        break;
      }
    }
  return true;
}