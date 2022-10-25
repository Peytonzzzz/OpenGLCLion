// 7-LetterPersp.cpp - shade Letter with perspective

#include <glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"
#include "CameraArcball.h"
#include "Draw.h"
#include "Letters.h"
#include "Misc.h"
#include "Quaternion.h"
#include "Text.h"
#include "Widgets.h"

// display parameters
int windowWidth = 500, windowHeight = 500;

// Letter vertices (8 points and 8 colors)
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; // left, right, bottom, top, near far
//vec3 points[] = { {l,b,n}, {l,b,f}, {l,t,n}, {l,t,f}, {r,b,n}, {r,b,f}, {r,t,n}, {r,t,f} };
vec3 points[] = { {.5f, .9f,0}, {.2f, .2f,0}, {.2f, 1.4f,0}, {.7f, 1.4f,0}, {.868f, 1.26f,0}, {.868f, 1.04f,0}, {.74f, .9f,0}, {1.0f, .7f,0}, {1.0f, .4f,0}, {.8f, .2f,0},
				  {.5f, .9f,-.3f}, {.2f, .2f, -.3f}, {.2f, 1.4f,-.3f}, {.7f, 1.4f,-.3f}, {.868f, 1.26f,-.3f}, {.868f, 1.04f,-.3f}, {.74f, .9f,-.3f}, {1.0f, .7f,-.3f}, {1.0f, .4f,-.3f}, {.8f, .2f,-.3f} };
//vec3 colors[] = { {0,0,0}, {0,0,1}, {0,1,0}, {0,1,1}, {1,0,0}, {1,0,1}, {1,1,0}, {1,1,1} };
vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0},
				  {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };

// Letter faces (6 quads or 12 triangles)
//int quads[][4] = { {1,3,2,0}, {6,7,5,4}, {4,5,1,0}, {3,7,6,2}, {2,6,4,0}, {5,7,3,1} };
int sideQuads[][4] = { {2,3,13,12}, {3,4,14,13}, {4,5,15,14}, {6,7,17,16}, {7,8,18,17}, {8,9,19,18}, {1,9,19,11}, {1,2,12,11}, {5,15,16,6} };
int frontTriangles[][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,5}, {0,5,6}, {0,6,7}, {0,7,8}, {0,8,9}, {0,9,1} };
int backTriangles[][3] = { {10,11,12}, {10,12,13}, {10,13,14}, {10,14,15}, {10,15,16}, {10,16,17}, {10,17,18}, {10,18,19}, {10,19,11} };

//int triangles[][3] = { {0,2,1}, {1,2,3}, {4,5,7}, {4,7,6}, {1,5,4}, {0,1,4},
//	                   {2,7,3}, {2,6,7}, {0,4,2}, {2,4,6}, {1,7,5}, {1,3,7} };

// IDs for vertex buffer, shader program
GLuint vBuffer = 0, program = 0;

// Shaders

const char *vertexShader = R"(
	#version 130
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
	#version 130
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1);
	}
)";

// Camera

CameraAB camera(0, 0, windowWidth, windowHeight, vec3(0, 0, 0), vec3(0, 0, -5), 30);

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
	if (action == GLFW_PRESS) {
		double x, y;
		glfwGetCursorPos(w, &x, &y);
		camera.MouseDown((int)x, (int)y, Shift());
	}
	if (action == GLFW_RELEASE) {
		camera.MouseUp();
	}
}

void MouseMove(GLFWwindow *w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		// drag
		camera.MouseDrag((int)x, (int)y);
	}
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	camera.MouseWheel(spin, Shift());
}

// Display

void Display(GLFWwindow *w) {
	// clear screen, enable z-buffer
	glClearColor(.7f, .7f, .7f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// init shader program, set vertex feed for points, colors
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	SetUniform(program, "view", camera.fullview);

	// draw shaded letter
	int nVerticesT = sizeof(frontTriangles) / sizeof(int);
	glDrawElements(GL_TRIANGLES, nVerticesT, GL_UNSIGNED_INT, frontTriangles);
	glDrawElements(GL_TRIANGLES, nVerticesT, GL_UNSIGNED_INT, backTriangles);
	int nVertices = sizeof(sideQuads) / sizeof(int); // not always true
	glDrawElements(GL_QUADS, nVertices, GL_UNSIGNED_INT, sideQuads);

	glFlush();
}

// Vertex Buffer

void InitVertexBuffer() {
	// create GPU buffer to hold points and colors, and make it the active vertex buffer
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate memory for vertex points and colors, and load data
	int sPoints = sizeof(points), sColors = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
}

// App

void Resize(GLFWwindow *window, int width, int height) {
	glViewport(0, 0, windowWidth = width, windowHeight = height);
	camera.Resize(width, height);
}

int main(int ac, char **av) {
	// init app window and GL context
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 4);
	GLFWwindow *w = glfwCreateWindow(windowWidth, windowHeight, "Letter Perspective", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	// init shader and GPU data
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	InitVertexBuffer();
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
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}
