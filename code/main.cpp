
//-------------------------------------------------------------------------//
// Transforms - Tests using transforms for objects
// David Cline
// June 27, 2014
//-------------------------------------------------------------------------//

#include "EngineUtil.h"

//-------------------------------------------------------------------------//
// Global State.  Eventually, this should include the global 
// state of the system, including multiple scenes, objects, shaders, 
// cameras, and all other resources needed by the system.
//-------------------------------------------------------------------------//

GLFWwindow* gWindow = NULL;
string gWindowTitle = "OpenGL App";
int gWidth = 600; // window width
int gHeight = 600; // window height
int gSPP = 16; // samples per pixel
glm::vec4 backgroundColor;

ISoundEngine* soundEngine = NULL;
ISound* music = NULL;

map<string, TriMesh*> gMeshes;
map<string, Material*> gMaterials;
vector<TriMeshInstance*> gMeshInstances;
vector<Camera*> gCameras;
vector<char*> gSceneFileNames;

vector<Light*> gLights;
int gMaxLights = 8;
GLuint gLightsUBO;

//These will not change until their keys are pressed.
unsigned int gActiveCamera = 0;
unsigned int gActiveScene = 0;
bool gShouldSwapScene = false;

//For controlling keyboard movement and rotation speeds.
const float step = 0.1f;
const float crawl = 0.01f;
double curr_xx, prev_xx, curr_yy, prev_yy;

//-------------------------------------------------------------------------//
// Callback for Keyboard Input
//-------------------------------------------------------------------------//

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	//Handle discrete movements.
	//Ctrl changes cameras, Shift changes scenes.
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_LEFT:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) gActiveCamera = (gActiveCamera == 0) ? gCameras.size() - 1 : gActiveCamera - 1;
			else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
				gActiveScene = (gActiveScene == 0) ? gSceneFileNames.size() - 1 : gActiveScene - 1;
				gShouldSwapScene = true;
			}
			else gCameras[gActiveCamera]->translateGlobal(glm::vec3(-step, 0, 0));
			break;
		case GLFW_KEY_RIGHT:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) gActiveCamera = (gActiveCamera == gCameras.size() - 1) ? 0 : gActiveCamera + 1;
			else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
				gActiveScene = (gActiveScene == gSceneFileNames.size() - 1) ? 0 : gActiveScene + 1;
				gShouldSwapScene = true;
			}
			else gCameras[gActiveCamera]->translateLocal(glm::vec3(step, 0, 0));
			break;
		case GLFW_KEY_DOWN: gCameras[gActiveCamera]->translateLocal(glfwGetKey(window, GLFW_KEY_LEFT_ALT) ? glm::vec3(0, 0, step) : glm::vec3(0, step, 0)); break;
		case GLFW_KEY_UP: gCameras[gActiveCamera]->translateLocal(glfwGetKey(window, GLFW_KEY_LEFT_ALT) ? glm::vec3(0, 0, -step) : glm::vec3(0, -step, 0)); break;
		case GLFW_KEY_Q: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 0, 1), crawl); break;
		case GLFW_KEY_E: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 0, 1), -crawl); break;
		case GLFW_KEY_W: gCameras[gActiveCamera]->rotateLocal(glm::vec3(1, 0, 0), crawl); break;
		case GLFW_KEY_S: gCameras[gActiveCamera]->rotateLocal(glm::vec3(1, 0, 0), -crawl); break;
		case GLFW_KEY_A: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 1, 0), crawl); break;
		case GLFW_KEY_D: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 1, 0), -crawl); break;
		}
	}

	//Handle continuous movements.
	if (action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_LEFT: gCameras[gActiveCamera]->translateLocal(glm::vec3(-crawl, 0, 0)); break;
		case GLFW_KEY_RIGHT: gCameras[gActiveCamera]->translateLocal(glm::vec3(crawl, 0, 0)); break;
		case GLFW_KEY_DOWN: gCameras[gActiveCamera]->translateLocal(glfwGetKey(window, GLFW_KEY_LEFT_ALT) ? glm::vec3(0, 0, crawl) : glm::vec3(0, crawl, 0)); break;
		case GLFW_KEY_UP: gCameras[gActiveCamera]->translateLocal(glfwGetKey(window, GLFW_KEY_LEFT_ALT) ? glm::vec3(0, 0, -crawl) : glm::vec3(0, -crawl, 0)); break;
		case GLFW_KEY_Q: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 0, 1), crawl); break;
		case GLFW_KEY_E: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 0, 1), -crawl); break;
		case GLFW_KEY_W: gCameras[gActiveCamera]->rotateLocal(glm::vec3(1, 0, 0), crawl); break;
		case GLFW_KEY_S: gCameras[gActiveCamera]->rotateLocal(glm::vec3(1, 0, 0), -crawl); break;
		case GLFW_KEY_A: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 1, 0), crawl); break;
		case GLFW_KEY_D: gCameras[gActiveCamera]->rotateLocal(glm::vec3(0, 1, 0), -crawl); break;
		}
	}

	if (action == GLFW_PRESS &&
		((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))) {
		printf("\n%c\n", (char)key);
	}
}

