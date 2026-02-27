#ifndef __TEXTURES_MANAGER_H__
#define __TEXTURES_MANAGER_H__

#define MAX_ENGINE_TEXTURES 256

#include "../PipeLine/Texture.h"

typedef struct STexturesManager
{
    Texture* textures; // double Pointers to our textures (array)

    // Tracking
    uint32_t count;
    uint32_t capacity;
    uint32_t activeResidentCount;

    // Search (Optional but highly recommended)
    // You could store a hash-map here later to find textures by name
} STexturesManager;

#endif 