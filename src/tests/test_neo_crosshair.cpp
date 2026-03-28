#include "neo_crosshair.h"
#include "test_util.h"

#define CURRENT_VER "6"

void TestDeserial_V1_PREALPHA_V8_2()
{
	// v1 - is actually non-existant, just a placeholder version
	// for back when the crosshair wasn't string serialized
	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, "1;");
	TEST_COMPARE_INT(bValid, false);
	TEST_COMPARE_INT(false, ValidateCrosshairSerial("1;"));
}

void TestDeserial_V2_ALPHA_V17()
{
	// v2 - First string serialized version and networked
	static const char SERIAL_TEST_STR[] = "2;2;-1;0;3;0.000;2;4;1;6;1;5;7;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;3;2;4;1;6;5;7;0;-16777216;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);
	TEST_COMPARE_INT(chr->iSize, 3);
	TEST_COMPARE_FLT(chr->flScrSize, 0.0f, 0.0001f);
	TEST_COMPARE_INT(chr->iThick, 2);
	TEST_COMPARE_INT(chr->iGap, 4);
	TEST_COMPARE_INT(chr->iOutline, 1);
	TEST_COMPARE_INT(chr->iCenterDot, 6);
	TEST_COMPARE_INT(chr->iCircleRad, 5);
	TEST_COMPARE_INT(chr->iCircleSegments, 7);

	// Test default values
	TEST_COMPARE_INT(chr->eDynamicType, defChr->eDynamicType);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), defChr->colorDot.GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), defChr->colorDotOutline.GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), defChr->colorDotOutline.GetRawColor());

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V17);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestDeserial_V3_ALPHA_V19()
{
	// v3 - Added dynamic crosshair
	static const char SERIAL_TEST_STR[] = "3;2;-1;0;3;0.000;2;4;1;6;1;5;7;1;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;3;2;4;1;6;5;7;1;-16777216;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);
	TEST_COMPARE_INT(chr->iSize, 3);
	TEST_COMPARE_FLT(chr->flScrSize, 0.0f, 0.0001f);
	TEST_COMPARE_INT(chr->iThick, 2);
	TEST_COMPARE_INT(chr->iGap, 4);
	TEST_COMPARE_INT(chr->iOutline, 1);
	TEST_COMPARE_INT(chr->iCenterDot, 6);
	TEST_COMPARE_INT(chr->iCircleRad, 5);
	TEST_COMPARE_INT(chr->iCircleSegments, 7);
	TEST_COMPARE_INT(chr->eDynamicType, CROSSHAIR_DYNAMICTYPE_GAP);

	// Test default values
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), defChr->colorDot.GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), defChr->colorDotOutline.GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), defChr->colorDotOutline.GetRawColor());

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V19);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestDeserial_V4_ALPHA_V22()
{
	// v4 - Separate colors for dots and outlines
	static const char SERIAL_TEST_STR[] = "4;2;-1;0;3;0.000;2;4;1;6;1;5;7;1;1;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;2;2;-1;0;3;2;4;1;6;5;7;1;-16776961;-16711936;-65536;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_SEPERATEDOTCOLOR);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);
	TEST_COMPARE_INT(chr->iSize, 3);
	TEST_COMPARE_FLT(chr->flScrSize, 0.0f, 0.0001f);
	TEST_COMPARE_INT(chr->iThick, 2);
	TEST_COMPARE_INT(chr->iGap, 4);
	TEST_COMPARE_INT(chr->iOutline, 1);
	TEST_COMPARE_INT(chr->iCenterDot, 6);
	TEST_COMPARE_INT(chr->iCircleRad, 5);
	TEST_COMPARE_INT(chr->iCircleSegments, 7);
	TEST_COMPARE_INT(chr->eDynamicType, CROSSHAIR_DYNAMICTYPE_GAP);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), Color(255, 0, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), Color(0, 255, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), Color(0, 0, 255, 255).GetRawColor());

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V22);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestDeserial_V5_ALPHA_V28()
{
	// v5 - Omits unused segments
	static const char SERIAL_TEST_STR[] = "5;2;-1;0;3;2;4;1;6;1;5;7;1;1;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;2;2;-1;0;3;2;4;1;6;5;7;1;-16776961;-16711936;-65536;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_SEPERATEDOTCOLOR);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);
	TEST_COMPARE_INT(chr->iSize, 3);
	TEST_COMPARE_INT(chr->iThick, 2);
	TEST_COMPARE_INT(chr->iGap, 4);
	TEST_COMPARE_INT(chr->iOutline, 1);
	TEST_COMPARE_INT(chr->iCenterDot, 6);
	TEST_COMPARE_INT(chr->iCircleRad, 5);
	TEST_COMPARE_INT(chr->iCircleSegments, 7);
	TEST_COMPARE_INT(chr->eDynamicType, CROSSHAIR_DYNAMICTYPE_GAP);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), Color(255, 0, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), Color(0, 255, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), Color(0, 0, 255, 255).GetRawColor());

	// Test default values
	TEST_COMPARE_FLT(chr->flScrSize, defChr->flScrSize, 0.0001f);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V28);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestDeserial_V6_ALPHA_V29()
{
	// v6 - Secondary + shotgun crosshairs, and hipfire variant
	// Further compact cross-segments by default's values
	// Run-length encoding/decoding on empty segments
	static const char SERIAL_TEST_STR[] = "6;0;0;2;2;-1;0;3;2;4;1;6;5;7;1;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;2;2;-1;0;3;2;4;1;6;5;7;1;-16776961;-16711936;-65536;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_SEPERATEDOTCOLOR);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);
	TEST_COMPARE_INT(chr->iSize, 3);
	TEST_COMPARE_INT(chr->iThick, 2);
	TEST_COMPARE_INT(chr->iGap, 4);
	TEST_COMPARE_INT(chr->iOutline, 1);
	TEST_COMPARE_INT(chr->iCenterDot, 6);
	TEST_COMPARE_INT(chr->iCircleRad, 5);
	TEST_COMPARE_INT(chr->iCircleSegments, 7);
	TEST_COMPARE_INT(chr->eDynamicType, CROSSHAIR_DYNAMICTYPE_GAP);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), Color(255, 0, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), Color(0, 255, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), Color(0, 0, 255, 255).GetRawColor());

	// Test default values
	TEST_COMPARE_FLT(chr->flScrSize, defChr->flScrSize, 0.0001f);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V29);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestSerial_LongestLength()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";63;15;3;2;-1;1;0.999;10;10;5;10;10;10;1;-1;-1;-1;2;;-16777217;0;11;11;11;4;11;11;11;2;-16777217;-16777217;-16777217;2;;-16777217;0;11;11;11;4;11;11;11;2;-16777217;-16777217;-16777217;2;;-16777217;0;11;11;11;4;11;11;11;2;-16777217;-16777217;-16777217;3;;-33554433;1;0.997;12;12;3;12;12;12;3;-33554433;-33554433;-33554433;3;;-33554433;1;0.997;12;12;3;12;12;12;3;-33554433;-33554433;-33554433;3;;-33554433;1;0.997;12;12;3;12;12;12;3;-33554433;-33554433;-33554433;";

	CrosshairInfo xhairInfo = {};
	xhairInfo.wepFlags = CROSSHAIR_WEP_FLAG_SECONDARY | CROSSHAIR_WEP_FLAG_SHOTGUN | CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE | CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE | CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE | CROSSHAIR_WEP_FLAG_THROWABLE_HIPFIRE;
	xhairInfo.hipfireFlags = CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT | CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY | CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN | CROSSHAIR_HIPFIRECUSTOM_FLAG_THROWABLE;

	{
		CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];
		chr->flags = CROSSHAIR_FLAG_NOTOPLINE | CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
		chr->iStyle = CROSSHAIR_STYLE_CUSTOM;
		chr->color = Color(255, 255, 255, 255);
		chr->eSizeType = CROSSHAIR_SIZETYPE_SCREEN;
		chr->iSize = 10;
		chr->flScrSize = 0.999;
		chr->iThick = 10;
		chr->iGap = 10;
		chr->iOutline = 5;
		chr->iCenterDot = 10;
		chr->iCircleRad = 10;
		chr->iCircleSegments = 10;
		chr->eDynamicType = CROSSHAIR_DYNAMICTYPE_GAP;
		chr->colorDot = Color(255, 255, 255, 255);
		chr->colorDotOutline = Color(255, 255, 255, 255);
		chr->colorOutline = Color(255, 255, 255, 255);
	}
	for (int i = (CROSSHAIR_WEP_DEFAULT + 1); i <= CROSSHAIR_WEP_DEFAULT_HIPFIRE; ++i)
	{
		CrosshairWepInfo *chr = &xhairInfo.wep[i];
		chr->flags = CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
		chr->iStyle = CROSSHAIR_STYLE_CUSTOM;
		chr->color = Color(255, 255, 255, 254);
		chr->eSizeType = CROSSHAIR_SIZETYPE_ABSOLUTE;
		chr->iSize = 11;
		chr->flScrSize = 0.998;
		chr->iThick = 11;
		chr->iGap = 11;
		chr->iOutline = 4;
		chr->iCenterDot = 11;
		chr->iCircleRad = 11;
		chr->iCircleSegments = 11;
		chr->eDynamicType = CROSSHAIR_DYNAMICTYPE_CIRCLE;
		chr->colorDot = Color(255, 255, 255, 254);
		chr->colorDotOutline = Color(255, 255, 255, 254);
		chr->colorOutline = Color(255, 255, 255, 254);
	}
	for (int i = (CROSSHAIR_WEP_DEFAULT_HIPFIRE + 1); i < CROSSHAIR_WEP__TOTAL; ++i)
	{
		CrosshairWepInfo *chr = &xhairInfo.wep[i];
		chr->flags = CROSSHAIR_FLAG_NOTOPLINE | CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
		chr->iStyle = CROSSHAIR_STYLE_CUSTOM;
		chr->color = Color(255, 255, 255, 253);
		chr->eSizeType = CROSSHAIR_SIZETYPE_SCREEN;
		chr->iSize = 12;
		chr->flScrSize = 0.997;
		chr->iThick = 12;
		chr->iGap = 12;
		chr->iOutline = 3;
		chr->iCenterDot = 12;
		chr->iCircleRad = 12;
		chr->iCircleSegments = 12;
		chr->eDynamicType = CROSSHAIR_DYNAMICTYPE_SIZE;
		chr->colorDot = Color(255, 255, 255, 253);
		chr->colorDotOutline = Color(255, 255, 255, 253);
		chr->colorOutline = Color(255, 255, 255, 253);
	}

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	// Make sure it's not maxing out the string
	TEST_VERIFY(V_strlen(szExportSeq) < (NEO_XHAIR_SEQMAX - 1 - 1));
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFailure_Invalid_SerialValues()
{
	CrosshairInfo xhairInfo = {};
	TEST_COMPARE_INT(ImportCrosshair(&xhairInfo, "0;"), false);
	TEST_COMPARE_INT(ImportCrosshair(&xhairInfo, "-1;"), false);
	TEST_COMPARE_INT(ImportCrosshair(&xhairInfo, "999999;"), false);
	TEST_COMPARE_INT(ImportCrosshair(&xhairInfo, "-999999;"), false);
}

