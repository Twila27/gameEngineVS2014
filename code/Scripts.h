#include "SceneState.h"

class MoverScript : public Script {
protected:
	glm::vec3 velocity;
public:
	MoverScript(SceneGraphNode *n) : Script(n) {velocity = glm::vec3(0);}
	~MoverScript(); //In case there are any properties above that are pointers we need to delete.
	void setProperty(const string& propertyName, const string& propertyVal) override;
};