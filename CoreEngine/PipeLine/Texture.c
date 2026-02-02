#include "Texture.h"
#include <glad/glad.h>
#include <stb_image/stb_image.h>
#include <math.h>
#include "Utils.h"
#include "../Core/CoreUtils.h"

bool Texture_Initialize(Texture* ppTexture)
{
	// This checks: "Did the user give me a valid address to write into?"
	// If you called Texture_Initialize(NULL), this would be true.
	if (!ppTexture)
	{
		syserr("ppTexture is NULL (invalid address)");
		return false;
	}

	// overwrite the NULL value inside your struct with new memory
	*ppTexture = tracked_calloc(1, sizeof(STexture));

	// Did the allocation actually work?
	if (*ppTexture == NULL)
	{
		syserr("Failed to Allocate Texture Memory");
		return (false);
	}

	// Default Initialize Values
	(*ppTexture)->textureTarget = GL_TEXTURE_2D;		// Texture target (e.g., GL_TEXTURE_2D)
	(*ppTexture)->sourceType = GL_UNSIGNED_BYTE;	// Data type of the source image data (e.g., GL_UNSIGNED_BYTE)
	(*ppTexture)->texturePrecision = TEXTURE_PRECISION_INT8;
	(*ppTexture)->minFilter = GL_LINEAR;
	(*ppTexture)->magFilter = GL_LINEAR;
	(*ppTexture)->wrapS = GL_REPEAT;
	(*ppTexture)->wrapT = GL_REPEAT;

	// Successfully Allocated with 0 value memory
	return (true);
}

void Texture_Destroy(Texture* ppTexture)
{
	if (!ppTexture || !*ppTexture)
	{
		return;
	}

	Texture pTexture = *ppTexture;

	// Before Anything Make it UnResident
	if (pTexture->isResident)
	{
		Texture_MakeResident(pTexture, false);
	}

	// Clean up the GPU resource
	GL_DeleteTexture(&pTexture->textureID);

	if (pTexture->imageData.szTexturePath)
	{
		tracked_free(pTexture->imageData.szTexturePath);
		pTexture->imageData.szTexturePath = NULL;
	}
	if (pTexture->imageData.szTextureName)
	{
		tracked_free(pTexture->imageData.szTextureName);
		pTexture->imageData.szTextureName = NULL;
	}
	if (pTexture->imageData.pData && pTexture->isRawTexture == false)
	{
		tracked_free(pTexture->imageData.pData);
		pTexture->imageData.pData = NULL;
	}

	tracked_free(pTexture);

	*ppTexture = NULL;
}

bool Texture_MakeResident(Texture pTexture, bool bSetResident)
{
	if (pTexture->isBindless == false)
	{
		syserr("Texture is not set Bindless, return true");
		return (true);
	}

	if (!GLAD_GL_ARB_bindless_texture)
	{
		pTexture->textureHandle = 0;
		pTexture->isResident = false;
		syserr("Bindless Textures not supported on this GPU/Driver!");
		return (false);
	}

	if (pTexture->textureID == 0)
	{
		syserr("Texture ID is 0, cannot manage residency");
		return (false);
	}

	// We only ask OpenGL for the GPU address ONCE in the texture's life.
	if (pTexture->textureHandle == 0)
	{
		pTexture->textureHandle = glGetTextureHandleARB(pTexture->textureID);

		if (pTexture->textureHandle == 0)
		{
			syserr("Failed to get Bindless Handle for Texture %d", pTexture->textureID);
			return (false);
		}
	}

	// Toggle Residency
	if (bSetResident)
	{
		if (pTexture->isResident == false)
		{
			// Get the 64-bit GPU address
			// Important: Once a handle is generated for a texture, it is permanent for the life of that texture ID.
			// You don't actually need to get it again if you just want to toggle residency.
			glMakeTextureHandleResidentARB(pTexture->textureHandle);
			pTexture->isResident = true;
		}
	}
	else
	{
		if (pTexture->isResident == true)
		{
			// No need to check glIsTextureHandleResidentARB (it's slow)
			// Just tell the driver this handle is no longer in use
			glMakeTextureHandleNonResidentARB(pTexture->textureHandle);
			pTexture->isResident = false;
		}
	}

	return (true);
}


/**
 * @brief Loads an image file from disk into a simple data structure.
 *
 * This is a utility function that uses the stbi_load library to read
 * an image file. It forces the image to have 4 channels (RGBA).
 *
 * @param filepath The path to the image file.
 * @return An ImageData struct containing the raw pixel data and dimensions.
 */
