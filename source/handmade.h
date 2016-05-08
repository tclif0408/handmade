#if !defined(HANDMADE_H)
/*========================================

	handmade.h

	=========================================*/

/*
 *	TODO: Services that the platform layer provides to the game
 * */

/*
 *	NOTE: Services that the game provides to the platform layer
 *	(this may expand in the future - sound on separate thread, etc)
 * */

struct game_offscreen_buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
};

struct game_sound_output_buffer {
	int SamplesPerSecond;
	int SampleCount;
	int16_t *Samples;
};

// FOUR THINGS Timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(game_offscreen_buffer* Buffer, game_sound_output_buffer *SoundBuffer);

#define HANDMADE_H
#endif
