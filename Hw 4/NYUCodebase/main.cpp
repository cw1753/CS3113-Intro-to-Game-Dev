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
#include <cstdlib>
#define LEVEL_HEIGHT 41
#define LEVEL_WIDTH 22
#define TILE_SIZE 0.27f
#define toAngle 3.1416/180
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
using namespace std;

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

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

	float width = 0.27f;
	float height = 0.27f;

	float speed;
	float vel_x;
	float vel_y;
	float acc_x = 0.0f;
	float acc_y =  -9.81f;

	bool isAlive = 1;
	bool isStatic;
	bool isPlayer;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

void worldToTileCoord(float worldX, float worldY, int * gridX, int * gridY){
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}

SDL_Window* displayWindow;

ShaderProgram *program;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

enum GameState{ State_Main_Menu, State_Game, State_Game_Over };
GameState state;

GLuint FontTexture;
GLuint playerTexture;
GLuint TileMapTexture;
SDL_Event event;
bool done = false;
float lastFrameTicks = 0.0f;
float elapsed;
float gameTimer = 0.0f;
int totalTime = 0.0;
entity myPlayer;
vector <entity> entVec;
float velc = 6.0f;
bool win = 0;
int score = 0;

bool isSolid(unsigned char index){
	if (index == 2){
		return true;
	}
	else if (index == 3){
		return true;
	}
	else if (index == 6){
		return true;
	}
	else if (index == 4){
		return true;
	}
	else
		return false;
}


unsigned char levelData[LEVEL_HEIGHT][LEVEL_WIDTH] =
{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 20, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 6, 6, 6, 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 6, 6, 6, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 6, 6, 6, 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0 },
	{ 0, 20, 125, 118, 0, 0, 116, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 116, 0, 127, 20, 0 },
	{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
	{ 32, 33, 33, 34, 32, 33, 33, 34, 33, 35, 100, 101, 35, 32, 33, 32, 34, 32, 33, 32, 33, 33 }
};

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
void drawMyPlayer(int spriteTexture, int index, int spriteCountX, int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;
	GLfloat texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight
	};
	float vertices[] = { -0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, 0.25f, -0.25f, -0.25f, 0.25f, -0.25f };

	if (myPlayer.isAlive){
		modelMatrix.Translate(myPlayer.x, myPlayer.y, 0.0f);
		program->setModelMatrix(modelMatrix);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, spriteTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
		modelMatrix.identity();
	}
}
void drawEntity(int spriteTexture, int index, int spriteCountX, int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;
	GLfloat texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight
	};
	float vertices[] = { -0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, 0.25f, -0.25f, -0.25f, 0.25f, -0.25f };

	for (int i = 1; i < entVec.size(); i++){
		if (entVec[i].isAlive){
			modelMatrix.Translate(entVec[i].x, entVec[i].y, 0.0f);
			program->setModelMatrix(modelMatrix);
			glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
			glEnableVertexAttribArray(program->positionAttribute);
			glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(program->texCoordAttribute);
			glBindTexture(GL_TEXTURE_2D, spriteTexture);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(program->positionAttribute);
			glDisableVertexAttribArray(program->texCoordAttribute);
			modelMatrix.identity();
		}
	}
}
void drawTile(int Texture, int SPRITE_COUNT_X, int SPRITE_COUNT_Y){
	vector<float> vertexData;
	vector<float> texCoordData;
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (levelData[y][x] != 0){
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
	}
	program->setModelMatrix(modelMatrix);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	modelMatrix.identity();
	program->setModelMatrix(modelMatrix);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glDrawArrays(GL_TRIANGLES, 0,  vertexData.size() / 2);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
	modelMatrix.identity();
}


