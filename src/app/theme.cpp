/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "theme.h"
#include "resource/inline_resource.h"
#include "../utils/bytes.h"
#include "../utils/encoding.h"
#include "../utils/file_handle.h"
#include "../utils/filesystem.h"
#include "../utils/hacks.h"
#include "../utils/image.h"
#include "../utils/texture.h"
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef THEME_FONT_RANGES_DEFAULT_NAME
#	define THEME_FONT_RANGES_DEFAULT_NAME "default"
#endif /* THEME_FONT_RANGES_DEFAULT_NAME */
#ifndef THEME_FONT_RANGES_CHINESE_NAME
#	define THEME_FONT_RANGES_CHINESE_NAME "chinese"
#endif /* THEME_FONT_RANGES_CHINESE_NAME */
#ifndef THEME_FONT_RANGES_JAPANESE_NAME
#	define THEME_FONT_RANGES_JAPANESE_NAME "japanese"
#endif /* THEME_FONT_RANGES_JAPANESE_NAME */
#ifndef THEME_FONT_RANGES_KOREAN_NAME
#	define THEME_FONT_RANGES_KOREAN_NAME "korean"
#endif /* THEME_FONT_RANGES_KOREAN_NAME */
#ifndef THEME_FONT_RANGES_CYRILLIC_NAME
#	define THEME_FONT_RANGES_CYRILLIC_NAME "cyrillic"
#endif /* THEME_FONT_RANGES_CYRILLIC_NAME */
#ifndef THEME_FONT_RANGES_THAI_NAME
#	define THEME_FONT_RANGES_THAI_NAME "thai"
#endif /* THEME_FONT_RANGES_THAI_NAME */
#ifndef THEME_FONT_RANGES_VIETNAMESE_NAME
#	define THEME_FONT_RANGES_VIETNAMESE_NAME "vietnamese"
#endif /* THEME_FONT_RANGES_VIETNAMESE_NAME */
#ifndef THEME_FONT_RANGES_POLISH_NAME
#	define THEME_FONT_RANGES_POLISH_NAME "polish"
#endif /* THEME_FONT_RANGES_POLISH_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Theme
*/

const char* const* Theme::generic_ByteHex(void) const {
	static constexpr const char* const result[256] = {
		"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
		"10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
		"20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
		"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
		"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
		"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
		"60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
		"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
		"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
		"90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
		"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
		"B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
		"C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
		"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
		"E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
		"F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"
	};

	return result;
}

const char* const* Theme::generic_ByteDec(void) const {
	static constexpr const char* const result[256] = {
		"  0", "  1", "  2", "  3", "  4", "  5", "  6", "  7",
		"  8", "  9", " 10", " 11", " 12", " 13", " 14", " 15",
		" 16", " 17", " 18", " 19", " 20", " 21", " 22", " 23",
		" 24", " 25", " 26", " 27", " 28", " 29", " 30", " 31",
		" 32", " 33", " 34", " 35", " 36", " 37", " 38", " 39",
		" 40", " 41", " 42", " 43", " 44", " 45", " 46", " 47",
		" 48", " 49", " 50", " 51", " 52", " 53", " 54", " 55",
		" 56", " 57", " 58", " 59", " 60", " 61", " 62", " 63",
		" 64", " 65", " 66", " 67", " 68", " 69", " 70", " 71",
		" 72", " 73", " 74", " 75", " 76", " 77", " 78", " 79",
		" 80", " 81", " 82", " 83", " 84", " 85", " 86", " 87",
		" 88", " 89", " 90", " 91", " 92", " 93", " 94", " 95",
		" 96", " 97", " 98", " 99", "100", "101", "102", "103",
		"104", "105", "106", "107", "108", "109", "110", "111",
		"112", "113", "114", "115", "116", "117", "118", "119",
		"120", "121", "122", "123", "124", "125", "126", "127",
		"128", "129", "130", "131", "132", "133", "134", "135",
		"136", "137", "138", "139", "140", "141", "142", "143",
		"144", "145", "146", "147", "148", "149", "150", "151",
		"152", "153", "154", "155", "156", "157", "158", "159",
		"160", "161", "162", "163", "164", "165", "166", "167",
		"168", "169", "170", "171", "172", "173", "174", "175",
		"176", "177", "178", "179", "180", "181", "182", "183",
		"184", "185", "186", "187", "188", "189", "190", "191",
		"192", "193", "194", "195", "196", "197", "198", "199",
		"200", "201", "202", "203", "204", "205", "206", "207",
		"208", "209", "210", "211", "212", "213", "214", "215",
		"216", "217", "218", "219", "220", "221", "222", "223",
		"224", "225", "226", "227", "228", "229", "230", "231",
		"232", "233", "234", "235", "236", "237", "238", "239",
		"240", "241", "242", "243", "244", "245", "246", "247",
		"248", "249", "250", "251", "252", "253", "254", "255"
	};

	return result;
}

const char* const* Theme::generic_ByteBin(void) const {
	static constexpr const char* const result[256] = {
		"00000000", "00000001", "00000010", "00000011",
		"00000100", "00000101", "00000110", "00000111",
		"00001000", "00001001", "00001010", "00001011",
		"00001100", "00001101", "00001110", "00001111",
		"00010000", "00010001", "00010010", "00010011",
		"00010100", "00010101", "00010110", "00010111",
		"00011000", "00011001", "00011010", "00011011",
		"00011100", "00011101", "00011110", "00011111",
		"00100000", "00100001", "00100010", "00100011",
		"00100100", "00100101", "00100110", "00100111",
		"00101000", "00101001", "00101010", "00101011",
		"00101100", "00101101", "00101110", "00101111",
		"00110000", "00110001", "00110010", "00110011",
		"00110100", "00110101", "00110110", "00110111",
		"00111000", "00111001", "00111010", "00111011",
		"00111100", "00111101", "00111110", "00111111",
		"01000000", "01000001", "01000010", "01000011",
		"01000100", "01000101", "01000110", "01000111",
		"01001000", "01001001", "01001010", "01001011",
		"01001100", "01001101", "01001110", "01001111",
		"01010000", "01010001", "01010010", "01010011",
		"01010100", "01010101", "01010110", "01010111",
		"01011000", "01011001", "01011010", "01011011",
		"01011100", "01011101", "01011110", "01011111",
		"01100000", "01100001", "01100010", "01100011",
		"01100100", "01100101", "01100110", "01100111",
		"01101000", "01101001", "01101010", "01101011",
		"01101100", "01101101", "01101110", "01101111",
		"01110000", "01110001", "01110010", "01110011",
		"01110100", "01110101", "01110110", "01110111",
		"01111000", "01111001", "01111010", "01111011",
		"01111100", "01111101", "01111110", "01111111",
		"10000000", "10000001", "10000010", "10000011",
		"10000100", "10000101", "10000110", "10000111",
		"10001000", "10001001", "10001010", "10001011",
		"10001100", "10001101", "10001110", "10001111",
		"10010000", "10010001", "10010010", "10010011",
		"10010100", "10010101", "10010110", "10010111",
		"10011000", "10011001", "10011010", "10011011",
		"10011100", "10011101", "10011110", "10011111",
		"10100000", "10100001", "10100010", "10100011",
		"10100100", "10100101", "10100110", "10100111",
		"10101000", "10101001", "10101010", "10101011",
		"10101100", "10101101", "10101110", "10101111",
		"10110000", "10110001", "10110010", "10110011",
		"10110100", "10110101", "10110110", "10110111",
		"10111000", "10111001", "10111010", "10111011",
		"10111100", "10111101", "10111110", "10111111",
		"11000000", "11000001", "11000010", "11000011",
		"11000100", "11000101", "11000110", "11000111",
		"11001000", "11001001", "11001010", "11001011",
		"11001100", "11001101", "11001110", "11001111",
		"11010000", "11010001", "11010010", "11010011",
		"11010100", "11010101", "11010110", "11010111",
		"11011000", "11011001", "11011010", "11011011",
		"11011100", "11011101", "11011110", "11011111",
		"11100000", "11100001", "11100010", "11100011",
		"11100100", "11100101", "11100110", "11100111",
		"11101000", "11101001", "11101010", "11101011",
		"11101100", "11101101", "11101110", "11101111",
		"11110000", "11110001", "11110010", "11110011",
		"11110100", "11110101", "11110110", "11110111",
		"11111000", "11111001", "11111010", "11111011",
		"11111100", "11111101", "11111110", "11111111"
	};

	return result;
}

Theme::Theme() {
}

Theme::~Theme() {
}

