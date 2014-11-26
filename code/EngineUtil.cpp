// Local includes
#include "EngineUtil.h"

//-------------------------------------------------------------------------//
// MISCELLANEOUS
//-------------------------------------------------------------------------//

void ERROR(const string &msg, bool doExit)
{
	cerr << "\nERROR! " << msg << endl;
	if (doExit) {
#ifdef _DEBUG
		cin >> doExit;
#endif
		exit(0);
	}
}
double TIME(void)
{
	return (double)clock() / (double)CLOCKS_PER_SEC;
}
void SLEEP(int millis)
{
	this_thread::sleep_for(chrono::milliseconds(millis));
}

//-------------------------------------------------------------------------//
// OPENGL STUFF
//-------------------------------------------------------------------------//

GLFWwindow* createOpenGLWindow(int width, int height, const char *title, int samplesPerPixel)
{
	// Initialise GLFW
	if (!glfwInit()) ERROR("Failed to initialize GLFW.", true);
	glfwWindowHint(GLFW_SAMPLES, samplesPerPixel);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, MAJOR_VERSION);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, MINOR_VERSION);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Open a window and create its OpenGL context
	GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		ERROR("Failed to open GLFW window.", true);
	}
	glfwMakeContextCurrent(window);

	// Ensure we can capture the escape key being pressed
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		glfwTerminate();
		ERROR("Failed to initialize GLEW.", true);
	}

	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);

	// Print the OpenGL version we are working with
	char *GL_version = (char *)glGetString(GL_VERSION);
	printf("OpenGL Version: %s\n", GL_version);
	return window;
}
GLuint loadShader(const string &fileName, GLuint shaderType)
{
	// load the shader as a file
	string mainCode;
	if (!loadFileAsString(fileName, mainCode)) {
		ERROR("Could not load file '" + fileName + "'", false);
		return NULL_HANDLE;
	}

	string shaderCode;
	string alreadyIncluded = fileName;
	replaceIncludes(mainCode, shaderCode, "#include", alreadyIncluded, true);


	// print the shader code
#ifdef PRINT_GLSL
	cout << "\n----------------------------------------------- SHADER CODE:\n";
	cout << shaderCode << endl;
	cout << "--------------------------------------------------------------\n";
#endif

	// transfer shader code to card and compile
	GLuint shaderHandle = glCreateShader(shaderType); // create handle for the shader
	const char* source = shaderCode.c_str();          // get C style string for shader code
	glShaderSource(shaderHandle, 1, &source, NULL);   // pass the shader code to the card
	glCompileShader(shaderHandle);                    // attempt to compile the shader

	// check to see if compilation worked
	// If the compilation did not work, print an error message and return NULL handle
	int status;
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		ERROR("compiling shader '" + fileName + "'", false);
		GLint msgLength = 0;
		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &msgLength);
		std::vector<char> msg(msgLength);
		glGetShaderInfoLog(shaderHandle, msgLength, &msgLength, &msg[0]);
		printf("%s\n", &msg[0]);
		glDeleteShader(shaderHandle);
		return NULL_HANDLE;
	}

	return shaderHandle;
}
GLuint createShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// Create and link the shader program
	GLuint shaderProgram = glCreateProgram(); // create handle
	if (!shaderProgram) {
		ERROR("could not create the shader program", false);
		return NULL_HANDLE;
	}
	glAttachShader(shaderProgram, vertexShader);    // attach vertex shader
	glAttachShader(shaderProgram, fragmentShader);  // attach fragment shader
	glLinkProgram(shaderProgram);

	// check to see if the linking was successful
	int linked;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked); // get link status
	if (!linked) {
		ERROR("could not link the shader program", false);
		GLint msgLength = 0;
		glGetShaderiv(shaderProgram, GL_INFO_LOG_LENGTH, &msgLength);
		std::vector<char> msg(msgLength);
		glGetShaderInfoLog(shaderProgram, msgLength, &msgLength, &msg[0]);
		printf("%s\n", &msg[0]);
		glDeleteProgram(shaderProgram);
		return NULL_HANDLE;
	}

	//Attach UBO to uniform block in GLSL via the same binding point.
	GLint locLightUB = glGetUniformBlockIndex(shaderProgram, "ubGlobalLights");
	glUniformBlockBinding(shaderProgram, locLightUB, 1); //Associates UB to binding point 1.
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, gLightsUBO); //Associates UBO to binding point 1. 

	return shaderProgram;
}

//-------------------------------------------------------------------------//
// GLM UTILITY STUFF
//-------------------------------------------------------------------------//

void printMat(const glm::mat4x4 &m)
{
	printf("\n");
	for (int r = 0; r < 4; r++) {
		printf("[ ");
		for (int c = 0; c < 4; c++) {
			printf("%7.3f ", m[c][r]); // glm uses column major order
		}
		printf("]\n");
	}
}

//-------------------------------------------------------------------------//
// FILE READING
//-------------------------------------------------------------------------//

