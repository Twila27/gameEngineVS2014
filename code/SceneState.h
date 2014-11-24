#pragma once 
#include "EngineUtil.h"

extern GLFWwindow* gWindow;
extern string gWindowTitle;
extern int gWidth; // window width
extern int gHeight; // window height
extern int gSPP; // samples per pixel
extern glm::vec4 gBackgroundColor;

extern ISoundEngine* soundEngine;
extern ISound* gBackgroundMusic;

//Reason not using Scene is to preserve hot-updating scene files.
extern map<string, TriMesh*> gMeshes;
extern map<string, Material*> gMaterials;
//vector<TriMeshInstance*> gMeshInstances;
extern map<string, SceneGraphNode*> gNodes;
extern map<string, Script*> gScripts;
extern vector<Camera*> gCameras;
extern vector<string> gSceneFileNames;
extern vector<string> gLibraries;
extern map<string, SceneGraphNode*> gSelected;

//These will not change until their keys are pressed.
extern unsigned int gActiveCamera; //Ctrl.
extern unsigned int gActiveScene; //Shift.
extern string gActiveSceneName; //Used in console, updated in loadScene().
extern bool gShouldSwapScene;
extern bool gShowPerFrameDebug; //F1.

//For dev controls.
extern bool gBuildMode;
extern string gGlobalTmpStr;
void useConsole(void);