//-------------------------------------------------------------------------//
// Parse Scene File
//-------------------------------------------------------------------------//

string ONE_TOKENS = "{}[]()<>+-*/,;";

void loadWorldSettings(FILE *F)
{
	string token, t;
	while (getToken(F, token, ONE_TOKENS)) {
		//cout << "  " << token << endl;
		if (token == "}") break;
		if (token == "windowTitle") getToken(F, gWindowTitle, ONE_TOKENS);
		else if (token == "width") getInts(F, &gWidth, 1);
		else if (token == "height") getInts(F, &gHeight, 1);
		else if (token == "spp") getInts(F, &gSPP, 1);
		else if (token == "backgroundColor") getFloats(F, &backgroundColor[0], 3);
		else if (token == "backgroundMusic") {
			string fileName, fullFileName;
			getToken(F, fileName, ONE_TOKENS);
			getFullFileName(fileName, fullFileName);
			
			//Only returns ISound* if 'track', 'startPaused' or 'enableSoundEffects' are true.
			ISound* music = soundEngine->play2D(fullFileName.c_str(), true); 
			
		}
		else if (token == "maxLights") getInts(F, &gMaxLights, 1);
	}

	// Initialize the window with OpenGL context
	gWindow = createOpenGLWindow(gWidth, gHeight, gWindowTitle.c_str(), gSPP);
	glfwSetKeyCallback(gWindow, keyCallback);

	// Generate a buffer that will send the lights to OpenGL.
	glGenBuffers(1, &gLightsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, gLightsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(gLights), nullptr, GL_STREAM_DRAW); //Unlike glBufferSubData(), actually allocates data!
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void loadMesh(FILE *F)
{
	string token, meshName(""), fileName("");

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "name") getToken(F, meshName, ONE_TOKENS);
		else if (token == "file") getToken(F, fileName, ONE_TOKENS);
	}

	gMeshes[meshName] = new TriMesh();
	gMeshes[meshName]->setName(meshName);
	gMeshes[meshName]->readFromPly(fileName, false);
	gMeshes[meshName]->sendToOpenGL();
}