vector<string> PATH;
const vector<string>& getPATH() { return PATH; }
void addToPath(const string &p)
{
	PATH.push_back(p);
}
void removeFromPath(const string &p)
{
	for (int i = (int)p.length() - 1; i >= 0; i--) {
		if (PATH[i] == p) PATH.erase(PATH.begin() + i);
	}
}
bool getFullFileName(const string &fileName, string &fullName)
{
	for (int i = -1; i < (int)PATH.size(); i++) {
		if (i < 0) fullName = fileName;
		else fullName = PATH[i] + fileName;

		FILE *f = fopen(fullName.c_str(), "rb");
		if (f != NULL) {
			fclose(f);
			return true;
		}
	}
	fullName = "";
	return false;
}
FILE *openFileForReading(const string &fileName)
{
	string fullName;
	bool fileExists = getFullFileName(fileName, fullName);
	if (fileExists) {
		cout << "Opening file '" << fileName << "'" << endl;
		return fopen(fullName.c_str(), "rb");
	}

	string msg = "Could not open file " + fileName;
	ERROR(msg.c_str(), false);
	return NULL;
}
bool getToken(FILE *f, string &token, const string &oneCharTokens)
{
	token = "";
	while (!feof(f)) {
		int c = getc(f);
		int tokenLength = (int)token.length();
		int cIsSpace = isspace(c);
		bool cIsOneCharToken = ((int)oneCharTokens.find((char)c) >= 0);

		if (cIsSpace && tokenLength == 0) { // spaces before token, ignore
			continue;
		}
		else if (cIsSpace) { // space after token, done
			break;
		}
		else if (c == EOF) { // end of file, done
			break;
		}
		else if ((tokenLength == 0) && cIsOneCharToken) { // oneCharToken, done
			token += (char)c;
			break;
		}
		else if (cIsOneCharToken) { // oneCharToken after another token, push back
			ungetc(c, f);
			break;
		}
		else if ((tokenLength == 0) && (c == '\"' || c == '\'')) { // quoted string, append til end quote found
			char endQuote = c;
			while (!feof(f)) {
				int d = getc(f);
				if (d == endQuote) return true;
				token += (char)d;
			}
			break;
		}
		else if (c == '\"' || c == '\'') { // quote after token started, push back
			ungetc(c, f);
			break;
		}
		else {
			token += (char)c;
		}
	}
	//cout << token << endl;
	return (token.length() > 0);
}
int getFloats(FILE *f, float *a, int num)
{
	string token;
	int count = 0;
	while (getToken(f, token, "[],")) {
		if (token == "]") {
			break;
		}
		else if (isdigit(token[0]) || token[0] == '-') {
			sscanf(token.c_str(), "%f", &a[count]);
			count++;
			if (count == num) break;
		}
	}
	return count;
}
int getInts(FILE *f, int *a, int num)
{
	string token;
	int count = 0;
	while (getToken(f, token, "[],")) {
		if (token == "]") {
			break;
		}
		else if (isdigit(token[0]) || token[0] == '-') {
			sscanf(token.c_str(), "%d", &a[count]);
			count++;
			if (count == num) break;
		}
	}
	return count;
}
bool loadFileAsString(const string &fileName, string &fileContents)
{
	printf("loading file '%s'\n", fileName.c_str());

	string fullName;
	bool fileExists = getFullFileName(fileName, fullName);
	if (!fileExists) return false;

	ifstream fileStream(fullName.c_str());
	if (fileStream.good()) {
		stringstream stringStream;
		stringStream << fileStream.rdbuf();
		fileContents = stringStream.str();
		fileStream.close();
		return true;
	}

	fileStream.close();
	return false;
}
void replaceIncludes(string &src, string &dest, const string &directive,
	string &alreadyIncluded, bool onlyOnce)
{
	int start = 0;

	while (true) {
		int includeIndex = (int)src.find("#include", start);
		if (includeIndex < 0) {
			dest += src.substr(start);
			break;
		}
		if (includeIndex > 0 && !isspace(src[includeIndex - 1])) continue;
		dest += src.substr(start, includeIndex - 1);
		//
		int quoteStart = (int)src.find("\"", start + 8);
		int quoteEnd = (int)src.find("\"", quoteStart + 1);
		start = quoteEnd + 1;
		if (quoteStart >= quoteEnd) {
			ERROR("could not replace includes");
			break;
		}
		string includeFileName = src.substr(quoteStart + 1, (quoteEnd - quoteStart - 1));
		if ((int)alreadyIncluded.find(includeFileName) < 0) {
			if (onlyOnce) {
				alreadyIncluded.append("|");
				alreadyIncluded.append(includeFileName);
			}
			string subSource;
			loadFileAsString(includeFileName, subSource);
			replaceIncludes(subSource, dest, directive, alreadyIncluded, onlyOnce);
		}
	}
}

//-------------------------------------------------------------------------//
// RGBAImage
//-------------------------------------------------------------------------//

RGBAImage::~RGBAImage()
{
	if (textureId != NULL_HANDLE) glDeleteTextures(1, &textureId);
	if (samplerId != NULL_HANDLE) glDeleteSamplers(1, &samplerId);
}
bool RGBAImage::loadPNG(const string &fileName, bool doFlipY)
{
	this->fileName = fileName;
	string fullName;
	getFullFileName(fileName, fullName);
	unsigned error = lodepng::decode(pixels, width, height, fullName.c_str());
	if (error) {
		ERROR(lodepng_error_text(error), false);
		ERROR("LoadPNG error, check if texture is in folder with right name!", false);
		return false;
	}

	if (doFlipY) flipY(); // PNGs go top-to-bottom, OpenGL is bottom-to-top
	//name = fileName;
	return true;
}
bool RGBAImage::writeToPNG(const string &fileName)
{
	unsigned error = lodepng::encode(fileName.c_str(), pixels, width, height);
	if (error) {
		ERROR(lodepng_error_text(error), false);
		return false;
	}
	return true;
}
void RGBAImage::flipY(void)
{
	unsigned int *a, *b;
	unsigned int temp;

	for (int y = 0; y < (int)height / 2; y++)
	{
		a = &pixel(0, y);
		b = &pixel(0, height - 1 - y);
		for (int x = 0; x < (int)width; x++) {
			temp = a[x];
			a[x] = b[x];
			b[x] = temp;
		}
	}
}
void RGBAImage::sendToOpenGL(GLuint magFilter, GLuint minFilter, bool createMipMap)
{
	if (width <= 0 || height <= 0) return;

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId); //Handled in Material init for uniforms.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]); //<-- The big call that actually creates the info used by the buffer made in glGenTextures()?
	if (createMipMap) glGenerateMipmap(GL_TEXTURE_2D);

	glGenSamplers(1, &samplerId);
	glBindSampler(textureId, samplerId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
}

