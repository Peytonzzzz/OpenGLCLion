// 2-ClearSimple.cpp

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"

GLuint program = 0;
int winWidth = 800, winHeight = 800;

const char *vertexShader = R"(
	#version 330 core
	void main() {
		vec2 points[] = vec2[4](vec2(-1, -1), vec2(-1, 1), vec2(1, 1), vec2(1, -1));
		gl_Position = vec4(points[gl_VertexID], 0, 1);
	}
)";

const char *pixelShader = R"(
	#version 330 core
	out vec4 pColor;
	void main() {
		pColor = vec4(0, 1, 0, 1);
	}
)";

void Display() {
	glUseProgram(program);
	glDrawArrays(GL_QUADS, 0, 4);
	glFlush();
}

int main() {
	// init window, build shader
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Clear to Green", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
