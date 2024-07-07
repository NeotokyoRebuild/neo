#pragma once

void NeoVersionPrint();

#if defined(CLIENT_DLL) && defined(DEBUG)
void InitializeDbgNeoClGitHashEdit();
#endif
