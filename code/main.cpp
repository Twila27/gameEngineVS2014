
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

const double FIXED_DT = 1 / 60.0;
float gDeltaTimeStep = 0.0f;

ISoundEngine* soundEngine = NULL;
ISound* music = NULL;

//Reason not using Scene is to preserve hot-updating scene files.
map<string, TriMesh*> gMeshes;
map<string, Material*> gMaterials;
//vector<TriMeshInstance*> gMeshInstances;
map<string, SceneGraphNode*> gNodes;
map<string, Script*> gScripts;
vector<Camera*> gCameras;
vector<char*> gSceneFileNames;

//These will not change until their keys are pressed.
unsigned int gActiveCamera = 0;
unsigned int gActiveScene = 0;
bool gShouldSwapScene = false;
bool gShowPerFrameDebug = false;

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
			else gCameras[gActiveCamera]->translateLocal(glm::vec3(-step, 0, 0));
			break;
		case GLFW_KEY_RIGHT:
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) gActiveCamera = (gActiveCamera == gCameras.size() - 1) ? 0 : gActiveCamera + 1;
			else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
				gActiveScene = (gActiveScene == gSceneFileNames.size() - 1) ? 0 : gActiveScene + 1;
				gShouldSwapScene = true;
			}
			else gCameras[gActiveCamera]->translateLocal(glm::vec3(step, 0, 0));
			break;
		case GLFW_KEY_LEFT_ALT:
			soundEngine->setAllSoundsPaused(true);
			break;
		case GLFW_KEY_RIGHT_ALT:
			soundEngine->setAllSoundsPaused(false);
			break;
		case GLFW_KEY_SPACE:
			soundEngine->play3D("bell.wav", irrklang::vec3df(gCameras[gActiveCamera]->center.x, gCameras[gActiveCamera]->center.y, gCameras[gActiveCamera]->center.z));
			break;
		case GLFW_KEY_F5:
			gShouldSwapScene = true;
			break;
		case GLFW_KEY_F1:
			gShowPerFrameDebug = true;
			break;
		}
	}

	//Handle continuous movements.
	//if (action == GLFW_REPEAT) {}

	if (action == GLFW_PRESS &&
		((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))) {
		printf("\n%c\n", (char)key);
	}
}
#define tAmt 0.025f
#define rAmt 0.01f
void keyboardCameraController(Camera &cam) {
	if (glfwGetKey(gWindow, 'A')) cam.translateLocal(glm::vec3(-tAmt, 0, 0));
	if (glfwGetKey(gWindow, 'D')) cam.translateLocal(glm::vec3(tAmt, 0, 0));
	if (glfwGetKey(gWindow, 'W')) cam.translateLocal(glm::vec3(0, 0, -tAmt));
	if (glfwGetKey(gWindow, 'S')) cam.translateLocal(glm::vec3(0, 0, tAmt));
	if (glfwGetKey(gWindow, 'Q')) cam.translateLocal(glm::vec3(0, -tAmt, 0));
	if (glfwGetKey(gWindow, 'E')) cam.translateLocal(glm::vec3(0, tAmt, 0));
	if (glfwGetKey(gWindow, GLFW_KEY_LEFT)) cam.rotateGlobal(glm::vec3(0, 1, 0), rAmt);
	if (glfwGetKey(gWindow, GLFW_KEY_RIGHT)) cam.rotateGlobal(glm::vec3(0, 1, 0), -rAmt);
	if (glfwGetKey(gWindow, GLFW_KEY_UP)) cam.rotateLocal(glm::vec3(1, 0, 0), rAmt);
	if (glfwGetKey(gWindow, GLFW_KEY_DOWN)) cam.rotateLocal(glm::vec3(1, 0, 0), -rAmt);
	cam.refreshTransform((float)gWidth, (float)gHeight);
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
	}

	// Initialize the window with OpenGL context
	gWindow = createOpenGLWindow(gWidth, gHeight, gWindowTitle.c_str(), gSPP);
	glfwSetKeyCallback(gWindow, keyCallback);

	// Prepare the lights.
	initLightBuffer();

	// Load the mesh for billboards and sprites into the gMeshes map.
	gMeshes["flatCard"] = new TriMesh();
	gMeshes["flatCard"]->setName("flatCard");
	gMeshes["flatCard"]->readFromPly("flatCard.ply", false);
	gMeshes["flatCard"]->sendToOpenGL();

	// Load the materials for billboards and sprites into the gMaterials map.
	// There's one for sprite which won't move, one for billboard rotating yOnly, and another for allAxes rotation.
	// Note these rely heavily on the Material ctor's settings for its color uniforms.
	gMaterials["sprite"] = new Material();
	gMaterials["sprite"]->name = "sprite";
	gMaterials["sprite"]->setShaderProgram(createShaderProgram(loadShader("basicVertexShader.vs", GL_VERTEX_SHADER), loadShader("phongShadingSprite.fs", GL_FRAGMENT_SHADER)));
	//Lines to set up texture needed.
	gMaterials["sprite"]->getAndInitUniforms();
	gMaterials["yOnly"] = new Material();
	gMaterials["yOnly"]->name = "yOnly";
	gMaterials["yOnly"]->setShaderProgram(createShaderProgram(loadShader("basicVertexShader.vs", GL_VERTEX_SHADER), loadShader("phongShading.fs", GL_FRAGMENT_SHADER)));
	gMaterials["yOnly"]->getAndInitUniforms();
	gMaterials["allAxes"] = new Material();
	gMaterials["allAxes"]->name = "allAxes";
	gMaterials["allAxes"]->setShaderProgram(createShaderProgram(loadShader("basicVertexShader.vs", GL_VERTEX_SHADER), loadShader("phongShading.fs", GL_FRAGMENT_SHADER)));
	gMaterials["allAxes"]->getAndInitUniforms();

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
		else if (token == "texture") {
			m->textures.push_back(new RGBAImage());
			getToken(F, m->textures.back()->name, ONE_TOKENS); //Store uniform name in RGBAImage.
			string texFileName;	getToken(F, texFileName, ONE_TOKENS);
			m->textures.back()->loadPNG(texFileName);
			m->textures.back()->sendToOpenGL();
		}
	}

	m->setShaderProgram(createShaderProgram(vertexShader, fragmentShader)); //Return to modify this to take a container of shaders.
	if (materialName != "") gMaterials[materialName] = m;
	m->name = materialName;
	m->getAndInitUniforms(); //Very important line!
}

