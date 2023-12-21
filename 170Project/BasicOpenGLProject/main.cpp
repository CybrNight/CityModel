#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>
#include <iostream>

// fStream - STD File I/O Library
#include <fstream>

#include <iostream>
#include "shader.h"
#include "shaderprogram.h"
#include "mesh.h"
#include "transform.h"
#include "camera.h"
#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "spline.h"


// OBJ_Loader - .obj Loader
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/*=================================================================================================
	DOMAIN
=================================================================================================*/

// Window dimensions
const int InitWindowWidth  = 1280;
const int InitWindowHeight = 720;
int WindowWidth  = InitWindowWidth;
int WindowHeight = InitWindowHeight;

// Last mouse cursor position
float LastMousePosX = WindowWidth/2;
float LastMousePosY = WindowHeight/2;

// Arrays that track which keys are currently pressed
bool key_states[256];
bool key_special_states[256]; 
bool mouse_states[8];
bool moveKeys[4];
bool freeCam = true;
bool doAnimate = false;

// Other parameters
bool drawWireframe = false;
bool smoothShading = true;
bool showNormals = false;
bool funMode = false;

/*=================================================================================================
	SHADERS & TRANSFORMATIONS
=================================================================================================*/

ShaderProgram PassthroughShader;
ShaderProgram PerspectiveShader;
ShaderProgram SmoothShader;
ShaderProgram FlatShader;

glm::mat4 PerspProjectionMatrix( 1.0f );
glm::mat4 PerspModelMatrix( 1.0f );

glm::vec3 camPosition;

float perspZoom = 1.0f, perspSensitivity = 0.35f;
float perspRotationX = 0.0f, perspRotationY = 0.0f;

/*=================================================================================================
	OBJECTS
=================================================================================================*/

GLuint axis_VAO;
GLuint axis_VBO[2];

GLuint modelVAO;
GLuint modelVBO[4];

Camera camera;

std::vector<Transform> gObjects;
std::vector<Model> models;
std::vector<Transform*> cars1;
std::vector<Transform*> cars2;

float carStartX = -1.75f;
float carStartY = 10.0f;

GLuint modelTexId;

#define PI 3.14159265358979323846f

int texIndex = 0;
float texRepeatX = 0.5;
float texRepeatY = 2;

float normalLength = 0.05f;

Assimp::Importer importer;

double bX = 0.0;
double bY = 0.0;
double bZ = 0.0;

int indexI = 0;
int indexJ = 1;
double indexU = 0.0;

int splineNum;
Spline* splinesArr;


float distance(glm::vec3 pos1, glm::vec3 pos2) {
	float x = pow(pos1.x - pos2.x, 2);
	float y = pow(pos1.y - pos2.y, 2);
	float z = pow(pos1.z - pos2.z, 2);
	return sqrt(x + y + z);
}

float distance(Transform trans1, Transform trans2) {
	return distance(trans1.position, trans2.position);
}

glm::vec3 catMullRom(double u, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4) {
	double x, y, z;
	x = y = z = 0;

	double s = 0.5;
	double uCube = u * u * u;
	double uSqr = u * u;

	double coefficientAndBasis[4] =
	{ (uCube * -s) + (uSqr * 2 * s) + (u * -s) + (0),
	  (uCube * (2 - s)) + (uSqr * (s - 3)) + (0) + (1),
	  (uCube * (s - 2)) + (uSqr * (3 - 2 * s)) + (u * s) + (0),
	  (uCube * s) + (uSqr * -s) + (0) + (0) };

	double cb1 = coefficientAndBasis[0];
	double cb2 = coefficientAndBasis[1];
	double cb3 = coefficientAndBasis[2];
	double cb4 = coefficientAndBasis[3];

	double pOfU[3] =
	{ (cb1 * p1.x) + (cb2 * p2.x) + (cb3 * p3.x) + (cb4 * p4.x),
	  (cb1 * p1.y) + (cb2 * p2.y) + (cb3 * p3.y) + (cb4 * p4.y),
	  (cb1 * p1.z) + (cb2 * p2.z) + (cb3 * p3.z) + (cb4 * p4.z) };

	return glm::vec3(pOfU[0], pOfU[1], pOfU[2]);
}

