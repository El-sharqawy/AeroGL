#ifndef __TEXTURES_MANAGER_H__
#define __TEXTURES_MANAGER_H__

#define MAX_ENGINE_TEXTURES 256

#include "../PipeLine/Texture.h"

typedef struct STexturesManager
{
    Texture textures[MAX_ENGINE_TEXTURES]; // Pointers to our textures
    uint32_t textureCount;
    uint32_t activeResidentCount;
} STexturesManager;

#endif 