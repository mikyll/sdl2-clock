#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#define DEFAULT_APP_WIDTH 400
#define DEFAULT_APP_HEIGHT 500
#define MIN_WIDTH 200
#define MIN_HEIGHT (MIN_WIDTH + MIN_WIDTH / 5)
#define MAX_WIDTH 1000
#define MAX_HEIGHT (MAX_WIDTH + MAX_WIDTH / 5)
#define DEFAULT_APP_HEIGHT 500
#define FPS_CAP 60
#define MINUTES_ANGLE_INCREMENT 360.0f / 60
#define HOURS_ANGLE_INCREMENT 360.0f / 12
#define DEFAULT_FONT_SIZE 24
const SDL_Color OFF_COLOR = { 230, 230, 230, 255 };
const SDL_Color ON_COLOR = { 0x00, 0x00, 0x00, 0xFF };

typedef struct tm timestamp;
typedef struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height;
	int enableHourMarks;
	int enableOffMarks;
} App;

int isInteger(const char* str);
int secure_sprintf(char* buffer, size_t bufferSize, const char* format, ...);

void parseArgs(int argc, char** argv);
void initSDL(void);
void cleanupSDL(void);

float calculateDeltaTime(void);
void capFrameRate(Uint32 initFrameTime, Uint32 fpsCap);
float calculateFPS(float deltaTime);


void doInput(void);
timestamp doTime(float deltaTime);

void drawText(TTF_Font* font, char* text, int x, int y, int size, SDL_Color color);
void drawClock(TTF_Font* font, timestamp timeInfo, int size);
void drawFPS(TTF_Font* font, int fps);
void drawCurrentTime(TTF_Font* font, timestamp tmInfo);

App app;

int main(int argc, char** argv)
{
	Uint32 initFrameTime = 0.0f;
	float deltaTime = 0.0f;
	float fps = 0.0f;

	parseArgs(argc, argv);

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

		SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255);
		SDL_RenderClear(app.renderer);

		doInput();

		timestamp timeInfo = doTime(deltaTime);

		drawClock(digitalFont, timeInfo, DEFAULT_FONT_SIZE);
		drawFPS(digitalFont, fps);
		drawCurrentTime(digitalFont, timeInfo);

		// Present scene
		SDL_RenderPresent(app.renderer);

		fps = calculateFPS(deltaTime);
		capFrameRate(initFrameTime, FPS_CAP);
	}

	return 0;
}


int isInteger(const char* str)
{
	char* endptr;
	errno = 0;
	long val = strtol(str, &endptr, 10);

	if (errno != 0 || *endptr != '\0' || endptr == str)
		return 0;

	return 1;
}

void parseArgs(int argc, char** argv)
{
	memset(&app, 0, sizeof(App));

	app.width = DEFAULT_APP_WIDTH;
	app.height = DEFAULT_APP_HEIGHT;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			//printUsage(argv[0]);
			return 0;
		}
		else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--hour-marks") == 0)
		{
			app.enableHourMarks = 1;
		}
		else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--off-marks") == 0)
		{
			app.enableOffMarks = 1;
		}
		else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0)
		{
			if (i + 1 < argc)
			{
				i++;
				if (isInteger(argv[i]) && argv[i] > MIN_WIDTH && argv[i] < MAX_WIDTH)
				{
					app.width = argv[i];
				}
				else
				{
					fprintf(stderr, "Option %s requires an integer argument between %d and %d\n", argv[i], MIN_WIDTH, MAX_WIDTH);
					return 1;
				}
			}
			else
			{
				fprintf(stderr, "Option %s requires an argument\n", argv[i]);
				return 1;
			}
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--height") == 0)
		{
			if (i + 1 < argc)
			{
				i++;
				if (isInteger(argv[i]) && argv[i] > MIN_HEIGHT && argv[i] < MAX_HEIGHT)
				{
					app.width = argv[i];
				}
				else
				{
					fprintf(stderr, "Option %s requires an integer argument between %d and %d\n", argv[i], MIN_HEIGHT, MAX_HEIGHT);
					return 1;
				}
			}
			else
			{
				fprintf(stderr, "Option %s requires an argument\n", argv[i]);
				return 1;
			}
		}
	}
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

	app.window = SDL_CreateWindow(
		"SDL2 Clock",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		DEFAULT_APP_WIDTH,
		DEFAULT_APP_HEIGHT,
		windowFlags
	);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	app.renderer = SDL_CreateRenderer(app.window, -1, rendererFlags);

	if (TTF_Init())
	{
		printf("Couldn't initialize SDL_ttf: %s\n", TTF_GetError());
		exit(1);
	}
}

