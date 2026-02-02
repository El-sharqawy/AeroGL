#include "Engine.h"
#include "Lib/Vector.h"
#include "PipeLine/Texture.h"

#pragma comment(lib, "glfw3.lib")
// #pragma comment(lib, "Debug\\assimp-vc143-mtd.lib")

int main(int argc, char* argv[])
{
	Engine engine = tracked_calloc(1, sizeof(SEngine));

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

	printf("Leaked: %zu objects with: %zu bytes (%zu KB)\n", allocation_count, bytes_allocated, bytes_allocated / 1024);
	return (EXIT_SUCCESS);
}