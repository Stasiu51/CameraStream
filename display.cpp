/*
This code for displaying the images to the screen and colouring them for clarity. 
*/
//Using SDL and standard IO

#include <SDL.h>
#include <stdio.h>
#include <windows.h>
#include "display.h"


using namespace std;

//Screen dimension constants
const int SCREEN_WIDTH = 640*1.5;
const int SCREEN_HEIGHT = 480*1.5;

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

SDL_Event evt;

//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;

/*
Data for the colour map. All arrays should be the same length. Each entry represents one 'anchor point' - if the input value (0-255) is exactly one of those specified then the RGB values are read from the arrays. Else they are linearly interpolated.
*/

int gradpoints[] = { 0, 43, 86, 129, 172, 215, 256 };
int gradR[] = { 0, 0, 0, 0, 255, 255, 255};
int gradG[] = {0,0,255,255,255,0,255};
int gradB[] = {0,255,255,0,0,0,255};


bool initWindow()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Create window
		gWindow = SDL_CreateWindow("CameraDisplay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Get window surface
			gScreenSurface = SDL_GetWindowSurface(gWindow);
		}
	}

	return success;
}

inline unsigned long long bitshift(unsigned long long x) {
	//Don't ask. Unscrambles the rows at a fine level by switching the bits around, eg  ...010100111aabb -> ...010100111bbaa
	return (x & (~0b1111)) | ((x & 0b1100) >> 2) | ((x & 0b0011) << 2);
}

void copy_rearrange(char* src, char* dst, long long width, long long height) {
	//This the code for copying the image data to the screen buffer in such a way that it unscrambles the scrambling applied by the camera.
	unsigned long long i;
	long long h_height = height/2;
	for (i = 0; i < h_height; i++) {
		memcpy(dst + bitshift(height - i - 1) * width, src + (2 * i)*width, width);
	}
	for (i = 0; i < h_height; i++) {
		memcpy(dst + bitshift(i) * width, src + (2 * i + 1) * width, width);
	}
}


DWORD WINAPI displayThread(LPVOID args)
{
	//Thread code for display window.

	// DisplayArgs object passed from main thread with parameters for the display.
	printf("Started display thread.\n");
	DisplayArgs* d_args = (DisplayArgs*)args;
	char* imCache = d_args->imCache;
	int imCache_N= d_args->imCache_N;
	long long imWidth = d_args->imWidth;
	long long imHeight = d_args->imHeight;
	unsigned long long imSize = d_args->imSize;
	
	char* buffer = (char*) malloc(imWidth * imHeight); // Allocate memeory for the rearranged image for display. We will copy to this from the image cache.
	
	SDL_Color colours[256];

	//Create palette
	int a, b, j = 0;
	for (int i = 0; i < 256; i++) {
		while (gradpoints[j+1] < i) j++;
		int norm = gradpoints[j + 1] - gradpoints[j];
		a = i - gradpoints[j];
		b = gradpoints[j+1] - i;
		//linear combination of adjacent colour anchors.
		colours[i].r = (a * gradR[j + 1] + b * gradR[j]) / norm;
		colours[i].g = (a * gradG[j + 1] + b * gradG[j]) / norm;
		colours[i].b = (a * gradB[j + 1] + b * gradB[j]) / norm;

	}

	SDL_Surface* imSurf;
	SDL_Surface* midSurf;
	printf("SDl er: %s\n", SDL_GetError());
	//Start up SDL and create window
	if (!initWindow())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		printf("Starting display loop...\n");
		//Get the format to which we must convert the surfaces for display.
		SDL_PixelFormat* format = SDL_AllocFormat(SDL_GetWindowPixelFormat(gWindow));
		int latest_frame, frame_cache_index, frame_buffer_index;
		char* srcImg;//pointer to image data in image cache.
		imSurf = SDL_CreateRGBSurfaceWithFormatFrom(0, imWidth, imHeight, 8, imWidth, SDL_PIXELFORMAT_INDEX8);
		while (*d_args->global_frame_count == 0) {//Hasn't started yet; do nothing}
		while (!*(d_args->done)) {
			latest_frame = *d_args->global_frame_count - 1; // This is the frame we will display.
			frame_cache_index = latest_frame % imCache_N; // This is its index in the cache.
			srcImg = imCache + imSize * frame_cache_index; // This is its address.
			copy_rearrange(srcImg, buffer, imWidth, imHeight); // copy it into the display cache that we malloc'ed.
			imSurf = SDL_CreateRGBSurfaceWithFormatFrom(buffer, imWidth, imHeight, 8, imWidth, SDL_PIXELFORMAT_INDEX8);
			
			SDL_SetPaletteColors(imSurf->format->palette, colours, 0, 256);
			midSurf = SDL_ConvertSurface(imSurf, format,0);// have to have an intermediate buffer to do the conversion, otherwise doesn't work.
			SDL_BlitScaled(midSurf, NULL, gScreenSurface, NULL); // scale and copy the image onto the screen surface.

			SDL_UpdateWindowSurface(gWindow);
			//Don't forget to free memory!
			SDL_FreeSurface(imSurf);
			SDL_FreeSurface(midSurf);
			SDL_PollEvent(&evt); //Stops it being unresponsive, allows you move window etc.
			
			SDL_Delay(100); // Adjust the framerate uisng this! Too high and it ends up causing more frame drops (not sure why).
		}
		printf("Ended Display Thread\n");
	}


	//Destroy window
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}