//-------------------------------------------------------------------------//

void Camera::refreshTransform(float screenWidth, float screenHeight)
{
	glm::mat4x4 worldView = glm::lookAt(eye, center, vup);
	glm::mat4x4 project = glm::perspective((float)fovy,
		(float)(screenWidth / screenHeight), (float)znear, (float)zfar);
	worldViewProject = project * worldView;
}
void Camera::translateLocal(const glm::vec3 &t) {
	glm::vec3 zz = glm::normalize(eye - center);
	glm::vec3 xx = glm::normalize(glm::cross(vup, zz));
	glm::vec3 yy = glm::cross(zz, xx);
	glm::vec3 tt = t.x*xx + t.y*yy + t.z*zz;
	eye += tt; center += tt;
}
void Camera::rotateGlobal(const glm::vec3 &axis, const float angle) {
	glm::mat4x4 R = glm::axisAngleMatrix(axis, angle); //Rotation transform matrix.
	glm::vec4 zz = glm::vec4(eye - center, 0); //Local z axis == obj.pos - obj.dir, unnormalized?
	glm::vec4 Rzz = R*zz; //Z-axis rotated.
	center = eye - glm::vec3(Rzz); //Add back eye via subtraction, as dir is opposite.
	//
	glm::vec4 up = glm::vec4(vup, 0); //Cast vec3 to vec4 for next line.
	glm::vec4 Rup = R*up;
	vup = glm::vec3(Rup);
}
void Camera::rotateLocal(const glm::vec3 &axis, const float angle) {
	//zz, xx, yy are local axes. Compute them first, then shift arguments into local space.
	glm::vec3 zz = glm::normalize(eye - center);
	glm::vec3 xx = glm::normalize(glm::cross(vup, zz));
	glm::vec3 yy = glm::cross(zz, xx);
	glm::vec3 aa = xx*axis.x + yy*axis.y + zz*axis.z; //Shifts here, aa is the axis you want to rotate around.
	rotateGlobal(aa, angle);
}

//-------------------------------------------------------------------------//

void Material::bindMaterial(void) {

	if (shaderProgramHandles[activeShaderProgram] == NULL_HANDLE) {
		ERROR("Cannot get uniforms because the shader program handle is not set.", false);
		return;
	}

	glUseProgram(shaderProgramHandles[activeShaderProgram]);

	//Set up textures.
	for (int i = 0; i < (int)textures.size(); ++i) {
		textures[i]->id = glGetUniformLocation(shaderProgramHandles[activeShaderProgram], textures[i]->name.c_str()); //The problem is this name is incorrect!
		if (textures[i]->id != -1) {
			//glGenSamplers(1, &textures[i]->samplerId);
			//glGenTextures(1, &textures[i]->textureId);
			glActiveTexture(GL_TEXTURE0 + i); //Set active texture unit in GL context.
			glUniform1i(textures[i]->id, i); //Set the handle to the texture being used?
			if (textures[i]->name == "uSpecularExponentTex") glBindTexture(GL_TEXTURE_1D, textures[i]->textureId); //This tex is 1D.
			else glBindTexture(GL_TEXTURE_2D, textures[i]->textureId); //Associate texture and GL target.
			//Important find--glBindTexture() needs to be called BEFORE glTexImage2D(), to not generate a bunch of stupid empty images every time.
			//Note that we axed the sendToOpenGL() method of RGBAImage*, all that code is here.
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i]->width, textures[i]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textures[i]->pixels[0]); //<-- THIS is the method we do NOT want to call repeatedly.
			//glGenerateMipmap(GL_TEXTURE_2D); //<-- Possibly also a memory biter.
			glBindSampler(textures[i]->textureId, textures[i]->samplerId); //Associate texture and sampler. Already done in sendToOpenGL().
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
#ifdef _DEBUG
		else ERROR("\n\tFailure in texture setup loop.", false);
#endif
	}

	for (int i = 0; i < (int)colors.size(); ++i) {
		colors[i]->id = glGetUniformLocation(shaderProgramHandles[activeShaderProgram], colors[i]->name.c_str());
		if (colors[i]->id != -1) glUniform4fv(colors[i]->id, 1, &colors[i]->val[0]);
#ifdef _DEBUG
		//else ERROR("\n\tFailure in color setup loop.", false);
#endif

	}


} // ONLY will run once, no streaming updates.

//-------------------------------------------------------------------------//
// TRIANGLE MESH
//-------------------------------------------------------------------------//

