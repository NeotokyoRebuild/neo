//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifdef WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

#include "tier0/minidump.h"
#include "tools_minidump.h"

static bool g_bToolsWriteFullMinidumps = false;
static ToolsExceptionHandler g_pCustomExceptionHandler = NULL;


// --------------------------------------------------------------------------------- //
// Internal helpers.
// --------------------------------------------------------------------------------- //

#ifdef WIN32
static LONG __stdcall ToolsExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	// Non VMPI workers write a minidump and show a crash dialog like normal.
	int iType = MiniDumpNormal;
	if ( g_bToolsWriteFullMinidumps )
		iType = MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory;
		
	WriteMiniDumpUsingExceptionInfo( ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo, (MINIDUMP_TYPE)iType );
	return EXCEPTION_CONTINUE_SEARCH;
}


static LONG __stdcall ToolsExceptionFilter_Custom( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	// Run their custom handler.
	g_pCustomExceptionHandler( ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo );
	return EXCEPTION_EXECUTE_HANDLER; // (never gets here anyway)
}
#endif

// --------------------------------------------------------------------------------- //
// Interface functions.
// --------------------------------------------------------------------------------- //

void EnableFullMinidumps( bool bFull )
{
	g_bToolsWriteFullMinidumps = bFull;
}


void SetupDefaultToolsMinidumpHandler()
{
#ifdef WIN32
	SetUnhandledExceptionFilter( ToolsExceptionFilter );
#endif
}


void SetupToolsMinidumpHandler( ToolsExceptionHandler fn )
{
#ifdef WIN32
	g_pCustomExceptionHandler = fn;
	SetUnhandledExceptionFilter( ToolsExceptionFilter_Custom );
#else
	(void)fn;
#endif
}
