#pragma once

void NeoVersionPrint();

#ifdef CLIENT_DLL
void InitializeNeoClRenderer();
#ifdef DEBUG
void InitializeDbgNeoClGitHashEdit();
#endif // DEBUG
#endif // CLIENT_DLL