int gridX = 0;
int gridY = 0;
void collisionBottom(entity &ent){
	worldToTileCoord(ent.x, ent.y - (ent.height / 2), &gridX, &gridY);
	if (gridY <= 0){
		gridY = 0;
	}
	else if (gridY >= LEVEL_HEIGHT){
		gridY = LEVEL_HEIGHT;
	}
	if (isSolid(levelData[gridY][gridX])){
		ent.collidedBottom = 1;

		ent.vel_y = 0;
		ent.acc_y = 0;
		float penetration_Y = (-TILE_SIZE *gridY) - (ent.y - ent.height / 2);
		ent.y += penetration_Y;
		return;
	}
	else{
		ent.collidedBottom = 0;
		ent.acc_y = -9.81;
	}
}
void collisionTop(entity &ent){
	worldToTileCoord(ent.x, ent.y + (ent.height / 2), &gridX, &gridY);
	if (gridY <= 0){
		gridY = 0;
	}
	else if (gridY >= LEVEL_HEIGHT){
		gridY = LEVEL_HEIGHT;
	}
	if (isSolid(levelData[gridY][gridX])){
		ent.collidedTop = 1;
		ent.vel_y = 0;
		//ent.acc_y = 0;
		float penetration_Y = (ent.y + ent.height / 2) - (-TILE_SIZE *gridY - TILE_SIZE);
		ent.y -= penetration_Y;
		return;
	}
	else{
		ent.collidedTop = 0;
		ent.acc_y = -9.81;
	}
}
void collisionLeft(entity &ent){
	float penetration_X;
	worldToTileCoord(ent.x - (ent.width / 2), ent.y, &gridX, &gridY);
	if (gridX <= 0){
		gridX = 0;
	}
	else if (gridX >= LEVEL_WIDTH){
		gridX = LEVEL_WIDTH;
	}
	if (levelData[gridY][gridX] == 20 || isSolid(levelData[gridY][gridX])){
		ent.vel_x = 0;
		ent.acc_x = 0;
		ent.collidedLeft = 1;
		penetration_X = (TILE_SIZE *gridX + TILE_SIZE) - (ent.x - ent.width / 2);
		ent.x += penetration_X;
		return;
	}
	ent.collidedLeft = 0;
}
void collisionRight(entity &ent){
	float penetration_X;
	worldToTileCoord(ent.x + (ent.width / 2), ent.y, &gridX, &gridY);
	if (gridX <= 0){
		gridX = 0;
	}
	else if (gridX >= LEVEL_WIDTH){
		gridX = LEVEL_WIDTH;
	}
	if (levelData[gridY][gridX] == 20 || isSolid(levelData[gridY][gridX])){
		ent.vel_x = 0;
		ent.acc_x = 0;
		ent.collidedRight = 1;
		penetration_X = (ent.x + ent.width / 2) - (TILE_SIZE *gridX);
		ent.x -= penetration_X;
		return;
	}
	ent.collidedLeft = 0;
}
bool collisionEntity(entity B1, entity B2){
	if (B1.x - B1.width / 2 <= B2.x + B2.width / 2
		&& B1.x + B1.width / 2 >= B2.x - B2.width / 2){
		if (B1.y + B1.height / 2 >= B2.y - B2.height / 2
			&& B1.y - B1.height / 2 <= B2.y + B2.height / 2){
			return true;
		}
	}
	return false;
}
void updateMainMenu(){}
void updateGame(float time){
	//Update Player
	myPlayer.vel_y = lerp(myPlayer.vel_y, 0.0f, time * 0.5f); // Friction
	myPlayer.vel_x = lerp(myPlayer.vel_x, 0.0f, time * 0.5f);

	myPlayer.vel_y += myPlayer.acc_y * time;
	myPlayer.vel_x += myPlayer.acc_x * time;

	myPlayer.y += myPlayer.vel_y * time;
	collisionBottom(myPlayer);
	collisionTop(myPlayer);

	myPlayer.x += myPlayer.vel_x * time;
	collisionLeft(myPlayer);
	collisionRight(myPlayer);
	
	//Update Kings
	for (int i = 0; i < entVec.size(); i++){
		entVec[i].vel_y = lerp(entVec[i].vel_y, 0.0f, time * 0.5f); // Friction
		entVec[i].vel_x = lerp(entVec[i].vel_x, 0.0f, time * 0.5f);

		entVec[i].vel_y += entVec[i].acc_y * time;
		entVec[i].vel_x += entVec[i].acc_x * time;

		entVec[i].y += entVec[i].vel_y * time;
		collisionBottom(entVec[i]);
		collisionTop(entVec[i]);

		entVec[i].x += entVec[i].vel_x * time;
		collisionLeft(entVec[i]);
		collisionRight(entVec[i]);
		
		//Check if Player and Kings collide
		if (collisionEntity(myPlayer, entVec[i]) && entVec[i].isAlive){
			score += 10;
			entVec[i].isAlive = 0;
		}
	}

	//Checking for Game Over (losing or Winning)
	if (myPlayer.y > 0){
		win = 1;
		totalTime = gameTimer;
		state = State_Game_Over;
	}
	else if (myPlayer.y - myPlayer.height / 2 < (-TILE_SIZE*LEVEL_HEIGHT - TILE_SIZE)){
		state = State_Game_Over;
	}
}
void updateGameOver(){}

