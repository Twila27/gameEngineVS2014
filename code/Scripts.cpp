#include "Scripts.h"

void MoverScript::initProperty(FILE *F, const string& propertyName, const string& propertyVal) {
	if (name == "velocity") getFloats(F, &velocity[0], 3);
}
bool MoverScript::setProperty(const string& propertyName, const string& propertyVal) {
	//if (name == "velocity") parseVec3 or getFloats revision to work off propertyVal!
	return false;
}
void MoverScript::update(Camera& cam, double dt) {

}