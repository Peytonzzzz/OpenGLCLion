// 2-ClearSimple.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"

GLuint program = 0;
int winWidth = 800, winHeight = 800;

const char *vertexShader = R"(
	#version 130
	void main() {
		vec2 points[] = vec2[4](vec2(-1, -1), vec2(-1, 1), vec2(1, 1), vec2(1, -1));
		gl_Position = vec4(points[gl_VertexID], 0, 1);
	}
)";

const char *pixelShader = R"(
	#version 130
	out vec4 pColor;
	void main() {
		//pColor = vec4(0, 1, 0, 1);
		// float intensity = (gl_FragCoord.x+gl_FragCoord.y)/1600;
		// pColor = vec4(intensity, intensity, intensity, 1);
		
		// Create a chess board pattern with 8x8 squares

		/* if (mod(gl_FragCoord.x, 64.0) < 32.0 && mod(gl_FragCoord.y, 64.0) < 32.0) {
        pColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else if (mod(gl_FragCoord.x, 64.0) < 32.0) {
        pColor = vec4(1.0, 1.0, 1.0, 1.0);
    } else if (mod(gl_FragCoord.y, 64.0) < 32.0) {
        pColor = vec4(1.0, 1.0, 1.0, 1.0);
    } else {
        pColor = vec4(0.0, 0.0, 0.0, 1.0);
    } */

		float x = floor(gl_FragCoord.x / 100.0);
		float y = floor(gl_FragCoord.y / 100.0);
		if (mod(x + y, 2.0) == 0.0) {
			pColor = vec4(0.0, 0.0, 0.0, 1.0);
		} else {
			pColor = vec4(1.0, 1.0, 1.0, 1.0);
		}


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
