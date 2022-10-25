// Assignment-5.cpp - smooth-shaded face

#include <glad.h>
#include <GLFW/glfw3.h>
#include "CameraArcball.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Mesh.h"
#include "VecMat.h"
#include "Widgets.h"
#include <vector>

// display parameters
GLFWwindow *w = NULL;
int winWidth = 800, winHeight = 800;
CameraAB camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -5), 30);

string dir("C:/Users/Jules/Code/G-Assns/");
string objName("Face.obj");

vector<vec3> points, normals;					// vertices, normals
vector<int3> triangles;							// triplets of vertex indices

// IDs for vertex buffer, shader program
GLuint VAO = 0, VBO = 0, EBO = 0, program = 0;

// lighting
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int nLights = sizeof(lights)/sizeof(vec3);

// interaction
void *picked = NULL;
Mover mover;

// Shaders

const char *vertexShader = R"(
	#version 410 core
	in vec3 point, normal;
	out vec3 vPoint, vNormal;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vNormal = normal;
	}
)";

const char *pixelShader = R"(
	#version 410 core
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform float amb = .1;				            // ambient
	uniform float dif = .7, spc = .5;	            // diffuse, specular
	uniform bool smoothNotFaceted = true;
	in vec3 vPoint, vNormal;
	uniform vec3 color = vec3(1, 1, 1);			    // default is white
	out vec4 pColor;
	void main() {
		vec3 N = normalize(vNormal);				// unit-length surface normal
		float intensity = 0;						// shade intensity
		for (int i = 0; i < nLights; i++) {
			vec3 L = normalize(lights[i]-vPoint);	// light vector
			float d = max(0, dot(N, L));
			vec3 E = normalize(vPoint);             // eye vector
			vec3 R = reflect(L, N);				    // reflection vector
			float h = max(0, dot(R, E));			// highlight term
			float s = pow(h, 100);				    // specular term
			intensity += dif*d+spc*s;               // accumulate
		}		
		intensity = min(intensity, 1);				// total intensity limited to 1
		pColor = vec4(intensity*color, 1);			// shade the color
	}
)";

// Keyboard Support

bool GetKey(int button) { return glfwGetKey(w, button) == GLFW_PRESS; }
bool ShiftKey() { return GetKey(GLFW_KEY_LEFT_SHIFT) || GetKey(GLFW_KEY_RIGHT_SHIFT); }
bool ControlKey() { return GetKey(GLFW_KEY_LEFT_CONTROL) || GetKey(GLFW_KEY_RIGHT_CONTROL); }

// Display

void Display(GLFWwindow *w) {
	// clear screen, enable z-buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// use shader program, set vertex feed for points and colors
	glEnable(GL_DEPTH_TEST);
	glUseProgram(program);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	int pSize = points.size()*sizeof(vec3);
	VertexAttribPointer(program, "normal", 3, 0, (void*)pSize);
	// update matrices
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// transform and update lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++) {
		vec4 xLight = camera.modelview*vec4(lights[i], 1);
		xLights[i] = vec3((float*) &xLight);
	}
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float *) xLights);
	// shade mesh
	glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, 0);
	// draw lights
	glDisable(GL_DEPTH_TEST);
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 8, .7f*vec3(1, .8f, 0), camera.fullview);
	if (!picked && !ShiftKey() && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
		camera.arcball.Draw(ControlKey());
	glFlush();
}

// Mouse Callbacks

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	int ix = (int) x, iy = (int) y;
	picked = NULL;
	if (action == GLFW_PRESS) {
		// test light selection
		float minDSq = 100;
		for (int i = 0; i < nLights; i++) {
			float dsq = ScreenDSq(ix, iy, lights[i], camera.fullview);
			if (dsq < minDSq) {
				minDSq = dsq;
				picked = &mover;
				mover.Down(&lights[i], ix, iy, camera.modelview, camera.persp);
			}
		}
		if (picked == NULL)
			camera.MouseDown(ix, iy, ShiftKey(), ControlKey());
	}
	if (action == GLFW_RELEASE)
		camera.MouseUp();
}

void MouseMove(GLFWwindow *w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		int ix = (int) x, iy = (int) y;
		if (picked == &mover)
			mover.Drag(ix, iy, camera.modelview, camera.persp);
		if (picked == NULL)
			camera.MouseDrag(ix, iy);
	}
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	camera.MouseWheel(spin, ShiftKey());
}

// Initialization

void BufferVertices() {
	// make GPU buffer for points and colors, make it active
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// allocate memory for vertex points, colors, and normals
	int nPoints = points.size(), sPoints = nPoints*sizeof(vec3), sNormals = sPoints;
	glBufferData(GL_ARRAY_BUFFER, sPoints+sNormals, NULL, GL_STATIC_DRAW);
	// load data
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points.data());
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sNormals, normals.data());
	// copy triangle data to GPU
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size()*sizeof(int3), triangles.data(), GL_STATIC_DRAW);
}

// Application

void Resize(GLFWwindow *window, int width, int height) {
	camera.Resize(winWidth = width, winHeight = height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	// enable anti-alias, init app window and GL context
	glfwInit();
#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_SAMPLES, 4);
	w = glfwCreateWindow(winWidth, winHeight, "Mesh smooth-shaded", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
#ifdef __APPLE__
	glfwGetFramebufferSize(w, &winWidth, &winHeight);
	glViewport(0, 0, winWidth, winHeight);
	camera.Resize(winWidth, winHeight);
#endif
	// init shader and GPU data
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	string file = dir+objName;
	ReadAsciiObj(file.c_str(), points, triangles, &normals);
	printf("read %i vertices, %i triangles from %s \n", points.size(), triangles.size(), objName.c_str());
	Normalize(points, .8f);	// fit object to +/- .8 space
	if (!normals.size())
		SetVertexNormals(points, triangles, normals);
	BufferVertices();
	// callbacks
	glfwSetCursorPosCallback(w, MouseMove);
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetScrollCallback(w, MouseWheel);
	glfwSetWindowSizeCallback(w, Resize);
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		glfwPollEvents();
		Display(w);
		glfwSwapBuffers(w);
	}
	// free memory, finish
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &VAO);
	glfwDestroyWindow(w);
	glfwTerminate();
}