void TestFailure_OutOfBoundStr_Under()
{
	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	TEST_COMPARE_INT(ImportCrosshair(&xhairInfo, "6;0;0;2;2;-1;0;3;2;4;1;6;5;"), true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];
	TEST_COMPARE_INT(chr->iCenterDot, 6);
	TEST_COMPARE_INT(chr->iCircleRad, 5);
	TEST_COMPARE_INT(chr->iCircleSegments, defChr->iCircleSegments);
	TEST_COMPARE_INT(chr->eDynamicType, defChr->eDynamicType);
}

void TestFailure_OutOfBoundStr_Over()
{
	CrosshairInfo xhairInfo = {};
	TEST_COMPARE_INT(ImportCrosshair(&xhairInfo, "6;0;0;2;2;-1;0;3;2;4;1;6;5;7;1;-16776961;-16711936;-65536;12;24;48;"), true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];
	TEST_COMPARE_INT(chr->iCenterDot, 6);
	TEST_COMPARE_INT(chr->iCircleRad, 5);
	TEST_COMPARE_INT(chr->iCircleSegments, 7);
	TEST_COMPARE_INT(chr->eDynamicType, CROSSHAIR_DYNAMICTYPE_GAP);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), Color(255, 0, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), Color(0, 255, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), Color(0, 0, 255, 255).GetRawColor());

	// Because both wepFlags and hipfireFlags are 0, the rest should really just be
	// copies of the default segments, nothing will pick up the "12;24;48;" at the end
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));
}

