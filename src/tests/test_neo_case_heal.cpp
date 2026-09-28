#include "test_util.h"
#include "../launcher_main/neo_case_heal.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

// Each test builds a throwaway install root under /tmp with mkdtemp and
// runs the heal against it. Helpers below keep the fixtures short.

static char g_szRoot[256];

static void MakeRoot()
{
	V_strcpy_safe( g_szRoot, "/tmp/neo_case_heal_XXXXXX" );
	TEST_VERIFY( mkdtemp( g_szRoot ) != nullptr );
	char szDir[512];
	V_sprintf_safe( szDir, "%s/bin", g_szRoot );
	TEST_VERIFY( mkdir( szDir, 0755 ) == 0 );
	V_sprintf_safe( szDir, "%s/bin/linux64", g_szRoot );
	TEST_VERIFY( mkdir( szDir, 0755 ) == 0 );
}

static void WriteFileContent( const char *pszRelPath, const char *pszContent )
{
	char szPath[512];
	V_sprintf_safe( szPath, "%s/%s", g_szRoot, pszRelPath );
	FILE *pFile = fopen( szPath, "w" );
	TEST_VERIFY( pFile != nullptr );
	if ( pFile )
	{
		fputs( pszContent, pFile );
		fclose( pFile );
	}
}

static void SetMTime( const char *pszRelPath, time_t iSeconds )
{
	char szPath[512];
	V_sprintf_safe( szPath, "%s/%s", g_szRoot, pszRelPath );
	struct timeval times[2] = { { iSeconds, 0 }, { iSeconds, 0 } };
	TEST_VERIFY( utimes( szPath, times ) == 0 );
}

static bool RelExists( const char *pszRelPath )
{
	char szPath[512];
	V_sprintf_safe( szPath, "%s/%s", g_szRoot, pszRelPath );
	struct stat st;
	return lstat( szPath, &st ) == 0;
}

static void ReadFileContent( const char *pszRelPath, char *pszOut, int nOutSize )
{
	pszOut[0] = '\0';
	char szPath[512];
	V_sprintf_safe( szPath, "%s/%s", g_szRoot, pszRelPath );
	FILE *pFile = fopen( szPath, "r" );
	TEST_VERIFY( pFile != nullptr );
	if ( pFile )
	{
		size_t nRead = fread( pszOut, 1, nOutSize - 1, pFile );
		pszOut[nRead] = '\0';
		fclose( pFile );
	}
}

void TestLowercaseOnlyGetsRenamed()
{
	MakeRoot();
	WriteFileContent( "bin/gameui.so", "engine bits" );
	WriteFileContent( "bin/linux64/gameui.so", "engine bits 64" );
	TEST_COMPARE_INT( 2, NEO_HealBinaryCasing( g_szRoot ) );
	TEST_VERIFY( RelExists( "bin/GameUI.so" ) );
	TEST_VERIFY( RelExists( "bin/linux64/GameUI.so" ) );
	TEST_VERIFY( !RelExists( "bin/gameui.so" ) );
	TEST_VERIFY( !RelExists( "bin/linux64/gameui.so" ) );
}

void TestBothExistLowercaseNewerWins()
{
	MakeRoot();
	WriteFileContent( "bin/GameUI.so", "stale" );
	WriteFileContent( "bin/gameui.so", "current" );
	SetMTime( "bin/GameUI.so", 1000000 );
	SetMTime( "bin/gameui.so", 2000000 );
	TEST_COMPARE_INT( 1, NEO_HealBinaryCasing( g_szRoot ) );
	TEST_VERIFY( RelExists( "bin/GameUI.so" ) );
	TEST_VERIFY( !RelExists( "bin/gameui.so" ) );
	char szContent[64];
	ReadFileContent( "bin/GameUI.so", szContent, sizeof( szContent ) );
	TEST_COMPARE_STR( szContent, "current" );
}

void TestBothExistCanonicalNewerKept()
{
	MakeRoot();
	WriteFileContent( "bin/GameUI.so", "current" );
	WriteFileContent( "bin/gameui.so", "stale" );
	SetMTime( "bin/GameUI.so", 2000000 );
	SetMTime( "bin/gameui.so", 1000000 );
	TEST_COMPARE_INT( 1, NEO_HealBinaryCasing( g_szRoot ) );
	TEST_VERIFY( RelExists( "bin/GameUI.so" ) );
	TEST_VERIFY( !RelExists( "bin/gameui.so" ) );
	char szContent[64];
	ReadFileContent( "bin/GameUI.so", szContent, sizeof( szContent ) );
	TEST_COMPARE_STR( szContent, "current" );
}

void TestCanonicalOnlyIsNoop()
{
	MakeRoot();
	WriteFileContent( "bin/GameUI.so", "engine bits" );
	TEST_COMPARE_INT( 0, NEO_HealBinaryCasing( g_szRoot ) );
	TEST_VERIFY( RelExists( "bin/GameUI.so" ) );
}

void TestEmptyRootIsNoop()
{
	MakeRoot();
	TEST_COMPARE_INT( 0, NEO_HealBinaryCasing( g_szRoot ) );
}

void TestAllSevenEntriesCovered()
{
	MakeRoot();
	WriteFileContent( "bin/gameui.so", "a" );
	WriteFileContent( "bin/serverbrowser.so", "b" );
	WriteFileContent( "bin/libmiles.so", "c" );
	WriteFileContent( "bin/libtelemetryx64.so", "d" );
	WriteFileContent( "bin/libtelemetryx86.so", "e" );
	WriteFileContent( "bin/linux64/gameui.so", "f" );
	WriteFileContent( "bin/linux64/serverbrowser.so", "g" );
	TEST_COMPARE_INT( 7, NEO_HealBinaryCasing( g_szRoot ) );
	TEST_VERIFY( RelExists( "bin/GameUI.so" ) );
	TEST_VERIFY( RelExists( "bin/ServerBrowser.so" ) );
	TEST_VERIFY( RelExists( "bin/libMiles.so" ) );
	TEST_VERIFY( RelExists( "bin/libTelemetryX64.so" ) );
	TEST_VERIFY( RelExists( "bin/libTelemetryX86.so" ) );
	TEST_VERIFY( RelExists( "bin/linux64/GameUI.so" ) );
	TEST_VERIFY( RelExists( "bin/linux64/ServerBrowser.so" ) );
}

TEST_INIT()
TEST_RUN( TestLowercaseOnlyGetsRenamed )
TEST_RUN( TestBothExistLowercaseNewerWins )
TEST_RUN( TestBothExistCanonicalNewerKept )
TEST_RUN( TestCanonicalOnlyIsNoop )
TEST_RUN( TestEmptyRootIsNoop )
TEST_RUN( TestAllSevenEntriesCovered )
TEST_END()
