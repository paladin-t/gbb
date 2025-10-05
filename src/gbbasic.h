/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __GBBASIC_H__
#define __GBBASIC_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** {===========================================================================
** Compiler
*/

#ifndef GBBASIC_CP
#	if defined __EMSCRIPTEN__
#		define GBBASIC_CP "Emscripten"
#		define GBBASIC_CP_EMSCRIPTEN
#	elif defined _MSC_VER
#		define GBBASIC_CP "VC++"
#		define GBBASIC_CP_VC _MSC_VER
#	elif defined __CYGWIN__
#		define GBBASIC_CP "Cygwin"
#		define GBBASIC_CP_CYGWIN
#	elif defined __MINGW32__
#		define GBBASIC_CP "MinGW"
#		define GBBASIC_CP_MINGW32
#	elif defined __clang__
#		define GBBASIC_CP "Clang"
#		define GBBASIC_CP_CLANG
#	elif defined __GNUC__ || defined __GNUG__
#		define GBBASIC_CP "GCC"
#		define GBBASIC_CP_GCC
#	else
#		define GBBASIC_CP "Unknown"
#		define GBBASIC_CP_UNKNOWN
#	endif /* Compiler dependent macros. */
#endif /* GBBASIC_CP */

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

#ifndef GBBASIC_OS
#	if defined __EMSCRIPTEN__
#		if defined __asmjs__
#			define GBBASIC_OS "HTML [AsmJS]"
#		else
#			define GBBASIC_OS "HTML [Wasm]"
#		endif
#		define GBBASIC_OS_HTML
#	elif defined _WIN64
#		if defined _M_ARM
#			define GBBASIC_OS "Windows [ARM64]"
#		else
#			define GBBASIC_OS "Windows [x86_64]"
#		endif
#		define GBBASIC_OS_WIN
#		define GBBASIC_OS_WIN64
#	elif defined _WIN32
#		if defined _M_ARM
#			define GBBASIC_OS "Windows [ARM32]"
#		else
#			define GBBASIC_OS "Windows [x86]"
#		endif
#		define GBBASIC_OS_WIN
#		define GBBASIC_OS_WIN32
#	elif defined __APPLE__
#		include <TargetConditionals.h>
#		define GBBASIC_OS_APPLE
#		if defined TARGET_OS_IPHONE && TARGET_OS_IPHONE == 1
#			define GBBASIC_OS "iOS"
#			define GBBASIC_OS_IOS
#		elif defined TARGET_IPHONE_SIMULATOR && TARGET_IPHONE_SIMULATOR == 1
#			define GBBASIC_OS "iOS [sim]"
#			define GBBASIC_OS_IOS_SIM
#		elif defined TARGET_OS_MAC && TARGET_OS_MAC == 1
#			if defined __x86_64__
#				define GBBASIC_OS "MacOS [x86_64]"
#			elif defined __i386__
#				define GBBASIC_OS "MacOS [x86]"
#			elif defined __aarch64__
#				define GBBASIC_OS "MacOS [ARM64]"
#			elif defined __arm__
#				define GBBASIC_OS "MacOS [ARM32]"
#			else
#				define GBBASIC_OS "MacOS"
#			endif
#			define GBBASIC_OS_MAC
#		endif
#	elif defined __ANDROID__
#		define GBBASIC_OS "Android"
#		define GBBASIC_OS_ANDROID
#	elif defined __linux__
#		if defined __x86_64__
#			define GBBASIC_OS "Linux [x86_64]"
#		elif defined __i386__
#			define GBBASIC_OS "Linux [x86]"
#		elif defined __aarch64__
#			define GBBASIC_OS "Linux [ARM64]"
#		elif defined __arm__
#			define GBBASIC_OS "Linux [ARM32]"
#		else
#			define GBBASIC_OS "Linux"
#		endif
#		define GBBASIC_OS_LINUX
#	elif defined __unix__
#		define GBBASIC_OS "Unix"
#		define GBBASIC_OS_UNIX
#	else
#		define GBBASIC_OS "Unknown"
#		define GBBASIC_OS_UNKNOWN
#	endif /* OS dependent macros. */
#endif /* GBBASIC_OS */

/* ===========================================================================} */

/*
** {===========================================================================
** Version
*/

#ifndef GBBASIC_TITLE
#	define GBBASIC_TITLE "GB BASIC"
#endif /* GBBASIC_TITLE */

