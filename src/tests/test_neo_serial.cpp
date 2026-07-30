#include "neo_crosshair.h"
#include "neo_serial.h"
#include "test_util.h"

void TestDeserialInt()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = "0;2;30;20000;-5;-2;";
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_DESERIALIZE,
		.iSeqSize = V_strlen(szMutStr),
	};
	TEST_COMPARE_INT(0, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(2, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(10, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx, 0, 10));
	TEST_COMPARE_INT(20000, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(-5, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(1, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx, 1, 2));
}

void TestSerialInt()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = {};
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_SERIALIZE,
		.iSeqSize = NEO_XHAIR_SEQMAX,
	};
	int iVal = 0;
	iVal = SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0;");
	iVal = SerialInt(2, 0, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0;2;");
	iVal = SerialInt(30, 0, COMPMODE_IGNORE, szMutStr, &ctx, 0, 10);
	TEST_COMPARE_STR(szMutStr, "0;2;10;");
	iVal = SerialInt(20000, 0, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0;2;10;20000;");
	iVal = SerialInt(-5, 0, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0;2;10;20000;-5;");
	iVal = SerialInt(-2, 0, COMPMODE_IGNORE, szMutStr, &ctx, 1, 2);
	TEST_COMPARE_STR(szMutStr, "0;2;10;20000;-5;1;");
}

void TestDeserialBool()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = "1;0;-1;5;0;";
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_DESERIALIZE,
		.iSeqSize = V_strlen(szMutStr),
	};
	TEST_COMPARE_INT(true, SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(false, SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(true, SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(true, SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(false, SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx));
}

void TestSerialBool()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = {};
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_SERIALIZE,
		.iSeqSize = NEO_XHAIR_SEQMAX,
	};
	bool bVal = 0;
	bVal = SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0;");
	bVal = SerialBool(true, false, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0;1;");
	bVal = SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0;1;0;");
}

void TestDeserialFloat()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = "0.000;0.999;1;-1.0;-999.999;999.99;123.457;-2;";
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_DESERIALIZE,
		.iSeqSize = V_strlen(szMutStr),
	};
	TEST_COMPARE_FLT(0.0f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_FLT(0.999f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_FLT(1.0f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_FLT(-1.0f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_FLT(-999.999f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_FLT(999.99f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_FLT(123.457f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_FLT(-2.0f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
}

void TestSerialFloat()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = {};
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_SERIALIZE,
		.iSeqSize = NEO_XHAIR_SEQMAX,
	};
	float flVal = 0;
	flVal = SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;");
	flVal = SerialFloat(0.999f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;0.999;");
	flVal = SerialFloat(1.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;0.999;1.000;");
	flVal = SerialFloat(-1.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;0.999;1.000;-1.000;");
	flVal = SerialFloat(-999.999f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;0.999;1.000;-1.000;-999.999;");
	flVal = SerialFloat(999.99f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;0.999;1.000;-1.000;-999.999;999.990;");
	flVal = SerialFloat(123.4567f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;0.999;1.000;-1.000;-999.999;999.990;123.457;");
	flVal = SerialFloat(2, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "0.000;0.999;1.000;-1.000;-999.999;999.990;123.457;2.000;");
}

void TestDeserialMixture()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = "1;0;12.24;-5;-2;";
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_DESERIALIZE,
		.iSeqSize = V_strlen(szMutStr),
	};
	TEST_COMPARE_INT(1, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(false, SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_FLT(12.24f, SerialFloat(0.0f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_INT(-5, SerialInt(0, 0, COMPMODE_IGNORE, szMutStr, &ctx));
	TEST_COMPARE_INT(true, SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx));
}

void TestSerialMixture()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = {};
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_SERIALIZE,
		.iSeqSize = NEO_XHAIR_SEQMAX,
	};
	bool bVal = 0;
	int iVal = 0;
	float flVal = 0;
	iVal = SerialInt(1, 0, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "1;");
	bVal = SerialBool(false, false, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "1;0;");
	flVal = SerialFloat(12.24f, 0.0f, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "1;0;12.240;");
	iVal = SerialInt(-5, 0, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "1;0;12.240;-5;");
	bVal = SerialBool(true, false, COMPMODE_IGNORE, szMutStr, &ctx);
	TEST_COMPARE_STR(szMutStr, "1;0;12.240;-5;1;");
}

void TestDeserialEmpty()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = ";;;;";
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_DESERIALIZE,
		.iSeqSize = V_strlen(szMutStr),
	};
	TEST_COMPARE_INT(2, SerialInt(2, 2, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_INT(true, SerialBool(true, true, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_FLT(12.24f, SerialFloat(12.24f, 12.24f, COMPMODE_EQUALS, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_INT(-5, SerialInt(-5, -5, COMPMODE_EQUALS, szMutStr, &ctx));
}

void TestSerialEmpty()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = {};
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_SERIALIZE,
		.iSeqSize = NEO_XHAIR_SEQMAX,
	};
	TEST_COMPARE_INT(2, SerialInt(2, 2, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_STR(szMutStr, ";");
	TEST_COMPARE_INT(true, SerialBool(true, true, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_STR(szMutStr, ";;");
	TEST_COMPARE_FLT(12.24f, SerialFloat(12.24f, 12.24f, COMPMODE_EQUALS, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_STR(szMutStr, ";;;");
	TEST_COMPARE_INT(-5, SerialInt(-5, -5, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_STR(szMutStr, ";;;;");
}

void TestDeserialRLE()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = "5;4^";
	{
		SerialContext ctx = {
			.eSerialMode = SERIALMODE_DESERIALIZE,
			.iSeqSize = V_strlen(szMutStr),
		};
		TEST_COMPARE_INT(5, SerialInt(5, 4, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(2, SerialInt(2, 2, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(true, SerialBool(true, true, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_FLT(12.24f, SerialFloat(12.24f, 12.24f, COMPMODE_EQUALS, szMutStr, &ctx), 0.001f);
		TEST_COMPARE_INT(-5, SerialInt(-5, -5, COMPMODE_EQUALS, szMutStr, &ctx));
	}
	{
		V_strcpy_safe(szMutStr, ";4^0;3^12;23;;;2;3^");
		SerialContext ctx = {
			.eSerialMode = SERIALMODE_DESERIALIZE,
			.iSeqSize = V_strlen(szMutStr),
		};
		// ;4^ (5 segments) use CompVal
		TEST_COMPARE_INT(4, SerialInt(0, 4, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(2, SerialInt(0, 2, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(true, SerialBool(false, true, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_FLT(12.24f, SerialFloat(0.0f, 12.24f, COMPMODE_EQUALS, szMutStr, &ctx), 0.001f);
		TEST_COMPARE_INT(-5, SerialInt(0, -5, COMPMODE_EQUALS, szMutStr, &ctx));
		// 0; use serial value
		TEST_COMPARE_INT(0, SerialInt(0, -5, COMPMODE_EQUALS, szMutStr, &ctx));
		// 3^ use CompVal
		TEST_COMPARE_INT(1, SerialInt(0, 1, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(2, SerialInt(0, 2, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(3, SerialInt(0, 3, COMPMODE_EQUALS, szMutStr, &ctx));
		// 12;23; use serial value
		TEST_COMPARE_INT(12, SerialInt(0, 0, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(23, SerialInt(0, 0, COMPMODE_EQUALS, szMutStr, &ctx));
		// ;; use CompVal
		TEST_COMPARE_INT(1, SerialInt(0, 1, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(5, SerialInt(0, 5, COMPMODE_EQUALS, szMutStr, &ctx));
		// 2; use serial value
		TEST_COMPARE_INT(2, SerialInt(0, 9, COMPMODE_EQUALS, szMutStr, &ctx));
		// 3^ use CompVal
		TEST_COMPARE_INT(9, SerialInt(0, 9, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(8, SerialInt(0, 8, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(7, SerialInt(0, 7, COMPMODE_EQUALS, szMutStr, &ctx));
		// Out of bounds - use CompVal
		TEST_COMPARE_INT(9, SerialInt(8, 9, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(10, SerialInt(9, 10, COMPMODE_EQUALS, szMutStr, &ctx));
		TEST_COMPARE_INT(11, SerialInt(10, 11, COMPMODE_EQUALS, szMutStr, &ctx));
	}
}

void TestSerialRLE()
{
	char szMutStr[NEO_XHAIR_SEQMAX] = {};
	SerialContext ctx = {
		.eSerialMode = SERIALMODE_SERIALIZE,
		.iSeqSize = NEO_XHAIR_SEQMAX,
	};
	TEST_COMPARE_INT(5, SerialInt(5, 4, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_STR(szMutStr, "5;");
	TEST_COMPARE_INT(2, SerialInt(2, 2, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_STR(szMutStr, "5;;");
	TEST_COMPARE_INT(true, SerialBool(true, true, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_STR(szMutStr, "5;;;");
	TEST_COMPARE_FLT(12.24f, SerialFloat(12.24f, 12.24f, COMPMODE_EQUALS, szMutStr, &ctx), 0.001f);
	TEST_COMPARE_STR(szMutStr, "5;;;;");
	TEST_COMPARE_INT(-5, SerialInt(-5, -5, COMPMODE_EQUALS, szMutStr, &ctx));
	TEST_COMPARE_STR(szMutStr, "5;;;;;");

	SerialRLEncode(szMutStr, ctx.eSerialMode);
	TEST_COMPARE_STR(szMutStr, "5;4^");

	V_strcpy_safe(szMutStr, ";;;;;");
	SerialRLEncode(szMutStr, ctx.eSerialMode);
	TEST_COMPARE_STR(szMutStr, ";4^");

	V_strcpy_safe(szMutStr, ";;;;;0;;;;");
	SerialRLEncode(szMutStr, ctx.eSerialMode);
	TEST_COMPARE_STR(szMutStr, ";4^0;3^");

	V_strcpy_safe(szMutStr, ";;;;;0;;;;12;23;;;2;;;;");
	SerialRLEncode(szMutStr, ctx.eSerialMode);
	TEST_COMPARE_STR(szMutStr, ";4^0;3^12;23;;;2;3^");
}

TEST_INIT()
	TEST_RUN(TestDeserialInt);
	TEST_RUN(TestSerialInt);
	TEST_RUN(TestDeserialBool);
	TEST_RUN(TestSerialBool);
	TEST_RUN(TestDeserialFloat);
	TEST_RUN(TestSerialFloat);
	TEST_RUN(TestDeserialMixture);
	TEST_RUN(TestSerialMixture);
	TEST_RUN(TestDeserialEmpty);
	TEST_RUN(TestSerialEmpty);
	TEST_RUN(TestDeserialRLE);
	TEST_RUN(TestSerialRLE);
TEST_END()

