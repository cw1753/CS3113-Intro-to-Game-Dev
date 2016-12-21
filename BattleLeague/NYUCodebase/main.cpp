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
#include <ctime>
#include <cstdlib>
#include <SDL_mixer.h>
#define LEVEL_HEIGHT 25
#define LEVEL_WIDTH 70
#define TILE_SIZE 0.27f
#define toAngle 3.1416/180
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#define screenWildth 1360
#define screenHeight screenWildth/1.778
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
float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}
void worldToTileCoord(float worldX, float worldY, int * gridX, int * gridY){
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}

class entity{
public:
	entity(int textIndex, float tWidth = 0, float tHeight = 0, float tX = 0, float tY = 0 ){
		textureIndex = textIndex;
		width = tWidth;
		height = tHeight;
		x = tX;
		y = tY;
		xDirection = 1;

		acc_x = 0.0f;
		acc_y = -9.81f;

		isAlive = 1;

		vertices = { -width / 2, -height / 2, width / 2, height / 2, -width / 2, height / 2, width / 2, height / 2, -width / 2, -height / 2, width / 2, -height / 2 };
	}
	int textureIndex;

	float x;
	float y;
	float xDirection;
	float rotation;

	float width;
	float height;
	vector<float> vertices;

	float vel_x;
	float vel_y;
	float acc_x;
	float acc_y;

	bool isAlive;
	float timeAlive;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};
class player : public entity{
public:
	player(int textIndex, float tWidth, float tHeight, float tX = 0, float tY = 0) : entity(textIndex, tWidth, tHeight, tX, tY){
		health = 100;
		attack = 10;
	}
	int health;
	int attack;
	int defense;
	int speed;
};


SDL_Window *displayWindow;
ShaderProgram *program;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

enum GameState{ State_Main_Menu, State_Game, State_Game_Over };
GameState state;

GLuint FontTexture;
GLuint playerTexture;
GLuint pickupsTexture;
GLuint TileMapTexture;
GLuint backgroundTexture;
Mix_Chunk *jumpSound;
Mix_Chunk *collectSound;
Mix_Chunk *hitPlayerSound;
Mix_Chunk *killEnemySound;
Mix_Chunk *shootSound;
Mix_Chunk *winSound;
Mix_Music *bkMusic;
SDL_Event event;
bool done = false;
float lastFrameTicks = 0.0f;
float elapsed;
float gameTimer = 0.0f;
int totalTime = 0.0;
float screenShakeValue;
float screenShakeIntensity = 1.3;
float p1ScreenShakeIntensity;
float p2ScreenShakeIntensity;
float enemyTimer;
float pickupTimer;

player player1(17, 0.44, 0.44, TILE_SIZE * (2), -TILE_SIZE * (LEVEL_HEIGHT - 4));
player player2(1, 0.44, 0.44, TILE_SIZE * (LEVEL_WIDTH - 2), -TILE_SIZE * (LEVEL_HEIGHT - 4));
vector<entity> pickups;
vector<entity> bulletVector;
vector <entity> enemies;
int level;
bool isFirstRun = 1;
float velc = 6.0f;
int winner = 0;
unsigned char levelData[LEVEL_HEIGHT][LEVEL_WIDTH];


