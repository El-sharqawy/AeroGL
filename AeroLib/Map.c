#include "Map.h"
#include "../Resources/MemoryTags.h"
#include "../Resources/MemoryManager.h"
#include "../Core/Log.h"
#include <string.h>

bool Map_Initialize(AeroOrderedMap* ppOrderedMap, EMemoryTag tag)
{
	if (!ppOrderedMap)
	{
		return false;
	}

	// Initialize Elements and everything inside struct to NULL / 0
	*ppOrderedMap = engine_new_zero(SAeroOrderedMap, 1, tag);

	AeroOrderedMap map = *ppOrderedMap;

	if (map == NULL)
	{
		syserr("Failed to Allocate Memory for UnorderedMap with tag %s", MemoryTagNames[tag]);
		return (false);
	}

	map->tag = tag;

	return (true);
}

void Map_Destroy(AeroOrderedMap* ppOrderedMap)
{
	if (!ppOrderedMap || !(*ppOrderedMap))
	{
		return;
	}

	AeroOrderedMap map = *ppOrderedMap;

	Map_Clear(map);

	engine_delete(map);

	*ppOrderedMap = NULL;
}

void Map_Clear(AeroOrderedMap map)
{
	if (!map || !map->pRoot)
	{
		return;
	}

	Map_ClearRecusive(map, map->pRoot);

	// Reset the map state
	map->pRoot = NULL;
	map->elementCount = 0;
}

bool Map_Insert(AeroOrderedMap map, const char* key, void* value)
{
	if (!map)
	{
		return (false);
	}

	if (!key || !value)
	{
		syserr("Trying to Insert invalid data into the map");
		return (false);
	}

	// Case 1: The tree is totally empty
	if (map->pRoot == NULL)
	{
		map->pRoot = engine_new_zero(SAeroOrderedMapNode, 1, map->tag);
		map->pRoot->szKey = engine_strdup(key, map->tag);
		map->pRoot->pValue = value;
		map->pRoot->height = 1; // what value should be here?
		map->pRoot->depth = 1;
		map->elementCount++;
		return (true);
	}

	AeroOrderedMapNode pCurrentNode = map->pRoot;
	AeroOrderedMapNode pParentNode = NULL;
	int32_t cmp = 0;

	// 1. Find the spot where the node SHOULD be
	while (pCurrentNode != NULL)
	{
		pParentNode = pCurrentNode; // Keep track of the parent!
		cmp = strcmp(key, pCurrentNode->szKey);

		// we will go to the left
		if (cmp < 0) 
		{
			pCurrentNode = pCurrentNode->pLeft;
		}
		// we will go to the right
		else if (cmp > 0) 
		{
			pCurrentNode = pCurrentNode->pRight;
		}
		else // Found existing key, Update the value
		{
			if (map->pfnDestructor)
			{
				map->pfnDestructor(&pCurrentNode->pValue); // Free Memory
			}
			pCurrentNode->pValue = value;
			return (true);
		}
	}

	// We hit NULL, so create the new node
	AeroOrderedMapNode newNode = engine_new_zero(SAeroOrderedMapNode, 1, map->tag);
	if (newNode == NULL)
	{
		syserr("Failed to Allocate newNode Memory");
		return (false);
	}

	newNode->szKey = engine_strdup(key, map->tag);
	newNode->pValue = value;
	newNode->depth = pParentNode->depth + 1;
	newNode->height = 1;			// New nodes are always height 1 (they are leaves)
	newNode->pParent = pParentNode;	// Useful for iterators later!

	// ATTACH it to the parent we remembered
	if (cmp < 0)
	{
		pParentNode->pLeft = newNode;
	}
	else
	{
		pParentNode->pRight = newNode;
	}

	// 4. Update the Parent's height (Optional but good for future Balancing)
	uint32_t rightHeight = pParentNode->pRight ? pParentNode->pRight->height : 0;
	uint32_t leftHeight = pParentNode->pLeft ? pParentNode->pLeft->height : 0;

	pParentNode->height = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);

	map->elementCount++;
	return (true);
}

