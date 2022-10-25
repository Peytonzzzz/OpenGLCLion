// ShadedLetter.cpp - facet-shade extruded letter

#include <glad.h>
#include <GLFW/glfw3.h>
#include "CameraArcball.h"
#include "Draw.h"
#include "GLXtras.h"
#include "VecMat.h"
#include "Widgets.h"

// display parameters
int winWidth = 800, winHeight = 800;
CameraAB camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -5), 30);

// 10 vertex locations, repeated
vec3 points[] = {
	// front letter 'B', z = 0
	{-.15f, .125f, 0},   {-.5f,  -.75f, 0},   {-.5f,  .75f, 0},   {.17f,  .75f, 0},   {.38f, .575f, 0},
	{ .38f,  .35f, 0},   { .23f, .125f, 0},   {.5f, -.125f, 0},   { .5f, -.5f, 0},    {.25f, -.75f, 0},
	// back, z = -.3
	{-.15f, .125f, -.3f}, {-.5f,  -.75f, -.3f}, {-.5f,  .75f, -.3f}, {.17f,  .75f, -.3f}, {.38f, .575f, -.3f},
	{ .38f,  .35f, -.3f}, { .23f, .125f, -.3f}, {.5f, -.125f, -.3f}, { .5f, -.5f, -.3f},  {.25f, -.75f, -.3f}
};

// 10 colors, repeated
vec3 colors[] = {
	// front colors
	{ .5, .5, .5}, { 1, 0, 0}, {.5, 0, 0}, {1, 1, 0},  {.5, 1, 0},
	{ 0, 1, 0}, { 0, 1, 1}, {0, 0, 1},  { 1, 0, 1}, {.5, 0, .5},
	// back (same as front)
	{ .5,.5,.5}, { 1, 0, 0}, {.5, 0, 0}, {1, 1, 0},  {.5, 1, 0},
	{ 0, 1, 0}, { 0, 1, 1}, {0, 0, 1},  { 1, 0, 1}, {.5, 0, .5}
};

// 9 front triangles
int frontTriangles[][3] = {
	{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5},
	{0, 5, 6}, {0, 6, 7}, {0, 7, 8}, {0, 8, 9}, {0, 9, 1}
};

// 9 back triangles
int backTriangles[][3] = {
	{10, 11, 12}, {10, 12, 13}, {10, 13, 14}, {10, 14, 15},
	{10, 15, 16}, {10, 16, 17}, {10, 17, 18}, {10, 18, 19}, {10, 19, 11}
};

// 18 side triangles
int sideTriangles[][3] = {
	{1, 2, 12}, {1, 12, 11}, {2, 3, 13}, {2, 13, 12}, {3, 4, 14}, {3, 14, 13}, {4, 5, 15}, {4, 15, 14},
	{5, 6, 16}, {5, 16, 15}, {6, 7, 17}, {6, 17, 16}, {7, 8, 18}, {7, 18, 17}, {8, 9, 19}, {8, 19, 18}, {9, 1, 11}, {9, 11, 19}
};

// IDs for vertex array object, vertex buffer object, element buffer object, shader program
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
	in vec3 point, color;
	out vec3 vPoint, vColor;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vColor = color;
	}
)";

const char *pixelShader = R"(
	#version 410 core
	uniform int nLights = 1;
	uniform vec3 lights[20] = vec3[20](vec3(1, 1, 1));
	in vec3 vPoint, vColor;
	out vec4 pColor;
	void main() {
		vec3 dx = dFdx(vPoint);						// change in vPoint in vertical direction
		vec3 dy = dFdy(vPoint);						// change in vPoint in horizontal direction
		vec3 N = normalize(cross(dx, dy));			// unit-length surface normal
		float intensity = 0;						// shade intensity
		for (int i = 0; i < nLights; i++) {
			vec3 L = normalize(lights[i]-vPoint);	// light vector
		//	float d = abs(dot(N, L));
			float d = max(0, dot(N, L));
			intensity += d;							// accumulate
		}
		intensity = min(intensity, 1);				// total intensity limited to 1
		pColor = vec4(intensity*vColor, 1);			// shade the color
	}
)";

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
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// update matrices
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// transform and update lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++) {
		vec4 xLight = camera.modelview*vec4(lights[i], 1);
		xLights[i] = vec3((float*) &xLight);
	}
	// TransformArray(lights, xLights, nLights, camera.modelview);
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float *) xLights);
	// shade triangles
	int nTriangles = (sizeof(frontTriangles)+sizeof(backTriangles)+sizeof(sideTriangles))/sizeof(int);
	glDrawElements(GL_TRIANGLES, nTriangles, GL_UNSIGNED_INT, 0);
	// draw lights
	glDisable(GL_DEPTH_TEST);
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 8, .7f*vec3(1, .8f, 0), camera.fullview);
	if (!picked && !Shift(w) && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
		camera.arcball.Draw(Control(w));
	glFlush();
}

// Mouse Callbacks

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
	picked = NULL;
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	int ix = (int) x, iy = (int) y;
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
			camera.MouseDown(ix, iy, Shift(w), Control(w));
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
	camera.MouseWheel(spin, Shift(w));
}

// Initialization

void BufferGPU() {
	// make GPU buffer for points and colors, make it active
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// allocate memory for vertex points, colors, and normals
	int sPoints = sizeof(points), sColors = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
	// load data
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
	// copy triangle data to GPU
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	int fSize = sizeof(frontTriangles), bSize = sizeof(backTriangles), sSize = sizeof(sideTriangles);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fSize+bSize+sSize, NULL, GL_STATIC_DRAW); 
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, fSize, frontTriangles);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, fSize, bSize, backTriangles);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, fSize+bSize, sSize, sideTriangles);
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
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Shaded Letter", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	// init shader and GPU data
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	BufferGPU();
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
	glfwDestroyWindow(w);
	glfwTerminate();
}
