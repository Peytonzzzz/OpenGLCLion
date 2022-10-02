// GLXtras.h - GLSL convenience routines (c) 2019-2022 Jules Bloomenthal

#ifndef GL_XTRAS_HDR
#define GL_XTRAS_HDR

#include "glad.h"
#include "VecMat.h"

// Print Info
int PrintGLErrors(const char *title = NULL);
void PrintVersionInfo();
void PrintExtensions();
void PrintProgramLog(int programID);
void PrintProgramAttributes(int programID);
void PrintProgramUniforms(int programID);

// Shader Compilation
GLuint CompileShaderViaFile(const char *filename, GLint type);
GLuint CompileShaderViaCode(const char **code, GLint type);

// Program Linking
GLuint LinkProgramViaCode(const char **vertexCode, const char **pixelCode);
GLuint LinkProgramViaCode(const char **vertexCode,
						  const char **tessellationControlCode,
						  const char **tessellationEvalCode,
						  const char **geometryCode,
						  const char **pixelCode);
GLuint LinkProgramViaCode(const char **computeCode);
GLuint LinkProgram(GLuint vshader, GLuint pshader);
GLuint LinkProgram(GLuint vshader, GLuint tcshader, GLuint teshader, GLuint gshader, GLuint pshader);
GLuint LinkProgramViaFile(const char *vertexShaderFile, const char *pixelShaderFile);
GLuint LinkProgramViaFile(const char *computeShaderFile);

// Miscellany
int CurrentProgram();
void DeleteProgram(int program);

// Binary Read/Write
void WriteProgramBinary(GLuint program, const char *filename);
bool ReadProgramBinary(GLuint program, const char *filename);
GLuint ReadProgramBinary(const char *filename);

// Uniform Access
void SetReport(bool report);
	// if report, print any unknown uniforms or attributes
bool SetUniform(int program, const char *name, bool val);
bool SetUniform(int program, const char *name, int val);
bool SetUniform(int program, const char *name, GLuint val);
	// some compilers confused by int/GLuint distinction
bool SetUniformv(int program, const char *name, int count, int *v);
bool SetUniform(int program, const char *name, float val);
bool SetUniformv(int program, const char *name, int count, float *v);
bool SetUniform(int program, const char *name, vec2 v);
bool SetUniform(int program, const char *name, vec3 v);
bool SetUniform(int program, const char *name, vec4 v);
bool SetUniform(int program, const char *name, vec3 *v);
bool SetUniform(int program, const char *name, vec4 *v);
bool SetUniform3(int program, const char *name, float *v);
bool SetUniform3v(int program, const char *name, int count, float *v);
bool SetUniform4v(int program, const char *name, int count, float *v);
bool SetUniform(int program, const char *name, mat4 m);
	// if no such named uniform and squawk, print error message

// Attribute Access
int EnableVertexAttribute(int program, const char *name);
	// find named attribute and enable
void DisableVertexAttribute(int program, const char *name);
	// find named attribute and disable
void VertexAttribPointer(int program, const char *name, GLint ncomponents, GLsizei stride, const GLvoid *offset);
	// find and set named attribute, with given number of components, stride between entries, offset into array
	// this calls glAttribPointer with type = GL_FLOAT and normalize = GL_FALSE

#endif // GL_XTRAS_HDR
