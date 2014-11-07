#include "SceneState.h"

class MoverScript : public Script {
protected:
	glm::vec3 velocity;
public:
	MoverScript(SceneGraphNode *n) : Script(n) { velocity = glm::vec3(0); }
	Script* MoverScript::clone(SceneGraphNode *n) override { return new MoverScript(n); } 
	~MoverScript(); //In case there are any properties above that are pointers we need to delete.
	void initProperty(FILE *F, const string& propertyName, const string& propertyVal) override;
	void setProperty(const string& propertyName, const string& propertyVal) override;
	void update(Camera& cam, double dt) override;
};