void loadMaterial(FILE *F)
{
	string token, materialName("");
	GLuint vertexShader = NULL_HANDLE;
	GLuint fragmentShader = NULL_HANDLE;
	GLuint shaderProgram = NULL_HANDLE;

	Material *m = new Material();

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "name") getToken(F, materialName, ONE_TOKENS);
		else if (token == "vertexShader") {
			string vsFileName;
			getToken(F, vsFileName, ONE_TOKENS);
			vertexShader = loadShader(vsFileName.c_str(), GL_VERTEX_SHADER);
		}
		else if (token == "fragmentShader") {
			string fsFileName;
			getToken(F, fsFileName, ONE_TOKENS);
			fragmentShader = loadShader(fsFileName.c_str(), GL_FRAGMENT_SHADER);
		}
		else if (token == "diffuseColor") getFloats(F, &(m->diffuseColor[0]), 4);
		else if (token == "specularColor") getFloats(F, &(m->specularColor[0]), 4);
		else if (token == "specularExponent") getFloats(F, &(m->specularExponent), 1);
		else if (token == "ambientIntensity") getFloats(F, &(m->ambientIntensity[0]), 4);
		else if (token == "emissiveColor") getFloats(F, &(m->emissiveColor[0]), 4);
		else if (token == "diffuseTexture") {
			string texFileName;
			getToken(F, texFileName, ONE_TOKENS);
			m->textures.push_back(new RGBAImage());
			m->textures[Material::TEXTURE_TYPE::DIFFUSE]->loadPNG(texFileName);
			m->textures[Material::TEXTURE_TYPE::DIFFUSE]->sendToOpenGL();
		}
		else if (token == "specularTexture") {
			string texFileName;
			getToken(F, texFileName, ONE_TOKENS);
			m->textures.push_back(new RGBAImage());
			m->textures[Material::TEXTURE_TYPE::SPECULAR]->loadPNG(texFileName);
			//m->textures[Material::TEXTURE_TYPE::SPECULAR]->sendToOpenGL();
		}
		else if (token == "specularExponentTexture") {
			string texFileName;
			getToken(F, texFileName, ONE_TOKENS);
			m->textures.push_back(new RGBAImage());
			m->textures[Material::TEXTURE_TYPE::SPECULAR_EXPONENT]->loadPNG(texFileName);
			//m->textures[Material::TEXTURE_TYPE::SPECULAR_EXPONENT]->sendToOpenGL();
		}
		else if (token == "emissiveTexture") {
			string texFileName;
			getToken(F, texFileName, ONE_TOKENS);
			m->textures.push_back(new RGBAImage());
			m->textures[Material::TEXTURE_TYPE::EMISSIVE]->loadPNG(texFileName);
			//m->textures[Material::TEXTURE_TYPE::EMISSIVE]->sendToOpenGL();
		}
		else if (token == "normalMap") {
			string texFileName;
			getToken(F, texFileName, ONE_TOKENS);
			m->textures.push_back(new RGBAImage());
			m->textures[Material::TEXTURE_TYPE::NORMAL]->loadPNG(texFileName);
			//m->textures[Material::TEXTURE_TYPE::NORMAL]->sendToOpenGL();
		}
		else if (token == "envReflectionMap") {
			string texFileName;
			getToken(F, texFileName, ONE_TOKENS);
			m->textures.push_back(new RGBAImage());
			m->textures[Material::TEXTURE_TYPE::REFLECTION]->loadPNG(texFileName);
			//m->textures[Material::TEXTURE_TYPE::REFLECTION]->sendToOpenGL();
		}
	}

	m->setShaderProgram(createShaderProgram(vertexShader, fragmentShader)); //Return to modify this to take a container of shaders.
	m->gLightsHandle = new vector<Light*>;
	*m->gLightsHandle = gLights;
	m->setLightUBOHandle(gLightsUBO);
	m->getAndInitUniforms();
	if (materialName != "") gMaterials[materialName] = m;
}

void loadMeshInstance(FILE *F)
{
	string token;
	GLuint vertexShader = NULL_HANDLE;
	GLuint fragmentShader = NULL_HANDLE;
	GLuint shaderProgram = NULL_HANDLE;

	gMeshInstances.push_back(new TriMeshInstance());

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") {
			break;
		}
		else if (token == "translation") getFloats(F, &(gMeshInstances.back()->instanceTransform.translation[0]), 3);
		else if (token == "rotation") getFloats(F, &(gMeshInstances.back()->instanceTransform.rotation[0]), 4);
		else if (token == "scale") getFloats(F, &(gMeshInstances.back()->instanceTransform.scale[0]), 3);
		else if (token == "material") {
			string materialName;
			getToken(F, materialName, ONE_TOKENS);
			if (gMaterials.count(materialName) > 0)	gMeshInstances.back()->setMaterial(gMaterials[materialName]);
			else ERROR("Unable to locate gMaterials[" + materialName + "], is the name right in .scene?", false);
		}
		else if (token == "mesh") {
			string meshName;
			getToken(F, meshName, ONE_TOKENS);
			if (gMeshes.count(meshName) > 0) gMeshInstances.back()->setMesh(gMeshes[meshName]);
			else ERROR("Unable to locate gMeshes[" + meshName + "], is the name right in .scene?", false);
		}
	}
}

