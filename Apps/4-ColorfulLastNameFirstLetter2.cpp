// 4-ColorfulLastNameFirstLetter.cpp - draw/rotate RGB triangle

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <time.h>
#include "GLXtras.h"
#include "VecMat.h"

// GPU identifiers
GLuint vBuffer = 0, vArray = 0;	// GPU vertex IDs
GLuint program = 0;				// GLSL program ID

// an equilateral triangle (3 2D locations, 3 RGB colors)
float h = 1.5f/(float)sqrt(3);
vec2 points[] = { {.125f, .225f}, {.05f, .05f}, {.05f, .35f}, {.175f, .35f}, {.217f, .315f}, {.217f, .260f}, {.185f, .225f}, {.250f, .175f}, {.250f, .1f}, {.2f, .05f} };
vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };
int triangles[][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,5}, {0,5,6}, {0,6,7}, {0,7,8}, {0,8,9}, {0,9,1}};

// animation
time_t startTime = clock();
float degPerSec = 30;

const char *vertexShader = R"(
	#version 130
	in vec2 point;
	in vec3 color;
	out vec3 vColor;
	uniform mat4 view;
	
	void main() {
		gl_Position = view*vec4(point, 0, 1);
		vColor = color;
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1); // 1: fully opaque
	}
)";

void Display() {
	// clear background
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	// access GPU vertex buffer
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// connect position input to shader with position array in vertex buffer
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	// connect color input to shader with color array in vertex buffer
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// compute rotation angle given elapsed time
	float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC;
	float degAng = dt * degPerSec;
	mat4 view = RotateZ(degAng); // RotateZ takes angle in degrees, not radians
	SetUniform(program, "view", view);
	// the above three lines can also be written SetUniform(program, "view", RotateZ(dt*degPerSec));
	// render ten vertices as a Letter B
	int nVertices = sizeof(triangles) / sizeof(int);
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	glFlush();
}

void BufferVertices() {
	// make GPU buffer for points and colors, set it active
	glGenVertexArrays(1, &vArray);
	glBindVertexArray(vArray);
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate buffer memory to hold vertex locations and colors
	int sPoints = sizeof(points), sColors = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
	// copy data to GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
		// start at beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
		// start at end of points array, for length of colors array
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
	GLFWwindow *w = glfwCreateWindow(800, 800, "Colorful Triangle", NULL, NULL);
	if (!w) {
		glfwTerminate();
		return 1;
	}
	// build shader program
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
	BufferVertices();
	// event loop
	glfwSwapInterval(1); // ensure no generated frame backlog
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}
