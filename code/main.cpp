#include "SceneState.h"
#include "Scripts.h"

//Keyboard input and camera manipulation.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	//Handle discrete movements.
	//Ctrl changes cameras, Shift changes scenes.
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_M:
			cout << gBackgroundMusic->getSoundSource()->getName() << endl;
			break;
		case GLFW_KEY_LEFT_CONTROL:
			gActiveCamera = (gActiveCamera == 0) ? gCameras.size() - 1 : gActiveCamera - 1;
			break;
		case GLFW_KEY_RIGHT_CONTROL:
			gActiveCamera = (gActiveCamera == gCameras.size() - 1) ? 0 : gActiveCamera + 1;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			gActiveScene = (gActiveScene == 0) ? gSceneFileNames.size() - 1 : gActiveScene - 1;
			gShouldSwapScene = true;
			break;
		case GLFW_KEY_RIGHT_SHIFT:
			gActiveScene = (gActiveScene == gSceneFileNames.size() - 1) ? 0 : gActiveScene + 1;
			gShouldSwapScene = true;
			break;
		case GLFW_KEY_LEFT_ALT:
			soundEngine->setAllSoundsPaused(true);
			break;
		case GLFW_KEY_RIGHT_ALT:
			soundEngine->setAllSoundsPaused(false);
			break;
		case GLFW_KEY_SPACE:
			soundEngine->play3D("bell.wav", irrklang::vec3df(gCameras[gActiveCamera]->eye.x, gCameras[gActiveCamera]->eye.y, gCameras[gActiveCamera]->eye.z));
			break;
		case GLFW_KEY_F5:
			gShouldSwapScene = true;
			break;
		case GLFW_KEY_F1:
			gShowPerFrameDebug = !gShowPerFrameDebug;
			break;
		case GLFW_KEY_B:
			gBuildMode = !gBuildMode;
			break;
		case GLFW_KEY_GRAVE_ACCENT:
			if (gBuildMode) useConsole();
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
const double FIXED_DT = 0.01; //Time step for things like sprite tick.
float tAmt = 0.025f;
const float rAmt = 0.01f;
const float minSpeed = 0.025f;
void keyboardCameraController(Camera &cam) {
	if (glfwGetKey(gWindow, 'A')) cam.translateLocal(glm::vec3(-tAmt, 0, 0));
	if (glfwGetKey(gWindow, 'D')) cam.translateLocal(glm::vec3(tAmt, 0, 0));
	if (glfwGetKey(gWindow, 'W')) cam.translateLocal(glm::vec3(0, 0, -tAmt));
	if (glfwGetKey(gWindow, 'S')) cam.translateLocal(glm::vec3(0, 0, tAmt));
	if (glfwGetKey(gWindow, 'Q')) cam.translateLocal(glm::vec3(0, -tAmt, 0));
	if (glfwGetKey(gWindow, 'E')) cam.translateLocal(glm::vec3(0, tAmt, 0));
	if (glfwGetKey(gWindow, '=')) tAmt += minSpeed;
	if (glfwGetKey(gWindow, '-')) tAmt -= (tAmt - minSpeed <= minSpeed ? 0 : minSpeed);
	if (glfwGetKey(gWindow, GLFW_KEY_LEFT)) cam.rotateGlobal(glm::vec3(0, 1, 0), rAmt);
	if (glfwGetKey(gWindow, GLFW_KEY_RIGHT)) cam.rotateGlobal(glm::vec3(0, 1, 0), -rAmt);
	if (glfwGetKey(gWindow, GLFW_KEY_UP)) cam.rotateLocal(glm::vec3(1, 0, 0), rAmt);
	if (glfwGetKey(gWindow, GLFW_KEY_DOWN)) cam.rotateLocal(glm::vec3(1, 0, 0), -rAmt);
	cam.refreshTransform((float)gWidth, (float)gHeight);
}