bool Texture_LoadImage(Texture pTexture, const char* szTexturePath)
{
	if (pTexture == NULL)
	{
		return (false);
	}

	if (File_IsFileExists(szTexturePath) == false)
	{
		syserr("Failed to Find file: %s", szTexturePath);
		return (false);
	}

	const char* szTexExtension = File_GetExtension(szTexturePath);
	if ((strcmp(szTexExtension, "ktx") == 0) || (strcmp(szTexExtension, "dds") == 0))
	{
		pTexture->isCompressed = true;
	}

	// Ensure that images are not flipped vertically upon loading.
	stbi_set_flip_vertically_on_load(false);
	pTexture->imageData.szTexturePath = tracked_strdup(szTexturePath);

	char nameBuffer[MAX_STRING_LEN];
	File_GetFileNameNoExtension(szTexturePath, nameBuffer, sizeof(nameBuffer));
	pTexture->imageData.szTextureName = tracked_strdup(nameBuffer);

	// Load the image, forcing 4 color channels (RGBA).
	if (pTexture->texturePrecision == TEXTURE_PRECISION_FLOAT16 || pTexture->texturePrecision == TEXTURE_PRECISION_FLOAT32)
	{
		pTexture->imageData.pData = stbi_loadf(szTexturePath, &pTexture->imageData.width, &pTexture->imageData.height, &pTexture->imageData.channels, 0); // should we make it 4?
		pTexture->sourceType = GL_FLOAT; // stbi_loadf always returns 32-bit floats
	}
	else
	{
		pTexture->imageData.pData = stbi_load(szTexturePath, &pTexture->imageData.width, &pTexture->imageData.height, &pTexture->imageData.channels, 0); // should we make it 4?
		pTexture->sourceType = GL_UNSIGNED_BYTE; // stbi_load returns 8-bit bytes
	}

	if (pTexture->imageData.pData == NULL)
	{
		tracked_free(pTexture->imageData.szTexturePath);
		tracked_free(pTexture->imageData.szTextureName);

		syserr("Failed to load stb image");
		return (false);
	}

	pTexture->imageData.dataSize = (size_t)pTexture->imageData.width * (size_t)pTexture->imageData.height * (size_t)pTexture->imageData.channels;
	pTexture->imageData.isLoaded = true;

	return (true);
}

/**
 * @brief Loads texture data from a file.
 *
 * @param filePath The file path to load the texture from.
 * @return true if the texture was loaded successfully, false otherwise.
 */
bool Texture_Load(Texture pTexture, const char* szTexturePath)
{
	if (!pTexture)
	{
		return (false);
	}

	if (!Texture_LoadImage(pTexture, szTexturePath))
	{
		Texture_Destroy(&pTexture);
		return (false);
	}

	// Create the OpenGL Resource (The ID)
	if (!GL_CreateTexture(&pTexture->textureID, pTexture->textureTarget))
	{
		Texture_Destroy(&pTexture);
		syserr("Failed to Generate GL Texture");
		return (false);
	}

	// Upload to GPU
	// This moves the data from RAM (m_pData) to VRAM
	if (!Texture_UploadToGPU(pTexture))
	{
		Texture_Destroy(&pTexture);
		syserr("Failed to Upload Texture to GPU");
		return (false);
	}

	if (!Texture_MakeResident(pTexture, pTexture->isBindless))
	{
		Texture_Destroy(&pTexture);
		syserr("Failed to Generate Texture Handle (Bindless)");
		return (false);
	}

	pTexture->isLoaded = true;

	syslog("Loaded Texture: %s (W:%d H:%d Channels:%d ID:%d Bindless:%d", pTexture->imageData.szTextureName, pTexture->imageData.width, pTexture->imageData.height, pTexture->imageData.channels, pTexture->textureID, pTexture->isBindless);
	return (true);
}

/**
 * @brief Uploads texture data to the GPU.
 *
 * Uses Direct State Access (DSA) on modern drivers or legacy binding methods
 * as a fallback to ensure cross-version compatibility.
 *
 * @param uiTextureID Reference to store the created texture ID.
 * @param sImageData The image data to upload to the GPU.
 * @param eTextureTarget The target texture type (e.g., GL_TEXTURE_2D).
 * @param bGenMipMaps Whether to generate mipmaps for the texture.
 * @param sourceType The data type of the source image data (e.g., GL_UNSIGNED_BYTE).
 * @return true if the texture was created and data uploaded successfully, false otherwise.
 */
