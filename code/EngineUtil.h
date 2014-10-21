
//-------------------------------------------------------------------------//
// EngineUtil.h
// Some utility stuff - file reading and such
//
// David Cline
// 8/23/2014
//-------------------------------------------------------------------------//

// Prevent visual studio warnings
#define _CRT_SECURE_NO_WARNINGS

// some standard includes
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
using namespace std;

// lodePNG stuff (image reading)
#include "lodepng.h"

// OpenGL related includes
#define GLM_FORCE_RADIANS
//#define PRINT_GLSL //Uncomment to print full shader code, separated to speed up Debug build time.
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// Sound (irrKlang)
#include <irrKlang.h>
using namespace irrklang;

//-------------------------------------------------------------------------//
// MISCELLANEOUS
//-------------------------------------------------------------------------//

void ERROR(const string &msg, bool doExit = true);
double TIME(void);
void SLEEP(int millis);

//For controlling the number of time steps we take between update() or render() calls.
extern const double FIXED_DT; //Represents the non-integral amount of frames between last and current game loop. Weak to pausing.

//-------------------------------------------------------------------------//
// OPENGL STUFF
//-------------------------------------------------------------------------//

#define MAJOR_VERSION 4
#define MINOR_VERSION 0
GLFWwindow* createOpenGLWindow(int width, int height, const char *title, int samplesPerPixel = 0);

#define NULL_HANDLE 0
GLuint loadShader(const string &fileName, GLuint shaderType);
GLuint createShaderProgram(GLuint vertexShader, GLuint fragmentShader);

//-------------------------------------------------------------------------//
// GLM UTILITY STUFF
//-------------------------------------------------------------------------//

inline void printVec(const glm::vec2 &v) { printf("[%1.3f %1.3f]\n", v[0], v[1]); }
inline void printVec(const glm::vec3 &v) { printf("[%1.3f %1.3f %1.3f]\n", v[0], v[1], v[2]); }
inline void printVec(const glm::vec4 &v) { printf("[%1.3f %1.3f %1.3f %1.3f]\n", v[0], v[1], v[2], v[3]); }
inline void printQuat(const glm::quat &q) { printf("[%1.3f %1.3f %1.3f %1.3f]\n", q[0], q[1], q[2], q[3]); }
void printMat(const glm::mat4x4 &m);

//-------------------------------------------------------------------------//
// FILE READING
//-------------------------------------------------------------------------//

void addToPath(const string &p);

void removeFromPath(const string &p);

bool getFullFileName(const string &fileName, string &fullName);
FILE *openFileForReading(const string &fileName);

bool getToken(FILE *f, string &token, const string &oneCharTokens);
int getFloats(FILE *f, float *a, int num);
int getInts(FILE *f, int *a, int num);

bool loadFileAsString(const string &fileName, string &buffer);

void replaceIncludes(string &src, string &dest, const string &directive,
	string &alreadyIncluded, bool onlyOnce);

//-------------------------------------------------------------------------//
// SOUND
//-------------------------------------------------------------------------//

void initSoundEngine(void);
ISound *loadSound();

//-------------------------------------------------------------------------//
// IMAGE
//-------------------------------------------------------------------------//

class RGBAImage
{
public:
	GLuint id; //Uniform loc.
	string name;
	vector<unsigned char> pixels;
	unsigned int width, height;
	GLuint textureId;
	GLuint samplerId;

	RGBAImage(void) { width = 0; height = 0; textureId = NULL_HANDLE; samplerId = NULL_HANDLE; }
	~RGBAImage();
	bool loadPNG(const string &fileName, bool doFlipY = true);
	bool writeToPNG(const string &fileName);
	void flipY(void);
	void sendToOpenGL(GLuint magFilter, GLuint minFilter, bool createMipMap);

	unsigned int &operator()(int x, int y) {
		return pixel(x, y);
	}
	unsigned int &pixel(int x, int y) {
		unsigned int *A = (unsigned int*)&pixels[0];
		return A[y*width + x];
	}
	void sendToOpenGL(void) {
		sendToOpenGL(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true);
	}
};

//-------------------------------------------------------------------------//
// TRANSFORM
//-------------------------------------------------------------------------//

