// 2-ClearScreen.cpp - use OpenGL shader architecture

#include <glad/glad.h>   // GL header file
#include <GLFW/glfw3.h>      // GL toolkit
#include <stdio.h>      // printf, etc

#include "GLXtras.h"    // convenience routines

GLuint vBuffer = 0; // GPU vert buf ID, valid if > 0
GLuint program = 0;	// shader prog ID, valid if > 0

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
unsigned int VBO, VAO;
void InitVertexBuffer() {
    // REQUIREMENT 3A) create GPU buffer, copy 4 vertices
// #ifdef GL_QUADS
//     float pts[][2] = {{-1,-1},{-1,1},{1,1},{1,-1}};			// �object�
// #else
    float pts[][2] = {{-1,-1},{-1,1},{1,1},{-1,-1},{1,1},{1,-1}};
// #endif
    // glGenBuffers(1, &vBuffer);								// ID for GPU buffer
    // glBindBuffer(GL_ARRAY_BUFFER, vBuffer);					// make it active
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
}

void Display() {
    glUseProgram(program);									// ensure correct program
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBindBuffer(GL_ARRAY_BUFFER, vBuffer);					// activate vertex buffer
    // REQUIREMENT 3B) set vertex feeder
    VertexAttribPointer(program, "point", 2, 0, (void *) 0);
        // GLint id = glGetAttribLocation(program, "point");
        // glEnableVertexAttribArray(id);
        // glVertexAttribPointer(id, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
// #ifdef GL_QUADS
//     glDrawArrays(GL_QUADS, 0, 4);							// display entire window
// #else
    glDrawArrays(GL_TRIANGLES, 0, 6);
// #endif
    glFlush();												// flush GL ops
}

void GlfwError(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
    getchar();
}

void APIENTRY GlslError(GLenum source, GLenum type, GLuint id, GLenum severity,
                        GLsizei len, const GLchar *msg, const void *data) {
    printf("GLSL Error: %s\n", msg);
    getchar();
}

int AppError(const char *msg) {
    glfwTerminate();
    printf("Error: %s\n", msg);
    getchar();
    return 1;
}

int main() {												// application entry
    // glfwSetErrorCallback(GlfwError);						// init GL framework
    printf("1\n");

    if (!glfwInit())
        return 1;

    printf("2\n");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    printf("3\n");

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
    printf("4\n");

        // create named window of given size
        GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Clear to Green", NULL, NULL);
        printf("5\n");
        if (!w) {
            printf("6\n");
            return AppError("can't open window");
        }
        printf("7\n");
        glfwMakeContextCurrent(w);
        gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);	// set OpenGL extensions
        printf("8\n");
        // following line will not compile if glad.h < OpenGLv4.3
        // or it will compile and give you a seg fault
        // glDebugMessageCallback(GlslError, NULL);
        printf("9\n");
        // REQUIREMENT 2) build shader program
        if (!(program = LinkProgramViaCode(&vertexShader, &pixelShader))) {
            printf("10\n");
            return AppError("can't link shader program");
        }

        InitVertexBuffer(); // set GPU vertex memory
        printf("11\n");     
        while (!glfwWindowShouldClose(w)) {						// event loop
            Display();
            printf("12\n");
            // if (PrintGLErrors())								// test for runtime GL error
            //     getchar();										// if so, pause
            glfwSwapBuffers(w);									// double-buffer is default
            printf("13\n");
            glfwPollEvents();
            printf("14\n");
        }
        printf("15: Outside while\n");
        glfwDestroyWindow(w);
        glfwTerminate();
}
