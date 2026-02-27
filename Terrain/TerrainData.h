#ifndef __TERRAIN_DATA_H__
#define __TERRAIN_DATA_H__

typedef enum ETerrainData
{
	TERRAIN_SIZE = 128,
	PATCH_SIZE = 16,

	PATCH_XSIZE = PATCH_SIZE,
	PATCH_ZSIZE = PATCH_SIZE,

	PATCH_COUNT = TERRAIN_SIZE / PATCH_SIZE,

	PATCH_XCOUNT = PATCH_COUNT,
	PATCH_ZCOUNT = PATCH_COUNT,

	TERRAIN_PATCH_COUNT = PATCH_XCOUNT * PATCH_ZCOUNT,

	// Core terrain grid dimensions (in cells)
	XSIZE = TERRAIN_SIZE,											// Number of cells along X-axis (e.g., 128 cells)
	ZSIZE = TERRAIN_SIZE,											// Number of cells along Z-axis (matches X for square terrain)

	// Heightmap data (vertex-based)
	HEIGHTMAP_XSIZE = XSIZE + 1,									// Heightmap width: cells + 1 (vertices per row) and This ensures the heightmap covers all vertex points needed to define the terrain’s surface.
	HEIGHTMAP_ZSIZE = ZSIZE + 1,									// Heightmap depth: cells + 1 (vertices per row) and This ensures the heightmap covers all vertex points needed to define the terrain’s surface.
	HEIGHTMAP_RAW_XSIZE = XSIZE + 3,								// Heightmap raw width: cells + 3 (vertices per row) (with padding for edge sampling)
	HEIGHTMAP_RAW_ZSIZE = ZSIZE + 3,								// Heightmap raw depth: cells + 3 (vertices per row) (prevents out-of-bounds access).


	// World Real Size stuff
	PATCH_CELL_SIZE = 1,
	TERRAIN_XSIZE = XSIZE * PATCH_CELL_SIZE,
	TERRAIN_ZSIZE = ZSIZE * PATCH_CELL_SIZE,

} ETerrainData;

static const char terrainMapsFolder[] = "Assets/Maps/";
static const char terrainMapScriptType[] = "AnubisMapSettings";
static const uint32_t TERRAIN_MAGIC_NUMBER = 0x47726964;
static const uint32_t TERRAIN_VERSION_NUMBER = 1;

#endif // __TERRAIN_DATA_H__