void TestFailure_OutOfBoundValues()
{
	static const char SERIAL_TEST_STR_OUTBOUND[] = CURRENT_VER ";0;0;2;99;-1;-2;101;123;-111;1000;999;121212;123;80;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_STR_FIXED[] = CURRENT_VER ";0;0;2;2;-1;0;100;25;0;5;25;50;50;3;-16776961;-16711936;-65536;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR_OUTBOUND);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM); // 99 -> 2
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1); 
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_SEPERATEDOTCOLOR);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE); // -2 -> 0
	TEST_COMPARE_INT(chr->iSize, CROSSHAIR_MAX_SIZE); // 101 -> 100
	TEST_COMPARE_INT(chr->iThick, CROSSHAIR_MAX_THICKNESS); // 123 -> 25
	TEST_COMPARE_INT(chr->iGap, 0); // -111 -> 0
	TEST_COMPARE_INT(chr->iOutline, CROSSHAIR_MAX_OUTLINE); // 1000 -> 5
	TEST_COMPARE_INT(chr->iCenterDot, CROSSHAIR_MAX_CENTER_DOT); // 999 -> 25
	TEST_COMPARE_INT(chr->iCircleRad, CROSSHAIR_MAX_CIRCLE_RAD); // 121212 -> 50
	TEST_COMPARE_INT(chr->iCircleSegments, CROSSHAIR_MAX_CIRCLE_SEGMENTS); // 123 -> 50
	TEST_COMPARE_INT(chr->eDynamicType, CROSSHAIR_DYNAMICTYPE_SIZE); // 80 -> 3
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), Color(255, 0, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), Color(0, 255, 0, 255).GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), Color(0, 0, 255, 255).GetRawColor());

	// Test default values
	TEST_COMPARE_FLT(chr->flScrSize, defChr->flScrSize, 0.0001f);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR_FIXED);

	TEST_COMPARE_INT(true, ValidateCrosshairSerial(SERIAL_TEST_STR_FIXED));
	TEST_COMPARE_INT(false, ValidateCrosshairSerial(SERIAL_TEST_STR_OUTBOUND));
}

void TestFeature_Omit_Style_Default()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;0;-1;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_DEFAULT);
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_Style_AltB()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;1;-1;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_ALT_B);
	TEST_COMPARE_INT(chr->color.GetRawColor(), -1);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_SizeType_Absolute()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;5;0;0;0;0;0;0;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_ABSOLUTE);
	TEST_COMPARE_INT(chr->iSize, 5);
	TEST_COMPARE_FLT(chr->flScrSize, defChr->flScrSize, 0.0001f);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_SizeType_Screen()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;1;0.333;0;0;0;0;0;0;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->eSizeType, CROSSHAIR_SIZETYPE_SCREEN);
	TEST_COMPARE_INT(chr->iSize, defChr->iSize);
	TEST_COMPARE_FLT(chr->flScrSize, 0.333f, 0.001f);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_CircleRad_Empty()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;6;0;0;0;1;0;1;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->iCircleRad, 0);
	TEST_COMPARE_INT(chr->iCircleSegments, defChr->iCircleSegments);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_CircleRad_Used()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;6;0;0;0;1;6;8;1;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->iCircleRad, 6);
	TEST_COMPARE_INT(chr->iCircleSegments, 8);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_Outline_Empty()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;6;0;0;0;1;0;1;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->iOutline, 0);
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), defChr->colorOutline.GetRawColor());

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_Outline_Used()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;6;0;0;1;1;0;1;-2;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->iOutline, 1);
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), -2);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_CenterDot_Empty()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;6;0;0;0;0;0;1;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->iOutline, 0);
	TEST_COMPARE_INT(chr->iCenterDot, 0);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), defChr->colorDot.GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), defChr->colorDotOutline.GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), defChr->colorOutline.GetRawColor());

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_CenterDot_SepDotColor_Not()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;6;0;0;0;1;0;1;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);
	TEST_COMPARE_INT(chr->iOutline, 0);
	TEST_COMPARE_INT(chr->iCenterDot, 1);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), defChr->colorDot.GetRawColor());
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), defChr->colorDotOutline.GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), defChr->colorOutline.GetRawColor());


	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_CenterDot_SepDotColor_NoOutline()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;2;2;-1;0;6;0;0;0;1;0;1;-2;";

	CrosshairInfo xhairDef = {};
	ResetCrosshairToDefault(&xhairDef);
	const CrosshairWepInfo *defChr = &xhairDef.wep[CROSSHAIR_WEP_DEFAULT];

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_SEPERATEDOTCOLOR);
	TEST_COMPARE_INT(chr->iOutline, 0);
	TEST_COMPARE_INT(chr->iCenterDot, 1);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), -2);
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), defChr->colorDotOutline.GetRawColor());
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), defChr->colorOutline.GetRawColor());

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Omit_CenterDot_SepDotColor_WithOutline()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;2;2;-1;0;6;0;0;2;1;0;1;-2;-3;-4;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_SEPERATEDOTCOLOR);
	TEST_COMPARE_INT(chr->iOutline, 2);
	TEST_COMPARE_INT(chr->iCenterDot, 1);
	TEST_COMPARE_INT(chr->colorDot.GetRawColor(), -2);
	TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), -3);
	TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), -4);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Flags_OldBool_None()
{
	// Convert v4 (non-compressed, 2 bools) to v6+ (flagged)
	static const char SERIAL_TEST_STR[] = "4;2;-1;0;3;0.000;2;4;1;6;1;5;7;1;0;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;0;2;-1;0;3;2;4;1;6;5;7;1;-65536;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_DEFAULT);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V22);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Flags_OldBool_ToplineOff()
{
	// Convert v4 (non-compressed, 2 bools) to v6+ (flagged)
	static const char SERIAL_TEST_STR[] = "4;2;-1;0;3;0.000;2;4;1;6;0;5;7;1;0;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;1;2;-1;0;3;2;4;1;6;5;7;1;-65536;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_NOTOPLINE);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V22);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Flags_OldBool_SepDotColor()
{
	// Convert v4 (non-compressed, 2 bools) to v6+ (flagged)
	static const char SERIAL_TEST_STR[] = "4;2;-1;0;3;0.000;2;4;1;6;1;5;7;1;1;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;2;2;-1;0;3;2;4;1;6;5;7;1;-16776961;-16711936;-65536;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_SEPERATEDOTCOLOR);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V22);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Flags_OldBool_ToplineOff_SepDotColor()
{
	// Convert v4 (non-compressed, 2 bools) to v6+ (flagged)
	static const char SERIAL_TEST_STR[] = "4;2;-1;0;3;0.000;2;4;1;6;0;5;7;1;1;-16776961;-16711936;-65536;";
	static const char SERIAL_TEST_LATEST_STR[] = CURRENT_VER ";0;0;3;2;-1;0;3;2;4;1;6;5;7;1;-16776961;-16711936;-65536;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_NOTOPLINE | CROSSHAIR_FLAG_SEPERATEDOTCOLOR);

	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_COMPARE_INT(0, V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_THROWABLE_HIPFIRE], sizeof(CrosshairWepInfo)));

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_LATEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	ExportCrosshair(&xhairInfo, szExportSeq, NEOXHAIR_SERIAL_ALPHA_V22);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Flags_Topline_Off()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;1;2;-1;0;6;0;0;0;0;0;1;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_NOTOPLINE);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_Flags_Topline_Off_SepDotColor()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";0;0;3;2;-1;0;6;0;0;0;0;0;1;";

	CrosshairInfo xhairInfo = {};
	const bool bValid = ImportCrosshair(&xhairInfo, SERIAL_TEST_STR);
	TEST_COMPARE_INT(bValid, true);
	TEST_COMPARE_INT(xhairInfo.wepFlags, CROSSHAIR_WEP_FLAG_DEFAULT);
	TEST_COMPARE_INT(xhairInfo.hipfireFlags, CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL);

	const CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];

	// Test values that's from the string itself
	TEST_COMPARE_INT(chr->iStyle, CROSSHAIR_STYLE_CUSTOM);
	TEST_COMPARE_INT(chr->flags, CROSSHAIR_FLAG_NOTOPLINE | CROSSHAIR_FLAG_SEPERATEDOTCOLOR);

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));
}

