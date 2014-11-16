#pragma once
#include "SceneState.h"

class MoverScript : public Script {
protected:
	glm::vec3 velocity;
	glm::vec3 minSpeed; //What we increment and decrement our velocity with.
public:
	MoverScript(SceneGraphNode *n) : Script(n) { velocity = glm::vec3(0); }
	Script* MoverScript::clone(SceneGraphNode *n) override { return new MoverScript(n); } 
	~MoverScript(); //In case there are any properties above that are pointers we need to delete.
	void initProperty(FILE *F, const string& propertyName, const string& propertyVal) override;
	bool setProperty(const string& propertyName, const string& propertyVal) override;
	void update(Camera& cam, float dt) override;
};