void cleanupSDL(void)
{
	SDL_DestroyRenderer(app.renderer);

	SDL_DestroyWindow(app.window);

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
	SDL_Texture* textureMessage = SDL_CreateTextureFromSurface(app.renderer, surfaceMessage);

	SDL_Rect messageRect;
	messageRect.x = x;
	messageRect.y = y;
	messageRect.w = surfaceMessage->w;
	messageRect.h = surfaceMessage->h;

	SDL_RenderCopy(app.renderer, textureMessage, NULL, &messageRect);

	SDL_FreeSurface(surfaceMessage);
	SDL_DestroyTexture(textureMessage);
}

timestamp doTime(float deltaTime)
{
	time_t currentTime = time(NULL);
	timestamp* timeInfo = localtime(&currentTime);
	return *timeInfo;
}

void drawClock(TTF_Font* font, timestamp timeInfo, int size)
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
		x = (DEFAULT_APP_WIDTH / 2) - 15 + 175 * sin(angle * M_PI / 180.0);
		y = DEFAULT_APP_HEIGHT / 2 + 175 * -cos(angle * M_PI / 180.0);

		if (app.enableOffMarks)
		{
			drawText(font, "88", x, y, size, OFF_COLOR);
		}

		if (secure_sprintf(buffer, sizeof(buffer), "%2d", i) < 0)
			return;
		drawText(font, buffer, x, y, size, ON_COLOR);
	}

	// Draw seconds
	for (int i = 0; i < 6; i++)
	{
		angle = seconds * MINUTES_ANGLE_INCREMENT;
		x = (DEFAULT_APP_WIDTH / 2) - 15 + (i * 150 / 6) * sin(angle * M_PI / 180.0);
		y = DEFAULT_APP_HEIGHT / 2 + (i * 150 / 6) * -cos(angle * M_PI / 180.0);

		if (app.enableOffMarks)
		{
			drawText(font, "88", x, y, size, OFF_COLOR);
		}

		if (secure_sprintf(buffer, sizeof(buffer), "%02d", seconds) < 0)
			return;
		drawText(font, buffer, x, y, size, ON_COLOR);
	}
	
	// Draw minutes
	for (int i = 1; i < 5; i++)
	{
		angle = minutes * MINUTES_ANGLE_INCREMENT;
		x = (DEFAULT_APP_WIDTH / 2) - 15 + (i * 120 / 4) * sin(angle * M_PI / 180.0);
		y = DEFAULT_APP_HEIGHT / 2 + (i * 120 / 4) * -cos(angle * M_PI / 180.0);

		if (app.enableOffMarks)
		{
			drawText(font, "88", x, y, size, OFF_COLOR);
		}

		if (secure_sprintf(buffer, sizeof(buffer), "%02d", minutes) < 0)
			return;
		drawText(font, buffer, x, y, size, ON_COLOR);
	}

	// Draw hours
	for (int i = 1; i < 4; i++)
	{
		angle = hours * HOURS_ANGLE_INCREMENT;
		x = (DEFAULT_APP_WIDTH / 2) - 15 + (i * 75 / 3) * sin(angle * M_PI / 180.0);
		y = DEFAULT_APP_HEIGHT / 2 + (i * 75 / 3) * -cos(angle * M_PI / 180.0);

		if (app.enableOffMarks)
		{
			drawText(font, "88", x, y, size, OFF_COLOR);
		}

		if (secure_sprintf(buffer, sizeof(buffer), "%02d", hours) < 0)
			return;
		drawText(font, buffer, x, y, size, ON_COLOR);
	}
}

void drawFPS(TTF_Font* font, int fps)
{
	char buffer[16];
	if (secure_sprintf(buffer, sizeof(buffer), "%5d", fps) < 0)
		return;

	drawText(font, buffer, DEFAULT_APP_WIDTH - 60, 10, DEFAULT_FONT_SIZE, ON_COLOR);
}

void drawCurrentTime(TTF_Font* font, timestamp timeInfo)
{
	char dateBuffer[32];
	char timeBuffer[32];

	if (secure_sprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%02d", timeInfo.tm_year - 100, timeInfo.tm_mon + 1, timeInfo.tm_mday) < 0)
		return;

	if (secure_sprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec) < 0)
		return;

	drawText(font, dateBuffer, 5, DEFAULT_APP_HEIGHT - 25, DEFAULT_FONT_SIZE, ON_COLOR);
	drawText(font, timeBuffer, DEFAULT_APP_WIDTH - 95, DEFAULT_APP_HEIGHT - 25, DEFAULT_FONT_SIZE, ON_COLOR);
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