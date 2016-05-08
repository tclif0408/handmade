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

// TODO: implement sine function by hand
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

typedef float real32;
typedef double real64;

#define Tau32 6.28318530718f

typedef int32_t bool32;

struct win32_screen_buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
};

struct win32_window_dimension {
	int Width;
	int Height;
};

struct win32_sound_output {
	int ToneVolume;
	int SamplesPerSecond;
	int ToneHz;
	uint32_t RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
};

#include "handmade.cpp"

global_variable bool GlobalRunning;
global_variable win32_screen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

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
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&PrimaryBufferDescription, &PrimaryBuffer, 0)))
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
			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&SecondaryBufferDescription, &GlobalSecondaryBuffer, 0)))
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
		HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
	}
	
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
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
	
	int BytesPerPixel = 4;
	
	Bitmap->Width = Width;
	Bitmap->Height = Height;
	
	Bitmap->Info.bmiHeader.biSize = sizeof(Bitmap->Info.bmiHeader);
	Bitmap->Info.bmiHeader.biWidth = Bitmap->Width;
	Bitmap->Info.bmiHeader.biHeight = -Bitmap->Height;
	Bitmap->Info.bmiHeader.biPlanes = 1;
	Bitmap->Info.bmiHeader.biBitCount = 32;
	Bitmap->Info.bmiHeader.biCompression = BI_RGB;	
	
	int BitmapMemorySize = Width * Height * BytesPerPixel;
	Bitmap->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

	//RenderWeirdGradient(Bitmap);
}

internal void Win32UpdateWindow(win32_screen_buffer *Bitmap, HDC DeviceContext, int Width, int Height)
{
	// TODO: Aspect ratio correction.
	// TODO: Screw with stretch modes.
	StretchDIBits(
		DeviceContext,
		0, 0, Width, Height, 
		0, 0, Bitmap->Width, Bitmap->Height,
		Bitmap->Memory,
		&Bitmap->Info,
		DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;;
	
	switch(Message)
	{		
		case WM_DESTROY:
		{
			// Temporary -- Handle as error, recreate window?
			GlobalRunning = false;
		} break;
		
		// Handle keyboard input
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32_t VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1 << 31)) == 0);
			
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
				
				}
				else if (VKCode == 'E')
				{
					
				}
				else if (VKCode == VK_DOWN)
				{
					
				}
				else if (VKCode == VK_UP)
				{
					
				}
				else if (VKCode == VK_LEFT)
				{
					
				}
				else if (VKCode == VK_RIGHT)
				{
					
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
			bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				GlobalRunning = false;
			}
		} break;
		
		case WM_CLOSE:
		{
			// Temporary -- Handle with message to user?
			GlobalRunning = false;
		} break;
		
		case WM_ACTIVATEAPP:
		{
		} break;
		
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			win32_window_dimension Dimension = Win32GetWindowDimensions(Window);
			Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
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

	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size, 0)))
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

internal int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
	Win32LoadXInput();
	
	WNDCLASSA WindowClass = {};
	
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "CoolGameWindowClass";

	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64_t PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(
			0,
			WindowClass.lpszClassName,
			"Cool Game",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0
			
		);
		
		if (Window)
		{
			HDC DeviceContext = GetDC(Window);

			// Graphics test stuff
			int XOffset = 0;
			int YOffset = 0;

			// Sound test stuff
			win32_sound_output SoundOutput = {};
			SoundOutput.ToneVolume = 2000;
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16_t)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			int64_t LastCycleCount = __rdtsc();

			GlobalRunning = true;
			while (GlobalRunning)
			{
				MSG Message;
				
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				
				// TODO: Test this with an actual gamepad and verify that XInput was loaded correctly.
				
				// TODO: Should we poll this more frequently?
				for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// If we got here, the controller is plugged in.
						// See if ControllerState.dwPacketNumber increments too much,
						//  (i.e. we're updating too slowly)
						
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						
						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
						
						int16_t StickX = Pad->sThumbLX;
						int16_t StickY = Pad->sThumbLY;
						
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
					ByteToLock = ((SoundOutput->RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput->SecondaryBufferSize);
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

				int16_t Samples[48000 / 30 * 2];
				game_sound_output_buffer SoundBuffer = {};
				SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
				SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample; 
				SoundBuffer.Samples = Samples;

				game_offscreen_buffer Buffer = {}; 
				Buffer.Memory = GlobalBackBuffer.Memory;
				Buffer.Width = GlobalBackBuffer.Width;
				Buffer.Height = GlobalBackBuffer.Height;

				GameUpdateAndRender(&Buffer, &SoundBuffer);

				// DirectSound output test
				if (SoundIsValid)
				{
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
				}

								
				win32_window_dimension Dimension = Win32GetWindowDimensions(Window);
				Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				
				int64_t EndCycleCount = __rdtsc();

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				int64_t CyclesElapsed = EndCycleCount - LastCycleCount;
				int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				real32 MSPerFrame = ((1000.0f * (real32)CounterElapsed) / (real32)PerfCountFrequency);
				real32 FPS = (real32)PerfCountFrequency / (real32)CounterElapsed;
			        real32 MSPF = ((real32)CyclesElapsed / 1000.0f / 1000.0f);	

#if 0
				// Save printfs for debug code
				char Buffer[256];
				sprintf(Buffer, "%f ms/f,\t%ffps,\t%fmc/f\n", MSPerFrame, FPS, MSPF);
				OutputDebugString(Buffer);
#endif
				
				LastCounter = EndCounter;
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
	
	return(0);
}
