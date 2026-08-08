#ifndef NEO_CASE_HEAL_H
#define NEO_CASE_HEAL_H
#ifdef _WIN32
#pragma once
#endif

// Heal hack :: fix poisoned installations

// Repairs engine binaries whose ondisk file names are fully lowercased.
// The engine loads these binaries by exact mixed case name, and Steam
// matches depot file names case-insensitively. Steam never renames files on
// disk. 
// 
// A lowercased copy is unreachable on a case sensitive filesystem. 
// Renames each affected file under `pszRootDir` back to its conventional casing. 
// When both casings exist, the newest file wins and ends up under the 
// canonical name. Returns the number of repairs made.
int NEO_HealBinaryCasing( const char *pszRootDir );

#endif // NEO_CASE_HEAL_H