#ifndef GBBASIC_VERSION
#	define GBBASIC_VER_MAJOR 1
#	define GBBASIC_VER_MINOR 2
#	define GBBASIC_VER_REVISION 0
#	define GBBASIC_VER_SUFFIX
#	define GBBASIC_VERSION ((GBBASIC_VER_MAJOR * 0x01000000) + (GBBASIC_VER_MINOR * 0x00010000) + (GBBASIC_VER_REVISION))
#	define GBBASIC_MAKE_STRINGIZE(A) #A
#	define GBBASIC_STRINGIZE(A) GBBASIC_MAKE_STRINGIZE(A)
#	define GBBASIC_VERSION_STRING GBBASIC_STRINGIZE(GBBASIC_VER_MAJOR.GBBASIC_VER_MINOR.GBBASIC_VER_REVISION)
#	define GBBASIC_VERSION_STRING_WITH_SUFFIX GBBASIC_STRINGIZE(GBBASIC_VER_MAJOR.GBBASIC_VER_MINOR.GBBASIC_VER_REVISION GBBASIC_VER_SUFFIX)
#endif /* GBBASIC_VERSION */

#ifndef GBBASIC_ENGINE_VERSION
#	define GBBASIC_ENGINE_VERSION 1
#endif /* GBBASIC_ENGINE_VERSION */

/* ===========================================================================} */

/*
** {===========================================================================
** Features
*/

// Indicates whether project code executes on a thread separately from graphics.
#ifndef GBBASIC_MULTITHREAD_ENABLED
#	if defined GBBASIC_OS_HTML
#		define GBBASIC_MULTITHREAD_ENABLED 0
#	else /* GBBASIC_OS_HTML */
#		define GBBASIC_MULTITHREAD_ENABLED 1
#	endif /* GBBASIC_OS_HTML */
#endif /* GBBASIC_MULTITHREAD_ENABLED */

// Indicates whether auto size is enabled.
#ifndef GBBASIC_DISPLAY_AUTO_SIZE_ENABLED
#	define GBBASIC_DISPLAY_AUTO_SIZE_ENABLED 0
#endif /* GBBASIC_DISPLAY_AUTO_SIZE_ENABLED */

// Indicates whether the splash is enabled.
#ifndef GBBASIC_SPLASH_ENABLED
#	define GBBASIC_SPLASH_ENABLED 1
#endif /* GBBASIC_SPLASH_ENABLED */

// Indicates whether the web module is enabled.
#ifndef GBBASIC_WEB_ENABLED
#	define GBBASIC_WEB_ENABLED 1
#endif /* GBBASIC_WEB_ENABLED */

// Indicates whether the static analyzer is enabled.
#ifndef GBBASIC_COMPILER_ANALYZER_ENABLED
#	if defined GBBASIC_OS_HTML
#		define GBBASIC_COMPILER_ANALYZER_ENABLED 1
#	else /* GBBASIC_OS_HTML */
#		define GBBASIC_COMPILER_ANALYZER_ENABLED 1
#	endif /* GBBASIC_OS_HTML */
#endif /* GBBASIC_COMPILER_ANALYZER_ENABLED */

// The maximum count of asset page.
#ifndef GBBASIC_ASSET_MAX_PAGE_COUNT
#	define GBBASIC_ASSET_MAX_PAGE_COUNT 100
#endif /* GBBASIC_ASSET_MAX_PAGE_COUNT */
// Indicates whether to show asset page number in HEX.
#ifndef GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED
#	define GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED 0
#endif /* GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED */
// Indicates whether to switch to the eyedropper tool automatically when switched to a byte-based layer.
// For map and scene editors.
#ifndef GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED
#	define GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED 0
#endif /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
// Indicates whether to modify mask on data toggled automatically with a byte-based layer.
// For map and scene editors.
#ifndef GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED
#	define GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED 0
#endif /* GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED */

// The maximum count of editor command queue.
#ifndef GBBASIC_EDITOR_MAX_COMMAND_COUNT
#	define GBBASIC_EDITOR_MAX_COMMAND_COUNT 2000
#endif /* GBBASIC_EDITOR_MAX_COMMAND_COUNT */

// Indicates whether code editor is splittable.
#ifndef GBBASIC_EDITOR_CODE_SPLIT_ENABLED
#	define GBBASIC_EDITOR_CODE_SPLIT_ENABLED 1
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

// Indicates whether to use full 2D paintable information other than slice-based for map/scene commands.
#ifndef GBBASIC_EDITOR_PAINTABLE2D_ENABLED
#	define GBBASIC_EDITOR_PAINTABLE2D_ENABLED 1
#endif /* GBBASIC_EDITOR_PAINTABLE2D_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Configurations
*/

// The frame rate when the application is active.
#ifndef GBBASIC_ACTIVE_FRAME_RATE
#	define GBBASIC_ACTIVE_FRAME_RATE 60
#endif /* GBBASIC_ACTIVE_FRAME_RATE */

