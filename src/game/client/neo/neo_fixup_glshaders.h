#ifndef NEO_FIXUP_GLSHADERS_H
#define NEO_FIXUP_GLSHADERS_H
#ifdef _WIN32
#pragma once
#endif
#ifdef LINUX

class ICvar;
class IFileSystem;

void FixupGlShaders(IFileSystem* filesystem, ICvar* cvar);

#endif // LINUX
#endif // NEO_FIXUP_GLSHADERS_H
