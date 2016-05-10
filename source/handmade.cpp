
/*========================================

	handmade.cpp

	=========================================*/

#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	local_persist real32 tSine;
	local_persist int16_t ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16_t *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
	{
		real32 SineValue = sinf(tSine);
		int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += Tau32 * 1.0f / (real32)(WavePeriod);
	}
}

internal void RenderWeirdGradient(game_offscreen_buffer *Bitmap, int off1, int off2, int off3)
{
	int Width = Bitmap->Width;
	int Height = Bitmap->Height;
	int BytesPerPixel = 4;
	
	int Pitch = Width * BytesPerPixel;
	uint8_t *Row = (uint8_t *)Bitmap->Memory;
	
	for (int y = 0; y < Height; y++)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		
		for (int x = 0; x < Width; x++)
		{
			uint8_t Red = (uint8_t)(x + off1);
			uint8_t Green = (uint8_t)(y - off2);
			uint8_t Blue = (uint8_t)(x + off3);
			
			*Pixel++ = ((Red << 16 ) | (Green << 8) | Blue);
		}
		
		Row += Pitch;
	}
}

internal void GameUpdateAndRender(game_memory *GameMemory,
								  game_offscreen_buffer *Buffer,
								  game_sound_output_buffer *SoundBuffer,
								  game_input *Input)
{
	Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);

	game_state *GameState = (game_state *)GameMemory->PermanentStorage;	
	if(!GameMemory->IsInitialized)
	{
		GameState->ToneHz = 256;

		// NOTE: This may be a more appropriate task for the platform layer
		GameMemory->IsInitialized = true;
	}
	
	// NOTE: Test controller input!
	
	if (Input->Controllers[0].Up.EndedDown)
	{
		GameState->RedOffset += 5;
	}
	if (Input->Controllers[0].Down.EndedDown)
	{
		GameState->RedOffset -= 5;
	}
	if (Input->Controllers[0].Left.EndedDown)
	{
		GameState->GreenOffset += 7;
	}
	if (Input->Controllers[0].Right.EndedDown)
	{
		GameState->BlueOffset += 7;
	}
	if (Input->Controllers[0].LeftShoulder.EndedDown)
	{
		GameState->GreenOffset += (GameState->RedOffset * GameState->BlueOffset);
	}
	if (Input->Controllers[0].RightShoulder.EndedDown)
	{
		if (GameState->RedOffset != 0 && GameState->BlueOffset != 0)
		{
			GameState->GreenOffset += (GameState->RedOffset / GameState->BlueOffset);
		}
		else
		{
			GameState->GreenOffset += (GameState->RedOffset - GameState->BlueOffset);
		}		
	}
	

	// TODO: Make sound output more flexible.
	GameOutputSound(SoundBuffer, GameState->ToneHz);
	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset, GameState->RedOffset);
}