int loadSplines(char* argv) {
	char* cName = new char[128];
	FILE* fileList;
	FILE* fileSpline;
	int iType, i = 0, j, iLength;

	/* load the routes file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf("can't open file\n");
		exit(1);
	}

	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &splineNum);

	splinesArr = new Spline[splineNum];

	/* reads through the spline files */
	for (j = 0; j < splineNum; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		std::cout << cName << std::endl;
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);
		std::cout << iLength << '\n';

		/* allocate memory for all the points */
		splinesArr[j].points = new glm::vec3[iLength];
		splinesArr[j].numControlPoints = iLength;

		/* saves the data to the struct */
		double x, y, z;
		while (fscanf(fileSpline, "%lf %lf %lf",
			&x,
			&y,
			&z) != EOF) {
			splinesArr[j].points[i] = glm::vec3(x * 20, y*2.5f, z * 20);
			i++;
		}

	}

	delete[] cName;

	return 0;
}

void tangent(double u, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4) {
	glm::vec3 crPoint = catMullRom(u, p1, p2, p3, p4);

	camera.position = glm::vec3(crPoint.x + 1.0, crPoint.y, crPoint.z);

	glm::vec3 crPointDeriv;
	glm::vec3 tanVec;
	glm::vec3 bVec;

	double s = 0.5;
	double uCubedDeriv = 3 * u * u;
	double uSquaredDeriv = u * 2;

	double coefficientAndBasisDeriv[4] =
	{ (uCubedDeriv * -s) + (uSquaredDeriv * 2 * s) + (1 * -s) + (0),
	  (uCubedDeriv * (2 - s)) + (uSquaredDeriv * (s - 3)) + (0) + (0),
	  (uCubedDeriv * (s - 2)) + (uSquaredDeriv * (3 - 2 * s)) + (1 * s) + (0),
	  (uCubedDeriv * s) + (uSquaredDeriv * -s) + (0) + (0) };

	double cb1D = coefficientAndBasisDeriv[0];
	double cb2D = coefficientAndBasisDeriv[1];
	double cb3D = coefficientAndBasisDeriv[2];
	double cb4D = coefficientAndBasisDeriv[3];

	double pOfUDeriv[3] =
	{ (cb1D * p1.x) + (cb2D * p2.x) + (cb3D * p3.x) + (cb4D * p4.x),
	  (cb1D * p1.y) + (cb2D * p2.y) + (cb3D * p3.y) + (cb4D * p4.y),
	  (cb1D * p1.z) + (cb2D * p2.z) + (cb3D * p3.z) + (cb4D * p4.z) };

	crPointDeriv = glm::vec3(pOfUDeriv[0], pOfUDeriv[1], pOfUDeriv[2]);

	double magnitude = sqrt((crPointDeriv.x * crPointDeriv.x) + (crPointDeriv.y * crPointDeriv.y) + (crPointDeriv.z * crPointDeriv.z));
	double tanX = crPointDeriv.x / magnitude;
	double tanY = crPointDeriv.y / magnitude;
	double tanZ = crPointDeriv.z / magnitude;
	tanVec = glm::vec3(tanX, tanY, tanZ);

	if (!freeCam)
		camera.front = glm::vec3(tanX, tanY, tanZ);

	if (u == 0.0)
	{
		glm::vec3 v = glm::vec3(1.0, 1.0, 1.0);

		//T x V
		//camera.up = glm::cross(tanVec, v);

		//T x N
		bX = (tanY * camera.up.z) - (tanZ * camera.up.y);
		bY = (tanZ * camera.up.x) - (tanX * camera.up.z);
		bZ = (tanX * camera.up.y) - (tanY * camera.up.x);
	}
	else
	{
		//B0 x T1
		//camera.up = glm::cross(glm::vec3(bX, bY, bZ), tanVec);
		
		//T1 x N1
		bX = (tanY * camera.up.z) - (tanZ * camera.up.y);
		bY = (tanZ * camera.up.x) - (tanX * camera.up.z);
		bZ = (tanX * camera.up.y) - (tanY * camera.up.x);
	}

	double upMagnitude = sqrt((camera.up.x * camera.up.x) + (camera.up.y * camera.up.y) + (camera.up.z * camera.up.z));
	//camera.up /= upMagnitude;

	double bMagnitude = sqrt((bX * bX) + (bY * bY) + (bZ * bZ));
	bX /= bMagnitude;
	bY /= bMagnitude;
	bZ /= bMagnitude;
}

