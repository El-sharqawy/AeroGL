#ifndef __SHADER_H__
#define __SHADER_H__

#include <stdbool.h>
#include <glad/glad.h>
#include "../Math/Matrix/Matrix4.h"

#define MAX_ATTACHED_SHADERS 4

typedef struct SGLShader* GLShader;

bool InitializeShader(GLShader* ppShader, const char* szName);
void AttachShader(GLShader pShader, const char* szShaderFile);
void LinkProgram(GLShader pShader);
void UseProgram(GLShader pShader);
void DestroyProgram(GLShader* pShader);

char* LoadFromFile(const char* szShaderFile);
GLenum GetShaderType(const char* szShaderFile);
bool CheckCompileErrors(GLuint uiID, const char* szShaderFile, bool IsProgram);

// Uniform Parts
void SetInt(GLShader pShader, const char* szUniformName, GLint iInt);
void SetMat4(GLShader pShader, const char* szUniformName, const Matrix4 mat);

#endif // __SHADER_H__