void loadTextureAndSound(){
	//Load Tecture
	FontTexture = LoadTexture("font1.png");
	playerTexture = LoadTexture("character.png");
	pickupsTexture = LoadTexture("pickups.png");
	TileMapTexture = LoadTexture("map.png");
	backgroundTexture = LoadTexture("background.png");

	//Load Sound
	jumpSound = Mix_LoadWAV("jump.wav");
	collectSound = Mix_LoadWAV("pickup.wav");
	hitPlayerSound = Mix_LoadWAV("hit.wav");
	killEnemySound = Mix_LoadWAV("kill.wav");
	shootSound = Mix_LoadWAV("shoot.wav");
	winSound = Mix_LoadWAV("winning.wav");
	bkMusic = Mix_LoadMUS("music.mp3");
}
void createMapArray(){
	for (int i = 0; i < LEVEL_HEIGHT; i++){  //Make Boraders
		for (int k = 0; k < LEVEL_WIDTH; k++){
			if (i == 0 || i == LEVEL_HEIGHT - 1){
				levelData[i][k] = 4;
			}
			else if (k == 0 || k == LEVEL_WIDTH - 1){
				levelData[i][k] = 4;
			}
			else{
				levelData[i][k] = 0;
			}
		}
	}

	int countPlatform = 0;
	int percetPlat = 30;
	vector<int> platEndVect = { 1 };

	for (int i = 4; i < LEVEL_HEIGHT-1; i += 4){
		int check = 1;
		int index = 0;
		vector<int> tempVect;

		if (!platEndVect.empty()){
			for (int k = platEndVect[index]; k < LEVEL_WIDTH-1; k++){
				int platEnd = 1;
				int randNum = rand() % 100 + 1;

				if (countPlatform == 3){
					countPlatform = 0;
					percetPlat = 80;
				}
				if (randNum > percetPlat || (countPlatform > 0 && countPlatform < 3)){ //draw platform
					levelData[i][k] = 4;
					countPlatform++;
					platEnd = k;
				}
				else{  //Not drawing Platform
					if (platEnd > check){
						index++;
						check = platEnd;
						tempVect.push_back(platEnd);

						//need to change the K according to the index of vect
						if (index < platEndVect.size()){
							k = platEndVect[index];
						}
						else{
							k += 3;
						}
						percetPlat = 50;
						countPlatform = 0;
					}
				}
				platEndVect.clear();
				if (i % 24)
					platEndVect = { 1 };
				else
					platEndVect = tempVect;
			}
		}
	}
}
bool isSolid(unsigned char index){
	if (index == 4)
		return true;
	else
		return false;
}
int getRandPickIndex(){
	int temp = rand() % 100 + 1;
	if (temp > 60){
		return 10;  //Attack Pickup
	}
	else if (temp > 30){
		return 11;  //Defence Pickup
	}
	return 82;  //Speed Pickup
}
void shootBullet(entity player){
	entity bullet(7, 0.5f, 0.5f);
	bullet.y = player.y;
	bullet.width = 0.3f;
	bullet.x = player.x + player.xDirection * ((player.width / 2) + (bullet.width/2) + 0.005);
	bullet.vel_x = 10.0f * player.xDirection;
	bullet.isAlive = 1;

	bulletVector.push_back(bullet);
 	Mix_PlayChannel(2, shootSound, 0);

}
void changeOrientation(entity &ent){ //Change Orientation of entity
	if (ent.vertices[0] < 0 && ent.xDirection > 0)
		return;
	else if (ent.vertices[0] > 0 && ent.xDirection < 0)
		return;
	for (int i = 0; i < ent.vertices.size(); i += 2){
		ent.vertices[i] = ent.vertices[i] * -1.0f;
	}
}
void setenemyVector(vector<entity> &entVect, int amount, float size){
	for (int i = 0; i < amount; i++){
		int num = rand() % 100 + 1;
		float tX = rand() % LEVEL_WIDTH - 1;
		float tY = rand() % LEVEL_HEIGHT;
		entity temp(26, size, size, TILE_SIZE * tX, -TILE_SIZE * tY);
		temp.width = 0.27;
		if (num>50)
			temp.vel_x = velc - 5;
		else
			temp.vel_x = -velc + 5;
		entVect.push_back(temp);
	}
}
void setPickupVector(vector<entity> &pickVect, int amount, float size){
	for (int i = 0; i < amount; i++){

		float tX = rand() % LEVEL_WIDTH - 1;
		float tY = rand() % LEVEL_HEIGHT;
		entity temp(getRandPickIndex(), size, size, TILE_SIZE * tX, -TILE_SIZE * tY);
		pickVect.push_back(temp);
	}
}

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
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	modelMatrix.identity();
	program->setModelMatrix(modelMatrix);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
	modelMatrix.identity();
}
void drawEntity(entity ent, int spriteTexture, int spriteCountX, int spriteCountY) {
	int index = ent.textureIndex;
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

	if (ent.isAlive){
		modelMatrix.Translate(ent.x, ent.y, 0.0f);
		program->setModelMatrix(modelMatrix);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, ent.vertices.data());
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
void drawBullet(int spriteTexture, int index, int spriteCountX, int spriteCountY) {
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

	for (int i = 0; i < bulletVector.size(); i++){ //draw all the Bullet
		modelMatrix.Translate(bulletVector[i].x, bulletVector[i].y, 0.0f);
		program->setModelMatrix(modelMatrix);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, bulletVector[i].vertices.data());
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

bool shouldRemoveBullet(entity bullet){
	//Remove Bullet by time limit or position limit
	if (bullet.timeAlive >0.8f){
		return 1;
	}
	else if (!bullet.isAlive)
		return 1;
	else
		return 0;
}
bool shouldRemoveEntity(entity ent){
	//Remove Bullet by time limit or position limit
	if (abs(ent.y) == 1){
		return 1;
	}
	else if (!ent.isAlive)
		return 1;
	else
		return 0;
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
		float penetration_Y = (-TILE_SIZE * gridY) - (ent.y - ent.height / 2);
		ent.y += penetration_Y;
		return;
	}
	else{
		ent.collidedBottom = 0;
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
		float penetration_Y = (ent.y + ent.height / 2) - (-TILE_SIZE *gridY - TILE_SIZE);
		ent.y -= penetration_Y;
		return;
	}
	else{
		ent.collidedTop = 0;
	}
}
void collisionLeft(entity &ent, bool isEnemy = 0){
	float penetration_X;
	worldToTileCoord(ent.x - (ent.width / 2), ent.y, &gridX, &gridY);
	if (gridX <= 0){
		gridX = 0;
	}
	else if (gridX >= LEVEL_WIDTH){
		gridX = LEVEL_WIDTH;
	}
	if (levelData[gridY][gridX] == 20 || isSolid(levelData[gridY][gridX])){
		if (isEnemy){
			if (level == 2)
				ent.acc_x = -ent.acc_x;
			ent.vel_x = -ent.vel_x;
		}
		else{
			if (level == 2)
				ent.acc_x = -ent.acc_x;
			ent.vel_x = 0;
		}
		if (level!=2)
			ent.acc_x = 0;
		ent.collidedLeft = 1;
		penetration_X = (TILE_SIZE *gridX + TILE_SIZE) - (ent.x - ent.width / 2);
		ent.x += penetration_X;
		return;
	}
	ent.collidedLeft = 0;
}
void collisionRight(entity &ent, bool isEnemy = 0){
	float penetration_X;
	worldToTileCoord(ent.x + (ent.width / 2), ent.y, &gridX, &gridY);
	if (gridX <= 0){
		gridX = 0;
	}
	else if (gridX >= LEVEL_WIDTH){
		gridX = LEVEL_WIDTH;
	}
	if (levelData[gridY][gridX] == 20 || isSolid(levelData[gridY][gridX])){
		if (isEnemy){
			if (level == 2)
				ent.acc_x = -ent.acc_x;
			ent.vel_x = -ent.vel_x;
		}
		else{
			if (level == 2)
				ent.acc_x = -ent.acc_x;
			ent.vel_x = 0;
		}
		if (level != 2)
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

void updateEntity(entity &ent, float time, bool isEnemy = 0){
	if (!isEnemy){
		ent.vel_y = lerp(ent.vel_y, 0.0f, time * 0.5f); // Friction
		ent.vel_x = lerp(ent.vel_x, 0.0f, time * 1.8f);
	}
	ent.vel_y += ent.acc_y * time;
	ent.vel_x += ent.acc_x * time;

	ent.y += ent.vel_y * time;
	collisionBottom(ent);
	collisionTop(ent);

	ent.x += ent.vel_x * time;
	collisionLeft(ent, isEnemy);
	collisionRight(ent, isEnemy);
	changeOrientation(ent);
}

void updateMainMenu(){}
void updateGame(float time){
	if (isFirstRun){ // Give initial enemies a x direction
		if (level == 2){
			for (int i = 0; i < enemies.size(); i++){
				int num = rand() % 100 + 1;
				if (num>50)
					enemies[i].acc_x = 1.0;
				else
					enemies[i].acc_x = -1.0;
			}
		}
		isFirstRun = 0;
	}
	//New Pickups
	pickupTimer += elapsed;
	if (pickupTimer >= 7.0f){
		setPickupVector(pickups, 1, 0.3f);
		pickupTimer = 0;
	}
	//New Enemies
	enemyTimer += elapsed;
	if (enemyTimer >= 4.0f){
		setenemyVector(enemies, 1, 0.44f);
		enemyTimer = 0;
	}
	//Update Players
	player1.timeAlive += time;
	updateEntity(player1, time);
	updateEntity(player2, time);

	//Update Pickups
	for (int i = 0; i < pickups.size(); i++){
		updateEntity(pickups[i], time);
	}

	//Update enemies
	for (int i = 0; i < enemies.size(); i++){
		updateEntity(enemies[i], time, true);
	}

	//Update Bullets
	for (int i = 0; i < bulletVector.size(); i++){
		bulletVector[i].x += bulletVector[i].vel_x * elapsed;
		bulletVector[i].timeAlive += time;
	}

	//Check Bullet and Entity Collision
	for (int k = 0; k < bulletVector.size(); k++){
		//Check Bullet and enemies Collision
		for (int i = 0; i < enemies.size(); i++){
			if (enemies[i].isAlive && bulletVector[k].isAlive && collisionEntity(enemies[i], bulletVector[k])){
				entity temp(96, 0.3f, 0.3f, enemies[i].x + 0.02, enemies[i].y + 0.005);
				pickups.push_back(temp);
				enemies[i].isAlive = 0;
				bulletVector[k].isAlive = 0;
				Mix_PlayChannel(2, killEnemySound, 0);
			}
		}
		//Check Bullet and Player1 Collision
		if (player1.isAlive && bulletVector[k].isAlive && collisionEntity(player1, bulletVector[k])){
			int dmg = player2.attack - player1.defense;
			if (dmg <= 0)
				player1.health -= 2;
			else
				player1.health -= dmg;
			bulletVector[k].isAlive = 0;
			p1ScreenShakeIntensity += 0.2;
			Mix_PlayChannel(2, hitPlayerSound, 0);
		}
		//Check Bullet and Player2 Collision
		else if (player2.isAlive && bulletVector[k].isAlive && collisionEntity(player2, bulletVector[k])){
			int dmg = player1.attack - player2.defense;
			if (dmg <= 0)
				player2.health -= 2;
			else
				player2.health -= dmg;
			bulletVector[k].isAlive = 0;
			p2ScreenShakeIntensity += 0.2;
			Mix_PlayChannel(2, hitPlayerSound, 0);
		}
	}

	//Check Players and Pickups Collision
	for (int k = 0; k < pickups.size(); k++){
		int pickUpType = pickups[k].textureIndex;
		//Check Player1 and Pickups Collision
		if (player1.isAlive && pickups[k].isAlive && collisionEntity(player1, pickups[k])){
			if (pickUpType == 96)
				player1.health += 10;
			else if (pickUpType == 10)
				player1.attack += 5;
			else if (pickUpType == 11)
				player1.defense += 5;
			else
				player1.speed++;

			pickups[k].isAlive = 0;
			Mix_PlayChannel(2, collectSound, 0);
		}
		//Check Player2 and Pickups Collision
		else if (player2.isAlive && pickups[k].isAlive && collisionEntity(player2, pickups[k])){
			if (pickUpType == 96)
				player2.health += 10;
			else if (pickUpType == 10)
				player2.attack += 5;
			else if (pickUpType == 11)
				player2.defense += 5;
			else
				player2.speed++;

			pickups[k].isAlive = 0;
			Mix_PlayChannel(2, collectSound, 0);
		}
	}

	//Check Player and Enemies Collision
	for (int i = 0; i < enemies.size(); i++){
		//Check Player1 and Enemies Collision
		if (player1.isAlive && enemies[i].isAlive && collisionEntity(player1, enemies[i])){
			player1.health -= 10;
			enemies[i].isAlive = 0;
			p1ScreenShakeIntensity += 0.2;
			Mix_PlayChannel(2, hitPlayerSound, 0);
		}
		//Check Player2 and Enemy Collision
		else if (player2.isAlive && enemies[i].isAlive && collisionEntity(player2, enemies[i])){
			player2.health -= 10;
			enemies[i].isAlive = 0;
			p2ScreenShakeIntensity += 0.2;
			Mix_PlayChannel(2, hitPlayerSound, 0);
		}
	}


	//Remove Pickups
	pickups.erase(remove_if(pickups.begin(), pickups.end(), shouldRemoveEntity), pickups.end());
	//Remove Emermy
	enemies.erase(remove_if(enemies.begin(), enemies.end(), shouldRemoveEntity), enemies.end());
	//Remove Bullet
	bulletVector.erase(remove_if(bulletVector.begin(), bulletVector.end(), shouldRemoveBullet), bulletVector.end());


	//Checking for Game Over (losing or Winning)
	if (player1.health <= 0){
		winner = 2;
		player1.health = 0;
		totalTime = gameTimer;
		state = State_Game_Over;
		Mix_PlayChannel(2, winSound, 0);
	}
	else if (player2.health <= 0){
		winner = 1;
		player2.health = 0;
		totalTime = gameTimer;
		state = State_Game_Over;
		Mix_PlayChannel(2, winSound, 0);
	}
	if (level == 3){
		if (gameTimer >= 60){
			if (player1.health > player2.health)
				winner = 1;
			else if (player1.health < player2.health)
				winner = 2;
			else
				winner = 0;
			state = State_Game_Over;
			totalTime = gameTimer;
			Mix_PlayChannel(2, winSound, 0);
		}
	}
}
void updateGameOver(){}

void renderMainMenu(){
	drawEntity(entity(0, 10, 8), backgroundTexture, 1, 1); //Background
	drawText(FontTexture, "Battle League", 0.75f, 0.001f, -4.5f, 2.0f);
	drawText(FontTexture, "Choose Game Mode", 0.4f, 0.00001f, -3.0f, 1.2f);
	drawText(FontTexture, "1.Battle to the Death", 0.4f, 0.0001f, -3.5f, 0.5f);
	drawText(FontTexture, "2.Start Out Slow", 0.4f, 0.0001f, -3.5f, 0.1f);
	drawText(FontTexture, "3.A Minute Game", 0.4f, 0.0001f, -3.5f, -0.3f);
	drawEntity(entity(96, 1.0, 1.0, -3.6f, -1.5f), pickupsTexture, 14, 7);
	drawEntity(entity(10, 1.0, 1.0, -1.2f, -1.5f), pickupsTexture, 14, 7);
	drawEntity(entity(11, 1.0, 1.0, 1.3f, -1.5f), pickupsTexture, 14, 7);
	drawEntity(entity(82, 1.0, 1.0, 3.8f, -1.5f), pickupsTexture, 14, 7);
	drawText(FontTexture, "Health		Attack		Defense		Speed", 0.3f, 0.001f, -4.3f, -2.4f);
}
void renderPlayer1Game(){
	p1ScreenShakeIntensity -= elapsed;
	if (p1ScreenShakeIntensity < 0)
		p1ScreenShakeIntensity = 0;

	glViewport(0, 0, screenWildth / 2, screenHeight);
	//Draw Background
	viewMatrix.identity();
	program->setViewMatrix(viewMatrix);
	drawEntity(entity(0, 10, 8, 0, 1), backgroundTexture, 1, 1); //Background
	//Draw Map
	viewMatrix.identity();
	viewMatrix.Scale(1.8f, 1.0f, 1.0f);
	viewMatrix.Translate(-player1.x, -player1.y - 1.0f, 0.0f);
	viewMatrix.Translate(sin(screenShakeValue * 1000.0f) * p1ScreenShakeIntensity, sin(screenShakeValue * 1000.0f) * p1ScreenShakeIntensity, 0.0f); // ScreenShake When Dmg
	program->setViewMatrix(viewMatrix);
	drawTile(TileMapTexture, 16, 8);
	//Draw Players
	drawEntity(player1, playerTexture, 8, 4);
	drawEntity(player2, playerTexture, 8, 4);
	//Draw Pickups
	for (int i = 0; i < pickups.size(); i++){
		drawEntity(pickups[i], pickupsTexture, 14, 7);
	}
	//Draw enemies
	for (int i = 0; i < enemies.size(); i++){
		drawEntity(enemies[i], playerTexture, 8, 4);
	}
	//Draw Bullets
	drawBullet(pickupsTexture, 6, 14, 7);
	//Draw Info
	viewMatrix.identity();
	viewMatrix.Scale(1.7f, 1.0f, 1.0f);
	program->setViewMatrix(viewMatrix);
	drawText(FontTexture, "Player1 ", 0.23f, 0.001f, -2.8f, 2.6f);
	drawText(FontTexture, "HP: " + to_string(player1.health), 0.23f, 0.001f, -2.8f, 2.40f);
	drawText(FontTexture, "ATK:" + to_string(player1.attack), 0.23f, 0.001f, -2.8f, 2.23f);
	drawText(FontTexture, "DEF:" + to_string(player1.defense), 0.23f, 0.001f, -2.8f, 2.06f);
	drawText(FontTexture, "SP: " + to_string(player1.speed), 0.23f, 0.001f, -2.8f, 1.89f);
	drawEntity(entity(14, 0.15, 10, 3.0f, 0.0f), 0, 14, 7); //middle black line
	program->setViewMatrix(viewMatrix);
}
void renderPlayer2Game(){
	p2ScreenShakeIntensity -= elapsed;
	if (p2ScreenShakeIntensity < 0)
		p2ScreenShakeIntensity = 0;

	glViewport(screenWildth / 2, 0, screenWildth/2, screenHeight);
	//Draw Background
	viewMatrix.identity();
	program->setViewMatrix(viewMatrix);
	drawEntity(entity(0, 10, 8, 0, 1), backgroundTexture, 1, 1); //Background
	//Draw Map
	viewMatrix.identity();
	viewMatrix.Scale(1.8f, 1.0f, 1.0f);
	viewMatrix.Translate(-player2.x, -player2.y - 1.0f, 0.0f);
	viewMatrix.Translate(-sin(screenShakeValue * 1000.0f) * p2ScreenShakeIntensity, sin(screenShakeValue * 1000.0f) * p2ScreenShakeIntensity, 0.0f); // ScreenShake When Dmg
	program->setViewMatrix(viewMatrix);
	//Draw Map
	drawTile(TileMapTexture, 16, 8);
	//Draw Players
	drawEntity(player1, playerTexture, 8, 4);
	drawEntity(player2, playerTexture, 8, 4);
	//Draw Pickups
	for (int i = 0; i < pickups.size(); i++){
		drawEntity(pickups[i], pickupsTexture, 14, 7);
	}
	//Draw enemies
	for (int i = 0; i < enemies.size(); i++){
		drawEntity(enemies[i], playerTexture, 8, 4);
	}
	//Draw Bullets
	drawBullet(pickupsTexture, 6, 14, 7);
	//Draw Info
	viewMatrix.identity();
	viewMatrix.Scale(1.7f, 1.0f, 1.0f);
	program->setViewMatrix(viewMatrix);
	drawText(FontTexture, "Player2", 0.23f, 0.001f, -2.8f, 2.6f);
	drawText(FontTexture, "HP: " + to_string(player2.health), 0.23f, 0.001f, -2.8f, 2.40f);
	drawText(FontTexture, "ATK:" + to_string(player2.attack), 0.23f, 0.001f, -2.8f, 2.23f);
	drawText(FontTexture, "DEF:" + to_string(player2.defense), 0.23f, 0.001f, -2.8f, 2.06f);
	drawText(FontTexture, "SP: " + to_string(player2.speed), 0.23f, 0.001f, -2.8f, 1.89f);
	program->setViewMatrix(viewMatrix);
}
void renderGameOver(){
	glViewport(0, 0, screenWildth, screenHeight);
	drawEntity(entity(0, 10, 8), backgroundTexture, 1, 1); //Background

	if (winner==1){
		glViewport(0, 0, screenWildth / 2, screenHeight);
		drawText(FontTexture, "Winner", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "HP: " + to_string(player1.health), 0.5f, 0.001f, -3.2f, 0.0f);
		drawText(FontTexture, "ATK:" + to_string(player1.attack), 0.5f, 0.001f, -3.2f, -0.4f);
		drawText(FontTexture, "DEF:" + to_string(player1.defense), 0.5f, 0.001f, -3.2f, -0.8f);
		drawText(FontTexture, "SP: " + to_string(player1.speed), 0.5f, 0.001f, -3.2f, -1.2f);
		drawText(FontTexture, "Time: " + to_string(totalTime) + " seconds", 0.5f, 0.001f, -3.2f, -1.6f);
		program->setViewMatrix(viewMatrix);
		glViewport(screenWildth / 2, 0, screenWildth / 2, screenHeight);
		drawText(FontTexture, "Loser", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "HP: " + to_string(player2.health), 0.5f, 0.001f, -3.2f, 0.0f);
		drawText(FontTexture, "ATK:" + to_string(player2.attack), 0.5f, 0.001f, -3.2f, -0.4f);
		drawText(FontTexture, "DEF:" + to_string(player2.defense), 0.5f, 0.001f, -3.2f, -0.8f);
		drawText(FontTexture, "SP: " + to_string(player2.speed), 0.5f, 0.001f, -3.2f, -1.2f);
		drawText(FontTexture, "Time: " + to_string(totalTime) + " seconds", 0.5f, 0.001f, -3.2f, -1.6f);
	}
	else if (winner==2){
		glViewport(0, 0, screenWildth / 2, screenHeight);
		drawText(FontTexture, "Winner", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "HP: " + to_string(player1.health), 0.5f, 0.001f, -3.2f, 0.0f);
		drawText(FontTexture, "ATK:" + to_string(player1.attack), 0.5f, 0.001f, -3.2f, -0.4f);
		drawText(FontTexture, "DEF:" + to_string(player1.defense), 0.5f, 0.001f, -3.2f, -0.8f);
		drawText(FontTexture, "SP: " + to_string(player1.speed), 0.5f, 0.001f, -3.2f, -1.2f);
		drawText(FontTexture, "Time: " + to_string(totalTime) + " seconds", 0.5f, 0.001f, -3.2f, -1.6f);
		program->setViewMatrix(viewMatrix);
		glViewport(screenWildth / 2, 0, screenWildth / 2, screenHeight);
		drawText(FontTexture, "Loser", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "HP: " + to_string(player2.health), 0.5f, 0.001f, -3.2f, 0.0f);
		drawText(FontTexture, "ATK:" + to_string(player2.attack), 0.5f, 0.001f, -3.2f, -0.4f);
		drawText(FontTexture, "DEF:" + to_string(player2.defense), 0.5f, 0.001f, -3.2f, -0.8f);
		drawText(FontTexture, "SP: " + to_string(player2.speed), 0.5f, 0.001f, -3.2f, -1.2f);
		drawText(FontTexture, "Time: " + to_string(totalTime) + " seconds", 0.5f, 0.001f, -3.2f, -1.6f);
	}
	else{
		glViewport(0, 0, screenWildth / 2, screenHeight);
		drawText(FontTexture, "Loser", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "HP: " + to_string(player1.health), 0.5f, 0.001f, -3.2f, 0.0f);
		drawText(FontTexture, "ATK:" + to_string(player1.attack), 0.5f, 0.001f, -3.2f, -0.4f);
		drawText(FontTexture, "DEF:" + to_string(player1.defense), 0.5f, 0.001f, -3.2f, -0.8f);
		drawText(FontTexture, "SP: " + to_string(player1.speed), 0.5f, 0.001f, -3.2f, -1.2f);
		drawText(FontTexture, "Time: " + to_string(totalTime) + " seconds", 0.5f, 0.001f, -3.2f, -1.6f);
		program->setViewMatrix(viewMatrix);
		glViewport(screenWildth / 2, 0, screenWildth / 2, screenHeight);
		drawText(FontTexture, "Loser", 0.6f, 0.001f, -2.0f, 1.6f);
		drawText(FontTexture, "HP: " + to_string(player2.health), 0.5f, 0.001f, -3.2f, 0.0f);
		drawText(FontTexture, "ATK:" + to_string(player2.attack), 0.5f, 0.001f, -3.2f, -0.4f);
		drawText(FontTexture, "DEF:" + to_string(player2.defense), 0.5f, 0.001f, -3.2f, -0.8f);
		drawText(FontTexture, "SP: " + to_string(player2.speed), 0.5f, 0.001f, -3.2f, -1.2f);
		drawText(FontTexture, "Time: " + to_string(totalTime) + " seconds", 0.5f, 0.001f, -3.2f, -1.6f);
	}
}

void setup(){
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Battle League", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWildth, screenHeight, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);  // Initialize SDL_mixer for Audio
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix.setOrthoProjection(-5.0, 5.0, -2.817f, 2.817f, -1.0f, 1.0f);

	glUseProgram(program->programID);

	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	srand(time(0));
	setPickupVector(pickups, 10, 0.3);
	setenemyVector(enemies, 10, 0.44);
	player1.width = 0.27f;
	player2.width = 0.27f;

	loadTextureAndSound();
	createMapArray();

	Mix_PlayMusic(bkMusic, -1);
	Mix_VolumeMusic(15);

}
void processEvents(){
	// Input Event
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event.type == SDL_KEYDOWN){ //Close the Game with Escape
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
				done = true;
			}
		}
		if (state == State_Main_Menu){
			if (event.type == SDL_KEYDOWN){ //User choose Mode
				if (event.key.keysym.scancode == SDL_SCANCODE_1){
					level = 1;
					state = State_Game;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_2){
					level = 2;
					state = State_Game;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_3){
					level = 3;
					state = State_Game;
				}
			}
		}
		else if (state == State_Game){
			if (event.type == SDL_KEYDOWN){
				if (event.key.keysym.scancode == SDL_SCANCODE_W){ //Player1 Jump
					if (player1.collidedBottom == 1){
						player1.vel_y = velc - 1.0;
						player1.collidedBottom = 0;
						Mix_PlayChannel(2, jumpSound, 0);
					}
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE){ //Player1 Shoot
					shootBullet(player1);
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_UP){ //player2 Jump
					if (player2.collidedBottom == 1){
						player2.vel_y = velc - 1.0;
						player2.collidedBottom = 0;
						Mix_PlayChannel(2, jumpSound, 0);
					}
				}
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN){ //Player2 Shoot
				shootBullet(player2);
			}
		}
	}
	//Polling Event
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (state == State_Game){
		if (keys[SDL_SCANCODE_A]){ //Player1 control
			player1.acc_x = -velc - player1.speed;
			player1.xDirection = -1;
		}
		else if (keys[SDL_SCANCODE_D]){
			player1.acc_x = velc + player1.speed;
			player1.xDirection = 1;
		}
		else{
			player1.acc_x = 0;
		}
		if (keys[SDL_SCANCODE_LEFT]){ //Player2 control
			player2.acc_x = -velc - player2.speed;
			player2.xDirection = -1;
		}
		else if (keys[SDL_SCANCODE_RIGHT]){
			player2.acc_x = velc + player2.speed;
			player2.xDirection = 1;
		}
		else{
			player2.acc_x = 0;
		}
	}
}
void update(){
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;

	switch (state){
	case State_Main_Menu:{
		updateMainMenu();
		break;
	}
	case State_Game:{
		
		gameTimer += elapsed;
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	screenShakeValue += elapsed;

	switch (state){
	case State_Main_Menu:
		viewMatrix.identity();
		screenShakeIntensity -= elapsed;
		if (screenShakeIntensity < 0)
			screenShakeIntensity = 0;
		//viewMatrix.Translate(0.0f, sin(screenShakeValue * 1000.0f) * screenShakeIntensity, 0.0f);
		viewMatrix.Translate(sin(screenShakeValue * 1000.0f) * screenShakeIntensity, sin(screenShakeValue * 1000.0f) * screenShakeIntensity, 0.0f);

		program->setViewMatrix(viewMatrix);
		renderMainMenu();
		break;
	case State_Game:
		renderPlayer1Game();
		renderPlayer2Game();
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
	Mix_FreeChunk(jumpSound);
	Mix_FreeChunk(collectSound);
	Mix_FreeChunk(hitPlayerSound);
	Mix_FreeChunk(killEnemySound);
	Mix_FreeChunk(shootSound);
	Mix_FreeChunk(winSound);
	Mix_FreeMusic(bkMusic);
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