void loadLight(FILE *F) 
{	
	string token;
	
	if (gLights.size() == gMaxLights) ERROR("Too many lights in scene.");

	gLights.push_back(new Light());

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "type") {
			string lightType;
			getToken(F, lightType, ONE_TOKENS);
			if (lightType == "point") gLights.back()->type = Light::LIGHT_TYPE::POINT;
			else if (lightType == "directional") gLights.back()->type = Light::LIGHT_TYPE::DIRECTIONAL;
			else if (lightType == "spot") gLights.back()->type = Light::LIGHT_TYPE::SPOT_LIGHT;
		}
		else if (token == "isOn") getInts(F, &(gLights.back()->isOn), 1);
		else if (token == "alpha") getFloats(F, &(gLights.back()->alpha), 1);
		else if (token == "theta") getFloats(F, &(gLights.back()->theta), 1);
		else if (token == "intensity") getFloats(F, &(gLights.back()->intensity[0]), 3);
		else if (token == "position") getFloats(F, &(gLights.back()->position[0]), 3);
		else if (token == "direction") getFloats(F, &(gLights.back()->direction[0]), 3);
		else if (token == "attenuation") getFloats(F, &(gLights.back()->attenuation[0]), 3);
	}
}

void loadCamera(FILE *F)
{
	string token;

	gCameras.push_back(new Camera());

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "eye") getFloats(F, &(gCameras.back()->eye[0]), 3);
		else if (token == "center") getFloats(F, &(gCameras.back()->center[0]), 3);
		else if (token == "vup") getFloats(F, &(gCameras.back()->vup[0]), 3);
		else if (token == "znear") getFloats(F, &(gCameras.back()->znear), 1);
		else if (token == "zfar") getFloats(F, &(gCameras.back()->zfar), 1);
		else if (token == "fovy") getFloats(F, &(gCameras.back()->fovy), 1);
	}

	gCameras.back()->refreshTransform((float)gWidth, (float)gHeight);
}

void loadScene(const char *sceneFile)
{
	//Unload the previous scene if there was one.
	if (!gMeshes.empty()) gMeshes.clear();
	if (!gMeshInstances.empty()) gMeshInstances.clear();
	if (!gCameras.empty()) gCameras.clear();
	if (!gMaterials.empty()) gMaterials.clear();
	if (!gLights.empty()) gLights.clear(); 
	if (gLightsUBO != NULL_HANDLE) glDeleteBuffers(1, &gLightsUBO);
	gLightsUBO = NULL_HANDLE;

	//Add the path used for the scene to the EngineUtil's PATH variable.
	string sceneFileName = sceneFile;
	int separatorIndex = sceneFileName.find_last_of("/");
	if (separatorIndex < 0)	separatorIndex = sceneFileName.find_last_of("\\");
	if (separatorIndex > 0)	addToPath(sceneFileName.substr(0, separatorIndex + 1));

	FILE *F = openFileForReading(sceneFile);
	string token;

	while (getToken(F, token, ONE_TOKENS)) {
		//cout << token << endl;
		if (token == "worldSettings") loadWorldSettings(F);
		else if (token == "mesh") loadMesh(F);
		else if (token == "material") loadMaterial(F);
		else if (token == "meshInstance") loadMeshInstance(F);
		else if (token == "camera") loadCamera(F);
		else if (token == "light") loadLight(F);
	}
}

//-------------------------------------------------------------------------//
// Update
//-------------------------------------------------------------------------//