// The minimum width of window.
#ifndef GBBASIC_WINDOW_MIN_WIDTH
#	define GBBASIC_WINDOW_MIN_WIDTH 320
#endif /* GBBASIC_WINDOW_MIN_WIDTH */
// The minimum height of window.
#ifndef GBBASIC_WINDOW_MIN_HEIGHT
#	define GBBASIC_WINDOW_MIN_HEIGHT 182 /* 144 + 19 + 19 */
#endif /* GBBASIC_WINDOW_MIN_HEIGHT */
// The default width of window.
#ifndef GBBASIC_WINDOW_DEFAULT_WIDTH
#	define GBBASIC_WINDOW_DEFAULT_WIDTH (GBBASIC_WINDOW_MIN_WIDTH * 2)
#endif /* GBBASIC_WINDOW_DEFAULT_WIDTH */
// The default height of window.
#ifndef GBBASIC_WINDOW_DEFAULT_HEIGHT
#	define GBBASIC_WINDOW_DEFAULT_HEIGHT (GBBASIC_WINDOW_MIN_HEIGHT * 2)
#endif /* GBBASIC_WINDOW_DEFAULT_HEIGHT */

// The width of screen.
#ifndef GBBASIC_SCREEN_WIDTH
#	define GBBASIC_SCREEN_WIDTH 160
#endif /* GBBASIC_SCREEN_WIDTH */
// The height of screen.
#ifndef GBBASIC_SCREEN_HEIGHT
#	define GBBASIC_SCREEN_HEIGHT 144
#endif /* GBBASIC_SCREEN_HEIGHT */

// The bit count of palette group.
#ifndef GBBASIC_PALETTE_DEPTH
#	define GBBASIC_PALETTE_DEPTH 2
#endif /* GBBASIC_PALETTE_DEPTH */
// The color count of a group of palette.
#ifndef GBBASIC_PALETTE_PER_GROUP_COUNT
#	define GBBASIC_PALETTE_PER_GROUP_COUNT 4
#endif /* GBBASIC_PALETTE_PER_GROUP_COUNT */
// The bit count of palette color.
#ifndef GBBASIC_PALETTE_COLOR_DEPTH
#	define GBBASIC_PALETTE_COLOR_DEPTH 6
#endif /* GBBASIC_PALETTE_COLOR_DEPTH */

// The minimum size of font.
#ifndef GBBASIC_FONT_MIN_SIZE
#	define GBBASIC_FONT_MIN_SIZE 1
#endif /* GBBASIC_FONT_MIN_SIZE */
// The maximum size of font.
#ifndef GBBASIC_FONT_MAX_SIZE
#	define GBBASIC_FONT_MAX_SIZE 16
#endif /* GBBASIC_FONT_MAX_SIZE */
// The default size of font.
#ifndef GBBASIC_FONT_DEFAULT_SIZE
#	define GBBASIC_FONT_DEFAULT_SIZE 13
#endif /* GBBASIC_FONT_DEFAULT_SIZE */
// The default offset of font.
#ifndef GBBASIC_FONT_DEFAULT_OFFSET
#	define GBBASIC_FONT_DEFAULT_OFFSET (-1)
#endif /* GBBASIC_FONT_DEFAULT_OFFSET */
// The default 2BPP property of font.
#ifndef GBBASIC_FONT_DEFAULT_IS_2BPP
#	define GBBASIC_FONT_DEFAULT_IS_2BPP false
#endif /* GBBASIC_FONT_DEFAULT_IS_2BPP */
// The default full word preference (for ASCII characters) property of font.
#ifndef GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD
#	define GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD true
#endif /* GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD */
// The default full word preference for non-ASCII characters property of font.
#ifndef GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII
#	define GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII false
#endif /* GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII */
// The minimum threshold of font.
#ifndef GBBASIC_FONT_MIN_THRESHOLD
#	define GBBASIC_FONT_MIN_THRESHOLD 0
#endif /* GBBASIC_FONT_MIN_THRESHOLD */
// The maximum threshold of font.
#ifndef GBBASIC_FONT_MAX_THRESHOLD
#	define GBBASIC_FONT_MAX_THRESHOLD 255
#endif /* GBBASIC_FONT_MAX_THRESHOLD */
// The maximum size of font content.
#ifndef GBBASIC_FONT_CONTENT_MAX_SIZE
#	define GBBASIC_FONT_CONTENT_MAX_SIZE 2048
#endif /* GBBASIC_FONT_CONTENT_MAX_SIZE */

// The tile size in pixels.
#ifndef GBBASIC_TILE_SIZE
#	define GBBASIC_TILE_SIZE 8
#endif /* GBBASIC_TILE_SIZE */

