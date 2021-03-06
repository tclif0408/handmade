/*	
 *	============================================
 *
 *	Handmade Hero / C Programming Tutorial
 *
 *	============================================
 *
 *
 *	NOTE: This is *NOT* the final platform layer
 *	
 *	- Saved game locations
 *	- Getting a handle to our own executable file
 *	- Asset loading path
 *	- threading/ launching threads
 *	- raw input (support for multiple keyboards)
 *	- sleep/timebeginperiod as to not melt any laptops
 *	- clipcursor() (for multimonitor support)
 *	- fullscreen support
 *	- wm_setcursor (control cursor visibility)
 *	- querycancelautoplay
 *	- wm_activateapp (for when we are not the active application)
 *	- blit speed improvements (bitblt)
 *	- hardware acceleration (opengl, direct3d, maybe both)
 *	- getkeyboardlayout (for french keyboards, internation WASD support)
 *
 *	^ just a partial list! ;^)
 ***/

#include <Windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <malloc.h>

// TODO: implement sine function by hand
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Tau32 6.28318530718f

typedef float real32;
typedef double real64;
typedef int32_t bool32;

struct win32_screen_buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension {
	int Width;
	int Height;
};

struct win32_sound_output {
	int ToneVolume;
	int SamplesPerSecond;
	int ToneHz;
	int LatencySampleCount;
	uint32_t RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
};

#include "handmade.cpp"

global_variable bool GlobalRunning;
global_variable win32_screen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64_t GlobalPerfCountFrequency;

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
{
	// Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if (DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate =
			(direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		// Double-check that this works on other versions of windows
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {}; 
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC PrimaryBufferDescription = {};
				PrimaryBufferDescription.dwSize = sizeof(PrimaryBufferDescription);
				PrimaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				// Create a primary buffer
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(
					&PrimaryBufferDescription, &PrimaryBuffer, 0)))
				{
					
					if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						// We have finally set the format! :D
					}
					else
					{
						// TODO: Diagnostics
					}
				}
				else
				{
					// TODO: Diagnostics
				}
			}
			else
			{
				// TODO: Diagnostics
			}


			DSBUFFERDESC SecondaryBufferDescription = {};
			SecondaryBufferDescription.dwSize = sizeof(SecondaryBufferDescription);
			SecondaryBufferDescription.dwFlags = 0;
			SecondaryBufferDescription.dwBufferBytes = BufferSize;
			SecondaryBufferDescription.lpwfxFormat = &WaveFormat;
			
			// Create secondary buffer
			if (SUCCEEDED(DirectSound->CreateSoundBuffer(
				&SecondaryBufferDescription, &GlobalSecondaryBuffer, 0)))
			{
			}
			
		}
		else
		{
			// TODO: Diagnostics
		}
	}
}

internal void Win32LoadXInput(void)
{
	// TODO: Fix for windows 8
	
	// Try to load XInput 1.4, if that doesn't work go to 1.3
	HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
	if (!XInputLibrary)
	{
		// TODO: Diagnostics
		XInputLibrary = LoadLibrary("xinput1_3.dll");
	}
	
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;	
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			Result.ContentsSize = SafeTruncateUInt64(FileSize.QuadPart);		// files shouldnt be too big
			Result.Contents = VirtualAlloc(0, Result.ContentsSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, Result.ContentsSize, &BytesRead, 0) && (Result.ContentsSize == BytesRead))
				{
					// File read successfully
				}
				else
				{
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				// TODO: Logging
			}	
		}
		else
		{
			// TODO: Logging
		}

		CloseHandle(FileHandle);
	}
	else
	{
		// TODO: Logging
	}

	return (Result);
}

internal void DEBUGPlatformFreeFileMemory(void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory)
{
	bool32 Result = false;

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			// File was read successfully
			Result = (BytesWritten == MemorySize);
		}		
		else
		{
			// TODO: Logging
		}
	}
	else
	{
		// TODO: Logging
	}

	return (Result);
}

internal win32_window_dimension Win32GetWindowDimensions(HWND Window)
{
	win32_window_dimension Result;
	RECT ClientRect;
	
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	
	return(Result);
}

