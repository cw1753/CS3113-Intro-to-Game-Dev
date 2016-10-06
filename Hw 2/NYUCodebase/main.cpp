#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#define toAngle 3.1416/180
using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif


class entity{
public:
	float x;
	float y;
	float rotation;

	float width;
	float height;

	float speed;
	float direction_x;
	float direction_y;

};

SDL_Window* displayWindow;
GLuint LoadTexture(const char *imagePath);

ShaderProgram *program;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

SDL_Event event;
bool done = false;
float lastFrameTicks = 0.0f;
float elapsed;
float velc = 6.0f;
bool start = 0;

entity ball;
entity rightBar;
entity leftBar;


void draw(int sides, float vert[]){
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vert);
	glEnableVertexAttribArray(program->positionAttribute);

	glDrawArrays(GL_TRIANGLES, 0, sides);

	glDisableVertexAttribArray(program->positionAttribute);
}

void setup(){
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);

	program = new ShaderProgram(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	projectionMatrix.setOrthoProjection(-5.0, 5.0, -2.817f, 2.817f, -1.0f, 1.0f);
	glUseProgram(program->programID);

	//ball
	ball.width = 0.2f;
	ball.height = 0.2f;
	ball.direction_x = 60.0f;
	ball.direction_y = 250.0f;
	//LeftBar
	leftBar.x = -4.87f;
	leftBar.width = 0.14f;
	leftBar.height = 1.0f;
	//rightBar
	rightBar.x = 4.87f;
	rightBar.width = 0.14f;
	rightBar.height = 1.0f;
}
void processEvents(){
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (start == 0){ // start playing by pressing spacebar
			if (keys[SDL_SCANCODE_SPACE]){
				start = 1;
				ball.speed = 5.0f;
				ball.direction_x = ball.direction_x + 180; 
			}
		}
		if (keys[SDL_SCANCODE_A]){
			leftBar.speed = velc;
		}
		else if (keys[SDL_SCANCODE_D]){
			leftBar.speed = -velc;
		}
		else{
			leftBar.speed = 0;
		}
		if (keys[SDL_SCANCODE_J]){
			rightBar.speed = velc;
		}
		else if (keys[SDL_SCANCODE_L]){
			rightBar.speed = -velc;
		}
		else{
			rightBar.speed = 0;
		}
	}
}
void update(){
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;


	//ball
	ball.x += cos(ball.direction_x * toAngle)*elapsed*ball.speed;
	ball.y += sin(ball.direction_y * toAngle)*elapsed*ball.speed;

	//leftBar (player 1)
	if (leftBar.speed > 0){
		if (leftBar.y + leftBar.height / 2 <= 2.7){
			leftBar.y += leftBar.speed*elapsed;
		}
		else{
			leftBar.y = 2.7 - leftBar.height / 2;
		}
	}
	else{
		if (leftBar.y - leftBar.height / 2 >= -2.7){
			leftBar.y += leftBar.speed*elapsed;
		}
		else{
			leftBar.y = -2.7 + leftBar.height / 2;
		}
	}

	//rightBar (player 2)
	if (rightBar.speed > 0){
		if (rightBar.y + rightBar.height / 2 <= 2.7){
			rightBar.y += rightBar.speed*elapsed;
		}
		else{
			rightBar.y = 2.7 - rightBar.height / 2;
		}
	}
	else{
		if (rightBar.y - rightBar.height / 2 >= -2.7){
			rightBar.y += rightBar.speed*elapsed;
		}
		else{
			rightBar.y = -2.7 + rightBar.height / 2;
		}
	}

	
	//Out of Bound (Winnning) checking
	if (ball.x + ball.width / 2 >= 5.0){
		OutputDebugString("Player 1 wins\n"); // display winner on windows
		ball.x = 0.0f;
		ball.y = 0.0f;
		ball.speed = 0; // reset ball to inital state at the middle after Win
		start = 0;
	}
	else if (ball.x - ball.width / 2 <= -5.0){
		OutputDebugString("Player 2 wins\n"); // display winner on windows
		ball.x = 0.0f;
		ball.y = 0.0f;
		ball.speed = 0;
		start = 0;
	}

	//collision checking
	//ball and topbound
	if (ball.y + ball.height / 2 >= 2.7f){
		ball.y = 2.7 - ball.height / 2;
		ball.direction_y = ball.direction_y + 180;
	}
	//ball and lowerbound
	else if (ball.y - ball.height / 2 <= -2.7f){
		ball.y = -2.7 + ball.height / 2;
		ball.direction_y = ball.direction_y - 180;
	}
	//ball and rightbar
	if (ball.y - ball.height / 2 <= rightBar.y + rightBar.height / 2 && ball.y + ball.height / 2 >= rightBar.y - rightBar.height / 2){
		if (ball.x + ball.width / 2 >= rightBar.x - rightBar.width / 2){
			ball.x = (rightBar.x - rightBar.width / 2) - ball.width / 2;
			ball.direction_x = ball.direction_x + 180;
		}
	}
	//ball and leftbar
	if (ball.y - ball.height / 2 <= leftBar.y + leftBar.height / 2 && ball.y + ball.height / 2 >= leftBar.y - leftBar.height / 2){
		if (ball.x - ball.width / 2 <= leftBar.x + leftBar.width / 2){
			ball.x = (leftBar.x + leftBar.width / 2) + ball.width / 2;
			ball.direction_x = ball.direction_x - 180;
		}
	}
}
void render(){
	float mLineCount = -2.5;
	glClear(GL_COLOR_BUFFER_BIT);

	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	float verticesLine[] = { -0.03, -0.1, 0.03, -0.1, 0.03, 0.1, -0.03, -0.1, 0.03, 0.1, -0.03, 0.1 }; //Defines vertices for middle line
	float verticesBall[] = { -0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, -0.1, 0.1, 0.1, -0.1, 0.1 }; //Defines vertices for ball
	float verticesRightBar[] = { 4.8, -0.5, 4.94, -0.5, 4.8, 0.5, 4.8, 0.5, 4.94, -0.5, 4.94, 0.5 }; //Defines vertices for rightBar
	float verticesLeftBar[] = { -4.94, -0.5, -4.8, -0.5, -4.94, 0.5, -4.94, 0.5, -4.8, -0.5, -4.8, 0.5 }; //Defines vertices for leftBar
	float verticesBound[] = { -4.94, -0.1, 4.94, -0.1, 4.94, 0.1, -4.94, -0.1, 4.94, 0.1, -4.94, 0.1 }; //Defines vertices for bound

	//draw middle line
	while (mLineCount <= 2.5){
		modelMatrix.Translate(0.0f, mLineCount, 0.0f);
		program->setModelMatrix(modelMatrix);
		draw(6, verticesLine); // calls the draw function.
		modelMatrix.identity();
		mLineCount += 0.5;
	}

	//draw ball
	modelMatrix.Translate(ball.x, ball.y, 0.0f);
	program->setModelMatrix(modelMatrix);
	draw(6, verticesBall); // calls the draw function.
	modelMatrix.identity();

	//draw rightBar
	modelMatrix.Translate(0.0f, rightBar.y, 0.0f);
	program->setModelMatrix(modelMatrix);
	draw(6, verticesRightBar);
	modelMatrix.identity();

	//draw leftBar
	modelMatrix.Translate(0.0f, leftBar.y, 0.0f);
	program->setModelMatrix(modelMatrix);
	draw(6, verticesLeftBar);
	modelMatrix.identity();

	//draw topBound
	modelMatrix.Translate(0.0f, 2.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	draw(6, verticesBound);
	modelMatrix.identity();

	//draw bottomBound
	modelMatrix.Translate(0.0f, -2.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	draw(6, verticesBound);
	modelMatrix.identity();

	SDL_GL_SwapWindow(displayWindow);
}
void cleanUp(){
	SDL_Quit();
}


int main(int argc, char *argv[])
{
	setup();

	while (!done) {   //FrameLOOP
		processEvents();
		update();
		render();
	}

	cleanUp();
	return 0;
}