bool TriMesh::readFromPly(const string &fileName, bool flipZ)
{
	FILE *f = openFileForReading(fileName);
	if (f == NULL) return false;
	string token, t;
	int numVertices = 0;
	int numFaces = 0;
	int numTriangles = 0;
	vector<int> faceIndices;

	// get num vertices
	while (getToken(f, token, "")) {
		//cout << token << endl;
		if (token == "vertex") break;
	}
	getToken(f, token, "");
	numVertices = atoi(token.c_str());

	// get vertex attributes
	while (getToken(f, token, "")) {
		if (token == "property") {
			getToken(f, t, ""); // token should be "float"
			getToken(f, t, ""); // attribute name
			attributes.push_back(t);
		}
		else if (token == "face") {
			getToken(f, t, "");
			numFaces = atoi(t.c_str());
			break;
		}
	}

	// read to end of header
	while (getToken(f, token, "")) {
		if (token == "end_header") break;
	}

	// get vertices
	float val;
	for (int i = 0; i < (int)(numVertices*attributes.size()); i++) {
		//getToken(f, token, "");
		//vertexData.push_back((float)atof(token.c_str()));
		fscanf(f, "%f", &val);
		vertexData.push_back(val);
	}

	// divide color values by 255, and flip normal directions if needed
	// This deals with issues related to exporting from Blender to ply
	// usin the y-axis as UP and the z-axis as FRONT.
	for (int i = 0; i < (int)attributes.size(); i++) {
		if (attributes[i] == "red" || attributes[i] == "green" || attributes[i] == "blue") {
			for (int j = 0; j < numVertices; j++) {
				vertexData[i + j*attributes.size()] /= 255.0f;
			}
		}
		else if (flipZ && (attributes[i] == "z" || attributes[i] == "nz")) {
			for (int j = 0; j < numVertices; j++) {
				vertexData[i + j*attributes.size()] *= -1.0f;
			}
		}
	}

	// get faces
	int idx;
	for (int i = 0; i < numFaces; i++) {
		faceIndices.clear();
		getToken(f, token, "");
		int numVerts = atoi(token.c_str());
		for (int j = 0; j < numVerts; j++) { // get all vertices in face
			//getToken(f, t, "");
			//faceIndices.push_back(atoi(t.c_str()));
			fscanf(f, "%d", &idx);
			faceIndices.push_back(idx);
		}
		for (int j = 2; j < numVerts; j++) { // make triangle fan
			indices.push_back(faceIndices[0]);
			indices.push_back(faceIndices[j - 1]);
			indices.push_back(faceIndices[j]);
			numTriangles++;
		}
	}
	numIndices = (int)indices.size();

	//printf("vertices:%d, triangles:%d, attributes:%d\n",
	//	vertexData.size()/attributes.size(),
	//	indices.size()/3,
	//	attributes.size());

	fclose(f);
	return true;
}
#define V_POSITION 0
#define V_NORMAL 1
#define V_ST 2
#define V_COLOR 3
int NUM_COMPONENTS[] = { 3, 3, 2, 3 };
bool TriMesh::sendToOpenGL(void)
{
	// Create vertex array object.  The vertex array object
	// holds the structure of how the vertices are stored. VAOs
	// also are bound for rendering.
	//
	glGenVertexArrays(1, &vao); // generate 1 array
	glBindVertexArray(vao);

	// Make and bind the vertex buffer object.  The vbo
	// holds the raw data that will be indexed by the vao.
	//
	GLuint vbo; // vertex buffer object
	glGenBuffers(1, &vbo); // generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size()*sizeof(float), &vertexData[0], GL_STATIC_DRAW);

	// At this point, we have to tell the vertex array what kind
	// of data it holds, and where it is located in the vertex buffer.
	// The code here uses the first field of four possible data types
	// (position, normal, textureCoordinate, color)
	//
	int stride = (int)attributes.size() * sizeof(float); // size of a vertex in bytes

	for (int i = 0; i < (int)attributes.size(); i++) {
		int bindIndex = -1;
		//int numComponents = 0;

		if (attributes[i] == "x") bindIndex = V_POSITION;
		else if (attributes[i] == "nx") bindIndex = V_NORMAL;
		else if (attributes[i] == "s") bindIndex = V_ST;
		else if (attributes[i] == "red") bindIndex = V_COLOR;

		if (bindIndex >= 0) {
			//printf("bindIndex = %d\n", bindIndex);
			glEnableVertexAttribArray(bindIndex);
			glVertexAttribPointer(bindIndex, NUM_COMPONENTS[bindIndex],
				GL_FLOAT, GL_FALSE, stride, (void*)(i * sizeof(float)));
		}
	}

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// Generate the index buffer
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int),
		&indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // unbind

	glDeleteBuffers(1, &vbo);

	return true;
}
void TriMesh::draw(void)
{
	glBindVertexArray(vao); // bind the vertices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); // bind the indices

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw the triangles.  modes: GL_TRIANGLES, GL_LINES, GL_POINTS
	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, (void*)0);

	glDisable(GL_BLEND);
}

//-------------------------------------------------------------------------//

void Sprite::prepareToDraw(const Camera& camera, Transform& T, Material& material) 
{
	Drawable::prepareToDraw(camera, T, material);

	//Always face the direction the camera is rotated to look at, i.e. norm(eye-center).
	glm::vec3 vd = glm::vec3(camera.eye - camera.center);
	vd = glm::normalize(vd);
	float yRot = atan2f(vd.x, vd.z);
	T.rotation = glm::quat(cos(yRot*0.5f), glm::vec3(0, 1, 0)*sin(yRot*0.5f));
	float xRot = -asin(vd.y);
	T.rotation *= glm::quat(cos(xRot*0.5f), glm::vec3(1, 0, 0)*sin(xRot*0.5f));

	//Update the current frame.
	if (currAccumulatedTime >= animRate) { //Specify animRate in FPS, roughly.
		activeFrame += animDir; //Could be +1 or -1.
		if (activeFrame < 0) activeFrame = frames.size() - 1;
		else if (activeFrame > frames.size() - 1) activeFrame = 0;
		currAccumulatedTime = 0;
	}
	else currAccumulatedTime += FIXED_DT;

	//Set the new sprite frame in the shader.
	glUseProgram(material.shaderProgramHandles[material.activeShaderProgram]);
	GLint loc = glGetUniformLocation(material.shaderProgramHandles[material.activeShaderProgram], "uSpriteFrame"); //a vec4 (x,y,z,w) <-> (x,y,w,h).
	if (loc != -1) glUniform4fv(loc, 1, glm::value_ptr(frames[activeFrame]));
#ifdef _DEBUG
	else ERROR("Could not load uniform uSpriteFrame.", false);
#endif
	glUseProgram(0);
}
void Billboard::prepareToDraw(const Camera &camera, Transform& T, Material& material)
{
	Drawable::prepareToDraw(camera, T, material);

	glm::vec3 vd = glm::vec3((material.name == "allAxes") ? camera.eye - T.translation : camera.eye - camera.center);
	vd = glm::normalize(vd);
	float yRot = atan2f(vd.x, vd.z);
	T.rotation = glm::quat(cos(yRot*0.5f), glm::vec3(0, 1, 0)*sin(yRot*0.5f));
	if (material.name == "allAxes") {
		float xRot = -asin(vd.y);
		T.rotation *= glm::quat(cos(xRot*0.5f), glm::vec3(1, 0, 0)*sin(xRot*0.5f));
	}
}