class Transform
{
public:
	// make transform class
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;

	glm::mat4x4 transform;
	glm::mat4x4 invTransform;

	void refreshTransform(const glm::mat4x4 &parentTransform = glm::mat4(), const glm::vec3 &parentTrans = glm::vec3(1), const glm::vec3 &parentScale = glm::vec3(0), bool shouldRotate = true)
	{
		glm::mat4x4 Mtrans = glm::translate(translation);
		glm::mat4x4 Mscale = glm::scale(scale);
		glm::mat4x4 Mrot = glm::toMat4(rotation);
		if (shouldRotate) transform = parentTransform * Mtrans * Mrot * Mscale;  // transforms happen right to left
		else transform = glm::translate(parentTrans) * glm::scale(parentScale) * Mtrans * Mrot * Mscale; // child will rot indep of parent
		invTransform = glm::inverse(transform);
	}
};

//-------------------------------------------------------------------------//
// Camera
//-------------------------------------------------------------------------//

class Camera
{
public:
	//Eye is camera position, the lookFrom.
	//Center is the camera lookAt target.
	glm::vec3 eye, center, vup;

	float fovy; // vertical field of view
	float znear, zfar; // near and far clip planes

	glm::mat4x4 worldViewProject;

	void refreshTransform(float screenWidth, float screenHeight)
	{
		glm::mat4x4 worldView = glm::lookAt(eye, center, vup);
		glm::mat4x4 project = glm::perspective((float)fovy,
			(float)(screenWidth / screenHeight), (float)znear, (float)zfar);
		worldViewProject = project * worldView;
	}
	void translateGlobal(glm::vec3 &t) { eye += t; center += t; }
	void translateLocal(glm::vec3 &t) {
		glm::vec3 zz = glm::normalize(eye - center);
		glm::vec3 xx = glm::normalize(glm::cross(vup, zz));
		glm::vec3 yy = glm::cross(zz, xx);
		glm::vec3 tt = t.x*xx + t.y*yy + t.z*zz;
		eye += tt; center += tt;
	}
	void rotateGlobal(glm::vec3 axis, float angle) {
		glm::mat4x4 R = glm::axisAngleMatrix(axis, angle); //Rotation transform matrix.
		glm::vec4 zz = glm::vec4(eye - center, 0); //Local z axis == obj.pos - obj.dir, unnormalized?
		glm::vec4 Rzz = R*zz; //Z-axis rotated.
		center = eye - glm::vec3(Rzz); //Add back eye via subtraction, as dir is opposite.
		//
		glm::vec4 up = glm::vec4(vup, 0); //Cast vec3 to vec4 for next line.
		glm::vec4 Rup = R*up;
		vup = glm::vec3(Rup);
	}
	void rotateLocal(glm::vec3 axis, float angle) {
		//zz, xx, yy are local axes.
		//Compute the local axes first, then shift arguments into local space.
		glm::vec3 zz = glm::normalize(eye - center); 
		glm::vec3 xx = glm::normalize(glm::cross(vup, zz));
		glm::vec3 yy = glm::cross(zz, xx);
		glm::vec3 aa = xx*axis.x + yy*axis.y + zz*axis.z; //Shifts here, aa is the axis you want to rotate around.
		rotateGlobal(aa, angle);
	}
};


//-------------------------------------------------------------------------//
// LIGHT
//-------------------------------------------------------------------------//

struct Light {
	//72 bytes = 4 int/float + 4 vec4.
	enum class LIGHT_TYPE : int { POINT = 1, DIRECTIONAL, SPOT_LIGHT, AMBIENT, HEAD_LIGHT, RIM_LIGHT };
	LIGHT_TYPE type;
	float alpha, theta; //Angles we can use for spotlights?
	int isOn;
	glm::vec4 intensity; //A color.
	glm::vec4 position;
	glm::vec4 direction;
	glm::vec4 attenuation; //ABC for the 1/(Add + Bd + C) attenuation computation.
	Light(void) {}
	Light(LIGHT_TYPE type, const glm::vec4 &pos, const glm::vec4 &dir, const glm::vec4 &atten)
		: type(type), position(pos), direction(dir), attenuation(atten) { }
};