internal void Win32ResizeDIBSection(win32_screen_buffer *Bitmap, int Width, int Height)
{	
	if(Bitmap->Memory)
	{
		VirtualFree(Bitmap->Memory, 0, MEM_RELEASE);
	}
	
	GlobalBackBuffer.BytesPerPixel = 4;
	GlobalBackBuffer.Pitch = Width * GlobalBackBuffer.BytesPerPixel;
	
	Bitmap->Width = Width;
	Bitmap->Height = Height;
	
	Bitmap->Info.bmiHeader.biSize = sizeof(Bitmap->Info.bmiHeader);
	Bitmap->Info.bmiHeader.biWidth = Bitmap->Width;
	Bitmap->Info.bmiHeader.biHeight = -Bitmap->Height;
	Bitmap->Info.bmiHeader.biPlanes = 1;
	Bitmap->Info.bmiHeader.biBitCount = 32;
	Bitmap->Info.bmiHeader.biCompression = BI_RGB;	
	
	int BitmapMemorySize = Width * Height * GlobalBackBuffer.BytesPerPixel;
	Bitmap->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

internal void Win32DisplayBufferInWindow(win32_screen_buffer *Bitmap, HDC DeviceContext, int Width, int Height)
{
	// TODO: Aspect ratio correction.
	// TODO: Screw with stretch modes.
	StretchDIBits(DeviceContext, 0, 0, Width, Height, 0, 0, Bitmap->Width, Bitmap->Height, Bitmap->Memory, &Bitmap->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;
	
	switch(Message)
	{		
		case WM_DESTROY:
		{
			// Temporary -- Handle as error, recreate window?
			GlobalRunning = false;
		} break;
		
		case WM_CLOSE:
		{
			// Temporary -- Handle with message to user?
			GlobalRunning = false;
		} break;
		
		case WM_ACTIVATEAPP:
		{
		} break;
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Assert(!"This code shouldn't execute");
		} break;
		
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			win32_window_dimension Dimension = Win32GetWindowDimensions(Window);
			Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		} break;
		
		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}
	
	return (Result);
}

void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
	VOID *Region1;
	DWORD Region1Size;	
	VOID *Region2;
	DWORD Region2Size;

	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		// TODO: assert region sizes are valid
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
		int16_t *DestSample = (int16_t *)Region1;
		int16_t *SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		DestSample = (int16_t *)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		// unlock
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}			
}

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;	
	VOID *Region2;
	DWORD Region2Size;

	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size, 0)))
	{
		uint8_t *DestSample = (uint8_t *)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		
		DestSample = (uint8_t *)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
	Assert(NewState->EndedDown != IsDown);
	NewState->EndedDown = IsDown;
	++NewState->HalfTransitionCount;
}

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
	MSG Message;

	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		if (Message.message == WM_QUIT)
		{
			GlobalRunning = false;
		}
		switch (Message.message)
		{
			// Handle keyboard input
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32_t VKCode = (uint32_t)Message.wParam;
				bool WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool IsDown = ((Message.lParam & (1 << 31)) == 0);
				
				if (WasDown != IsDown)
				{
					if (VKCode == 'W')
					{
						
					}
					else if (VKCode == 'A')
					{
					
					}
					else if (VKCode == 'S')
					{
					
					}
					else if (VKCode == 'D')
					{
					
					}
					else if (VKCode == 'Q')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if (VKCode == 'E')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
					}
					else if (VKCode == VK_DOWN)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Down, IsDown);
					}
					else if (VKCode == VK_UP)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Up, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Left, IsDown);
					}
					else if (VKCode == VK_RIGHT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Right, IsDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
						GlobalRunning = false;
					}
					else if (VKCode == VK_SPACE)
					{
						
					}
				}
			
				// Use Alt+F4 to close the game	
				bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
				if ((VKCode == VK_F4) && AltKeyWasDown)
				{
					GlobalRunning = false;
				}
			} break;
			
			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);			
			} break;
		}
	}
}

inline LARGE_INTEGER Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return (Result);
}
	
inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = (((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency));
	return (Result);
}

