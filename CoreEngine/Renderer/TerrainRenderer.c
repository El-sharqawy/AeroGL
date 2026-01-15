#include "TerrainRenderer.h"

bool TerrainRenderer_Initialize(TerrainRenderer* ppTerrainRenderer, GLCamera pCamera, const char* szRendererName)
{
	*ppTerrainRenderer = (TerrainRenderer)tracked_calloc(1, sizeof(STerrainRenderer));
	
	TerrainRenderer pBuffer = *ppTerrainRenderer;
	return false;
}

void TerrainRenderer_Destroy(TerrainRenderer* pTerrainRenderer)
{

}
