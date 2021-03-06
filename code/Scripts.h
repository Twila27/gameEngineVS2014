#pragma once
#include "SceneState.h"

class MoverScript : public Script {
protected:
	glm::vec3 velocity;
	glm::vec3 minSpeed; //What we increment and decrement our velocity with.
public:
	MoverScript(SceneGraphNode *n) : Script(n) { type = "moverScript"; minSpeed = velocity = glm::vec3(0); }
	Script* clone(SceneGraphNode *n) override { return new MoverScript(n); }
	void postParseInit() override { return; }
	~MoverScript(); //In case there are any properties above that are pointers we need to delete.
	bool setProperty(const string& propertyName, const string& propertyVal) override;
	void update(Camera& cam, double dt) override;
	void toSDL(FILE *F, const char* tabs) override;
};

class EmitterScript : public Script {
protected:
	struct Particle {
		Billboard card; //Generalize to Drawable?
		Transform T;
		glm::vec3 velocity;
		float timeToLive;
	};
	Particle p;
	glm::vec3 rotOffset = glm::vec3(0);
	glm::vec3 posOffset = glm::vec3(0); //Offsets used to vary pos/rot around the node this emitterScript attaches to.
	glm::vec3 avgVelocity = glm::vec3(0);
	list<Particle*> particles; //List to enable faster removal from head--oldest particles will always be at or near [0].
	bool active;
	int particleMax;
	float emitRate; //Particles per update frame.
	float currAccumulatedTime;
public:
	EmitterScript(SceneGraphNode *n);
	Script* clone(SceneGraphNode *n) override { return new EmitterScript(n); }
	void postParseInit() override { return; }
	~EmitterScript() { for (auto it = particles.begin(); it != particles.end(); ++it) delete *it; }
	bool setProperty(const string& propertyName, const string& propertyVal) override;
	void update(Camera& cam, double dt) override;
	void toSDL(FILE *F, const char* tabs) override;
};

class RGBGameScript : public Script {
protected:
	enum Enemy {NORTH = 1, SOUTH, EAST, WEST};
	enum Color {R = 1, G, B};
	bool cooldown = true; //Used to control when player can attack, which is between after enemy ticks.
	const float bgColorChangeAmt = 0.1f;
	const float colorThreshold = 5.0f;
	int accEnemyTicks = 0;
	SceneGraphNode *currAttacker;
	int currAttackerId;
	bool hit, block, counterR, counterG, counterB, counteredR, counteredG, counteredB;
	double accTime;
	glm::vec4 order;
	float enemyTickRate;
	int initPlayerHP, initEnemyHP;
	int maxPlayerHP, maxEnemyHP, currMaxPlayerHP;
	SceneGraphNode *N, *S, *E, *W, *P;
	SceneGraphNode *WinText, *LossText;
	SceneGraphNode *currTarget;
	ISound *winSound, *lossSound, *deathSound, *hitSound, *enemySound;
public:
	RGBGameScript(SceneGraphNode *n);
	Script* clone(SceneGraphNode *n) override { return new RGBGameScript(n); }
	void postParseInit() override;
	bool setProperty(const string& propertyName, const string& propertyVal) override;
	void update(Camera& cam, double dt) override;
	void toSDL(FILE *F, const char* tabs) override;
	SceneGraphNode* getAttackerFromInt(int id);
	int getNextEnemyFromOrder();
	void healPlayer();
	void hurtPlayer();
};