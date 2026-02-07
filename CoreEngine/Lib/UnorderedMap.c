#include "UnorderedMap.h"
#include "../Resources/MemoryTags.h"
#include "../Resources/MemoryManager.h"
#include "../Core/Log.h"
#include <string.h>

bool UnorderedMap_Initialize(AeroUnorderedMap* ppUnorderedMap, EMemoryTag tag)
{
	// Initialize Elements and everything inside struct to NULL / 0
	*ppUnorderedMap = engine_new_zero(SAeroUnorderedMap, 1, tag);

	AeroUnorderedMap map = *ppUnorderedMap;

	if (map == NULL)
	{
		syserr("Failed to Allocate Memory for UnorderedMap with tag %s", MemoryTagNames[tag]);
		return (false);
	}

	map->ppBuckets = engine_new_zero(AeroUnorderedMapNode, unordered_map_init_val, tag);

	if (map->ppBuckets == NULL)
	{
		syserr("Failed to Allocate Buckets for UnorderedMap (Tag: %s)", MemoryTagNames[tag]);
		// Clean up the header so we don't leak it on partial failure
		engine_delete(map);
		return false;
	}

	// Set Metadata
	map->bucketCount = unordered_map_init_val;
	map->tag = tag;
	map->elementCount = 0;

	return (true);
}

void UnoderedMap_Destroy(AeroUnorderedMap* ppUnorderedMap)
{
	if (!ppUnorderedMap || !(*ppUnorderedMap))
	{
		return;
	}

	AeroUnorderedMap map = *ppUnorderedMap;

	// Remove all nodes, keys, and values
	UnorderedMap_Clear(map);

	// Delete the array of bucket pointers (the "parking lot" pavement)
	if (map->ppBuckets)
	{
		engine_delete(map->ppBuckets);
	}

	engine_delete(map);

	*ppUnorderedMap = NULL;
}

void UnorderedMap_Clear(AeroUnorderedMap map)
{
	for (size_t i = 0; i < map->bucketCount; i++)
	{
		// Get the head of the list for this bucket
		SAeroUnorderedMapNode* pCurrentNode = map->ppBuckets[i];

		// Walk the linked list
		while (pCurrentNode != NULL)
		{
			// Store the next node BEFORE we delete the current one
			SAeroUnorderedMapNode* pNextNode = pCurrentNode->next;

			// Call the destructor for the user's data (if provided)
			if (map->pfnDestructor && pCurrentNode->pValue)
			{
				map->pfnDestructor(&pCurrentNode->pValue);
			}

			// Free the key (since we used engine_strdup)
			if (pCurrentNode->szKey)
			{
				engine_delete(pCurrentNode->szKey);
			}

			// Free the node itself
			engine_delete(pCurrentNode);

			// Move to the next node in the chain
			pCurrentNode = pNextNode;
		}

		// Set the bucket head to NULL so the map is safe to use again
		map->ppBuckets[i] = NULL;
	}
	map->elementCount = 0;
}

bool UnorderedMap_Insert(AeroUnorderedMap map, const char* key, void* value)
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

	if (map->elementCount >= map->bucketCount)
	{
		// A simple way is to double and then find the next prime, 
		// or just multiply by 2 + 1 to keep it odd (cheap version)
		uint32_t newSize = (map->bucketCount * 2) + 1;
		UnorderedMap_Resize(map, newSize);
	}

	uint32_t hash = fnv1a_str(key);
	uint32_t index = hash % map->bucketCount;

	// Search for existing key to update it
	SAeroUnorderedMapNode* pCurrentNode = map->ppBuckets[index];
	while (pCurrentNode != NULL)
	{
		// Compare hashes FIRST (fast integer check)
		// Only call strcmp (slow string check) if hashes match!
		if (pCurrentNode->hash == hash && strcmp(key, pCurrentNode->szKey) == 0) 
		{
			if (map->pfnDestructor)
			{
				map->pfnDestructor(&pCurrentNode->pValue); // Free Memory
			}
			pCurrentNode->pValue = value;
			return (true);
		}

		pCurrentNode = pCurrentNode->next;
	}

	// Not found, create a NEW node
	AeroUnorderedMapNode newNode = engine_new_zero(SAeroUnorderedMapNode, 1, map->tag);
	if (newNode == NULL)
	{
		syserr("Failed to Allocate newNode Memory");
		return (false);
	}

	// Setup the node
	newNode->hash = hash;
	newNode->szKey = engine_strdup(key, map->tag);
	newNode->pValue = value;

	// Link it into the head of the bucket list
	newNode->next = map->ppBuckets[index];
	map->ppBuckets[index] = newNode;


	map->elementCount++;
	return (true);
}

void* UnorderedMap_Find(AeroUnorderedMap map, const char* key)
{
	if (!map)
	{
		return (NULL);
	}

	if (!key)
	{
		syserr("Trying to Find invalid key into the map");
		return (NULL);
	}

	uint32_t hash = fnv1a_str(key);
	uint32_t index = hash % map->bucketCount;

	// Search for existing key to update it
	SAeroUnorderedMapNode* pCurrentNode = map->ppBuckets[index];
	while (pCurrentNode != NULL)
	{
		// Compare hashes FIRST (fast integer check)
		// Only call strcmp (slow string check) if hashes match!
		if (pCurrentNode->hash == hash && strcmp(key, pCurrentNode->szKey) == 0)
		{
			return (pCurrentNode->pValue);
		}

		pCurrentNode = pCurrentNode->next;
	}

	return (NULL);
}

