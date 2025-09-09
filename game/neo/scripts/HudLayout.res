"Resource/HudLayout.res"
{
	HudHealth [!$DECK]
	{
		"fieldName"		"HudHealth"
		"xpos"	"16"
		"ypos"	"432"
		"wide"	"102"
		"tall"  "36"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "50"
		"digit_ypos" "2"
	}
	HudHealth [$DECK]
	{
		"fieldName"		"HudHealth"
		"xpos"	"16"
		"ypos"	"426"
		"wide"	"130"
		"tall"  "42"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "23"
		"digit_xpos" "66"
		"digit_ypos" "0"
	}

	TargetID
	{
		"fieldName" "TargetID"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	TeamDisplay
	{
		"fieldName" "TeamDisplay"
	    "visible" "0"
	    "enabled" "1"
		"xpos"	"16"
		"ypos"	"410" [!$DECK]
		"ypos"	"400" [$DECK]
	    "wide" "200"
	    "tall" "60"
	    "text_xpos" "8"
	    "text_ypos" "4"
	}

	HudVoiceSelfStatus
	{
		"fieldName" "HudVoiceSelfStatus"
		"visible" "1"
		"enabled" "1"
		"xpos" "0"
		"ypos" "412"
		"wide" "38"
		"tall" "32"
	}

	HudVoiceStatus
	{
		"fieldName" "HudVoiceStatus"
		"visible" "1"
		"enabled" "1"
		"xpos" "r145"
		"ypos" "0"
		"wide" "145"
		"tall" "400"

		"item_wide"	"135"

		"show_avatar"		"0"

		"show_dead_icon"	"1"
		"dead_xpos"			"1"
		"dead_ypos"			"0"
		"dead_wide"			"16"
		"dead_tall"			"16"

		"show_voice_icon"	"1"
		"icon_ypos"			"0"
		"icon_xpos"			"15"
		"icon_tall"			"16"
		"icon_wide"			"16"

		"text_xpos"			"33"
	}

	HudSuit [!$DECK]
	{
		"fieldName"		"HudSuit"
		"xpos"	"140"
		"ypos"	"432"
		"wide"	"108"
		"tall"  "36"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "50"
		"digit_ypos" "2"
	}
	HudSuit [$DECK]
	{
		"fieldName"		"HudSuit"
		"xpos"	"150"
		"ypos"	"426"
		"wide"	"120"
		"tall"  "42"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "23"
		"digit_xpos" "56"
		"digit_ypos" "0"
	}


	HudAmmo	[!$DECK]
	{
		"fieldName" "HudAmmo"
		"xpos"	"r150"
		"ypos"	"432"
		"wide"	"136"
		"tall"  "36"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "44"
		"digit_ypos" "2"
		"digit2_xpos" "98"
		"digit2_ypos" "16"
	}
	HudAmmo	[$DECK]
	{
		"fieldName" "HudAmmo"
		"xpos"	"r150"
		"ypos"	"426"
		"wide"	"152"
		"tall"  "42"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "24"
		"digit_xpos" "55"
		"digit_ypos" "0"
		"digit2_xpos" "100"
		"digit2_ypos" "12"
	}

	HudAmmoSecondary [!$DECK]
	{
		"fieldName" "HudAmmoSecondary"
		"xpos"	"r76"
		"ypos"	"432"
		"wide"	"60"
		"tall"  "36"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "22"
		"digit_xpos" "36"
		"digit_ypos" "2"
	}
	HudAmmoSecondary [$DECK]
	{
		"fieldName" "HudAmmoSecondary"
		"xpos"	"r82"
		"ypos"	"426"
		"wide"	"70"
		"tall"  "42"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "24"
		"digit_xpos" "42"
		"digit_ypos" "0"
	}

	HudSuitPower [!$DECK]
	{
		"fieldName" "HudSuitPower"
		"visible" "1"
		"enabled" "1"
		"xpos"	"16"
		"ypos"	"396"
		"wide"	"102"
		"tall"	"26"

		"AuxPowerLowColor" "255 0 0 220"
		"AuxPowerHighColor" "255 220 0 220"
		"AuxPowerDisabledAlpha" "70"

		"BarInsetX" "8"
		"BarInsetY" "15"
		"BarWidth" "92"
		"BarHeight" "4"
		"BarChunkWidth" "6"
		"BarChunkGap" "3"

		"text_xpos" "8"
		"text_ypos" "4"
		"text2_xpos" "8"
		"text2_ypos" "22"
		"text2_gap" "10"

		"PaintBackgroundType"	"2"
	}

	HudSuitPower	[$DECK]
	{
		"fieldName" "HudSuitPower"
		"visible" "1"
		"enabled" "1"
		"xpos"	"16"
		"ypos"	"386"
		"wide"	"112"
		"tall"	"54"

		"AuxPowerLowColor" "255 0 0 220"
		"AuxPowerHighColor" "255 220 0 220"
		"AuxPowerDisabledAlpha" "70"

		"BarInsetX" "8"
		"BarInsetY" "18"
		"BarWidth" "102"
		"BarHeight" "5"
		"BarChunkWidth" "6"
		"BarChunkGap" "3"

		"text_xpos" "8"
		"text_ypos" "4"
		"text2_xpos" "8"
		"text2_ypos" "26"
		"text2_gap" "14"

		"PaintBackgroundType"	"2"
	}

	HudPosture	[$WIN32]
	{
		"fieldName" 		"HudPosture"
		"visible" 		"1"
		"PaintBackgroundType"	"2"
		"xpos"	"16"
		"ypos"	"316"
		"tall"  "35"
		"wide"	"36"
		"font"	"WeaponIconsSmall"
		"icon_xpos"	"8"
		"icon_ypos" 	"-2"
	}
	HudPosture	[$X360]
	{
		"fieldName" 		"HudPosture"
		"visible" 		"1"
		"PaintBackgroundType"	"2"
		"xpos"	"48"
		"ypos"	"316"
		"tall"  "36"
		"wide"	"36"
		"font"	"WeaponIconsSmall"
		"icon_xpos"	"10"
		"icon_ypos" 	"2"
	}

	HudFlashlight
	{
		"fieldName" "HudFlashlight"
		"visible" "1"
		"PaintBackgroundType"	"2"
		"xpos"	"270"		[$WIN32]
		"ypos"	"444"		[!$DECK]
		"ypos"	"436"		[$DECK]
		"xpos_hidef"	"306"		[$X360]		// aligned to left
		"xpos_lodef"	"c-18"		[$X360]		// centered in screen
		"ypos"	"428"		[$X360]
		"tall"  "24" [!$DECK]
		"tall"  "30" [$DECK]
		"wide"	"36" [!$DECK]
		"wide"	"46" [$DECK]
		"font"	"WeaponIconsSmall" [!$DECK]
		"font"	"FlashlightDeck" [$DECK]

		"icon_xpos"	"4"
		"icon_ypos" "-8" [!$DECK]
		"icon_ypos" "-12"  [$DECK]

		"BarInsetX" "4"
		"BarInsetY" "18" [!$DECK]
		"BarInsetY" "22" [$DECK]
		"BarWidth" "28" [!$DECK]
		"BarWidth" "36" [$DECK]
		"BarHeight" "2" [!$DECK]
		"BarChunkWidth" "2" [!$DECK]
		"BarHeight" "3" [$DECK]
		"BarChunkWidth" "3" [$DECK]
		"BarChunkGap" "1"
	}
	HudDamageIndicator
	{
		"fieldName" "HudDamageIndicator"
		"visible" "1"
		"enabled" "1"
		"DmgColorLeft" "255 0 0 0"
		"DmgColorRight" "255 0 0 0"

		"dmg_xpos" "30"
		"dmg_ypos" "100"
		"dmg_wide" "36"
		"dmg_tall1" "240"
		"dmg_tall2" "200"
	}

	HudZoom
	{
		"fieldName" "HudZoom"
		"visible" "1"
		"enabled" "1"
		"Circle1Radius" "66"
		"Circle2Radius"	"74"
		"DashGap"	"16"
		"DashHeight" "4"	[$WIN32]
		"DashHeight" "6"	[$X360]
		"BorderThickness" "88"
	}
	HudWeaponSelection
	{
		"fieldName" "HudWeaponSelection"
		"ypos" 	"16"	[$WIN32]
		"ypos" 	"32"	[$X360]
		"visible" "1"
		"enabled" "1"
		"SmallBoxSize" "32"
		"MediumBoxWide"	"95"
		"MediumBoxWide_hidef"	"78"
		"MediumBoxTall"	"50"
		"MediumBoxTall_hidef"	"50"
		"MediumBoxWide_lodef"	"74"
		"MediumBoxTall_lodef"	"50"
		"LargeBoxWide" "112"
		"LargeBoxTall" "84"
		"BoxGap" "8"
		"SelectionNumberXPos" "4"
		"SelectionNumberYPos" "4"
		"SelectionGrowTime"	"0.4"
		"TextYPos" "64"
	}

	HudCrosshair
	{
		"fieldName" "HudCrosshair"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	neo_death_notice
	{
		"fieldName" 		"neo_death_notice"
		"visible" 			"1"
		"enabled" 			"1"
		//"wide"	 			"640"
		//"tall"	 			"480"
		"LineMargin"		"2"
		"MaxDeathNotices" 	"8"
		"RightJustify"		"1"
		"BackgroundColor"	"200 200 200 40"
		"BackgroundColourInvolved"	"20 20 20 220"
	}

	HudVehicle
	{
		"fieldName" "HudVehicle"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	ScorePanel
	{
		"fieldName" "ScorePanel"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudTrain
	{
		"fieldName" "HudTrain"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudMOTD
	{
		"fieldName" "HudMOTD"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudMessage
	{
		"fieldName" "HudMessage"
		"visible" "1"
		"enabled" "1"
		"wide"	 "f0"
		"tall"	 "480"
	}

	HudMenu
	{
		"fieldName" "HudMenu"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudCloseCaption
	{
		"fieldName" "HudCloseCaption"
		"visible"	"1"
		"enabled"	"1"
		"xpos"		"c-250"
		"ypos"		"276"	[$WIN32]
		"ypos"		"236"	[$X360]
		"wide"		"500"
		"tall"		"136"	[$WIN32]
		"tall"		"176"	[$X360]

		"BgAlpha"	"128"

		"GrowTime"		"0.25"
		"ItemHiddenTime"	"0.2"  // Nearly same as grow time so that the item doesn't start to show until growth is finished
		"ItemFadeInTime"	"0.15"	// Once ItemHiddenTime is finished, takes this much longer to fade in
		"ItemFadeOutTime"	"0.3"
		"topoffset"		"0"		[$WIN32]
		"topoffset"		"0"	[$X360]
	}

	HudChat
	{
		"fieldName" "HudChat"
		"visible" "0"
		"enabled" "1"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	 "4"
		"tall"	 "4"
	}

	HudHistoryResource	[$WIN32]
	{
		"fieldName" "HudHistoryResource"
		"visible" "1"
		"enabled" "1"
		"xpos"	"r272" [$DECK]
		"xpos"	"r252" [!$DECK]
		"ypos"	"40"
		"wide"	 "248"
		"tall"	 "320"

		"history_gap"	"64" [$DECK]
		"history_gap"	"56" [!$DECK]
		"icon_inset"	"38"
		"text_inset"	"36"
		"text_inset"	"26"
		"NumberFont"	"HudNumbersSmall"
	}

	HudGeiger
	{
		"fieldName" "HudGeiger"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HUDQuickInfo
	{
		"fieldName" "HUDQuickInfo"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudWeapon
	{
		"fieldName" "HudWeapon"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}
	HudAnimationInfo
	{
		"fieldName" "HudAnimationInfo"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudPredictionDump
	{
		"fieldName" "HudPredictionDump"
		"visible" "1"
		"enabled" "1"
		"wide"	 "3840"
		"tall"	 "1080"
	}

	HudHintDisplay
	{
		"fieldName"				"HudHintDisplay"
		"visible"				"0"
		"enabled"				"1"
		"xpos"					"c-240"
		"ypos"					"c60"
		"xpos"	"r148"	[$X360]
		"ypos"	"r338"	[$X360]
		"wide"					"480"
		"tall"					"100"
		"HintSize"				"1"
		"text_xpos"				"8"
		"text_ypos"				"8"
		"center_x"				"0"	// center text horizontally
		"center_y"				"-1"	// align text on the bottom
		"paintbackground"		"0"
	}

	HudHintKeyDisplay
	{
		"fieldName"	"HudHintKeyDisplay"
		"visible"	"0"
		"enabled" 	"1"
		"xpos"		"0"
		"ypos"		"420"
		"wide"		"200"
		"tall"		"200"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_xgap"	"0"
		"text_ygap"	"0"
		"TextColor"	"255 170 0 220"

		"PaintBackgroundType"	"2"
	}


	HudSquadStatus	[!$DECK]
	{
		"fieldName"	"HudSquadStatus"
		"visible"	"1"
		"enabled" "1"
		"xpos"	"r120"
		"ypos"	"380"
		"wide"	"104"
		"tall"	"46"
		"text_xpos"	"8"
		"text_ypos"	"34"
		"SquadIconColor"	"255 220 0 160"
		"IconInsetX"	"8"
		"IconInsetY"	"0"
		"IconGap"		"24"

		"PaintBackgroundType"	"2"
	}
	HudSquadStatus	[$DECK]
	{
		"fieldName"	"HudSquadStatus"
		"visible"	"1"
		"enabled" "1"
		"xpos"	"r160"
		"ypos"	"372"
		"wide"	"144"
		"tall"	"46"
		"text_xpos"	"8"
		"text_ypos"	"28"
		"SquadIconColor"	"255 220 0 160"
		"IconInsetX"	"8"
		"IconInsetY"	"-10"
		"IconGap"		"39"

		"PaintBackgroundType"	"2"
	}


	HudPoisonDamageIndicator	[!$DECK]
	{
		"fieldName"	"HudPoisonDamageIndicator"
		"visible"	"0"
		"enabled" "1"
		"xpos"	"16"
		"ypos"	"338"
		"wide"	"136"
		"tall"	"38"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_ygap" "14"
		"TextColor"	"255 170 0 220"
		"PaintBackgroundType"	"2"
	}
	HudPoisonDamageIndicator	[$DECK]
	{
		"fieldName"	"HudPoisonDamageIndicator"
		"visible"	"0"
		"enabled" "1"
		"xpos"	"16"
		"ypos"	"262"
		"wide"	"190"
		"tall"	"42"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_ygap" "14"
		"TextColor"	"255 170 0 220"
		"PaintBackgroundType"	"2"
	}
	HudCredits
	{
		"fieldName"	"HudCredits"
		"TextFont"	"Default"
		"visible"	"1"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
		"TextColor"	"255 255 255 192"

	}

	HudMenu
	{
		"fieldName" "HudMenu"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
		"zpos"	"1"
		"TextFont"	"Default"
		"ItemFont"	"Default"
		"ItemFontPulsing"	"Default"
	}

	HudRadio
	{
		"fieldName"	"HudRadio"
		"TextFont"	"Default"
		"visible"	"1"
		"xpos"	"10"
		"ypos"	"c"
		"wide"	"Default"
		"tall"	"Default"
		"text_ygap"	"2"
		"TextColor"	"255 255 255 192"
		"PaintBackgroundType"	"0"
	}

	"HudChat"
	{
		"ControlName"		"EditablePanel"
		"fieldName" 		"HudChat"
		"visible" 		"0"
		"enabled" 		"1"
		"xpos"			"10"
		"ypos"			"275"
		"wide"	 		"320"
		"tall"	 		"120"
		"PaintBackgroundType"	"2"
	}

	AchievementNotificationPanel
	{
		"fieldName"	"AchievementNotificationPanel"
		"visible"	"0"
		"enabled"	"0"
		"wide"	"0"
		"tall"	"0"
	}

	HudHintKeyDisplay
	{
		"fieldName"	"HudHintKeyDisplay"
		"visible"	"0"
		"enabled" 	"0"
	}

	HUDAutoAim
	{
		"fieldName" "HUDAutoAim"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"	[$WIN32]
		"tall"	 "480"	[$WIN32]
		"wide"	 "960"	[$X360]
		"tall"	 "720"	[$X360]
	}

	HudCommentary
	{
		"fieldName" "HudCommentary"
		"xpos"	"c-190"
		"ypos"	"350"
		"wide"	"380"
		"tall"  "40"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"bar_xpos"		"50"
		"bar_ypos"		"20"
		"bar_height"	"8"
		"bar_width"		"320"
		"speaker_xpos"	"50"
		"speaker_ypos"	"8"
		"count_xpos_from_right"	"10"	// Counts from the right side
		"count_ypos"	"8"

		"icon_texture"	"vgui/hud/icon_commentary"
		"icon_xpos"		"0"
		"icon_ypos"		"0"
		"icon_width"	"40"
		"icon_height"	"40"
	}

	HudHDRDemo
	{
		"fieldName" "HudHDRDemo"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"  "480"
		"visible" "1"
		"enabled" "1"

		"Alpha"	"255"
		"PaintBackgroundType"	"2"

		"BorderColor"	"0 0 0 255"
		"BorderLeft"	"16"
		"BorderRight"	"16"
		"BorderTop"		"16"
		"BorderBottom"	"64"
		"BorderCenter"	"0"

		"TextColor"		"255 255 255 255"
		"LeftTitleY"	"422"
		"RightTitleY"	"422"
	}

	AchievementNotificationPanel
	{
		"fieldName"				"AchievementNotificationPanel"
		"visible"				"1"
		"enabled"				"1"
		"xpos"					"0"
		"ypos"					"180"
		"wide"					"f10"	[$WIN32]
		"wide"					"f60"	[$X360]
		"tall"					"100"
	}

	CHudVote
	{
		"fieldName"		"CHudVote"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"640"
		"tall"			"480"
		"visible"		"1"
		"enabled"		"1"
		"bgcolor_override"	"0 0 0 0"
		"PaintBackgroundType"	"0" // rounded corners
	}

	NHudCompass
	{
		"fieldName"		"NHudCompass"
		"visible"		"1"
		"y_bottom_pos"		"3"
		"needle_visible"	"0"
		"needle_colored"	"0"
		"objective_visible"	"1"
		"box_color"		"200 200 200 40"
	}
	NHudWeapon
	{
		"fieldName"		"NHudWeapon"
		"visible"		"1"
		"enabled"		"1"
		"xpos"			"r203"
		"ypos"			"446"
		"wide"			"203"
		"tall"			"32"
		"box_color"		"200 200 200 40"
		"top_left_corner"	"1"
		"top_right_corner"	"0"
		"bottom_left_corner"	"1"
		"bottom_right_corner"	"0"
		"text_xpos"		"194"
		"text_ypos"		"2"
		"digit_as_number"	"0"
		"digit_xpos"		"24"
		"digit_ypos"		"6"
		"digit_max_width"	"150"
		"digit2_xpos"		"194"
		"digit2_ypos"		"16"
		"icon_xpos"		"3"
		"icon_ypos"		"5"
		"ammo_text_color"	"255 255 255 100"
		"ammo_color"		"255 255 255 150"
		"emptied_ammo_color"	"255 255 255 50"
		"heatbar_xpos"	"45"
		"heatbar_ypos"	"12"
		"heatbar_w"		"150"
		"heatbar_h"		"15"
		"heat_color"	"255 50 50 200"
	}
	NHudHealth
	{
		"fieldName"		"NHudHealth"
		"visible"		"1"
		"enabled"		"1"
		"xpos"			"0"
		"ypos"			"446"
		"wide"			"203"
		"tall"			"32"
		"box_color"		"200 200 200 40"
		"top_left_corner"	"0"
		"top_right_corner"	"1"
		"bottom_left_corner"	"0"
		"bottom_right_corner"	"1"
		"healthtext_xpos"	"6"
		"healthtext_ypos"	"2"
		"healthbar_xpos"	"86"
		"healthbar_ypos"	"4"
		"healthbar_w"		"93"
		"healthbar_h"		"6"
		"healthnum_xpos"	"198"
		"healthnum_ypos"	"2"
		"health_text_color"		"255 255 255 100"
		"health_color"		"255 255 255 150"
		"camotext_xpos"		"6"
		"camotext_ypos"		"12"
		"camobar_xpos"		"86"
		"camobar_ypos"		"14"
		"camobar_w"		"93"
		"camobar_h"		"6"
		"camonum_xpos"		"198"
		"camonum_ypos"		"12"
		"camo_text_color"		"255 255 255 100"
		"camo_color"		"255 255 255 150"
		"sprinttext_xpos"	"6"
		"sprinttext_ypos"	"22"
		"sprintbar_xpos"	"86"
		"sprintbar_ypos"	"24"
		"sprintbar_w"		"93"
		"sprintbar_h"		"6"
		"sprintnum_xpos"	"198"
		"sprintnum_ypos"	"22"
		"sprint_text_color"		"255 255 255 100"
		"sprint_color"		"255 255 255 150"
	}
	RoundResult
	{
		"fieldName"		"RoundResult"
		"image_y_offset"	"60"
		"text_y_offset"		"420"
	}
	NRoundState
	{
		"fieldName"		"NRoundState"
		"box_color"		"200 200 200 40"
		"health_monochrome"	"1"
	}
	NHudPlayerPing
	{
		"fieldName"		"NHudPlayerPing"
	}

	neo_ghost_uplink_state
	{
		"fieldName"		"neo_ghost_uplink_state"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
	}

	neo_ghost_marker
	{
		"fieldName"		"neo_ghost_marker"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
	}

	neo_ghost_beacons
	{
		"fieldName"		"neo_ghost_beacons"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
	}

	CNEOHud_GameEvent
	{
		"fieldName"		"CNEOHud_GameEvent"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
		"image_y_offset" "60"
		"text_y_offset" "420"
	}

	neo_iff
	{
		"fieldName"		"neo_iff"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
	}

	neo_hint
	{
		"fieldName"		"neo_hint"
		"xpos"	"466"
		"ypos"	"59"
		"wide"	"142"
		"tall"	"284"
	}

	neo_ghost_startup_sequence
	{
		"fieldName"		"neo_ghost_startup_sequence"
		"xpos"	"6"
		"ypos"	"440"
		"wide"	"640"
	}
	
	neo_message
	{
		"fieldName"		"neo_message"
		"xpos"	"80"
		"ypos"	"60"
		"wide"	"300"
		"tall"	"200"
	}

	neo_killer_damage_info
	{
		"fieldName"		"neo_killer_damage_info"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
	}
}
