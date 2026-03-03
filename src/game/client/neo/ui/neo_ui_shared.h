#pragma once
// Stuff shared by NeoUI, without necessarily wanting to bring in a whole other header.

namespace NeoUI {

// Whether to flash the app window in OS task bar to get user's attention for whatever reason
enum ENeoFlashTaskbarOption : int
{
	Never = 0,
	CompMatchStart,
	CompRoundStart,
	AnyMatchStart,
	AnyRoundStart,

	// Any new options must go ABOVE this line.
	// Please don't reorder existing choices for config compatibility.
	EnumCount,
	MaxValue = EnumCount
};

} // namespace NeoUI