// The maximum area size of tiles asset in tiles.
#ifndef GBBASIC_TILES_MAX_AREA_SIZE
#	define GBBASIC_TILES_MAX_AREA_SIZE 256
#endif /* GBBASIC_TILES_MAX_AREA_SIZE */
// The default width of tiles asset in tiles.
#ifndef GBBASIC_TILES_DEFAULT_WIDTH
#	define GBBASIC_TILES_DEFAULT_WIDTH 16
#endif /* GBBASIC_TILES_DEFAULT_WIDTH */
// The default height of tiles asset in tiles.
#ifndef GBBASIC_TILES_DEFAULT_HEIGHT
#	define GBBASIC_TILES_DEFAULT_HEIGHT 16
#endif /* GBBASIC_TILES_DEFAULT_HEIGHT */

// The maximum area size of map asset in tiles.
#ifndef GBBASIC_MAP_MAX_AREA_SIZE
#	define GBBASIC_MAP_MAX_AREA_SIZE 4095
#endif /* GBBASIC_MAP_MAX_AREA_SIZE */
// The maximum width of map asset in tiles.
#ifndef GBBASIC_MAP_MAX_WIDTH
#	define GBBASIC_MAP_MAX_WIDTH 255
#endif /* GBBASIC_MAP_MAX_WIDTH */
// The maximum height of map asset in tiles.
#ifndef GBBASIC_MAP_MAX_HEIGHT
#	define GBBASIC_MAP_MAX_HEIGHT 255
#endif /* GBBASIC_MAP_MAX_HEIGHT */
// The default width of map asset in tiles.
#ifndef GBBASIC_MAP_DEFAULT_WIDTH
#	define GBBASIC_MAP_DEFAULT_WIDTH 20
#endif /* GBBASIC_MAP_DEFAULT_WIDTH */
// The default height of map asset in tiles.
#ifndef GBBASIC_MAP_DEFAULT_HEIGHT
#	define GBBASIC_MAP_DEFAULT_HEIGHT 18
#endif /* GBBASIC_MAP_DEFAULT_HEIGHT */
// The palette bit 0 of map.
#ifndef GBBASIC_MAP_PALETTE_BIT0
#	define GBBASIC_MAP_PALETTE_BIT0 0
#endif /* GBBASIC_MAP_PALETTE_BIT0 */
// The palette bit 1 of map.
#ifndef GBBASIC_MAP_PALETTE_BIT1
#	define GBBASIC_MAP_PALETTE_BIT1 1
#endif /* GBBASIC_MAP_PALETTE_BIT1 */
// The palette bit 2 of map.
#ifndef GBBASIC_MAP_PALETTE_BIT2
#	define GBBASIC_MAP_PALETTE_BIT2 2
#endif /* GBBASIC_MAP_PALETTE_BIT2 */
// The bank bit of map.
#ifndef GBBASIC_MAP_BANK_BIT
#	define GBBASIC_MAP_BANK_BIT 3
#endif /* GBBASIC_MAP_BANK_BIT */
// The first unused bit of map.
#ifndef GBBASIC_MAP_UNUSED_A_BIT
#	define GBBASIC_MAP_UNUSED_A_BIT 4
#endif /* GBBASIC_MAP_UNUSED_A_BIT */
// The horizontal flip bit of map.
#ifndef GBBASIC_MAP_HFLIP_BIT
#	define GBBASIC_MAP_HFLIP_BIT 5
#endif /* GBBASIC_MAP_HFLIP_BIT */
// The vertical flip bit of map.
#ifndef GBBASIC_MAP_VFLIP_BIT
#	define GBBASIC_MAP_VFLIP_BIT 6
#endif /* GBBASIC_MAP_VFLIP_BIT */
// The priority bit of map.
#ifndef GBBASIC_MAP_PRIORITY_BIT
#	define GBBASIC_MAP_PRIORITY_BIT 7
#endif /* GBBASIC_MAP_PRIORITY_BIT */

// The blocking left bit of scene.
#ifndef GBBASIC_SCENE_BLOCKING_LEFT_BIT
#	define GBBASIC_SCENE_BLOCKING_LEFT_BIT 0
#endif /* GBBASIC_SCENE_BLOCKING_LEFT_BIT */
// The blocking right bit of scene.
#ifndef GBBASIC_SCENE_BLOCKING_RIGHT_BIT
#	define GBBASIC_SCENE_BLOCKING_RIGHT_BIT 1
#endif /* GBBASIC_SCENE_BLOCKING_RIGHT_BIT */
// The blocking up bit of scene.
#ifndef GBBASIC_SCENE_BLOCKING_UP_BIT
#	define GBBASIC_SCENE_BLOCKING_UP_BIT 2
#endif /* GBBASIC_SCENE_BLOCKING_UP_BIT */
// The blocking down bit of scene.
#ifndef GBBASIC_SCENE_BLOCKING_DOWN_BIT
#	define GBBASIC_SCENE_BLOCKING_DOWN_BIT 3
#endif /* GBBASIC_SCENE_BLOCKING_DOWN_BIT */
// The is ladder bit of scene.
#ifndef GBBASIC_SCENE_IS_LADDER_BIT
#	define GBBASIC_SCENE_IS_LADDER_BIT 4
#endif /* GBBASIC_SCENE_IS_LADDER_BIT */
// The first unused bit of scene.
#ifndef GBBASIC_SCENE_UNUSED_A_BIT
#	define GBBASIC_SCENE_UNUSED_A_BIT 5
#endif /* GBBASIC_SCENE_UNUSED_A_BIT */
// The second unused bit of scene.
#ifndef GBBASIC_SCENE_UNUSED_B_BIT
#	define GBBASIC_SCENE_UNUSED_B_BIT 6
#endif /* GBBASIC_SCENE_UNUSED_B_BIT */
// The third unused bit of scene.
#ifndef GBBASIC_SCENE_UNUSED_C_BIT
#	define GBBASIC_SCENE_UNUSED_C_BIT 7
#endif /* GBBASIC_SCENE_UNUSED_C_BIT */