void Drawable::prepareToDraw(const Camera &camera, Transform& T, Material& material) {
	if (diffuseTexture == nullptr) return;
	//Handle setting the diffuse texture uniform, if there is one. Assumes uniform name is a sampler2D named uDiffuseTex.
	glUseProgram(material.shaderProgramHandles[material.activeShaderProgram]);
	diffuseTexture->id = glGetUniformLocation(material.shaderProgramHandles[material.activeShaderProgram], "uDiffuseTex");
	if (diffuseTexture->id != -1) {
		glActiveTexture(GL_TEXTURE0 + 0); //Set active texture unit in GL context.
		glUniform1i(diffuseTexture->id, 0); //Set the handle to the texture being used? Matches the # added to GL_TEXTURE0 above.
		glBindTexture(GL_TEXTURE_2D, diffuseTexture->textureId); //Associate texture and GL target. 
		glBindSampler(diffuseTexture->textureId, diffuseTexture->samplerId); //Associate texture and sampler. Already done in sendToOpenGL().
	}
#ifdef _DEBUG
	else ERROR("Could not load uniform uDiffuseTex.", false);
#endif
	glUseProgram(0);
}
void Drawable::draw(Camera &camera) 
{
	glUseProgram(material->shaderProgramHandles[material->activeShaderProgram]);

	material->bindMaterial();

	triMesh->draw();

	glUseProgram(0);
}

//-------------------------------------------------------------------------//

GLuint gLightsUBO = NULL_HANDLE;
int gNumLights = 0;
Light gLights[MAX_LIGHTS];
void initLightBuffer() {
	if (gLightsUBO != NULL_HANDLE) return;
	glGenBuffers(1, &gLightsUBO); // Generate a buffer that will send the lights to OpenGL, shared between shaders.

	glBindBuffer(GL_UNIFORM_BUFFER, gLightsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Light)* MAX_LIGHTS, gLights, GL_STREAM_DRAW); //Unlike glBufferSubData(), actually allocates data!
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void Light::typeToString() {
	switch (type)
	{
		case Light::LIGHT_TYPE::POINT: cout << "point light\n"; break;
		case Light::LIGHT_TYPE::DIRECTIONAL: cout << "directional light\n"; break;
		case Light::LIGHT_TYPE::SPOT_LIGHT: cout << "spot light\n"; break;
		case Light::LIGHT_TYPE::AMBIENT: cout << "ambient light\n"; break;
		case Light::LIGHT_TYPE::HEAD_LIGHT: cout << "head light\n"; break;
		case Light::LIGHT_TYPE::RIM_LIGHT: cout << "rim light\n"; break;
	}
}

//-------------------------------------------------------------------------//

SceneGraphNode::SceneGraphNode(void) {
	T.scale = glm::vec3(1, 1, 1);
	T.translation = glm::vec3(0, 0, 0);
	T.rotation = glm::quat(T.translation); //Lookup over an allocation.
	activeLOD = 0;
	isRendered = true;
}
SceneGraphNode::~SceneGraphNode(void) {
	for (auto it = LODstack.begin(); it != LODstack.end(); ++it) delete *it; 
	//for (auto it = cameras.begin(); it != cameras.end(); ++it) delete *it; //Handled by gCameras.
	for (auto it = sounds.begin(); it != sounds.end(); ++it) if (*it != nullptr) (*it)->drop();
	for (auto it = scripts.begin(); it != scripts.end(); ++it) delete *it;
	if (collider != nullptr) delete collider;
}
void SceneGraphNode::setTranslation(const glm::vec3 &t) {
	T.translation = t;
	for (int i = 0; i < (int)cameras.size(); ++i) {
		cameras[i]->center -= cameras[i]->eye; //Tmp storing this dist in center.
		cameras[i]->eye = t; //However, center needs to still be eye-center away from eye.
		cameras[i]->center = t + cameras[i]->center; //Should preserve eye and center, both translated to the new t.
	}
}
void SceneGraphNode::update(Camera &camera, double dt) 
{
	//Update transform for self, if there is no parent to update us for ourselves.
	if (parent == nullptr) T.refreshTransform();

	//Update collider position to match current translation.
	if (collider != nullptr) collider->center = T.translation + collider->offset;

	//Update children.
	for (int i = 0; i < (int)children.size(); ++i) {
		children[i]->update(camera, dt); //Won't refresh self thanks to above line.
		if (children[i]->activeLOD != -1 && children[i]->LODstack[children[i]->activeLOD]->type != Drawable::TRIMESHINSTANCE)
			children[i]->T.refreshTransform(T.transform, T.translation, T.scale, false);
		else children[i]->T.refreshTransform(T.transform);
	}

	//Update LOD stack. Reverse iter due to switchingDistances[0] == distance from cam at which we stop rendering the object.
	if (LODstack.size() != 0) {
		int currLOD = 0;
		glm::vec3 camDistVec = T.translation - camera.eye;
		float camDistSqr = camDistVec.x*camDistVec.x + camDistVec.y*camDistVec.y + camDistVec.z*camDistVec.z;
		if (camDistSqr > switchingDistances[0] * switchingDistances[0]) activeLOD = -1; //Outside all thresholds.
		else for (auto it = switchingDistances.rbegin(); it != switchingDistances.rend(); ++it) {
			if (camDistSqr <= (*it)*(*it)) {
				activeLOD = currLOD;
				break;
			}
			currLOD++;
		} //So the first element of LODstack is the one viewed when closest up, see sprint2b.scene.
	}

	//Run any scripts attached to the node.
	for (int i = 0; i < (int)scripts.size(); ++i) if (scripts[i]->active) scripts[i]->update(camera, dt);
}
void SceneGraphNode::draw(Camera &camera) {

	if (LODstack.size() == 0) return;

	//printMat(transform);
	if (!isRendered || activeLOD == -1) return; //Do not render objects beyond their renderThreshold of switchingDistances[0].
	LODstack[activeLOD]->prepareToDraw(camera, T, *LODstack[activeLOD]->material);

	glUseProgram(this->LODstack[activeLOD]->material->shaderProgramHandles[LODstack[activeLOD]->material->activeShaderProgram]);

	GLint loc;

	loc = glGetUniformLocation(LODstack[activeLOD]->material->shaderProgramHandles[LODstack[activeLOD]->material->activeShaderProgram], "uObjectWorldM");
	if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(T.transform));
