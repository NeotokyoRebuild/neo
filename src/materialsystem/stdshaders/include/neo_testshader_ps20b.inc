#include "shaderlib/cshader.h"
class neo_testshader_ps20b_Static_Index
{
public:
	neo_testshader_ps20b_Static_Index( )
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderStaticTest_neo_testshader_ps20b 0
class neo_testshader_ps20b_Dynamic_Index
{
public:
	neo_testshader_ps20b_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_neo_testshader_ps20b 0