bool Texture_UploadToGPU(Texture pTexture)
{
	if (!pTexture)
	{
		return (false);
	}

	// Prevent Immutable Storage re-allocation crashes
	if (pTexture->isLoaded == true)
	{
		syserr("Texture already on GPU. Delete first to re-upload.");
		return (false);
	}

	if (pTexture->imageData.isLoaded == false)
	{
		syserr("Failed to Upload to GPU, load the texture first!");
		return (false);
	}

	if (pTexture->textureID == 0)
	{
		if (!GL_CreateTexture(&pTexture->textureID, pTexture->textureTarget))
		{
			syserr("Failed to Generate GL Texture");
			return (false);
		}
	}

	// Calculate the total number of mipmap levels required for a full chain.
	// A full chain starts at the original resolution and ends at a 1x1 pixel.
	if (pTexture->isMipMap)
	{
		// Get the largest dimension to ensure the mip chain covers the entire image.
		// We use float for the math functions (log2f/floorf).
		float mipMapSize = fmaxf((float)pTexture->imageData.width, (float)pTexture->imageData.height);

		/* * The number of times a dimension can be divided by 2 is log2(size).
		 * floorf() handles non-power-of-two dimensions (e.g., 1000px).
		 * We add 1 to include the base level (Level 0).
		 * Example: 1024px -> log2 is 10.0 -> 10 + 1 = 11 levels.
		 */
		pTexture->mipMapLevels = (int32_t)floorf(log2f(mipMapSize)) + 1;
	}
	else
	{
		pTexture->mipMapLevels = 1;
	}

	Texture_SetTextureFormats(pTexture);

	if (IsGLVersionHigher(4, 5))
	{
		return (Texture_DSAUploadToGPU(pTexture));
	}
	else
	{
		return (Texture_LegacyUploadToGPU(pTexture));
	}
}

/**
 * @brief Uploads texture data to the GPU.
 *
 * Uses Direct State Access (DSA) on modern drivers or legacy binding methods
 * as a fallback to ensure cross-version compatibility.
 *
 * @param uiTextureID Reference to store the created texture ID.
 * @param sImageData The image data to upload to the GPU.
 * @param eTextureTarget The target texture type (e.g., GL_TEXTURE_2D).
 * @param bGenMipMaps Whether to generate mipmaps for the texture.
 * @param sourceType The data type of the source image data (e.g., GL_UNSIGNED_BYTE).
 * @return true if the texture was created and data uploaded successfully, false otherwise.
 */