Drawable* loadAndReturnMeshInstance(FILE *F)
{
	string token;
	GLuint vertexShader = NULL_HANDLE;
	GLuint fragmentShader = NULL_HANDLE;
	GLuint shaderProgram = NULL_HANDLE;

	TriMeshInstance *instance = new TriMeshInstance();

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") {
			break;
		}
		else if (token == "material") {
			string materialName;
			getToken(F, materialName, ONE_TOKENS);
			if (gMaterials.count(materialName) > 0)	instance->setMaterial(gMaterials[materialName]);
			else ERROR("Unable to locate gMaterials[" + materialName + "], is the name right in .scene?", false);
		}
		else if (token == "mesh") {
			string meshName;
			getToken(F, meshName, ONE_TOKENS);
			if (gMeshes.count(meshName) > 0) instance->setMesh(gMeshes[meshName]);
			else ERROR("Unable to locate gMeshes[" + meshName + "], is the name right in .scene?", false);
		}
	}
	
	return instance;
}

Drawable* loadAndReturnSprite(FILE *F) 
{
	string token;

	Sprite *sprite = new Sprite();

	//Assign sprite material since there's only ever one for it to be. Else uncomment below code.
	if (gMaterials.count("sprite") > 0) sprite->setMaterial(gMaterials["sprite"]);
	else ERROR("Unable to locate gMeshes[\"sprite\"], verify its addition in loadWorldSettings()?", false);

	//Assign flat card mesh.
	if (gMeshes.count("flatCard") > 0) sprite->setMesh(gMeshes["flatCard"]);
	else ERROR("Unable to locate gMeshes[\"flatCard\"], verify its addition in loadWorldSettings()?", false);

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		//else if (token == "material") {
		//	string materialName;
		//	getToken(F, materialName, ONE_TOKENS);
		//	if (gMaterials.count(materialName) > 0)	sprite->setMaterial(gMaterials[materialName]);
		//	else ERROR("Unable to locate gMaterials[" + materialName + "], is the name right in .scene?", false);
		//}
		else if (token == "image") {
			Material* m = sprite->getMaterial();
			m->textures.push_back(new RGBAImage());
			getToken(F, m->textures.back()->name, ONE_TOKENS); //Store uniform name in RGBAImage.
			string texFileName;	getToken(F, texFileName, ONE_TOKENS);
			m->textures.back()->loadPNG(texFileName);
			m->textures.back()->sendToOpenGL();
			m->getAndInitUniforms();
		}
		else if (token == "animDir") getInts(F, &sprite->animDir, 1);
		else if (token == "animRate") getFloats(F, &sprite->animRate, 1);
		else if (token == "frameWidth") getInts(F, &sprite->frameWidth, 1);
		else if (token == "frameHeight") getInts(F, &sprite->frameHeight, 1);
	}

	if (sprite->getMaterial()->textures.size() < 1) ERROR("Sprite needs an image uSheetName \"img.png\"!");
	sprite->sheetWidth = sprite->getMaterial()->textures[0]->width;
	sprite->sheetHeight = sprite->getMaterial()->textures[0]->height;
	sprite->amtRows = sprite->sheetHeight / sprite->frameHeight;
	sprite->amtCols = sprite->sheetWidth / sprite->frameWidth;

	//Sheet properties can be accessed via textures[0]; frames assigned as below:
	for (int r = 0; r < sprite->amtRows; ++r)
		for (int c = 0; c < sprite->amtCols; ++c)
			sprite->frames.push_back({
				round((c*sprite->sheetWidth) / sprite->amtCols) / (float)sprite->sheetWidth, //u1
				round((r*sprite->sheetHeight) / sprite->amtRows) / (float)sprite->sheetHeight, //v1
				sprite->frameWidth / (float)sprite->sheetWidth, //u2 = currently the normalized location of the top-right, needs to be a normalized absolute width amount
				sprite->frameHeight / (float)sprite->sheetHeight //v2
			});

	return sprite;
}