// The pattern count of music asset.
#ifndef GBBASIC_MUSIC_PATTERN_COUNT
#	define GBBASIC_MUSIC_PATTERN_COUNT 64
#endif /* GBBASIC_MUSIC_PATTERN_COUNT */
// The order count of music asset.
#ifndef GBBASIC_MUSIC_ORDER_COUNT
#	define GBBASIC_MUSIC_ORDER_COUNT 128
#endif /* GBBASIC_MUSIC_ORDER_COUNT */
// The max sequence count of music asset.
#ifndef GBBASIC_MUSIC_SEQUENCE_MAX_COUNT
#	define GBBASIC_MUSIC_SEQUENCE_MAX_COUNT 128
#endif /* GBBASIC_MUSIC_SEQUENCE_MAX_COUNT */
// The channel count of music asset.
#ifndef GBBASIC_MUSIC_CHANNEL_COUNT
#	define GBBASIC_MUSIC_CHANNEL_COUNT 4
#endif /* GBBASIC_MUSIC_CHANNEL_COUNT */
// The instrument count of music asset.
#ifndef GBBASIC_MUSIC_INSTRUMENT_COUNT
#	define GBBASIC_MUSIC_INSTRUMENT_COUNT 15
#endif /* GBBASIC_MUSIC_INSTRUMENT_COUNT */
// The sample count per waveform of music asset.
#ifndef GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT
#	define GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT 32
#endif /* GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT */
// The waveform count of music asset.
#ifndef GBBASIC_MUSIC_WAVEFORM_COUNT
#	define GBBASIC_MUSIC_WAVEFORM_COUNT 16
#endif /* GBBASIC_MUSIC_WAVEFORM_COUNT */
// The effect count of music asset.
#ifndef GBBASIC_MUSIC_EFFECT_COUNT
#	define GBBASIC_MUSIC_EFFECT_COUNT 16
#endif /* GBBASIC_MUSIC_EFFECT_COUNT */

// The default palette of actor asset.
#ifndef GBBASIC_ACTOR_DEFAULT_PALETTE
#	define GBBASIC_ACTOR_DEFAULT_PALETTE 8
#endif /* GBBASIC_ACTOR_DEFAULT_PALETTE */
// The maximum width of actor asset in tiles.
#ifndef GBBASIC_ACTOR_MAX_WIDTH
#	define GBBASIC_ACTOR_MAX_WIDTH 8
#endif /* GBBASIC_ACTOR_MAX_WIDTH */
// The maximum height of actor asset in tiles.
#ifndef GBBASIC_ACTOR_MAX_HEIGHT
#	define GBBASIC_ACTOR_MAX_HEIGHT 8
#endif /* GBBASIC_ACTOR_MAX_HEIGHT */
// The default width of actor asset in tiles.
#ifndef GBBASIC_ACTOR_DEFAULT_WIDTH
#	define GBBASIC_ACTOR_DEFAULT_WIDTH 2
#endif /* GBBASIC_ACTOR_DEFAULT_WIDTH */
// The default height of actor asset in tiles.
#ifndef GBBASIC_ACTOR_DEFAULT_HEIGHT
#	define GBBASIC_ACTOR_DEFAULT_HEIGHT 2
#endif /* GBBASIC_ACTOR_DEFAULT_HEIGHT */
// The maximum frame count of actor asset.
#ifndef GBBASIC_ACTOR_FRAME_MAX_COUNT
#	define GBBASIC_ACTOR_FRAME_MAX_COUNT 1024
#endif /* GBBASIC_ACTOR_FRAME_MAX_COUNT */
// The palette bit 0 of actor.
#ifndef GBBASIC_ACTOR_PALETTE_BIT0
#	define GBBASIC_ACTOR_PALETTE_BIT0 0
#endif /* GBBASIC_ACTOR_PALETTE_BIT0 */
// The palette bit 1 of actor.
#ifndef GBBASIC_ACTOR_PALETTE_BIT1
#	define GBBASIC_ACTOR_PALETTE_BIT1 1
#endif /* GBBASIC_ACTOR_PALETTE_BIT1 */
// The palette bit 2 of actor.
#ifndef GBBASIC_ACTOR_PALETTE_BIT2
#	define GBBASIC_ACTOR_PALETTE_BIT2 2
#endif /* GBBASIC_ACTOR_PALETTE_BIT2 */
// The horizontal flip bit of actor.
#ifndef GBBASIC_ACTOR_HFLIP_BIT
#	define GBBASIC_ACTOR_HFLIP_BIT 5
#endif /* GBBASIC_ACTOR_HFLIP_BIT */
// The vertical flip bit of actor.
#ifndef GBBASIC_ACTOR_VFLIP_BIT
#	define GBBASIC_ACTOR_VFLIP_BIT 6
#endif /* GBBASIC_ACTOR_VFLIP_BIT */

