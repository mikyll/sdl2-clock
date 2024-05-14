#include <stdio.h>
#include <math.h>
#include <time.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#define APP_WIDTH 400
#define APP_HEIGHT 500
#define FPS_CAP 60
#define MINUTES_ANGLE_INCREMENT 360.0f / 60
#define HOURS_ANGLE_INCREMENT 360.0f / 12

#define DEFAULT_FONT_SIZE 24

typedef struct tm timestamp;

const SDL_Color BLACK = { 0x00, 0x00, 0x00, 0x00 };

void initSDL(void);
void cleanupSDL(void);
float calculateDeltaTime(void);
void capFrameRate(Uint32 initFrameTime, Uint32 fpsCap);
float calculateFPS(float deltaTime);
void doInput(void);
void drawText(TTF_Font* font, char* text, int x, int y, int size, SDL_Color color);
timestamp doTime(float deltaTime);
void drawClock(TTF_Font* font, timestamp timeInfo);
void drawFPS(TTF_Font* font, int fps);
void drawCurrentTime(TTF_Font* font, timestamp tmInfo);
int secure_sprintf(char* buffer, size_t bufferSize, const char* format, ...);

SDL_Window* window;
SDL_Renderer* renderer;

int main(int argc, char** argv)
{
	Uint32 initFrameTime = 0.0f;
	float deltaTime = 0.0f;
	float fps = 0.0f;

	initSDL();
	atexit(cleanupSDL);

	TTF_Font* digitalFont = TTF_OpenFont("resources/fonts/digital-7 (mono).ttf", DEFAULT_FONT_SIZE);
	if (!digitalFont)
	{
		printf("Couldn't open font: %s\n", TTF_GetError());
		exit(1);
	}

	while (1)
	{
		initFrameTime = SDL_GetTicks();
		deltaTime = calculateDeltaTime();

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		doInput();

		timestamp timeInfo = doTime(deltaTime);

		drawClock(digitalFont, timeInfo);
		drawFPS(digitalFont, fps);
		drawCurrentTime(digitalFont, timeInfo);

		// Present scene
		SDL_RenderPresent(renderer);

		fps = calculateFPS(deltaTime);
		capFrameRate(initFrameTime, FPS_CAP);
	}

	return 0;
}





void initSDL(void)
{
	int rendererFlags, windowFlags;

	rendererFlags = SDL_RENDERER_ACCELERATED;

	windowFlags = SDL_WINDOW_SHOWN;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	window = SDL_CreateWindow(
		"SDL2 Clock",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		APP_WIDTH,
		APP_HEIGHT,
		windowFlags
	);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	renderer = SDL_CreateRenderer(window, -1, rendererFlags);

	if (TTF_Init())
	{
		printf("Couldn't initialize SDL_ttf: %s\n", TTF_GetError());
		exit(1);
	}
}

void cleanupSDL(void)
{
	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);

	SDL_Quit();
}

float calculateDeltaTime(void)
{
	static Uint32 lastFrameTime = 0.0f;
	float deltaTime;

	deltaTime = (float)(SDL_GetTicks() - lastFrameTime) / 1000.0f;
	lastFrameTime = SDL_GetTicks();

	// Clamp maximum delta time value
	if (deltaTime > 0.05f)
		deltaTime = 0.05f;

	return deltaTime;
}

void capFrameRate(Uint32 initFrameTime, Uint32 fpsCap)
{
	Uint32 frameTime = SDL_GetTicks() - initFrameTime;
	Uint32 frameDurationMS = 1000.0f / fpsCap;

	if (frameTime < frameDurationMS)
		SDL_Delay(frameDurationMS - frameTime);
}

float calculateFPS(float deltaTime)
{
	const float smoothingFactor = 0.1f;

	float instantFPS = 1.0f / deltaTime;

	static float smoothedFPS = 0.0f;
	static int isFirstFrame = 1;

	// If it's the first frame, set smoothed FPS to the instant FPS
	if (isFirstFrame)
	{
		smoothedFPS = instantFPS;
		isFirstFrame = 0;
	}
	else
	{
		// Otherwise, update smoothed FPS using EWMA
		smoothedFPS = smoothingFactor * instantFPS + (1.0f - smoothingFactor) * smoothedFPS;
	}

	return smoothedFPS;
}

void doInput(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			exit(0);
			break;

		default:
			break;
		}
	}
}

void drawText(TTF_Font* font, char* text, int x, int y, int size, SDL_Color color)
{
	if (TTF_SetFontSize(font, size) != 0)
	{
		printf("Couldn't change the font size: %s\n", TTF_GetError());
	}

	SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, text, color);
	SDL_Texture* textureMessage = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

	SDL_Rect messageRect;
	messageRect.x = x;
	messageRect.y = y;
	messageRect.w = surfaceMessage->w;
	messageRect.h = surfaceMessage->h;

	SDL_RenderCopy(renderer, textureMessage, NULL, &messageRect);

	SDL_FreeSurface(surfaceMessage);
	SDL_DestroyTexture(textureMessage);
}