Drawable* loadAndReturnBillboard(FILE *F)
{
	string token;

	Billboard *billboard = new Billboard();

	//Assign flat card mesh.
	if (gMeshes.count("flatCard") > 0) billboard->setMesh(gMeshes["flatCard"]);
	else ERROR("Unable to locate gMeshes[\"flatCard\"], verify its addition in loadWorldSettings()?", false);

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "material") {
			string materialName;
			getToken(F, materialName, ONE_TOKENS);
			if (gMaterials.count(materialName) > 0)	billboard->setMaterial(gMaterials[materialName]);
			else ERROR("Unable to locate gMaterials[" + materialName + "], is the name right in .scene?", false);
		}
		else if (token == "image") {
			Material* m = billboard->getMaterial();
			m->textures.push_back(new RGBAImage());
			getToken(F, m->textures.back()->name, ONE_TOKENS); //Store uniform name in RGBAImage.
			string texFileName;	getToken(F, texFileName, ONE_TOKENS);
			m->textures.back()->loadPNG(texFileName);
			m->textures.back()->sendToOpenGL();
			m->getAndInitUniforms();
		}
	}

	return billboard;
}

void loadLight(FILE *F)
{
	string token;

	if (gNumLights + 1 > MAX_LIGHTS) ERROR("Too many lights in scene.");
	gLights[gNumLights] = Light();
	gLights[gNumLights].isOn = 1;
	gLights[gNumLights].alpha = gLights[gNumLights].theta = 0;

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "type") {
			string lightType;
			getToken(F, lightType, ONE_TOKENS);
			if (lightType == "point") gLights[gNumLights].type = Light::LIGHT_TYPE::POINT;
			else if (lightType == "directional") gLights[gNumLights].type = Light::LIGHT_TYPE::DIRECTIONAL;
			else if (lightType == "spot") gLights[gNumLights].type = Light::LIGHT_TYPE::SPOT_LIGHT;
		}
		else if (token == "isOn") getInts(F, &(gLights[gNumLights].isOn), 1);
		else if (token == "alpha") getFloats(F, &(gLights[gNumLights].alpha), 1);
		else if (token == "theta") getFloats(F, &(gLights[gNumLights].theta), 1);
		else if (token == "intensity") getFloats(F, &(gLights[gNumLights].intensity[0]), 3);
		else if (token == "position") getFloats(F, &(gLights[gNumLights].position[0]), 3);
		else if (token == "direction") getFloats(F, &(gLights[gNumLights].direction[0]), 3);
		else if (token == "attenuation") getFloats(F, &(gLights[gNumLights].attenuation[0]), 3);
	}

	++gNumLights;
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