void UnorderedMap_Remove(AeroUnorderedMap map, const char* key)
{
	if (!map)
	{
		return;
	}

	if (!key)
	{
		syserr("Trying to Find invalid key into the map");
		return;
	}

	uint32_t hash = fnv1a_str(key);
	uint32_t index = hash % map->bucketCount;
	// Search for existing key to update it
	SAeroUnorderedMapNode* pCurrentNode = map->ppBuckets[index];
	SAeroUnorderedMapNode* pPreviousNode = NULL;
	while (pCurrentNode != NULL)
	{
		// Compare hashes FIRST (fast integer check)
		// Only call strcmp (slow string check) if hashes match!
		if (pCurrentNode->hash == hash && strcmp(key, pCurrentNode->szKey) == 0)
		{
			// "Unplug" the node from the chain
			if (pPreviousNode == NULL)
			{
				// This was the first node in the bucket
				map->ppBuckets[index] = pCurrentNode->next;
			}
			else
			{
				// Link the previous node to the one after the current one
				pPreviousNode->next = pCurrentNode->next;
			}

			// Cleanup the data
			if (map->pfnDestructor)
			{
				map->pfnDestructor(&pCurrentNode->pValue);
			}

			engine_delete(pCurrentNode->szKey);
			engine_delete(pCurrentNode);

			map->elementCount--;

			return; // We found it and deleted it, we can stop now
		}

		// Move pointers forward
		pPreviousNode = pCurrentNode;
		pCurrentNode = pCurrentNode->next;
	}
}

void UnorderedMap_Resize(AeroUnorderedMap map, uint32_t newSize)
{
	AeroUnorderedMapNode* pNewBuckets = engine_new_zero(AeroUnorderedMapNode, 1, map->tag);
	if (!pNewBuckets)
	{
		return; // Fail gracefully
	}

	// Iterate through old buckets
	for (uint32_t i = 0; i < map->bucketCount; i++)
	{
		AeroUnorderedMapNode pNode = map->ppBuckets[i];

		while (pNode)
		{
			// Save the next node in the OLD bucket before we move the current one
			SAeroUnorderedMapNode* pNext = pNode->next;

			// Calculate new index using the cached hash
			uint32_t newIndex = pNode->hash % newSize;

			// Insert at the head of the NEW bucket
			pNode->next = pNewBuckets[newIndex];
			pNewBuckets[newIndex] = pNode;

			pNode = pNext;
		}
	}

	// Swap the arrays and update metadata
	engine_delete(map->ppBuckets);
	map->ppBuckets = pNewBuckets;
	map->bucketCount = newSize;
}

SAeroUnorderedMapIterator UnorderedMap_Begin(AeroUnorderedMap map)
{
	if (!map)
	{
		syserr("did you forget to initialize the map?!");
		return (SAeroUnorderedMapIterator){ 0 };
	}

	SAeroUnorderedMapIterator it = { 0 }; // Zero out everything
	it._map = map;
	it._bucketIndex = -1;
	it.node = NULL;

	if (!map || map->elementCount == 0)
	{
		return it;
	}

	// Find the first non-empty bucket
	for (uint32_t i = 0; i < map->bucketCount; i++)
	{
		if (map->ppBuckets[i] != NULL)
		{
			it.node = map->ppBuckets[i];
			it._bucketIndex = i;
			it.key = it.node->szKey;
			it.value = it.node->pValue;
			break;
		}
	}

	return (it);
}

SAeroUnorderedMapIterator UnorderedMap_End(AeroUnorderedMap map)
{
	SAeroUnorderedMapIterator it = { 0 }; // Zero out everything
	it._map = map;
	it._bucketIndex = -1;
	it.node = NULL;
	return (it);
}

bool UnorderedMap_IterCompare(SAeroUnorderedMapIterator a, SAeroUnorderedMapIterator b)
{
	// If both nodes are NULL, they are both "End" iterators and therefore equal
	return a.node == b.node;
}

bool UnorderedMap_IteratorNext(AeroUnorderedMapIterator iterator)
{
	if (!iterator || !iterator->node)
	{
		return (false);
	}

	// Try to move to the next node in the current linked list
	if (iterator->node->next != NULL)
	{
		iterator->node = iterator->node->next;
		iterator->key = iterator->node->szKey;
		iterator->value = iterator->node->pValue;
		return (true);
	}

	// If we hit the end of a list, find the next non-empty bucket
	for (uint32_t i = iterator->_bucketIndex + 1; i < iterator->_map->bucketCount; i++)
	{
		if (iterator->_map->ppBuckets[i] != NULL)
		{
			iterator->_bucketIndex = i;
			iterator->node = iterator->_map->ppBuckets[i];
			iterator->key = iterator->node->szKey;
			iterator->value = iterator->node->pValue;
			return (true);
		}
	}

	// No more items found
	iterator->node = NULL;
	iterator->key = NULL;
	iterator->value = NULL;
	return (false);
}

uint32_t UnorderedMap_Count(AeroUnorderedMap map)
{
	if (!map)
	{
		return (0);
	}

	return (map->elementCount);
}

uint32_t UnorderedMap_Capactiy(AeroUnorderedMap map)
{
	if (!map)
	{
		return (0);
	}

	return (map->bucketCount);
}

bool UnorderedMap_IsEmpty(AeroUnorderedMap map)
{
	return (UnorderedMap_Count(map) == 0);
}