float axis_vertices[] = {
	//x axis
	-1.0f,  0.0f,  0.0f, 1.0f,
	1.0f,  0.0f,  0.0f, 1.0f,
	//y axis
	0.0f, -1.0f,  0.0f, 1.0f,
	0.0f,  1.0f,  0.0f, 1.0f,
	//z axis
	0.0f,  0.0f, -1.0f, 1.0f,
	0.0f,  0.0f,  1.0f, 1.0f
};

float axis_colors[] = {
	//x axis
	1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
	//y axis
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	//z axis
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f
};

/*=================================================================================================
	HELPER FUNCTIONS
=================================================================================================*/

void window_to_scene( int wx, int wy, float& sx, float& sy )
{
	sx = ( 2.0f * (float)wx / WindowWidth ) - 1.0f;
	sy = 1.0f - ( 2.0f * (float)wy / WindowHeight );
}

/*=================================================================================================
	SHADERS
=================================================================================================*/

void CreateTransformationMatrices( void )
{
	// PROJECTION MATRIX
	PerspProjectionMatrix = glm::perspective<float>( glm::radians( 60.0f ), (float)WindowWidth / (float)WindowHeight, 0.01f, 1000.0f );

}

void CreateShaders( void )
{
	// Renders without any transformations
	//PassthroughShader.Create( "./shaders/simple.vert", "./shaders/simple.frag" );

	// Renders using perspective projection
	//PerspectiveShader.Create( "./shaders/texpersp.vert", "./shaders/texpersp.frag" );
	SmoothShader.Create("./shaders/texpersplight.vert", "./shaders/texpersplight.frag");
}

/*=================================================================================================
	BUFFERS
=================================================================================================*/

void CreateAxisBuffers( void )
{
	glGenVertexArrays( 1, &axis_VAO );
	glBindVertexArray( axis_VAO );

	glGenBuffers( 2, &axis_VBO[0] );

	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[0] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_vertices ), axis_vertices, GL_STATIC_DRAW );
	glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 );
	glEnableVertexAttribArray( 0 );

	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[1] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_colors ), axis_colors, GL_STATIC_DRAW );
	glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 );
	glEnableVertexAttribArray( 1 );
}

GLuint loadTexture(const char* filename, GLint wrapMode=GL_REPEAT) {
	int imgWidth, imgHeight, numColCh;
	unsigned char* tex;
	GLuint texId;
	GLint format = 0;
	tex = stbi_load(filename, &imgWidth, &imgHeight, &numColCh, 0);

	if (numColCh == 3) {
		format = GL_RGB;
	}
	else if (numColCh == 4) {
		format = GL_RGBA;
	}

	if (tex == nullptr) {
		std::cout << "Missing texture " << filename << "\n";
		tex = stbi_load("missingtex.bmp", &imgWidth, &imgHeight, &numColCh, 0);
		format = GL_RGB;
	}

	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, format, imgWidth, imgHeight, 0,
		format, GL_UNSIGNED_BYTE, tex);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texId;
}


