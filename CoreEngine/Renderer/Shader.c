#include "Shader.h"
#include "../Core/CoreUtils.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

typedef struct SGLShader
{
	GLuint programID;		// OpenGL program object ID
	char* szProgramName;	// Program name for debugging
	bool IsInitialized;	// Program object created
	bool IsLinked;			// Shaders linked successfully
	GLint shadersNum;		// Number of shaders in the current program
	GLuint shaders[MAX_ATTACHED_SHADERS];		// Temporary storage for shader IDs
} SGLShader;

bool InitializeShader(GLShader* ppShader, const char* szName)
{
	*ppShader = (GLShader)tracked_malloc(sizeof(SGLShader));

	GLShader pShader = *ppShader;
	if (pShader == NULL)
	{
		syserr("Failed to Create Shader %s", szName);
		return (false);
	}

	// Set all bytes to 0, ensuring m_szWindowTitle is NULL
	memset(pShader, 0, sizeof(SGLShader));

	pShader->programID = glCreateProgram();
	pShader->szProgramName = tracked_strdup(szName);

	return (true);
}

void AttachShader(GLShader pShader, const char* szShaderFile)
{
	if (!pShader)
	{
		syserr("Attemp to use a NULL shader");
		return;
	}

	// Check if we have a valid program to attach to
	if (pShader->programID == 0)
	{
		syslog("Shader Is not Initialized, Attemp to Initialize it ..");
	}

	// Load Source
	char* shaderSource = LoadFromFile(szShaderFile);
	if (shaderSource == NULL)
	{
		syserr("Failde to Load Shader %s", szShaderFile);
		return;
	}

	// Determine shader type from file extension
	GLenum shaderType = GetShaderType(szShaderFile);
	if (shaderType == GL_INVALID_ENUM)
	{
		syserr("Failed to Find Shader %s Type", szShaderFile);
		return;
	}

	// Create and Compile
	GLuint uiShaderID = glCreateShader(shaderType);
	glShaderSource(uiShaderID, 1, (const GLchar**)&shaderSource, nullptr);
	glCompileShader(uiShaderID);

	// Check for compilation errors
	if (CheckCompileErrors(uiShaderID, szShaderFile, false) == false)
	{
		glDeleteShader(uiShaderID);
		syserr("Failed Compiling shader %s", szShaderFile);
		return;
	}

	// Attach and Store ID for later cleanup
	glAttachShader(pShader->programID, uiShaderID);

	// Safety check for array bounds (assuming max 4-8 shaders)
	if (pShader->shadersNum < MAX_ATTACHED_SHADERS)
	{
		pShader->shaders[pShader->shadersNum] = uiShaderID;
		pShader->shadersNum++;
	}

	// free allocated space for shader source code
	tracked_free(shaderSource);

	// Assign it as Initialized Shader
	pShader->IsInitialized = true;
}

void LinkProgram(GLShader pShader)
{
	if (pShader->IsInitialized == false)
	{
		syserr("Attemp to Link a non Initialized Program %s", pShader->szProgramName)
		return;
	}
	if (pShader->IsLinked == true)
	{
		syserr("Attemp to Link a Linked Program %s", pShader->szProgramName)
		return;
	}

	// Link the program
	glLinkProgram(pShader->programID);

	// Check for linking errors
	if (!CheckCompileErrors(pShader->programID, pShader->szProgramName, true))
	{
		syserr("Failed to link program %s (%d)", pShader->szProgramName, pShader->programID);
		return;
	}

	pShader->IsLinked = true;

	// Clean up shader objects (no longer needed after linking)
	for (int i = 0; i < pShader->shadersNum; i++)
	{
		if (pShader->shaders[i] != 0)
		{
			glDetachShader(pShader->programID, pShader->shaders[i]); // Detach Shaders First
			glDeleteShader(pShader->shaders[i]);
		}
	}

	// Set all bytes to 0, ensuring all values are 0
	memset(pShader->shaders, 0, sizeof(pShader->shaders));
}

void UseProgram(GLShader pShader)
{
	if (pShader && pShader->IsLinked && pShader->programID != 0)
	{
		glUseProgram(pShader->programID);
	}
	else
	{
		glUseProgram(0);
	}
}