bool Theme::open(class Renderer* rnd) {
	if (_opened)
		return false;
	_opened = true;

	ImGuiStyle &style_ = ImGui::GetStyle();
	style_.DisabledAlpha = 0.45f;

	style(&styleDefault());

	static_assert(ImGuiCol_COUNT == 53, "The size of `ImGuiCol_COUNT` has been changed, consider modify corresponding array.");
	styleDefault().builtin[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	styleDefault().builtin[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	styleDefault().builtin[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	styleDefault().builtin[ImGuiCol_ChildBg]                = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	styleDefault().builtin[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	styleDefault().builtin[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	styleDefault().builtin[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDefault().builtin[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.19f, 0.18f, 0.54f);
	styleDefault().builtin[ImGuiCol_FrameBgHovered]         = ImVec4(0.16f, 0.29f, 0.18f, 0.40f);
	styleDefault().builtin[ImGuiCol_FrameBgActive]          = ImVec4(0.22f, 0.38f, 0.16f, 0.67f);
	styleDefault().builtin[ImGuiCol_TitleBg]                = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_TitleBgActive]          = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_MenuBarBg]              = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	styleDefault().builtin[ImGuiCol_ScrollbarGrab]          = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
	styleDefault().builtin[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.20f, 0.56f, 0.19f, 1.00f);
	styleDefault().builtin[ImGuiCol_CheckMark]              = ImVec4(0.34f, 0.66f, 0.51f, 1.00f);
	styleDefault().builtin[ImGuiCol_SliderGrab]             = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
	styleDefault().builtin[ImGuiCol_SliderGrabActive]       = ImVec4(0.20f, 0.56f, 0.19f, 1.00f);
	styleDefault().builtin[ImGuiCol_Button]                 = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_ButtonHovered]          = ImVec4(0.20f, 0.56f, 0.19f, 1.00f);
	styleDefault().builtin[ImGuiCol_ButtonActive]           = ImVec4(0.15f, 0.42f, 0.15f, 1.00f);
	styleDefault().builtin[ImGuiCol_Header]                 = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_HeaderHovered]          = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.56f, 0.19f, 1.00f);
	styleDefault().builtin[ImGuiCol_Separator]              = styleDefault().builtin[ImGuiCol_Border];
	styleDefault().builtin[ImGuiCol_SeparatorHovered]       = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_SeparatorActive]        = ImVec4(0.20f, 0.56f, 0.19f, 1.00f);
	styleDefault().builtin[ImGuiCol_ResizeGrip]             = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	styleDefault().builtin[ImGuiCol_ResizeGripHovered]      = ImVec4(0.17f, 0.48f, 0.16f, 1.00f);
	styleDefault().builtin[ImGuiCol_ResizeGripActive]       = ImVec4(0.20f, 0.56f, 0.19f, 1.00f);
	styleDefault().builtin[ImGuiCol_Tab]                    = ImLerp(styleDefault().builtin[ImGuiCol_Header],       styleDefault().builtin[ImGuiCol_TitleBgActive], 0.80f);
	styleDefault().builtin[ImGuiCol_TabHovered]             = styleDefault().builtin[ImGuiCol_HeaderHovered];
	styleDefault().builtin[ImGuiCol_TabActive]              = ImLerp(styleDefault().builtin[ImGuiCol_HeaderActive], styleDefault().builtin[ImGuiCol_TitleBgActive], 1.00f);
	styleDefault().builtin[ImGuiCol_TabUnfocused]           = ImLerp(styleDefault().builtin[ImGuiCol_Tab],          styleDefault().builtin[ImGuiCol_TitleBg],       0.80f);
	styleDefault().builtin[ImGuiCol_TabUnfocusedActive]     = ImLerp(styleDefault().builtin[ImGuiCol_TabActive],    styleDefault().builtin[ImGuiCol_TitleBg],       1.00f);
	styleDefault().builtin[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	styleDefault().builtin[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	styleDefault().builtin[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	styleDefault().builtin[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDefault().builtin[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	styleDefault().builtin[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.98f, 0.30f, 0.35f);
	styleDefault().builtin[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	styleDefault().builtin[ImGuiCol_NavHighlight]           = ImVec4(0.27f, 0.98f, 0.26f, 1.00f);
	styleDefault().builtin[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	styleDefault().builtin[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.08f);
	styleDefault().builtin[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.04f);
	styleDefault().projectIconScale                         = 2;
	styleDefault().screenClearingColor                      = ImGui::GetColorU32(ImVec4(0.18f, 0.20f, 0.22f, 1.00f));
	styleDefault().graphColor                               = ImGui::GetColorU32(ImVec4(0.20f, 0.56f, 0.19f, 1.00f));
	styleDefault().graphDrawingColor                        = ImGui::GetColorU32(ImVec4(0.48f, 0.48f, 0.16f, 1.00f));
	styleDefault().highlightFrameColor                      = ImGui::GetColorU32(ImVec4(0.16f, 0.19f, 0.58f, 0.54f));
	styleDefault().highlightFrameHoveredColor               = ImGui::GetColorU32(ImVec4(0.16f, 0.29f, 0.58f, 0.40f));
	styleDefault().highlightFrameActiveColor                = ImGui::GetColorU32(ImVec4(0.22f, 0.38f, 0.56f, 0.67f));
	styleDefault().highlightButtonColor                     = ImGui::GetColorU32(ImVec4(0.37f, 0.58f, 0.98f, 1.00f));
	styleDefault().highlightButtonHoveredColor              = ImGui::GetColorU32(ImVec4(0.40f, 0.64f, 0.99f, 1.00f));
	styleDefault().highlightButtonActiveColor               = ImGui::GetColorU32(ImVec4(0.35f, 0.52f, 0.97f, 1.00f));
	styleDefault().selectedButtonColor                      = ImGui::GetColorU32(ImVec4(0.34f, 0.66f, 0.51f, 1.00f));
	styleDefault().selectedButtonHoveredColor               = ImGui::GetColorU32(ImVec4(0.22f, 0.55f, 0.40f, 1.00f));
	styleDefault().selectedButtonActiveColor                = ImGui::GetColorU32(ImVec4(0.18f, 0.51f, 0.36f, 1.00f));
	styleDefault().dangerButtonColor                        = ImGui::GetColorU32(ImVec4(0.93f, 0.22f, 0.43f, 1.00f));
	styleDefault().dangerButtonHoveredColor                 = ImGui::GetColorU32(ImVec4(0.93f, 0.34f, 0.60f, 1.00f));
	styleDefault().dangerButtonActiveColor                  = ImGui::GetColorU32(ImVec4(0.93f, 0.12f, 0.39f, 1.00f));
	styleDefault().tabTextColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDefault().tabPendingColor                          = ImGui::GetColorU32(ImVec4(0.40f, 0.13f, 0.47f, 1.00f));
	styleDefault().tabPendingHoveredColor                   = ImGui::GetColorU32(ImVec4(0.50f, 0.23f, 0.57f, 1.00f));
	styleDefault().iconColor                                = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDefault().iconDisabledColor                        = ImGui::GetColorU32(ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
	styleDefault().messageColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDefault().warningColor                             = ImGui::GetColorU32(ImVec4(0.95f, 0.93f, 0.10f, 1.00f));
	styleDefault().errorColor                               = ImGui::GetColorU32(ImVec4(0.93f, 0.00f, 0.00f, 1.00f));
	styleDefault().debugColor                               = ImGui::GetColorU32(ImVec4(0.00f, 0.58f, 1.00f, 1.00f));
	styleDefault().i18nHeadColor                            = ImGui::GetColorU32(ImVec4(0.17f, 0.57f, 0.69f, 1.00f));
	styleDefault().musicSideColor                           = ImGui::GetColorU32(ImVec4(0.17f, 0.57f, 0.69f, 1.00f));
	styleDefault().musicNoteColor                           = ImGui::GetColorU32(ImVec4(0.95f, 0.50f, 0.04f, 1.00f));
	styleDefault().musicInstrumentColor                     = ImGui::GetColorU32(ImVec4(0.17f, 0.48f, 0.16f, 1.00f));
	styleDefault().musicEffectColor                         = ImGui::GetColorU32(ImVec4(0.00f, 0.58f, 1.00f, 1.00f));

	generic_Apply("Apply");
	generic_Browse("Browse...");
	generic_Build("Build");
	generic_Cancel("Cancel");
	generic_Clear("Clear");
	generic_Close("Close");
	generic_Dismiss("Dismiss");
	generic_Empty("Empty");
	generic_Enabled("Enabled");
	generic_Fill("Fill");
	generic_Goto("Goto");
	generic_Import("Import");
	generic_Install("Install");
	generic_Later("Later");
	generic_New("New");
	generic_No("No");
	generic_Noname("Noname");
	generic_None("None");
	generic_Ok("Ok");
	generic_Open("Open");
	generic_Path("Path");
	generic__Path(" Path");
	generic_Replace("Replace");
	generic_Reset("Reset");
	generic_Resize("Resize");
	generic_Resolve("Resolve");
	generic_SaveTo("Save to");
	generic_Search("Search");
	generic_Size("Size");
	generic_Uninstall("Uninstall");
	generic_Yes("Yes");

	menu_1Bpp("1BPP");
	menu_2Bpp("2BPP");
	menu_About("About...");
	menu_Activities("Activities...");
	menu_Actor("Actor");
	menu_Actors("Actors");
	menu_Add("Add");
	menu_Anchor("Anchor");
	menu_Application("Application");
	menu_ArbitraryMapping("Arbitrary Mapping");
	menu_ArbitraryMappingFile("Arbitrary Mapping File...");
	menu_Attributes("Attributes");
	menu_Bank("Bank");
	menu_Bin("BIN");
	menu_BindOnHits("Bind \"ON HITS\"...");
	menu_BindOnTrigger("Bind \"ON TRIGGER\"...");
	menu_BindUpdate("Bind \"UPDATE\"...");
	menu_BlockingDown("Blocking Down");
	menu_BlockingLeft("Blocking Left");
	menu_BlockingRight("Blocking Right");
	menu_BlockingUp("Blocking Up");
	menu_Browse("Browse...");
	menu_BrowseData("Browse Data...");
	menu_BrowseExamples("Browse Examples...");
	menu_Build("Build");
	menu_C("C");
	menu_CFile("C File...");
	menu_ClearConsole("Clear Console");
	menu_ClearRecent("Clear Recent");
	menu_Close("Close");
	menu_Code("Code");
	menu_CodeAsActor("Code (as Actor)");
	menu_CodeAsProjectile("Code (as Projectile)");
	menu_CodeFile("Code File...");
	menu_Compile("Compile");
	menu_CompileAndRun("Compile & Run");
	menu_Copy("Copy");
	menu_CopyAsImage("Copy as Image");
	menu_CopyJumpCode("Copy Jump Code");
	menu_Csv("CSV");
	menu_CsvFile("CSV File...");
	menu_Cut("Cut");
	menu_DataSequence("Data Sequence");
	menu_Dec("DEC");
	menu_DecreaseIndent("Decrease Indent");
	menu_Delete("Delete");
	menu_DeleteAll("Delete All");
	menu_Download("Download");
	menu_Duplicate("Duplicate");
	menu_EjectSourceCodeVm("Eject Source Code (VM)...");
	menu_Examples("Examples");
	menu_Export("Export...");
	menu_Find("Find");
	menu_FindInProject("Find in Project...");
	menu_FineZooming("Fine Zooming");
	menu_Font("Font");
	menu_ForSprite("For Sprite");
	menu_ForTiles("For Tiles");
	menu_FromLibrary("From Library...");
	menu_FxHammerFile("FxHammer File...");
	menu_GitHub("GitHub");
	menu_Goto("Goto");
	menu_Grids("Grids");
	menu_Help("Help");
	menu_Hex("HEX");
	menu_Hflip("H-flip");
	menu_Howto("Howto");
	menu_I18n("I18n dictionary");
	menu_Image("Image");
	menu_ImageFile("Image File...");
	menu_Import("Import...");
	menu_ImportExamples("Import Examples...");
	menu_IncreaseIndent("Increase Indent");
	menu_InsertSnippet("Insert Snippet");
	menu_IsLadder("Is Ladder");
	menu_Json("Json");
	menu_JsonFile("Json File...");
	menu_Jump("Jump");
	menu_JumpToTheActor("Jump to The Actor");
	menu_JumpToTheMap("Jump to The Map");
	menu_Kernels("Kernels...");
	menu_Library("Library");
	menu_Manual("Manual...");
	menu_Map("Map");
	menu_MoreOfficialKernels("More Official Kernels");
	menu_MoveBackward("Move Backward");
	menu_MoveDown("Move Down");
	menu_MoveForward("Move Forward");
	menu_MoveLeft("Move Left");
	menu_MoveRight("Move Right");
	menu_MoveUp("Move Up");
	menu_New("New");
	menu_OnscreenDebug("Onscreen Debug");
	menu_OnscreenGamepad("Onscreen Gamepad");
	menu_Music("Music");
	menu_Open("Open");
	menu_OpenExample("Open Example...");
	menu_Page("Page");
	menu_Palette("Palette");
	menu_PaletteBits("Palette Bits");
	menu_Paste("Paste");
	menu_Preferences("Preferences...");
	menu_Priority("Priority");
	menu_Project("Project");
	menu_Property("Property...");
	menu_Properties("Properties");
	menu_Quit("Quit");
	menu_RecordGif("Record GIF");
	menu_Redo("Redo");
	menu_Reload("Reload");
	menu_Remove("Remove");
	menu_RemoveSramState("Remove SRAM State");
	menu_Rename("Rename");
	menu_Restart("Restart");
	menu_Run("Run");
	menu_Save("Save");
	menu_SaveAsImageFile("Save as Image File...");
	menu_Scene("Scene");
	menu_Screenshot("Screenshot");
	menu_SelectAll("Select All");
	menu_SetAsFigure("Set as Figure");
	menu_Sfx("SFX");
	menu_Sort("Sort");
	menu_SortAssets("Sort Assets...");
	menu_SpriteSheet("Sprite Sheet...");
	menu_StopRecording("Stop Recording");
	menu_StopRunning("Stop Running");
	menu_SwapOrder("Swap Order");
	menu_Tiles("Tiles");
	menu_ToggleComment("Toggle Comment");
	menu_ToLowerCase("To Lower Case");
	menu_ToUpperCase("To Upper Case");
	menu_TriggerCallback("Trigger Callback");
	menu_Triggers("Triggers");
	menu_Undo("Undo");
	menu_UseByteMatrix("Use Byte Matrix");
	menu_Vflip("V-flip");
	menu_VgmFile("VGM File...");
	menu_VramDebugger("VRAM Debugger");
	menu_WavFile("WAV File...");
	menu_X0_1("x0.1");
	menu_X0_2("x0.2");
	menu_X0_5("x0.5");
	menu_X1("x1");
	menu_X2("x2");
	menu_X4("x4");
	menu_X8("x8");
	menu_X16("x16");

	status_Col("Col");
	status_Default("Default");
	status_EarliestCreated("Earliest created");
	status_EarliestModified("Earliest modified");
	status_EmptyCreateANewFrameToContinue("Empty, create a new frame to continue");
	status_EmptyCreateANewPageToContinue("Empty, create a new page to continue");
	status_Idx("Idx");
	status_LatestCreated("Latest created");
	status_LatestModified("Latest modified");
	status_Ln("Ln");
	status_OpenOrCreateAProjectToContinue("Open or create a project to continue");
	status_Pg("Pg");
	status_Pos("Pos");
	status_Readonly("Readonly");
	status_RefIsMissing("Ref is missing");
	status_Sz("Sz");
	status_Title("Title");
	status_Tl("Tl");
	status_Tls("Tls");
	status_Tokens("Tokens");
	status_Tr("Tr");

	dialogPrompt_16x16Grid("16x16 grid");
	dialogPrompt_16x16Player("16x16 player");
	dialogPrompt_2Bpp("2BPP");
	dialogPrompt_8x16("8x16");
	dialogPrompt_Actor("Actor");
	dialogPrompt_AddedAssetPage("Added asset page");
	dialogPrompt_AllowFlip("Allow flip");
	dialogPrompt_Analyzing("Analyzing...");
	dialogPrompt_Anchor("Anchor");
	dialogPrompt_Animations("Animations");
	dialogPrompt_Arbitrary("Arbitrary");
	dialogPrompt_Artist("Artist");
	dialogPrompt_AssetPageNotAdded("Asset page not added");
	dialogPrompt_Behave("Behave");
	dialogPrompt_Behaviour("Behaviour");
	dialogPrompt_BitCount("Bit count");
	dialogPrompt_Bounds("Bounds");
	dialogPrompt_Byte("Byte");
	dialogPrompt_CameraDeadZone("Camera dead zone");
	dialogPrompt_CameraPosition("Camera position");
	dialogPrompt_CannotAddMoreFrames("Cannot add more frames.");
	dialogPrompt_CannotAddMoreItems("Cannot add more items");
	dialogPrompt_CannotAddMoreLanguages("Cannot add more languages");
	dialogPrompt_CannotAddMorePages("Cannot add more pages.");
	dialogPrompt_CannotAddMoreTilesPages("Cannot add more tiles pages.");
	dialogPrompt_CannotDecodeBorderImage("Cannot decode border image");
	dialogPrompt_CannotExportProject("Cannot export project.");
	dialogPrompt_CannotFindAnyKernel("Cannot find any kernel.");
	dialogPrompt_CannotFindValidKernel("Cannot find valid kernel.");
	dialogPrompt_CannotModifyTheBuiltinAssetPage("Cannot change the builtin asset page.");
	dialogPrompt_CannotOpenFile("Cannot open file");
	dialogPrompt_CannotOpenProject("Cannot open project.");
	dialogPrompt_CannotRemoveProject("Cannot remove project.");
	dialogPrompt_CannotRemoveTheBuiltinAssetPage("Cannot remove the builtin asset page.");
	dialogPrompt_CannotRemoveTheOnlyAssetPage("Cannot remove the only asset page.");
	dialogPrompt_CannotRenameProject("Cannot rename project.");
	dialogPrompt_CannotSaveData("Cannot save data.");
	dialogPrompt_CannotSaveProject("Cannot save project.");
	dialogPrompt_CannotSaveProjectSeeTheConsoleForDetails(
		"Cannot save project. See the\n"
		"console for details."
	);
	dialogPrompt_CannotSaveSramState("Cannot save SRAM state.");
	dialogPrompt_CannotSaveToReadonlyLocations("Cannot save to readonly locations.");
	dialogPrompt_CannotUseThisImage("Cannot use this image");
	dialogPrompt_Checking("Checking...");
	dialogPrompt_ClearedProjects("Cleared projects");
	dialogPrompt_ClickToEdit("Click to edit");
	dialogPrompt_ClickToPut("Click to put");
	dialogPrompt_ClimbVelocity("Climb velocity");
	dialogPrompt_CollisionGroup("Collision group");
	dialogPrompt_ColoredOnly("Colored only");
	dialogPrompt_Comment("Comment");
	dialogPrompt_Compacted("Compacted");
	dialogPrompt_Compacting("Compacting...");
	dialogPrompt_CopiedCodeToClipboard("Copied code to clipboard");
	dialogPrompt_CopiedLineNumberToClipboard("Copied line number to clipboard");
	dialogPrompt_CopiedToClipboard("Copied to clipboard");
	dialogPrompt_Copy("Copy");
	dialogPrompt_Cpy("Cpy.");
	dialogPrompt_CreatedProject("Created project");
	dialogPrompt_CreateAPageOfMapFirst("Create a page of map first.");
	dialogPrompt_CreateAPageOfTilesFirst("Create a page of tiles first.");
	dialogPrompt_Cut("Cut");
	dialogPrompt_CutSound("Cut sound");
	dialogPrompt_Data("Data");
	dialogPrompt_Definition("Definition");
	dialogPrompt_Delay("Delay");
	dialogPrompt_Direction("Direction");
	dialogPrompt_Direction_S("S");
	dialogPrompt_Direction_E("E");
	dialogPrompt_Direction_N("N");
	dialogPrompt_Direction_W("W");
	dialogPrompt_Direction_SE("SE");
	dialogPrompt_Direction_NE("NE");
	dialogPrompt_Direction_NW("NW");
	dialogPrompt_Direction_SW("SW");
	dialogPrompt_DuplicatedProject("Duplicated project");
	dialogPrompt_Duty("Duty");
	dialogPrompt_Duty_12_5("12.5%");
	dialogPrompt_Duty_25_0("25.0%");
	dialogPrompt_Duty_50_0("50.0% (Square)");
	dialogPrompt_Duty_75_0("75.0%");
	dialogPrompt_EditingAsAnImageMayRemvoeUnusedTilesContinueAnyway(
		"Editing as an image may remove\n"
		"unused tiles. Continue anyway?"
	);
	dialogPrompt_EditProps("Edit props.");
	dialogPrompt_Effect("Effect");
	dialogPrompt_EjectedSourceCode("Ejected source code");
	dialogPrompt_Ejecting("Ejecting");
	dialogPrompt_ExportedAsset("Exported asset");
	dialogPrompt_ExportedCode("Exported code");
	dialogPrompt_ExportedProject("Exported project");
	dialogPrompt_FileNameHasNotBeenChanged("File name has not been changed.");
	dialogPrompt_Filling("Filling...");
	dialogPrompt_FillLocalPalette("Fill local palette");
	dialogPrompt_Find("Find ");
	dialogPrompt_Following("Following");
	dialogPrompt_Frame("Frame ");
	dialogPrompt_FromImage("From image");
	dialogPrompt_FromTiles("From tiles");
	dialogPrompt_Goto("Goto ");
	dialogPrompt_Gravity("Gravity");
	dialogPrompt_Hidden("Hidden");
	dialogPrompt_HitType("Hit type");
	dialogPrompt_ImportedAsset("Imported asset");
	dialogPrompt_ImportedProject("Imported project");
	dialogPrompt_Importing("Importing...");
	dialogPrompt_InBits("In bits");
	dialogPrompt_Index("Index");
	dialogPrompt_InitialOffset("Initial offset");
	dialogPrompt_InitialVolume("Initial volume");
	dialogPrompt_InstalledKernel("Installed kernel \"{0}\".");
	dialogPrompt_Installing("Installing...");
	dialogPrompt_Interval("Interval");
	dialogPrompt_InvalidBorderImageSize256x224pxRequired(
		"Invalid border image size,\n"
		"256x224px required"
	);
	dialogPrompt_InvalidData("Invalid data");
	dialogPrompt_InvalidDataSeeTheConsoleWindowForDetails("Invalid data; see the console window for details.");
	dialogPrompt_InvalidFile("Invalid file.");
	dialogPrompt_Inverted("Inverted");
	dialogPrompt_ItemAlreadyExists("Item already exists");
	dialogPrompt_JumpGravity("Jump gravity");
	dialogPrompt_LanguageAlreadyExists("Language already exists");
	dialogPrompt_Languages("Languages");
	dialogPrompt_Length("Length");
	dialogPrompt_LifeTime("Lift time");
	dialogPrompt_LinuxFileDialogRequirements(
		"Requires zenity/matedialog/qarma/kdialog\n"
		"to show file dialogs. (To open or save\n"
		"something.)"
	);
	dialogPrompt_Loading("Loading...");
	dialogPrompt_Map("Map:");
	dialogPrompt_MapPage("Map page");
	dialogPrompt_MaxJumpCount("Max jump count");
	dialogPrompt_MaxJumpTicks("Max jump ticks");
	dialogPrompt_MaxSize("Max size");
	dialogPrompt_Missing("Missing");
	dialogPrompt_MoveSpeed("Move speed");
	dialogPrompt_MultipleMapAssetsReferenceTheSourceTilesAssetCreateANewTilesAssetPageForImporting(
		"Multiple map assets reference the\n"
		"source tiles asset. Create a new\n"
		"tiles asset page for importing?"
	);
	dialogPrompt_MultipleMapAssetsReferenceTheSourceTilesAssetContinueAnyway(
		"Multiple map assets reference the\n"
		"source tiles asset. Editing as an\n"
		"image may break other maps, and\n"
		"remove unused tiles. Continue\n"
		"anyway?"
	);
	dialogPrompt_Name("Name");
	dialogPrompt_NoData("No data");
	dialogPrompt_NoInit("No init");
	dialogPrompt_NoIssuesFound("No issues found.");
	dialogPrompt_NoNeedToBuildRom("No need to build ROM");
	dialogPrompt_NoNr1x("No NR1x");
	dialogPrompt_NoNr2x("No NR2x");
	dialogPrompt_NoNr3x("No NR3x");
	dialogPrompt_NoNr4x("No NR4x");
	dialogPrompt_NoNr5x("No NR5x");
	dialogPrompt_NoPreview("No preview");
	dialogPrompt_NoValidContent("No valid content.");
	dialogPrompt_NoValidProject("No valid project.");
	dialogPrompt_NoWave("No wave");
	dialogPrompt_NothingAvailable("Nothing available");
	dialogPrompt_NotReferencedByMapNoIssuesFound("Not referenced by map.\nNo issues found.");
	dialogPrompt_ObjectList("Object list");
	dialogPrompt_Offset("Offset");
	dialogPrompt_OnHits("On hits");
	dialogPrompt_OpenedProject("Opened project");
	dialogPrompt_Opening("Opening");
	dialogPrompt_Optimize("Optimize");
	dialogPrompt_Orders("Orders");
	dialogPrompt_Persistent("Persistent");
	dialogPrompt_Pinned("Pinned");
	dialogPrompt_Pitch("Pitch");
	dialogPrompt_Pitch_InTiles("Pitch (in tiles)");
	dialogPrompt_Processing("Processing...");
	dialogPrompt_ProjectAlreadyExists("Project already exists.");
	dialogPrompt_Projectile("Projectile");
	dialogPrompt_Pst("Pst.");
	dialogPrompt_Refill("Refill");
	dialogPrompt_RefMapSizeHasBeenChanged("Ref map size has been changed");
	dialogPrompt_ReloadedProject("Reloaded project");
	dialogPrompt_Reloading("Reloading");
	dialogPrompt_RemovedAssetPage("Removed asset page");
	dialogPrompt_RemovedProject("Removed project");
	dialogPrompt_RemovedUnreferencedActorRoutineOverriding("Removed unreferenced actor routine overriding");
	dialogPrompt_RenamedProject("Renamed project");
	dialogPrompt_Rendering("Rendering...");
	dialogPrompt_Repeat("Repeat");
	dialogPrompt_ReplacedKernel("Replaced kernel \"{0}\".");
	dialogPrompt_ResourceSizeIsNotAMultipleOf8x8("Resource size is not a multiple of 8x8");
	dialogPrompt_ResourceSizeOutOfBounds("Resource size out of bounds");
	dialogPrompt_Routines("Routines");
	dialogPrompt_Rst("Rst.");
	dialogPrompt_SavedData("Saved data");
	dialogPrompt_Saving("Saving");
	dialogPrompt_SearchForFound("Search for \"{0}\", found {1}.");
	dialogPrompt_SearchForFoundDoubleClickToJump("Search for \"{0}\", found {1}, double click to jump:");
	dialogPrompt_SelectActor("Select actor");
	dialogPrompt_SetColor("Set color");
	dialogPrompt_Shape("Shape");
	dialogPrompt_Size("Size");
	dialogPrompt_Size_InPixels("Size (in pixels)");
	dialogPrompt_Size_InTiles("Size (in tiles)");
	dialogPrompt_SizeOfLayersDoNotMatch("Size of layers do not match");
	dialogPrompt_SomeIssuesWereFoundClickTheWarningIconForDetails(
		"Some issues were found. Click the\n"
		"warning icon for details."
	);
	dialogPrompt_SomeUnusedFramesWereFoundClickTheWarningIconForDetails(
		"Some unused frames were found.\n"
		"Click the warning icon for\n"
		"details."
	);
	dialogPrompt_Split("Split");
	dialogPrompt_Strength("Strength");
	dialogPrompt_Stroke("Stroke");
	dialogPrompt_SweepChange("Sweep change");
	dialogPrompt_SweepShift("Sweep shift");
	dialogPrompt_SweepTime("Sweep time");
	dialogPrompt_SweepTime_Off("Off");
	dialogPrompt_SweepTime_1(" 7.8ms (1/128Hz)");
	dialogPrompt_SweepTime_2("15.6ms (2/128Hz)");
	dialogPrompt_SweepTime_3("23.4ms (3/128Hz)");
	dialogPrompt_SweepTime_4("31.3ms (4/128Hz)");
	dialogPrompt_SweepTime_5("39.1ms (5/128Hz)");
	dialogPrompt_SweepTime_6("46.9ms (6/128Hz)");
	dialogPrompt_SweepTime_7("54.7ms (7/128Hz)");
	dialogPrompt_TheProjectIsNotSavedYet("The project is not saved yet.");
	dialogPrompt_TheRomIsEncodedInTheUrlOfTheOpenedBrowserPageMakeSureYourBrowserSupportsLongUrl(
		"The ROM is encoded in the URL of\n"
		"the opened browser page. Make sure\n"
		"your browser supports long URL."
	);
	dialogPrompt_Thresholds("Thresholds");
	dialogPrompt_TicksPerRow("Ticks per row");
	dialogPrompt_Tiles("Tiles:");
	dialogPrompt_TilesPage("Tiles page");
	dialogPrompt_TooManyColors("Too many colors");
	dialogPrompt_Transferring("Transferring...");
	dialogPrompt_Trim("Trim");
	dialogPrompt_UninstalledKernel("Uninstalled kernel \"{0}\".");
	dialogPrompt_UnsupportedData("Unsupported data.");
	dialogPrompt_UnsupportedOperation("Unsupported operation.");
	dialogPrompt_UnsupportedRom("Unsupported ROM.");
	dialogPrompt_UseGravity("Use gravity");
	dialogPrompt_UsePan("Use pan");
	dialogPrompt_ViewAs("View as");
	dialogPrompt_Volume("Volume");
	dialogPrompt_Volume_100("100%");
	dialogPrompt_Volume_25(" 25%");
	dialogPrompt_Volume_50(" 50%");
	dialogPrompt_Volume_Mute("Mute");
	dialogPrompt_Waveform("Waveform");
	dialogPrompt_Waveform_0("Waveform 0");
	dialogPrompt_Waveform_1("Waveform 1");
	dialogPrompt_Waveform_2("Waveform 2");
	dialogPrompt_Waveform_3("Waveform 3");
	dialogPrompt_Waveform_4("Waveform 4");
	dialogPrompt_Waveform_5("Waveform 5");
	dialogPrompt_Waveform_6("Waveform 6");
	dialogPrompt_Waveform_7("Waveform 7");
	dialogPrompt_Waveform_8("Waveform 8");
	dialogPrompt_Waveform_9("Waveform 9");
	dialogPrompt_Waveform_10("Waveform 10");
	dialogPrompt_Waveform_11("Waveform 11");
	dialogPrompt_Waveform_12("Waveform 12");
	dialogPrompt_Waveform_13("Waveform 13");
	dialogPrompt_Waveform_14("Waveform 14");
	dialogPrompt_Waveform_15("Waveform 15");
	dialogPrompt_WordWrap("Word wrap");
	dialogPrompt_Writing("Writing");
	dialogPrompt_KernelError_AKernelWithIdAlreadyExists("A kernel with ID \"{0}\" already exists.");
	dialogPrompt_KernelError_CannotCreateKernelDirectory("Cannot create kernel directory.");
	dialogPrompt_KernelError_CannotFindKernelManifest("Cannot find kernel manifest.");
	dialogPrompt_KernelError_CannotInstallKernelFiles("Cannot install kernel files.");
	dialogPrompt_KernelError_CannotOpenKernelPackage("Cannot open kernel package.");
	dialogPrompt_KernelError_CannotReadKernelManifest("Cannot read kernel manifest.");
	dialogPrompt_KernelError_CannotReadKernelPackage("Cannot read kernel package.");
	dialogPrompt_KernelError_MissingKernelComponent("Missing kernel component.");
	dialogAsk_BrowseTheExportedFile("Browser the exported file?");
	dialogAsk_ClearAllRecentProjects("Clear all recent projects?");
	dialogAsk_ProjectHasBeenBuiltAndIsBeingHostedAt_StopHostingAndBrowseThePackage(
		"The project has been built\n"
		"and is being hosted at:\n"
		"  {0}.\n"
		"Stop hosting and browse the\n"
		"package?"
	);
	dialogAsk_RemoveFromDisk("Remove from disk");
	dialogAsk_RemoveTheCurrentAssetPage("Remove the current asset page?");
	dialogAsk_RemoveTheCurrentProject("Remove the current project?");
	dialogAsk_SaveTheCurrentProject("Save the current project?");
	dialogAsk_KernelQuestion_AKernelWithIdAlreadyExistsOverwrite(
		"A kernel with ID \"{0}\" already\n"
		"exists. Overwrite?"
	);
	dialogInput_Input("Input");
	dialogInput_ProjectName("Project name:");
	dialogProjectCreating_Kernel("Kernel:");
	dialogProjectCreating_ProjectName("Project name:");
	dialogProjectCreating_StarterKit("Starter kit:");

	tabPreferences_Debug("Debug");
	tabPreferences_Device("Device");
	tabPreferences_Emulator("Emulator");
	tabPreferences_Graphics("Graphics");
	tabPreferences_Input("Input");
	tabPreferences_Main("Main");
	tabPreferences_Misc("Misc.");

	tabProjectProperty_Advanced("Advanced");
	tabProjectProperty_Compiling("Compiling");
	tabProjectProperty_Project("Project");

	tabMusic_Piano("Piano");
	tabMusic_Tracker("Tracker");
	tabMusic_Waves("Waves");

	windowRecent_Created("Created: %s");
	windowRecent_Modified("Modified: %s");

	windowBuildingSettings("Build Settings");
	windowBuildingSettings_Cart("Cart");
	windowBuildingSettings_CartridgeTypeOfOutputRom("Cartridge type of output ROM:");
	windowBuildingSettings_GameLanguage("Game language:");
	windowBuildingSettings_I18n("I18n");
	windowBuildingSettings_I18n_("I18n            ");
	windowBuildingSettings_Misc("Misc");
	windowBuildingSettings_NotSpecified("Not specified");
	windowBuildingSettings_Rtc("RTC");
	windowBuildingSettings_Rumble("Rumble");
	windowBuildingSettings_Sram("SRAM");
	windowBuildingSettings_Sram_None("0KB (None)");

	windowPreferences("Preferences");
	windowPreferences_Main_2Spaces("2 spaces");
	windowPreferences_Main_40("40");
	windowPreferences_Main_4Spaces("4 spaces");
	windowPreferences_Main_60("60");
	windowPreferences_Main_80("80");
	windowPreferences_Main_8Spaces("8 spaces");
	windowPreferences_Main_100("100");
	windowPreferences_Main_120("120");
	windowPreferences_Main_Application("Application:");
	windowPreferences_Main_ClearOnStart("Clear on start");
	windowPreferences_Main_CodeEditor("Code editor:");
	windowPreferences_Main_ColumnIndicator("Comlmn indicator");
	windowPreferences_Main_Console("Console:");
	windowPreferences_Main_IndentWith("     Indent with");
	windowPreferences_Main_None("None");
	windowPreferences_Main_ShowProjectPathAtTitleBar("Show project path at title bar");
	windowPreferences_Main_ShowWhiteSpaces("Show white spaces");
	windowPreferences_Main_Tab2SpacesWide("Tab (2 spaces wide)");
	windowPreferences_Main_Tab4SpacesWide("Tab (4 spaces wide)");
	windowPreferences_Main_Tab8SpacesWide("Tab (8 spaces wide)");
	windowPreferences_Emulator_ApplicationIcon("Application icon");
	windowPreferences_Emulator_InvalidIconSize_512x512IsRecommended("Invalid icon size, 512x512 is recommended");
	windowPreferences_Emulator_LaunchOptions("Launch options  ");
	windowPreferences_Emulator_ShowPreferenceDialogOnEscPress("Show preference dialog on ESC press");
	windowPreferences_Emulator_ShowStatusBar("Show status bar");
	windowPreferences_Emulator_ShowTitleBar("Show title bar");
	windowPreferences_Debug_Compiler("Compiler:");
	windowPreferences_Debug_Debugger("Debugger:");
	windowPreferences_Debug_Emulation("Emulation:");
	windowPreferences_Debug_EnableLogging("Enable logging");
	windowPreferences_Debug_EnableOnscreenDebug("Enable onscreen debug");
	windowPreferences_Debug_EnableVramDebugger("Enable VRAM debugger");
	windowPreferences_Debug_Log("Log:");
	windowPreferences_Debug_Path("Path");
	windowPreferences_Debug_ShowAstEnabled("Show AST");
	windowPreferences_Device_Classic("Classic");
	windowPreferences_Device_ClassicWithExtension("Classic with extension");
	windowPreferences_Device_Colored("Colored");
	windowPreferences_Device_ColoredWithExtension("Colored with extension");
	windowPreferences_Device_EmulationPalette("Emulation palette");
	windowPreferences_Device_Emulator("Emulator:");
	windowPreferences_Device_PreferSgb("Prefer SGB");
	windowPreferences_Device_SaveSramOnStop("Save SRAM on stop");
	windowPreferences_Device_Type("Type");
	windowPreferences_Graphics_Application("Application:");
	windowPreferences_Graphics_BorderlessWindow("Borderless window (F11)");
	windowPreferences_Graphics_Canvas("Canvas:");
	windowPreferences_Graphics_FixCanvasRatio("Fix canvas ratio");
	windowPreferences_Graphics_Fullscreen("Fullscreen");
	windowPreferences_Graphics_IntegerScale("Integer scale");
	windowPreferences_Input_ClickAgainToCancelBackspaceToClear("(Click again to cancel, Backspace to clear)");
	windowPreferences_Input_ClickToChange("(Click to change)");
	windowPreferences_Input_Gamepads("Gamepads:");
	windowPreferences_Input_Onscreen("Onscreen");
	windowPreferences_Input_Onscreen_Enabled("Enabled");
	windowPreferences_Input_Onscreen_PaddingX("Padding X");
	windowPreferences_Input_Onscreen_PaddingY("Padding Y");
	windowPreferences_Input_Onscreen_Scale("    Scale");
	windowPreferences_Input_Onscreen_SwapAB("Swap A/B");
	windowPreferences_Input_WaitingForInput("Waiting for input...");

	windowInstalledKernels("Installed Kernels");
	windowInstalledKernels_Kernels("Kernels:");
	windowInstalledKernels_KernelTitleUsed("{0} (used: {1})");
	WindowInstalledKernels_ClickAgainToUninstall("Click again to uninstall \"%s\"");

	windowCreateProject("Create Project");

	windowProjectProperty("Property");
	windowProjectProperty_Project_Title("  Title");    windowProjectProperty_Project_Path("Path ");
	windowProjectProperty_Project_Kernel(" Kernel");   windowProjectProperty_Project_Cart("Cart ");
	windowProjectProperty_Project_Sram("   SRAM");     windowProjectProperty_Project_Misc("Misc ");  windowProjectProperty_Project_Rtc("RTC");  windowProjectProperty_Project_Rumble("Rumble");
	windowProjectProperty_Project_Desc("   Desc");
	windowProjectProperty_Project_Author(" Author");  windowProjectProperty_Project_Genre("Genre");
	windowProjectProperty_Project_Version("Version");   windowProjectProperty_Project_Url("URL  ");
	windowProjectProperty_Project_Running("Running");
	windowProjectProperty_Project_Icon("   Icon");
	windowProjectProperty_Project_Sram_None("0KB (None)");
	windowProjectProperty_Project_Running_StartOnOpen("Start on open");
	windowProjectProperty_Compiling_Compiler("Compiler:");
	windowProjectProperty_Compiling_Compiler_I18n("  I18n");
	windowProjectProperty_Compiling_Compiler_Macros("Macros");
	windowProjectProperty_Compiling_Compiler_Optimize("Optimize");
	windowProjectProperty_Compiling_Compiler_StrictOn("Strict on");
	windowProjectProperty_Compiling_Parser("Parser:");
	windowProjectProperty_Compiling_Parser_CaseInsensitive("Case-insensitive");
	windowProjectProperty_Advanced_SgbFeatures("SGB features:");
	windowProjectProperty_Advanced_SgbFeatures_Enabled("Enabled");
	windowProjectProperty_Advanced_Palettes("Palettes");
	windowProjectProperty_Advanced_PaletteFormat("Pal: {0}, Col: {1}");
	windowProjectProperty_Advanced_Border("  Border");
	windowProjectProperty_Advanced_Border_None("None");
	windowProjectProperty_Advanced_Border_Default("Default");
	windowProjectProperty_Advanced_Border_Custom("Custom");
	windowProjectProperty_Advanced_Preview(" Preview");

	windowSortAssets("Sort Assets");

	windowSearch("Search");

	windowGlobalPalette("Global Palette");

	windowCode_SearchFor("Search for");

	windowEmulator_VramDebugger("VRAM Debugger");
	windowEmulator_VramDebugger_BgWinMap("BG/WIN map");
	windowEmulator_VramDebugger_Layer("Layer");
	windowEmulator_VramDebugger_Oam("OAM");
	windowEmulator_VramDebugger_Palettes("Palettes");
	windowEmulator_VramDebugger_StatusReadonly("Status (readonly)");
	windowEmulator_VramDebugger_StatusReadonly_BgOn("BG on");
	windowEmulator_VramDebugger_StatusReadonly_ObjOn("OBJ on");
	windowEmulator_VramDebugger_StatusReadonly_WinOn("WIN on");
	windowEmulator_VramDebugger_StatusReadonly_TileArea("Tile area: 0x{0}");
	windowEmulator_VramDebugger_StatusReadonly_BgArea("  BG area: 0x{0}");
	windowEmulator_VramDebugger_StatusReadonly_WinArea(" WIN area: 0x{0}");
	windowEmulator_VramDebugger_Tiles("Tiles");

	windowTiles_Analize("Analyze");
	windowTiles_CreateMap("Create map");

	windowMap("Map");
	windowMap_1_Graphics("1. Graphics");
	windowMap_2_Attributes("2. Attributes");
	windowMap_CreateScene("Create scene");
	windowMap_EditAsImage("Edit as image");
	windowMap_LocalPalette("Local palette");
	windowMap_RefTiles("Tiles #{0}");

	windowAudio_Duty1("DUTY 1");
	windowAudio_Duty2("DUTY 2");
	windowAudio_Instruments("Instruments");
	windowAudio_Noise("NOISE");
	windowAudio_Wave("WAVE");

	windowSfx("SFX");

	windowFont("Font");
	windowFont_Arbitrary("Arbitrary");
	windowFont_FullWordForAll("Full word for all");
	windowFont_FullWordForAscii("Full word for ASCII");
	windowFont_Outline("Outline");
	windowFont_Shadow("Shadow");

	windowArbitrary_Basic("Basic");
	windowArbitrary_Ranges("Ranges");
	windowArbitrary_Ranges_Preset("Preset");
	windowArbitrary_Ranges_Range(" Range");

	windowActor("Actor");
	windowActor_Analize("Analyze");
	windowActor_Properties("Properties");

	windowScene("Scene");
	windowScene_1_Actors("1. Actors");
	windowScene_2_Triggers("2. Triggers");
	windowScene_3_Properties("3. Properties");
	windowScene_4_Attributes("4. Attributes");
	windowScene_5_Map("5. Map");
	windowScene_CodeBinding("Code Binding");
	windowScene_RefMap("Map #{0}");
	windowScene_TriggerEventTypeBoth("Both");
	windowScene_TriggerEventTypeEnter("Enter");
	windowScene_TriggerEventTypeLeave("Leave");
	windowScene_TriggerEventTypeNone("None");

	windowSearchResult("Search Result");
	windowSearchResult_CaseSensitive("Case sensitive");
	windowSearchResult_GlobalSearch("Global search");
	windowSearchResult_MatchWholeWords("Match whole words");

	windowKernelDocument("Kernel Document");

	windowActivities("Activities");
	windowActivities_Activities("Activities:");
	windowActivities_ContentInfo(
		" Total opened projects: {0}\n"
		"         Opened BASICs: {1}\n"
		"           Opened ROMs: {2}\n"
		"----------------------------------------\n"
		"Total created projects: {3}\n"
		"        Created BASICs: {4}\n"
		"----------------------------------------\n"
		"    Total played times: {5}\n"
		"      Played BASIC for: {6}\n"
		"        Played ROM for: {7}\n"
		"----------------------------------------\n"
		"     Total played time: {8}\n"
		"          Played BASIC: {9}\n"
		"            Played ROM: {10}\n"
	);

	windowAbout("About");

	tooltip_Analyzing("Analyzing...");
	tooltip_Back("Back");
	tooltip_Bit0("Bit 0");
	tooltip_Bit1("Bit 1");
	tooltip_Bit2("Bit 2");
	tooltip_Bit3("Bit 3");
	tooltip_Bit4("Bit 4");
	tooltip_Bit5("Bit 5");
	tooltip_Bit6("Bit 6");
	tooltip_Bit7("Bit 7");
	tooltip_BitwiseOperations("Bitwise operations");
	tooltip_BitwiseSet("Set directly");
	tooltip_BitwiseAnd("a AND b");
	tooltip_BitwiseOr("a OR b");
	tooltip_BitwiseXor("a XOR b");
	tooltip_Browse("Browse");
	tooltip_CaseSensitive("Case-sensitive");
	tooltip_ChangeRefFont("Change ref font");
	tooltip_ChangeRefMap("Change ref map");
	tooltip_ChangeRefTiles("Change ref tiles");
	tooltip_CheckToPreferSgbRatherThanColoredDevice("Check to prefer SGB rather than colored device");
	tooltip_Clear("Clear");
	tooltip_Compacting("Compacting...");
	tooltip_Compiling("Compiling...");
	tooltip_Delete("Delete");
	tooltip_DeleteFrame("Delete frame");
	tooltip_EjectSourceCodeVm("Eject Source Code (VM)");
	tooltip_Export("Export");
	tooltip_ForAlignToTileMovementForControllers(
		"For align-to-tile movement for controllers\n"
		"Enable for 16x16px aligned, otherwise 8x8px aligned\n"
		"(Usually leave disabled)"
	);
	tooltip_ForAutoAlignToTileMovementForControllers(
		"Determined automatically!\n"
		"For auto-align-to-tile movement for controllers\n"
		"Enable for 16x16px-based auto aligning, otherwise 8x8px-based"
	);
	tooltip_GenerateAndCopyCode("Generate and copy code");
	tooltip_GlobalSearch("Global search");
	tooltip_HighBits("High bits");
	tooltip_HoldCtrlToApplyToTheCurrentFrameOnly("Hold Ctrl to apply to the current frame only");
	tooltip_HoldShiftToModifyWithLargerBounds("Hold Shift to modify with larger bounds");
	tooltip_IconView("Icon view");
	tooltip_Import("Import");
	tooltip_ImportAsNew("Import as new");
	tooltip_InCsvFormat("In CSV format");
	tooltip_InstalledKernels("Installed kernels");
	tooltip_JumpToRefMap("Jump to ref map");
	tooltip_JumpToRefTiles("Jump to ref tiles");
	tooltip_LayerActors("Placed actors");
	tooltip_LayerAttributes("Palette, bank, flip, priorities, etc.");
	tooltip_LayerMap("Graphics/visual");
	tooltip_LayerProperties("Blocking, ladder information, etc.");
	tooltip_LayerTriggers("Placed trigger definitions");
	tooltip_ListView("List view");
	tooltip_LmbToPickRmbToChange("LMB to pick, RMB to change");
	tooltip_LowBits("Low bits");
	tooltip_MatchWholeWords("Match whole words");
	tooltip_Menu("Menu");
	tooltip_MonoEditor("Mono editor");
	tooltip_OpenRefPalette("Open ref palette");
	tooltip_OpenRefPaletteTurnOffPaletteBitsPreviewToUseThis(
		"Open ref palette\n"
		"(Turn off palette bits preview to use this)"
	);
	tooltip_OrderedByDefault("Ordered by default");
	tooltip_OrderedByEarliestCreated("Ordered by earliest created");
	tooltip_OrderedByEarliestModified("Ordered by earliest modified");
	tooltip_OrderedByLatestCreated("Ordered by latest created");
	tooltip_OrderedByLatestModified("Ordered by latest modified");
	tooltip_OrderedByTitle("Ordered by title");
	tooltip_OverrideThreaded("Override; threaded");
	tooltip_Pause("Pause");
	tooltip_PreviewAnchorPoint("Preview anchor point");
	tooltip_PreviewPaletteBitsForColoredOnly("Preview palette bits (for colored only)");
	tooltip_Refreshing("Refreshing...");
	tooltip_Reset("Reset");
	tooltip_ResizeTheCurrentAssetToResolve(
		"Resize the current asset to resolve\n"
		"(Will clear any redo/undo record)"
	);
	tooltip_SetRoutine("Set routine");
	tooltip_Space("(Space)");
	tooltip_SplitEditor("Split editor");
	tooltip_Threaded("Threaded");
	tooltip_Url("URL");
	tooltip_UseByteMatrixViewOnTheToolbar("Use byte matrix view on the toolbar");
	tooltip_ViaClipboard("Via clipboard");
	tooltip_ViaClipboardCsv("Via clipboard (CSV)");
	tooltip_ViaClipboardForTheCurrentFrameOnly(
		"Via clipboard\n"
		"(For the current frame only)"
	);
	tooltip_View("View");
	tooltip_ViewReadme("View README");
	tooltip_Warning("Warning");
	tooltip_WhetherToBuildThisLayer("Whether to build this layer");

	tooltipRecent_Document("Document (F1)");

	tooltipFile_Find("Find (" GBBASIC_MODIFIER_KEY_NAME "+F)");
	tooltipFile_Import("Import (" GBBASIC_MODIFIER_KEY_NAME "+O)");
	tooltipFile_New("New (" GBBASIC_MODIFIER_KEY_NAME "+N)");
	tooltipFile_Open("Open (" GBBASIC_MODIFIER_KEY_NAME "+O)");
	tooltipFile_Save("Save (" GBBASIC_MODIFIER_KEY_NAME "+S)");

	tooltipEdit_Actor("Actor/Projectile (" GBBASIC_MODIFIER_KEY_NAME "+5)");
	tooltipEdit_Audio("Audio (" GBBASIC_MODIFIER_KEY_NAME "+8/9)");
	tooltipEdit_Code("Code (" GBBASIC_MODIFIER_KEY_NAME "+1)");
	tooltipEdit_Console("Console (" GBBASIC_MODIFIER_KEY_NAME "+0)");
	tooltipEdit_Emulator("Emulator (" GBBASIC_MODIFIER_KEY_NAME "+`)");
	tooltipEdit_FontAndI18n("Font/I18n dictionary (" GBBASIC_MODIFIER_KEY_NAME "+6/7)");
#if GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CTRL
	tooltipEdit_Magnifiable("(-/+ or Ctrl+Mouse Wheel)");
#elif GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CMD
	tooltipEdit_Magnifiable("(-/+ or Cmd+Mouse Wheel)");
#endif /* GBBASIC_MODIFIER_KEY */
	tooltipEdit_Map("Map (" GBBASIC_MODIFIER_KEY_NAME "+3)");
	tooltipEdit_NextPage("Next page (" "Ctrl" "+PgDn)");
	tooltipEdit_PreviousPage("Previous page (" "Ctrl" "+PgUp)");
	tooltipEdit_Redo("Redo (" GBBASIC_MODIFIER_KEY_NAME "+Y)");
	tooltipEdit_Scene("Scene (" GBBASIC_MODIFIER_KEY_NAME "+4)");
	tooltipEdit_Tiles("Tiles (" GBBASIC_MODIFIER_KEY_NAME "+2)");
	tooltipEdit_ToolBox("Box (X)");
	tooltipEdit_ToolBoxFill("Box fill (Shift+X)");
	tooltipEdit_ToolEllipse("Ellipse (E)");
	tooltipEdit_ToolEllipseFill("Ellipse fill (Shift+E)");
	tooltipEdit_ToolEraser("Delete (D)");
	tooltipEdit_ToolEyedropper(
		"Pick (I)\n"
		"Alt+LMB"
	);
	tooltipEdit_ToolHand(
		"Move the edit area (H)\n"
		"Shift+LMB or MMB"
	);
	tooltipEdit_ToolJumpToRef("Jump to ref (R)");
	tooltipEdit_ToolLasso("Select (M)");
	tooltipEdit_ToolLassoWithTilewise(
		"Select (M)\n"
		"Hold " GBBASIC_MODIFIER_KEY_NAME " for tilewise"
	);
	tooltipEdit_ToolLine("Line (L)");
	tooltipEdit_ToolMove("Move (P)");
	tooltipEdit_ToolPaintbucket("Fill (G)");
	tooltipEdit_ToolPencil("Draw (B)");
	tooltipEdit_ToolResize("Resize (Z)");
	tooltipEdit_ToolSmudge("Put (U)");
	tooltipEdit_ToolStamp("Stamp (S)");
	tooltipEdit_Undo("Undo (" GBBASIC_MODIFIER_KEY_NAME "+Z)");

	tooltipProject_CompileAndRun("Compile and run (F5)");
	tooltipProject_DownloadProject("Download project");
	tooltipProject_New("New (" GBBASIC_MODIFIER_KEY_NAME "+N)");
	tooltipProject_Resume("Resume (F5)");
	tooltipProject_Stop("Stop (Shift+F5)");

	tooltipActor_AddFrame(
		"Add frame\n"
		"Hold Shift to copy"
	);
	tooltipActor_AnalyzeUnreferencedFrames("Analyze unreferenced frames");
	tooltipActor_AppendFrame(
		"Append frame\n"
		"Hold Shift to copy"
	);
	tooltipActor_Bit0(
		"Bit 0\n"
		"(Palette bit, colored only)"
	);
	tooltipActor_Bit1(
		"Bit 1\n"
		"(Palette bit, colored only)"
	);
	tooltipActor_Bit2(
		"Bit 2\n"
		"(Palette bit, colored only)"
	);
	tooltipActor_Bit3(
		"Bit 3\n"
		"(Bank index, colored only)"
	);
	tooltipActor_Bit4(
		"Bit 4\n"
		"(Palette index, classic only)"
	);
	tooltipActor_Bit5(
		"Bit 5\n"
		"(Horizontal flip, determined automatically)"
	);
	tooltipActor_Bit6(
		"Bit 6\n"
		"(Vertical flip, determined automatically)"
	);
	tooltipActor_Bit7(
		"Bit 7\n"
		"(Priority)"
	);
	tooltipActor_CheckToApplyToAllFrames("Check to apply to all frames");
	tooltipActor_CheckToChangeBitToAllTiles("Check to change bit to all tiles");
	tooltipActor_CopyDefinition("Copy definition");
	tooltipActor_Export("Export");
	tooltipActor_ForAnimationTicking("For animation ticking");
	tooltipActor_ForPlacingInSceneEditor("For placing in scene editor");
	tooltipActor_Info(
		" Tile count: {0}\n"
		" Tile bytes: {1}\n"
		"Max sprites: {2}\n"
		"Actor bytes: {3}"
	);
	tooltipActor_InsertFrame(
		"Insert frame\n"
		"Hold Shift to copy"
	);
	tooltipActor_IntervalInfo(
		"0, 1, 3, 7, 15, 31, 63, 127 are prefered\n"
		"255 for paused"
	);
	tooltipActor_IntervalTooltips().push_back("Paused");
	tooltipActor_IntervalTooltips().push_back("0.47 FPS (Slow)");
	tooltipActor_IntervalTooltips().push_back("0.94 FPS");
	tooltipActor_IntervalTooltips().push_back("1.88 FPS");
	tooltipActor_IntervalTooltips().push_back("3.75 FPS");
	tooltipActor_IntervalTooltips().push_back(" 7.5 FPS");
	tooltipActor_IntervalTooltips().push_back("  15 FPS");
	tooltipActor_IntervalTooltips().push_back("  30 FPS");
	tooltipActor_IntervalTooltips().push_back("  60 FPS (Fast)");
	tooltipActor_PasteDefinition("Paste definition");
	tooltipActor_Refresh("Refresh");
	tooltipActor_StopTesting("Stop testing");
	tooltipActor_TestActor("Test actor");
	tooltipActor_ToAllFrames("To all frames");
	tooltipActor_ToAllTiles("To all tiles");

	tooltipArbitrary_Note(
		"* For parameterized GUI text\n"
		"* Each font asset has its own arbitrary data"
	);

	tooltipAudio_AppendRowOfOrders("Append row of orders");
	tooltipAudio_CopyInstrument("Copy instrument");
	tooltipAudio_CutInstrument("Cut instrument");
	tooltipAudio_DeleteRowOfOrders("Delete row of orders");
	tooltipAudio_InsertRowOfOrders("Insert row of orders");
	tooltipAudio_MoveRowOfOrdersDown("Move row of orders down");
	tooltipAudio_MoveRowOfOrdersUp("Move row of orders up");
	tooltipAudio_Music("Music (" GBBASIC_MODIFIER_KEY_NAME "+8)");
	tooltipAudio_MusicEffects().push_back("Fx: Arpeggio");
	tooltipAudio_MusicEffects().push_back("Fx: Portamento up");
	tooltipAudio_MusicEffects().push_back("Fx: Portamento down");
	tooltipAudio_MusicEffects().push_back("Fx: Tone portamento");
	tooltipAudio_MusicEffects().push_back("Fx: Vibrato");
	tooltipAudio_MusicEffects().push_back("Fx: Set master volume");
	tooltipAudio_MusicEffects().push_back("Fx: Unused" /* "Call routine" */);
	tooltipAudio_MusicEffects().push_back("Fx: Note delay");
	tooltipAudio_MusicEffects().push_back("Fx: Set panning");
	tooltipAudio_MusicEffects().push_back("Fx: Set duty cycle");
	tooltipAudio_MusicEffects().push_back("Fx: Volume slide");
	tooltipAudio_MusicEffects().push_back("Fx: Position jump");
	tooltipAudio_MusicEffects().push_back("Fx: Set volume");
	tooltipAudio_MusicEffects().push_back("Fx: Pattern break");
	tooltipAudio_MusicEffects().push_back("Fx: Note cut");
	tooltipAudio_MusicEffects().push_back("Fx: Set speed");
	for (int i = 0x00; i <= 0xff; ++i) {
		if (i == 0x00)
			tooltipAudio_MusicEffectParameters().push_back("Fx param: --");
		else
			tooltipAudio_MusicEffectParameters().push_back("Fx param: " + Text::toHex(i, 2, '0', true));
	}
	tooltipAudio_MusicInstruments().push_back("Instrument: --");
	tooltipAudio_MusicInstruments().push_back("Instrument: 01");
	tooltipAudio_MusicInstruments().push_back("Instrument: 02");
	tooltipAudio_MusicInstruments().push_back("Instrument: 03");
	tooltipAudio_MusicInstruments().push_back("Instrument: 04");
	tooltipAudio_MusicInstruments().push_back("Instrument: 05");
	tooltipAudio_MusicInstruments().push_back("Instrument: 06");
	tooltipAudio_MusicInstruments().push_back("Instrument: 07");
	tooltipAudio_MusicInstruments().push_back("Instrument: 08");
	tooltipAudio_MusicInstruments().push_back("Instrument: 09");
	tooltipAudio_MusicInstruments().push_back("Instrument: 10");
	tooltipAudio_MusicInstruments().push_back("Instrument: 11");
	tooltipAudio_MusicInstruments().push_back("Instrument: 12");
	tooltipAudio_MusicInstruments().push_back("Instrument: 13");
	tooltipAudio_MusicInstruments().push_back("Instrument: 14");
	tooltipAudio_MusicInstruments().push_back("Instrument: 15");
	tooltipAudio_MusicPlayOnStroke("Play on stroke");
	tooltipAudio_PasteInstrument("Paste instrument");
	tooltipAudio_PlayMusic("Play music");
	tooltipAudio_PlaySfx("Play SFX");
	tooltipAudio_ResetInstrument("Reset instrument");
	tooltipAudio_Sfx("SFX (" GBBASIC_MODIFIER_KEY_NAME "+9)");
	tooltipAudio_SfxChannelDuty1("Duty 1");
	tooltipAudio_SfxChannelDuty2("Duty 2");
	tooltipAudio_SfxChannelNoise("Noise");
	tooltipAudio_SfxChannelWave("Wave");
	tooltipAudio_SfxShowSoundShape("Show sound shape");
	tooltipAudio_StopMusic("Stop music");
	tooltipAudio_StopSfx("Stop SFX");

	tooltipCode_Info(
		"{0} {5} in {1} {6}\n"
		"    Var: {2}\n"
		"  Array: {3}\n"
		"   Loop: {4}"
	);
	tooltipCode_InfoAllocation("allocation");
	tooltipCode_InfoAllocations("allocations");
	tooltipCode_InfoWord("word");
	tooltipCode_InfoWords("words");

	tooltipEmulator_AlternativeSpeed("Alternative speed (" GBBASIC_MODIFIER_KEY_NAME "+/" ")");
	tooltipEmulator_NormalSpeed("Normal speed (" GBBASIC_MODIFIER_KEY_NAME "+/" ")");
	tooltipEmulator_StatusNote(
		"Cartridge flag   : {0}\n"
		"Cartridge ext.   : {1}\n"
		"   Device flag   : {2}\n"
		"Cartridge mapper : {3}\n"
		"Cartridge SRAM   : {4}\n"
		"Cartridge RTC    : {5}"
	);
	tooltipEmulator_StatusNoteRumble(
		"Cartridge rumble : {0}"
	);
	tooltipEmulator_ToggleOnscreenGamepad("Toggle onscreen gamepad");
	tooltipEmulator_VramDebugger_Map(
		" Position: {0}, {1}\n"
		"  Tile No. {2} ({3})\n"
		"Attribute: {4}\n"
		" Map addr: {5}\n"
		"Tile addr: {6}:{7}\n"
		"   H-flip: {8}\n"
		"   V-flip: {9}\n"
		"  Palette: {10}\n"
		" Priority: {11}"
	);
	tooltipEmulator_VramDebugger_Oam(
		" Position: {0}, {1}\n"
		"  Tile No. {2} ({3})\n"
		"Attribute: {4}\n"
		" OAM addr: {5}\n"
		"Tile addr: {6}:{7}\n"
		"   H-flip: {8}\n"
		"   V-flip: {9}\n"
		"  Palette: {10}\n"
		" Priority: {11}"
	);
	tooltipEmulator_VramDebugger_Tile(
		"    Tile No. {0} ({1})\n"
		"  Tile addr: {2}:{3}\n"
		"Guessed plt. {4}\n"
		"  Ref count: {5}"
	);

	tooltipFontAndI18n_I18n("Internationalization\ndictionary (" GBBASIC_MODIFIER_KEY_NAME "+7)");
	tooltipFontAndI18n_Font("Font (" GBBASIC_MODIFIER_KEY_NAME "+6)");
	tooltipFontAndI18n_FontInfo("Supports {0} glyphs");
	tooltipFontAndI18n_FontTrim(
		"Whether to trim glyph height.\n"
		"It is recommended to keep this option checked."
	);

	tooltipI18n_Info(
		"    Items: {0}\n"
		"Languages: {1}"
	);

	tooltipMap_Attributes("Attributes:");
	tooltipMap_Bit0(
		"Bit 0\n"
		"(Palette bit, colored only)"
	);
	tooltipMap_Bit1(
		"Bit 1\n"
		"(Palette bit, colored only)"
	);
	tooltipMap_Bit2(
		"Bit 2\n"
		"(Palette bit, colored only)"
	);
	tooltipMap_Bit3(
		"Bit 3\n"
		"(Bank index, colored only)"
	);
	tooltipMap_Bit4(
		"Bit 4\n"
		"(Not used)"
	);
	tooltipMap_Bit5(
		"Bit 5\n"
		"(Horizontal flip, colored only)"
	);
	tooltipMap_Bit6(
		"Bit 6\n"
		"(Vertical flip, colored only)"
	);
	tooltipMap_Bit7(
		"Bit 7\n"
		"(Priority, colored only)"
	);
	tooltipMap_BitBank    ("      Bank: {0} (colored only)\n");
	tooltipMap_BitHFlip   ("    H-flip: {0} (colored only)\n");
	tooltipMap_BitPriority("  Priority: {0} (colored only)");
	tooltipMap_BitVFlip   ("    V-flip: {0} (colored only)\n");
	tooltipMap_BitsPalette("   Palette: {0} (colored only)\n");
	tooltipMap_CreateANewTilesAssetPage("Create a new tiles asset page");
	tooltipMap_CreateASceneReferencingThisMapPage("Create a scene referencing this map page");
	tooltipMap_EditAsImage(
		"Check to edit map as image with:\n"
		"  * Editing image by pixel by pixel\n"
		"  * Picking colors from local palette\n"
		"  * Transferring to tiled data\n"
		"  * Removing unused tiles\n"
		"Uncheck to edit map normally with:\n"
		"  * Editing by tiles\n"
		"  * Directly accessing tiled data"
	);
	tooltipMap_EditAsImageWithEditingRecordsTips(
		"Exiting this mode will merge and commit the\n"
		"edit-as-image changes into the tile-based data,\n"
		"changes may also remove unused tiles"
	);
	tooltipMap_Info(
		"{0} tiles\n"
		"{1} active {2}"
	);
	tooltipMap_InfoLayer("layer");
	tooltipMap_InfoLayers("layers");
	tooltipMap_Layers("(Ctrl+Shift+1/2)");
	tooltipMap_LayersWithEditingRecordsTips(
		"Switching to other layers will merge and commit the\n"
		"edit-as-image changes into the tile-based data"
	);
	tooltipMap_LocalPalette(
		"Check to enable the local palette for:\n"
		"  * Local palette data for this map asset\n"
		"  * Palette operations for code export\n"
		"  * Edit-time preview using the local palette\n"
		"It doesn't compile into map resource, consider\n"
		"using code export for it"
	);
	tooltipMap_Optimize(
		"It is recommended to keep this option checked to\n"
		"optimize footprint, i.e. by trimming unused tail\n"
		"tiles when filling data for statements like:\n"
		"  `IMAGE(...) = WITH MAP ...`\n"
		"Otherwise all tiles will be filled"
	);
	tooltipMap_StopTesting("Stop testing");
	tooltipMap_TestMap("Test map");
	tooltipMap_UpdateTheRefTilesAssetPage("Update the ref tiles asset page");

	tooltipPalette_Index0(
		"(1)\n"
		"Ignored by sprites"
	);
	tooltipPalette_Index1("(2)");
	tooltipPalette_Index2("(3)");
	tooltipPalette_Index3("(4)");
	tooltipPalette_Note(
		"* Works with colored devices only\n"
		"* Will be compiled into final ROM\n"
		"* Copy and paste code to change programmingly"
	);

	tooltipProjectProperty_ProjectIcon2BppDetails("32x32px, 2bpp for ROM");
	tooltipProjectProperty_ProjectIconDetails("32x32px for application");
	tooltipProjectProperty_ProjectProperty("Project property");
	tooltipProjectProperty_ReplaceBorder("Replace border");
	tooltipProjectProperty_ReplaceIcon("Replace icon");
	tooltipProjectProperty_ResetBorder("Reset border");
	tooltipProjectProperty_ResetPalettesToUseTheSharedAsset("Reset palettes to use the shared asset");
	tooltipProjectProperty_ResetIcon("Reset icon");
	tooltipProjectProperty_SpecifyPreDefinedMacros(
		"Specify pre-defined macros,\n"
		"macroes are separated by \",\",\n"
		"string values are quoted with \"'\"\n"
		"(I.e. AAA=1,BBB='Ok')"
	);
	tooltipProjectProperty_WhetherToCompileInStrictModeTreatsSomeWarningsAsErrors(
		"Whether to compile in strict mode\n"
		"(Treats some warnings as errors)"
	);
	tooltipProjectProperty_WhetherToCompileWithNecessaryOptimizationsRecommendedToKeepItChecked(
		"Whether to compile with necessary\n"
		"optimizations\n"
		"(Recommended to keep it checked)"
	);

	tooltipScene_ActorDetails(
		"  Index: {0}\n"
		"    Pos: {1},{2}\n"
		" Bounds: {3},{4},{5},{6}\n"
		" Update: {7}\n"
		"On hits: {8}"
	);
	tooltipScene_Attributes("Attributes:");
	tooltipScene_Bit0(
		"Bit 0\n"
		"(Blocking left)"
	);
	tooltipScene_Bit1(
		"Bit 1\n"
		"(Blocking right)"
	);
	tooltipScene_Bit2(
		"Bit 2\n"
		"(Blocking up)"
	);
	tooltipScene_Bit3(
		"Bit 3\n"
		"(Blocking down)"
	);
	tooltipScene_Bit4(
		"Bit 4\n"
		"(Is ladder)"
	);
	tooltipScene_Bit5(
		"Bit 5\n"
		"(Not used)"
	);
	tooltipScene_Bit6(
		"Bit 6\n"
		"(Not used)"
	);
	tooltipScene_Bit7(
		"Bit 7\n"
		"(Not used)"
	);
	tooltipScene_BitBlockingDown ("   Blocking down: {0}\n");
	tooltipScene_BitBlockingLeft ("   Blocking left: {0}\n");
	tooltipScene_BitBlockingRight("  Blocking right: {0}\n");
	tooltipScene_BitBlockingUp   ("     Blocking up: {0}\n");
	tooltipScene_BitIsLadder     ("       Is ladder: {0}");
	tooltipScene_CopyDefinition("Copy definition");
	tooltipScene_Info(
		"{0} tiles\n"
		"{1} active {2}"
	);
	tooltipScene_InfoLayer("layer");
	tooltipScene_InfoLayers("layers");
	tooltipScene_PasteDefinition("Paste definition");
	tooltipScene_Properties("Properties:");
	tooltipScene_StopTesting("Stop testing");
	tooltipScene_TestScene("Test scene");
	tooltipScene_TriggerDetails(
		"Index: {0}\n"
		"  Pos: {1},{2}\n"
		" Size: {3}x{4}"
	);

	tooltipTiles_AnalyzeUnreferencedOrDuplicateTiles("Analyze unreferenced or duplicate tiles");
	tooltipTiles_CreateAMapReferencingThisTilesPage("Create a map referencing this tiles page");
	tooltipTiles_Info("{0} tiles");

	warning_ActorActorNameIsEmptyAtPage("Actor name is empty at page {0}");
	warning_ActorAnchorOutOfBounds("Anchor out of bounds");
	warning_ActorDuplicateActorNameAtPages("Duplicate actor name \"{0}\" at page {1} and {2}");
	warning_ActorFrameIndexOutOfBoundsOfAnimationN("Frame index out of bounds of animation \"{0}\"");
	warning_ActorNameIsAlreadyInUse("Name is already in use");
	warning_FontDuplicateFontNameAtPages("Duplicate font name \"{0}\" at page {1} and {2}");
	warning_FontFontNameIsEmptyAtPage("Font name is empty at page {0}");
	warning_FontMaxSizeOutOfBounds("Max size out of bounds");
	warning_FontNameIsAlreadyInUse("Name is already in use");
	warning_FontOutlineOutOfBounds("Outline out of bounds");
	warning_FontPreviewContentIsTooLong("Preview content is too long");
	warning_FontShadowOutOfBounds("Shadow out of bounds");
	warning_FontSizeOutOfBounds("Size out of bounds");
	warning_I18nColumnCountOutOfBounds("Column count out of bounds");
	warning_I18nDuplicateI18nNameAtPages("Duplicate i18n name \"{0}\" at page {1} and {2}");
	warning_I18nDuplicateKeyNames("Duplicate key names \"{0}\"");
	warning_I18nDuplicateKeyNames_Detail("row {0}");
	warning_I18nDuplicateLanguages("Duplicate languages \"{0}\"");
	warning_I18nDuplicateLanguages_Detail("column {0}");
	warning_I18nI18nNameIsEmptyAtPage("I18n name is empty at page {0}");
	warning_I18nMissingKeyColumn("Missing \"key\" column");
	warning_I18nRowCountOutOfBounds("Row count out of bounds");
	warning_I18nTheKeyColumnWasNotAtTheBeginning("The \"key\" column was not at the beginning");
	warning_I18nTooFewColumns("Too few columns");
	warning_MapDuplicateMapNameAtPages("Duplicate map name \"{0}\" at page {1} and {2}");
	warning_MapMapNameIsEmptyAtPage("Map name is empty at page {0}");
	warning_MapNameIsAlreadyInUse("Name is already in use");
	warning_MapSizeOutOfBounds("Size out of bounds");
	warning_MusicDuplicateMusicInstrumentNameAt("Duplicate music instrument name \"{0}\" at {1} and {2}");
	warning_MusicDuplicateMusicNameAtPages("Duplicate music name \"{0}\" at page {1} and {2}");
	warning_MusicMusicInstrumentNameIsEmptyAt("Music instrument name is empty at {0}");
	warning_MusicMusicNameIsEmptyAtPage("Music name is empty at page {0}");
	warning_MusicNameIsAlreadyInUse("Name is already in use");
	warning_RoutineExplicitLineNumberIsNotSupported("Explicit line number is not supported");
	warning_SceneActorCountOutOfBounds("Actor out of bounds");
	warning_SceneCameraPositionOutOfBounds("Camera position out of bounds");
	warning_SceneDuplicateSceneNameAtPages("Duplicate scene name \"{0}\" at page {1} and {2}");
	warning_SceneInvalidActorBehaviour("Invalid actor behaviour");
	warning_SceneMultiplePlayerActors("Multiple player actors");
	warning_SceneNameIsAlreadyInUse("Name is already in use");
	warning_SceneSceneNameIsEmptyAtPage("Scene name is empty at page {0}");
	warning_SceneTriggerCountOutOfBounds("Trigger out of bounds");
	warning_SfxDuplicateSfxNameAtPages("Duplicate SFX name \"{0}\" at page {1} and {2}");
	warning_SfxNameIsAlreadyInUse("Name is already in use");
	warning_SfxSfxNameIsEmptyAtPage("SFX name is empty at page {0}");
	warning_SfxTimeoutWhenRenderingSoundShapeAtPage("Timeout when rendering sound shape at page {0}");
	warning_TileSizeOutOfBounds("Size out of bounds");
	warning_TilesDuplicateTilesNameAtPages("Duplicate tiles name \"{0}\" at page {1} and {2}");
	warning_TilesNameIsAlreadyInUse("Name is already in use");
	warning_TilesPitchOutOfBounds("Pitch out of bounds");
	warning_TilesSizeOutOfBounds("Size out of bounds");
	warning_TilesTilesNameIsEmptyAtPage("Tiles name is empty at page {0}");

	iconAnchor(createTexture(rnd, RES_ICON_ANCHOR, GBBASIC_COUNTOF(RES_ICON_ANCHOR), nullptr));

	iconSeparator(createTexture(rnd, RES_ICON_SEPARATOR, GBBASIC_COUNTOF(RES_ICON_SEPARATOR), nullptr));

	iconCursor(createTexture(rnd, RES_ICON_CURSOR, GBBASIC_COUNTOF(RES_ICON_CURSOR), &imageCursor()));

	iconMainMenu(createTexture(rnd, RES_ICON_MAIN_MENU, GBBASIC_COUNTOF(RES_ICON_MAIN_MENU), nullptr));
	iconCode(createTexture(rnd, RES_ICON_CODE, GBBASIC_COUNTOF(RES_ICON_CODE), nullptr));
	iconTiles(createTexture(rnd, RES_ICON_TILES, GBBASIC_COUNTOF(RES_ICON_TILES), nullptr));
	iconMap(createTexture(rnd, RES_ICON_MAP, GBBASIC_COUNTOF(RES_ICON_MAP), nullptr));
	iconFont(createTexture(rnd, RES_ICON_FONT, GBBASIC_COUNTOF(RES_ICON_FONT), nullptr));
	iconFontMore(createTexture(rnd, RES_ICON_FONT_MORE, GBBASIC_COUNTOF(RES_ICON_FONT_MORE), nullptr));
	iconI18n(createTexture(rnd, RES_ICON_I18N, GBBASIC_COUNTOF(RES_ICON_I18N), nullptr));
	iconAudio(createTexture(rnd, RES_ICON_AUDIO, GBBASIC_COUNTOF(RES_ICON_AUDIO), nullptr));
	iconAudioMore(createTexture(rnd, RES_ICON_AUDIO_MORE, GBBASIC_COUNTOF(RES_ICON_AUDIO_MORE), nullptr));
	iconMusic(createTexture(rnd, RES_ICON_MUSIC, GBBASIC_COUNTOF(RES_ICON_MUSIC), nullptr));
	iconSfx(createTexture(rnd, RES_ICON_SFX, GBBASIC_COUNTOF(RES_ICON_SFX), nullptr));
	iconActor(createTexture(rnd, RES_ICON_ACTOR, GBBASIC_COUNTOF(RES_ICON_ACTOR), nullptr));
	iconScene(createTexture(rnd, RES_ICON_SCENE, GBBASIC_COUNTOF(RES_ICON_SCENE), nullptr));
	iconConsole(createTexture(rnd, RES_ICON_CONSOLE, GBBASIC_COUNTOF(RES_ICON_CONSOLE), nullptr));
	iconEmulator(createTexture(rnd, RES_ICON_EMULATOR, GBBASIC_COUNTOF(RES_ICON_EMULATOR), nullptr));
	iconDocument(createTexture(rnd, RES_ICON_DOCUMENT, GBBASIC_COUNTOF(RES_ICON_DOCUMENT), nullptr));
	iconIconView(createTexture(rnd, RES_ICON_ICON_VIEW, GBBASIC_COUNTOF(RES_ICON_ICON_VIEW), nullptr));
	iconListView(createTexture(rnd, RES_ICON_LIST_VIEW, GBBASIC_COUNTOF(RES_ICON_LIST_VIEW), nullptr));
	iconKernels(createTexture(rnd, RES_ICON_KERNELS, GBBASIC_COUNTOF(RES_ICON_KERNELS), nullptr));
	iconSave(createTexture(rnd, RES_ICON_SAVE, GBBASIC_COUNTOF(RES_ICON_SAVE), nullptr));
	iconWorking(createTexture(rnd, RES_ICON_WORKING, GBBASIC_COUNTOF(RES_ICON_WORKING), nullptr));
	iconInfo(createTexture(rnd, RES_ICON_INFO, GBBASIC_COUNTOF(RES_ICON_INFO), nullptr));
	iconGamepad(createTexture(rnd, RES_ICON_GAMEPAD, GBBASIC_COUNTOF(RES_ICON_GAMEPAD), nullptr));
	iconProperty(createTexture(rnd, RES_ICON_PROPERTY, GBBASIC_COUNTOF(RES_ICON_PROPERTY), nullptr));
	iconBrowse(createTexture(rnd, RES_ICON_BROWSE, GBBASIC_COUNTOF(RES_ICON_BROWSE), nullptr));
	iconLoud(createTexture(rnd, RES_ICON_LOUD, GBBASIC_COUNTOF(RES_ICON_LOUD), nullptr));
	iconMuted(createTexture(rnd, RES_ICON_MUTED, GBBASIC_COUNTOF(RES_ICON_MUTED), nullptr));
	iconFastForward(createTexture(rnd, RES_ICON_FAST_FORWARD, GBBASIC_COUNTOF(RES_ICON_FAST_FORWARD), nullptr));
	iconNormalSpeed(createTexture(rnd, RES_ICON_NORMAL_SPEED, GBBASIC_COUNTOF(RES_ICON_NORMAL_SPEED), nullptr));
	iconWarning(createTexture(rnd, RES_ICON_WARNING, GBBASIC_COUNTOF(RES_ICON_WARNING), nullptr));
	iconWaiting(createTexture(rnd, RES_ICON_WAITING, GBBASIC_COUNTOF(RES_ICON_WAITING), nullptr));

	iconBack(createTexture(rnd, RES_ICON_BACK, GBBASIC_COUNTOF(RES_ICON_BACK), nullptr));
	iconOpen(createTexture(rnd, RES_ICON_OPEN, GBBASIC_COUNTOF(RES_ICON_OPEN), nullptr));
	iconRefresh(createTexture(rnd, RES_ICON_REFRESH, GBBASIC_COUNTOF(RES_ICON_REFRESH), nullptr));
	iconRecycle(createTexture(rnd, RES_ICON_RECYCLE, GBBASIC_COUNTOF(RES_ICON_RECYCLE), nullptr));
	iconClear(createTexture(rnd, RES_ICON_CLEAR, GBBASIC_COUNTOF(RES_ICON_CLEAR), nullptr));
	iconReset(createTexture(rnd, RES_ICON_RESET, GBBASIC_COUNTOF(RES_ICON_RESET), nullptr));
	iconCopy(createTexture(rnd, RES_ICON_COPY, GBBASIC_COUNTOF(RES_ICON_COPY), nullptr));
	iconRedo(createTexture(rnd, RES_ICON_REDO, GBBASIC_COUNTOF(RES_ICON_REDO), nullptr));
	iconUndo(createTexture(rnd, RES_ICON_UNDO, GBBASIC_COUNTOF(RES_ICON_UNDO), nullptr));
	iconImport(createTexture(rnd, RES_ICON_IMPORT, GBBASIC_COUNTOF(RES_ICON_IMPORT), nullptr));
	iconExport(createTexture(rnd, RES_ICON_EXPORT, GBBASIC_COUNTOF(RES_ICON_EXPORT), nullptr));
	iconExternal(createTexture(rnd, RES_ICON_EXTERNAL, GBBASIC_COUNTOF(RES_ICON_EXTERNAL), nullptr));
	iconDownload(createTexture(rnd, RES_ICON_DOWNLOAD, GBBASIC_COUNTOF(RES_ICON_DOWNLOAD), nullptr));
	iconViews(createTexture(rnd, RES_ICON_VIEWS, GBBASIC_COUNTOF(RES_ICON_VIEWS), nullptr));

	iconMinimize(createTexture(rnd, RES_ICON_MINIMIZE, GBBASIC_COUNTOF(RES_ICON_MINIMIZE), nullptr));
	iconMaximize(createTexture(rnd, RES_ICON_MAXIMIZE, GBBASIC_COUNTOF(RES_ICON_MAXIMIZE), nullptr));
	iconRestore(createTexture(rnd, RES_ICON_RESTORE, GBBASIC_COUNTOF(RES_ICON_RESTORE), nullptr));
	iconClose(createTexture(rnd, RES_ICON_CLOSE, GBBASIC_COUNTOF(RES_ICON_CLOSE), nullptr));

	iconFind(createTexture(rnd, RES_ICON_FIND, GBBASIC_COUNTOF(RES_ICON_FIND), nullptr));
	iconSort(createTexture(rnd, RES_ICON_SORT, GBBASIC_COUNTOF(RES_ICON_SORT), nullptr));

	iconUp(createTexture(rnd, RES_ICON_UP, GBBASIC_COUNTOF(RES_ICON_UP), nullptr));
	iconDown(createTexture(rnd, RES_ICON_DOWN, GBBASIC_COUNTOF(RES_ICON_DOWN), nullptr));
	iconLeft(createTexture(rnd, RES_ICON_LEFT, GBBASIC_COUNTOF(RES_ICON_LEFT), nullptr));
	iconRight(createTexture(rnd, RES_ICON_RIGHT, GBBASIC_COUNTOF(RES_ICON_RIGHT), nullptr));

	iconCaseSensitive(createTexture(rnd, RES_ICON_CASE_SENSITIVE, GBBASIC_COUNTOF(RES_ICON_CASE_SENSITIVE), nullptr));
	iconWholeWord(createTexture(rnd, RES_ICON_WHOLE_WORD, GBBASIC_COUNTOF(RES_ICON_WHOLE_WORD), nullptr));
	iconGlobal(createTexture(rnd, RES_ICON_GLOBAL, GBBASIC_COUNTOF(RES_ICON_GLOBAL), nullptr));

	iconPlus(createTexture(rnd, RES_ICON_PLUS, GBBASIC_COUNTOF(RES_ICON_PLUS), nullptr));
	iconMinus(createTexture(rnd, RES_ICON_MINUS, GBBASIC_COUNTOF(RES_ICON_MINUS), nullptr));
	iconInsert(createTexture(rnd, RES_ICON_INSERT, GBBASIC_COUNTOF(RES_ICON_INSERT), nullptr));
	iconAppend(createTexture(rnd, RES_ICON_APPEND, GBBASIC_COUNTOF(RES_ICON_APPEND), nullptr));

	iconPlay(createTexture(rnd, RES_ICON_PLAY, GBBASIC_COUNTOF(RES_ICON_PLAY), nullptr));
	iconStartPreview(createTexture(rnd, RES_ICON_PLAY_AUDIO, GBBASIC_COUNTOF(RES_ICON_PLAY_AUDIO), nullptr));
	iconStop(createTexture(rnd, RES_ICON_STOP, GBBASIC_COUNTOF(RES_ICON_STOP), nullptr));
	iconStopPreview(createTexture(rnd, RES_ICON_STOP_AUDIO, GBBASIC_COUNTOF(RES_ICON_STOP_AUDIO), nullptr));
	iconPause(createTexture(rnd, RES_ICON_PAUSE, GBBASIC_COUNTOF(RES_ICON_PAUSE), nullptr));

	iconNumber0(createTexture(rnd, RES_ICON_NUMBER_0, GBBASIC_COUNTOF(RES_ICON_NUMBER_0), nullptr));
	iconNumber1(createTexture(rnd, RES_ICON_NUMBER_1, GBBASIC_COUNTOF(RES_ICON_NUMBER_1), nullptr));
	iconNumber2(createTexture(rnd, RES_ICON_NUMBER_2, GBBASIC_COUNTOF(RES_ICON_NUMBER_2), nullptr));
	iconNumber3(createTexture(rnd, RES_ICON_NUMBER_3, GBBASIC_COUNTOF(RES_ICON_NUMBER_3), nullptr));
	iconNumber4(createTexture(rnd, RES_ICON_NUMBER_4, GBBASIC_COUNTOF(RES_ICON_NUMBER_4), nullptr));

	iconDual(createTexture(rnd, RES_ICON_DUAL, GBBASIC_COUNTOF(RES_ICON_DUAL), nullptr));
	iconMajor(createTexture(rnd, RES_ICON_MAJOR, GBBASIC_COUNTOF(RES_ICON_MAJOR), nullptr));
	iconMinor(createTexture(rnd, RES_ICON_MINOR, GBBASIC_COUNTOF(RES_ICON_MINOR), nullptr));

	iconTableDropdown(createTexture(rnd, RES_ICON_DROPDOWN, GBBASIC_COUNTOF(RES_ICON_DROPDOWN), nullptr));

	iconBackground(createTexture(rnd, RES_ICON_BACKGROUND, GBBASIC_COUNTOF(RES_ICON_BACKGROUND), nullptr));

	iconTransparent(createTexture(rnd, RES_ICON_TRANSPARENT, GBBASIC_COUNTOF(RES_ICON_TRANSPARENT), nullptr));
	iconUnknown(createTexture(rnd, RES_ICON_UNKNOWN, GBBASIC_COUNTOF(RES_ICON_UNKNOWN), nullptr));

	iconHand(createTexture(rnd, RES_ICON_HAND, GBBASIC_COUNTOF(RES_ICON_HAND), nullptr));
	iconEyedropper(createTexture(rnd, RES_ICON_EYEDROPPER, GBBASIC_COUNTOF(RES_ICON_EYEDROPPER), nullptr));
	iconPencil(createTexture(rnd, RES_ICON_PENCIL, GBBASIC_COUNTOF(RES_ICON_PENCIL), nullptr));
	iconPaintbucket(createTexture(rnd, RES_ICON_PAINTBUCKET, GBBASIC_COUNTOF(RES_ICON_PAINTBUCKET), nullptr));
	iconLasso(createTexture(rnd, RES_ICON_LASSO, GBBASIC_COUNTOF(RES_ICON_LASSO), nullptr));
	iconLine(createTexture(rnd, RES_ICON_LINE, GBBASIC_COUNTOF(RES_ICON_LINE), nullptr));
	iconBox(createTexture(rnd, RES_ICON_BOX, GBBASIC_COUNTOF(RES_ICON_BOX), nullptr));
	iconBoxFill(createTexture(rnd, RES_ICON_BOX_FILL, GBBASIC_COUNTOF(RES_ICON_BOX_FILL), nullptr));
	iconEllipse(createTexture(rnd, RES_ICON_ELLIPSE, GBBASIC_COUNTOF(RES_ICON_ELLIPSE), nullptr));
	iconEllipseFill(createTexture(rnd, RES_ICON_ELLIPSE_FILL, GBBASIC_COUNTOF(RES_ICON_ELLIPSE_FILL), nullptr));
	iconStamp(createTexture(rnd, RES_ICON_STAMP, GBBASIC_COUNTOF(RES_ICON_STAMP), nullptr));

	iconSmudge(createTexture(rnd, RES_ICON_SMUDGE, GBBASIC_COUNTOF(RES_ICON_SMUDGE), nullptr));
	iconEraser(createTexture(rnd, RES_ICON_ERASER, GBBASIC_COUNTOF(RES_ICON_ERASER), nullptr));
	iconMove(createTexture(rnd, RES_ICON_MOVE, GBBASIC_COUNTOF(RES_ICON_MOVE), nullptr));
	iconResize(createTexture(rnd, RES_ICON_RESIZE, GBBASIC_COUNTOF(RES_ICON_RESIZE), nullptr));

	iconRef(createTexture(rnd, RES_ICON_REF, GBBASIC_COUNTOF(RES_ICON_REF), nullptr));

	iconBitwise(createTexture(rnd, RES_ICON_BITWISE, GBBASIC_COUNTOF(RES_ICON_BITWISE), nullptr));
	iconBitwiseSet(createTexture(rnd, RES_ICON_BITWISE_SET, GBBASIC_COUNTOF(RES_ICON_BITWISE_SET), nullptr));
	iconBitwiseAnd(createTexture(rnd, RES_ICON_BITWISE_AND, GBBASIC_COUNTOF(RES_ICON_BITWISE_AND), nullptr));
	iconBitwiseOr(createTexture(rnd, RES_ICON_BITWISE_OR, GBBASIC_COUNTOF(RES_ICON_BITWISE_OR), nullptr));
	iconBitwiseXor(createTexture(rnd, RES_ICON_BITWISE_XOR, GBBASIC_COUNTOF(RES_ICON_BITWISE_XOR), nullptr));

	iconMagnify(createTexture(rnd, RES_ICON_MAGNIFY, GBBASIC_COUNTOF(RES_ICON_MAGNIFY), nullptr));

	iconChannels(createTexture(rnd, RES_ICON_CHANNELS, GBBASIC_COUNTOF(RES_ICON_CHANNELS), nullptr));

	iconPencils(createTexture(rnd, RES_ICON_PENCILS, GBBASIC_COUNTOF(RES_ICON_PENCILS), nullptr));

	iconClock(createTexture(rnd, RES_ICON_CLOCK, GBBASIC_COUNTOF(RES_ICON_CLOCK), nullptr));

	iconRotateClockwise(createTexture(rnd, RES_ICON_ROTATE_CLOCKWISE, GBBASIC_COUNTOF(RES_ICON_ROTATE_CLOCKWISE), nullptr));
	iconRotateAnticlockwise(createTexture(rnd, RES_ICON_ROTATE_ANTICLOCKWISE, GBBASIC_COUNTOF(RES_ICON_ROTATE_ANTICLOCKWISE), nullptr));
	iconRotateHalfTurn(createTexture(rnd, RES_ICON_ROTATE_HALF_TURN, GBBASIC_COUNTOF(RES_ICON_ROTATE_HALF_TURN), nullptr));
	iconFlipVertically(createTexture(rnd, RES_ICON_FLIP_VERTICALLY, GBBASIC_COUNTOF(RES_ICON_FLIP_VERTICALLY), nullptr));
	iconFlipHorizontally(createTexture(rnd, RES_ICON_FLIP_HORIZONTALLY, GBBASIC_COUNTOF(RES_ICON_FLIP_HORIZONTALLY), nullptr));

	iconMinus_24x24(createTexture(rnd, RES_ICON_MINUS_24x24, GBBASIC_COUNTOF(RES_ICON_MINUS_24x24), nullptr));
	iconInsert_24x24(createTexture(rnd, RES_ICON_INSERT_24x24, GBBASIC_COUNTOF(RES_ICON_INSERT_24x24), nullptr));
	iconAppend_24x24(createTexture(rnd, RES_ICON_APPEND_24x24, GBBASIC_COUNTOF(RES_ICON_APPEND_24x24), nullptr));

	iconUp_24x24(createTexture(rnd, RES_ICON_UP_24x24, GBBASIC_COUNTOF(RES_ICON_UP_24x24), nullptr));
	iconDown_24x24(createTexture(rnd, RES_ICON_DOWN_24x24, GBBASIC_COUNTOF(RES_ICON_DOWN_24x24), nullptr));
	iconLeft_24x24(createTexture(rnd, RES_ICON_LEFT_24x24, GBBASIC_COUNTOF(RES_ICON_LEFT_24x24), nullptr));
	iconRight_24x24(createTexture(rnd, RES_ICON_RIGHT_24x24, GBBASIC_COUNTOF(RES_ICON_RIGHT_24x24), nullptr));

	iconExample(createTexture(rnd, RES_ICON_EXAMPLE, GBBASIC_COUNTOF(RES_ICON_EXAMPLE), nullptr));
	iconPlainCode(createTexture(rnd, RES_ICON_PLAIN_CODE, GBBASIC_COUNTOF(RES_ICON_PLAIN_CODE), nullptr));
	iconProjectOmitted(createTexture(rnd, RES_ICON_PROJECT_OMITTED, GBBASIC_COUNTOF(RES_ICON_PROJECT_OMITTED), nullptr));
	iconRom(createTexture(rnd, RES_ICON_ROM, GBBASIC_COUNTOF(RES_ICON_ROM), nullptr));

	createImage(rnd, RES_ICON_APP, GBBASIC_COUNTOF(RES_ICON_APP), &imageApp());

	textureDefaultBorder(createTexture(rnd, RES_IMAGE_DEFAULT_BORDER, GBBASIC_COUNTOF(RES_IMAGE_DEFAULT_BORDER), nullptr));

	textureWaitingForSoundShape(createTexture(rnd, RES_IMAGE_WAITING_FOR_SOUND_SHAPE, GBBASIC_COUNTOF(RES_IMAGE_WAITING_FOR_SOUND_SHAPE), nullptr));

	textureByte(createTexture(rnd, RES_IMAGE_BYTE, GBBASIC_COUNTOF(RES_IMAGE_BYTE), &imageByte()));
	textureActors(createTexture(rnd, RES_IMAGE_ACTORS, GBBASIC_COUNTOF(RES_IMAGE_ACTORS), &imageActors()));

	memcpy(ImGui::GetStyle().Colors, style()->builtin, sizeof(ImGui::GetStyle().Colors));

	return true;
}

bool Theme::close(class Renderer* rnd) {
	if (!_opened)
		return false;
	_opened = false;

	style(nullptr);

	destroyTexture(rnd, iconAnchor(), nullptr);

	destroyTexture(rnd, iconSeparator(), nullptr);

	destroyTexture(rnd, iconCursor(), &imageCursor());

	destroyTexture(rnd, iconMainMenu(), nullptr);
	destroyTexture(rnd, iconCode(), nullptr);
	destroyTexture(rnd, iconTiles(), nullptr);
	destroyTexture(rnd, iconMap(), nullptr);
	destroyTexture(rnd, iconFont(), nullptr);
	destroyTexture(rnd, iconFontMore(), nullptr);
	destroyTexture(rnd, iconI18n(), nullptr);
	destroyTexture(rnd, iconAudio(), nullptr);
	destroyTexture(rnd, iconAudioMore(), nullptr);
	destroyTexture(rnd, iconMusic(), nullptr);
	destroyTexture(rnd, iconSfx(), nullptr);
	destroyTexture(rnd, iconActor(), nullptr);
	destroyTexture(rnd, iconScene(), nullptr);
	destroyTexture(rnd, iconConsole(), nullptr);
	destroyTexture(rnd, iconEmulator(), nullptr);
	destroyTexture(rnd, iconDocument(), nullptr);
	destroyTexture(rnd, iconIconView(), nullptr);
	destroyTexture(rnd, iconListView(), nullptr);
	destroyTexture(rnd, iconKernels(), nullptr);
	destroyTexture(rnd, iconWorking(), nullptr);
	destroyTexture(rnd, iconInfo(), nullptr);
	destroyTexture(rnd, iconSave(), nullptr);
	destroyTexture(rnd, iconGamepad(), nullptr);
	destroyTexture(rnd, iconProperty(), nullptr);
	destroyTexture(rnd, iconBrowse(), nullptr);
	destroyTexture(rnd, iconLoud(), nullptr);
	destroyTexture(rnd, iconMuted(), nullptr);
	destroyTexture(rnd, iconFastForward(), nullptr);
	destroyTexture(rnd, iconNormalSpeed(), nullptr);
	destroyTexture(rnd, iconWarning(), nullptr);
	destroyTexture(rnd, iconWaiting(), nullptr);

	destroyTexture(rnd, iconBack(), nullptr);
	destroyTexture(rnd, iconOpen(), nullptr);
	destroyTexture(rnd, iconRefresh(), nullptr);
	destroyTexture(rnd, iconRecycle(), nullptr);
	destroyTexture(rnd, iconClear(), nullptr);
	destroyTexture(rnd, iconReset(), nullptr);
	destroyTexture(rnd, iconCopy(), nullptr);
	destroyTexture(rnd, iconRedo(), nullptr);
	destroyTexture(rnd, iconUndo(), nullptr);
	destroyTexture(rnd, iconImport(), nullptr);
	destroyTexture(rnd, iconExport(), nullptr);
	destroyTexture(rnd, iconExternal(), nullptr);
	destroyTexture(rnd, iconDownload(), nullptr);
	destroyTexture(rnd, iconViews(), nullptr);

	destroyTexture(rnd, iconMinimize(), nullptr);
	destroyTexture(rnd, iconMaximize(), nullptr);
	destroyTexture(rnd, iconRestore(), nullptr);
	destroyTexture(rnd, iconClose(), nullptr);

	destroyTexture(rnd, iconFind(), nullptr);
	destroyTexture(rnd, iconSort(), nullptr);

	destroyTexture(rnd, iconUp(), nullptr);
	destroyTexture(rnd, iconDown(), nullptr);
	destroyTexture(rnd, iconLeft(), nullptr);
	destroyTexture(rnd, iconRight(), nullptr);

	destroyTexture(rnd, iconCaseSensitive(), nullptr);
	destroyTexture(rnd, iconWholeWord(), nullptr);
	destroyTexture(rnd, iconGlobal(), nullptr);

	destroyTexture(rnd, iconPlus(), nullptr);
	destroyTexture(rnd, iconMinus(), nullptr);
	destroyTexture(rnd, iconInsert(), nullptr);
	destroyTexture(rnd, iconAppend(), nullptr);

	destroyTexture(rnd, iconPlay(), nullptr);
	destroyTexture(rnd, iconStartPreview(), nullptr);
	destroyTexture(rnd, iconStop(), nullptr);
	destroyTexture(rnd, iconStopPreview(), nullptr);
	destroyTexture(rnd, iconPause(), nullptr);

	destroyTexture(rnd, iconNumber0(), nullptr);
	destroyTexture(rnd, iconNumber1(), nullptr);
	destroyTexture(rnd, iconNumber2(), nullptr);
	destroyTexture(rnd, iconNumber3(), nullptr);
	destroyTexture(rnd, iconNumber4(), nullptr);

	destroyTexture(rnd, iconDual(), nullptr);
	destroyTexture(rnd, iconMajor(), nullptr);
	destroyTexture(rnd, iconMinor(), nullptr);

	destroyTexture(rnd, iconTableDropdown(), nullptr);

	destroyTexture(rnd, iconBackground(), nullptr);

	destroyTexture(rnd, iconTransparent(), nullptr);
	destroyTexture(rnd, iconUnknown(), nullptr);

	destroyTexture(rnd, iconHand(), nullptr);
	destroyTexture(rnd, iconEyedropper(), nullptr);
	destroyTexture(rnd, iconPencil(), nullptr);
	destroyTexture(rnd, iconPaintbucket(), nullptr);
	destroyTexture(rnd, iconLasso(), nullptr);
	destroyTexture(rnd, iconLine(), nullptr);
	destroyTexture(rnd, iconBox(), nullptr);
	destroyTexture(rnd, iconBoxFill(), nullptr);
	destroyTexture(rnd, iconEllipse(), nullptr);
	destroyTexture(rnd, iconEllipseFill(), nullptr);
	destroyTexture(rnd, iconStamp(), nullptr);

	destroyTexture(rnd, iconSmudge(), nullptr);
	destroyTexture(rnd, iconEraser(), nullptr);
	destroyTexture(rnd, iconMove(), nullptr);
	destroyTexture(rnd, iconResize(), nullptr);

	destroyTexture(rnd, iconRef(), nullptr);

	destroyTexture(rnd, iconBitwise(), nullptr);
	destroyTexture(rnd, iconBitwiseSet(), nullptr);
	destroyTexture(rnd, iconBitwiseAnd(), nullptr);
	destroyTexture(rnd, iconBitwiseOr(), nullptr);
	destroyTexture(rnd, iconBitwiseXor(), nullptr);

	destroyTexture(rnd, iconMagnify(), nullptr);

	destroyTexture(rnd, iconChannels(), nullptr);

	destroyTexture(rnd, iconPencils(), nullptr);

	destroyTexture(rnd, iconClock(), nullptr);

	destroyTexture(rnd, iconRotateClockwise(), nullptr);
	destroyTexture(rnd, iconRotateAnticlockwise(), nullptr);
	destroyTexture(rnd, iconRotateHalfTurn(), nullptr);
	destroyTexture(rnd, iconFlipVertically(), nullptr);
	destroyTexture(rnd, iconFlipHorizontally(), nullptr);

	destroyTexture(rnd, iconMinus_24x24(), nullptr);
	destroyTexture(rnd, iconInsert_24x24(), nullptr);
	destroyTexture(rnd, iconAppend_24x24(), nullptr);

	destroyTexture(rnd, iconUp_24x24(), nullptr);
	destroyTexture(rnd, iconDown_24x24(), nullptr);
	destroyTexture(rnd, iconLeft_24x24(), nullptr);
	destroyTexture(rnd, iconRight_24x24(), nullptr);

	destroyTexture(rnd, iconExample(), nullptr);
	destroyTexture(rnd, iconProjectOmitted(), nullptr);
	destroyTexture(rnd, iconPlainCode(), nullptr);
	destroyTexture(rnd, iconRom(), nullptr);

	destroyImage(rnd, &imageApp());

	destroyTexture(rnd, textureDefaultBorder(), nullptr);

	destroyTexture(rnd, textureWaitingForSoundShape(), nullptr);

	destroyTexture(rnd, textureByte(), &imageByte());
	destroyTexture(rnd, textureActors(), &imageActors());

	return true;
}

bool Theme::load(class Renderer* rnd) {
	// Prepare.
	ImGuiIO &io = ImGui::GetIO();

	// Load the theme configuration.
	fromFile(THEME_CONFIG_DIR THEME_CONFIG_DEFAULT_NAME ".json");

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(THEME_CONFIG_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", false);
	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);
		if (fileInfo->fileName() == THEME_CONFIG_DEFAULT_NAME)
			continue;

		fromFile(fileInfo->fullPath().c_str());
	}

	// Load the builtin fonts.
	ImFontConfig fontCfg;
	fontCfg.FontDataOwnedByAtlas = false;
	fontBlock(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK, sizeof(RES_FONT_BLOCK),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);
	fontBlock_Bold(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK_BOLD, sizeof(RES_FONT_BLOCK_BOLD),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);
	fontBlock_Italic(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK_ITALIC, sizeof(RES_FONT_BLOCK_ITALIC),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);
	fontBlock_BoldItalic(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK_BOLD_ITALIC, sizeof(RES_FONT_BLOCK_BOLD_ITALIC),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);

	// Rebuild the font glyphs.
	if (io.Fonts->TexID) {
		ImGuiSDLHack::Texture* texture = static_cast<ImGuiSDLHack::Texture*>(io.Fonts->TexID);
		delete texture;
		io.Fonts->TexID = nullptr;
	}

	unsigned char* pixels = nullptr;
	int width = 0, height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	ImGuiSDLHack::Texture* texture = new ImGuiSDLHack::Texture(rnd, pixels, width, height);
	io.Fonts->TexID = (void*)texture;

	// Finish.
	memcpy(ImGui::GetStyle().Colors, style()->builtin, sizeof(ImGui::GetStyle().Colors));

	return true;
}

bool Theme::save(void) const {
	return true;
}

int Theme::iconSize(void) const {
	return iconHand()->width(); // Borrowed.
}

bool Theme::createImage(class Renderer*, const Byte* buf, size_t len, class Image** img) {
	if (img)
		*img = nullptr;

	Image* image = Image::create();
	image->fromBytes(buf, len);

	if (img)
		*img = image;

	return true;
}

bool Theme::createImage(class Renderer* rnd, const char* path, class Image** img) {
	if (img)
		*img = nullptr;

	File::Ptr file(File::create());
	if (file->open(path, Stream::READ)) {
		Bytes::Ptr buf(Bytes::create());
		file->readBytes(buf.get());
		createImage(rnd, buf->pointer(), buf->count(), img);
		file->close();
	}

	return true;
}

void Theme::destroyImage(class Renderer*, class Image** img) {
	if (img && *img) {
		Image::destroy(*img);
		*img = nullptr;
	}
}

class Texture* Theme::createTexture(class Renderer* rnd, const Byte* buf, size_t len, class Image** img) {
	if (img)
		*img = nullptr;

	Texture* texture = Texture::create();
	Image* image = Image::create();
	image->fromBytes(buf, len);
	texture->fromImage(rnd, Texture::STATIC, image, Texture::NEAREST);

	if (img)
		*img = image;
	else
		Image::destroy(image);

	return texture;
}

class Texture* Theme::createTexture(class Renderer* rnd, const char* path, class Image** img) {
	if (img)
		*img = nullptr;

	Texture* texture = nullptr;
	File::Ptr file(File::create());
	if (file->open(path, Stream::READ)) {
		Bytes::Ptr buf(Bytes::create());
		file->readBytes(buf.get());
		texture = createTexture(rnd, buf->pointer(), buf->count(), img);
		file->close();
	}

	return texture;
}

void Theme::destroyTexture(class Renderer* /* rnd */, class Texture* &tex, class Image** img) {
	Texture::destroy(tex);
	tex = nullptr;

	if (img && *img) {
		Image::destroy(*img);
		*img = nullptr;
	}
}

void Theme::setValue(const std::string &idx, ImU32 val) {
	if (idx == "project_icon_scale")
		styleDefault().projectIconScale = val;
}

void Theme::setColor(ImGuiCol idx, const ImColor &col) {
	if (idx < 0 || idx >= ImGuiCol_COUNT)
		return;

	styleDefault().builtin[idx] = (ImVec4)col;
}

void Theme::setColor(const std::string &idx, const ImColor &col) {
	if (idx == "screen_clearing_color")
		styleDefault().screenClearingColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "graph_color")
		styleDefault().graphColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "graph_drawing_color")
		styleDefault().graphDrawingColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "highlight_frame_color")
		styleDefault().highlightFrameColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "highlight_frame_hovered_color")
		styleDefault().highlightFrameHoveredColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "highlight_frame_active_color")
		styleDefault().highlightFrameActiveColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "highlight_button_color")
		styleDefault().highlightButtonColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "highlight_button_hovered_color")
		styleDefault().highlightButtonHoveredColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "highlight_button_active_color")
		styleDefault().highlightButtonActiveColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "selected_button")
		styleDefault().selectedButtonColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "selected_button_hovered")
		styleDefault().selectedButtonHoveredColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "selected_button_active")
		styleDefault().selectedButtonActiveColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "danger_button")
		styleDefault().dangerButtonColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "danger_button_hovered")
		styleDefault().dangerButtonHoveredColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "danger_button_active")
		styleDefault().dangerButtonActiveColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "tab_text")
		styleDefault().tabTextColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "tab_pending")
		styleDefault().tabPendingColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "tab_pending_hovered")
		styleDefault().tabPendingHoveredColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "icon")
		styleDefault().iconColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "icon_disabled")
		styleDefault().iconDisabledColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "message")
		styleDefault().messageColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "warning")
		styleDefault().warningColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "error")
		styleDefault().errorColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "debug")
		styleDefault().debugColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "i18n_head")
		styleDefault().i18nHeadColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "music_side")
		styleDefault().musicSideColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "music_note")
		styleDefault().musicNoteColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "music_instrument")
		styleDefault().musicInstrumentColor = ImGui::GetColorU32((ImVec4)col);
	else if (idx == "music_effect")
		styleDefault().musicEffectColor = ImGui::GetColorU32((ImVec4)col);
}