#ifdef _DEBUG
	//else ERROR("Could not load uniform uObjectWorldM.", false);
#endif

	loc = glGetUniformLocation(LODstack[activeLOD]->material->shaderProgramHandles[LODstack[activeLOD]->material->activeShaderProgram], "uObjectWorldInverseM");
	if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(T.invTransform));
#ifdef _DEBUG
	//else ERROR("Could not load uniform uObjectWorldInverseM.", false);
#endif

	glm::mat4x4 objectWorldViewPerspect = camera.worldViewProject * T.transform;
	loc = glGetUniformLocation(LODstack[activeLOD]->material->shaderProgramHandles[LODstack[activeLOD]->material->activeShaderProgram], "uObjectPerpsectM");
	if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(objectWorldViewPerspect));
#ifdef _DEBUG
	//else ERROR("Could not load uniform uObjectPerpsectM.", false);
#endif

	loc = glGetUniformLocation(LODstack[activeLOD]->material->shaderProgramHandles[LODstack[activeLOD]->material->activeShaderProgram], "uViewDirection");
	if (loc != -1) glUniform4fv(loc, 1, glm::value_ptr(camera.center));
#ifdef _DEBUG
	//else ERROR("Could not load uniform uViewPosition.", false);
#endif

	loc = glGetUniformLocation(LODstack[activeLOD]->material->shaderProgramHandles[LODstack[activeLOD]->material->activeShaderProgram], "uViewPosition");
	if (loc != -1) glUniform4fv(loc, 1, glm::value_ptr(camera.eye));
#ifdef _DEBUG
	//else ERROR("Could not load uniform uViewDirection.", false);
#endif

	LODstack[activeLOD]->draw(camera);
	if (collider != nullptr && collider->isRendered) collider->meshInstance->draw(camera);
}

//-------------------------------------------

