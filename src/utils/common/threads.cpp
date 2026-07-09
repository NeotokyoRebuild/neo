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

#ifndef WIN32
#include <pthread.h>
#include <sched.h>
#endif

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
		if (numthreads < 1)
			numthreads = 1;
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

static void SetThreadPriorityLow( std::thread &thread, bool idle )
{
#ifdef WIN32
	SetThreadPriority( thread.native_handle(), idle ? THREAD_PRIORITY_IDLE : THREAD_PRIORITY_LOWEST );
#else
	// On Linux "lowest" and "idle" both map onto the SCHED_IDLE policy, which is
	// the closest portable equivalent for background worker threads.
	sched_param param = {};
	pthread_setschedparam( thread.native_handle(), SCHED_IDLE, &param );
	(void)idle;
#endif
}

void RunThreads_Start( RunThreadsFn fn, void *pUserData, ERunThreadsPriority ePriority )
{
	Assert( numthreads > 0 );
	threaded = true;

	if ( numthreads > MAX_TOOL_THREADS )
		numthreads = MAX_TOOL_THREADS;

	for ( int i = 0; i < numthreads; i++ )
	{
		std::thread &thread = g_ThreadHandles.emplace_back( fn, i, pUserData );

		if ( ePriority == k_eRunThreadsPriority_UseGlobalState && g_bLowPriorityThreads )
		{
			SetThreadPriorityLow( thread, false );
		}
		else if ( ePriority == k_eRunThreadsPriority_Idle )
		{
			SetThreadPriorityLow( thread, true );
		}
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
	g_ThreadHandles.clear();

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