#define MAX_LIGHTS 8
extern GLuint gLightsUBO;
extern int gNumLights;
extern Light gLights[MAX_LIGHTS];

void initLightBuffer(void);

//-------------------------------------------------------------------------//
// MATERIAL, MESH & DRAWABLE
//-------------------------------------------------------------------------//

class Material
{
public:
	string name;
	GLuint shaderProgramHandle;
	glm::vec4 diffuseColor; //If we add setters, do we get tinting? Might need to make uniforms streaming then...
	glm::vec4 specularColor;
	float specularExponent; //Shiny factor.
	glm::vec4 ambientIntensity;
	glm::vec4 emissiveColor;
	vector<RGBAImage*> textures;
	vector<glm::vec4> colors; //!
	//"You want to be able to reuse the same shader and just send colors to the material."
	//"Really you should have a MATERIAL CLASS that looks up the indices one time and stores those indices."
	//"Once the shader program is compiled, the indices of the different uniforms then do not change."
	Material(void);
	~Material(void)
	{
		for (auto it = textures.begin(); it != textures.end(); ++it) delete *it;
		glDeleteProgram(shaderProgramHandle);
	}
	void setShaderProgram(GLuint shaderProgram) { shaderProgramHandle = shaderProgram; }
	void getAndInitUniforms()
	{ //Placing this code here potentially enables shader program swaps. Can move it to the .cpp though.

		if (shaderProgramHandle == NULL_HANDLE) {
			ERROR("Cannot get uniforms because the shader program handle is not set.");
			return;
		}

		glUseProgram(shaderProgramHandle);

		GLint loc;

		//Set up textures.
		for (int i = 0; i < (int)textures.size(); ++i) {
			loc = textures[i]->id = glGetUniformLocation(shaderProgramHandle, textures[i]->name.c_str()); //The problem is this name is incorrect!
			if (loc != -1) {
				glGenSamplers(1, &textures[i]->samplerId);
				glGenTextures(1, &textures[i]->textureId);

				glActiveTexture(GL_TEXTURE0 + i); //Set active texture unit in GL context.
				glUniform1i(textures[i]->id, i); //Set the handle to the texture being used?

				if (textures[i]->name == "uSpecularExponentTex") glBindTexture(GL_TEXTURE_1D, textures[i]->textureId); //This tex is 1D.
				else glBindTexture(GL_TEXTURE_2D, textures[i]->textureId); //Associate texture and GL target.

				//Important find--glBindTexture() needs to be called BEFORE glTexImage2D(), to not generate a bunch of stupid empty images every time.
				//Note that we axed the sendToOpenGL() method of RGBAImage*, all that code is here.

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i]->width, textures[i]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textures[i]->pixels[0]);
				glGenerateMipmap(GL_TEXTURE_2D);

				glBindSampler(textures[i]->textureId, textures[i]->samplerId); //Associate texture and sampler. Already done in sendToOpenGL().
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
#ifdef _DEBUG
			else ERROR("\n\tFailure in texture setup loop.", false);
#endif
		}

		//Set up non-textures.
		loc = glGetUniformLocation(shaderProgramHandle, "uDiffuseColor");
		if (loc != -1) glUniform4fv(loc, 1, &diffuseColor[0]);
#ifdef _DEBUG
		else ERROR("\n\tFailure getting diffuse color uniform.", false);
#endif 

		loc = glGetUniformLocation(shaderProgramHandle, "uAmbientIntensity");
		if (loc != -1) glUniform4fv(loc, 1, &ambientIntensity[0]);
#ifdef _DEBUG
		else ERROR("\n\tFailure getting ambient intensity uniform.", false);
#endif 

		loc = glGetUniformLocation(shaderProgramHandle, "uSpecularColor");
		if (loc != -1) glUniform4fv(loc, 1, &specularColor[0]);
#ifdef _DEBUG
		else ERROR("\n\tFailure getting specular color uniform.", false);
#endif 

