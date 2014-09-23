
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

//-------------------------------------------------------------------------//
// OPENGL STUFF
//-------------------------------------------------------------------------//

#define MAJOR_VERSION 4
#define MINOR_VERSION 0
GLFWwindow* createOpenGLWindow(int width, int height, const char *title, int samplesPerPixel=0);

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

	void refreshTransform(void)
	{
		glm::mat4x4 Mtrans = glm::translate(translation);
		glm::mat4x4 Mscale = glm::scale(scale);
		glm::mat4x4 Mrot = glm::toMat4(rotation);
		transform = Mtrans * Mrot * Mscale;  // transforms happen right to left
		invTransform = glm::inverse(transform);
	}
};

//-------------------------------------------------------------------------//
// Camera
//-------------------------------------------------------------------------//

class Camera
{
public:
	// look from, look at, view up
	// yaw, pitch, roll then affect look at or center!
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
		glm::mat4x4 R = glm::axisAngleMatrix(axis, angle);
		glm::vec4 zz = glm::vec4(eye - center, 0);
		glm::vec4 Rzz = R*zz;
		center = eye - glm::vec3(Rzz);
		//
		glm::vec4 up = glm::vec4(vup, 0);
		glm::vec4 Rup = R*up;
		vup = glm::vec3(Rup);
	}
	void rotateLocal(glm::vec3 axis, float angle) {
		glm::vec3 zz = glm::normalize(eye - center);
		glm::vec3 xx = glm::normalize(glm::cross(vup, zz));
		glm::vec3 yy = glm::cross(zz, xx);
		glm::vec3 aa = xx*axis.x + yy*axis.y + zz*axis.z;
		rotateGlobal(aa, angle);
	}
};

//-------------------------------------------------------------------------//
// LIGHT SOURCE
//-------------------------------------------------------------------------//

struct Light {
	//72 bytes = 4 int/float + 4 vec4.
	enum class LIGHT_TYPE : int { POINT = 27, DIRECTIONAL, SPOT_LIGHT, HEMISPHERICAL };
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

//-------------------------------------------------------------------------//
// TRIANGLE MESH
//-------------------------------------------------------------------------//

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

class Material
{
public:
	GLuint shaderProgramHandle; //Currently in instance class.
	GLuint lightUBOHandle; 
	vector<Light>* gLightsHandle; //Used in TriMeshInstance::draw() to send lights to pixel shader via above UBO.
	map<string, int> updatingUniforms; //Holds all uniforms locations indicated by their uniform name.
	glm::vec4 diffuseColor;
	glm::vec4 specularColor;
	float specularExponent; //Shiny factor.
	glm::vec4 ambientIntensity;
	glm::vec4 emissiveColor;
	vector<RGBAImage*> textures;
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
	void setLightUBOHandle(GLuint lightUBOHandle) { this->lightUBOHandle = lightUBOHandle; }
	void getAndInitUniforms()
	{ //Placing this code here potentially enables shader program swaps. Can move it to the .cpp though.

		if (shaderProgramHandle == NULL_HANDLE) {
			ERROR("Cannot get uniforms because the shader program handle is not set.");
			return;
		}

		glUseProgram(shaderProgramHandle);

		GLint loc;

		//Set up textures.
		for (int i = 0; i < textures.size(); ++i) {
			loc = textures[i]->id = glGetUniformLocation(shaderProgramHandle, textures[i]->name.c_str()); //The problem is this name is incorrect!
			if (loc != -1) {
				glGenSamplers(1, &textures[i]->samplerId);
				glGenTextures(1, &textures[i]->textureId);

				glActiveTexture(GL_TEXTURE0 + i); //Set active texture unit in GL context.
				glUniform1i(textures[i]->id, i); //Set the handle to the texture being used?

				if (textures[i]->name == "uSpecularExponentTex") glBindTexture(GL_TEXTURE_1D, textures[i]->textureId); //This tex is 1D.
				else glBindTexture(GL_TEXTURE_2D, textures[i]->textureId); //Associate texture and GL target.
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i]->width, textures[i]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textures[i]->pixels[0]);
				glGenerateMipmap(GL_TEXTURE_2D);

				glBindSampler(textures[i]->textureId, textures[i]->samplerId); //Associate texture and sampler. Already done in sendToOpenGL().
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			#ifdef _DEBUG
			else ERROR("Failure in texture setup loop.", false);
			#endif
		}