//Parsing.
string ONE_TOKENS = "{}[]()<>+-*/,;";
void loadWorldSettings(FILE *F)
{
	string token, t;
	string fontFileName;
	int fontTexNumRows(-1), fontTexNumCols(-1);
	while (getToken(F, token, ONE_TOKENS)) {
		//cout << "  " << token << endl;
		if (token == "}") break;
		if (token == "windowTitle") getToken(F, gWindowTitle, ONE_TOKENS);
		else if (token == "width") getInts(F, &gWidth, 1);
		else if (token == "height") getInts(F, &gHeight, 1);
		else if (token == "spp") getInts(F, &gSPP, 1);
		else if (token == "debugFont") getToken(F, fontFileName, ONE_TOKENS);
		else if (token == "fontTexNumRows") getInts(F, &fontTexNumRows, 1);
		else if (token == "fontTexNumCols") getInts(F, &fontTexNumCols, 1);
		else if (token == "backgroundColor") getFloats(F, &backgroundColor[0], 3);
		else if (token == "backgroundMusic") {
			string fileName, fullFileName;
			getToken(F, fileName, ONE_TOKENS);
			getFullFileName(fileName, fullFileName);
			gBackgroundMusic = soundEngine->play2D(fullFileName.c_str(), true, false, true, irrklang::ESM_AUTO_DETECT, true);
			//Only returns ISound* if 'track', 'startPaused' or 'enableSoundEffects' are true.
		}
	}

	// Initialize the window with OpenGL context
	gWindow = createOpenGLWindow(gWidth, gHeight, gWindowTitle.c_str(), gSPP);
	glfwSetKeyCallback(gWindow, keyCallback);

	//if (fontTexNumRows != -1) initText2D(fontFileName.c_str(), fontTexNumRows, fontTexNumCols); //Loading font.

	// Prepare the lights.
	initLightBuffer();
}
void loadMesh(FILE *F, bool inLibrary = false)
{
	string token, meshName(""), fileName("");

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "name") getToken(F, meshName, ONE_TOKENS);
		else if (token == "file") getToken(F, fileName, ONE_TOKENS);
	}
	gMeshes[meshName] = new TriMesh();
	gMeshes[meshName]->setName(meshName);
	gMeshes[meshName]->filename = fileName;
	gMeshes[meshName]->inLibrary = inLibrary;
	gMeshes[meshName]->readFromPly(fileName, false);
	gMeshes[meshName]->sendToOpenGL();
}
void loadMaterial(FILE *F, bool inLibrary = false)
{
	string token, materialName("");
	GLuint vertexShader = NULL_HANDLE;
	GLuint fragmentShader = NULL_HANDLE;

	Material *m = new Material();

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "name") getToken(F, materialName, ONE_TOKENS);
		else if (token == "vertexShader") {
			getToken(F, m->vertexShaderName, ONE_TOKENS);
			vertexShader = loadShader(m->vertexShaderName.c_str(), GL_VERTEX_SHADER);
		}
		else if (token == "fragmentShader") {
			getToken(F, m->fragmentShaderName, ONE_TOKENS);
			fragmentShader = loadShader(m->fragmentShaderName.c_str(), GL_FRAGMENT_SHADER);
		}
		else if (token == "color") {
			m->colors.push_back(new NameIdVal<glm::vec4>());
			getToken(F, m->colors.back()->name, ONE_TOKENS); //Store uniform name in NameIdVal<>.
			getFloats(F, &m->colors.back()->val[0], 4);
		}
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
	m->inLibrary = inLibrary;
	m->bindMaterial(); //Very important line!
}
Drawable* loadAndReturnMeshInstance(FILE *F)
{
	string token;

	TriMeshInstance *instance = new TriMeshInstance();

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") {
			break;
		}
		else if (token == "material") {
			string materialName;
			getToken(F, materialName, ONE_TOKENS);
			if (gMaterials.count(materialName) > 0)	instance->setMaterial(gMaterials[materialName]);
			else ERROR("Unable to locate gMaterials[" + materialName + "], check scene and library files?", false);
		}
		else if (token == "mesh") {
			string meshName;
			getToken(F, meshName, ONE_TOKENS);
			if (gMeshes.count(meshName) > 0) instance->setMesh(gMeshes[meshName]);
			else ERROR("Unable to locate gMeshes[" + meshName + "], check scene and library files?", false);
		}
		else if (token == "image") {
			instance->diffuseTexture = new RGBAImage();
			instance->diffuseTexture->name = "uDiffuseTex";
			string texFileName;	getToken(F, texFileName, ONE_TOKENS);
			instance->diffuseTexture->loadPNG(texFileName);
			instance->diffuseTexture->sendToOpenGL();
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
	else ERROR("Unable to locate gMeshes[\"sprite\"], check scene and library files?", false);

	//Assign flat card mesh.
	if (gMeshes.count("flatCard") > 0) sprite->setMesh(gMeshes["flatCard"]);
	else ERROR("Unable to locate gMeshes[\"flatCard\"], check scene and library files?", false);

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		//else if (token == "material") {
		//	string materialName;
		//	getToken(F, materialName, ONE_TOKENS);
		//	if (gMaterials.count(materialName) > 0)	sprite->setMaterial(gMaterials[materialName]);
		//	else ERROR("Unable to locate gMaterials[" + materialName + "], check scene and library files?", false);
		//}
		else if (token == "image") {
			sprite->diffuseTexture = new RGBAImage();
			sprite->diffuseTexture->name = "uDiffuseTex";
			string texFileName;	getToken(F, texFileName, ONE_TOKENS);
			sprite->diffuseTexture->loadPNG(texFileName);
			sprite->diffuseTexture->sendToOpenGL();
		}
		else if (token == "animDir") getInts(F, &sprite->animDir, 1);
		else if (token == "animRate") getFloats(F, &sprite->animRate, 1);
		else if (token == "frameWidth") getInts(F, &sprite->frameWidth, 1);
		else if (token == "frameHeight") getInts(F, &sprite->frameHeight, 1);
	}

	if (sprite->diffuseTexture == nullptr) ERROR("Sprite needs an image uSheetName \"img.png\"!");
	sprite->sheetWidth = sprite->diffuseTexture->width;
	sprite->sheetHeight = sprite->diffuseTexture->height;
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
	else ERROR("Unable to locate gMeshes[\"flatCard\"], check scene and library files?", false);

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "material") {
			string materialName;
			getToken(F, materialName, ONE_TOKENS);
			if (gMaterials.count(materialName) > 0)	billboard->setMaterial(gMaterials[materialName]);
			else ERROR("Unable to locate gMaterials[" + materialName + "], check scene and library files?", false);
		}
		else if (token == "image") {
			billboard->diffuseTexture = new RGBAImage();
			billboard->diffuseTexture->name = "uDiffuseTex";
			string texFileName;	getToken(F, texFileName, ONE_TOKENS);
			billboard->diffuseTexture->loadPNG(texFileName);
			billboard->diffuseTexture->sendToOpenGL();
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
Camera* loadCamera(FILE *F)
{
	string token;

	gCameras.push_back(new Camera());

	while (getToken(F, token, ONE_TOKENS)) {
		if (token == "}") break;
		else if (token == "name") getToken(F, gCameras.back()->name, ONE_TOKENS);
		else if (token == "eye") getFloats(F, &(gCameras.back()->eye[0]), 3);
		else if (token == "center") getFloats(F, &(gCameras.back()->center[0]), 3);
		else if (token == "vup") getFloats(F, &(gCameras.back()->vup[0]), 3);
		else if (token == "znear") getFloats(F, &(gCameras.back()->znear), 1);
		else if (token == "zfar") getFloats(F, &(gCameras.back()->zfar), 1);
		else if (token == "fovy") getFloats(F, &(gCameras.back()->fovy), 1);
	}
	gCameras.back()->inNode = false;
	gCameras.back()->refreshTransform((float)gWidth, (float)gHeight);

	return gCameras.back();
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
		else if (token == "rotation") { 
			glm::vec3 R;
			getFloats(F, &R[0], 3);
			n->T.rotation = glm::quat(R);
		}
		else if (token == "scale") getFloats(F, &(n->T.scale[0]), 3);
		else if (token == "node") {
			n->children.push_back(loadAndReturnNode(F));
			n->children.back()->parent = n;
		}
		else if (token == "camera") {
			n->cameras.push_back(loadCamera(F));
			//gCameras.push_back(n->cameras.back()); //Already done in loadCamera().
			n->cameras.back()->inNode = true;
		}
		else if (token == "script") {
			string scriptName;
			getToken(F, scriptName, ONE_TOKENS);
			if (gScripts.count(scriptName) > 0) n->scripts.push_back(gScripts[scriptName]->clone(n));
			else ERROR("Unable to locate gScripts[" + scriptName + "], check scene and library files?", false);
		}
		else if (token == "sound") {
			string fileName, fullFileName;
			getToken(F, fileName, ONE_TOKENS);
			getFullFileName(fileName, fullFileName);
			n->sounds.push_back(soundEngine->play2D(fullFileName.c_str(), false, false, true));
			n->sounds.back()->stop();
			//Only returns ISound* if 'track', 'startPaused' or 'enableSoundEffects' are true.
		}
	}

	//Auto-generate the other class members that the parser isn't supplying.

	//First, the switching distances. 
		//Given n items on the LODstack, we consider [0, n->renderThreshold].
		//We subdivide this interval by the amount of objects in the LOD stack.
	if (renderThreshold == -1.0f) {
		ERROR("Need to specify maxRenderDist in node{}!", false);
		renderThreshold = 100; //Just a default, but really should specify, so I'm leaving in the warning.
	}
	for (float div = 1.0f; div <= (int)n->LODstack.size(); ++div)
		n->switchingDistances.push_back(renderThreshold / div); //Note this implies descending order! But makes switchingDistances[0] our easy-access for a render cutoff.
		//For now, the subdivision is binary, but it could gradually skew to one side of the interval too!
		//The node isn't rendered when the distance to the camera center is past its threshold.
	
	//Second, configure cameras to be oriented to the node.
	n->setTranslation(n->T.translation); //Also handles camera updates.

	return n;
}
void loadLibrary(const char *libFile) {
	//No unloading needed. Add path used to EngineUtil PATH variable.
	string lib = libFile;
	gLibraries.push_back(lib);
	int separatorIndex = lib.find_last_of("/");
	if (separatorIndex < 0)	separatorIndex = lib.find_last_of("\\");
	if (separatorIndex > 0)	addToPath(lib.substr(0, separatorIndex + 1));

	FILE *F = openFileForReading(libFile);
	string token;

	while (getToken(F, token, ONE_TOKENS)) {
		//cout << token << endl;
		if (token == "mesh") loadMesh(F, true);
		else if (token == "material") loadMaterial(F, true);
	}
	fclose(F);
}
void loadScene(const char *sceneFile)
{
	//Unload the previous scene if there was one.
	if (!gLibraries.empty()) gLibraries.clear();
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
	gActiveCamera = 0;

	//Add the path used for the scene to the EngineUtil's PATH variable.
	gActiveSceneName = sceneFile;
	int separatorIndex = gActiveSceneName.find_last_of("/");
	if (separatorIndex < 0)	separatorIndex = gActiveSceneName.find_last_of("\\");
	if (separatorIndex > 0)	addToPath(gActiveSceneName.substr(0, separatorIndex + 1));

	FILE *F = openFileForReading(sceneFile);
	string token;

	while (getToken(F, token, ONE_TOKENS)) {
		//cout << token << endl;
		if (token == "worldSettings") loadWorldSettings(F);
		else if (token == "library") { (getToken(F, token, ONE_TOKENS)); loadLibrary(token.c_str()); }
		else if (token == "node") loadAndReturnNode(F);
		else if (token == "mesh") loadMesh(F);
		else if (token == "material") loadMaterial(F);
		else if (token == "meshInstance") loadAndReturnMeshInstance(F);
		else if (token == "camera") loadCamera(F);
		else if (token == "light") loadLight(F);
	}
	fclose(F);
}

