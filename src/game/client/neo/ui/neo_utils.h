#pragma once

#define VTF_PATH_MAX (512)

namespace NeoUtils
{
static constexpr int SPRAY_WH = 256;

// CropScaleTo256, crop + scales RGBA8888 data into 256x256 scaled RGBA8888 data.
// It will always allocate and return even if there's no crop/scaling. It will not
// alter/free rgba8888Data pointer.
[[nodiscard]] uint8 *CropScaleTo256(const uint8 *rgba8888Data, int width, int height);

// Convert input data into a sprayable - VTF DXT1/5 4 MIPS - DXT1 if no alpha, DXT5 if has alpha
// Input "rgba8888Data" has to be 256 x 256 RGBA8888
// NEO NOTE (nullsystem): This exists because the Source SDK API doesn't seem to generate a valid DXT1 VTF
void SerializeVTFDXTSprayToBuffer(CUtlBuffer *buffer, const uint8 *rgba8888Data);
}  // namespace NeoUtils

void bpr(int level, CUtlBuffer& buf, char const* fmt, ...);
