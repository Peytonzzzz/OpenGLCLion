// 4-ColorfulTriangleRotate-Mac.cpp
// draw RGB triangle using glDrawElements and a GPU element buffer

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <time.h>
#include "GLXtras.h"
#include "VecMat.h"

// GPU identifiers
GLuint VAO = 0, VBO = 0; // vertex array, vertex buffer IDs
GLuint EBO = 0;			 // element buffer ID
GLuint program = 0;		 // shader program ID

// an equilateral triangle (3 2D locations, 3 RGB colors)
float h = 1.5f/(float)sqrt(3);
vec2 points[] = { {-.75f, -h/2}, {0.f, h}, {.75f, -h/2} };
vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };

// triangles
int triangles[][3] = { {0, 1, 2} };

// animation
time_t startTime = clock();
float degPerSec = 30;

const char *vertexShader = R"(
	#version 330 core
	in vec2 point;
	in vec3 color;
	out vec3 vColor;
	uniform float radAng = 0;
	vec2 Rotate2D(vec2 v) {
		float c = cos(radAng), s = sin(radAng);
		return vec2(c*v.x-s*v.y, s*v.x+c*v.y);
	}
	void main() {
		gl_Position = vec4(Rotate2D(point), 0, 1);
		vColor = color;
	}
)";

const char *pixelShader = R"(
	#version 330 core
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1); // 1 means fully opaque
	}
)";

void Display() {
	// clear background
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT);
	// access GPU vertex buffer
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// associate position input to shader with position array in vertex buffer
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	// associate color input to shader with color array in vertex buffer
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// animate
	float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC;
	SetUniform(program, "radAng", (3.1415f/180.f)*dt*degPerSec);
	// render triangle(s)
	int nVertices = sizeof(triangles)/sizeof(int);
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, 0);
//	glDrawArrays(GL_TRIANGLES, 0, 3);
	glFlush();
}

void BufferGPU() {
	// make GPU buffer for points and colors, set it active buffer
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// allocate buffer memory to hold vertex locations and colors
	int sPoints = sizeof(points), sColors = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPoints + sColors, NULL, GL_STATIC_DRAW);
	// copy vertex data to the GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
		// start at beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
		// start at end of points array, for length of colors array
	// copy triangle data to GPU
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW); 
}

// application

int main() {
	// initialize window
	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	// build shader program
	GLFWwindow *w = glfwCreateWindow(800, 800, "Colorful Triangle", NULL, NULL);
	if (!w) {
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	PrintGLErrors();
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	if (!program) {
		printf("can't init shader program\n");
		getchar();
		return 0;
	}
	// allocate vertex memory in the GPU
	BufferGPU();
	// event loop
	glfwSwapInterval(1); // ensure no generated frame backlog
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &VBO);
	glfwDestroyWindow(w);
	glfwTerminate();
}
