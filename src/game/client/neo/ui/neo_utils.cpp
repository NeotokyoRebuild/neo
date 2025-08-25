#include "neo_utils.h"

#include <filesystem.h>

#include "neo_misc.h"

#include <stb_image_resize2.h>
#include <stb_dxt.h>

uint8 *NeoUtils::CropScaleTo256(const uint8 *rgba8888Data, int width, int height)
{
	static constexpr int STRIDE = 4;

	uint8 *data = (uint8 *)(calloc(width * height, sizeof(uint8) * STRIDE));
	V_memcpy(data, rgba8888Data, width * height * STRIDE);

	// Crop to square
	if (width != height)
	{
		const int targetWH = min(width, height);
		uint8 *croppedData = (uint8 *)(calloc(targetWH * targetWH, sizeof(uint8) * STRIDE));

		const int dstYLines = targetWH * STRIDE;
		const int dstYEnd = dstYLines * targetWH;
		const int dstXOffset = (width < targetWH) ? (targetWH - width) : 0;
		const int dstYOffset = (height < targetWH) ? (targetWH - height) : 0;
		const int dstOffsetStart = (dstYOffset * dstYLines) + (dstXOffset * STRIDE);

		const int srcYLines = width * STRIDE;
		const int srcYEnd = srcYLines * height;
		const int srcXOffset = (width > targetWH) ? ((width / 2) - (targetWH / 2)) : 0;
		const int srcYOffset = (height > targetWH) ? ((height / 2) - (targetWH / 2)) : 0;
		const int srcOffsetStart = (srcYOffset * srcYLines) + (srcXOffset * STRIDE);

		const int xTakeMem = ((width > targetWH) ? targetWH : width) * STRIDE;

		for (int srcOffset = srcOffsetStart, dstOffset = dstOffsetStart;
			 (srcOffset < srcYEnd) && (dstOffset < dstYEnd);
			 srcOffset += srcYLines, dstOffset += dstYLines)
		{
			V_memcpy(croppedData + dstOffset, data + srcOffset, xTakeMem);
		}

		free(data);
		data = croppedData;
		width = targetWH;
		height = targetWH;
	}

	// Downscale to 256x256 (SPRAY_WH)
	if (width != SPRAY_WH || height != SPRAY_WH)
	{
		uint8 *downData = (uint8 *)(calloc(SPRAY_WH * SPRAY_WH, sizeof(uint8) * STRIDE));
		stbir_resize_uint8_linear(data, width, height, 0,
								  downData, SPRAY_WH, SPRAY_WH, 0,
								  STBIR_4CHANNEL); // Alternatively could use STBIR_RGBA?

		free(data);
		data = downData;
	}
	return data;
}