void update(void)
{	
	gCameras[gActiveCamera]->refreshTransform((float)gWidth, (float)gHeight);

	if (gShouldSwapScene) {
		gShouldSwapScene = false;
		if (music) music->drop();
		soundEngine->stopAllSounds();
		glfwTerminate();
		loadScene(gSceneFileNames[gActiveScene]);
	}

	///*
	//// move mesh instance
	//gMeshInstance.translation[0] += 0.003f;
	//if (gMeshInstance.translation[0] >= 1.0f) gMeshInstance.translation[0] = -1.0f;
 //   */
	//
	//// scale mesh instance
	//static float dScale = 0.0005f;
	//float scale = gMeshInstance.T.scale[0];
	//scale += dScale;
	//if (scale > 1.25f) dScale = -0.0005f;
	//if (scale < 0.25f) dScale = 0.0005f;
	//gMeshInstance.setScale(glm::vec3(scale));

	//// rotate mesh
	//glm::quat r = glm::quat(glm::vec3(0.0f, 0.0051f, 0.00f));
	//gMeshInstance.T.rotation *= r;
	//
	//gMeshInstance.diffuseColor += glm::vec4(0.0013f, 0.000921f, 0.00119f, 0.0f);
	//if (gMeshInstance.diffuseColor[0] > 1.0f) gMeshInstance.diffuseColor[0] = 0.25f;
	//if (gMeshInstance.diffuseColor[1] > 1.0f) gMeshInstance.diffuseColor[1] = 0.25f;
	//if (gMeshInstance.diffuseColor[2] > 1.0f) gMeshInstance.diffuseColor[2] = 0.25f;


}

//-------------------------------------------------------------------------//
// Draw a frame
//-------------------------------------------------------------------------//

void render(void)
{
	// clear color and depth buffer
	glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	// draw scene
	for (int i = 0; i < (int)gMeshInstances.size(); ++i) gMeshInstances[i]->draw(*gCameras[gActiveCamera]);
}

//-------------------------------------------------------------------------//
// Main method
//-------------------------------------------------------------------------//

int main(int numArgs, char **args)
{
	// check usage
	if (numArgs < 2) {
		cout << "Proper Input: gameEngine.exe sceneFile.scene [sceneFile2.scene ...]" << endl;
		exit(0);
	}

	// Start sound engine
	soundEngine = createIrrKlangDevice();
	if (!soundEngine) return 0;
	soundEngine->setListenerPosition(vec3df(0, 0, 0), vec3df(0, 0, 1));
	soundEngine->setSoundVolume(0.25f); // master volume control

	// Play 3D sound
	//string soundFileName;
	//ISound* music = soundEngine->play3D(soundFileName.c_str(), vec3df(0, 0, 10), true); // position and looping
	//if (music) music->setMinDistance(5.0f); // distance of full volume

	// Load all curernt args into gSceneFileNames to swap about later. i=1 for start because args[0] is just the program name.
	for (int i = 1; i < numArgs; ++i) gSceneFileNames.push_back(args[i]);

	// Load first scene.
	loadScene(gSceneFileNames[0]);

	// start time (used to time framerate)
	double startTime = TIME();
    
	// render loop
	while (true) {
		// update and render
		update();
		render();
        
		// handle input
		glfwPollEvents();
		//if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;
		if (glfwWindowShouldClose(gWindow) != 0) break;

		double xx, yy;
		glfwGetCursorPos(gWindow, &xx, &yy);
		printf("%1.3f %1.3f ", xx, yy);
        
		// print framerate
		double endTime = TIME();
		printf("\rFPS: %1.0f  ", 1.0/(endTime-startTime));
		startTime = endTime;
        
		// swap buffers
		//SLEEP(1); // sleep 1 millisecond to avoid busy waiting
		glfwSwapBuffers(gWindow);
	}

	// Shut down sound engine
	if (music) music->drop(); // release music stream.
	soundEngine->drop(); // delete engine
    
	// Close OpenGL window and terminate GLFW
	glfwTerminate();
	for (auto it = gCameras.begin(); it != gCameras.end(); ++it) delete *it;
	for (auto it = gLights.begin(); it != gLights.end(); ++it) delete *it;
	for (auto it = gMeshInstances.begin(); it != gMeshInstances.end(); ++it) delete *it;
	for (auto it = gMaterials.begin(); it != gMaterials.end(); ++it) delete (*it).second;
	for (auto it = gMeshes.begin(); it != gMeshes.end(); ++it) delete (*it).second;
	return 0;
}

//-------------------------------------------------------------------------//


