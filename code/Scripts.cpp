#include "Scripts.h"

void MoverScript::initProperty(FILE *F, const string& propertyName, const string& propertyVal) {
	if (name == "velocity") getFloats(F, &velocity[0], 3);
}
bool MoverScript::setProperty(const string& propertyName, const string& propertyVal) {
	if (name == "velocity") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &velocity.x, &velocity.y, &velocity.z);
}
void MoverScript::update(Camera& cam, float dt) {
	assert(node != nullptr);
	node->addTranslation(dt*velocity);
}