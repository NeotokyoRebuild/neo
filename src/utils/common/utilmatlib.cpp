//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

// C callable material system interface for the utils.

#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include <cmdlib.h>
#include "utilmatlib.h"
#include "tier0/dbg.h"
//#include <windows.h>
#include "filesystem.h"
#include "materialsystem/materialsystem_config.h"
#include "mathlib/mathlib.h"

#if defined(POSIX) && defined(USE_SDL)
#include <appframework/ilaunchermgr.h>

static CreateInterfaceFn g_fileSystemFactory;

class FakeLauncherMgr : public ILauncherMgr
{
public:
	FakeLauncherMgr()
	{
		setbuf(stdout, NULL);
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual bool Connect( CreateInterfaceFn factory )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return true;
	}

	virtual void Disconnect()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void *QueryInterface( const char *pInterfaceName )
	{
		printf("%s\n", __PRETTY_FUNCTION__);

		if (V_strcmp(pInterfaceName, SDLMGR_INTERFACE_VERSION) == 0)
		{
			return this;
		}

		return nullptr;
	}

	virtual InitReturnVal_t Init()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		//return (InitReturnVal_t) 0;
		return INIT_OK;
	}

	virtual void Shutdown()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual bool CreateGameWindow( const char *pTitle, bool bWindowed, int width, int height )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return true;
	}

	virtual void IncWindowRefCount()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void DecWindowRefCount()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual int GetEvents( CCocoaEvent *pEvents, int nMaxEventsToReturn, bool debugEvents = false )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return 0;
	}

#ifdef LINUX
	virtual int PeekAndRemoveKeyboardEvents( bool *pbEsc, bool *pbReturn, bool *pbSpace, bool debugEvents = false )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return 0;
	}
#endif

	virtual void SetCursorPosition( int x, int y )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void SetWindowFullScreen( bool bFullScreen, int nWidth, int nHeight )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual bool IsWindowFullScreen()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return true;
	}

	virtual void MoveWindow( int x, int y )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void SizeWindow( int width, int tall )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void PumpWindowsMessageLoop()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void DestroyGameWindow()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}
	virtual void SetApplicationIcon( const char *pchAppIconFile )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void GetMouseDelta( int &x, int &y, bool bIgnoreNextMouseDelta = false )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void GetNativeDisplayInfo( int nDisplay, uint &nWidth, uint &nHeight, uint &nRefreshHz )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void RenderedSize( uint &width, uint &height, bool set )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void DisplayedSize( uint &width, uint &height )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

#if defined( DX_TO_GL_ABSTRACTION )
	virtual PseudoGLContextPtr	GetMainContext()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return NULL;
	}

	virtual PseudoGLContextPtr GetGLContextForWindow( void* windowref )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return NULL;
	}

	virtual PseudoGLContextPtr CreateExtraContext()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return NULL;
	}

	virtual void DeleteContext( PseudoGLContextPtr hContext )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual bool MakeContextCurrent( PseudoGLContextPtr hContext )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return true;
	}

	virtual GLMDisplayDB *GetDisplayDB( void )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return NULL;
	}

	virtual void GetDesiredPixelFormatAttribsAndRendererInfo( uint **ptrOut, uint *countOut, GLMRendererInfoFields *rendInfoOut )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void ShowPixels( CShowPixelsParams *params )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}
#endif

	virtual void GetStackCrawl( CStackCrawlParams *params )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void WaitUntilUserInput( int msSleepTime )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void *GetWindowRef()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return NULL;
	}

	virtual void SetMouseVisible( bool bState )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void SetMouseCursor( SDL_Cursor *hCursor )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void SetForbidMouseGrab( bool bForbidMouseGrab )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void OnFrameRendered()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual void SetGammaRamp( const uint16 *pRed, const uint16 *pGreen, const uint16 *pBlue )
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	virtual double GetPrevGLSwapWindowTime()
	{
		printf("%s\n", __PRETTY_FUNCTION__);
		return 0.0;
	}

} g_FakeLauncherMgr;