// The width of icon.
#ifndef GBBASIC_ICON_WIDTH
#	define GBBASIC_ICON_WIDTH 32
#endif /* GBBASIC_ICON_WIDTH */
// The height of icon.
#ifndef GBBASIC_ICON_HEIGHT
#	define GBBASIC_ICON_HEIGHT 32
#endif /* GBBASIC_ICON_HEIGHT */

// The maximum safe width of texture in pixels.
#ifndef GBBASIC_TEXTURE_SAFE_MAX_WIDTH
#	define GBBASIC_TEXTURE_SAFE_MAX_WIDTH 32768
#endif /* GBBASIC_TEXTURE_SAFE_MAX_WIDTH */
// The maximum safe height of texture in pixels.
#ifndef GBBASIC_TEXTURE_SAFE_MAX_HEIGHT
#	define GBBASIC_TEXTURE_SAFE_MAX_HEIGHT 32768
#endif /* GBBASIC_TEXTURE_SAFE_MAX_HEIGHT */

#ifndef GBBASIC_MODIFIER_KEY_CTRL
#	define GBBASIC_MODIFIER_KEY_CTRL 0
#endif /* GBBASIC_MODIFIER_KEY_CTRL */
#ifndef GBBASIC_MODIFIER_KEY_CMD
#	define GBBASIC_MODIFIER_KEY_CMD 1
#endif /* GBBASIC_MODIFIER_KEY_CMD */
#ifndef GBBASIC_MODIFIER_KEY
#	if defined GBBASIC_OS_APPLE
#		define GBBASIC_MODIFIER_KEY GBBASIC_MODIFIER_KEY_CMD
#	else /* GBBASIC_OS_APPLE */
#		define GBBASIC_MODIFIER_KEY GBBASIC_MODIFIER_KEY_CTRL
#	endif /* GBBASIC_OS_APPLE */
#endif /* GBBASIC_MODIFIER_KEY */
#ifndef GBBASIC_MODIFIER_KEY_NAME
#	if GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CTRL
#		define GBBASIC_MODIFIER_KEY_NAME "Ctrl"
#	elif GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CMD
#		define GBBASIC_MODIFIER_KEY_NAME "Cmd"
#	endif /* GBBASIC_MODIFIER_KEY */
#endif /* GBBASIC_MODIFIER_KEY_NAME */

// The extension name of classic ROM.
#ifndef GBBASIC_CLASSIC_ROM_EXT
#	define GBBASIC_CLASSIC_ROM_EXT "gb"
#endif /* GBBASIC_CLASSIC_ROM_EXT */
// The extension name of colored ROM.
#ifndef GBBASIC_COLORED_ROM_EXT
#	define GBBASIC_COLORED_ROM_EXT "gbc"
#endif /* GBBASIC_COLORED_ROM_EXT */

// The extension name of SRAM state.
#ifndef GBBASIC_SRAM_STATE_EXT
#	define GBBASIC_SRAM_STATE_EXT "sav"
#endif /* GBBASIC_SRAM_STATE_EXT */
// The extension name of dumped ROM.
#ifndef GBBASIC_ROM_DUMP_EXT
#	define GBBASIC_ROM_DUMP_EXT "txt"
#endif /* GBBASIC_ROM_DUMP_EXT */

// The "noname" text for project.
#ifndef GBBASIC_NONAME_PROJECT_NAME
#	define GBBASIC_NONAME_PROJECT_NAME "Noname"
#endif /* GBBASIC_NONAME_PROJECT_NAME */

// The extension name of rich project.
#ifndef GBBASIC_RICH_PROJECT_EXT
#	define GBBASIC_RICH_PROJECT_EXT "gbb"
#endif /* GBBASIC_RICH_PROJECT_EXT */
// The extension name of plain project.
#ifndef GBBASIC_PLAIN_PROJECT_EXT
#	define GBBASIC_PLAIN_PROJECT_EXT "bas"
#endif /* GBBASIC_PLAIN_PROJECT_EXT */

