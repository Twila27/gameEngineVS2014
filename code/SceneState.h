#pragma once 
#include "EngineUtil.h"

static GLFWwindow* gWindow = NULL;
static string gWindowTitle = "OpenGL App";
static int gWidth; // window width
static int gHeight; // window height
static int gSPP = 16; // samples per pixel
static glm::vec4 backgroundColor;

static ISoundEngine* soundEngine = NULL;
static ISound* gBackgroundMusic = NULL;

//Reason not using Scene is to preserve hot-updating scene files.
static map<string, TriMesh*> gMeshes;
static map<string, Material*> gMaterials;
//vector<TriMeshInstance*> gMeshInstances;
static map<string, SceneGraphNode*> gNodes;
static map<string, Script*> gScripts;
static vector<Camera*> gCameras;
static vector<string> gSceneFileNames;
static vector<string> gLibraries;
static map<string, SceneGraphNode*> gSelected;

//These will not change until their keys are pressed.
static unsigned int gActiveCamera = 0; //Ctrl.
static unsigned int gActiveScene = 0; //Shift.
static string gActiveSceneName; //Used in console, updated in loadScene().
static bool gShouldSwapScene = false;
static bool gShowPerFrameDebug = false; //F1.

//For dev controls.
static bool gBuildMode = false;
static string gGlobalTmpStr("");
void useConsole(void);