timestamp doTime(float deltaTime)
{
	time_t currentTime = time(NULL);
	timestamp* timeInfo = localtime(&currentTime);
	return *timeInfo;
}

void drawClock(TTF_Font* font, timestamp timeInfo)
{
	float angle, x, y;
	int hours, minutes, seconds;
	char buffer[16];

	seconds = timeInfo.tm_sec;
	minutes = timeInfo.tm_min;
	hours = timeInfo.tm_hour;

	// Drak ticks
	for (int i = 1; i < 13; i++)
	{
		angle = i * HOURS_ANGLE_INCREMENT;
		x = (APP_WIDTH / 2) - 15 + 175 * sin(angle * M_PI / 180.0);
		y = APP_HEIGHT / 2 + 175 * -cos(angle * M_PI / 180.0);

		if (secure_sprintf(buffer, sizeof(buffer), "%2d", i) < 0)
			return;

		drawText(font, buffer, x, y, DEFAULT_FONT_SIZE, BLACK);
	}

	// Draw seconds
	for (int i = 0; i < 6; i++)
	{
		angle = seconds * MINUTES_ANGLE_INCREMENT;
		x = (APP_WIDTH / 2) - 15 + (i * 150 / 6) * sin(angle * M_PI / 180.0);
		y = APP_HEIGHT / 2 + (i * 150 / 6) * -cos(angle * M_PI / 180.0);

		if (secure_sprintf(buffer, sizeof(buffer), "%02d", seconds) < 0)
			return;

		drawText(font, buffer, x, y, DEFAULT_FONT_SIZE, BLACK);
	}
	
	// Draw minutes
	for (int i = 1; i < 5; i++)
	{
		angle = minutes * MINUTES_ANGLE_INCREMENT;
		x = (APP_WIDTH / 2) - 15 + (i * 120 / 4) * sin(angle * M_PI / 180.0);
		y = APP_HEIGHT / 2 + (i * 120 / 4) * -cos(angle * M_PI / 180.0);

		if (secure_sprintf(buffer, sizeof(buffer), "%02d", minutes) < 0)
			return;

		drawText(font, buffer, x, y, DEFAULT_FONT_SIZE, BLACK);
	}

	// Draw hours
	for (int i = 1; i < 4; i++)
	{
		angle = hours * HOURS_ANGLE_INCREMENT;
		x = (APP_WIDTH / 2) - 15 + (i * 75 / 3) * sin(angle * M_PI / 180.0);
		y = APP_HEIGHT / 2 + (i * 75 / 3) * -cos(angle * M_PI / 180.0);

		if (secure_sprintf(buffer, sizeof(buffer), "%02d", hours) < 0)
			return;


		drawText(font, buffer, x, y, DEFAULT_FONT_SIZE, BLACK);
	}
}

void drawFPS(TTF_Font* font, int fps)
{
	char buffer[16];
	if (secure_sprintf(buffer, sizeof(buffer), "%5d", fps) < 0)
		return;

	drawText(font, buffer, APP_WIDTH - 60, 10, DEFAULT_FONT_SIZE, BLACK);
}

void drawCurrentTime(TTF_Font* font, timestamp timeInfo)
{
	char dateBuffer[32];
	char timeBuffer[32];

	if (secure_sprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%02d", timeInfo.tm_year - 100, timeInfo.tm_mon + 1, timeInfo.tm_mday) < 0)
		return;

	if (secure_sprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec) < 0)
		return;

	drawText(font, dateBuffer, 5, APP_HEIGHT - 25, DEFAULT_FONT_SIZE, BLACK);
	drawText(font, timeBuffer, APP_WIDTH - 95, APP_HEIGHT - 25, DEFAULT_FONT_SIZE, BLACK);
}

int secure_sprintf(char* buffer, size_t bufferSize, const char* format, ...)
{
	if (buffer == NULL || bufferSize == 0 || format == NULL || bufferSize >= 1024)
	{
		return -1;
	}

	va_list args;
	va_start(args, format);

	char tempBuffer[1024];
	int result = vsnprintf(tempBuffer, sizeof(tempBuffer), format, args);

	va_end(args);

	if (result < 0)
	{
		return result;
	}

	if ((size_t)result >= bufferSize)
	{
		return -1;
	}

	va_start(args, format);
	vsnprintf(buffer, bufferSize, format, args);
	va_end(args);

	return result;
}