void renderMainMenu(){
	drawText(FontTexture, "Save The Royal", 0.6f, 0.001f, -3.9f, 1.6f);
	drawText(FontTexture, "Press", 0.5f, 0.001f, -1.0f, 0.0f);
	drawText(FontTexture, "SPACEBAR", 0.5f, 0.001f, -1.8f, -1.0f);
}
void renderGame(){
	drawTile(TileMapTexture, 16, 8);
	drawMyPlayer(playerTexture, 17, 8, 4);
	drawEntity(playerTexture, 9, 8, 4);

}
void renderGameOver(){
	if (win){
		drawText(FontTexture, "Winner", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "Score: " + to_string(score), 0.5f, 0.001f, -3.0f, 0.0f);
		drawText(FontTexture, "Time: " + to_string(totalTime) + " seconds", 0.5f, 0.001f, -3.0f, -1.0f);
	}
	else{
		drawText(FontTexture, "LOSER", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "YOU SUCK", 0.6f, 0.001f, -2.0f, -0.0f);
	}
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

	myPlayer.x = TILE_SIZE * (LEVEL_WIDTH - 19);
	myPlayer.y = -TILE_SIZE * (LEVEL_HEIGHT - 4);
	int yCount = 4;
	int xCount = 3;
	for (int i = 0; i < 10; i++){
		entity king;
		king.x = TILE_SIZE * (LEVEL_WIDTH - xCount);
		king.y = -TILE_SIZE * (LEVEL_HEIGHT - yCount);
		king.vel_x = 0;
		king.vel_y = 0;
		entVec.push_back(king);
		if (i <= 6){
			yCount += 3;
			xCount += 2;
		}
		else{
			yCount += 2.7;
			xCount -= 2;
		}
	}

	FontTexture = LoadTexture("font1.png");
	playerTexture = LoadTexture("characters.png");
	TileMapTexture = LoadTexture("map.png");
}
void processEvents(){
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (state == State_Game){
			if (event.type == SDL_KEYDOWN){
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE){
					if (myPlayer.collidedBottom == 1 ){
						myPlayer.vel_y = velc - 1.0;
						myPlayer.collidedBottom = 0;
					}
				}
			}
		}
	}
	if (state == State_Main_Menu){ // start playing by pressing spacebar
		if (keys[SDL_SCANCODE_SPACE]){
			state = State_Game;
		}
	}
	else if (state == State_Game){
		if (keys[SDL_SCANCODE_LEFT]){
			myPlayer.acc_x = -velc;
		}
		else if (keys[SDL_SCANCODE_RIGHT]){
			myPlayer.acc_x = velc;
		}
		else{
			myPlayer.acc_x = 0;
		}
	}
}
void update(){
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	gameTimer += elapsed;

	switch (state){
	case State_Main_Menu:{
		updateMainMenu();
		break;
	}
	case State_Game:{
		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}
		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			updateGame(FIXED_TIMESTEP);
		}
		updateGame(fixedElapsed);
		break;
	}
	case State_Game_Over:
		updateGameOver();
		break;
	}


}
void render(){
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);


	switch (state){
	case State_Main_Menu:
		renderMainMenu();
		break;
	case State_Game:
		viewMatrix.identity();
		viewMatrix.Translate(-myPlayer.x, -myPlayer.y, 0.0f);
		viewMatrix.Scale(1.3f, 1.1f, 1.0f);
		program->setViewMatrix(viewMatrix);
		viewMatrix.identity();
		renderGame();
		break;
	case State_Game_Over:
		viewMatrix.identity();
		program->setViewMatrix(viewMatrix);
		renderGameOver();
		break;
	}

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