void TestFeature_RunLengthEncode_Full()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";63;15;3;2;-1;0;10;10;10;5;10;10;10;1;-1;-1;-1;90^";

	CrosshairInfo xhairInfo = {};
	ResetCrosshairToDefault(&xhairInfo);
	xhairInfo.wepFlags = CROSSHAIR_WEP_FLAG_SECONDARY | CROSSHAIR_WEP_FLAG_SHOTGUN | CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE | CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE | CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE | CROSSHAIR_WEP_FLAG_THROWABLE_HIPFIRE;
	xhairInfo.hipfireFlags = CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT | CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY | CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN | CROSSHAIR_HIPFIRECUSTOM_FLAG_THROWABLE;

	for (int i = 0; i < CROSSHAIR_WEP__TOTAL; ++i)
	{
		CrosshairWepInfo *chr = &xhairInfo.wep[i];
		chr->flags = CROSSHAIR_FLAG_NOTOPLINE | CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
		chr->iStyle = CROSSHAIR_STYLE_CUSTOM;
		chr->color = Color(255, 255, 255, 255);
		chr->eSizeType = CROSSHAIR_SIZETYPE_ABSOLUTE;
		chr->iSize = 10;
		chr->iThick = 10;
		chr->iGap = 10;
		chr->iOutline = 5;
		chr->iCenterDot = 10;
		chr->iCircleRad = 10;
		chr->iCircleSegments = 10;
		chr->eDynamicType = CROSSHAIR_DYNAMICTYPE_GAP;
		chr->colorDot = Color(255, 255, 255, 255);
		chr->colorDotOutline = Color(255, 255, 255, 255);
		chr->colorOutline = Color(255, 255, 255, 255);
	}

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	CrosshairInfo xhairInfoImport = {};
	ImportCrosshair(&xhairInfoImport, szExportSeq);
	TEST_COMPARE_INT(xhairInfoImport.wepFlags, xhairInfo.wepFlags);
	TEST_COMPARE_INT(xhairInfoImport.hipfireFlags, xhairInfo.hipfireFlags);
	for (int i = 0; i < CROSSHAIR_WEP__TOTAL; ++i)
	{
		const CrosshairWepInfo *chr = &xhairInfo.wep[i];
		const CrosshairWepInfo *chrImp = &xhairInfoImport.wep[i];
		TEST_COMPARE_INT(chr->flags, chrImp->flags);
		TEST_COMPARE_INT(chr->iStyle, chrImp->iStyle);
		TEST_COMPARE_INT(chr->color.GetRawColor(), chrImp->color.GetRawColor());
		TEST_COMPARE_INT(chr->eSizeType, chrImp->eSizeType);
		TEST_COMPARE_INT(chr->iSize, chrImp->iSize);
		TEST_COMPARE_INT(chr->iThick, chrImp->iThick);
		TEST_COMPARE_INT(chr->iGap, chrImp->iGap);
		TEST_COMPARE_INT(chr->iOutline, chrImp->iOutline);
		TEST_COMPARE_INT(chr->iCenterDot, chrImp->iCenterDot);
		TEST_COMPARE_INT(chr->iCircleRad, chrImp->iCircleRad);
		TEST_COMPARE_INT(chr->iCircleSegments, chrImp->iCircleSegments);
		TEST_COMPARE_INT(chr->eDynamicType, chrImp->eDynamicType);
		TEST_COMPARE_INT(chr->colorDot.GetRawColor(), chrImp->colorDot.GetRawColor());
		TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), chrImp->colorDotOutline.GetRawColor());
		TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), chrImp->colorOutline.GetRawColor());
	}
}

