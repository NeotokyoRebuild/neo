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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";31;7;3;2;-1;1;0.999;10;10;5;10;10;10;1;-1;-1;-1;2;;-16777217;0;11;11;11;4;11;11;11;2;-16777217;-16777217;-16777217;2;;-16777217;0;11;11;11;4;11;11;11;2;-16777217;-16777217;-16777217;2;;-16777217;0;11;11;11;4;11;11;11;2;-16777217;-16777217;-16777217;3;;-33554433;1;0.997;12;12;3;12;12;12;3;-33554433;-33554433;-33554433;3;;-33554433;1;0.997;12;12;3;12;12;12;3;-33554433;-33554433;-33554433;";

	CrosshairInfo xhairInfo = {};
	xhairInfo.wepFlags = CROSSHAIR_WEP_FLAG_SECONDARY | CROSSHAIR_WEP_FLAG_SHOTGUN | CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE | CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE | CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE;
	xhairInfo.hipfireFlags = CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT | CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY | CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN;

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
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));
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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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

	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_DEFAULT_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SECONDARY_HIPFIRE], sizeof(CrosshairWepInfo)));
	TEST_VERIFY(0 == V_memcmp(chr, &xhairInfo.wep[CROSSHAIR_WEP_SHOTGUN_HIPFIRE], sizeof(CrosshairWepInfo)));

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
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";31;7;3;2;-1;0;10;10;10;5;10;10;10;1;-1;-1;-1;75^";

	CrosshairInfo xhairInfo = {};
	ResetCrosshairToDefault(&xhairInfo);
	xhairInfo.wepFlags = CROSSHAIR_WEP_FLAG_SECONDARY | CROSSHAIR_WEP_FLAG_SHOTGUN | CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE | CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE | CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE;
	xhairInfo.hipfireFlags = CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT | CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY | CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN;

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
	static const char SERIAL_TEST_STR[] = CURRENT_VER ";31;7;2;2;-1;1;0.999;10;10;5;10;10;10;1;-1;-1;-1;;;-16777217;0;10;;11;7^-16777217;;;-16777217;0;10;;11;7^-16777217;;;-16777217;0;10;;11;7^-16777217;;;-33554433;;12;;10;;12;;;3;-16777217;;-1;;;-33554433;;12;;10;;12;;;3;-16777217;;-1;";

	CrosshairInfo xhairInfo = {};
	ResetCrosshairToDefault(&xhairInfo);
	xhairInfo.wepFlags = CROSSHAIR_WEP_FLAG_SECONDARY | CROSSHAIR_WEP_FLAG_SHOTGUN | CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE | CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE | CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE;
	xhairInfo.hipfireFlags = CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT | CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY | CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN;

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
TEST_END()

