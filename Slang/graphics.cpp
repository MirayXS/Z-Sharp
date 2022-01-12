#include <chrono>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <string>
//#include "graphics.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

enum Buttons
{
	PaddleOneUp = 0,
	PaddleOneDown,
	PaddleTwoUp,
	PaddleTwoDown,
};

const float PADDLE_SPEED = 1.0f;


//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The renderer we'll be rendering to
SDL_Renderer* gRenderer = NULL;

//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;


SDL_Surface* loadSurface(std::string path)
{
	//The final optimized image
	SDL_Surface* optimizedSurface = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Convert surface to screen format
		optimizedSurface = SDL_ConvertSurface(loadedSurface, gScreenSurface->format, 0);
		if (optimizedSurface == NULL)
		{
			printf("Unable to optimize image %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	return optimizedSurface;
}

class Vec2
{
public:
	Vec2()
		: x(0.0f), y(0.0f)
	{}

	Vec2(float x, float y)
		: x(x), y(y)
	{}

	Vec2 operator+(Vec2 const& rhs)
	{
		return Vec2(x + rhs.x, y + rhs.y);
	}

	Vec2& operator+=(Vec2 const& rhs)
	{
		x += rhs.x;
		y += rhs.y;

		return *this;
	}

	Vec2 operator*(float rhs)
	{
		return Vec2(x * rhs, y * rhs);
	}

	float x, y;
};

const int BALL_WIDTH = 15;
const int BALL_HEIGHT = 15;

class Ball
{
public:
	Ball(Vec2 position)
		: position(position)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = BALL_WIDTH;
		rect.h = BALL_HEIGHT;
	}

	void Draw(SDL_Renderer* renderer)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(renderer, &rect);
	}

	Vec2 position;
	SDL_Rect rect{};
};


const int PADDLE_WIDTH = 15;
const int PADDLE_HEIGHT = 80;
class Paddle
{
public:
	Paddle(Vec2 position, Vec2 velocity)
		: position(position), velocity(velocity)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = PADDLE_WIDTH;
		rect.h = PADDLE_HEIGHT;
	}

	void Update(float dt)
	{
		position += velocity * dt;

		if (position.y < 0)
		{
			// Restrict to top of the screen
			position.y = 0;
		}
		else if (position.y > (WINDOW_HEIGHT - PADDLE_HEIGHT))
		{
			// Restrict to bottom of the screen
			position.y = WINDOW_HEIGHT - PADDLE_HEIGHT;
		}
	}

	void Draw(SDL_Renderer* renderer)
	{
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(renderer, &rect);
	}

	Vec2 position;
	Vec2 velocity;
	SDL_Rect rect{};
};

class Sprite
{
public:
	Sprite(Vec2 position, std::string path)
		: position(position)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = PADDLE_WIDTH;
		rect.h = PADDLE_HEIGHT;

		SDL_Surface* surface = loadSurface(path);
		texture = SDL_CreateTextureFromSurface(gRenderer, surface);
		SDL_FreeSurface(surface);
	}

	void Draw()
	{
		rect.y = static_cast<int>(position.y);

		//SDL_RenderFillRect(renderer, &rect);
		SDL_RenderCopy(gRenderer, texture, NULL, &rect);
	}

	Vec2 position;
	std::string path;
	SDL_Rect rect{};
	SDL_Texture* texture;
};

class PlayerScore
{
public:
	PlayerScore(Vec2 position, SDL_Renderer* renderer, TTF_Font* font, SDL_Color scoreColor)
		: renderer(renderer), font(font)
	{
		surface = TTF_RenderText_Solid(font, "0", scoreColor);
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = width;
		rect.h = height;
	}

	~PlayerScore()
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
	}

	void Draw()
	{
		SDL_RenderCopy(renderer, texture, nullptr, &rect);
	}

	SDL_Renderer* renderer;
	TTF_Font* font;
	SDL_Surface* surface{};
	SDL_Texture* texture{};
	SDL_Rect rect{};
};