		//Set up non-textures.
		loc = glGetUniformLocation(shaderProgramHandle, "uDiffuseColor");
		if (loc != -1) glUniform4fv(loc, 1, &diffuseColor[0]);
#ifdef _DEBUG
		else ERROR("Failure getting diffuse color uniform.", false);
#endif 

		loc = glGetUniformLocation(shaderProgramHandle, "uAmbientIntensity");
		if (loc != -1) glUniform4fv(loc, 1, &ambientIntensity[0]);
#ifdef _DEBUG
		else ERROR("Failure getting ambient intensity uniform.", false);
#endif 

		loc = glGetUniformLocation(shaderProgramHandle, "uSpecularColor");
		if (loc != -1) glUniform4fv(loc, 1, &specularColor[0]);
#ifdef _DEBUG
		else ERROR("Failure getting specular color uniform.", false);
#endif 

		loc = glGetUniformLocation(shaderProgramHandle, "uSpecularExponent");
		if (loc != -1) glUniform1f(loc, specularExponent);
#ifdef _DEBUG
		else ERROR("Failure getting specular exponent uniform.", false);
#endif 

		loc = glGetUniformLocation(shaderProgramHandle, "uLoadedLights");
		updatingUniforms["uLoadedLights"] = loc;
		if (loc != -1) glUniform1i(loc, gLightsHandle->size());
#ifdef _DEBUG
		else ERROR("Failure getting max lights uniform.", false);
#endif 

		//Fill UBO at lightUBOHandle, copy into buffer w/o glBufferData()'s allocation.
		glBindBuffer(GL_UNIFORM_BUFFER, lightUBOHandle); //Bind to target.
		glBufferSubData(GL_UNIFORM_BUFFER, 0, gLightsHandle->size() * sizeof(Light), (void*)gLightsHandle);
		//^We know this much works because we see the sudden data change for the UBO on gDEBugger.

		//Attach UBO to uniform block in GLSL.
		loc = glGetUniformBlockIndex(shaderProgramHandle, "ubGlobalLights");
		updatingUniforms["ubGlobalLights"] = loc;
		if (loc != -1) {
			glUniformBlockBinding(shaderProgramHandle, loc, 1); //Associates UB to binding point 1.
			glBindBufferBase(GL_UNIFORM_BUFFER, 1, lightUBOHandle); //Associates UBO to binding point 1.
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0); //Unbind.
#ifdef _DEBUG
		if (loc == -1) ERROR("Failure getting lights uniform block.", false);
#endif 
		glUseProgram(0);
	}
};

// should extend EngineObject
class TriMeshInstance 
{
public:
	TriMesh *triMesh;
	Material *instanceMaterial;
	Transform instanceTransform;
	
public:
	TriMeshInstance(void);

	void setMesh(TriMesh *mesh) { triMesh = mesh; }
	void setMaterial(Material *material) { instanceMaterial = material; }
	void setDiffuseColor(const glm::vec4 &c) { instanceMaterial->diffuseColor = c; }
	void setSpecularColor(const glm::vec4 &c) { instanceMaterial->specularColor = c; }
	void setScale(const glm::vec3 &s) { instanceTransform.scale = s; }
	void setRotation(const glm::quat &r) { instanceTransform.rotation = r; }
	void setTranslation(const glm::vec3 &t) { instanceTransform.translation = t; }
    
	void refreshTransform(void);
    
	void draw(Camera &camera);
};

//-------------------------------------------------------------------------//