SceneGraphNode* loadAndReturnNode(FILE *F) 
{
	SceneGraphNode *n = new SceneGraphNode();
	string token, nodeName("");
	float renderThreshold = -1.0f;

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "name")
		{
			getToken(F, nodeName, ONE_TOKENS);
			if (nodeName == "") ERROR("Scene file does not name node!");
			gNodes[nodeName] = n;
			n->name = nodeName;
		}
		else if (token == "meshInstance") {
			n->LODstack.push_back(loadAndReturnMeshInstance(F));
			n->LODstack.back()->type = Drawable::TRIMESHINSTANCE;
		}
		else if (token == "sprite") {
			n->LODstack.push_back(loadAndReturnSprite(F));
			n->LODstack.back()->type = Drawable::SPRITE;
		}
		else if (token == "billboard") {
			n->LODstack.push_back(loadAndReturnBillboard(F));
			n->LODstack.back()->type = Drawable::BILLBOARD;
		}
		else if (token == "maxRenderDist") getFloats(F, &renderThreshold, 1);
		else if (token == "translation") getFloats(F, &(n->T.translation[0]), 3);
		else if (token == "rotation") getFloats(F, &(n->T.rotation[0]), 4);
		else if (token == "scale") getFloats(F, &(n->T.scale[0]), 3);
		else if (token == "node") {
			n->children.push_back(loadAndReturnNode(F));
			n->children.back()->parent = n;
		}
		else if (token == "script") {
			string scriptName;
			getToken(F, scriptName, ONE_TOKENS);
			if (gNodes.count(scriptName) > 0) n->scripts.push_back(gScripts[scriptName]);
			else ERROR("Unable to locate gScripts[" + scriptName + "], is the name right in .scene?", false);
		}
	}

	//Auto-generate the other class members that the parser isn't supplying.

	//First, the switching distances. 
		//Given n items on the LODstack, we consider [0, n->renderThreshold].
		//We subdivide this interval by the amount of objects in the LOD stack.
	if (renderThreshold == -1.0f) {
		ERROR("Need to specify maxRenderDist in node{}!", false);
		renderThreshold = 10; //Just a default, but really should specify, so I'm leaving in the warning.
	}
	for (float div = 1.0f; div <= (int)n->LODstack.size(); ++div)
		n->switchingDistances.push_back(renderThreshold / div); //Note this implies descending order! But makes switchingDistances[0] our easy-access for a render cutoff.
		//For now, the subdivision is binary, but it could gradually skew to one side of the interval too!
		//The node isn't rendered when the distance to the camera center is past its threshold.

	//Second, 

	return n;
}

