#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum ETexturePrecision 
{
	TEXTURE_PRECISION_INT8,    // Standard images (PNG, JPG)
	TEXTURE_PRECISION_INT16,   // High-quality heightmaps
	TEXTURE_PRECISION_FLOAT16, // HDR textures, Framebuffers (Half-Float)
	TEXTURE_PRECISION_FLOAT32, // Physics data, G-Buffers (Full-Float)
} ETexturePrecision;

/**
 * @brief Structure to hold image data.
 */
typedef struct SImageData
{
	// 8-byte types
	size_t dataSize;				// Total size in bytes

	// 8-byte pointers at the top
	char* szTexturePath;			// File path of the texture
	char* szTextureName;			// Name identifier for the texture (grass_tex, snow_tex, etc..)
	void* pData;					// Pointer to the image data, NULL after upload, unless needed for CPU logic (e.g. Physics)

	// 4-byte integers grouped
	int32_t width;					// Width of the image
	int32_t height;					// Height of the image
	int32_t channels;				// Number of color channels (e.g., 3 for RGB, 4 for RGBA)
	bool isLoaded;					// image successfully loaded
	uint8_t padding[3];				// padding to match 48 bytes size
} SImageData;

typedef struct SImageData* ImageData;

/* Group Elements by their size to reach maximum optimization */
typedef struct STexture
{
	// 8-byte types
	uint64_t textureHandle;		// For Bindless rendering (advanced)

	// Texture CPU Features (40 bytes)
	SImageData imageData;

	// OpenGL State (12 bytes)
	uint32_t textureID;			// OpenGL Texture ID
	uint32_t textureTarget;		// Texture target (e.g., GL_TEXTURE_2D)

	uint32_t internalFormat;	// image format (GL_RGBA8, etc..)
	uint32_t pixelFormat;		// image pixel format (GL_RGB, GL_RGBA, etc..)
	uint32_t sourceType;		// Data type of the source image data (e.g., GL_UNSIGNED_BYTE)

	uint32_t minFilter;			// GL_TEXTURE_MIN_FILTER value (GL_LINEAR, GL_REPEAT, etc)
	uint32_t magFilter;			// GL_TEXTURE_MAG_FILTER value (GL_LINEAR, GL_REPEAT, etc)
	uint32_t wrapS;				// GL_TEXTURE_WRAP_S value (GL_REPEAT, GL_CLAMP_TO_EDGE, etc)
	uint32_t wrapT;				// GL_TEXTURE_WRAP_T value (GL_REPEAT, GL_CLAMP_TO_EDGE, etc)
	int32_t mipMapLevels;		// mipMap Level

	// 1-byte types (Grouped together to save space)
	uint16_t texturePrecision;	// Uses ETexturePrecision, to setup Image format
	bool isLoaded;				// Flag indicating if the texture is loaded
	bool isCompressed;			// Flag indicating if the texture is compressed

	// OpenGL GPU Data
	bool isBindless;			// Flag indicating if the texture is bindless
	bool isResident;			// Flag indicating if the texture is resident in GPU memory
	bool isMipMap;				// Flag indicating if the texture has mipmaps
	bool isSRGB;				// Flag indicating if the texture to automatically convert the texture to linear space (Diffuse/Albedo)
	bool isSwizzle;				// Flag indicating if the texture need swizzle mask
	bool isRawTexture;
} STexture;

typedef struct STexture* Texture;

bool Texture_Initialize(Texture* ppTexture);
void Texture_Destroy(Texture* ppTexture);
bool Texture_MakeResident(Texture pTexture, bool bSetResident);

bool Texture_LoadImage(Texture pTexture, const char* szTexturePath);
bool Texture_Load(Texture pTexture, const char* szTexturePath);
bool Texture_UploadToGPU(Texture pTexture);
bool Texture_DSAUploadToGPU(Texture pTexture);
bool Texture_LegacyUploadToGPU(Texture pTexture);

// Load Texture with Data (pointer)
bool Texture_LoadHeightMapImage(Texture pTexture, const char* szTexName, void* pImageData, int32_t width, int32_t height, uint32_t sourceType);
bool Texture_LoadHeightMap(Texture pTexture, const char* szTexName, void* pImageData, int32_t width, int32_t height, uint32_t sourceType, bool isBindless);
// 
void Texture_SetTextureFormats(Texture pTexture);
uint32_t Texture_GetSourceDataType(uint16_t precision);
uint32_t Texture_GetInternalFormat(Texture pTexture);
uint32_t Texture_GetPixelFormat(uint8_t channels);

// Accessors

/**
 * @brief Sets the bit-depth and data representation of the texture.
 *
 * @param pTexture Pointer to the texture object.
 * @param texPrecision The precision level (e.g., TEXTURE_PRECISION_INT8).
 */
static inline void Texture_SetTexturePrecision(Texture pTexture, uint16_t texPrecision)
{
	if (!pTexture)
	{
		return;
	}

	pTexture->texturePrecision = texPrecision;
}

/**
 * @brief Sets the OpenGL texture target.
 *
 * Defines how the texture is sampled and stored (e.g., GL_TEXTURE_2D,
 * GL_TEXTURE_CUBE_MAP, or GL_TEXTURE_3D).
 *
 * @param pTexture Pointer to the texture object.
 * @param texTarget The OpenGL enum representing the target.
 */
static inline void Texture_SetTextureTarget(Texture pTexture, uint32_t texTarget)
{
	if (!pTexture)
	{
		return;
	}

	pTexture->textureTarget = texTarget;
}

/**
 * @brief Sets the GPU internal storage format.
 *
 * This tells the GPU how to store the texels in VRAM (e.g., GL_RGBA8,
 * GL_RGBA16F, or GL_COMPRESSED_RGBA_S3TC_DXT5_EXT).
 *
 * @param pTexture Pointer to the texture object.
 * @param internalFormat The OpenGL enum for the internal storage.
 */
static inline void Texture_SetInternalFormat(Texture pTexture, uint32_t internalFormat)
{
	if (!pTexture)
	{
		return;
	}

	pTexture->internalFormat = internalFormat;
}

/**
 * @brief Sets the layout of the pixel data channels.
 *
 * Describes the order and presence of color components in the source
 * data (e.g., GL_RGB, GL_RGBA, or GL_BGRA).
 *
 * @param pTexture Pointer to the texture object.
 * @param pixelFormat The OpenGL enum for the pixel data layout.
 */
static inline void Texture_SetPixelFormat(Texture pTexture, uint32_t pixelFormat)
{
	if (!pTexture)
	{
		return;
	}

	pTexture->pixelFormat = pixelFormat;
}

/**
 * @brief Sets the data type of the source pixel components.
 *
 * Defines the CPU-side data type of each channel in the provided pixel
 * buffer (e.g., GL_UNSIGNED_BYTE for 8-bit or GL_FLOAT for 32-bit).
 *
 * @param pTexture Pointer to the texture object.
 * @param sourceType The OpenGL enum for the component data type.
 */
static inline void Texture_SetSourceTypeFormat(Texture pTexture, uint32_t sourceType)
{
	if (!pTexture)
	{
		return;
	}

	pTexture->sourceType = sourceType;
}

#endif