void DestroyProgram(GLShader* ppShader)
{
	if (!ppShader || !*ppShader)
	{
		syserr("Attemp to delete a NULL shader");
		return;
	}

	GLShader pShader = *ppShader; // Get the actual pointer

	// Free the name first
	if (pShader->szProgramName)
	{
		tracked_free(pShader->szProgramName);
	}

	// Release GPU resources
	glDeleteProgram(pShader->programID);

	// Release Shader from Memory
	tracked_free(pShader);

	*ppShader = NULL; // Reach back to the caller and set it to NULL
}

char* LoadFromFile(const char* szShaderFile)
{
	/* retrieve the shader source code from filePath */
	char* shaderCode = { 0 };

	// File pointer to store the 
	FILE* pFile = NULL;
	long length = 0;
	errno_t err;

	// Opening the file in read mode
	err = fopen_s(&pFile, szShaderFile, "rb"); // Open for reading in Binary Mode
	if (err != 0)
	{
		if (err == ENOENT)
		{
			syserr("Failed to Open File %s, File Not Found", szShaderFile);
			return (NULL);
		}
		else
		{
			syserr("Failed to Open File %s For Reading, Error: %d", szShaderFile, err);
			return (NULL);
		}
	}

	// succeed opening the file
	fseek(pFile, 0, SEEK_END);
	length = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	if (length <= 0)
	{
		syserr("Shader file is empty: %s", szShaderFile);
		fclose(pFile);
		return NULL;
	}

	shaderCode = (char*)tracked_malloc(length + 1); // +1 for null terminator
	if (shaderCode)
	{
		size_t bytesRead = fread(shaderCode, 1, length, pFile);

		// Add Null terminator at the end
		shaderCode[bytesRead] = '\0'; // Use actual bytes read for safety

		if (bytesRead == 0 || bytesRead < length && ferror(pFile))
		{
			tracked_free(shaderCode);
			shaderCode = NULL;
		}
	}

	fclose(pFile);
	return (shaderCode);
}

GLenum GetShaderType(const char* szShaderFile)
{
	const char* szShaderType = get_filename_ext(szShaderFile);
	if (!szShaderType)
	{
		syserr("Failed to Find Vertex %s type", szShaderFile);
		return (GL_INVALID_ENUM);
	}

	if (strcmp("vert", szShaderType) == 0)
	{
		return (GL_VERTEX_SHADER);
	}
	if (strcmp("frag", szShaderType) == 0)
	{
		return (GL_FRAGMENT_SHADER);
	}
	if (strcmp("tes", szShaderType) == 0)
	{
		return (GL_TESS_EVALUATION_SHADER);
	}
	if (strcmp("tcs", szShaderType) == 0)
	{
		return (GL_TESS_CONTROL_SHADER);
	}
	if (strcmp("geom", szShaderType) == 0)
	{
		return (GL_GEOMETRY_SHADER);
	}
	if (strcmp("comp", szShaderType) == 0)
	{
		return (GL_COMPUTE_SHADER);
	}

	syserr("Unknown Shader Type %s - (%s)", szShaderFile, szShaderType);
	return (GL_INVALID_ENUM);
}

bool CheckCompileErrors(GLuint uiID, const char* szShaderFile, bool IsProgram)
{
	GLint iSuccess = 0;
	char szInfoLog[1024] = { 0 };

	if (IsProgram)
	{
		glGetProgramiv(uiID, GL_LINK_STATUS, &iSuccess);
		if (iSuccess == 0)
		{
			glGetProgramInfoLog(uiID, 1024, nullptr, szInfoLog);
			syserr("Linking the program %s Failed, Error: %s", szShaderFile, szInfoLog);
		}
	}
	else
	{
		glGetShaderiv(uiID, GL_COMPILE_STATUS, &iSuccess);
		if (iSuccess == 0)
		{
			glGetShaderInfoLog(uiID, 1024, nullptr, szInfoLog);
			syserr("Compiling the Shader %s Failed, Error: %s", szShaderFile, szInfoLog);
		}
	}
	return (iSuccess);
}

void SetMat4(GLShader pShader, const char* szUniformName, const Matrix4 mat)
{
	if (!pShader)
	{
		return;
	}

	// Use the program from the shader struct
	GLuint program = pShader->programID;

	// Optional: Only set if the shader is currently 'In Use'
	UseProgram(pShader);

	GLint iMatIndex = glGetUniformLocation(program, szUniformName);

	if (iMatIndex == -1)
	{
		syserr("Failed to Find Uniform %s", szUniformName);
		return;
	}

	glUniformMatrix4fv(iMatIndex, 1, GL_FALSE, &mat.cols[0].x);
}
