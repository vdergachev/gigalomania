//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <ctime>

#include "screen.h"
#include "sound.h"
#include "game.h"
#include "utils.h"
#include "gamestate.h"
#include "image.h"
#include "sector.h"
#include "panel.h"
#include "gui.h"
#include "player.h"

//---------------------------------------------------------------------------

Gigalomania::Screen::Screen() : m_pos_x(0), m_pos_y(0), m_down_left(false), m_down_middle(false), m_down_right(false) {
	sdlWindow = NULL;
	sdlRenderer = NULL;
	width = 0;
	height = 0;
	windowed_w = 0;
	windowed_h = 0;
}

Gigalomania::Screen::~Screen() {
	Image::setGraphicsOutput(NULL);
	// must delete renderer before window (as renderer depends on window), otherwise get assertion failure on Android
	// also ensure that all Images (and hence SDL_Textures) have been destroyed
	if( sdlRenderer != NULL ) {
		SDL_DestroyRenderer(sdlRenderer);
		sdlRenderer = NULL;
	}
	if( sdlWindow != NULL ) {
		SDL_DestroyWindow(sdlWindow);
		sdlWindow = NULL;
	}
}

bool Gigalomania::Screen::canOpenFullscreen(int width, int height) {
	return true;
}

bool Gigalomania::Screen::open(int screen_width, int screen_height, bool fullscreen) {
	LOG("Screen::open(%d, %d, %s)\n", screen_width, screen_height, fullscreen?"fullscreen":"windowed");
	if( !SDL_CreateWindowAndRenderer("Gigalomania", screen_width, screen_height, fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE, &sdlWindow, &sdlRenderer) ) {
		LOG("failed to open screen at this resolution: %s\n", SDL_GetError());
		return false;
	}
	this->width = screen_width;
	this->height = screen_height;
	{
		SDL_BlendMode blendMode = SDL_BLENDMODE_NONE;
		SDL_GetRenderDrawBlendMode(sdlRenderer, &blendMode);
		LOG("render draw blend mode: %d\n", blendMode);
	}
	SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND); // needed for Screen::fillRectWithAlpha, as blending is off by default for drawing functions
	LOG("screen opened ok\n");

	if( game_g->isMobileUI() ) {
		SDL_HideCursor(); // comment out to test with system cursor, for testing purposes when ran on non-touchscreen devices
	}
	else {
		//SDL_ShowCursor(false);
		// Set up a blank cursor
		// We use this hacky way due to a bug in SDL_ShowCursor(false), where when alt-tabbing out of full screen mode in Windows, the mouse pointer is
		// trapped in the top left corner. See 
		unsigned char data[1] = "";
		unsigned char mask[1] = "";
		SDL_Cursor *cursor = SDL_CreateCursor(data, mask, 8, 1, 0, 0);
		SDL_Cursor *old_cursor = SDL_GetCursor();
		SDL_SetCursor(cursor);
		SDL_DestroyCursor(old_cursor);
	}

	Image::setGraphicsOutput(sdlRenderer);

	return true;
}

