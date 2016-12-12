#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <ctgmath>
#include <cstdlib>
#define toAngle 3.1416/180
using namespace std;

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

class vect{
public:
	vect(float x1, float y1){
		x = x1;
		y = y1;
		z = 1;
		one = 1;
	}
	float x;
	float y;
	float z;
	float one;
};

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<vect> &points1, const std::vector<vect> &points2) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];
	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p < 0) {
		return true;
	}
	return false;
}

bool checkSATCollision(const std::vector<vect> &e1Points, const std::vector<vect> &e2Points) {
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}

		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points);
		if (!result) {
			return false;
		}
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points);
		if (!result) {
			return false;
		}
	}
	return true;
}


GLuint LoadTexture(const char *imagePath){
	SDL_Surface *surface = IMG_Load(imagePath);

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	SDL_FreeSurface(surface);

	return textureID;
}

class entity{
public:
	float x;
	float y;
	float rotation;

	float width;
	float height;

	float speed;
	float velX;
	float velY;
	float size = 1;

	bool Alive = 1;
	bool player;
	vector<vect> pointsVect;
	vector<vect> worldCoord;

	vector<float> transformationMatrix;
};

SDL_Window* displayWindow;

ShaderProgram *program;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

enum GameState{ State_Main_Menu, State_Game, State_Game_Over };
GameState state;

GLuint FontTexture;
SDL_Event event;
bool done = false;
float lastFrameTicks = 0.0f;
float elapsed;
float gameTimer = 0.0f;
float velc = 6.0f;
float theta = 30;
vector <entity> entVect;
entity player;
entity ball1;
entity ball2;
float vertices[] = { -0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, 0.25f, -0.25f, -0.25f, 0.25f, -0.25f };

void drawText(int fontSpriteTexture, std::string text, float size, float spacing, float xStart, float yStart){
	float texture_size = 1.0 / 16.0f;
	vector<float> vertexData;
	vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
		float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	modelMatrix.Translate(xStart, yStart, 0.0f);
	program->setModelMatrix(modelMatrix);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, fontSpriteTexture);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
	modelMatrix.identity();
}
void drawShape(entity ent){

	modelMatrix.Translate(ent.x, ent.y, 0.0f);
	modelMatrix.Rotate(ent.rotation * toAngle);
	modelMatrix.Scale(ent.size, ent.size, 1);
	program->setModelMatrix(modelMatrix);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	modelMatrix.identity();
}