internal void Win32DebugDrawVertical(win32_screen_buffer *ScreenBuffer, int X, int Top, int Bottom, uint32_t Color)
{
	uint8_t *Pixel = (uint8_t *)ScreenBuffer->Memory + X * ScreenBuffer->BytesPerPixel + Top * ScreenBuffer->Pitch;
	for (int Y = Top; Y < Bottom; Y++)
	{
		*(uint32_t *)Pixel = Color;
		Pixel += ScreenBuffer->Pitch;
	}
}

internal void Win32DebugSyncDisplay(win32_screen_buffer *ScreenBuffer, DWORD *LastPlayCursor, size_t LastPlayCursorCount, win32_sound_output *SoundOutput, real32 TargetSecondsPerFrame)
{
	int PadX = 6;
	int PadY = 6;
	
	int Top = PadY;
	int Bottom = ScreenBuffer->Height - PadY;
	
	real32 C = ((real32)ScreenBuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;
	for (int unsigned PlayCursorIndex = 0; PlayCursorIndex < LastPlayCursorCount; PlayCursorIndex++)
	{
		int X = PadX + (int)(C * (real32)LastPlayCursor[PlayCursorIndex]);
		Win32DebugDrawVertical(ScreenBuffer, X, Top, Bottom, 0xFFFFFFFF);
	}
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
	WNDCLASSA WindowClass = {};
	
	Win32LoadXInput();
	
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "CoolGameWindowClass";
	
	// NOTE: set the windows scheduler's granularity to 1 ms so that our sleed can be more granular
	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	// TODO: How do we reliably figure this out on Windows?
	int MonitorRefreshHz = 60;
	
	int GameUpdateHz = MonitorRefreshHz / 2;
	real32 TargetSecondsPerFrame = 1.0f / (real32)MonitorRefreshHz;

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "Cool Game", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
		
		if (Window)
		{
			// NOTE: Since we specified CS_OWNDC, we can just get one device context and use it
			// forever because we are not sharing it with anyone
			HDC DeviceContext = GetDC(Window);

			// Sound test stuff
			win32_sound_output SoundOutput = {};
			SoundOutput.ToneVolume = 2000;
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			// TODO: Pool with bitmap virtualalloc
			int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

			// #if HANDMADE_INTERNAL
			// LPVOID BaseAddress = Terabytes((uint64_t)2);
			// #else
			// LPVOID BaseAddress = 0;
			// #endif
			
			LPVOID BaseAddress = 0;

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes((uint64_t)1);

			uint64_t TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = ((uint8_t *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			if (Samples && GameMemory.PermanentStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				int64_t LastCycleCount = __rdtsc();		// NOTE: __rdtsc() is somewhat hardware specific
				
				int DebugLastPlayCursorIndex = 0;
				DWORD DebugLastPlayCursor[30] = {};
				
				while (GlobalRunning)
				{
					// TODO: Zeroing macro
					game_controller_input *OldKeyboardController = &OldInput->Controllers[0];
					game_controller_input *NewKeyboardController = &NewInput->Controllers[0];
					
					game_controller_input ZeroController = {};
					*NewKeyboardController = ZeroController;
					
					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}
					
					Win32ProcessPendingMessages(NewKeyboardController);
					
					// TODO: Test this with an actual gamepad and
					//   verify that XInput was loaded correctly.
				
					DWORD MaxControllerCount = 1 + XUSER_MAX_COUNT;
					if (MaxControllerCount > ArrayCount(NewInput->Controllers))
					{
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}
		
					// TODO: Should we poll this more frequently?
					for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
					{
						DWORD OurControllerIndex = ControllerIndex + 1;
						game_controller_input *OldController = &OldInput->Controllers[OurControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[OurControllerIndex];

						XINPUT_STATE ControllerState;
						
						if (XInputGetState(OurControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							// TODO (maybe): Make gamepad input less crappy
							
							// If we got here, the controller is plugged in.
							// See if ControllerState.dwPacketNumber increments too much,
							//  (i.e. we're updating too slowly)
							
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
								
							bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;
							
							// TODO: min/max macros
							
							// TODO (later (maybe)): gamepad support
							real32 X = 0.0f;
							real32 Y = 0.0f;

							if (Pad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								X = (real32)Pad->sThumbLX / 32768.0f;
							}
							else if (Pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								X = (real32)Pad->sThumbLX / 32767.0f;
							}
							
							if (Pad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								Y = (real32)Pad->sThumbLY / 32768.0f;
							}
							else if (Pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								Y = (real32)Pad->sThumbLY / 32767.0f;
							}
							
							NewController->MinX = NewController->MaxX = NewController->EndX = X;

							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Down, XINPUT_GAMEPAD_A, &NewController->Down);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Right, XINPUT_GAMEPAD_B, &NewController->Right); 
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Left, XINPUT_GAMEPAD_X, &NewController->Left); 
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Up, XINPUT_GAMEPAD_Y, &NewController->Up); 
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder); 
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

							//bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
							//bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
							bool32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
							bool32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
							bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
							bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
							bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
							bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
						}
						else
						{
							// The controller is not available
						}
					}

					DWORD ByteToLock;
					DWORD TargetCursor;
					DWORD BytesToWrite;
					DWORD PlayCursor;
					DWORD WriteCursor;
					bool SoundIsValid = false;
					
					if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
					{
						ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
						TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);

						if (ByteToLock > TargetCursor)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}

						SoundIsValid = true;
					}

					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample; 
					SoundBuffer.Samples = Samples;

					game_offscreen_buffer Buffer = {}; 
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;

					GameUpdateAndRender(&GameMemory, &Buffer, &SoundBuffer, NewInput);

					if (SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}
					
					// NOTE: Sound timing is a little screwed up
					LARGE_INTEGER WorkCounter = Win32GetWallClock();
					real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
					
					real32 SecondsElapsedForFrame = WorkSecondsElapsed;
					if (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						if (SleepIsGranular)
						//while (SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
							if (SleepMS > 0)
							{
								Sleep(SleepMS);
								//SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
							}
						}
						
						real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						Assert(TestSecondsElapsedForFrame < TargetSecondsPerFrame);
						
						while(SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						}
					}
					else
					{
						// Missed frame! D:
						// TODO: Logging
					}
					
					win32_window_dimension Dimension = Win32GetWindowDimensions(Window);
					
					// NOTE: This is debug code
					#if HANDMADE_INTERNAL
					{
						Win32DebugSyncDisplay(&GlobalBackBuffer, DebugLastPlayCursor, ArrayCount(DebugLastPlayCursor), &SoundOutput, TargetSecondsPerFrame);
					}
					#endif
					
					Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
					
					// NOTE: This is debug code
					#if HANDMADE_INTERNAL
					{
						DWORD TestPlayCursor;
						DWORD TestWriteCursor;
						GlobalSecondaryBuffer->GetCurrentPosition(&TestPlayCursor, &TestWriteCursor);
						
						DebugLastPlayCursor[DebugLastPlayCursorIndex++] = TestPlayCursor;
						if (DebugLastPlayCursorIndex > ArrayCount(DebugLastPlayCursor))
						{
							DebugLastPlayCursorIndex = 0;
						}
						
					}
					#endif
					
					// real32 MSPerFrame = ((1000.0f * (real32)CounterElapsed) * (real32)PerfCountFrequency);
					// real32 FPS = (real32)PerfCountFrequency / (real32)CounterElapsed;
					// real32 MSPF = ((real32)CyclesElapsed / 1000.0f / 1000.0f);

					// Save printfs for debug code
					// char Buffer[256];
					// sprintf(Buffer, "%f ms/f,\t%ffps,\t%fmc/f\n", MSPerFrame, FPS, MSPF);
					// OutputDebugString(Buffer);
					
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
					
					LARGE_INTEGER EndCounter = Win32GetWallClock();
					LastCounter = EndCounter;
					
					int64_t EndCycleCount = __rdtsc();
					int64_t CyclesElapsed = EndCycleCount - LastCycleCount;
					LastCycleCount = EndCycleCount;
				}
			}
			else
			{
				// TODO: Logging
			}
		}
		else
		{
			// TODO: Logging
		}
	}
	else
	{
		// TODO: Logging
	}
	
	return(0);
}
