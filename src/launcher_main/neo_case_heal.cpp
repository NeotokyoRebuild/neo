#include "neo_case_heal.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Heal hack :: fix poisoned installations

// Binaries the engine loads by exact mixed-case name. 
// Keep this list in sync with src/devtools/check_depot_casing.sh. 
// It's hacky but it works, and should be harmless to leave in 
// clearly marked, use `git blame` to find the commit to revert. 
static const char *k_pszCanonicalBinaryPaths[] =
{
	"bin/GameUI.so",
	"bin/ServerBrowser.so",
	"bin/libMiles.so",
	"bin/libTelemetryX64.so",
	"bin/libTelemetryX86.so",
	"bin/linux64/GameUI.so",
	"bin/linux64/ServerBrowser.so",
};

int NEO_HealBinaryCasing( const char *pszRootDir )
{
	int nRepairs = 0;
	const size_t nRootLen = strlen( pszRootDir );

	for ( size_t i = 0; i < sizeof( k_pszCanonicalBinaryPaths ) / sizeof( k_pszCanonicalBinaryPaths[0] ); ++i )
	{
		const char *pszCanonical = k_pszCanonicalBinaryPaths[i];

		char szCanonicalPath[PATH_MAX];
		char szLowerPath[PATH_MAX];
		int nLen = snprintf( szCanonicalPath, sizeof( szCanonicalPath ), "%s/%s", pszRootDir, pszCanonical );
		if ( nLen <= 0 || nLen >= (int)sizeof( szCanonicalPath ) )
		{
			continue;
		}

		// The miscased variant is always the fully lower cased relative
		// path. The directory parts are already lowercase, so
		// lower casing the whole relative path is A-OK.

		memcpy( szLowerPath, szCanonicalPath, nLen + 1 );
		for ( char *pch = szLowerPath + nRootLen + 1; *pch; ++pch )
		{
			*pch = (char)tolower( (unsigned char)*pch );
		}

		if ( strcmp( szLowerPath, szCanonicalPath ) == 0 )
		{
			continue;
		}

		// `lstat` instead of `stat`: a twin left as a symlink should be
		// handled as the link itself, not its target.
		// see: `man lstat`
		struct stat lowerStat;
		if ( lstat( szLowerPath, &lowerStat ) != 0 )
		{
			continue;
		}

		struct stat canonStat;
		const bool bHaveCanonical = ( lstat( szCanonicalPath, &canonStat ) == 0 );

		bool bRepaired;
		if ( !bHaveCanonical || lowerStat.st_mtime > canonStat.st_mtime )
		{
			// Either its just the lowercased file exists, or both exist and
			// the lowercased one is newer. Newer means Steam wrote it
			// last, so it carries the current depot content. rename()
			// replaces any stale canonical file atomically.

			bRepaired = ( rename( szLowerPath, szCanonicalPath ) == 0 );
		}
		else
		{
			// Both exist and the canonical file is current. Drop the twin.
			bRepaired = ( unlink( szLowerPath ) == 0 );
		}

		if ( bRepaired )
		{
			fprintf( stderr, "[NT;RE launcher] Repaired file name casing: %s\n", pszCanonical );
			++nRepairs;
		}
		else
		{
			fprintf( stderr, "[NT;RE launcher] Warning: could not repair file name casing of %s. The engine may fail to load it.\n", pszCanonical );
		}
	}

	return nRepairs;
}