void Gigalomania::Screen::setLogicalSize(int width, int height, bool smooth) {
	this->width = width;
	this->height = height;
	LOG("width, height: %d, %d\n", width, height);
	SDL_SetRenderLogicalPresentation(sdlRenderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void Gigalomania::Screen::setWindowedSize(int w, int h) {
	this->windowed_w = w;
	this->windowed_h = h;
}

void Gigalomania::Screen::setFullscreen(bool fullscreen) {
	LOG("Screen::setFullscreen(%s)\n", fullscreen ? "fullscreen" : "windowed");
	SDL_SetWindowFullscreen(sdlWindow, fullscreen);
	if( !fullscreen && windowed_w > 0 && windowed_h > 0 ) {
		SDL_SetWindowSize(sdlWindow, windowed_w, windowed_h);
	}
}

void Gigalomania::Screen::setTitle(const char *title) {
	SDL_SetWindowTitle(sdlWindow, title);
}

void Gigalomania::Screen::clear() {
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
}

void Gigalomania::Screen::refresh() {
	SDL_RenderPresent(sdlRenderer);
}

int Gigalomania::Screen::getWidth() const {
	return this->width;
}

int Gigalomania::Screen::getHeight() const {
	return this->height;
}

void Gigalomania::Screen::fillRect(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b) {
	SDL_FRect rect;
	rect.x = x; rect.y = y; rect.w = w; rect.h = h;
	SDL_SetRenderDrawColor(sdlRenderer, r, g, b, 255);
	SDL_RenderFillRect(sdlRenderer, &rect);
}

void Gigalomania::Screen::fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha) {
	SDL_FRect rect;
	rect.x = x; rect.y = y; rect.w = w; rect.h = h;
	SDL_SetRenderDrawColor(sdlRenderer, r, g, b, alpha);
	SDL_RenderFillRect(sdlRenderer, &rect);
}

void Gigalomania::Screen::drawLine(short x1, short y1, short x2, short y2, unsigned char r, unsigned char g, unsigned char b) {
	SDL_SetRenderDrawColor(sdlRenderer, r, g, b, 255);
	SDL_RenderLine(sdlRenderer, (float)x1, (float)y1, (float)x2, (float)y2);
}

void Gigalomania::Screen::convertWindowToLogical(int *m_x, int *m_y) const {
	SDL_Rect rect;
	SDL_GetRenderViewport(sdlRenderer, &rect);
	float scale_x = 0.0f, scale_y = 0.0f;
	SDL_GetRenderScale(sdlRenderer, &scale_x, &scale_y);
	*m_x = (int)(*m_x / scale_x);
	*m_y = (int)(*m_y / scale_y);
	*m_x -= rect.x;
	*m_y -= rect.y;
}

void Gigalomania::Screen::getWindowSize(int *window_width, int *window_height) const {
	SDL_GetWindowSize(sdlWindow, window_width, window_height);
}

/* Converts SDL touch coordinates (from SDL_TouchFingerEvent) to window coordinates (consistent
   with those used for mouse events).
*/
void Gigalomania::Screen::convertTouchCoords(int *m_x, int *m_y, float tx, float ty) const {
	*m_x = (int)(tx*this->width);
	*m_y = (int)(ty*this->height);
}


void Gigalomania::Screen::getMouseCoords(int *m_x, int *m_y) const {
	*m_x = this->m_pos_x;
	*m_y = this->m_pos_y;
	//LOG("Screen::getMouseCoords: %d, %d\n", *m_x, *m_y);
}

bool Gigalomania::Screen::getMouseState(int *m_x, int *m_y, bool *m_left, bool *m_middle, bool *m_right) const {
	*m_x = this->m_pos_x;
	*m_y = this->m_pos_y;
	*m_left = this->m_down_left;
	*m_middle = this->m_down_middle;
	*m_right = this->m_down_right;
	/*if( *m_left ) {
		LOG("mouse down at: %d, %d\n", *m_x, *m_y);
	}*/
	//LOG("Screen::getMouseState: %d, %d\n", *m_x, *m_y);
	return ( *m_left || *m_middle || *m_right );
}

Application::Application() : quit(false), blank_mouse(false), compute_fps(false), fps(0.0f), last_time(0) {
	// uncomment to display fps
	//compute_fps = true;
}

Application::~Application() {
	LOG("quit SDL\n");
	SDL_Quit();
}

bool Application::init() {
#if defined(_WIN32)
	_putenv("SDL_VIDEO_CENTERED=0,0");
#elif defined(__MORPHOS__)
#else
	setenv("SDL_VIDEO_CENTERED", "0,0", 1);
#endif
	if( !SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) ) {
		LOG("SDL_Init failed\n");
		return false;
	}
	return true;
}

unsigned int Application::getTicks() const {
	return (unsigned int)SDL_GetTicks();
}

void Application::delay(unsigned int time) {
	SDL_Delay(time);
}

const int TICK_INTERVAL = 16; // 62.5 fps max

void Application::wait() {
	unsigned int now = game_g->getApplication()->getTicks();
	// use unsigned int and compare by subtraction to avoid overflow problems - see http://blogs.msdn.com/b/oldnewthing/archive/2005/05/31/423407.aspx, http://www.gammon.com.au/millis
	unsigned int elapsed = now - last_time;
	unsigned int delay = 0;
	if( elapsed >= TICK_INTERVAL ) {
		// fine, we've already passed TICK_INTERVAL
	}
	else {
		delay = TICK_INTERVAL - elapsed;
		game_g->getApplication()->delay(delay);
	}
	last_time = now + delay;
}