void* Map_Find(AeroOrderedMap map, const char* key)
{
	if (!map || !map->pRoot || !key)
	{
		return (NULL);
	}

	SAeroOrderedMapNode* pCurrent = map->pRoot;
	while (pCurrent)
	{
		int32_t cmp = strcmp(key, pCurrent->szKey);

		if (cmp == 0)
		{
			return pCurrent->pValue;
		}

		pCurrent = (cmp > 0) ? pCurrent->pRight : pCurrent->pLeft;
	}

	return (NULL);
}

void Map_Remove(AeroOrderedMap map, const char* key)
{
	if (!map || !map->pRoot || !key)
	{
		return;
	}

	SAeroOrderedMapNode* pCurrent = map->pRoot;
	SAeroOrderedMapNode* pParent = NULL;

	// Find the node and its parent
	while (pCurrent != NULL && strcmp(key, pCurrent->szKey) != 0)
	{
		pParent = pCurrent;
		int32_t cmp = strcmp(key, pCurrent->szKey);
		pCurrent = (cmp > 0) ? pCurrent->pRight : pCurrent->pLeft;
	}

	if (!pCurrent)
	{
		return; // Key not found
	}

	// 2. Scenario: Two Children
	if (pCurrent->pLeft && pCurrent->pRight)
	{
		// Find the In-Order Successor (Smallest in the Right Subtree)
		SAeroOrderedMapNode* pSuccessor = pCurrent->pRight;
		SAeroOrderedMapNode* pSuccessorParent = pCurrent;

		while (pSuccessor->pLeft)
		{
			pSuccessorParent = pSuccessor;
			pSuccessor = pSuccessor->pLeft;
		}

		// Swap the data (Key and Value)
		// We strdup the successor key and free the old one to be safe
		char* oldKey = pCurrent->szKey;
		pCurrent->szKey = engine_strdup(pSuccessor->szKey, map->tag);
		engine_delete(oldKey);

		// Move the value
		if (map->pfnDestructor && pCurrent->pValue)
		{
			map->pfnDestructor(&pCurrent->pValue);
		}

		pCurrent->pValue = pSuccessor->pValue;

		// Now we prepare to delete the successor node instead
		pCurrent = pSuccessor;
		pParent = pSuccessorParent;
	}

	// 3. Scenario: One Child or Leaf
	SAeroOrderedMapNode* pChild = (pCurrent->pLeft) ? pCurrent->pLeft : pCurrent->pRight;

	if (!pParent) // Deleting the Root
	{
		map->pRoot = pChild;
	}
	else if (pParent->pLeft == pCurrent)
	{
		pParent->pLeft = pChild;
	}
	else
	{
		pParent->pRight = pChild;
	}

	// 4. Final Cleanup
	engine_delete(pCurrent->szKey);
	engine_delete(pCurrent);
	map->elementCount--;

	return;
}

void Map_ForEach(AeroOrderedMap map, void(*pfnCallback)(const char* key, void* value))
{
	if (!map || !map->pRoot || !pfnCallback)
	{
		return;
	}

	Map_ForEachRecursive(map->pRoot, pfnCallback);
}

void Map_ClearRecusive(AeroOrderedMap map, SAeroOrderedMapNode* pNode)
{
	if (pNode == NULL)
	{
		return;
	}

	// Visit children first (Recursion)
	Map_ClearRecusive(map, pNode->pLeft);
	Map_ClearRecusive(map, pNode->pRight);

	// Now clean up this node's data
	if (map->pfnDestructor && pNode->pValue)
	{
		// Remember the double-pointer trick from before if needed!
		map->pfnDestructor(&pNode->pValue);
	}

	engine_delete(pNode->szKey);
	engine_delete(pNode);
}

void Map_ForEachRecursive(AeroOrderedMapNode pNode, void(*pfnCallback)(const char* key, void* value))
{
	if (pNode == NULL)
	{
		return;
	}

	// Go Left
	Map_ForEachRecursive(pNode->pLeft, pfnCallback);

	// Process the function
	pfnCallback(pNode->szKey, pNode->pValue);

	// Go Right
	Map_ForEachRecursive(pNode->pRight, pfnCallback);
}