// The any file filter.
#ifndef GBBASIC_ANY_FILE_FILTER
#	define GBBASIC_ANY_FILE_FILTER { \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_ANY_FILE_FILTER */
// The project file filter.
#ifndef GBBASIC_PROJECT_FILE_FILTER
#	define GBBASIC_PROJECT_FILE_FILTER { \
			"GB BASIC files (*." GBBASIC_RICH_PROJECT_EXT ", *." GBBASIC_PLAIN_PROJECT_EXT ")", "*." GBBASIC_RICH_PROJECT_EXT " *." GBBASIC_PLAIN_PROJECT_EXT, \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_PROJECT_FILE_FILTER */
// The code file filter.
#ifndef GBBASIC_CODE_FILE_FILTER
#	define GBBASIC_CODE_FILE_FILTER { \
			"Code files (*.bas, *.gbb, *.txt)", "*.bas *.gbb *.txt", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_CODE_FILE_FILTER */
// The C file filter.
#ifndef GBBASIC_C_FILE_FILTER
#	define GBBASIC_C_FILE_FILTER { \
			"C files (*.c, *.h)", "*.c *.h", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_C_FILE_FILTER */
// The JSON file filter.
#ifndef GBBASIC_JSON_FILE_FILTER
#	define GBBASIC_JSON_FILE_FILTER { \
			"JSON files (*.json)", "*.json", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_JSON_FILE_FILTER */
// The image file filter.
#ifndef GBBASIC_IMAGE_FILE_FILTER
#	define GBBASIC_IMAGE_FILE_FILTER { \
			"Image files (*.png, *.jpg, *.bmp, *.tga)", "*.png *.jpg *.bmp *.tga", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_IMAGE_FILE_FILTER */
// The font file filter.
#ifndef GBBASIC_FONT_FILE_FILTER
#	define GBBASIC_FONT_FILE_FILTER { \
			"Font/image files (*.ttf, *.png, *.jpg, *.bmp, *.tga)", "*.ttf *.png *.jpg *.bmp *.tga", \
			"Font files (*.ttf)", "*.ttf", \
			"Image files (*.png, *.jpg, *.bmp, *.tga)", "*.png *.jpg *.bmp *.tga", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_FONT_FILE_FILTER */
// The VGM file filter.
#ifndef GBBASIC_VGM_FILE_FILTER
#	define GBBASIC_VGM_FILE_FILTER { \
			"VGM files (*.vgm)", "*.vgm", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_VGM_FILE_FILTER */
// The wav file filter.
#ifndef GBBASIC_WAV_FILE_FILTER
#	define GBBASIC_WAV_FILE_FILTER { \
			"WAV files (*.wav)", "*.wav", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_WAV_FILE_FILTER */
// The FxHammer file filter.
#ifndef GBBASIC_FX_HAMMER_FILE_FILTER
#	define GBBASIC_FX_HAMMER_FILE_FILTER { \
			"FxHammer files (*.sav)", "*.sav", \
			"All files (*.*)", "*" \
		}
#endif /* GBBASIC_FX_HAMMER_FILE_FILTER */

/* ===========================================================================} */

/*
** {===========================================================================
** Constants
*/

// Defined with debug build.
#ifndef GBBASIC_DEBUG
#	if defined DEBUG || defined _DEBUG
#		define GBBASIC_DEBUG
#	endif /* DEBUG || _DEBUG */
#endif /* GBBASIC_DEBUG */

// The maximum path length.
#ifndef GBBASIC_MAX_PATH
#	if defined __EMSCRIPTEN__
#		define GBBASIC_MAX_PATH PATH_MAX
#	elif defined _WIN64 || defined _WIN32
#		define GBBASIC_MAX_PATH _MAX_PATH
#	elif defined __ANDROID__
#		include <linux/limits.h>
#		define GBBASIC_MAX_PATH PATH_MAX
#	elif defined __APPLE__
#		define GBBASIC_MAX_PATH PATH_MAX
#	elif defined __linux__
#		include <linux/limits.h>
#		define GBBASIC_MAX_PATH PATH_MAX
#	endif /* Platform macro. */
#endif /* GBBASIC_MAX_PATH */

/* ===========================================================================} */

/*
** {===========================================================================
** Functions
*/

#ifndef GBBASIC_COUNTOF
#	define GBBASIC_COUNTOF(A) (sizeof(A) / sizeof(*(A)))
#endif /* GBBASIC_COUNTOF */

#ifndef GBBASIC_STRICMP
#	if defined GBBASIC_CP_VC
#		define GBBASIC_STRICMP _strcmpi
#	else /* GBBASIC_CP_VC */
#		define GBBASIC_STRICMP strcasecmp
#	endif /* GBBASIC_CP_VC */
#endif /* GBBASIC_STRICMP */