void TestFeature_RunLengthEncode_Partial()
{
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";63;15;2;2;-1;1;0.999;10;10;5;10;10;10;1;-1;-1;-1;;;-16777217;0;10;;11;7^-16777217;;;-16777217;0;10;;11;7^-16777217;;;-16777217;0;10;;11;7^-16777217;;;-33554433;;12;;10;;12;;;3;-16777217;;-1;;;-33554433;;12;;10;;12;;;3;-16777217;;-1;;;-33554433;;12;;10;;12;;;3;-16777217;;-1;";

	CrosshairInfo xhairInfo = {};
	ResetCrosshairToDefault(&xhairInfo);
	xhairInfo.wepFlags = CROSSHAIR_WEP_FLAG_SECONDARY | CROSSHAIR_WEP_FLAG_SHOTGUN | CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE | CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE | CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE | CROSSHAIR_WEP_FLAG_THROWABLE_HIPFIRE;
	xhairInfo.hipfireFlags = CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT | CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY | CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN | CROSSHAIR_HIPFIRECUSTOM_FLAG_THROWABLE;

	{
		CrosshairWepInfo *chr = &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT];
		chr->flags = CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
		chr->iStyle = CROSSHAIR_STYLE_CUSTOM;
		chr->color = Color(255, 255, 255, 255);
		chr->eSizeType = CROSSHAIR_SIZETYPE_SCREEN;
		chr->flScrSize = 0.999;
		chr->iThick = 10;
		chr->iGap = 10;
		chr->iOutline = 5;
		chr->iCenterDot = 10;
		chr->iCircleRad = 10;
		chr->iCircleSegments = 10;
		chr->eDynamicType = CROSSHAIR_DYNAMICTYPE_GAP;
		chr->colorDot = Color(255, 255, 255, 255);
		chr->colorDotOutline = Color(255, 255, 255, 255);
		chr->colorOutline = Color(255, 255, 255, 255);
	}
	for (int i = (CROSSHAIR_WEP_DEFAULT + 1); i <= CROSSHAIR_WEP_DEFAULT_HIPFIRE; ++i)
	{
		CrosshairWepInfo *chr = &xhairInfo.wep[i];
		chr->flags = CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
		chr->iStyle = CROSSHAIR_STYLE_CUSTOM;
		chr->color = Color(255, 255, 255, 254);
		chr->eSizeType = CROSSHAIR_SIZETYPE_ABSOLUTE;
		chr->iSize = 10;
		chr->iThick = 10;
		chr->iGap = 11;
		chr->iOutline = 5;
		chr->iCenterDot = 10;
		chr->iCircleRad = 10;
		chr->iCircleSegments = 10;
		chr->eDynamicType = CROSSHAIR_DYNAMICTYPE_GAP;
		chr->colorDot = Color(255, 255, 255, 255);
		chr->colorDotOutline = Color(255, 255, 255, 255);
		chr->colorOutline = Color(255, 255, 255, 254);
	}
	for (int i = (CROSSHAIR_WEP_DEFAULT_HIPFIRE + 1); i < CROSSHAIR_WEP__TOTAL; ++i)
	{
		CrosshairWepInfo *chr = &xhairInfo.wep[i];
		chr->flags = CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
		chr->iStyle = CROSSHAIR_STYLE_CUSTOM;
		chr->color = Color(255, 255, 255, 253);
		chr->eSizeType = CROSSHAIR_SIZETYPE_ABSOLUTE;
		chr->iSize = 12;
		chr->iThick = 10;
		chr->iGap = 10;
		chr->iOutline = 5;
		chr->iCenterDot = 12;
		chr->iCircleRad = 10;
		chr->iCircleSegments = 10;
		chr->eDynamicType = CROSSHAIR_DYNAMICTYPE_SIZE;
		chr->colorDot = Color(255, 255, 255, 254);
		chr->colorDotOutline = Color(255, 255, 255, 255);
		chr->colorOutline = Color(255, 255, 255, 255);
	}

	char szExportSeq[NEO_XHAIR_SEQMAX];
	ExportCrosshair(&xhairInfo, szExportSeq);
	TEST_COMPARE_STR(szExportSeq, SERIAL_TEST_STR);
	TEST_COMPARE_INT(true, ValidateCrosshairSerial(szExportSeq));

	CrosshairInfo xhairInfoImport = {};
	ImportCrosshair(&xhairInfoImport, szExportSeq);
	TEST_COMPARE_INT(xhairInfoImport.wepFlags, xhairInfo.wepFlags);
	TEST_COMPARE_INT(xhairInfoImport.hipfireFlags, xhairInfo.hipfireFlags);
	for (int i = 0; i < CROSSHAIR_WEP__TOTAL; ++i)
	{
		const CrosshairWepInfo *chr = &xhairInfo.wep[i];
		const CrosshairWepInfo *chrImp = &xhairInfoImport.wep[i];
		TEST_COMPARE_INT(chr->flags, chrImp->flags);
		TEST_COMPARE_INT(chr->iStyle, chrImp->iStyle);
		TEST_COMPARE_INT(chr->color.GetRawColor(), chrImp->color.GetRawColor());
		TEST_COMPARE_INT(chr->eSizeType, chrImp->eSizeType);
		if (chr->eSizeType == CROSSHAIR_SIZETYPE_ABSOLUTE)
		{
			TEST_COMPARE_INT(chr->iSize, chrImp->iSize);
		}
		else
		{
			TEST_COMPARE_FLT(chr->flScrSize, chrImp->flScrSize, 0.0001f);
		}
		TEST_COMPARE_INT(chr->iThick, chrImp->iThick);
		TEST_COMPARE_INT(chr->iGap, chrImp->iGap);
		TEST_COMPARE_INT(chr->iOutline, chrImp->iOutline);
		TEST_COMPARE_INT(chr->iCenterDot, chrImp->iCenterDot);
		TEST_COMPARE_INT(chr->iCircleRad, chrImp->iCircleRad);
		TEST_COMPARE_INT(chr->iCircleSegments, chrImp->iCircleSegments);
		TEST_COMPARE_INT(chr->eDynamicType, chrImp->eDynamicType);
		TEST_COMPARE_INT(chr->colorDot.GetRawColor(), chrImp->colorDot.GetRawColor());
		TEST_COMPARE_INT(chr->colorDotOutline.GetRawColor(), chrImp->colorDotOutline.GetRawColor());
		TEST_COMPARE_INT(chr->colorOutline.GetRawColor(), chrImp->colorOutline.GetRawColor());
	}
}

struct TestFeature_WepFlags_Hipfire_data_s
{
	CrosshairInfo xhairInfo;
	char szSerial[NEO_XHAIR_SEQMAX];
};

enum TestFeature_WepFlags_Hipfire_data_e
{
	WEPFLAG_HIPFIRE_DATA_NOWEP = 0,
	WEPFLAG_HIPFIRE_DATA__TOTAL = 64 * 2,
};