const char* addTabs(const int amt) { 
	char *t = new char[amt+1];
	for (int i = 0; i < amt; ++i) t[i] = '\t'; 
	t[amt] = '\0';
	return t; 
}
void Camera::toSDL(FILE *F, int tabAmt) {
	/*
	camera name "camera1" {
		eye [0 6 10]
		center [0 0 1]
		vup [0 1 0]
		fovy 0.5
		znear 0.1
		zfar 1000
	}
	*/
	const char* t = addTabs(tabAmt);
	fprintf(F, "%scamera name \"%s\" {\n", t, name.c_str());
	fprintf(F, "\t%seye [%f %f %f]\n", t, eye.x, eye.y, eye.z);
	fprintf(F, "\t%scenter [%f %f %f]\n", t, center.x, center.y, center.z);
	fprintf(F, "\t%svup [%f %f %f]\n", t, vup.x, vup.y, vup.z);
	fprintf(F, "\t%sfovy %f\n", t, fovy);
	fprintf(F, "\t%sznear %f\n", t, znear);
	fprintf(F, "\t%szfar %f\n", t, zfar);
	fprintf(F, "%s}\n", t);
}
void TriMesh::toSDL(FILE *F) {
	/*
	mesh name "monkeyMesh" {
		file "monkeyTex.ply"
	}
	*/
	fprintf(F, "mesh name \"%s\" {\n", name.c_str());
	fprintf(F, "\tfile \"%s\"\n", filename.c_str());
	fprintf(F, "}\n");
}
void Light::toSDL(FILE *F) {
	/*
	light {
		isOn 1
		type "point"
		position [2 0 2]
		intensity [0.5 0.5 0.5]
		attenuation [1 1 1]
	}
	light {
		isOn 1
		type "directional"
		direction [0 1 0]
		intensity [1 1 1]
		attenuation [1 1 1]
	}
	light {
		isOn 1
		type "spot"
		position [2 2 2]
		direction [1 1 1]
		intensity [0 0 0] //colours can and should be outside 0.0 to 1.0!
		attenuation [0.5 1 1] //F_att value 0 means full light?
		alpha 0.1 //used to end fade?
		theta 0.1 //used to start fade?
	}
	*/
	fprintf(F, "light {\n");
	fprintf(F, "\tisOn %i\n", isOn);
	switch (type) {
	case LIGHT_TYPE::POINT:
			fprintf(F, "\ttype \"point\"\n");
			fprintf(F, "\tposition [%f %f %f]\n", position.x, position.y, position.z);
			fprintf(F, "\tintensity [%f %f %f]\n", intensity.x, intensity.y, intensity.z);
			fprintf(F, "\tattenuation [%f %f %f]\n", attenuation.x, attenuation.y, attenuation.z);
			break;
		case LIGHT_TYPE::DIRECTIONAL:
			fprintf(F, "\ttype \"directional\"\n");
			fprintf(F, "\tdirection [%f %f %f]\n", direction.x, direction.y, direction.z);
			fprintf(F, "\tintensity [%f %f %f]\n", intensity.x, intensity.y, intensity.z);
			fprintf(F, "\tattenuation [%f %f %f]\n", attenuation.x, attenuation.y, attenuation.z);
			break;
		case LIGHT_TYPE::SPOT_LIGHT:
			fprintf(F, "\ttype \"spot\"\n");
			fprintf(F, "\tposition [%f %f %f]\n", position.x, position.y, position.z);
			fprintf(F, "\tdirection [%f %f %f]\n", direction.x, direction.y, direction.z);
			fprintf(F, "\tintensity [%f %f %f]\n", intensity.x, intensity.y, intensity.z);
			fprintf(F, "\tattenuation [%f %f %f]\n", attenuation.x, attenuation.y, attenuation.z);
			fprintf(F, "\talpha %f\n", alpha);
			fprintf(F, "\ttheta %f\n", theta);
			break;
	}
	fprintf(F, "}\n");
}
void Material::toSDL(FILE *F) {
	/*
	material name "phongShader" {
		vertexShader "basicVertexShader.vs"
		fragmentShader "phongShading.fs"
		texture uDiffuseTex "hex.png"
		color uDiffuseColor [2 2 2 1]
		color uSpecularColor [0.2 0.2 0.2 3]
		color uAmbientIntensity [0.2 0.2 0.2 1]
	}
	*/
	fprintf(F, "material name \"%s\" {\n", name.c_str());
	fprintf(F, "\tvertexShader \"%s\"\n", vertexShaderName.c_str());
	fprintf(F, "\tfragmentShader \"%s\"\n", fragmentShaderName.c_str());
	for (int i = 0; i < colors.size(); ++i) fprintf(F, "\tcolor %s [%f %f %f]\n", colors[i]->name.c_str(), colors[i]->val.r, colors[i]->val.g, colors[i]->val.b);
	for (int i = 0; i < textures.size(); ++i) fprintf(F, "\ttexture %s \"%s\"\n", textures[i]->name.c_str(), textures[i]->fileName.c_str());
	fprintf(F, "}\n");
}
void Sprite::toSDL(FILE *F, int tabAmt) {
	/*
	sprite {
		image uDiffuseTex "spriteSheet.png"
		animDir -1
		animRate 1
		frameWidth 480
		frameHeight 600
	}
	*/
	const char* t = addTabs(tabAmt);
	fprintf(F, "%ssprite {\n", t);
	if (diffuseTexture != nullptr) fprintf(F, "\t%simage \"%s\"\n", t, diffuseTexture->fileName.c_str());
	if (material->name != "sprite") fprintf(F, "\t%ssprite \"%s\"\n", t, material->name.c_str());
	fprintf(F, "\t%sanimDir %i\n", t, animDir);
	fprintf(F, "\t%sanimRate %f\n", t, animRate);
	fprintf(F, "\t%sframeWidth %i\n", t, frameWidth);
	fprintf(F, "\t%sframeHeight %i\n", t, frameHeight);
	fprintf(F, "%s}\n", t);
}
void Billboard::toSDL(FILE *F, int tabAmt) {
	/*
	billboard {
		material "allAxes"
		image uDiffuseTex "uNormalMap.png"
	}
	*/
	const char* t = addTabs(tabAmt);
	fprintf(F, "%sbillboard {\n", t);
	fprintf(F, "\t%smaterial \"%s\"\n", t, material->name.c_str());
	if (diffuseTexture != nullptr) fprintf(F, "\t%simage \"%s\"\n", t, diffuseTexture->fileName.c_str());
	fprintf(F, "%s}\n", t);
}
void TriMeshInstance::toSDL(FILE *F, int tabAmt) {
	/*
	meshInstance {
		mesh "cubeCenter"
		material "cubeMat"
	}
	*/
	const char* t = addTabs(tabAmt);
	fprintf(F, "%smeshInstance {\n", t);
	fprintf(F, "\t%smesh \"%s\"\n", t, triMesh->name.c_str());
	fprintf(F, "\t%smaterial \"%s\"\n", t, material->name.c_str());
	if (diffuseTexture != nullptr) fprintf(F, "\t%simage \"%s\"\n", t, diffuseTexture->fileName.c_str());
	fprintf(F, "%s}\n", t);
}
void SceneGraphNode::toSDL(FILE *F, int tabAmt) {
	/*
	node name "meshParent oNode" {	
		sound "beek-blue_slide.it"
		camera name "camera3" {
			eye [0 6 10]
			center [0 0 1]
			vup [0 1 0]
			fovy 0.5
			znear 0.1
			zfar 1000
		}
		meshInstance {
			mesh "oMesh"
			material "phongLetters"
		}
		node name "monkeyNode" {
			meshInstance
			{
				mesh "monkeyMesh"
				material "phongShader"
			}
			translation [0 0 0]
			scale [1 1 1]	
		}
		node name "monkeyNode2" {
			meshInstance
			{
				mesh "monkeyMesh"
				material "phongShader"
			}
			translation [0 2 0]
			scale [1 1 1]	
		}
		translation [-2 0 0]
		scale [0.5 0.5 0.5]
	}
	*/
	const char* t = addTabs(tabAmt);
	fprintf(F, "%snode name \"%s\" {\n", t, name.c_str());
	for (int i = 0; i < sounds.size(); ++i)  {
		if (sounds[i] != nullptr && sounds[i]->getSoundSource() != 0) fprintf(F, "\t%ssound \"%s\"\n", t, sounds[i]->getSoundSource()->getName());
#ifdef _DEBUG
		else ERROR("\tWarning: no sound was found or getSoundSource() returned 0.");
#endif
	}
	for (int i = 0; i < cameras.size(); ++i) cameras[i]->toSDL(F, tabAmt + 1);
	for (int i = 0; i < LODstack.size(); ++i) LODstack[i]->toSDL(F, tabAmt + 1);
	for (int i = 0; i < children.size(); ++i) children[i]->toSDL(F, tabAmt + 1);
	for (int i = 0; i < scripts.size(); ++i) scripts[i]->toSDL(F, addTabs(tabAmt + 1));
	fprintf(F, "\t%sisRendered %d\n", t, isRendered);
	if (switchingDistances.size() > 0) fprintf(F, "\t%smaxRenderDist %f\n", t, switchingDistances[0]);
	fprintf(F, "\t%stranslation [%f %f %f]\n", t, T.translation.x, T.translation.y, T.translation.z);
	fprintf(F, "\t%srotation [%f %f %f]\n", t, T.rotation.x, T.rotation.y, T.rotation.z);
	fprintf(F, "\t%sscale [%f %f %f]\n", t, T.scale.x, T.scale.y, T.scale.z);
	fprintf(F, "%s}\n", t);

}