void Application::runMainLoop() {
	unsigned int elapsed_time = game_g->getApplication()->getTicks();

	SDL_Event event;
	quit = false;
	int last_fps_time = clock();
	const int fps_frames_c = 50;
	int frames = 0;
	while(!quit) {
		if( compute_fps && frames == fps_frames_c ) {
			int new_fps_time = clock();
			float t = ((float)(new_fps_time - last_fps_time)) / (float)CLOCKS_PER_SEC;
			this->fps = fps_frames_c / t;
			//LOG("FPS: %f\n", fps);
			frames = 0;
			last_fps_time = new_fps_time;
		}
		frames++;

		updateSound();

		// draw screen
		game_g->drawGame();

		/* wait() to avoid 100% CPU - it's debatable whether we should do this,
		 * due to risk of SDL_Delay waiting too long, but since Gigalomania
		 * doesn't really need high frame rate, might as well avoid using full
		 * CPU. Also good for battery-life on mobile platforms.
		 * This also has the side-effect of meaning we don't call updateGame()
		 * with too small timestep - we have a minimum step of at least
		 * TICK_INTERVAL.
		 */
		wait();

		unsigned int new_time = game_g->getApplication()->getTicks();
		//LOG("%d, %d\n", new_time, new_time - elapsed_time);
		if( !game_g->isPaused() ) {
			game_g->updateTime(new_time - elapsed_time);
		}
		else
			game_g->updateTime(0);
		elapsed_time = new_time;

		// user input
		while( SDL_PollEvent(&event) == 1 ) {
			switch (event.type) {
			case SDL_EVENT_QUIT:
				// SDL_EVENT_QUIT may mean a message from the OS (e.g., OS shutting down) - so we quit immediately, saving the current state
				game_g->requestQuit(true);
				break;
			case SDL_EVENT_KEY_DOWN:
				{
					SDL_Keycode key = event.key.key;
					if( key == SDLK_ESCAPE || key == SDLK_Q
						|| key == SDLK_AC_BACK // SDLK_AC_BACK required for Android
						) {
						game_g->requestQuit(false);
					}
					else if( key == SDLK_P ) {
						game_g->togglePause();
					}
					else if( key == SDLK_RETURN ) {
						game_g->keypressReturn();
					}
					break;
				}
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					//LOG("received mouse down\n");
					// if this code is changed, consider whether SDL_EVENT_FINGER_DOWN also needs updating
					int m_x = (int)event.button.x;
					int m_y = (int)event.button.y;
					game_g->getScreen()->setMousePos(m_x, m_y);
					bool m_left = false, m_middle = false, m_right = false;
					Uint8 button = event.button.button;
					if( button == SDL_BUTTON_LEFT ) {
						m_left = true;
						game_g->getScreen()->setMouseLeft(true);
					}
					else if( button == SDL_BUTTON_MIDDLE ) {
						m_middle = true;
						game_g->getScreen()->setMouseMiddle(true);
					}
					else if( button == SDL_BUTTON_RIGHT ) {
						m_right = true;
						game_g->getScreen()->setMouseRight(true);
					}

					if( game_g->isPaused() ) {
						// click automatically unpaused (needed to work without keyboard!)
						game_g->togglePause();
					}
					else if( m_left || m_middle || m_right ) {
						game_g->mouseClick(m_x, m_y, m_left, m_middle, m_right, true);
					}

					break;
				}
			case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					//LOG("received mouse up\n");
					// if this code is changed, consider whether SDL_EVENT_FINGER_UP also needs updating
					Uint8 button = event.button.button;
					if( button == SDL_BUTTON_LEFT ) {
						game_g->getScreen()->setMouseLeft(false);
					}
					else if( button == SDL_BUTTON_MIDDLE ) {
						game_g->getScreen()->setMouseMiddle(false);
					}
					else if( button == SDL_BUTTON_RIGHT ) {
						game_g->getScreen()->setMouseRight(false);
					}
					break;
				}
			case SDL_EVENT_MOUSE_MOTION:
				{
					// if this code is changed, consider whether SDL_EVENT_FINGER_MOTION also needs updating
					int m_x = (int)event.motion.x;
					int m_y = (int)event.motion.y;
					this->blank_mouse = false;
					game_g->getScreen()->setMousePos(m_x, m_y);
					break;
				}
			// support for touchscreens
			case SDL_EVENT_FINGER_DOWN:
				{
					//LOG("received fingerdown: %f , %f\n", event.tfinger.x, event.tfinger.y);
					int m_x = 0, m_y = 0;
					game_g->getScreen()->convertTouchCoords(&m_x, &m_y, event.tfinger.x, event.tfinger.y);
					game_g->getScreen()->setMousePos(m_x, m_y);
					game_g->getScreen()->setMouseLeft(true);
					this->blank_mouse = true;

					if( game_g->isPaused() ) {
						// click automatically unpaused (needed to work without keyboard!)
						game_g->togglePause();
					}
					else {
						game_g->mouseClick(m_x, m_y, true, false, false, true);
					}
					break;
				}
			case SDL_EVENT_FINGER_UP:
				{
					//LOG("received fingerup\n");
					game_g->getScreen()->setMouseLeft(false);
					this->blank_mouse = true;
					break;
				}
			case SDL_EVENT_FINGER_MOTION:
				{
					//LOG("received fingermotion: %f , %f\n", event.tfinger.x, event.tfinger.y);
					int m_x = 0, m_y = 0;
					game_g->getScreen()->convertTouchCoords(&m_x, &m_y, event.tfinger.x, event.tfinger.y);
					game_g->getScreen()->setMousePos(m_x, m_y);
					this->blank_mouse = true;
					break;
				}
			case SDL_EVENT_WINDOW_SHOWN:
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				game_g->activate();
				break;
			case SDL_EVENT_WINDOW_HIDDEN:
			case SDL_EVENT_WINDOW_FOCUS_LOST:
				game_g->deactivate();
				break;
			}
		}
		SDL_PumpEvents();

		game_g->updateGame();
	}
}

//#endif