// Based on info from VPC: https://developer.valvesoftware.com/wiki/VTF_(Valve_Texture_Format)
// VTF 7.1 seemed to have a byte assigned to 1 but no idea what it is
void NeoUtils::SerializeVTFDXTSprayToBuffer(CUtlBuffer *buffer, const uint8 *data)
{
	static constexpr int MIP_TOTAL = 4;
	static constexpr int MIP_STARTWH = (SPRAY_WH / (1 << (MIP_TOTAL - 1)));

	// If there's no alpha values, output DXT1 for size-reason, otherwise output DXT5 to save alpha
	bool bHasAlpha = false;
	// Alpha value starts at offset 3
	for (int i = 3; i < (SPRAY_WH * SPRAY_WH * 4); i += 4)
	{
		if (data[i] < UINT8_MAX)
		{
			bHasAlpha = true;
			break;
		}
	}

	// VTF debugging compile definitions - kept so the VTF data can be checked when necessary:
	// Undef DXT_OUT will instead outputs RGBA8888. This is just to check if the headers and raw input/output are fine.
	// VTF_VER_MINOR 1 or 4 to switch between older and newer formats
#define DXT_OUT
#define VTF_VER_MINOR 4

#if (VTF_VER_MINOR == 1)
	static constexpr int HEADER_SIZE = 64;
#else
	static constexpr int HEADER_SIZE = 88;
#endif

	// Write out the header first
	{
		// Signature
		buffer->PutString("VTF");

		// Version
		buffer->PutUnsignedInt(7);
		buffer->PutUnsignedInt(VTF_VER_MINOR);

		// Header size
		buffer->PutUnsignedInt(HEADER_SIZE);

		// Width + height
		buffer->PutUnsignedShort(SPRAY_WH);
		buffer->PutUnsignedShort(SPRAY_WH);

		// Flags
		const int iFlags = TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_NOLOD;
		buffer->PutUnsignedInt(iFlags);

		// Frames
		buffer->PutUnsignedShort(1);

		// First frame
		buffer->PutUnsignedShort(0);

		// padding0 u8[4];
		for (int i = 0; i < 4; ++i) buffer->PutUnsignedChar(0);

		// reflectivity float[3] (aka vector)
		buffer->PutFloat(0.0f);
		buffer->PutFloat(0.0f);
		buffer->PutFloat(0.0f);

		// padding1 u8[4];
		for (int i = 0; i < 4; ++i) buffer->PutUnsignedChar(0);

		// bumpmapScale
#ifdef DXT_OUT
		buffer->PutFloat(0.0f);
#else
		buffer->PutFloat(1.0f);
#endif

		// highResImageFormat
#ifdef DXT_OUT
		buffer->PutInt(bHasAlpha ? IMAGE_FORMAT_DXT5 : IMAGE_FORMAT_DXT1);
#else
		buffer->PutInt(IMAGE_FORMAT_RGBA8888);
#endif

		// mipmapCount
		buffer->PutUnsignedChar(MIP_TOTAL);

		// lowResImage Format + Width + Height (Always DXT1 and 0x0/empty)
		buffer->PutInt(IMAGE_FORMAT_DXT1);
		buffer->PutUnsignedChar(0);
		buffer->PutUnsignedChar(0);

#if (VTF_VER_MINOR == 1)
		// Unknown, but required for VTF 7.1
		buffer->PutUnsignedChar(1);
#elif (VTF_VER_MINOR >= 2)
		// 7.2
		// depth
		buffer->PutUnsignedShort(1);

#if (VTF_VER_MINOR >= 3)
		// 7.3+
		// padding
		for (int i = 0; i < 3; ++i) buffer->PutUnsignedChar(0);

		// Number of resources
		buffer->PutUnsignedInt(1);

		// padding
		for (int i = 0; i < 8; ++i) buffer->PutUnsignedChar(0);
#endif
#endif
	}
#if (VTF_VER_MINOR >= 3)
	{
		// Resource infos
		// tag - u8[3] - Three-byte tag that identifies what this resource is
		// x30 0 0 = High res image data
		buffer->PutUnsignedChar('\x30');
		buffer->PutUnsignedChar('\0');
		buffer->PutUnsignedChar('\0');

		// flag
		buffer->PutUnsignedChar(0);

		// offset
		buffer->PutUnsignedInt(HEADER_SIZE);
	}
#endif

	// Start of image data
	// Usually low res starts here but it's empty so skipped to high res
	// For each mipmaps - from 4 to 1 (smallest to highest res)
	//     For each frame - but there's only 1
	//         For each face - but there's only 1
	//             For each Z slice (min to max) - but there's only 1
	//                 "High res" data
	int mipWidth = MIP_STARTWH;
	int mipHeight = MIP_STARTWH;
	uchar *mipData = reinterpret_cast<uchar *>(malloc(SPRAY_WH * SPRAY_WH * 4));
	// In VTF, mip starts from the smallest resolution (32px for 4-MIP-256px) to the highest one (256px)
	for (int mip = MIP_TOTAL; mip >= 1; --mip, mipWidth *= 2, mipHeight *= 2)
	{
		if (mip == 1)
		{
			V_memcpy(mipData, data, SPRAY_WH * SPRAY_WH * 4);
		}
		else
		{
			V_memset(mipData, 0, SPRAY_WH * SPRAY_WH * 4);
			stbir_resize_uint8_linear(data, SPRAY_WH, SPRAY_WH, 0,
									  mipData, mipWidth, mipHeight, 0,
									  STBIR_4CHANNEL);
		}

#ifdef DXT_OUT
		const int mipStride = mipWidth * 4;
		for (int cy = 0; cy < mipHeight; cy += 4)
		{
			for (int cx = 0; cx < mipWidth; cx += 4)
			{
				int inOffset = 0;
				unsigned char ucBufIn[64];
				for (int by = 0; by < 4; ++by)
				{
					const int y = cy + by;
					for (int bx = 0; bx < 4; ++bx)
					{
						const int x = cx + bx;
						V_memcpy(ucBufIn + inOffset,
								 mipData + ((y * mipStride) + (x * 4)),
								 4);
						inOffset += 4;
					}
				}
				Assert(inOffset == 64);

				// DXT1 blocks (8-sized) if no transparency needed, or DXT5 blocks (16-sized) if there is
				unsigned char ucBufOut[16];
				stb_compress_dxt_block(ucBufOut, ucBufIn, bHasAlpha, STB_DXT_HIGHQUAL);
				buffer->Put(ucBufOut, bHasAlpha ? 16 : 8);
			}
		}
#else
		// RGBA8888 data
		buffer->Put(mipData, mipWidth * mipHeight * 4);
#endif
	}
	free(mipData);
}

void bpr( int level, CUtlBuffer& buf, char const *fmt, ... )
{
	char txt[ 4096 ];
	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf( txt, sizeof( txt ) - 1, fmt, argptr );
	va_end( argptr );

	int indent = 2;
	for ( int i = 0; i < ( indent * level ); ++i )
	{
		buf.Printf( " " );
	}
	buf.Printf( "%s", txt );
}