#ifndef GBBASIC_ASSERT
#	if defined GBBASIC_OS_APPLE
#		define GBBASIC_ASSERT(EXPR) ((void)0)
#	else /* GBBASIC_OS_APPLE */
#		define GBBASIC_ASSERT(EXPR) assert(EXPR)
#	endif /* GBBASIC_OS_APPLE */
#endif /* GBBASIC_ASSERT */

#ifndef GBBASIC_UNIQUE_NAME
#	define GBBASIC_CONCAT_INNER(A, B) A##B
#	define GBBASIC_CONCAT(A, B) GBBASIC_CONCAT_INNER(A, B)
#	define GBBASIC_UNIQUE_NAME(BASE) GBBASIC_CONCAT(BASE, __LINE__)
#endif /* GBBASIC_UNIQUE_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Properties and attributes
*/

#ifndef GBBASIC_FIELD
#	define GBBASIC_FIELD(Y, V) \
	protected: \
		Y _##V; \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
		Y &V(void) { \
			return _##V; \
		}
#endif /* GBBASIC_FIELD */
#ifndef GBBASIC_FIELD_READONLY
#	define GBBASIC_FIELD_READONLY(Y, V) \
	protected: \
		Y _##V; \
	public: \
		const Y &V(void) const { \
			return _##V; \
		}
#endif /* GBBASIC_FIELD_READONLY */

#ifndef GBBASIC_PROPERTY_READONLY
#	define GBBASIC_PROPERTY_READONLY(Y, V) \
	protected: \
		Y _##V; \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
	protected: \
		void V(const Y &v) { \
			_##V = v; \
		}
#endif /* GBBASIC_PROPERTY_READONLY */
#ifndef GBBASIC_PROPERTY
#	define GBBASIC_PROPERTY(Y, V) \
	protected: \
		Y _##V; \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
		Y &V(void) { \
			return _##V; \
		} \
		void V(const Y &v) { \
			_##V = v; \
		}
#endif /* GBBASIC_PROPERTY */

#ifndef GBBASIC_PROPERTY_READONLY_VAL
#	define GBBASIC_PROPERTY_READONLY_VAL(Y, V, VAL) \
	protected: \
		Y _##V = (VAL); \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
	protected: \
		void V(const Y &v) { \
			_##V = v; \
		}
#endif /* GBBASIC_PROPERTY_READONLY_VAL */
#ifndef GBBASIC_PROPERTY_VAL
#	define GBBASIC_PROPERTY_VAL(Y, V, VAL) \
	protected: \
		Y _##V = (VAL); \
	public: \
		const Y &V(void) const { \
			return _##V; \
		} \
		Y &V(void) { \
			return _##V; \
		} \
		void V(const Y &v) { \
			_##V = v; \
		}
#endif /* GBBASIC_PROPERTY_VAL */

#ifndef GBBASIC_PROPERTY_READONLY_PTR
#	define GBBASIC_PROPERTY_READONLY_PTR(Y, V) \
	protected: \
		Y* _##V = nullptr; \
	public: \
		Y* V(void) const { \
			return _##V; \
		} \
	protected: \
		void V(Y* v) { \
			_##V = v; \
		} \
		void V(std::nullptr_t) { \
			_##V = nullptr; \
		}
#endif /* GBBASIC_PROPERTY_READONLY_PTR */
#ifndef GBBASIC_PROPERTY_PTR
#	define GBBASIC_PROPERTY_PTR(Y, V) \
	protected: \
		Y* _##V = nullptr; \
	public: \
		const Y* V(void) const { \
			return _##V; \
		} \
		Y* &V(void) { \
			return _##V; \
		} \
		void V(Y* v) { \
			_##V = v; \
		} \
		void V(std::nullptr_t) { \
			_##V = nullptr; \
		}
#endif /* GBBASIC_PROPERTY_PTR */

#ifndef GBBASIC_MISSING
#	define GBBASIC_MISSING do { fprintf(stderr, "Missing function \"%s\".\n", __FUNCTION__); } while (false);
#endif /* GBBASIC_MISSING */

#ifndef GBBASIC_MAKE_UINT32
#	define GBBASIC_MAKE_UINT32(A, B, C, D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))
#endif /* GBBASIC_MAKE_UINT32 */

#ifndef GBBASIC_CLASS_TYPE
#	define GBBASIC_CLASS_TYPE(A, B, C, D) static constexpr unsigned TYPE(void) { return GBBASIC_MAKE_UINT32((A), (B), (C), (D)); }
#endif /* GBBASIC_CLASS_TYPE */

/* ===========================================================================} */

#endif /* __GBBASIC_H__ */