bool Texture_DSAUploadToGPU(Texture pTexture)
{
	if (pTexture->textureTarget == GL_TEXTURE_2D)
	{
		// Reset Swizzle to standard
		GLint defaultSwizzle[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
		glTextureParameteriv(pTexture->textureID, GL_TEXTURE_SWIZZLE_RGBA, defaultSwizzle);

		// 1. Allocate Immutable Storage
		glTextureStorage2D(pTexture->textureID, pTexture->mipMapLevels, pTexture->internalFormat, pTexture->imageData.width, pTexture->imageData.height);

		// 2. Upload Data - USE sourceType HERE!
		if (pTexture->imageData.pData)
		{
			glTextureSubImage2D(pTexture->textureID, 0, 0, 0, pTexture->imageData.width, pTexture->imageData.height, pTexture->pixelFormat, pTexture->sourceType, pTexture->imageData.pData);
		}

		// 3. Handle Grayscale Swizzle
		if (pTexture->imageData.channels == 1 && pTexture->isSwizzle)
		{
			GLint greySwizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTextureParameteriv(pTexture->textureID, GL_TEXTURE_SWIZZLE_RGBA, greySwizzle);
		}
	}

	// Parameters
	if (pTexture->isMipMap)
	{
		pTexture->minFilter = GL_LINEAR_MIPMAP_LINEAR;
	}
	glTextureParameteri(pTexture->textureID, GL_TEXTURE_MIN_FILTER, pTexture->minFilter);
	glTextureParameteri(pTexture->textureID, GL_TEXTURE_MAG_FILTER, pTexture->magFilter);
	glTextureParameteri(pTexture->textureID, GL_TEXTURE_WRAP_S, pTexture->wrapS);
	glTextureParameteri(pTexture->textureID, GL_TEXTURE_WRAP_T, pTexture->wrapT);

	if (pTexture->isMipMap == true && pTexture->mipMapLevels > 1)
	{
		glGenerateTextureMipmap(pTexture->textureID);
	}

	return (true);
}

/**
 * @brief Uploads texture data to the GPU using legacy binding methods.
 *
 * This function creates a new OpenGL texture using legacy binding methods,
 * uploads the provided image data, and sets texture parameters such as filtering
 * and wrapping. It supports generating mipmaps if requested.
 *
 * @param uiTextureID Reference to store the created texture ID.
 * @param sImageData The image data to upload to the GPU.
 * @param eTextureTarget The target texture type (e.g., GL_TEXTURE_2D).
 * @param bGenMipMaps Whether to generate mipmaps for the texture.
 * @param sourceType The data type of the source image data (e.g., GL_UNSIGNED_BYTE).
 * @return true if the texture was created and data uploaded successfully, false otherwise.
 */
bool Texture_LegacyUploadToGPU(Texture pTexture)
{
	glBindTexture(pTexture->textureTarget, pTexture->textureID);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // "My rows are packed tightly. Don't look for 4-byte boundaries; just start the next row exactly where the last one ended."

	// Upload Data (Mutable Storage)
	// Note: Legacy uses glTexImage2D. The '0' is the base mip level, unlike DSA
	glTexImage2D(pTexture->textureTarget, 0, pTexture->internalFormat, pTexture->imageData.width, pTexture->imageData.height, 0, pTexture->pixelFormat, pTexture->sourceType, pTexture->imageData.pData);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // reset to normal

	// Set Parameters (Using target-based functions)
	if (pTexture->isMipMap)
	{
		pTexture->minFilter = GL_LINEAR_MIPMAP_LINEAR;
	}
	glTexParameteri(pTexture->textureTarget, GL_TEXTURE_MIN_FILTER, pTexture->minFilter);
	glTexParameteri(pTexture->textureTarget, GL_TEXTURE_MAG_FILTER, pTexture->magFilter);
	glTexParameteri(pTexture->textureTarget, GL_TEXTURE_WRAP_S, pTexture->wrapS);
	glTexParameteri(pTexture->textureTarget, GL_TEXTURE_WRAP_T, pTexture->wrapT);

	// Handle Swizzle (If 1-channel)
	if (pTexture->imageData.channels == 1)
	{
		GLint greySwizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
		glTexParameteriv(pTexture->textureTarget, GL_TEXTURE_SWIZZLE_RGBA, greySwizzle);
	}

	// Generate Mipmaps if needed
	if (pTexture->isMipMap && pTexture->mipMapLevels > 1)
	{
		glGenerateMipmap(pTexture->textureTarget);
	}


	// Unbind to avoid accidental state changes elsewhere
	glBindTexture(pTexture->textureTarget, 0);

	return (true);
}

bool Texture_LoadHeightMapImage(Texture pTexture, const char* szTexName, void* pImageData, int32_t width, int32_t height, uint32_t sourceType)
{
	if (pTexture == NULL)
	{
		return (false);
	}

	if (pImageData == NULL)
	{
		syserr("Texture Data is not set.");
		return (false);
	}

	pTexture->imageData.width = width;
	pTexture->imageData.height = height;
	pTexture->imageData.channels = 1;
	pTexture->imageData.szTexturePath = tracked_strdup("VirtualTexture");
	pTexture->imageData.szTextureName = tracked_strdup(szTexName);
	// Load the image, forcing 4 color channels (RGBA).
	pTexture->imageData.pData = pImageData;
	pTexture->imageData.dataSize = (size_t)pTexture->imageData.width * (size_t)pTexture->imageData.height * (size_t)pTexture->imageData.channels;
	pTexture->imageData.isLoaded = true;

	return (true);
}

bool Texture_LoadHeightMap(Texture pTexture, const char* szTexName, void* pImageData, int32_t width, int32_t height, uint32_t sourceType, bool isBindless)
{
	if (!Texture_LoadHeightMapImage(pTexture, szTexName, pImageData, width, height, sourceType))
	{
		Texture_Destroy(&pTexture);
		syserr("Faile to set Height Map Data");
		return (false);
	}

	pTexture->texturePrecision = TEXTURE_PRECISION_FLOAT32;
	pTexture->internalFormat = GL_R32F;
	pTexture->pixelFormat = GL_RED;
	pTexture->sourceType = GL_FLOAT;

	pTexture->minFilter = GL_LINEAR;
	pTexture->magFilter = GL_LINEAR;
	pTexture->wrapS = GL_CLAMP_TO_EDGE;
	pTexture->wrapT = GL_CLAMP_TO_EDGE;
	pTexture->isMipMap = false;
	pTexture->isBindless = isBindless;
	pTexture->isSwizzle = false;
	pTexture->isRawTexture = true; // prevent deleteing Image.pData

	// Create the OpenGL Resource (The ID)
	if (!GL_CreateTexture(&pTexture->textureID, pTexture->textureTarget))
	{
		Texture_Destroy(&pTexture);
		syserr("Failed to Generate GL Texture");
		return (false);
	}

	// Upload to GPU
	// This moves the data from RAM (m_pData) to VRAM
	if (!Texture_UploadToGPU(pTexture))
	{
		Texture_Destroy(&pTexture);
		syserr("Failed to Upload Texture to GPU");
		return (false);
	}

	if (!Texture_MakeResident(pTexture, pTexture->isBindless))
	{
		Texture_Destroy(&pTexture);
		syserr("Failed to Generate Texture Handle (Bindless)");
		return (false);
	}

	pTexture->isLoaded = true;

	syslog("Loaded HeightMap Texture: %s (W:%d H:%d Channels:%d ID:%d Bindless:%d", pTexture->imageData.szTextureName, pTexture->imageData.width, pTexture->imageData.height, pTexture->imageData.channels, pTexture->textureID, pTexture->isBindless);
	return (true);
}

/**
 * @brief Synchronizes all OpenGL-specific format enums based on current texture state.
 *
 * This should be called after loading image metadata but before allocating GPU storage.
 *
 * @param pTexture Pointer to the texture object to synchronize.
 */
void Texture_SetTextureFormats(Texture pTexture)
{
	if (pTexture == NULL)
	{
		return;
	}

	pTexture->internalFormat = Texture_GetInternalFormat(pTexture);
	pTexture->pixelFormat = Texture_GetPixelFormat(pTexture->imageData.channels);
	pTexture->sourceType = Texture_GetSourceDataType(pTexture->texturePrecision);
}

/**
 * @brief Maps engine precision to OpenGL source data types.
 */
uint32_t Texture_GetSourceDataType(uint16_t precision)
{
	switch (precision)
	{
	case TEXTURE_PRECISION_INT8:
		return (GL_UNSIGNED_BYTE);
	case TEXTURE_PRECISION_INT16:
		return (GL_UNSIGNED_SHORT);
		break;
	case TEXTURE_PRECISION_FLOAT16:
		return (GL_HALF_FLOAT);
		break;
	case TEXTURE_PRECISION_FLOAT32:
		return (GL_FLOAT);
		break;
	default:
		return (GL_UNSIGNED_BYTE);
		break;
	}
}

/**
 * @brief Resolves the OpenGL internal storage format based on precision and channels.
 *
 * This function determines the exact hardware format for VRAM allocation.
 * It handles the distinction between 8-bit, 16-bit, and 32-bit types.
 *
 * @param pTexture Pointer to the texture containing precision and channel data.
 * @return The OpenGL internal format (e.g., GL_RGBA16F, GL_R8).
 */
uint32_t Texture_GetInternalFormat(Texture pTexture)
{
	int32_t channels = pTexture->imageData.channels;
	uint16_t precision = pTexture->texturePrecision;

	switch (precision)
	{
	case TEXTURE_PRECISION_INT8:
		if (channels == 1) return GL_R8;
		if (channels == 2) return GL_RG8;
		if (channels == 3) return GL_RGB8;
		if (channels == 4) return GL_RGBA8;
		break;

	case TEXTURE_PRECISION_INT16:
		if (channels == 1) return GL_R16;
		if (channels == 2) return GL_RG16;
		if (channels == 3) return GL_RGB16;
		if (channels == 4) return GL_RGBA16;
		break;

	case TEXTURE_PRECISION_FLOAT16:
		if (channels == 1) return GL_R16F;
		if (channels == 2) return GL_RG16F;
		if (channels == 3) return GL_RGB16F;
		if (channels == 4) return GL_RGBA16F;
		break;

	case TEXTURE_PRECISION_FLOAT32:
		if (channels == 1) return GL_R32F;
		if (channels == 2) return GL_RG32F;
		if (channels == 3) return GL_RGB32F;
		if (channels == 4) return GL_RGBA32F;
		break;
	}

	syslog("Warning: Format resolution failed for %d channels. Defaulting to RGBA8.", channels);
	return GL_RGBA8;
}

/**
 * @brief Resolves the pixel layout format (CPU-side layout).
 *
 * @param channels Number of color channels in the source data.
 * @return The OpenGL pixel format (e.g., GL_RED, GL_RGB, GL_RGBA).
 */
uint32_t Texture_GetPixelFormat(uint8_t channels)
{
	switch (channels)
	{
	case 1:  return GL_RED;
	case 2:  return GL_RG;
	case 3:  return GL_RGB;
	case 4:  return GL_RGBA;
	default: return GL_RGBA;
	}
}