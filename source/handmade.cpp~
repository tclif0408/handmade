
/*========================================

	handmade.cpp

	=========================================*/

#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer)
{
	local_persist real32 tSine;
	local_persist int16_t ToneVolume = 3000;
	int ToneHz = 256;
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

internal void RenderWeirdGradient(game_offscreen_buffer *Bitmap)
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
			uint8_t Red = (uint8_t)(x);
			uint8_t Green = (uint8_t)(y);
			uint8_t Blue = (uint8_t)(x);
			
			*Pixel++ = ((Red << 16 ) | (Green << 8) | Blue);
		}
		
		Row += Pitch;
	}
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer)
{
	// TODO: Make sound output more flexible.
	GameOutputSound(SoundBuffer);
	RenderWeirdGradient(Buffer);
}