//-------------------------

//RGBAImage * textTex;
//GLuint textVboID;
//GLuint textUVboID;
//GLuint textShaderProgramHandle;
//GLuint fontTexNumRows, fontTexNumCols;

/*
//Call with a path to a font texture.
void initText2D(const char * texturePath, int numTexRows, int numTexCols) {

	// Save arguments.
	fontTexNumRows = numTexRows;
	fontTexNumCols = numTexCols;

	// Initialize texture
	textTex = new RGBAImage();
	textTex->loadPNG(texturePath);

	// Initialize VBO
	glGenBuffers(1, &textVboID);
	glGenBuffers(1, &textUVboID);

	// Initialize Shader
	textShaderProgramHandle = createShaderProgram(loadShader("fontShader.vs", GL_VERTEX_SHADER), loadShader("fontShader.fs", GL_FRAGMENT_SHADER));

	// Initialize texture uniform ID, the sampler, the texture.
	textTex->id = glGetUniformLocation(textShaderProgramHandle, "uFontTex");
	
	glGenSamplers(1, &textTex->samplerId);
	glGenTextures(1, &textTex->textureId);
	

	// Bind shader
	glUseProgram(textShaderProgramHandle);

	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	// Set our "myTextureSampler" sampler to user Texture Unit 0
	glUniform1i(textTex->id, 0);
	glBindTexture(GL_TEXTURE_2D, textTex->textureId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textTex->width, textTex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textTex->pixels[0]);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindSampler(textTex->textureId, textTex->samplerId); //Associate texture and sampler. Already done in sendToOpenGL().
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glUseProgram(0);
}

void printText2D(const char * text, int x, int y, int size) {

	if (fontTexNumCols <= 0) return;

	unsigned int length = strlen(text);

	// Fill buffers
	vector<glm::vec2> vertices;
	vector<glm::vec2> UVs;
	for (unsigned int i = 0; i<length; i++){

		//Calculate the vertices, the regions on the screen the letters appear at.
		glm::vec2 vertex_up_left = glm::vec2(x + i*size, y + size);
		glm::vec2 vertex_up_right = glm::vec2(x + i*size + size, y + size);
		glm::vec2 vertex_down_right = glm::vec2(x + i*size + size, y);
		glm::vec2 vertex_down_left = glm::vec2(x + i*size, y);

		vertices.push_back(vertex_up_left); //(x,h)
		vertices.push_back(vertex_down_left); //(x,y)
		vertices.push_back(vertex_up_right); //(w,h)
		vertices.push_back(vertex_down_right); //(w,y)
		vertices.push_back(vertex_up_right); //z==x, as 2D
		vertices.push_back(vertex_down_left); //z===y, as 2D

		//Now the UVs, the portions of the font texture those letters correspond to.
		//It relies on this ASCII division trick.
		char character = text[i]; //84 for T.
		float uv_x = (character % fontTexNumCols) / (float)fontTexNumCols; //4/16 = .25. for T.
		float uv_y = (character / fontTexNumRows) / (float)fontTexNumRows; //84/16/16 = .328125 for T.

		glm::vec2 uv_up_left = glm::vec2(uv_x, uv_y);
		glm::vec2 uv_up_right = glm::vec2(uv_x + 1.0f / (float)fontTexNumCols, uv_y);
		glm::vec2 uv_down_right = glm::vec2(uv_x + 1.0f / (float)fontTexNumCols, (uv_y + 1.0f / (float)fontTexNumRows));
		glm::vec2 uv_down_left = glm::vec2(uv_x, (uv_y + 1.0f / (float)fontTexNumRows));

		UVs.push_back(uv_up_left);
		UVs.push_back(uv_down_left);
		UVs.push_back(uv_up_right);
		UVs.push_back(uv_down_right);
		UVs.push_back(uv_up_right);
		UVs.push_back(uv_down_left);
	}
	glBindBuffer(GL_ARRAY_BUFFER, textVboID);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), &vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, textUVboID);
	glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs[0], GL_STATIC_DRAW);

	// Bind shader
	glUseProgram(textShaderProgramHandle);

	// Bind texture
	//glActiveTexture(GL_TEXTURE0);
	// Set our "myTextureSampler" sampler to user Texture Unit 0
	glUniform1i(textTex->id, 0);
	glBindTexture(GL_TEXTURE_2D, textTex->textureId);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, textVboID);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 2nd attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, textUVboID);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw call
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());

	glDisable(GL_BLEND);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glUseProgram(0);
}

void cleanupText2D() {

	if (textVboID == 0) return;

	// Delete buffers
	glDeleteBuffers(1, &textVboID);
	glDeleteBuffers(1, &textUVboID);

	// Delete texture
	glDeleteTextures(1, &textTex->id);

	// Delete shader
	glDeleteProgram(textShaderProgramHandle);
}
*/