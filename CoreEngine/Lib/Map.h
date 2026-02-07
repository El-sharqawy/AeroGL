#ifndef __MAP_H__
#define __MAH_H__

#include <stdint.h>
#include <stdbool.h>

#include "../Resources/MemoryTags.h"

typedef void (*AeroOrderedMapDestructor)(void* pValue);

typedef struct SAeroOrderedMapNode
{
    char* szKey;
    void* pValue;

    struct SAeroOrderedMapNode* pLeft;  // Alphabetically smaller
    struct SAeroOrderedMapNode* pRight; // Alphabetically larger

    // Pro-Tip: Storing the 'height' or 'parent' helps later 
    // if you want to make it an AVL or Red-Black tree (balanced)
    struct SAeroOrderedMapNode* pParent;
    uint32_t height;
    uint32_t depth;
} SAeroOrderedMapNode;

typedef struct SAeroOrderedMapNode* AeroOrderedMapNode;

typedef struct SAeroOrderedMap
{
    AeroOrderedMapNode pRoot;
    uint32_t elementCount;
    uint32_t tag; // Memory tracking
    AeroOrderedMapDestructor pfnDestructor;
} SAeroOrderedMap;

typedef struct SAeroOrderedMap* AeroOrderedMap;

bool Map_Initialize(AeroOrderedMap* ppOrderedMap, EMemoryTag tag);
void Map_Destroy(AeroOrderedMap* ppOrderedMap);

void Map_Clear(AeroOrderedMap map);
bool Map_Insert(AeroOrderedMap map, const char* key, void* value);
void* Map_Find(AeroOrderedMap map, const char* key);
void Map_Remove(AeroOrderedMap map, const char* key);
void Map_ForEach(AeroOrderedMap map, void (*pfnCallback)(const char* key, void* value));

// Private Class stuff
void Map_ClearRecusive(AeroOrderedMap map, SAeroOrderedMapNode* pNode);
void Map_ForEachRecursive(AeroOrderedMapNode pNode, void (*pfnCallback)(const char* key, void* value));

#endif // __MAP_H__