"Resource/HudLayout.res"
{
	HudCrosshair
	{
		"fieldName" "HudCrosshair"
		"visible" "1"
		"enabled" "1"
		"wide"	 "1920"  // I resized these values from 640x480 because for some reason?
		"tall"	 "1080"  // The crosshair wasn't being drawn on larger resolutions. -- Xunie, July 12th, 2014
	}	
	HudScope
	{
		"fieldName"		"HudScope"
		"wide"	"640"
		"tall"  "480"			
	}
	NHudHealth
	{
		"fieldName"		"NHudHealth"
		"xpos"	"0"
		"ypos"	"446"
		"wide"	"203"
		"tall"  "32"
		"visible" "1"
		"enabled" "1"
		
		"healthtext_xpos" "6"
		"healthtext_ypos" "2"
		"healthbar_xpos" "86"
		"healthbar_ypos" "4"
		"healthbar_w" "93"
		"healthbar_h" "6"
		"healthnum_xpos" "198"
		"healthnum_ypos" "2"				
		
		"camotext_xpos" "6"
		"camotext_ypos" "12"
		"camobar_xpos" "86"
		"camobar_ypos" "14"
		"camobar_w" "93"
		"camobar_h" "6"
		"camonum_xpos" "198"
		"camonum_ypos" "12"				
		
		"sprinttext_xpos" "6"
		"sprinttext_ypos" "22"
		"sprintbar_xpos" "86"
		"sprintbar_ypos" "24"
		"sprintbar_w" "93"
		"sprintbar_h" "6"
		"sprintnum_xpos" "198"
		"sprintnum_ypos" "22"				
	}	
	NHudWeapon
	{
		"fieldName"		"NHudWeapon"
		"xpos"	"r203"
		"ypos"	"446"
		"wide"	"203"
		"tall"  "32"
		"visible" "1"
		"enabled" "1"
		
		"text_xpos" "194"
		"text_ypos" "2"
		"digit_xpos" "176"
		"digit_ypos" "6"
		"digit2_xpos" "194"
		"digit2_ypos" "16"
		
		"icon_xpos" "3"
		"icon_ypos" "5"
	}	
	TargetID
	{
		"fieldName" "TargetID"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}
	NHudRangeFinder
	{
		"fieldName"		"NHudRangeFinder"
		"xpos"	"C40"
		"ypos"	"C30"
		"wide"	"120"
		"tall"  "12"
		"visible" "1"
		"enabled" "1"
		
		"text_xpos" "2"
		"text_ypos" "1"
		"digit_xpos" "65"
		"digit_ypos" "1"		
	}
	
	NHudRoundTimer
	{
		"fieldName"		"NHudRoundTimer"
		"xpos"	"R70"
		"ypos"	"2"
		"wide"	"90"
		"tall"  "40"
		"visible" "1"
		"enabled" "1"
		"timer_xpos" "0"
		"timer_ypos" "0"
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
	}
	
	neo_iff
	{
		"fieldName"		"neo_iff"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
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
		"xpos" "r100"
		"ypos" "0"
		"wide" "100"
		"tall" "444"

		"item_tall"	"24"
		"item_wide"	"100"

		"item_spacing" "2"

		"icon_ypos"	"0"
		"icon_xpos"	"0"
		"icon_tall"	"24"
		"icon_wide"	"24"

		"text_xpos"	"26"
	}
	
	HudSuit
	{
		"fieldName"		"HudSuit"
		"xpos"	"140"
		"ypos"	"432"
		"wide"	"108"
		"tall"  "36"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		
		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "50"
		"digit_ypos" "2"
	}

	HudAmmo
	{
		"fieldName" "HudAmmo"
		"xpos"	"r150"
		"ypos"	"432"
		"wide"	"136"
		"tall"  "36"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "44"
		"digit_ypos" "2"
		"digit2_xpos" "98"
		"digit2_ypos" "16"
	}

	HudAmmoSecondary
	{
		"fieldName" "HudAmmoSecondary"
		"xpos"	"r76"
		"ypos"	"432"
		"wide"	"60"
		"tall"  "36"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"digit_xpos" "10"
		"digit_ypos" "2"
	}
	
	HudSuitPower
	{
		"fieldName" "HudSuitPower"
		"visible" "0"
		"enabled" "0"
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
	
	HudFlashlight
	{
		"fieldName" "HudFlashlight"
		"visible" "0"
		"enabled" "0"
		"xpos"	"16"
		"ypos"	"370"
		"wide"	"102"
		"tall"	"20"
		
		"text_xpos" "8"
		"text_ypos" "6"
		"TextColor"	"255 170 0 220"

		"PaintBackgroundType"	"2"
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
		"visible" "0"
		"enabled" "0"
		"Circle1Radius" "66"
		"Circle2Radius"	"74"
		"DashGap"	"16"
		"DashHeight" "4"
		"BorderThickness" "88"
	}

	HudWeaponSelection
	{
		"fieldName" "HudWeaponSelection"
		"ypos" 	"16"
		"visible" "1"
		"enabled" "1"
		"SmallBoxSize" "32"
		"LargeBoxWide" "112"
		"LargeBoxTall" "38"
		"BoxGap" "8"
		"SelectionNumberXPos" "4"
		"SelectionNumberYPos" "4"
		"SelectionGrowTime"	"0.4"
		"TextYPos" "18"
	}

	HudDeathNotice
	{
		"fieldName" "HudDeathNotice"
		"visible" "1"
		"enabled" "1"
		"xpos"	 "r640"
		"ypos"	 "12"
		"wide"	 "628"
		"tall"	 "468"

		"MaxDeathNotices" "4"
		"LineHeight"	  "22"
		"RightJustify"	  "1"	// If 1, draw notices from the right

		"TextFont"				"Default"
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
		"wide"	 "640"
		"tall"	 "480"
	}

	HudGameMessage
	{
		"fieldName" "HudMessage"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
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

	HudCloseCaption
	{
		"fieldName" "HudCloseCaption"
		"visible"	"1"
		"enabled"	"1"
		"xpos"		"c-250"
		"ypos"		"276"
		"wide"		"500"
		"tall"		"136"

		"BgAlpha"	"128"

		"GrowTime"		"0.25"
		"ItemHiddenTime"	"0.2"  // Nearly same as grow time so that the item doesn't start to show until growth is finished
		"ItemFadeInTime"	"0.15"	// Once ItemHiddenTime is finished, takes this much longer to fade in
		"ItemFadeOutTime"	"0.3"

	}

	HudChat
	{
		"fieldName" "HudChat"
		"visible" "1"
		"enabled" "1"
		"xpos"	"10"
		"ypos"	"300"
		"wide"	 "400"
		"tall"	 "100"
	}

	HudHistoryResource
	{
		"fieldName" "HudHistoryResource"
		"visible" "0"
		"enabled" "0"
		"xpos"	"r252"
		"ypos"	"40"
		"wide"	 "248"
		"tall"	 "320"

		"history_gap"	"56"
		"icon_inset"	"28"
		"text_inset"	"26"
		"NumberFont"	"HudNumbersSmall"
	}

	HudGeiger
	{
		"fieldName" "HudGeiger"
		"visible" "0"
		"enabled" "0"
		"wide"	 "640"
		"tall"	 "480"
	}

	HUDQuickInfo
	{
		"fieldName" "HUDQuickInfo"
		"visible" "0"
		"enabled" "0"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudWeapon
	{
		"fieldName" "HudWeapon"
		"visible" "0"
		"enabled" "0"
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
		"wide"	 "640"
		"tall"	 "480"
	}

	HudHintDisplay
	{
		"fieldName"	"HudHintDisplay"
		"visible"	"0"
		"enabled" "1"
		"xpos"	"r120"
		"ypos"	"r340"
		"wide"	"100"
		"tall"	"200"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_xgap"	"8"
		"text_ygap"	"8"
		"TextColor"	"255 170 0 220"

		"PaintBackgroundType"	"2"
	}

	HudSquadStatus
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

	HudPoisonDamageIndicator
	{
		"fieldName"	"HudPoisonDamageIndicator"
		"visible"	"0"
		"enabled" "1"
		"xpos"	"16"
		"ypos"	"346"
		"wide"	"136"
		"tall"	"38"
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
}