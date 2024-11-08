#include "neo_misc.h"

[[nodiscard]] bool InRect(const vgui::IntRect &rect, const int x, const int y)
{
	return IN_BETWEEN_EQ(rect.x0, x, rect.x1) && IN_BETWEEN_EQ(rect.y0, y, rect.y1);
}

[[nodiscard]] int LoopAroundMinMax(const int iValue, const int iMin, const int iMax)
{
	if (iValue < iMin)
	{
		return iMax;
	}
	else if (iValue > iMax)
	{
		return iMin;
	}
	return iValue;
}

[[nodiscard]] int LoopAroundInArray(const int iValue, const int iSize)
{
	return LoopAroundMinMax(iValue, 0, iSize - 1);
}