float clamp(float val, float min, float max) {
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

glm::vec4 calcVert(float theta, float phi, float R, float r) {
	float x = (R + r * cos(theta)) * cos(phi);
	float y = (R + r * cos(theta)) * sin(phi);
	float z = r * sin(theta);

	return glm::vec4(x, y, z, 1);
}

/*=================================================================================================
	CALLBACKS
=================================================================================================*/

//-----------------------------------------------------------------------------
// CALLBACK DOCUMENTATION
// https://www.opengl.org/resources/libraries/glut/spec3/node45.html
// http://freeglut.sourceforge.net/docs/api.php#WindowCallback
//-----------------------------------------------------------------------------

int dir = 1;
bool reverse = false;
float cameraSpeed = 0.1f;

void idle_func()
{
	//uncomment below to repeatedly draw new frames
	glutPostRedisplay();

	if (moveKeys[0]) {
		camera.position += (cameraSpeed * camera.front);
	}
	if (moveKeys[1]) {
		camera.position -= (cameraSpeed * camera.right);
	}
	if (moveKeys[2]) {
		camera.position -= (cameraSpeed * camera.front);
	}
	if (moveKeys[3]) {
		camera.position += (cameraSpeed * camera.right);
	}

	if (funMode) {
		for (auto car = cars1.begin(); car != cars1.end(); car++) {
			(*car)->rotate(5, glm::vec3(1, 0, 1));
		}

		for (auto car = cars2.begin(); car != cars2.end(); car++) {
			(*car)->rotate(2.5f, glm::vec3(1, 0, 0));
		}

		gObjects[0].rotate(0.1f, glm::vec3(0, 1, 0));
	}

	for (auto car = cars1.begin(); car  != cars1.end(); car++) {
		(*car)->translate(0, 0, 0.5f);

		if ((*car)->position.z > gObjects[0].center.z + gObjects[0]._scale.z) {
			float z = gObjects[0].center.z - gObjects[0]._scale.z;
			(*car)->setPosition(carStartX, carStartY, z - 150);
		}
	}

	for (auto car = cars2.begin(); car != cars2.end(); car++) {
		(*car)->translate(0, 0, -0.5f);

		if ((*car)->position.z < gObjects[0].center.z - gObjects[0]._scale.z) {
			float z = gObjects[0].center.z + gObjects[0]._scale.z;
			(*car)->setPosition(carStartX+8, carStartY, z + 150);
		}
	}


	if (indexI < splineNum && doAnimate)
	{
		if (indexJ < splinesArr[indexI].numControlPoints - 2)
		{
			glm::vec3 p1 = splinesArr[indexI].points[indexJ - 1];
			glm::vec3 p2 = splinesArr[indexI].points[indexJ];
			glm::vec3 p3 = splinesArr[indexI].points[indexJ + 1];
			glm::vec3 p4 = splinesArr[indexI].points[indexJ + 2];

			if (indexU <= 1.0)
			{
				tangent(indexU, p1, p2, p3, p4);
				indexU += cameraSpeed/50;
			}
			else
			{
				indexU = 0.0;
				indexJ++;
			}
		}
		else
		{
			indexJ = 1;
		}
	}
}

void reshape_func( int width, int height )
{
	WindowWidth  = width;
	WindowHeight = height;

	glViewport( 0, 0, width, height );
	glutPostRedisplay();
}

void keyboard_func( unsigned char key, int x, int y )
{
	key_states[ key ] = true;

	switch( key )
	{
		case '`':
			drawWireframe = !drawWireframe;
			if (drawWireframe)
				printf("Wireframe on!\n");
			else
				printf("Wireframe off!\n");
			break;
		case 'w':
			moveKeys[0] = true;
			break;
		case 'a':
			moveKeys[1] = true;
			break;
		case 's':
			moveKeys[2] = true;
			break;
		case 'd':
			moveKeys[3] = true;
			break;
		case 'q':
			camera.up = camera.worldUp;
			doAnimate = !doAnimate;
			break;
		case '1':
			indexJ = 0;
			indexI = 0;
			break;
		case '2':
			indexJ = 0;
			indexI = 1;
			break;
		case '3':
			indexJ = 0;
			indexI = 2;
			break;
		case '9':
			funMode = !funMode;
			break;

		// Exit on escape key press
		case '\x1B':
		{
			exit( EXIT_SUCCESS );
			break;
		}
	}
}

void key_released( unsigned char key, int x, int y )
{
	key_states[ key ] = false;

	switch (key)
	{
		case 'w':
			moveKeys[0] = false;
			break;
		case 'a':
			moveKeys[1] = false;
			break;
		case 's':
			moveKeys[2] = false;
			break;
		case 'd':
			moveKeys[3] = false;
			break;

			// Exit on escape key press
		case '\x1B':
		{
			exit(EXIT_SUCCESS);
			break;
		}
	}
}

void key_special_pressed( int key, int x, int y )
{
	key_special_states[ key ] = true;
}

void key_special_released( int key, int x, int y )
{
	key_special_states[ key ] = false;
}

void mouse_func( int button, int state, int x, int y )
{
	// Key 0: left button
	// Key 1: middle button
	// Key 2: right button
	// Key 3: scroll up
	// Key 4: scroll down

	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( button == 3 )
	{
		cameraSpeed += 0.01f;
	}
	else if( button == 4 )
	{
		cameraSpeed -= 0.01f;
	}

	if (button == 0 && state == GLUT_UP) {
		camera.up = camera.worldUp;
		freeCam = false;
	}

	if (button == 0 && state == GLUT_DOWN) {
		freeCam = true;
	}

	cameraSpeed = clamp(cameraSpeed, 0.1f, 1);

	mouse_states[ button ] = ( state == GLUT_DOWN );
}

void passive_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	LastMousePosX = x;
	LastMousePosY = y;
}

