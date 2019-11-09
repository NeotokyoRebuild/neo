// ----------------------------------------------------------------------------
// Neo_TestShader.cpp
// 
// Adapted from the tutorial:
// https://developer.valvesoftware.com/wiki/Source_SDK_2013:_Your_First_Shader
//
// This file defines the C++ component of the example shader.
// ----------------------------------------------------------------------------

// Must include this. Contains a bunch of macro definitions along with the
// declaration of CBaseShader.
#include "BaseVSShader.h"

// Vertex shaders convention: ..._vs<shader model ver>...
// Pixel shaders convention: ..._ps<shader model ver>...

// We're going to be making a screenspace effect. Therefore, we need the
// screenspace vertex shader.
#include "SDK_screenspaceeffect_vs20.inc"

// We also need to include the pixel shader for our own shader.
// Note that the shader compiler generates both 2.0 and 2.0b versions.
// Need to include both.
#include "neo_testshader_ps20.inc"
#include "neo_testshader_ps20b.inc"

BEGIN_SHADER(Neo_TestShader, "Help string for shader.")

	// ----------------------------------------------------------------------------
	// This block is where you'd define inputs that users can feed to your
	// shader.
	// ----------------------------------------------------------------------------
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	// ----------------------------------------------------------------------------
	// This is the shader initialization block.
	// ----------------------------------------------------------------------------
	SHADER_INIT
	{

	}

	// ----------------------------------------------------------------------------
	// We want this shader to operate on the frame buffer itself. Therefore,
	// we need to set this to true.
	// ----------------------------------------------------------------------------
	bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame /* = true */) const
	{
		return true;
	}
	/*
	// Could we also do this, instead? Perf implications?
	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2(MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE);
	}
	*/

	// ----------------------------------------------------------------------------
	// This block should return the name of the shader to fall back to if
	// we fail to bind this shader for any reason.
	// ----------------------------------------------------------------------------
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if (g_pHardwareConfig->GetDXSupportLevel() < 90)
		{
			Assert(0);
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		// ----------------------------------------------------------------------------
		// This section is called when the shader is bound for the first time.
		// You should setup any static state variables here.
		// ----------------------------------------------------------------------------
		SHADOW_STATE
		{
			// Setup the vertex format.
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);

			// We don't need to write to the depth buffer.
			pShaderShadow->EnableDepthWrites(false);

			// Precache and set the screenspace shader.
			DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
			SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

			// Precache and set the example shader.
			if (g_pHardwareConfig->SupportsPixelShaders_2_b())
			{
				DECLARE_STATIC_PIXEL_SHADER(neo_testshader_ps20b);
				SET_STATIC_PIXEL_SHADER(neo_testshader_ps20b);
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER(neo_testshader_ps20);
				SET_STATIC_PIXEL_SHADER(neo_testshader_ps20);
			}
		}

		// ----------------------------------------------------------------------------
		// This section is called every frame.
		// ----------------------------------------------------------------------------
		DYNAMIC_STATE
		{
			// Use the sdk_screenspaceeffect_vs20 vertex shader.
			DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
			SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

			// Use our custom pixel shader.
			if (g_pHardwareConfig->SupportsPixelShaders_2_b())
			{
				DECLARE_DYNAMIC_PIXEL_SHADER(neo_testshader_ps20b);
				SET_DYNAMIC_PIXEL_SHADER(neo_testshader_ps20b);
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER(neo_testshader_ps20);
				SET_DYNAMIC_PIXEL_SHADER(neo_testshader_ps20);
			}
		}

	Draw(); // Need to call for each draw

	}

END_SHADER
