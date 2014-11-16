#include "SceneState.h"

GLFWwindow* gWindow = NULL;
string gWindowTitle = "OpenGL App";
int gWidth; // window width
int gHeight; // window height
int gSPP = 16; // samples per pixel
glm::vec4 backgroundColor;

ISoundEngine* soundEngine = NULL;
ISound* gBackgroundMusic = NULL;

//Reason not using Scene is to preserve hot-updating scene files.
map<string, TriMesh*> gMeshes;
map<string, Material*> gMaterials;
//vector<TriMeshInstance*> gMeshInstances;
map<string, SceneGraphNode*> gNodes;
map<string, Script*> gScripts;
vector<Camera*> gCameras;
vector<string> gSceneFileNames;
vector<string> gLibraries;
map<string, SceneGraphNode*> gSelected;

//These will not change until their keys are pressed.
unsigned int gActiveCamera = 0; //Ctrl.
unsigned int gActiveScene = 0; //Shift.
string gActiveSceneName; //Used in console, updated in loadScene().
bool gShouldSwapScene = false;
bool gShowPerFrameDebug = false; //F1.

//For dev controls.
bool gBuildMode = false;
string gGlobalTmpStr("");
void useConsole(void);