void Theme::fromFile(const char* path_) {
	ImGuiIO &io = ImGui::GetIO();

	const std::string cfgPath = path_;

	rapidjson::Document doc;
	File::Ptr file(File::create());
	if (file->open(cfgPath.c_str(), Stream::READ)) {
		std::string buf;
		file->readString(buf);
		file->close();
		if (!Json::fromString(doc, buf.c_str()))
			doc.SetNull();
	}
	file.reset();
	std::string cfgDir;
	Path::split(cfgPath, nullptr, nullptr, &cfgDir);

	const rapidjson::Value* values = nullptr;
	Jpath::get(doc, values, "values");
	if (values && values->IsObject()) {
		for (rapidjson::Value::ConstMemberIterator ittheme = values->MemberBegin(); ittheme != values->MemberEnd(); ++ittheme) {
			const rapidjson::Value &jktheme = ittheme->name;
			const rapidjson::Value &jvtheme = ittheme->value;

			const std::string theme = jktheme.GetString();
			ImU32 val = 0;
			if (!Jpath::get(jvtheme, val, 0))
				continue;

			if (theme == "project_icon_scale")
				setValue("project_icon_scale", val);
		}
	}

	const rapidjson::Value* colors = nullptr;
	Jpath::get(doc, colors, "colors");
	if (colors && colors->IsObject()) {
		for (rapidjson::Value::ConstMemberIterator ittheme = colors->MemberBegin(); ittheme != colors->MemberEnd(); ++ittheme) {
			const rapidjson::Value &jktheme = ittheme->name;
			const rapidjson::Value &jvtheme = ittheme->value;

			const std::string theme = jktheme.GetString();
			Colour col;
			if (!Jpath::get(jvtheme, col.r, 0) || !Jpath::get(jvtheme, col.g, 1) || !Jpath::get(jvtheme, col.b, 2) || !Jpath::get(jvtheme, col.a, 3))
				continue;

			if (theme == "text")
				setColor(ImGuiCol_Text, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "text_disabled")
				setColor(ImGuiCol_TextDisabled, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "window_background")
				setColor(ImGuiCol_WindowBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "child_background")
				setColor(ImGuiCol_ChildBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "popup_background")
				setColor(ImGuiCol_PopupBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "border")
				setColor(ImGuiCol_Border, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "border_shadow")
				setColor(ImGuiCol_BorderShadow, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "frame_background")
				setColor(ImGuiCol_FrameBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "frame_background_hovered")
				setColor(ImGuiCol_FrameBgHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "frame_background_active")
				setColor(ImGuiCol_FrameBgActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "title_background")
				setColor(ImGuiCol_TitleBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "title_background_active")
				setColor(ImGuiCol_TitleBgActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "title_background_collapsed")
				setColor(ImGuiCol_TitleBgCollapsed, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "menu_bar_background")
				setColor(ImGuiCol_MenuBarBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "scrollbar_background")
				setColor(ImGuiCol_ScrollbarBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "scrollbar_grab")
				setColor(ImGuiCol_ScrollbarGrab, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "scrollbar_grab_hovered")
				setColor(ImGuiCol_ScrollbarGrabHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "scrollbar_grab_active")
				setColor(ImGuiCol_ScrollbarGrabActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "check_mark")
				setColor(ImGuiCol_CheckMark, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "slider_grab")
				setColor(ImGuiCol_SliderGrab, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "slider_grab_active")
				setColor(ImGuiCol_SliderGrabActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "button")
				setColor(ImGuiCol_Button, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "button_hovered")
				setColor(ImGuiCol_ButtonHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "button_active")
				setColor(ImGuiCol_ButtonActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "header")
				setColor(ImGuiCol_Header, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "header_hovered")
				setColor(ImGuiCol_HeaderHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "header_active")
				setColor(ImGuiCol_HeaderActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "separator")
				setColor(ImGuiCol_Separator, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "separator_hovered")
				setColor(ImGuiCol_SeparatorHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "separator_active")
				setColor(ImGuiCol_SeparatorActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "resize_grip")
				setColor(ImGuiCol_ResizeGrip, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "resize_grip_hovered")
				setColor(ImGuiCol_ResizeGripHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "resize_grip_active")
				setColor(ImGuiCol_ResizeGripActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab")
				setColor(ImGuiCol_Tab, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab_hovered")
				setColor(ImGuiCol_TabHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab_active")
				setColor(ImGuiCol_TabActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab_unfocused")
				setColor(ImGuiCol_TabUnfocused, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab_unfocused_active")
				setColor(ImGuiCol_TabUnfocusedActive, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "plot_lines")
				setColor(ImGuiCol_PlotLines, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "plot_lines_hovered")
				setColor(ImGuiCol_PlotLinesHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "plot_histogram")
				setColor(ImGuiCol_PlotHistogram, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "plot_histogram_hovered")
				setColor(ImGuiCol_PlotHistogramHovered, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "table_header_background")
				setColor(ImGuiCol_TableHeaderBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "table_border_strong")
				setColor(ImGuiCol_TableBorderStrong, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "table_border_light")
				setColor(ImGuiCol_TableBorderLight, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "table_row_background")
				setColor(ImGuiCol_TableRowBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "table_row_background_alt")
				setColor(ImGuiCol_TableRowBgAlt, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "text_selected_background")
				setColor(ImGuiCol_TextSelectedBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "drag_drop_target")
				setColor(ImGuiCol_DragDropTarget, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "nav_highlight")
				setColor(ImGuiCol_NavHighlight, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "nav_windowing_highlight")
				setColor(ImGuiCol_NavWindowingHighlight, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "nav_windowing_mask_background")
				setColor(ImGuiCol_NavWindowingDimBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "window_mask_background")
				setColor(ImGuiCol_ModalWindowDimBg, ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "screen_clearing_color")
				setColor("screen_clearing_color", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "graph_color")
				setColor("graph_color", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "graph_drawing_color")
				setColor("graph_drawing_color", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "selected_button")
				setColor("selected_button", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "selected_button_hovered")
				setColor("selected_button_hovered", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "selected_button_active")
				setColor("selected_button_active", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab_text")
				setColor("tab_text", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab_pending")
				setColor("tab_pending", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "tab_pending_hovered")
				setColor("tab_pending_hovered", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "icon")
				setColor("icon", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "icon_disabled")
				setColor("icon_disabled", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "message")
				setColor("message", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "warning")
				setColor("warning", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "error")
				setColor("error", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "debug")
				setColor("debug", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "music_side")
				setColor("music_side", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "music_note")
				setColor("music_note", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "music_instrument")
				setColor("music_instrument", ImColor(col.r, col.g, col.b, col.a));
			else if (theme == "music_effect")
				setColor("music_effect", ImColor(col.r, col.g, col.b, col.a));
		}
	}

	const rapidjson::Value* fonts = nullptr;
	Jpath::get(doc, fonts, "fonts");
	if (fonts && fonts->IsArray()) {
		for (int i = 0; i < (int)fonts->Capacity(); ++i) {
			std::string operation = "merge";
			std::string usage = "generic";
			std::string path;
			float size = 0;
			std::string ranges;
			Math::Vec2i oversample(0, 0);
			Math::Vec2f glyphOffset(0, 0);

			Jpath::get(*fonts, operation, i, "operation");
			Jpath::get(*fonts, usage, i, "usage");
			Jpath::get(*fonts, path, i, "path");
			Jpath::get(*fonts, size, i, "size");
			Jpath::get(*fonts, ranges, i, "ranges");
			Jpath::get(*fonts, oversample.x, i, "oversample", 0);
			Jpath::get(*fonts, oversample.y, i, "oversample", 1);
			Jpath::get(*fonts, glyphOffset.x, i, "glyph_offset", 0);
			Jpath::get(*fonts, glyphOffset.y, i, "glyph_offset", 1);

			path = Path::combine(cfgDir.c_str(), path.c_str());
			size = Math::clamp(size, 4.0f, 96.0f);
			oversample.x = Math::clamp(oversample.x, (Int)1, (Int)8);
			oversample.y = Math::clamp(oversample.y, (Int)1, (Int)8);
			glyphOffset.x = Math::clamp(glyphOffset.x, (Real)-96, (Real)96);
			glyphOffset.y = Math::clamp(glyphOffset.y, (Real)-96, (Real)96);

			const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesDefault();
			if (ranges == THEME_FONT_RANGES_DEFAULT_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesDefault();
			} else if (ranges == THEME_FONT_RANGES_CHINESE_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesChineseFull();
			} else if (ranges == THEME_FONT_RANGES_JAPANESE_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesJapanese();
			} else if (ranges == THEME_FONT_RANGES_KOREAN_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesKorean();
			} else if (ranges == THEME_FONT_RANGES_CYRILLIC_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesCyrillic();
			} else if (ranges == THEME_FONT_RANGES_THAI_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesThai();
			} else if (ranges == THEME_FONT_RANGES_VIETNAMESE_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesVietnamese();
			} else if (ranges == THEME_FONT_RANGES_POLISH_NAME) {
				static constexpr const ImWchar RANGES_POLISH[] = {
					0x0020, 0x00ff, // Basic Latin + Latin supplement.
					0x0100, 0x017f, // Polish alphabet.
					0
				};
				glyphRanges = RANGES_POLISH;
			} else if (!ranges.empty()) {
				const std::wstring wstr = Unicode::toWide(ranges);
				if (!wstr.empty() && wstr.size() % 2 == 0) {
					if (!customFontRanges().empty())
						customFontRanges().pop_back();
					for (int j = 0; j < (int)wstr.length(); j += 2) {
						const wchar_t ch0 = wstr[j];
						const wchar_t ch1 = wstr[j + 1];
						if (ch0 > ch1) {
							customFontRanges().clear();

							break;
						}
						customFontRanges().push_back(ch0);
						customFontRanges().push_back(ch1);
					}
					if (!customFontRanges().empty()) {
						if (customFontRanges().back() != 0)
							customFontRanges().push_back(0);
						glyphRanges = &customFontRanges().front();
					}
				}
			}

			ImFontConfig fontCfg;
			fontCfg.OversampleH = (int)oversample.x; fontCfg.OversampleV = (int)oversample.y;
			fontCfg.GlyphOffset = ImVec2((float)glyphOffset.x, (float)glyphOffset.y);
			fontCfg.MergeMode = operation == "merge";
			if (operation == "set" || operation == "merge") {
				const bool setDefault = operation == "set" && usage == "generic" && glyphRanges == io.Fonts->GetGlyphRangesDefault();
				if (setDefault)
					io.Fonts->Clear();
				ImFont* font = io.Fonts->AddFontFromFileTTF(
					path.c_str(),
					size,
					&fontCfg, glyphRanges
				);
				if (setDefault && !font)
					io.Fonts->AddFontDefault();
				if (usage == "code")
					fontCode(font);
			} else if (operation == "clear") {
				io.Fonts->Clear();
				io.Fonts->AddFontDefault();
			}
		}
	}
}

/* ===========================================================================} */