vector <float> matrixMult44(vector<float> vect1, vector<float> vect2){
	vector<float> temp;
	int k = 0;
	for (int i = 0; i < vect1.size(); i += 4){
		for (int k = 0; k * 4 < vect2.size(); k++){
			temp.push_back((vect1[i] * vect2[k]) + (vect1[i + 1] * vect2[k + 4]) + (vect1[i + 2] * vect2[k + 8]) + (vect1[i + 3] * vect2[k + 12]));
		}
	}
	return temp;
}
vector<float> getTransMatrix(entity ent){
	vector<float> temp;
	float sine = (float)(sin(ent.rotation * toAngle));
	float cosine = (float)(cos(ent.rotation * toAngle));

	vector<float> translate = { 1, 0, 0, ent.x, 0, 1, 0, ent.y, 0, 0, 1, 0, 0, 0, 0, 1 };
	vector<float> rotate = { cosine, -sine, 0, 0, sine, cosine, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	vector<float> scale = { ent.size, 0, 0, 0, 0, ent.size, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	temp = matrixMult44(translate, rotate);
	temp = matrixMult44(temp, scale);
	return temp;
}
vect matrixMult41(vector<float> transMatrix, vect point){
	float tempX = (transMatrix[0] * point.x) + (transMatrix[1] * point.y) + (transMatrix[3]);
	float tempY = (transMatrix[4] * point.x) + (transMatrix[5] * point.y) + (transMatrix[7]);
	vect tempCood(tempX, tempY);
	return tempCood;
}
void toNormMatrix(entity &ent){
	ent.transformationMatrix = getTransMatrix(ent);
	ent.worldCoord.clear();
	for (int i = 0; i < ent.pointsVect.size(); i++){
		//float tempX = (transMatrix[0] * ent.pointsVect[i].x) + (transMatrix[1] * ent.pointsVect[i].y) + (transMatrix[3]);
		//float tempY = (transMatrix[4] * ent.pointsVect[i].x) + (transMatrix[5] * ent.pointsVect[i].y) + (transMatrix[7]);
		//vect tempCood(tempX, tempY);
		ent.worldCoord.push_back(matrixMult41(ent.transformationMatrix, ent.pointsVect[i]));
	}
}
void normalize(vect &coord){
	float length = sqrt(coord.x * coord.x + coord.y * coord.y);
	coord.x = coord.x / length;
	coord.y = coord.y / length;
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

	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.setOrthoProjection(-5.0, 5.0, -2.817f, 2.817f, -1.0f, 1.0f);
	glUseProgram(program->programID);

	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	ball1.x = 3;
	ball2.x = -2;

	for (int i = 0; i < 4; i++){
		vect vTemp(vertices[i], vertices[i + 1]);
		player.pointsVect.push_back(vTemp);
		ball1.pointsVect.push_back(vTemp);
		ball2.pointsVect.push_back(vTemp);
	}
	entVect.push_back(player);
	entVect.push_back(ball1);
	entVect.push_back(ball2);



	FontTexture = LoadTexture("font1.png");

}
void processEvents(){
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
	}
	if (keys[SDL_SCANCODE_SPACE]){
		player.speed = theta;
	}
	else{
		player.speed = 0;
	}
	if (keys[SDL_SCANCODE_LEFT]){
		player.velX = -velc;
	}
	else if (keys[SDL_SCANCODE_RIGHT]){
		player.velX = velc;
	}
	else{
		player.velX = 0;
	}
	if (keys[SDL_SCANCODE_UP]){
		player.velY = velc;
	}
	else if (keys[SDL_SCANCODE_DOWN]){
		player.velY = -velc;
	}
	else{
		player.velY = 0;
	}
}
void update(){
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	gameTimer += elapsed;

	player.rotation += player.speed*elapsed;
	player.x += player.velX * elapsed;
	player.y += player.velY * elapsed;

	ball1.rotation += theta*elapsed;
	ball2.rotation += -theta*elapsed;
	ball2.size = 3;

	
	toNormMatrix(player);
	toNormMatrix(ball1);
	toNormMatrix(ball2);
	int maxChecks = 10;
	while (checkSATCollision(player.worldCoord, ball1.worldCoord) && maxChecks > 0){
	vect responeseVector = vect(player.x - ball1.x, player.y - ball1.y);
	normalize(responeseVector);
	player.x += responeseVector.x * 0.006;
	player.y += responeseVector.y * 0.006;

	ball1.x -= responeseVector.x * 0.006;
	ball1.y -= responeseVector.y * 0.006;

	maxChecks -= 1;
	toNormMatrix(player);
	toNormMatrix(ball1);
	}
	
	maxChecks = 10;
	while (checkSATCollision(player.worldCoord, ball2.worldCoord) && maxChecks > 0){
		vect responeseVector = vect(player.x - ball2.x, player.y - ball2.y);
		normalize(responeseVector);
		player.x += responeseVector.x * 0.006;
		player.y += responeseVector.y * 0.006;

		ball2.x -= responeseVector.x * 0.006;
		ball2.y -= responeseVector.y * 0.006;

		maxChecks -= 1;
		toNormMatrix(player);
		toNormMatrix(ball2);
	}
	maxChecks = 10;
	while (checkSATCollision(ball1.worldCoord, ball2.worldCoord) && maxChecks > 0){
		vect responeseVector = vect(ball1.x - ball2.x, ball1.y - ball2.y);
		normalize(responeseVector);
		ball1.x += responeseVector.x * 0.006;
		ball1.y += responeseVector.y * 0.006;

		ball2.x -= responeseVector.x * 0.006;
		ball2.y -= responeseVector.y * 0.006;

		maxChecks -= 1;
		toNormMatrix(ball1);
		toNormMatrix(ball2);
	}
}
void render(){
	//glClearColor(0.4f, 0.2f, 0.4f, 1.0f);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	glClear(GL_COLOR_BUFFER_BIT);

	drawShape(player);
	drawShape(ball1);
	drawShape(ball2);

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
