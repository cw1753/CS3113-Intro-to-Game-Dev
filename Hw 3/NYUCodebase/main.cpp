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
#define toAngle 3.1416/180
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

	float width;
	float height;

	float speed;
	float direction_x;
	float direction_y;
	
	bool Alive = 1;
	bool player;

};

SDL_Window* displayWindow;

ShaderProgram *program;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

enum GameState{State_Main_Menu, State_Game, State_Game_Over};
GameState state;

GLuint FontTexture;
GLuint spaceShipTexture;
SDL_Event event;
bool done = false;
float lastFrameTicks = 0.0f;
float elapsed;
float gameTimer = 0.0f;
float bulletPerSecond = 4.0f;
float velc = 6.0f;
bool start = 0;
int score = 0;
bool win = 0;
int initialSpaceVectorSize = 0;
int colCount = 0;


vector<entity> spaceShipVector;
vector<entity> bulletVector;
vector<int> lowestShips;


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
void drawMyShip(int spriteTexture, int index, int spriteCountX, int spriteCountY) {
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

	if (spaceShipVector[0].Alive){
		modelMatrix.Translate(spaceShipVector[0].x, spaceShipVector[0].y, 0.0f);
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
void drawEnermyShip(int spriteTexture, int index, int spriteCountX, int spriteCountY) {
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

	for (int i = 0; i < spaceShipVector.size(); i++){ //draw all the ship
		if (spaceShipVector[i].Alive){
			modelMatrix.Translate(spaceShipVector[i].x, spaceShipVector[i].y, 0.0f);
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
	float vertices[] = { -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, -0.1f };

	for (int i = 0; i < bulletVector.size(); i++){ //draw all the Bullet
		modelMatrix.Translate(bulletVector[i].x, bulletVector[i].y, 0.0f);
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

bool shouldRemoveShip(entity ship){
	if (!ship.Alive){
		return true;
	}
	else{
		return false;
	}
}
void shootBullet(entity ship, bool myship){
	entity bullet;
	bullet.width = 0.14f;
	bullet.height = 0.14f;
	bullet.x = ship.x;
	if (myship){   //MyShip's bullet
		bullet.y = ship.y + ship.height / 2 + bullet.height / 2 + 0.001;
		bullet.speed = 20.0f;
		bullet.player = 1;
	}
	else{          //Enemey's Bullet
		bullet.y = ship.y - ship.height / 2 - bullet.height / 2 - 0.151;
		bullet.speed = -2.0f;
		bullet.player = 0;
	}
	
	
	bullet.Alive = 1;
	bulletVector.push_back(bullet);
}
bool shouldRemoveBullet(entity bullet){
	//Remove Bullet by time limit or position limit
	if (abs(bullet.y) >= 2.7){
		return 1;
	}
	else if (!bullet.Alive)
		return 1;
	else
		return 0;
}
bool collsion(entity B1, entity B2){
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
void updateGame(){
	//Bullets (Move y)
	for (int i = 0; i < bulletVector.size(); i++){
		bulletVector[i].y += bulletVector[i].speed*elapsed;
	}
	
	//InvadersShip (Move x)
	for (int i = 1; i < spaceShipVector.size(); i++){
		if (spaceShipVector[i].Alive)
			spaceShipVector[i].x += spaceShipVector[i].speed*elapsed;
	}

	//MyShip (Bound and Position Update)
	if (spaceShipVector[0].speed > 0){
		if (spaceShipVector[0].x + spaceShipVector[0].width / 2 < 5.0f){
			spaceShipVector[0].x += spaceShipVector[0].speed*elapsed;
		}
		else{
			spaceShipVector[0].x = 5.0f - spaceShipVector[0].width / 2;
		}
	}
	else{
		if (spaceShipVector[0].x - spaceShipVector[0].width / 2 > -5.0f){
			spaceShipVector[0].x += spaceShipVector[0].speed*elapsed;
		}
		else{
			spaceShipVector[0].x =-5.0f + spaceShipVector[0].width / 2;
		}
	}

	//Enermy Ship Shoots Bullet
	if (gameTimer > 1.0f/bulletPerSecond){
		gameTimer = 0.0f;
		int randomShip = rand() % colCount;
		if (lowestShips[randomShip] > 0){
			shootBullet(spaceShipVector[lowestShips[randomShip]], 0);
		}
	}

	//Collsion Checking for my bullet and spaceship, bullet and myShip, bullet and bullet
	for (int b = 0; b < bulletVector.size(); b++){
		if (bulletVector[b].player){ // My bullet with Enermy Ship
			for (int s = 0; s < lowestShips.size(); s++){
				if (lowestShips[s] > 0){
					if (collsion(bulletVector[b], spaceShipVector[lowestShips[s]])){
						score += 10;
						spaceShipVector[lowestShips[s]].Alive = 0;
						lowestShips[s] -= colCount;
						bulletVector[b].Alive = 0;
						break;
					}
				}
			}
		}
		else{  // Enermy bullet with myShip
			if (collsion(bulletVector[b], spaceShipVector[0])){
				bulletVector[b].Alive = 0;
				spaceShipVector[0].Alive = 0;
				break;
			}
		}
		for (int eb = b+1; eb < bulletVector.size(); eb++){ //Bullet and Bullet
			if (collsion(bulletVector[b], bulletVector[eb])){
				bulletVector[b].Alive = 0;
				bulletVector[eb].Alive = 0;
			}
		}
	}

	//InvaderShip
	for (int i = 1; i < spaceShipVector.size(); i++){
		if (spaceShipVector[i].Alive){
			if (spaceShipVector[i].x + spaceShipVector[i].width / 2 >= 5.0f && spaceShipVector[i].speed > 0){
				float pent = spaceShipVector[i].x + spaceShipVector[i].width / 2 - 5.0f;
				for (int k = 1; k < spaceShipVector.size(); k++){
					spaceShipVector[k].x -= pent;
					spaceShipVector[k].y -= 0.15f;
					spaceShipVector[k].speed = -spaceShipVector[k].speed;
				}
				break;
			}
			else if (spaceShipVector[i].x - spaceShipVector[i].width / 2 <= -5.0f && spaceShipVector[i].speed < 0){
				float pent = spaceShipVector[i].x - spaceShipVector[i].width / 2 - -5.0f;
				for (int k = 1; k < spaceShipVector.size(); k++){
					spaceShipVector[k].x -= pent;
					spaceShipVector[k].y -= 0.15f;
					spaceShipVector[k].speed = -spaceShipVector[k].speed;
				}
				break;
			}
		}
	}

	//Remove Not Alive Ship
	//spaceShipVector.erase(remove_if(spaceShipVector.begin(), spaceShipVector.end(), shouldRemoveShip), spaceShipVector.end());

	//Remove Bullet
	bulletVector.erase(remove_if(bulletVector.begin(), bulletVector.end(), shouldRemoveBullet), bulletVector.end());
	
	//Checking for Game Over (losing or Winning)
	for (int i = spaceShipVector.size() - 1; i > 0; i--){  // Check if Enermy Ship reach the bottem
		if (spaceShipVector[i].Alive){
			if (spaceShipVector[i].y - spaceShipVector[i].height / 2 <= spaceShipVector[0].y + spaceShipVector[0].height / 2)
				state = State_Game_Over;
			break;
		}
	}
	if (!spaceShipVector[0].Alive){  //Check if myShip is alive
		state = State_Game_Over;
	}
	else if (score == (initialSpaceVectorSize - 1) * 10){ //Check if all Enermy Ships are destoryed
		state = State_Game_Over;
		win = 1;
	}
}
void updateGameOver(){}

void renderMainMenu(){
	drawText(FontTexture, "Space Invaders", 0.6f, 0.001f, -3.9f, 1.6f);
	drawText(FontTexture, "Press", 0.5f, 0.001f, -1.0f, 0.0f);
	drawText(FontTexture, "SPACEBAR", 0.5f, 0.001f, -1.8f, -1.0f);
}
void renderGame(){
	drawEnermyShip(spaceShipTexture, 15, 8, 4);
	drawMyShip(spaceShipTexture, 17, 8, 4);
	drawBullet(spaceShipTexture, 1, 8, 4);
	
	string count = "Score: " + to_string(score);
	drawText(FontTexture, count, 0.4f, 0.001f, -4.7f, -2.5f);
}
void renderGameOver(){
	if (win){
		drawText(FontTexture, "Winner", 0.7f, 0.001f, -1.8f, 2.0f);
	}
	else{
		drawText(FontTexture, "GameOver", 0.8f, 0.001f, -3.0f, 2.0f);
	}
	drawText(FontTexture, "Score: " + to_string(score), 0.5f, 0.001f, -3.0f, 0.0f);
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

	float xtemp = -3.3f;
	float ytemp = 2.45f;
	entity myShip;
	myShip.width = 0.27f;
	myShip.height = 0.27f;
	myShip.x = 0.0f;
	myShip.y = -2.1f;
	spaceShipVector.push_back(myShip);
	while (ytemp >= 0.0f){
		while (xtemp <= 3.3f){
			entity spaceShip;
			spaceShip.width = 0.27f;
			spaceShip.height = 0.27f;
			spaceShip.x = xtemp;
			spaceShip.y = ytemp;
			spaceShip.speed = 0.8f;
			spaceShipVector.push_back(spaceShip);
			xtemp += 0.5f;
		}
		xtemp = -3.3f;
		ytemp -= 0.5f;
	}
	initialSpaceVectorSize = spaceShipVector.size();
	colCount = (initialSpaceVectorSize - 1) / 5;
	for (int i = initialSpaceVectorSize - colCount; i < initialSpaceVectorSize; i++){
		lowestShips.push_back(i);
	}
	FontTexture = LoadTexture("font1.png");
	spaceShipTexture = LoadTexture("characters.png");
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
					shootBullet(spaceShipVector[0], 1);
				}
			}
		}
	}
	if (state == State_Main_Menu){ // start playing by pressing spacebar
		if (keys[SDL_SCANCODE_SPACE]){
			state = State_Game;
		}
	}
	if (keys[SDL_SCANCODE_LEFT]){
		spaceShipVector[0].speed = -velc;
	}
	else if (keys[SDL_SCANCODE_RIGHT]){
		spaceShipVector[0].speed = velc;
	}
	else{
		spaceShipVector[0].speed = 0;
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
			updateGame();
			break;
		}
		case State_Game_Over:
			updateGameOver();
			break;
	}
	
	
}
void render(){
	//glClearColor(0.4f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	switch (state){
		case State_Main_Menu:
			renderMainMenu();
			break;
		case State_Game:
			renderGame();
			break;
		case State_Game_Over:
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
