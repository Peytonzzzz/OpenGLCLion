// 4-ColorfulTriangle.cpp - draw RGB triangle via GLSL, vBuffer

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include "GLXtras.h"
#include "VecMat.h"
#include "time.h"

// GPU identifiers
GLuint VBO = 0, VAO = 0; // GPU vertex buffer ID
GLuint program = 0;		 // GLSL program ID
// a triangle (3 2D locations, 3 RGB colors)
vec2 points[] = { {-.5f, .4f}, {-.5f, .5f}, {.0f, .4f},
                  {.0f, .5f},{.4f,.4f},{.5f,.5f},
                  {.5f,.4f},{.2f,.2f},{.2f,.1f},
                  {.0f,-.1f},{-.3f,-.3f},{-.3f,-.4f},
                  {-.5f,-.5f},{-.3f,-.5f},
                  {.0f,-.5f},{.2f,-.4f},{.5f,-.5f},
                  {.5f,-.4f}};
vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0},
                  {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, 
                  {0, 0, 1}, {1, 0, 0},{0, 1, 0},
                  {0, 0, 1}, {1, 0, 0}, {0, 1, 0},
                  {0, 0, 1}, {1, 0, 0}, {0, 1, 0},
                  {0, 0, 1}, {1, 0, 0}, {0, 1, 0}};
int triangles[][3]={{0,1,2},{1,2,3},{3,2,4},{3,5,4},{5,6,7},{7,6,8},{7,8,9},
                    {7,9,10},{9,10,11},{10,11,12},{11,12,13},{11,13,14},
                    {11,14,15},{14,15,16},{15,16,17}};
time_t startTime = clock();
float degPerSec=25;
const char *vertexShader = R"(
	#version 410 core
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
	#version 410 core
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
  // render three vertices as a triangle
  float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC;
  float degAng = dt * degPerSec;
  mat4 view = RotateZ(degAng);
  SetUniform(program,"view",view);
  int nVertices = sizeof(triangles)/sizeof(int);
  glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
  glFlush();
}

void BufferVertices() {
  // make GPU buffer for points and colors, set it active buffer
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
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
  glDeleteBuffers(1, &VBO);
  glfwDestroyWindow(w);
  glfwTerminate();
}
