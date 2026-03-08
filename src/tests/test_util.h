#pragma once

#include "strtools.h"
#include "mathlib/mathlib.h"
#include <cstdio>

const char *g_testFnName;
const char *g_szName;
int g_verbose;

static int g_testFailuresIdv;
static int g_testTotalIdv;
static int g_testFailuresFn;
static int g_testTotalFn;
static int g_retVal;
static bool g_testFnPass;

static bool _TestVerify(const bool bPass, const char *pszCondStr,
		const char *pszFile, const int iLine)
{
	++g_testTotalIdv;
	if (false == bPass)
	{
		g_retVal = 1;
		++g_testFailuresIdv;
		g_testFnPass = false;
		fprintf(stderr, "%s: FAIL! %s (%d): %s\n",
				g_szName, pszFile, iLine, g_testFnName);
		if (pszCondStr)
		{
			fprintf(stderr, "%s: FAIL! \tcond: %s\n", g_szName, pszCondStr);
		}
	}
	return bPass;
}

static void _TestCompareInt(const int lhs, const char *pszLhs,
		const int rhs, const char *pszRhs,
		const char *pszFile, const int iLine)
{
	if (false == _TestVerify(lhs == rhs, nullptr, pszFile, iLine))
	{
		fprintf(stderr, "%s: FAIL! \tint: %s [%d] != %s [%d]\n",
				g_szName, pszLhs, lhs, pszRhs, rhs);
	}
}

static void _TestCompareFlt(const float lhs, const char *pszLhs,
		const float rhs, const char *pszRhs,
		const float epsilon,
		const char *pszFile, const int iLine)
{
	if (false == _TestVerify(lhs == rhs, nullptr, pszFile, iLine))
	{
		fprintf(stderr, "%s: FAIL! \tfloat: %s [%.3f] != %s [%.3f] | Epsilon %f\n",
				g_szName, pszLhs, lhs, pszRhs, rhs, epsilon);
	}
}

static void _TestCompareStr(const char *lhs, const char *pszLhs,
		const char *rhs, const char *pszRhs,
		const char *pszFile, const int iLine)
{
	if (false == _TestVerify(0 == V_strcmp(lhs, rhs), nullptr, pszFile, iLine))
	{
		fprintf(stderr, "%s: FAIL! \tstring: %s \"%s\" (len: %d) != %s \"%s\" (len: %d)\n",
				g_szName, pszLhs, lhs, V_strlen(lhs), pszRhs, rhs, V_strlen(rhs));
	}
}
	
#define TEST_VERIFY(cond) _TestVerify(cond, #cond, __FILE__, __LINE__);
#define TEST_COMPARE_INT(lhs, rhs) _TestCompareInt((lhs), #lhs, (rhs), #rhs, __FILE__, __LINE__);
#define TEST_COMPARE_FLT(lhs, rhs, epsilon) _TestCompareFlt((lhs), #lhs, (rhs), #rhs, (epsilon), __FILE__, __LINE__);
#define TEST_COMPARE_STR(lhs, rhs) _TestCompareStr((lhs), #lhs, (rhs), #rhs, __FILE__, __LINE__);

#define TEST_RUN(test_name) \
	if (argc < 2 || V_strstr(#test_name, argv[1])) \
	{ \
		++g_testTotalFn; \
		g_testFnPass = true; \
		g_testFnName = #test_name; \
		test_name(); \
		if (g_testFnPass) \
		{ \
			printf("%s: %s PASSED\n", g_szName, #test_name); \
		} \
		else \
		{ \
			printf("%s: %s FAILED\n", g_szName, #test_name); \
			++g_testFailuresFn; \
		} \
	}

#ifdef WIN32

#define TEST_INIT() \
	int main(int argc, char **argv) \
	{ \
		const char *szBasename1 = V_strrchr(argv[0], '\\'); \
		g_szName = (szBasename1) ? szBasename1 + 1 : nullptr; \
		if (!g_szName) \
		{ \
			const char *szBasename2 = V_strrchr(argv[0], '/'); \
			g_szName = (szBasename2) ? szBasename2 + 1 : argv[0]; \
		}

#else

#define TEST_INIT() \
	int main(int argc, char **argv) \
	{ \
		const char *szBasename = V_strrchr(argv[0], '/'); \
		g_szName = (szBasename) ? szBasename + 1 : argv[0];

#endif

#define TEST_END() \
		printf(	"%s: Final results:\n" \
				"%s: \tPer-run: Total: %d | Failures: %d\n" \
				"%s: \tPer-verify/compare: Total: %d | Failures: %d\n" \
				, g_szName \
				, g_szName, g_testTotalFn, g_testFailuresFn \
				, g_szName, g_testTotalIdv, g_testFailuresIdv); \
		return g_retVal; \
	}

