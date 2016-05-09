#if !defined(HANDMADE_H)
/*========================================

	handmade.h

	=========================================*/

/*

	NOTE:

	HANDMADE_INTERNAL:
		0 - Build for public release
		1 - Build for devs only

	HANDMADE_SLOW:
		0 - No slow code allowed!
		1 - Slow code for testing is used

*/

#if HANDMADE_SLOW
#define Assert(Expression) if (!(Expression)) {*(int *)0 = 5;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

/*
 *	NOTE: Services that the platform layer provides to the game
 * */

inline uint32_t SafeTruncateUInt64(uint64_t Value)
{
	// TODO: defines for max values
	Assert(Value <= 0xFFFFFFFF);
	return ((uint32_t)Value);
}


// STUDY: Preprocessor stuff
//#if HANDMADE_INTERNAL

struct debug_read_file_result {
	uint32_t ContentsSize;
	void *Contents;
};

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);

internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint64_t MemorySize, void *Memory);
//#endif

/*
 *	NOTE: Services that the game provides to the platform layer
 *	(this may expand in the future - sound on separate thread, etc)
 * */

struct game_offscreen_buffer {
	void *Memory;
	int Width;
	int Height;
};

struct game_sound_output_buffer {
	int SamplesPerSecond;
	int SampleCount;
	int16_t *Samples;
};

struct game_button_state {
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input {
	bool32 IsAnalog;

	real32 StartX;
	real32 EndX;
	real32 StartY;
	real32 EndY;

	real32 MinX;
	real32 MinY;
	real32 MaxX;
	real32 MaxY;

	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};

struct game_input {
	game_controller_input Controllers[4];	
};

struct game_state {
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
	int RedOffset;
};

struct game_memory {
	bool32 IsInitialized;
	uint64_t PermanentStorageSize;
	void *PermanentStorage;	// NOTE: memory needs to be cleared to zero on every platform

	uint64_t TransientStorageSize;
	void *TransientStorage;
};

// FOUR THINGS: Timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
void GameUpdateAndRender(game_memory *GameMemory,
					     game_offscreen_buffer* Buffer,
					     game_sound_output_buffer *SoundBuffer,
					     game_input *Input);

#define HANDMADE_H
#endif