#endif

#ifdef USE_SDL
void* SDLMgrFactoryRedirector( const char *pName, int *pReturnCode )
{
	printf("!!! SDLMgrFactoryRedirector pName=%s\n", pName);

	if (strcmp(SDLMGR_INTERFACE_VERSION, pName) == 0)
	{
		return &g_FakeLauncherMgr;
	}

	// materialsystem.so's Connect() resolves its own interface from the factory
	// we pass to Init(). Our fileSystemFactory doesn't know about it, so hand
	// back the material system pointer we already obtained above; otherwise
	// materialsystem caches a NULL self-pointer and crashes dereferencing it.
	if (strcmp(MATERIAL_SYSTEM_INTERFACE_VERSION, pName) == 0)
	{
		return g_pMaterialSystem;
	}

	return g_fileSystemFactory(pName, pReturnCode);

}
#endif

void LoadMaterialSystemInterface( CreateInterfaceFn fileSystemFactory )
{
	if( g_pMaterialSystem )
		return;

#if defined(POSIX) && defined(USE_SDL)
	// The materialsystem shared object currently asks our fileSystemFactory for
	// the SDLMgrInterface001 interface, which it doesn't provide. After that,
	// materialsystem self-destructs because it can't load our shaderapiempty
	// shared object. However, materialsystem doesn't really need that interface
	// so we'll simply set up a redirector that catches the request and returns
	// a dummy object it won't really use.
	g_fileSystemFactory = fileSystemFactory;
	fileSystemFactory = SDLMgrFactoryRedirector;
#endif
	
	// materialsystem library should be in the path, it's in bin along with vbsp.

#ifdef WIN32
	const char *pMaterialSystemLibraryName = "materialsystem.dll";
#else
	const char *pMaterialSystemLibraryName = "materialsystem.so";
#endif

	CSysModule *materialSystemDLLHInst;
	materialSystemDLLHInst = g_pFullFileSystem->LoadModule( pMaterialSystemLibraryName );
	if( !materialSystemDLLHInst )
	{
		Error( "Can't load %s", pMaterialSystemLibraryName );
	}

	CreateInterfaceFn clientFactory = Sys_GetFactory( materialSystemDLLHInst );
	if ( clientFactory )
	{
		int pReturnCode;
		g_pMaterialSystem = (IMaterialSystem *)clientFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, &pReturnCode );
		if ( !g_pMaterialSystem )
		{
			Error( "Could not get the material system interface from %s (" __FILE__ ")", pMaterialSystemLibraryName );
		}

		// if ( !g_pMaterialSystemHardwareConfig )
		// {
		// 	g_pMaterialSystemHardwareConfig = ( IMaterialSystemHardwareConfig * )clientFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, NULL );
		// 	if ( !g_pMaterialSystemHardwareConfig )
		// 	{
		// 		Error( "Could not get the material system interface from %s (" __FILE__ ")", pMaterialSystemLibraryName );
		// 	}
		// }
		// else
		// {
		// 	Error("!!! g_pMaterialSystemHardwareConfig is not null");
		// }
	}
	else
	{
		Error( "Could not find factory interface in library %s", pMaterialSystemLibraryName );
	}

#ifdef WIN32
	const char *pShaderApiEmptyLibraryName = "shaderapiempty.dll";
#else
	const char *pShaderApiEmptyLibraryName = "shaderapiempty.so"; // shaderapiempty shaderapivk shaderapidx9
#endif

#if 1
	//Warning("!!! g_pMaterialSystem->GetRenderBackend()=%s\n", GetRenderBackendName(g_pMaterialSystem->GetRenderBackend()));

	if (!g_pMaterialSystem->Init( pShaderApiEmptyLibraryName, 0, fileSystemFactory ))
	{
		Error( "Could not start the empty shader (%s)!", pShaderApiEmptyLibraryName );
	}
