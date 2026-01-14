#include "Engine.h"

#include "Math/Vectors/Vector4.h"
#include "Math/Vectors/Vector3.h"
#include "Math/EngineMath.h"

#include "Lib/Vector.h"

#include <malloc.h> // For _aligned_malloc or aligned_alloc
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "Debug\\assimp-vc143-mtd.lib")

int main(int argc, char* argv[])
{
	Engine engine = (Engine)tracked_calloc(1, sizeof(SEngine));

	if (!InitializeEngine(engine))
	{
		return EXIT_FAILURE;
	}

	while (engine->isRunning && glfwWindowShouldClose(GetGLWindow(engine->window)) == false)
	{
		UpdateEngine(engine);
	}

	DestroyEngine(engine);

	tracked_free(engine);

	printf("Leaks: %zu\n", allocation_count);
	return (EXIT_SUCCESS);
}