int main(int argc, char* argv[])
{
	// Initialize SDL components
	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();

	gWindow = SDL_CreateWindow("Pong", 20, 20, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	gRenderer = SDL_CreateRenderer(gWindow, -1, 0);

	//Get window surface
	gScreenSurface = SDL_GetWindowSurface(gWindow);

	// Initialize the font
	TTF_Font* scoreFont = TTF_OpenFont("arial.ttf", 40);

	SDL_Color scoreColor = { 0xFF, 0xFF, 0xFF, 0xFF };

	// Create the player score text fields
	PlayerScore playerOneScoreText(Vec2(WINDOW_WIDTH / 4, 20), gRenderer, scoreFont, scoreColor);

	PlayerScore playerTwoScoreText(Vec2(3 * WINDOW_WIDTH / 4, 20), gRenderer, scoreFont, scoreColor);

	// Create the ball
	Ball ball(
		Vec2((WINDOW_WIDTH / 2.0f) - (BALL_WIDTH / 2.0f),
			(WINDOW_HEIGHT / 2.0f) - (BALL_WIDTH / 2.0f)));

	// Create the paddles
	Paddle paddleOne(
		Vec2(50.0f, WINDOW_HEIGHT / 2.0f),
		Vec2(0.0f, 0.0f));

	Paddle paddleTwo(
		Vec2(WINDOW_WIDTH - 50.0f, WINDOW_HEIGHT / 2.0f),
		Vec2(0.0f, 0.0f));

	Sprite randomAssSprite(
		Vec2((WINDOW_WIDTH / 2.0f) - (100 / 2.0f),
			(WINDOW_HEIGHT / 2.0f) - (100 / 2.0f)), "./circle.png");


	// Game logic
	{
		bool running = true;
		bool buttons[4] = {};
		float dt = 0.0f;

		// Continue looping and processing events until user exits
		while (running)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					running = false;
				}
				else if (event.type == SDL_KEYDOWN)
				{
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						running = false;
					}
					else if (event.key.keysym.sym == SDLK_w)
					{
						buttons[Buttons::PaddleOneUp] = true;
					}
					else if (event.key.keysym.sym == SDLK_s)
					{
						buttons[Buttons::PaddleOneDown] = true;
					}
					else if (event.key.keysym.sym == SDLK_UP)
					{
						buttons[Buttons::PaddleTwoUp] = true;
					}
					else if (event.key.keysym.sym == SDLK_DOWN)
					{
						buttons[Buttons::PaddleTwoDown] = true;
					}
				}
				else if (event.type == SDL_KEYUP)
				{
					if (event.key.keysym.sym == SDLK_w)
					{
						buttons[Buttons::PaddleOneUp] = false;
					}
					else if (event.key.keysym.sym == SDLK_s)
					{
						buttons[Buttons::PaddleOneDown] = false;
					}
					else if (event.key.keysym.sym == SDLK_UP)
					{
						buttons[Buttons::PaddleTwoUp] = false;
					}
					else if (event.key.keysym.sym == SDLK_DOWN)
					{
						buttons[Buttons::PaddleTwoDown] = false;
					}
				}
			}

			if (buttons[Buttons::PaddleOneUp])
			{
				paddleOne.velocity.y = -PADDLE_SPEED;
			}
			else if (buttons[Buttons::PaddleOneDown])
			{
				paddleOne.velocity.y = PADDLE_SPEED;
			}
			else
			{
				paddleOne.velocity.y = 0.0f;
			}

			if (buttons[Buttons::PaddleTwoUp])
			{
				paddleTwo.velocity.y = -PADDLE_SPEED;
			}
			else if (buttons[Buttons::PaddleTwoDown])
			{
				paddleTwo.velocity.y = PADDLE_SPEED;
			}
			else
			{
				paddleTwo.velocity.y = 0.0f;
			}

			// Update the paddle positions
			paddleOne.Update(dt);
			paddleTwo.Update(dt);

			// Clear the window to black
			SDL_SetRenderDrawColor(gRenderer, 0x0, 0x0, 0x0, 0xFF);
			SDL_RenderClear(gRenderer);

			// Set the draw color to be white
			SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

			// Draw the net
			for (int y = 0; y < WINDOW_HEIGHT; ++y)
			{
				if (y % 5)
				{
					SDL_RenderDrawPoint(gRenderer, WINDOW_WIDTH / 2, y);
				}
			}

			// Draw the ball
			ball.Draw(gRenderer);

			// Draw the paddles
			paddleOne.Draw(gRenderer);
			paddleTwo.Draw(gRenderer);

			// Display the scores
			playerOneScoreText.Draw();
			playerTwoScoreText.Draw();

			randomAssSprite.Draw();

			// Present the backbuffer
			SDL_RenderPresent(gRenderer);

			// Calculate frame time
			auto stopTime = std::chrono::high_resolution_clock::now();
			dt = std::chrono::duration<float, std::chrono::milliseconds::period>(stopTime - startTime).count();
		}
	}

	// Cleanup
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	TTF_CloseFont(scoreFont);
	SDL_Quit();

	return 0;
}