#else
	g_pMaterialSystem->SetShaderAPI(pShaderApiEmptyLibraryName);


	if (!g_pMaterialSystem->Init())
	{
		Error( "Could not start the empty shader (%s)!", pShaderApiEmptyLibraryName );
	}
#endif
}

void InitMaterialSystem( const char *materialBaseDirPath, CreateInterfaceFn fileSystemFactory )
{
	LoadMaterialSystemInterface( fileSystemFactory );
	MaterialSystem_Config_t config;
	g_pMaterialSystem->OverrideConfig( config, false );
}

void ShutdownMaterialSystem( )
{
	if ( g_pMaterialSystem )
	{
		g_pMaterialSystem->Shutdown();
		g_pMaterialSystem = NULL;
	}
}

MaterialSystemMaterial_t FindMaterial( const char *materialName, bool *pFound, bool bComplain )
{
	IMaterial *pMat = g_pMaterialSystem->FindMaterial( materialName, TEXTURE_GROUP_OTHER, bComplain );
	MaterialSystemMaterial_t matHandle = pMat;
	
	if ( pFound )
	{
		*pFound = true;
		if ( IsErrorMaterial( pMat ) )
			*pFound = false;
	}

	return matHandle;
}

void GetMaterialDimensions( MaterialSystemMaterial_t materialHandle, int *width, int *height )
{
	PreviewImageRetVal_t retVal;
	ImageFormat dummyImageFormat;
	IMaterial *material = ( IMaterial * )materialHandle;
	bool translucent;
	retVal = material->GetPreviewImageProperties( width, height, &dummyImageFormat, &translucent );
	if (retVal != MATERIAL_PREVIEW_IMAGE_OK ) 
	{
#if 0
		if (retVal == MATERIAL_PREVIEW_IMAGE_BAD ) 
		{
			Error( "problem getting preview image for %s", 
				g_pMaterialSystem->GetMaterialName( materialInfo[matID].materialHandle ) );
		}
#else
		*width = 128;
		*height = 128;
#endif
	}
}

void GetMaterialReflectivity( MaterialSystemMaterial_t materialHandle, float *reflectivityVect )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	const IMaterialVar *reflectivityVar;

	bool found;
	reflectivityVar = material->FindVar( "$reflectivity", &found, false );
	if( !found )
	{
		Vector tmp;
		material->GetReflectivity( tmp );
		VectorCopy( tmp.Base(), reflectivityVect );
	}
	else
	{
		reflectivityVar->GetVecValue( reflectivityVect, 3 );
	}
}

int GetMaterialShaderPropertyBool( MaterialSystemMaterial_t materialHandle, int propID )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	switch( propID )
	{
	case UTILMATLIB_NEEDS_BUMPED_LIGHTMAPS:
		return material->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS );

	case UTILMATLIB_NEEDS_LIGHTMAP:
		return material->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_LIGHTMAP );

	default:
		Assert( 0 );
		return 0;
	}
}

int GetMaterialShaderPropertyInt( MaterialSystemMaterial_t materialHandle, int propID )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	switch( propID )
	{
	case UTILMATLIB_OPACITY:
		if (material->IsTranslucent())
			return UTILMATLIB_TRANSLUCENT;
		if (material->IsAlphaTested())
			return UTILMATLIB_ALPHATEST;
		return UTILMATLIB_OPAQUE;

	default:
		Assert( 0 );
		return 0;
	}
}

const char *GetMaterialVar( MaterialSystemMaterial_t materialHandle, const char *propertyName )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	IMaterialVar *var;
	bool found;
	var = material->FindVar( propertyName, &found, false );
	if( found )
	{
		return var->GetStringValue();
	}
	else
	{
		return NULL;
	}
}

const char *GetMaterialShaderName( MaterialSystemMaterial_t materialHandle )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	return material->GetShaderName();
}
