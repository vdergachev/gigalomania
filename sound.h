#pragma once

/** Handles sound samples and music.
*/

#include <string>
using std::string;

#include "resources.h"

#if defined(__MORPHOS__)
#include <SDL/SDL_mixer.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
// SDL3_mixer not yet in package managers; use SDL2_mixer via compatibility shim
// We need SDL2_mixer's API declarations without including SDL2 headers that clash with SDL3
// Use a minimal declaration approach: declare only what we need from Mix_ API
extern "C" {
// types from SDL2_mixer that we need (that don't conflict)
typedef struct Mix_Chunk Mix_Chunk;
typedef struct _Mix_Music Mix_Music;
typedef enum {
    MUS_NONE, MUS_CMD, MUS_WAV, MUS_MOD, MUS_MID, MUS_OGG,
    MUS_MP3, MUS_MP3_MAD_UNUSED, MUS_FLAC, MUS_MODPLUG_UNUSED, MUS_OPUS, MUS_WAVPACK, MUS_GME
} Mix_MusicType;
#define MIX_DEFAULT_FORMAT 0x8010
#define MIX_MAX_VOLUME 128
#define MIX_CHANNELS 8
// function declarations
extern int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
extern void Mix_CloseAudio(void);
extern void Mix_Quit(void);
extern Mix_Music *Mix_LoadMUS(const char *file);
extern void Mix_FreeMusic(Mix_Music *music);
extern int Mix_PlayMusic(Mix_Music *music, int loops);
extern int Mix_FadeInMusic(Mix_Music *music, int loops, int ms);
extern int Mix_VolumeMusic(int volume);
extern void Mix_PauseMusic(void);
extern void Mix_ResumeMusic(void);
extern Mix_Chunk *Mix_LoadWAV(const char *file);
extern void Mix_FreeChunk(Mix_Chunk *chunk);
extern int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);
extern int Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
extern int Mix_HaltChannel(int channel);
extern int Mix_Paused(int channel);
extern int Mix_Playing(int channel);
extern void Mix_Pause(int channel);
extern void Mix_Resume(int channel);
extern int Mix_Init(int flags);
extern int Mix_Volume(int channel, int volume);
extern int Mix_FadeOutMusic(int ms);
extern int Mix_FadeOutChannel(int which, int ms);
}
#else
#include <SDL_mixer.h>
#endif

bool initSound();
void updateSound();
void freeSound();

namespace Gigalomania {
	class Sample : public TrackedObject {
		bool is_music;
		Mix_Music *music;
		Mix_Chunk *chunk;
		int channel;

		string text;

		Sample(bool is_music, Mix_Music *music, Mix_Chunk *chunk) : is_music(is_music), music(music), chunk(chunk) {
			channel = -1;
		}
	public:
		Sample() : is_music(false), music(NULL), chunk(NULL), channel(0) {
			// create dummy Sample
		}
		virtual ~Sample();
		virtual const char *getClass() const { return "CLASS_SAMPLE"; }
		void play(int ch, int loops);
		//bool isPlaying() const;
		void fadeOut(int duration_ms);
		void setVolume(float volume);
		void setText(const char *text) {
			this->text = text;
		}

		static void pauseMusic();
		static void unpauseMusic();
		static void pauseChannel(int ch);
		static void unpauseChannel(int ch);

		static Sample *loadSample(const char *filename, bool iff = false);
		static Sample *loadSample(string filename, bool iff = false);
		static Sample *loadMusic(const char *filename);
		static Sample *loadMusic(string filename);
	};
}

const int SOUND_CHANNEL_SAMPLES   = 0;
const int SOUND_CHANNEL_MUSIC     = 1;
const int SOUND_CHANNEL_FX        = 2;
const int SOUND_CHANNEL_BIPLANE   = 3;
const int SOUND_CHANNEL_BOMBER    = 4;
const int SOUND_CHANNEL_SPACESHIP = 5;

inline void playSample(Gigalomania::Sample *sample, int channel = SOUND_CHANNEL_SAMPLES, int loops = 0) {
	sample->play(channel, loops);
}

bool isPlaying(int ch);

bool errorSound();
void resetErrorSound();
