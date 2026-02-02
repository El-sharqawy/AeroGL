#define STB_IMAGE_IMPLEMENTATION

#include "../Core/CoreUtils.h"

#define STBI_MALLOC tracked_malloc
#define STBI_REALLOC tracked_realloc
#define STBI_FREE tracked_free

#include <stb_image/stb_image.h>