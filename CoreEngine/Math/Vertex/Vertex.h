#ifndef __VERTEX_H__
#define __VERTEX_H__

#include "../Vectors/Vector2.h"
#include "../Vectors/Vector3.h"
#include "../Vectors/Vector4.h"

typedef struct SVertex
{
	Vector3 m_v3Position;		// World position
	Vector3 m_v3Normals;		// Normal
	Vector2 m_v2TexCoords;		// UVs (For Texturing)
	Vector4 m_v4Color;			// Color
} SVertex;

#define Vertex3D(x, y, z) (SVertex){{x, y, z}};
#endif // __VERTEX_H__