#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

// NEO NOTE (nullsystem): miniaudio.h have been
// altered slightly around "NEO EDIT..." comments
// inside miniaudio.h just to expose the ma_dr_mp3_...
// APIs
#define MA_NO_FLAC
#define MA_NO_WAV
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