//Console and main loops.
void consoleLoadCamera()
{
	cout << "\tEye - Camera Position x: ";
	cin >> gCameras.back()->eye.x;
	cout << "\tEye - Camera Position y: ";
	cin >> gCameras.back()->eye.y;
	cout << "\tEye - Camera Position z: ";
	cin >> gCameras.back()->eye.z;

	cout << "\tCenter - Camera Direction x: ";
	cin >> gCameras.back()->center.x;
	cout << "\tCenter - Camera Direction y: ";
	cin >> gCameras.back()->center.y;
	cout << "\tCenter - Camera Direction z: ";
	cin >> gCameras.back()->center.z;

	cout << "\tVup - Camera Vertical Tilt Direction x: ";
	cin >> gCameras.back()->vup.x;
	cout << "\tVup - Camera Vertical Tilt Direction y: ";
	cin >> gCameras.back()->vup.y;
	cout << "\tVup - Camera Vertical Tilt Direction z: ";
	cin >> gCameras.back()->vup.z;

	cout << "\tFoVy - 0.5 Norm: ";
	cin >> gCameras.back()->fovy;

	cout << "\tzNear - 0.1 Norm: ";
	cin >> gCameras.back()->znear;

	cout << "\tzFar - 1000 Norm: ";
	cin >> gCameras.back()->zfar;

	gCameras.back()->inNode = false;
	cout << "\tCamera successfully initialized and added to gCameras.\n";
} //Relies on the camera having been just added, see in useConsole's create camera command code.
SceneGraphNode* consoleLoadNode(const string& name = "") { 
	string nodeName(name), tmp("");

	//0. Get node name if none exists, e.g. for child nodes set below.
	while (nodeName == "") { cout << "\tName of Node: "; cin >> nodeName; }
	gNodes[nodeName] = new SceneGraphNode();
	gNodes[nodeName]->name = nodeName;

	//1. Initializing node Transform.
	cout << "\tTranslation.x: "; cin >> gNodes[nodeName]->T.translation[0];
	cout << "\tTranslation.y: "; cin >> gNodes[nodeName]->T.translation[1];
	cout << "\tTranslation.z: "; cin >> gNodes[nodeName]->T.translation[2];

	float pitch, yaw, roll;
	cout << "\tRotation.x: "; cin >> pitch;
	cout << "\tRotation.y: "; cin >> yaw;
	cout << "\tRotation.z: "; cin >> roll;
	gNodes[nodeName]->T.rotation = glm::quat(glm::vec3(pitch, yaw, roll));

	cout << "\tScale.x: "; cin >> gNodes[nodeName]->T.scale[0];
	cout << "\tScale.y: "; cin >> gNodes[nodeName]->T.scale[1];
	cout << "\tScale.z: "; cin >> gNodes[nodeName]->T.scale[2];

	//2. Add cameras.
	do {
		cout << "\tAdd camera (Y/N)? "; cin >> tmp; if (tmp == "N" || tmp == "n") break;
		consoleLoadCamera();
		gCameras.back()->inNode = true;
		gNodes[nodeName]->cameras.push_back(gCameras.back());
	} while (true);

	//3. Add Drawables to LODstack.
	float renderThreshold = -1.0f;
	cout << "\tMaximum render distance - Norm 10.0: ";
	cin >> renderThreshold;
	do {
		cout << "\tAdd sprite, billboard, or mesh instance (Y/N)? "; cin >> tmp; if (tmp == "N" || tmp == "n") break;
		cout << "\tTip: first addition is seen when closest to node.\n";
		cout << "\tPlease type s, b, mi for which drawable to add: "; cin >> tmp;
		if (tmp == "s") {
			Sprite *sprite = new Sprite();

			//Assign sprite material since there's only ever one for it to be. Else uncomment below code.
			if (gMaterials.count("sprite") > 0) sprite->setMaterial(gMaterials["sprite"]);
			else ERROR("\tUnable to locate gMeshes[\"sprite\"], check scene and library files?", false);

			//Assign flat card mesh.
			if (gMeshes.count("flatCard") > 0) sprite->setMesh(gMeshes["flatCard"]);
			else ERROR("\tUnable to locate gMeshes[\"flatCard\"], check scene and library files?", false);

			//Assign image or sprite sheet.
			Material* m = sprite->getMaterial();
			m->textures.push_back(new RGBAImage());
			cout << "\tTip: uDiffuseTex is the image's sampler2D uniform name by default.\n";
			m->textures.back()->name = "uDiffuseTex"; //Store uniform name in RGBAImage.
			cout << "\tFilename, e.g. spritesheet.png (active paths besides cwd below): \n";
			for (auto it = getPATH().cbegin(); it != getPATH().cend(); ++it) cout << '\t' << '\t' << *it << endl;
			cin >> tmp;
			m->textures.back()->loadPNG(tmp);
			m->textures.back()->sendToOpenGL();
			m->bindMaterial();

			cout << "\tAnimation Direction (1 or -1) -- Norm 1: "; cin >> sprite->animDir;
			cout << "\tAnimation FPS Rate -- Norm 1: "; cin >> sprite->animRate;
			cout << "\tFrame Width in Pixels: "; cin >> sprite->frameWidth;
			cout << "\tFrame Height in Pixels: "; cin >> sprite->frameHeight;

			if (sprite->getMaterial()->textures.size() < 1) ERROR("Sprite lacks an image!");
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

			gNodes[nodeName]->LODstack.push_back(sprite);
			gNodes[nodeName]->LODstack.back()->type = Drawable::SPRITE;
		}
		else if (tmp == "b") {
			Billboard *billboard = new Billboard();

			//Assign flat card mesh.
			if (gMeshes.count("flatCard") > 0) billboard->setMesh(gMeshes["flatCard"]);
			else ERROR("\tUnable to locate gMeshes[\"flatCard\"], check scene and library files?", false);

			//Assign material.
			cout << "Does the billboard rotate only vertically on the y-axis (Y/N)? "; cin >> tmp;
			if (tmp == "N" || tmp == "n") {
				if (gMaterials.count("allAxes") > 0) billboard->setMaterial(gMaterials["allAxes"]);
				else ERROR("Unable to locate gMaterials[\"allAxes\"], check scene and library files?", false);
			}
			else {
				if (gMaterials.count("yOnly") > 0) billboard->setMaterial(gMaterials["yOnly"]);
				else ERROR("Unable to locate gMaterials[\"yOnly\"], check scene and library files?", false);
			}

			//Assign image.
			Material* m = billboard->getMaterial();
			m->textures.push_back(new RGBAImage());
			cout << "\tTip: Assigning uDiffuseTex as the image's corresponding sampler2D uniform name by default.\n";
			m->textures.back()->name = "uDiffuseTex"; //Store uniform name in RGBAImage.
			cout << "\tPlease enter the filename of the image.png located among the current paths below: \n";
			for (auto it = getPATH().cbegin(); it != getPATH().cend(); ++it) cout << '\t' << '\t' << *it << endl;
			cin >> tmp;
			m->textures.back()->loadPNG(tmp);
			m->textures.back()->sendToOpenGL();
			m->bindMaterial();

			gNodes[nodeName]->LODstack.push_back(billboard);
			gNodes[nodeName]->LODstack.back()->type = Drawable::BILLBOARD;
		}
		else if (tmp == "mi") {
			TriMeshInstance *instance = new TriMeshInstance();

			//Assign material.
			cout << "\tPlease choose a material from those below: \n";
			for (auto it = gMaterials.cbegin(); it != gMaterials.cend(); ++it) cout << '\t' << '\t' << it->second->name << endl;
			cin >> tmp;
			if (gMaterials.count(tmp) > 0)	instance->setMaterial(gMaterials[tmp]);
			else ERROR("\tUnable to locate gMaterials[" + tmp + "], check scene and library files?", false);

			//Assign mesh.
			cout << "\tPlease choose a mesh from those below: \n";
			for (auto it = gMeshes.cbegin(); it != gMeshes.cend(); ++it) cout << '\t' << '\t' << it->second->name << endl;
			cin >> tmp;
			if (gMeshes.count(tmp) > 0)	instance->setMesh(gMeshes[tmp]);
			else ERROR("\tUnable to locate gMeshes[" + tmp + "], check scene and library files?", false);

			gNodes[nodeName]->LODstack.push_back(instance);
			gNodes[nodeName]->LODstack.back()->type = Drawable::TRIMESHINSTANCE;
		}
		gNodes[nodeName]->cameras.push_back(gCameras.back());
	} while (true);

	//4. Add node(s) as child(ren).
	do {
		cout << "\tAdd child node (Y/N)? "; cin >> tmp; if (tmp == "N" || tmp == "n") break;
		gNodes[nodeName]->children.push_back(consoleLoadNode());
	} while (true);


	//5. Add script(s).
	do {
		cout << "\tAdd script (Y/N)? "; cin >> tmp; if (tmp == "N" || tmp == "n") break;
		cout << "\tAdding scripts is currently not supported.\n";
		//gNodes[nodeName]->scripts.push_back(scriptName?);
	} while (true);

	//6. Add sound(s).
	do {
		cout << "\tAdd sound (Y/N)? "; cin >> tmp; if (tmp == "N" || tmp == "n") break;
		cout << "\tPlease enter the name of the sound, e.g. bell.wav: "; cin >> tmp;
		gNodes[nodeName]->sounds.push_back(soundEngine->play2D(tmp.c_str(), false, false, true));
		gNodes[nodeName]->sounds.back()->stop();
	} while (true);

	//Auto-generate the other class members that the parser isn't supplying.
	//First, the switching distances. 
	//Given n items on the LODstack, we consider [0, n->renderThreshold].
	//We subdivide this interval by the amount of objects in the LOD stack.
	if (renderThreshold == -1.0f) {
		ERROR("Need to specify maxRenderDist in node{}!", false);
		renderThreshold = 100; //Just a default, but really should specify, so I'm leaving in the warning.
	}
	for (float div = 1.0f; div <= (int)gNodes[nodeName]->LODstack.size(); ++div)
		gNodes[nodeName]->switchingDistances.push_back(renderThreshold / div); //Note this implies descending order! But makes switchingDistances[0] our easy-access for a render cutoff.
	//For now, the subdivision is binary, but it could gradually skew to one side of the interval too!
	//The node isn't rendered when the distance to the camera center is past its threshold.

	//Second, configure cameras to be oriented to the node.
	gNodes[nodeName]->setTranslation(gNodes[nodeName]->T.translation); //Also handles camera updates.

	return gNodes[nodeName];
}
void update(double dt)
{
	gCameras[gActiveCamera]->refreshTransform((float)gWidth, (float)gHeight);

	if (gShouldSwapScene) {
		gShouldSwapScene = false;
		if (gBackgroundMusic) gBackgroundMusic->drop();
		soundEngine->stopAllSounds();
		glfwTerminate();
		cout << "\n================================================================================\n";
		loadScene(gSceneFileNames[gActiveScene].c_str());
	}

	for (auto it = gNodes.cbegin(); it != gNodes.cend(); ++it) it->second->update(*gCameras[gActiveCamera], dt);

	//Examples of how to do transforms, although rotation should be done with quats, e.g. glm::quat r = glm::quat(glm::vec3(0.0f, 0.0051f, 0.00f)); gMeshInstance.T.rotation *= r;
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
			//it->second->setTranslation(glm::vec3(0, rAmt, 0));
			it->second->addTranslation(glm::vec3(0, rAmt, 0));
	}
	if (glfwGetKey(gWindow, GLFW_KEY_G)) {
		for (auto it = gNodes.begin(); it != gNodes.end(); ++it)
		if (it->second->children.size() > 0) //For all nodes with oNode in the name, slow because strings, but just for funsies and to test all parent-child transforms.
			//it->second->setTranslation(glm::vec3(0, -rAmt, 0));
			it->second->addTranslation(glm::vec3(0, -rAmt, 0));
	}

	//Play the sound of an object within the specified number range below, if it isn't yet played.
	//Could even add in a tick within the node class to check whether a sound is ready or should delay playing, so it's not just effectively looping.
	for (auto it = gNodes.cbegin(); it != gNodes.cend(); ++it) {
		glm::vec3 camDistVec = it->second->T.translation - (*gCameras[gActiveCamera]).eye;
		if (it->second->sounds.size() > 0
			&& !soundEngine->isCurrentlyPlaying(it->second->sounds.back()->getSoundSource())
			&& camDistVec.x*camDistVec.x + camDistVec.y*camDistVec.y + camDistVec.z*camDistVec.z <= 10.0)
			soundEngine->play3D(it->second->sounds.back()->getSoundSource(), irrklang::vec3df(it->second->T.translation.x, it->second->T.translation.y, it->second->T.translation.z), false, false, false, false); //Outside all thresholds.
	}
}
void render(void)
{
	// clear color and depth buffer
	if (gBuildMode) glClearColor(0, 0, 0, 1.0f);
	else glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (gShowPerFrameDebug) {
		//printText2D("This is a test", 0, 0, 32);
	}

	//Update lights.
	glBindBuffer(GL_UNIFORM_BUFFER, gLightsUBO); //If we get an exception being thrown here, double-check the scene file contents, may be overwritten/corrupted.
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Light)*gNumLights, gLights); //Copy data into buffer w/o glBufferData()'s allocation.
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// draw scene
	for (auto it = gNodes.cbegin(); it != gNodes.cend(); ++it) it->second->draw(*gCameras[gActiveCamera]);
}
int main(int numArgs, char **args)
{
	// check usage
	if (numArgs < 2) {
		cout << "Proper Input: gameEngine.exe sceneFile.scene [sceneFile2.scene ...]" << endl;
		exit(0);
	}

	if (args[1] == "-b") gBuildMode = true;

	// Start sound engine
	soundEngine = createIrrKlangDevice();
	if (!soundEngine) return 0;
	soundEngine->setListenerPosition(vec3df(0, 0, 0), vec3df(0, 0, 1));
	soundEngine->setSoundVolume(0.25f); // master volume control

	//Ready script pool.
	gScripts["moverScript"] = new MoverScript(nullptr);

	// Play 3D sound
	//string soundFileName;
	//ISound* music = soundEngine->play3D(soundFileName.c_str(), vec3df(0, 0, 10), true); // position and looping
	//if (music) music->setMinDistance(5.0f); // distance of full volume

	// Load all curernt args into gSceneFileNames to swap about later. i=1 for start because args[0] is just the program name.
	for (int i = (gBuildMode ? 2 : 1); i < numArgs; ++i) gSceneFileNames.push_back(args[i]);

	if (gSceneFileNames.size() == 0) ERROR("Failed to supply any scene file names, exiting engine.");

	// Load first scene.
	loadScene(gSceneFileNames[0].c_str());

	// start time (used to time framerate)
	double newTime = 0;
	double frameTime = 0.0;
	double runningTime = 0.0;
	double currTime = TIME();
	double accumulator = 0.0;
	double gFPS = 0.0;

	// game loop
	while (true) {
		//handle time
		newTime = TIME();
		frameTime = newTime - currTime;
		currTime = newTime;

		accumulator += frameTime;

		// update wrapped in the GafferOnPhysics final dt loop trick.
		while (accumulator >= FIXED_DT) {
			update(FIXED_DT);
			accumulator -= FIXED_DT;
			runningTime += FIXED_DT;
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
			printf("\rFPS: %1.0f  ", gFPS);
		}
		//Update framerate.
		gFPS = 1.0 / (newTime - currTime);
		
		// swap buffers
		//SLEEP(1); // sleep 1 millisecond to avoid busy waiting
		glfwSwapBuffers(gWindow);
	}

	// Shut down sound engine
	if (gBackgroundMusic) gBackgroundMusic->drop(); // release music stream.
	soundEngine->drop(); // delete engine

	// Close OpenGL window and terminate GLFW
	for (auto it = gCameras.begin(); it != gCameras.end(); ++it) delete *it;
	//for (auto it = gLights.begin(); it != gLights.end(); ++it) delete *it; //Because noptr array.
	for (auto it = gNodes.begin(); it != gNodes.end(); ++it) delete it->second;
	for (auto it = gMaterials.begin(); it != gMaterials.end(); ++it) delete (*it).second;
	for (auto it = gMeshes.begin(); it != gMeshes.end(); ++it) delete (*it).second;
	//cleanupText2D(); // Delete font VBO, shader, texture.
	glfwTerminate();

	return 0;
}
void useConsole(void)
{
	string input;
	bool flag = false; //For generic use below to avoid some code repetition's bloating.
	cout << "\n================================================================================";
	cout << "\t\t\t\tEntering Console";
	cout << "\n================================================================================\n";
	cout << "Commands: \n\tshh, noshh \n\tq, quit, exit \n\tb, build \n\tcreate \n\tload \n\tsave \n\tprint\n";
	do {
		flag = false;
		getline(cin, input);
		//cout << input << endl;

		//Single-token commands first.
		if (input == "b" || input == "build") gBuildMode = !gBuildMode;
		else if (input == "shh") soundEngine->setAllSoundsPaused(true);
		else if (input == "noshh") soundEngine->setAllSoundsPaused(false);
		else if (input == "q" || input == "quit" || input == "exit") break;
		else { //Multiple-token commands.
			istringstream iss(input); //HAVE to instantiate every iteration or it breaks.
			while (iss) {
				string token;
				iss >> token; //Same as iss.operator>>(token).
				if (token == "load")
				{
					iss >> token;
					if (token == "-a") //Append to the gSceneFileNames container.
					{
						cout << "\tAppending to gSceneFileNames, updating gActiveScene...\n";
						flag = true; //Not pushing it back here because it may be a bad name.
						iss >> token; //Catch token up to the non -a case.
					}
					if (token == "load" || token == "-a")
					{
						cout << "\tValid Commands:\n";
						cout << "\tload [-a] nameOfFileToLoad.scene\n";
						break;
					}
					FILE * f = openFileForReading(token);
					if (f == nullptr) cout << "\tFile not found. Please ensure the name is correct and try again.\n";
					else //Valid file on first try, so load it.
					{
						glfwTerminate();
						if (flag) {
							gSceneFileNames.push_back(token);
							gActiveScene = gSceneFileNames.size() - 1; //Else gActiveScene stays on the scene before loading.
						}
						loadScene(token.c_str());
						return;
					}
					break;
				}
				else if (token == "save")
				{
					iss >> token;
					if (token == "-q")
					{
						cout << "\tWill quit console after save completes.\n";
						flag = true;
						iss >> token; //Catch token up to the non -q case.
					}
					if (token == "save" || token == "-q")
					{
						cout << "\tValid Commands:\n";
						cout << "\tsave [-q] fileNameToSaveTo.scene\n";
						cout << "\tReplace fileNameToSaveToScene.scene by \'this\' to save active scene.\n";
						break;
					}
					else if (token == "this") token = gActiveSceneName;
					//Call toString() of all appropriate objects over all global state containers.
					//fprintf(F, "\n%s\n", objOfInterestInRightOrder.toSDL());
					FILE *F = fopen(token.c_str(), "w"); //Will erase old file and create a new one over it.
					saveWorldSettings(F); fprintf(F, "\n"); cout << "\tFinished saving worldSettings.\n";
					for (auto it = gCameras.cbegin(); it != gCameras.cend(); ++it) if (!(*it)->inNode) (*it)->toSDL(F);	
						fprintf(F, "\n"); cout << "\tFinished saving cameras.\n";
					for (auto it = gMeshes.cbegin(); it != gMeshes.cend(); ++it) if (!it->second->inLibrary) it->second->toSDL(F); 
						fprintf(F, "\n"); cout << "\tFinished saving meshes.\n";
					for (int i = 0; i < gNumLights; ++i) gLights[i].toSDL(F); 
						fprintf(F, "\n"); cout << "\tFinished saving lights.\n";
					for (auto it = gMaterials.cbegin(); it != gMaterials.cend(); ++it) if (!it->second->inLibrary) it->second->toSDL(F); 
						fprintf(F, "\n"); cout << "\tFinished saving materials.\n";
					for (auto it = gNodes.cbegin(); it != gNodes.cend(); ++it) if (it->second->parent == nullptr) it->second->toSDL(F); fprintf(F, "\n"); cout << "\tFinished saving nodes.\n";
					fclose(F);
					if (flag) {
						cout << "\n================================================================================";
						cout << "\t\t\t\tExiting Console & Engine";
						cout << "\n================================================================================\n";
						glfwSetWindowShouldClose(gWindow, true);
						return;
					}
				}
				else if (token == "create")
				{
					iss >> token;
					if (token == "scene")
					{
						iss >> token;
						if (token == "scene")
						{
							cout << "\tName of scene (no \"\", no .scene): ";
							getline(cin, token);
							break;
						}
						FILE * g = fopen(token.c_str(), "w+");
						if (g == nullptr) break; //Shouldn't really happen as we create file on 404.

						cout << "\tPlease supply world settings:\n";
						fprintf(g, "worldSettings ");

						cout << "\tWindow Title (no \"\"): ";
						getline(cin, token);
						fprintf(g, "windowTitle \"%s\" {\n", token.c_str());

						cout << "\tWidth: ";
						getline(cin, token);
						fprintf(g, "\twidth %s\n", token.c_str());

						cout << "\tHeight: ";
						getline(cin, token);
						fprintf(g, "\theight %s\n", token.c_str());

						cout << "\tSamples per Pixel: ";
						getline(cin, token);
						fprintf(g, "\tspp %s\n", token.c_str());

						cout << "\tBackground Color [r g b]: ";
						getline(cin, token);
						fprintf(g, "\tbackgroundColor %s\n", token.c_str());

						cout << "\tBackground Music (no \"\"): ";
						getline(cin, token);
						fprintf(g, "\tbackgroundMusic \"%s\"\n", token.c_str());

						cout << "\tPrinting end of worldSettings.\n";
						fprintf(g, "}\n\n");

						cout << "\tAdding default camera.\n";
						fprintf(g, "\
								   						camera name \"camera1\" {\n\
																					eye[0 6 10]\n\
																												center[0 0 1]\n\
																																			vup[0 1 0]\n\
																																										fovy 0.5\n\
																																																	znear 0.1\n\
																																																								zfar 1000\n\
																																																														}\n");

						fclose(g);
						cout << "\tScene creation successful.\n";
						cout << "\tPlease now use \'load\' to open the scene and begin adding to it.\n";
					}
					else if (token == "camera")
					{
						gCameras.push_back(new Camera());
						iss >> token;
						if (token == "camera") //No name argument afterward.
						{
							cout << "\tName (no \"\"): ";
							getline(cin, gCameras.back()->name);
						}
						else gCameras.back()->name = token;
						consoleLoadCamera();
					}
					else if (token == "light")
					{
						if (gNumLights + 1 > MAX_LIGHTS)
						{
							cout << "\tNo more lights can be added to the scene, please \'delete\' one first.";
							break;
						}
						gLights[gNumLights] = Light();
						gLights[gNumLights].isOn = 1;

						cout << "\tLight Type {Point, Directional, Spot, Ambient, Head, Rim}: ";
						while (true) {
							cin >> token;
							if (token == "Point") { gLights[gNumLights].type = Light::LIGHT_TYPE::POINT; break; }
							else if (token == "Directional") { gLights[gNumLights].type = Light::LIGHT_TYPE::DIRECTIONAL; break; }
							else if (token == "Spot") { gLights[gNumLights].type = Light::LIGHT_TYPE::SPOT_LIGHT; break; }
							else if (token == "Ambient") { gLights[gNumLights].type = Light::LIGHT_TYPE::AMBIENT; break; }
							else if (token == "Head") { gLights[gNumLights].type = Light::LIGHT_TYPE::HEAD_LIGHT; break; }
							else if (token == "Rim") { gLights[gNumLights].type = Light::LIGHT_TYPE::RIM_LIGHT; break; }
							else cout << "\tPlease enter a valid light type: ";
						}

						cout << "\tLight isOn (0/1): ";
						cin >> gLights[gNumLights].isOn;

						if (gLights[gNumLights].type == Light::LIGHT_TYPE::SPOT_LIGHT)
						{
							cout << "\tLight attenLightCutoffCosine Alpha: ";
							cin >> gLights[gNumLights].alpha;

							cout << "\tLight attenLightCutoffCosine Theta: ";
							cin >> gLights[gNumLights].theta;
						}
						else gLights[gNumLights].alpha = gLights[gNumLights].theta = 0;

						cout << "\tLight Intensity/Color.r: ";
						cin >> gLights[gNumLights].intensity.r;
						cout << "\tLight Intensity/Color.g: ";
						cin >> gLights[gNumLights].intensity.g;
						cout << "\tLight Intensity/Color.b: ";
						cin >> gLights[gNumLights].intensity.b;

						if (!(gLights[gNumLights].type == Light::LIGHT_TYPE::DIRECTIONAL || gLights[gNumLights].type == Light::LIGHT_TYPE::AMBIENT))
						{
							cout << "\tLight Position.x: ";
							cin >> gLights[gNumLights].position.x;
							cout << "\tLight Position.y: ";
							cin >> gLights[gNumLights].position.y;
							cout << "\tLight Position.z: ";
							cin >> gLights[gNumLights].position.z;

							cout << "\tAttenuation: 1/(x*dd + y*d + z*1)\n";
							cout << "\tQuadratic Attenuation.x: ";
							cin >> gLights[gNumLights].attenuation.x;
							cout << "\tLinear Attenuation.y: ";
							cin >> gLights[gNumLights].attenuation.y;
							cout << "\tConstant Attenuation.z: ";
							cin >> gLights[gNumLights].attenuation.z;
						}
						else gLights[gNumLights].position = gLights[gNumLights].attenuation = glm::vec4(0);


						if (!(gLights[gNumLights].type == Light::LIGHT_TYPE::POINT || gLights[gNumLights].type == Light::LIGHT_TYPE::AMBIENT))
						{
							cout << "\tLight Direction.x: ";
							cin >> gLights[gNumLights].direction.x;
							cout << "\tLight Direction.y: ";
							cin >> gLights[gNumLights].direction.y;
							cout << "\tLight Direction.z: ";
							cin >> gLights[gNumLights].direction.z;
						}
						else gLights[gNumLights].direction = glm::vec4(0);

						++gNumLights;

						cout << "\tLight successfully initialized and added to gLights.\n";
					}
					else if (token == "material")
					{
						iss >> token;
						if (token == "material")
						{
							cout << "\tValid Commands:\n";
							cout << "\tcreate material materialName\n";
							break;
						}
						gMaterials[token] = new Material();
						gMaterials[token]->name = token;

						cout << "\tVertex Shader Filename.vs: ";
						cin >> gMaterials[token]->vertexShaderName;
						cout << "\tFragment Shader Filename.fs: ";
						cin >> gMaterials[token]->fragmentShaderName;
						gMaterials[token]->setShaderProgram(createShaderProgram(
							loadShader(gMaterials[token]->vertexShaderName, GL_VERTEX_SHADER),
							loadShader(gMaterials[token]->fragmentShaderName, GL_FRAGMENT_SHADER)
							)); //Return to modify this to take a container of shaders.

						string tmp;
						do {
							cout << "\tAdd color uniform (Y/N)? "; cin >> tmp; if (tmp == "N" || tmp == "n") break;
							gMaterials[token]->colors.push_back(new NameIdVal<glm::vec4>());
							cout << "\tColor's uniformName: "; cin >> gMaterials[token]->colors.back()->name;
							cout << "\tR-value: "; cin >> gMaterials[token]->colors.back()->val.r;
							cout << "\tG-value: "; cin >> gMaterials[token]->colors.back()->val.g;
							cout << "\tB-value: "; cin >> gMaterials[token]->colors.back()->val.b;
							cout << "\tA-value: "; cin >> gMaterials[token]->colors.back()->val.a;
						} while (true);

						do {
							cout << "\tAdd texture uniform (Y/N)? "; cin >> tmp; if (tmp == "N" || tmp == "n") break;
							gMaterials[token]->textures.push_back(new RGBAImage());
							cout << "\tSampler uniformName: "; cin >> gMaterials[token]->textures.back()->name;
							cout << "\tTexture fileName.png: "; cin >> gMaterials[token]->textures.back()->fileName;
							gMaterials[token]->textures.back()->loadPNG(gMaterials[token]->textures.back()->fileName);
							gMaterials[token]->textures.back()->sendToOpenGL();
						} while (true);

						gMaterials[token]->bindMaterial(); //Very important line!
						cout << "\tMaterial successfully created and added to gMaterials.\n";
					}
					else if (token == "mesh")
					{
						iss >> token; //Treated as meshName in loadMesh().
						if (token == "mesh")
						{
							cout << "\tValid Commands:\n";
							cout << "\tcreate mesh nameForMesh [nameOfFileToLoad.ply]\n";
							cout << "\tOmitting the last argument asks if filename is nameForMesh.ply.\n";
							break;
						}
						string fileName;
						gMeshes[token] = new TriMesh();
						gMeshes[token]->setName(token);
						iss >> fileName;
						if (fileName == token)
						{
							cout << "\tFound no filename supplied.\n";
							cout << "\tAssume the filename should be " << token << ".ply (Y/N): ";
							cin >> fileName;
							if (fileName == "N") { cout << "\tReturning to top level console.\n"; break; }
						}
						if (!gMeshes[token]->readFromPly(token + ".ply", false)) { cout << "\tReadFromPly() returned false, erasing mesh and returning to top-level console.\n"; gMeshes.erase(token); break; }
						if (!gMeshes[token]->sendToOpenGL()) { cout << "\tSendToOpenGL() returned false, erasing mesh and returning to top-level console.\n"; gMeshes.erase(token);  break; }
						cout << "\tMesh object successfully created and added to gMeshes.\n";
					}
					else if (token == "node")
					{
						iss >> token;
						if (token == "node") //No name argument afterward.
						{
							cout << "\tName (no \"\"): ";
							cin >> token;
						}
						consoleLoadNode(token);
					}
					else if (token == "script") cout << "\tNot supported at runtime as it requires recompilation.\n";
					else
					{
						cout << "\tValid Commands:\n";
						cout << "\tcreate camera\n\tcreate light\n\tcreate material\n\tcreate mesh\n\tcreate node\n\tcreate scene\n\tcreate script\n";
					}
					cout << "\tRemember to follow up with \'save\' to write to the file for non-scenes.\n";
				}
				else if (token == "print")
				{
					iss >> token;
					if (token == "cameras") for (auto it = gCameras.cbegin(); it != gCameras.cend(); ++it) cout << '\t' << (*it)->name << endl;
					else if (token == "lights")
					{
						for (int i = 0; i < gNumLights; ++i)
						{
							switch (gLights[i].type)
							{
							case Light::LIGHT_TYPE::POINT: cout << "\tpoint light\n"; break;
							case Light::LIGHT_TYPE::DIRECTIONAL: cout << "\tdirectional light\n"; break;
							case Light::LIGHT_TYPE::SPOT_LIGHT: cout << "\tspot light\n"; break;
							case Light::LIGHT_TYPE::AMBIENT: cout << "\tambient light\n"; break;
							case Light::LIGHT_TYPE::HEAD_LIGHT: cout << "\thead light\n"; break;
							case Light::LIGHT_TYPE::RIM_LIGHT: cout << "\trim light\n"; break;
							}
						}
					}
					else if (token == "materials") for (auto it = gMaterials.cbegin(); it != gMaterials.cend(); ++it) cout << '\t' << it->second->name << endl;
					else if (token == "meshes") for (auto it = gMeshes.cbegin(); it != gMeshes.cend(); ++it) cout << '\t' << it->second->name << endl;
					else if (token == "nodes") for (auto it = gNodes.cbegin(); it != gNodes.cend(); ++it) cout << '\t' << it->second->name << endl;
					else if (token == "scenes") for (auto it = gSceneFileNames.cbegin(); it != gSceneFileNames.cend(); ++it) cout << '\t' << *it << endl;
					else if (token == "scripts") for (auto it = gScripts.cbegin(); it != gScripts.cend(); ++it) cout << '\t' << it->second->name << endl;
					else if (token == "paths") for (auto it = getPATH().cbegin(); it != getPATH().cend(); ++it) cout << '\t' << *it << endl;
					else cout << "\tValid Commands:\n\tprint cameras\n\tprint lights\n\tprint materials\n\tprint meshes\n\tprint nodes\n\tprint scenes\n\tprint scripts\n\tprint paths\n";
				}
				else if (token == "help" || token == "man")
				{
					iss >> token;
					cout << "\tValid Commands:\n";
					if (token == "create")
					{
						iss >> token;
						if (token == "create");
						else if (token == "scene")
						{
							cout << "\tcreate scene sceneNameToCreate.scene\n";
							cout << "\tFile content will be overwritten; a file will be created if none exists.\n";
						}
					}
					else if (token == "load");
					else if (token == "save");
				}
			}
		}
	} while (true); //Console IO loop.
	cout << "\n================================================================================";
	cout << "\t\t\t\tExiting Console";
	cout << "\n================================================================================\n";
}