void loadScene(const char *sceneFile)
{
	//Unload the previous scene if there was one.
	if (!gMeshes.empty()) gMeshes.clear();
	if (!gNodes.empty()) gNodes.clear();
	if (!gCameras.empty()) gCameras.clear();
	if (!gMaterials.empty()) gMaterials.clear();
	for (int i = 0; i < gNumLights; ++i) {
		gLights[i].isOn = 0;
		gLights[i].alpha = gLights[i].theta = 0.0f;
		gLights[i].attenuation = gLights[i].direction = gLights[i].intensity = gLights[i].position = glm::vec4(0);
	}
	if (gLightsUBO != NULL_HANDLE) glDeleteBuffers(1, &gLightsUBO);
	gNumLights = 0;
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
		else if (token == "node") loadAndReturnNode(F);
		else if (token == "mesh") loadMesh(F);
		else if (token == "material") loadMaterial(F);
		else if (token == "meshInstance") loadAndReturnMeshInstance(F);
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
		cout << "\n================================================================================\n";
		loadScene(gSceneFileNames[gActiveScene]);
	}

	///*
	//// move mesh instances
	//for (int i = 0; i < (int)gMeshInstances.size(); ++i) {
	//	gMeshInstances[i]->setTranslation(glm::vec3(gMeshInstances[i]->instanceTransform.translation.x + 0.003f, gMeshInstances[i]->instanceTransform.translation.y, gMeshInstances[i]->instanceTransform.translation.z));
	//	if (gMeshInstances[i]->instanceTransform.translation.x >= 1.0f) gMeshInstances[i]->setTranslation(glm::vec3(-1, gMeshInstances[i]->instanceTransform.translation.y, gMeshInstances[i]->instanceTransform.translation.z));
	//}
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

	for (auto it = gNodes.cbegin(); it != gNodes.cend(); ++it) it->second->update(*gCameras[gActiveCamera]);

	if (glfwGetKey(gWindow, GLFW_KEY_R)) {
		for (auto it = gNodes.begin(); it != gNodes.end(); ++it)
		if (it->second->name.find("oNode") != string::npos) //For all nodes with oNode in the name, slow because strings, but just for funsies and to test all parent-child transforms.
			it->second->T.rotation.y += rAmt;
	}
	if (glfwGetKey(gWindow, GLFW_KEY_F)) {
		for (auto it = gNodes.begin(); it != gNodes.end(); ++it)
		if (it->second->name.find("oNode") != string::npos) //For all nodes with oNode in the name, slow because strings, but just for funsies and to test all parent-child transforms.
			it->second->T.rotation.y -= rAmt;
	}
	if (glfwGetKey(gWindow, GLFW_KEY_T)) {
		for (auto it = gNodes.begin(); it != gNodes.end(); ++it)
		if (it->second->children.size() > 0) //For all nodes with oNode in the name, slow because strings, but just for funsies and to test all parent-child transforms.
			it->second->T.translation.y += rAmt;
	}
	if (glfwGetKey(gWindow, GLFW_KEY_G)) {
		for (auto it = gNodes.begin(); it != gNodes.end(); ++it)
		if (it->second->children.size() > 0) //For all nodes with oNode in the name, slow because strings, but just for funsies and to test all parent-child transforms.
			it->second->T.translation.y -= rAmt;
	}

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

	//Update lights.
	glBindBuffer(GL_UNIFORM_BUFFER, gLightsUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Light)*gNumLights, gLights); //Copy data into buffer w/o glBufferData()'s allocation.
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// draw scene
	for (auto it = gNodes.cbegin(); it != gNodes.cend(); ++it) it->second->draw(*gCameras[gActiveCamera]);
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
	double endTime = 0;
	gDeltaTimeStep = 0.0f;
	double frameTime = 0.0;
	double dt = 1 / 60.0;
	double gFPS = 0.0;

	// render loop
	while (true) {
		//handle time
		// update wrapped in the GafferOnPhysics semi-fixed dt loop trick.
		while (frameTime > 0.0) {
			double dtTmp = glm::min(frameTime, FIXED_DT);
			update();
			frameTime -= dtTmp;

		}	
		render();

		// handle input
		glfwPollEvents();
		keyboardCameraController(*gCameras[gActiveCamera]);
		if (glfwWindowShouldClose(gWindow) != 0) break;

		if (gShowPerFrameDebug) {
			//Print mouse pos.
			double xx, yy;
			glfwGetCursorPos(gWindow, &xx, &yy);
			printf("%1.3f %1.3f ", xx, yy);

			//Print framerate.
			printf("\rFPS: %1.0f  ", gDeltaTimeStep);
		}
		//Update framerate.
		endTime = TIME();
		/*gFPS = */gDeltaTimeStep = 1.0 / (endTime - startTime); //FPS.
		frameTime = endTime - startTime;

		startTime = endTime;

		// swap buffers
		//SLEEP(1); // sleep 1 millisecond to avoid busy waiting
		glfwSwapBuffers(gWindow);
	}

	// Shut down sound engine
	if (music) music->drop(); // release music stream.
	soundEngine->drop(); // delete engine

	// Close OpenGL window and terminate GLFW
	for (auto it = gCameras.begin(); it != gCameras.end(); ++it) delete *it;
	//for (auto it = gLights.begin(); it != gLights.end(); ++it) delete *it; //Because array.
	for (auto it = gNodes.begin(); it != gNodes.end(); ++it) delete it->second;
	for (auto it = gMaterials.begin(); it != gMaterials.end(); ++it) delete (*it).second;
	for (auto it = gMeshes.begin(); it != gMeshes.end(); ++it) delete (*it).second;
	glfwTerminate();

	return 0;
}

//-------------------------------------------------------------------------//