bool firstMouse = true;
void active_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( firstMouse )
	{
		freeCam = true;
		LastMousePosX = (float)x;
		LastMousePosY = (float)y;
		firstMouse = false;
	}

	float xPos = static_cast<float>(x);
	float yPos = static_cast<float>(y);

	float xOffset = x - LastMousePosX;
	float yOffset = LastMousePosY - y;
	LastMousePosX = x;
	LastMousePosY = y;

		
	camera.processMouseMovement(xOffset, yOffset);
}

/*=================================================================================================
	RENDERING
=================================================================================================*/

void display_func( void )
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	CreateTransformationMatrices();

	SmoothShader.Use();
	SmoothShader.SetUniform("projectionMatrix", glm::value_ptr(PerspProjectionMatrix), 4, GL_FALSE, 1);
	SmoothShader.SetUniform("viewMatrix", glm::value_ptr(camera.getViewMatrix()), 4, GL_FALSE, 1);
	SmoothShader.SetUniform("modelMatrix", glm::value_ptr(PerspModelMatrix), 4, GL_FALSE, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, modelTexId);

	if( drawWireframe == true )
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	glBindVertexArray( axis_VAO );
	glDrawArrays( GL_LINES, 0, 6 );

	int oldTimeSinceStart = 0;

	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	int deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;

	for (auto i = gObjects.begin(); i != gObjects.end(); i++) {
		i->draw(SmoothShader);
	}

		
	glBindVertexArray( 0 );
	if( drawWireframe == true )
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glutSwapBuffers();
}

std::vector<Texture> loadMaterialTextures(std::string name, aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<Texture> textures;
	
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		Texture texture;
		texture.texId = loadTexture(("models/" + name + "/" + std::string(str.C_Str())).c_str());
		texture.type = typeName;
		texture.path = ("models/" + name + "/" + std::string(str.C_Str())).c_str();
		textures.push_back(texture);
	}

	if (textures.size() < 1) {
		textures.push_back(Texture(loadTexture("missingtex.bmp"), "name"));
	}
	
	return textures;
}

Mesh* loadMesh(std::string name, aiMesh* mesh, const aiScene* scene) {
	std::vector<Vertex> verts;
	std::vector<unsigned int> indices;
	std::vector<Texture> tex;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vert = Vertex();
		vert.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		vert.color = glm::vec4(1, 1, 1, 1);
		vert.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

		if (mesh->mTextureCoords[0]) {
			vert.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		}
		else{
			vert.texCoords = glm::vec2(0.0f, 0.0f);
		}
		verts.push_back(vert);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}


	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<Texture> diffuseMaps = loadMaterialTextures(name, material,
			aiTextureType_DIFFUSE, "texture_diffuse");
		tex.insert(tex.end(), diffuseMaps.begin(), diffuseMaps.end());
	}

	return new Mesh(verts, indices, tex);
}

