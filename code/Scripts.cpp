#include "Scripts.h"

void MoverScript::initProperty(FILE *F, const string& propertyName, const string& propertyVal) {
	if (name == "velocity") getFloats(F, &velocity[0], 3);
}
bool MoverScript::setProperty(const string& propertyName, const string& propertyVal) {
	if (propertyName == "velocity") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &velocity.x, &velocity.y, &velocity.z);
	if (propertyName == "minSpeed") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &minSpeed.x, &minSpeed.y, &minSpeed.z);
}
void MoverScript::update(Camera& cam, float dt) {
	assert(node != nullptr);
	if (gWindow == nullptr) return;
	if (glfwGetKey(gWindow, 'A')) node->addTranslation(glm::vec3(-dt*velocity.x, 0, 0));
	if (glfwGetKey(gWindow, 'D')) node->addTranslation(glm::vec3(dt*velocity.x, 0, 0));
	if (glfwGetKey(gWindow, 'W')) node->addTranslation(glm::vec3(0, 0, -dt*velocity.z));
	if (glfwGetKey(gWindow, 'S')) node->addTranslation(glm::vec3(0, 0, dt*velocity.z));
	if (glfwGetKey(gWindow, 'Q')) node->addTranslation(glm::vec3(0, -dt*velocity.y, 0));
	if (glfwGetKey(gWindow, 'E')) node->addTranslation(glm::vec3(0, dt*velocity.y, 0));
	if (glfwGetKey(gWindow, '=')) velocity += minSpeed;
	if (glfwGetKey(gWindow, '-')) velocity -= minSpeed;
	//if (glfwGetKey(gWindow, GLFW_KEY_LEFT)) cam.rotateGlobal(glm::vec3(0, 1, 0), rAmt);
	//if (glfwGetKey(gWindow, GLFW_KEY_RIGHT)) cam.rotateGlobal(glm::vec3(0, 1, 0), -rAmt);
	//if (glfwGetKey(gWindow, GLFW_KEY_UP)) cam.rotateLocal(glm::vec3(1, 0, 0), rAmt);
	//if (glfwGetKey(gWindow, GLFW_KEY_DOWN)) cam.rotateLocal(glm::vec3(1, 0, 0), -rAmt);
	node->T.refreshTransform();
}