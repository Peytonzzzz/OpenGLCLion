// Assn4-ExtrudedLetter-Mac.cpp: uses element buffer, no quads

#include <glad.h>
#include <GLFW/glfw3.h>
#include "CameraArcball.h"
#include "GLXtras.h"
#include "VecMat.h"

// display parameters
int windowWidth = 500, windowHeight = 500;
CameraAB camera(0, 0, windowWidth, windowHeight, vec3(0, 0, 0), vec3(0, 0, -5), 30);

// Letter
float f = 0, b = -.5f; // front and back z-values
// 10 points, repeated
vec3 points[] = { {-.15f, .125f, f}, {-.5f, -.75f, f}, {-.5f, .75f, f},  {.17f, .75f, f}, {.38f, .575f, f},
				  {.38f, .35f, f},   {.23f, .125f, f}, {.5f, -.125f, f}, {.5f, -.5f, f},  {.25f, -.75f, f},
				  {-.15f, .125f, b}, {-.5f, -.75f, b}, {-.5f, .75f, b},  {.17f, .75f, b}, {.38f, .575f, b},
				  {.38f, .35f, b},   {.23f, .125f, b}, {.5f, -.125f, b}, {.5f, -.5f, b},  {.25f, -.75f, b}};

// 10 colors, repeated
vec3 colors[] = { { .5, .5, .5}, { 1, 0, 0}, {.5, 0, 0}, {1, 1, 0}, {.5, 1, 0}, { 0, 1, 0}, { 0, 1, 1}, { 0, 0, 1}, {1, 0, 1}, {.5, 0, .5},
				  { .5, .5, .5}, { 1, 0, 0}, {.5, 0, 0}, {1, 1, 0}, {.5, 1, 0}, { 0, 1, 0}, { 0, 1, 1}, { 0, 0, 1}, {1, 0, 1}, {.5, 0, .5} };

int triangles[][3]= {
		// front triangles
		{0,1,2},{0,2,3},{0,3,4},{0,4,5},{0,5,6},{0,6,7},{0,7,8},{0,8,9},{0,9,1},
		// back triangles
		{10,11,12},{10,12,13},{10,13,14},{10,14,15},{10,15,16},{10,16,17},{10,17,18},{10,18,19},{10,19,11},
		// side triangles
		{3, 13, 12}, {2, 3, 12}, {3, 4, 13}, {4, 13, 14}, {4, 5, 14}, {5, 14, 15}, {5, 6, 15}, {6, 16, 15}, {6, 7, 16},
		{7, 17, 16}, {7, 8, 17}, {8, 18, 17}, {8, 9, 18}, {9, 19, 18}, {1, 9, 19}, {1, 19, 11}, {1, 2, 12}, {1, 11, 12}
};

// vertex buffer, shader IDs
GLuint VBO = 0, VAO = 0, EBO = 0;	// IDs for vertex buffer, vertex array, element buffer
GLuint program = 0;					// IDs for shader program

// shaders
const char *vertexShader = R"(
	#version 410 core
	in vec3 point;
	in vec3 color;
	out vec3 vColor;
	uniform mat4 view;
	void main() {
		gl_Position = view*vec4(point, 1);
		vColor = color;
	}
)";
const char *pixelShader = R"(
	#version 410 core
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1);
	}
)";

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
  if (action == GLFW_PRESS) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
#ifdef __APPLE__
	camera.MouseDown((int) x, (int) y, Shift(w));
#else
	camera.MouseDown((int) x, (int) y, Shift());
#endif
  }
  if (action == GLFW_RELEASE)
	  camera.MouseUp();
}

void MouseMove(GLFWwindow *w, double x, double y) {
  if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	  camera.MouseDrag((int) x, (int) y);
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
#ifdef __APPLE__
	camera.MouseWheel(spin, Shift(w));
#else
	camera.MouseWheel(spin, Shift());
#endif
}

// Display

void Display(GLFWwindow *w) {
  // clear screen, enable z-buffer
  glClearColor(.7f, .7f, .7f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  // init shader program, set vertex feed for points and colors
  glUseProgram(program);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  VertexAttribPointer(program, "point", 3, 0, (void *) 0);
  VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
  SetUniform(program, "view", camera.fullview);
  // draw shaded letter
  glDrawElements(GL_TRIANGLES, sizeof(triangles)/sizeof(int), GL_UNSIGNED_INT, 0);
  glFlush();
}

// Vertex Buffer

void BufferGPU() {
  // create GPU buffer to hold points and colors, and make it the active vertex buffer
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // allocate memory for vertex points and colors, and load data
  int sPoints = sizeof(points), sColors = sizeof(colors);
  glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
  glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);
}

// Application

void Resize(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, windowWidth = width, windowHeight = height);
}

int main(int ac, char **av) {
	// init app window and GL context
	glfwInit();
#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_SAMPLES, 4);
	GLFWwindow *w = glfwCreateWindow(windowWidth, windowHeight, "Letter Perspective", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	// init GL context, init shader program
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
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
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &VBO);
	glfwDestroyWindow(w);
	glfwTerminate();
}