void processNode(std::string name, Model& model, aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		model.meshes.push_back(loadMesh(name, mesh, scene));
	}

	//Process all child nodes recursively
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(name, model, node->mChildren[i], scene);
	}
}

void loadModel(std::string name, std::string file, int index=-1) {
	const aiScene* scene = importer.ReadFile("models/"+name+"/"+file, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords |
															 aiProcess_FixInfacingNormals | aiProcess_OptimizeMeshes | aiProcess_PreTransformVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	Model model;

		
	if (index == -1) {
		processNode(name, model, scene->mRootNode, scene);
		
	}

	models.push_back(model);
}



/*=================================================================================================
	INIT
=================================================================================================*/
void init( void )
{
	// Print some info
	std::cout << "Vendor:         " << glGetString( GL_VENDOR   ) << "\n";
	std::cout << "Renderer:       " << glGetString( GL_RENDERER ) << "\n";
	std::cout << "OpenGL Version: " << glGetString( GL_VERSION  ) << "\n";
	std::cout << "GLSL Version:   " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n\n";

	// Set OpenGL settings
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ); // background color
	glEnable( GL_DEPTH_TEST ); // enable depth test
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	glEnable( GL_CULL_FACE ); // enable back-face culling
	
	loadModel("city", "city.obj");
	loadModel("car", "free_car_001.fbx");

	gObjects.push_back(Transform(models[0]));

	gObjects[0].scale(150);

	int count = 64;
	for (int i = 0; i < count; i++) {
		gObjects.push_back(Transform(models[1]));
	}

	for (int i = 0; i < count/2; i++) {
		cars1.push_back(&gObjects[i + 1]);
		cars1[i]->setPosition(carStartX, carStartY, 20*i);
		cars1[i]->scale(5);
		std::cout << i << '\n';
	}

	for (int i = 0; i < count/2; i++) {
		cars2.push_back(&gObjects[i + ((count / 2) + 1)]);
		cars2[i]->setPosition(carStartX+8, carStartY, 20 * i);
		cars2[i]->rotate(180, glm::vec3(0, 1, 0));
		cars2[i]->scale(5);
		std::cout << i << '\n';
	}

	// Create shaders
	CreateShaders();

	// Create buffers
	CreateAxisBuffers();
	//createCubeBuffers();

	std::cout << "Finished initializing...\n\n";
}

/*=================================================================================================
	MAIN
=================================================================================================*/

int main( int argc, char** argv )
{
	glutInit( &argc, argv );

	glutInitWindowPosition( 100, 100 );
	glutInitWindowSize( InitWindowWidth, InitWindowHeight );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
	char path[100] = "C:\\Users\\naest\\Documents\\GitHub\\CSE170\\170Project\\BasicOpenGLProject\\routes.txt";


	glutCreateWindow( "Minecraft City Fly Through" );

	loadSplines(path);

	// Initialize GLEW
	GLenum ret = glewInit();
	if( ret != GLEW_OK ) {
		std::cerr << "GLEW initialization error." << std::endl;
		glewGetErrorString( ret );
		return -1;
	}

	glutDisplayFunc( display_func );
	glutIdleFunc( idle_func );
	glutReshapeFunc( reshape_func );
	glutKeyboardFunc( keyboard_func );
	glutKeyboardUpFunc( key_released );
	glutSpecialFunc( key_special_pressed );
	glutSpecialUpFunc( key_special_released );
	glutMouseFunc( mouse_func );
	glutMotionFunc( active_motion_func );
	glutPassiveMotionFunc( passive_motion_func );

	init();

	glutMainLoop();

	return EXIT_SUCCESS;
}