		loc = glGetUniformLocation(shaderProgramHandle, "uSpecularExponent");
		if (loc != -1) glUniform1f(loc, specularExponent);
#ifdef _DEBUG
		else ERROR("\n\tFailure getting specular exponent uniform.", false);
#endif 
		glUseProgram(0);
	} // ONLY will run once, no streaming updates.
};

class TriMesh
{
public:
	string name;
	vector<string> attributes;
	vector<float> vertexData;
	vector<int> indices;
	int numIndices;

	GLuint vao; // vertex array handle
	GLuint ibo; // index buffer handle

	~TriMesh() { glDeleteBuffers(1, &vao); glDeleteBuffers(1, &ibo); }
	void setName(const string &str) { name = str; }
	bool readFromPly(const string &fileName, bool flipZ = false);
	bool sendToOpenGL(void);
	void draw(void);
};

class Drawable {
public:
	TriMesh *triMesh;
	Material *material;
	enum TYPE { TRIMESHINSTANCE, SPRITE, BILLBOARD };
	TYPE type;

	Drawable(void) { triMesh = nullptr; material = nullptr; }
	Material* getMaterial() { return material; }
	void setMesh(TriMesh *mesh) { triMesh = mesh; }
	void setMaterial(Material *material_) { material = material_; }
	virtual void draw(Camera& camera); //Not pure anymore, handles general mesh render.
	virtual void prepareToDraw(Camera& camera, Transform& T, Material& material) {} //Handle subclass-specific preparation.
};

//-------------------------------------------------------------------------//
// DRAWABLES
//-------------------------------------------------------------------------//

class Sprite : public Drawable {
public:
	vector<glm::vec4> frames; //Need this list of [x y w h] normalized, (0,0) top-left and (1,1) width-height, UV frames specified.
	int frameWidth, frameHeight;
	int sheetWidth, sheetHeight; //Yanked from material->textures[0].
	int amtRows, amtCols;
	int animDir;
	float animRate, currAccumulatedTime;
	int activeFrame, activeRow;
	//Assign alpha (using discard() GLSL function) and texture frame in fragment shader.
	//May enable back-face culling.
	//We can get the size of frames[][] from nRows or := textures[0].w/spriteW * nCols or := textures[0].h/spriteH.
	//Stick to a row per animation, kept in activeRow variable in this class.
	Sprite(void) { 
		animDir = animRate = amtRows = amtCols = 1; 
		activeFrame = activeRow = frameWidth = frameHeight = sheetWidth = sheetHeight = currAccumulatedTime = 0;
	}
	void switchAnim(int newRow) { activeRow = newRow; } //Just ensure animations have an enum.
	virtual void prepareToDraw(Camera& camera, Transform& T, Material& material) override;
	//virtual void draw(Camera& camera) override;
};

class Billboard : public Sprite {
public:
	//void draw(Camera& camera) override;
	void prepareToDraw(Camera& camera, Transform& T, Material& material) override;
};

// should extend EngineObject
class TriMeshInstance : public Drawable
{
public:
	//void draw(Camera& camera) override;
};


//-------------------------------------------------------------------------//
// SCRIPT
//-------------------------------------------------------------------------//

class Script {};

//-------------------------------------------------------------------------//
// SCENE GRAPH NODE
//-------------------------------------------------------------------------//

class SceneGraphNode {
public:
	string name;
	int activeLOD;
	vector<Drawable*> LODstack; //Level of detail stack.
	vector<float> switchingDistances; //Decreasing order such that [0] is max render threshold.
	vector<Camera*> cameras; //How to get it to follow the object?
	vector<SceneGraphNode*> children;
	SceneGraphNode * parent;
	vector<Script*> scripts;
	Transform T;
	ISound * objSound;
	void setScale(const glm::vec3 &s) { T.scale = s; }
	void setRotation(const glm::quat &r) { T.rotation = r; }
	void setTranslation(const glm::vec3 &t) { T.translation = t; }
	
	SceneGraphNode(void);
	~SceneGraphNode(void);
	void draw(Camera &camera); //Make it use the LODstack!
	void update(Camera &camera);
};

//===========
// TEXT API
//===========
//void initText2D(const char * texturePath, int numRows, int numCols);
//void printText2D(const char * text, int x, int y, int size);
//void cleanupText2D();
