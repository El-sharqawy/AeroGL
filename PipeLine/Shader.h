#ifndef __SHADER_H__
#define __SHADER_H__

#include <stdbool.h>
#include <glad/glad.h>
#include "../Math/Vectors/Vector2.h"
#include "../Math/Matrix/Matrix4.h"

#define MAX_ATTACHED_SHADERS 4

typedef struct SGLShader* GLShader;

bool Shader_Initialize(GLShader* ppShader, const char* szName);
void Shader_AttachShader(GLShader pShader, const char* szShaderFile);
void Shader_LinkProgram(GLShader pShader);
void Shader_UseProgram(GLShader pShader);
void Shader_Destroy(GLShader* pShader);

void Shader_SetInjection(GLShader pShader, bool bAllow);

// Shader Private Methods
char* LoadFromFile(const char* szShaderFile);
GLenum GetShaderType(const char* szShaderFile);
bool CheckCompileErrors(GLuint uiID, const char* szShaderFile, bool IsProgram);

// Uniform Parts
void Shader_SetBool(GLShader pShader, const char* szUniformName, bool bValue);
void Shader_SetInt(GLShader pShader, const char* szUniformName, GLint iValue);
void Shader_SetFloat(GLShader pShader, const char* szUniformName, float fValue);
void Shader_SetVec2(GLShader pShader, const char* szUniformName, const Vector2 vec2);
void Shader_SetVec3(GLShader pShader, const char* szUniformName, const Vector3 vec3);
void Shader_SetVec4(GLShader pShader, const char* szUniformName, const Vector4 vec4);
void Shader_SetMat4(GLShader pShader, const char* szUniformName, const Matrix4 mat);
void Shader_SetBindlessSampler2D(GLShader pShader, const char* szUniformName, GLuint64 value);

#endif // __SHADER_H__