void TestFeature_WepFlags_Hipfire_data(
		TestFeature_WepFlags_Hipfire_data_s (&datas)[WEPFLAG_HIPFIRE_DATA__TOTAL],
		char (&szNames)[WEPFLAG_HIPFIRE_DATA__TOTAL][DATA_XTRA_NAME_SIZE])
{
	for (int iFlags = 0; iFlags < 64; ++iFlags)
	{
		TestFeature_WepFlags_Hipfire_data_s *data = &datas[iFlags];
		ResetCrosshairToDefault(&data->xhairInfo);
		data->xhairInfo.wepFlags = iFlags;
		data->xhairInfo.hipfireFlags = 0;

		V_strcpy_safe(szNames[iFlags], "flags_empty");
		if (iFlags & CROSSHAIR_WEP_FLAG_SECONDARY)
		{
			V_strcat_safe(szNames[iFlags], "_se");
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_SHOTGUN)
		{
			V_strcat_safe(szNames[iFlags], "_sh");
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags], "_hfdef");
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags], "_hfse");
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags], "_hfsh");
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_THROWABLE_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags], "_hfth");
		}
	}

	for (int iFlags = 0; iFlags < 64; ++iFlags)
	{
		TestFeature_WepFlags_Hipfire_data_s *data = &datas[iFlags + 64];
		ResetCrosshairToDefault(&data->xhairInfo);
		data->xhairInfo.wepFlags = iFlags;
		data->xhairInfo.hipfireFlags = 0;

		V_strcpy_safe(szNames[iFlags + 64], "flags_partial");
		if (iFlags & CROSSHAIR_WEP_FLAG_SECONDARY)
		{
			V_strcat_safe(szNames[iFlags + 64], "_se");
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_SHOTGUN)
		{
			V_strcat_safe(szNames[iFlags + 64], "_sh");
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags + 64], "_hfdef");
			data->xhairInfo.hipfireFlags |= CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT;
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags + 64], "_hfse");
			data->xhairInfo.hipfireFlags |= CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY;
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags + 64], "_hfsh");
			data->xhairInfo.hipfireFlags |= CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN;
		}
		if (iFlags & CROSSHAIR_WEP_FLAG_THROWABLE_HIPFIRE)
		{
			V_strcat_safe(szNames[iFlags + 64], "_hfth");
			data->xhairInfo.hipfireFlags |= CROSSHAIR_HIPFIRECUSTOM_FLAG_THROWABLE;
		}
		for (int i = 1; i < CROSSHAIR_WEP__TOTAL; ++i)
		{
			if (iFlags & (1 << (i - 1)))
			{
				data->xhairInfo.wep[i].iStyle = CROSSHAIR_STYLE_CUSTOM;
				data->xhairInfo.wep[i].iSize = 1;
			}
		}
	}

	V_strcpy_safe(datas[0].szSerial, CURRENT_VER ";0;0;0;0;-1;");
	V_strcpy_safe(datas[1].szSerial, CURRENT_VER ";1;0;0;0;-1;3^");
	V_strcpy_safe(datas[2].szSerial, CURRENT_VER ";2;0;0;0;-1;3^");
	V_strcpy_safe(datas[3].szSerial, CURRENT_VER ";3;0;0;0;-1;6^");
	V_strcpy_safe(datas[4].szSerial, CURRENT_VER ";4;0;0;0;-1;");
	V_strcpy_safe(datas[5].szSerial, CURRENT_VER ";5;0;0;0;-1;3^");
	V_strcpy_safe(datas[6].szSerial, CURRENT_VER ";6;0;0;0;-1;3^");
	V_strcpy_safe(datas[7].szSerial, CURRENT_VER ";7;0;0;0;-1;6^");
	V_strcpy_safe(datas[8].szSerial, CURRENT_VER ";8;0;0;0;-1;");
	V_strcpy_safe(datas[9].szSerial, CURRENT_VER ";9;0;0;0;-1;3^");
	V_strcpy_safe(datas[10].szSerial, CURRENT_VER ";10;0;0;0;-1;3^");
	V_strcpy_safe(datas[11].szSerial, CURRENT_VER ";11;0;0;0;-1;6^");
	V_strcpy_safe(datas[12].szSerial, CURRENT_VER ";12;0;0;0;-1;");
	V_strcpy_safe(datas[13].szSerial, CURRENT_VER ";13;0;0;0;-1;3^");
	V_strcpy_safe(datas[14].szSerial, CURRENT_VER ";14;0;0;0;-1;3^");
	V_strcpy_safe(datas[15].szSerial, CURRENT_VER ";15;0;0;0;-1;6^");
	V_strcpy_safe(datas[16].szSerial, CURRENT_VER ";16;0;0;0;-1;");
	V_strcpy_safe(datas[17].szSerial, CURRENT_VER ";17;0;0;0;-1;3^");
	V_strcpy_safe(datas[18].szSerial, CURRENT_VER ";18;0;0;0;-1;3^");
	V_strcpy_safe(datas[19].szSerial, CURRENT_VER ";19;0;0;0;-1;6^");
	V_strcpy_safe(datas[20].szSerial, CURRENT_VER ";20;0;0;0;-1;");
	V_strcpy_safe(datas[21].szSerial, CURRENT_VER ";21;0;0;0;-1;3^");
	V_strcpy_safe(datas[22].szSerial, CURRENT_VER ";22;0;0;0;-1;3^");
	V_strcpy_safe(datas[23].szSerial, CURRENT_VER ";23;0;0;0;-1;6^");
	V_strcpy_safe(datas[24].szSerial, CURRENT_VER ";24;0;0;0;-1;");
	V_strcpy_safe(datas[25].szSerial, CURRENT_VER ";25;0;0;0;-1;3^");
	V_strcpy_safe(datas[26].szSerial, CURRENT_VER ";26;0;0;0;-1;3^");
	V_strcpy_safe(datas[27].szSerial, CURRENT_VER ";27;0;0;0;-1;6^");
	V_strcpy_safe(datas[28].szSerial, CURRENT_VER ";28;0;0;0;-1;");
	V_strcpy_safe(datas[29].szSerial, CURRENT_VER ";29;0;0;0;-1;3^");
	V_strcpy_safe(datas[30].szSerial, CURRENT_VER ";30;0;0;0;-1;3^");
	V_strcpy_safe(datas[31].szSerial, CURRENT_VER ";31;0;0;0;-1;6^");
	V_strcpy_safe(datas[32].szSerial, CURRENT_VER ";32;0;0;0;-1;");
	V_strcpy_safe(datas[33].szSerial, CURRENT_VER ";33;0;0;0;-1;3^");
	V_strcpy_safe(datas[34].szSerial, CURRENT_VER ";34;0;0;0;-1;3^");
	V_strcpy_safe(datas[35].szSerial, CURRENT_VER ";35;0;0;0;-1;6^");
	V_strcpy_safe(datas[36].szSerial, CURRENT_VER ";36;0;0;0;-1;");
	V_strcpy_safe(datas[37].szSerial, CURRENT_VER ";37;0;0;0;-1;3^");
	V_strcpy_safe(datas[38].szSerial, CURRENT_VER ";38;0;0;0;-1;3^");
	V_strcpy_safe(datas[39].szSerial, CURRENT_VER ";39;0;0;0;-1;6^");
	V_strcpy_safe(datas[40].szSerial, CURRENT_VER ";40;0;0;0;-1;");
	V_strcpy_safe(datas[41].szSerial, CURRENT_VER ";41;0;0;0;-1;3^");
	V_strcpy_safe(datas[42].szSerial, CURRENT_VER ";42;0;0;0;-1;3^");
	V_strcpy_safe(datas[43].szSerial, CURRENT_VER ";43;0;0;0;-1;6^");
	V_strcpy_safe(datas[44].szSerial, CURRENT_VER ";44;0;0;0;-1;");
	V_strcpy_safe(datas[45].szSerial, CURRENT_VER ";45;0;0;0;-1;3^");
	V_strcpy_safe(datas[46].szSerial, CURRENT_VER ";46;0;0;0;-1;3^");
	V_strcpy_safe(datas[47].szSerial, CURRENT_VER ";47;0;0;0;-1;6^");
	V_strcpy_safe(datas[48].szSerial, CURRENT_VER ";48;0;0;0;-1;");
	V_strcpy_safe(datas[49].szSerial, CURRENT_VER ";49;0;0;0;-1;3^");
	V_strcpy_safe(datas[50].szSerial, CURRENT_VER ";50;0;0;0;-1;3^");
	V_strcpy_safe(datas[51].szSerial, CURRENT_VER ";51;0;0;0;-1;6^");
	V_strcpy_safe(datas[52].szSerial, CURRENT_VER ";52;0;0;0;-1;");
	V_strcpy_safe(datas[53].szSerial, CURRENT_VER ";53;0;0;0;-1;3^");
	V_strcpy_safe(datas[54].szSerial, CURRENT_VER ";54;0;0;0;-1;3^");
	V_strcpy_safe(datas[55].szSerial, CURRENT_VER ";55;0;0;0;-1;6^");
	V_strcpy_safe(datas[56].szSerial, CURRENT_VER ";56;0;0;0;-1;");
	V_strcpy_safe(datas[57].szSerial, CURRENT_VER ";57;0;0;0;-1;3^");
	V_strcpy_safe(datas[58].szSerial, CURRENT_VER ";58;0;0;0;-1;3^");
	V_strcpy_safe(datas[59].szSerial, CURRENT_VER ";59;0;0;0;-1;6^");
	V_strcpy_safe(datas[60].szSerial, CURRENT_VER ";60;0;0;0;-1;");
	V_strcpy_safe(datas[61].szSerial, CURRENT_VER ";61;0;0;0;-1;3^");
	V_strcpy_safe(datas[62].szSerial, CURRENT_VER ";62;0;0;0;-1;3^");
	V_strcpy_safe(datas[63].szSerial, CURRENT_VER ";63;0;0;0;-1;6^");
	V_strcpy_safe(datas[64].szSerial, CURRENT_VER ";0;0;0;0;-1;");
	V_strcpy_safe(datas[65].szSerial, CURRENT_VER ";1;0;0;0;-1;;2;;;1;6^");
	V_strcpy_safe(datas[66].szSerial, CURRENT_VER ";2;0;0;0;-1;;2;;;1;6^");
	V_strcpy_safe(datas[67].szSerial, CURRENT_VER ";3;0;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[68].szSerial, CURRENT_VER ";4;1;0;0;-1;;2;;;1;6^");
	V_strcpy_safe(datas[69].szSerial, CURRENT_VER ";5;1;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[70].szSerial, CURRENT_VER ";6;1;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[71].szSerial, CURRENT_VER ";7;1;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[72].szSerial, CURRENT_VER ";8;2;0;0;-1;;2;;;1;6^");
	V_strcpy_safe(datas[73].szSerial, CURRENT_VER ";9;2;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[74].szSerial, CURRENT_VER ";10;2;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[75].szSerial, CURRENT_VER ";11;2;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[76].szSerial, CURRENT_VER ";12;3;0;0;-1;;2;;;1;17^");
	V_strcpy_safe(datas[77].szSerial, CURRENT_VER ";13;3;0;0;-1;;2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[78].szSerial, CURRENT_VER ";14;3;0;0;-1;;2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[79].szSerial, CURRENT_VER ";15;3;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[80].szSerial, CURRENT_VER ";16;4;0;0;-1;;2;;;1;6^");
	V_strcpy_safe(datas[81].szSerial, CURRENT_VER ";17;4;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[82].szSerial, CURRENT_VER ";18;4;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[83].szSerial, CURRENT_VER ";19;4;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[84].szSerial, CURRENT_VER ";20;5;0;0;-1;;2;;;1;17^");
	V_strcpy_safe(datas[85].szSerial, CURRENT_VER ";21;5;0;0;-1;;2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[86].szSerial, CURRENT_VER ";22;5;0;0;-1;;2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[87].szSerial, CURRENT_VER ";23;5;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[88].szSerial, CURRENT_VER ";24;6;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[89].szSerial, CURRENT_VER ";25;6;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[90].szSerial, CURRENT_VER ";26;6;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[91].szSerial, CURRENT_VER ";27;6;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[92].szSerial, CURRENT_VER ";28;7;0;0;-1;;2;;;1;28^");
	V_strcpy_safe(datas[93].szSerial, CURRENT_VER ";29;7;0;0;-1;;2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[94].szSerial, CURRENT_VER ";30;7;0;0;-1;;2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[95].szSerial, CURRENT_VER ";31;7;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[96].szSerial, CURRENT_VER ";32;8;0;0;-1;;2;;;1;6^");
	V_strcpy_safe(datas[97].szSerial, CURRENT_VER ";33;8;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[98].szSerial, CURRENT_VER ";34;8;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[99].szSerial, CURRENT_VER ";35;8;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[100].szSerial, CURRENT_VER ";36;9;0;0;-1;;2;;;1;17^");
	V_strcpy_safe(datas[101].szSerial, CURRENT_VER ";37;9;0;0;-1;;2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[102].szSerial, CURRENT_VER ";38;9;0;0;-1;;2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[103].szSerial, CURRENT_VER ";39;9;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;17^");
	V_strcpy_safe(datas[104].szSerial, CURRENT_VER ";40;10;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[105].szSerial, CURRENT_VER ";41;10;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[106].szSerial, CURRENT_VER ";42;10;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[107].szSerial, CURRENT_VER ";43;10;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[108].szSerial, CURRENT_VER ";44;11;0;0;-1;;2;;;1;28^");
	V_strcpy_safe(datas[109].szSerial, CURRENT_VER ";45;11;0;0;-1;;2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[110].szSerial, CURRENT_VER ";46;11;0;0;-1;;2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[111].szSerial, CURRENT_VER ";47;11;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[112].szSerial, CURRENT_VER ";48;12;0;0;-1;;2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[113].szSerial, CURRENT_VER ";49;12;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[114].szSerial, CURRENT_VER ";50;12;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[115].szSerial, CURRENT_VER ";51;12;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[116].szSerial, CURRENT_VER ";52;13;0;0;-1;;2;;;1;28^");
	V_strcpy_safe(datas[117].szSerial, CURRENT_VER ";53;13;0;0;-1;;2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[118].szSerial, CURRENT_VER ";54;13;0;0;-1;;2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[119].szSerial, CURRENT_VER ";55;13;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;28^");
	V_strcpy_safe(datas[120].szSerial, CURRENT_VER ";56;14;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[121].szSerial, CURRENT_VER ";57;14;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[122].szSerial, CURRENT_VER ";58;14;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[123].szSerial, CURRENT_VER ";59;14;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;7^2;;;1;7^2;;;1;6^");
	V_strcpy_safe(datas[124].szSerial, CURRENT_VER ";60;15;0;0;-1;;2;;;1;39^");
	V_strcpy_safe(datas[125].szSerial, CURRENT_VER ";61;15;0;0;-1;;2;;;1;7^2;;;1;39^");
	V_strcpy_safe(datas[126].szSerial, CURRENT_VER ";62;15;0;0;-1;;2;;;1;7^2;;;1;39^");
	V_strcpy_safe(datas[127].szSerial, CURRENT_VER ";63;15;0;0;-1;;2;;;1;7^2;;;1;7^2;;;1;39^");
}

void TestFeature_WepFlags_Hipfire(TestFeature_WepFlags_Hipfire_data_s *data, [[maybe_unused]] const int idx)
{
	{
		// Make sure what's expected the same as the string to check
		char szExportSeq[NEO_XHAIR_SEQMAX];
		ExportCrosshair(&data->xhairInfo, szExportSeq);
		TEST_COMPARE_STR(szExportSeq, data->szSerial);
	}
	{
		CrosshairInfo xhairInfo = {};
		ImportCrosshair(&xhairInfo, data->szSerial);

		// Make sure import from serial exactly matches what's expected
		TEST_COMPARE_INT(0, V_memcmp(&xhairInfo, &data->xhairInfo, sizeof(CrosshairInfo)));

		char szExportSeq[NEO_XHAIR_SEQMAX];
		ExportCrosshair(&xhairInfo, szExportSeq);
		TEST_COMPARE_STR(szExportSeq, data->szSerial);
	}
}

TEST_INIT()
	TEST_RUN(TestDeserial_V1_PREALPHA_V8_2);
	TEST_RUN(TestDeserial_V2_ALPHA_V17);
	TEST_RUN(TestDeserial_V3_ALPHA_V19);
	TEST_RUN(TestDeserial_V4_ALPHA_V22);
	TEST_RUN(TestDeserial_V5_ALPHA_V28);
	TEST_RUN(TestDeserial_V6_ALPHA_V29);
	TEST_RUN(TestSerial_LongestLength);
	TEST_RUN(TestFailure_Invalid_SerialValues);
	TEST_RUN(TestFailure_OutOfBoundStr_Under);
	TEST_RUN(TestFailure_OutOfBoundStr_Over);
	TEST_RUN(TestFailure_OutOfBoundValues);
	TEST_RUN(TestFeature_Omit_Style_Default);
	TEST_RUN(TestFeature_Omit_Style_AltB);
	TEST_RUN(TestFeature_Omit_SizeType_Absolute);
	TEST_RUN(TestFeature_Omit_SizeType_Screen);
	TEST_RUN(TestFeature_Omit_CircleRad_Empty);
	TEST_RUN(TestFeature_Omit_CircleRad_Used);
	TEST_RUN(TestFeature_Omit_Outline_Empty);
	TEST_RUN(TestFeature_Omit_Outline_Used);
	TEST_RUN(TestFeature_Omit_CenterDot_Empty);
	TEST_RUN(TestFeature_Omit_CenterDot_SepDotColor_Not);
	TEST_RUN(TestFeature_Omit_CenterDot_SepDotColor_NoOutline);
	TEST_RUN(TestFeature_Omit_CenterDot_SepDotColor_WithOutline);
	TEST_RUN(TestFeature_Flags_OldBool_None);
	TEST_RUN(TestFeature_Flags_OldBool_ToplineOff);
	TEST_RUN(TestFeature_Flags_OldBool_SepDotColor);
	TEST_RUN(TestFeature_Flags_OldBool_ToplineOff_SepDotColor);
	TEST_RUN(TestFeature_Flags_Topline_Off);
	TEST_RUN(TestFeature_Flags_Topline_Off_SepDotColor);
	TEST_RUN(TestFeature_RunLengthEncode_Full);
	TEST_RUN(TestFeature_RunLengthEncode_Partial);
	TEST_RUN_MULTI(TestFeature_WepFlags_Hipfire, WEPFLAG_HIPFIRE_DATA__TOTAL);
TEST_END()

