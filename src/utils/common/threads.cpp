//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#define	USED

#ifdef WIN32
#include <windows.h>
#endif

#include "cmdlib.h"
#define NO_THREAD_NAMES
#include "threads.h"
#include "pacifier.h"

#include <thread>
#include <mutex>
#include <vector>

#define	MAX_THREADS	16


class CRunThreadsData
{
public:
	int m_iThread;
	void *m_pUserData;
	RunThreadsFn m_Fn;
};

CRunThreadsData g_RunThreadsData[MAX_THREADS];

int dispatch;
int workcount;
qboolean pacifier;

qboolean threaded;
bool g_bLowPriorityThreads = false;

std::vector<std::thread> g_ThreadHandles;

ThreadWorkerFn workfunction;

int numthreads = -1;
std::mutex crit;
static int enter;

int GetThreadWork (void)
{
	int	r;

	ThreadLock ();

	if (dispatch == workcount)
	{
		ThreadUnlock ();
		return -1;
	}

	UpdatePacifier( (float)dispatch / workcount );

	r = dispatch;
	dispatch++;
	ThreadUnlock ();

	return r;
}

void ThreadWorkerFunction( int iThread, void *pUserData )
{
	int work;

	while (1)
	{
		work = GetThreadWork ();
		if (work == -1)
			break;
		 
		workfunction( iThread, work );
	}
}

void RunThreadsOnIndividual (int workcnt, qboolean showpacifier, ThreadWorkerFn func)
{
	if (numthreads == -1)
		ThreadSetDefault ();
	
	workfunction = func;
	RunThreadsOn (workcnt, showpacifier, ThreadWorkerFunction);
}

void SetLowPriority()
{
#ifdef WIN32
	SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );
#else
	// TODO
#endif
}


void ThreadSetDefault (void)
{
	if (numthreads == -1) // not set manually
	{
		numthreads = std::thread::hardware_concurrency();
	}

	Msg ("%i threads\n", numthreads);
}


void ThreadLock (void)
{
	if (!threaded)
		return;
	crit.lock();
	if (enter)
		Error ("Recursive ThreadLock\n");
	enter = 1;
}

void ThreadUnlock (void)
{
	if (!threaded)
		return;
	if (!enter)
		Error ("ThreadUnlock without lock\n");
	enter = 0;
	crit.unlock();
}

// This runs in the thread and dispatches a RunThreadsFn call.
// DWORD WINAPI InternalRunThreadsFn( LPVOID pParameter )
// {
// 	CRunThreadsData *pData = (CRunThreadsData*)pParameter;
// 	pData->m_Fn( pData->m_iThread, pData->m_pUserData );
// 	return 0;
// }

void RunThreads_Start( RunThreadsFn fn, void *pUserData, ERunThreadsPriority ePriority )
{
	Assert( numthreads > 0 );
	threaded = true;

	if ( numthreads > MAX_TOOL_THREADS )
		numthreads = MAX_TOOL_THREADS;

	for ( int i = 0; i < numthreads; i++ )
	{
		// g_RunThreadsData[i].m_iThread = i;
		// g_RunThreadsData[i].m_pUserData = pUserData;
		// g_RunThreadsData[i].m_Fn = fn;

		// DWORD dwDummy;
		// g_ThreadHandles[i] = CreateThread(
		//    NULL,	// LPSECURITY_ATTRIBUTES lpsa,
		//    0,		// DWORD cbStack,
		//    InternalRunThreadsFn,	// LPTHREAD_START_ROUTINE lpStartAddr,
		//    &g_RunThreadsData[i],	// LPVOID lpvThreadParm,
		//    0,			// DWORD fdwCreate,
		//    &dwDummy );

		// if ( ePriority == k_eRunThreadsPriority_UseGlobalState )
		// {
		// 	if( g_bLowPriorityThreads )
		// 		SetThreadPriority( g_ThreadHandles[i], THREAD_PRIORITY_LOWEST );
		// }
		// else if ( ePriority == k_eRunThreadsPriority_Idle )
		// {
		// 	SetThreadPriority( g_ThreadHandles[i], THREAD_PRIORITY_IDLE );
		// }

		g_ThreadHandles.emplace_back(fn, i, pUserData);
	}
}


void RunThreads_End()
{
	//WaitForMultipleObjects( numthreads, g_ThreadHandles, TRUE, INFINITE );
	for (auto& thread : g_ThreadHandles)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	threaded = false;
}
	
void RunThreadsOn( int workcnt, qboolean showpacifier, RunThreadsFn fn, void *pUserData )
{
	int		start, end;

	start = Plat_FloatTime();
	dispatch = 0;
	workcount = workcnt;
	StartPacifier("");
	pacifier = showpacifier;

#ifdef _PROFILE
	threaded = false;
	(*func)( 0 );
	return;
#endif

	RunThreads_Start( fn, pUserData );
	RunThreads_End();

	end = Plat_FloatTime();
	if (pacifier)
	{
		EndPacifier(false);
		printf (" (%i)\n", end-start);
	}
}
