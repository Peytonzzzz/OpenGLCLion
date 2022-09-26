// 2-ClearScreen-Mac.cpp - OpenGL test program for Apple Silicon architecture

#include <glad/glad.h>	// GL header file
#include <GLFW/glfw3.h> // GL toolkit
#include <stdio.h>		// printf, etc
#include "GLXtras.h"	// convenience routines

GLuint VBO = 0, VAO = 0;
GLuint program = 0; // shader prog ID, valid if > 0

int winWidth = 400, winHeight = 400;

// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
	#version 330 core
	in vec2 point;											// 2D point from GPU memory
	void main() {
		// REQUIREMENT 1A) transform vertex:
		gl_Position = vec4(point, 0, 1);					// 'built-in' variable
	}
)";

// pixel shader: operations after the rasterizer
const char *pixelShader = R"(
	#version 330 core
	out vec4 pColor;
	void main() {
		// REQUIREMENT 1B) shade pixel:
		pColor = vec4(0, 1, 0, 1);							// r, g, b, alpha
	}
)";

void InitVertexBuffer()
{
	// REQUIREMENT 3A) create GPU buffer, copy 6 vertices
	float pts[][2] = {{-1, -1}, {-1, 1}, {1, 1}, {-1, -1}, {1, 1}, {1, -1}};
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
}

void Display()
{
	glUseProgram(program); // ensure correct program
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// REQUIREMENT 3B) set vertex feeder
	VertexAttribPointer(program, "point", 2, 0, (void *)0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glFlush(); // flush GL ops
}

int main()
{ // application entry
	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	// create named window of given size
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Clear to Green", NULL, NULL);
	if (!w)
	{
		printf("can't open window\n");
		return 1;
	}
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); // set OpenGL extensions
	// REQUIREMENT 2) build shader program
	if (!(program = LinkProgramViaCode(&vertexShader, &pixelShader)))
	{
		printf("can't link shader program\n");
		return 1;
	}
	InitVertexBuffer(); // set GPU vertex memory
	while (!glfwWindowShouldClose(w))
	{ // event loop
		Display();
		glfwSwapBuffers(w); // double-buffer is default
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
