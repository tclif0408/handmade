internal void RenderWeirdGradient(win32_screen_buffer *Bitmap, int XOffset, int YOffset)
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
			uint8_t Red = (uint8_t)((y - XOffset) * TAU);
			uint8_t Green = (x + YOffset);
			uint8_t Blue = (x * XOffset) - Green;
			
			*Pixel++ = ((Red << 16 ) | (Green << 8) | Blue);
		}
		
		Row += Pitch;
	}
}
