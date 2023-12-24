#include "stdafx.h"

#ifndef CODE_MENU_HEADER_CONSTANTS_V1_H
#define CODE_MENU_HEADER_CONSTANTS_V1_H

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define MAX_SUBPAGE_DEPTH 20

static const int START_OF_CODE_MENU_HEADER = 0x804E0000;
static const int CURRENT_PAGE_PTR_LOC = START_OF_CODE_MENU_HEADER; //4
static const int MAIN_PAGE_PTR_LOC = CURRENT_PAGE_PTR_LOC + 4; //4
static const int SALTY_RUNBACK_BUTTON_COMBO_LOC = MAIN_PAGE_PTR_LOC + 4; //4
static const int SKIP_RESULTS_BUTTON_COMBO_LOC = SALTY_RUNBACK_BUTTON_COMBO_LOC + 4; //4
static const int OLD_COLOR_ARRAY = SKIP_RESULTS_BUTTON_COMBO_LOC + 4; // 0x14; This is deprecated, the color array has been moved to LINE_COLOR_TABLE!
static const int MOVE_FRAME_TIMER_LOC = OLD_COLOR_ARRAY + 0x14; //4
static const int INCREMENT_FRAME_TIMER_LOC = MOVE_FRAME_TIMER_LOC + 4; //4
static const int FRAME_ADVANCE_FRAME_TIMER = INCREMENT_FRAME_TIMER_LOC + 4; //4

static const int PREV_CODE_MENU_CONTROL_FLAG = FRAME_ADVANCE_FRAME_TIMER + 4; //4
static const int CODE_MENU_CONTROL_FLAG = PREV_CODE_MENU_CONTROL_FLAG + 4; //4
static const int INFINITE_FRIENDLIES_FLAG_LOC = CODE_MENU_CONTROL_FLAG + 4; //4
static const int AUTO_SAVE_REPLAY_FLAG_LOC = INFINITE_FRIENDLIES_FLAG_LOC + 4; //4
static const int ON_GROUP_RECORDS_FLAG_LOC = AUTO_SAVE_REPLAY_FLAG_LOC + 4; //4

static const int CODE_MENU_BUTTON_MASK_LOC = ON_GROUP_RECORDS_FLAG_LOC + 4; //4
static const int BUTTON_ACTIVATOR_MASK_LOC = CODE_MENU_BUTTON_MASK_LOC + 4; //4
static const int MAIN_BUTTON_MASK_LOC = BUTTON_ACTIVATOR_MASK_LOC + 4; //4 * 8

static const int OLD_DEBUG_STATE_LOC = MAIN_BUTTON_MASK_LOC + 4 * 8; //4
static const int OLD_CAMERA_LOCK_STATE_LOC = OLD_DEBUG_STATE_LOC + 4; //4

static const int OLD_CAMERA_POS_LOC = OLD_CAMERA_LOCK_STATE_LOC + 4; //4

static const int SAVE_STATE_BUFFER_PTR_LOC = OLD_CAMERA_POS_LOC + 4; //4
static const int SAVE_STATE_ARTICLE_LIST_PTR_LOC = SAVE_STATE_BUFFER_PTR_LOC + 4; //4
static const int SAVE_STATE_ARTICLE_ID_LIST_PTR_LOC = SAVE_STATE_ARTICLE_LIST_PTR_LOC + 4; //4
static const int SAVE_STATE_ARTICLE_SAVED_RESOURCE_LIST_PTR_LOC = SAVE_STATE_ARTICLE_ID_LIST_PTR_LOC + 4; //4
static const int SAVE_STATE_LOCATIONS_TO_UPDATE_PTR_LOC = SAVE_STATE_ARTICLE_SAVED_RESOURCE_LIST_PTR_LOC + 4; //4
static const int SAVE_STATE_LOCATIONS_TO_CLEAR_PTR_LOC = SAVE_STATE_LOCATIONS_TO_UPDATE_PTR_LOC + 4; //4
static const int SAVE_STATE_SAVED_ARTICLE_LIST_PTR_LOC = SAVE_STATE_LOCATIONS_TO_CLEAR_PTR_LOC + 4; //4

static const int RESET_LINES_STACK_LOC = SAVE_STATE_SAVED_ARTICLE_LIST_PTR_LOC + 4; // 4 * MAX_SUBPAGE_DEPTH + 8

static const int CHARACTER_SWITCHER_ARRAY_LOC = RESET_LINES_STACK_LOC + 4 * MAX_SUBPAGE_DEPTH + 8; //0x10
static const int INIFINITE_SHIELDS_ARRAY_LOC = CHARACTER_SWITCHER_ARRAY_LOC + 0x10; //0x10
static const int PERCENT_SELCTION_VALUE_ARRAY_LOC = INIFINITE_SHIELDS_ARRAY_LOC + 0x10; //0x10
static const int PERCENT_SELCTION_ACTIVATOR_ARRAY_LOC = PERCENT_SELCTION_VALUE_ARRAY_LOC + 0x10; //0x10
static const int DISABLE_DPAD_ACTIVATOR_ARRAY_LOC = PERCENT_SELCTION_ACTIVATOR_ARRAY_LOC + 0x10; //0x10

static const int ENDLESS_ROTATION_QUEUE_LOC = DISABLE_DPAD_ACTIVATOR_ARRAY_LOC + 0x10; //8
static const int ENDLESS_ROTATION_PLACEMENT_LIST_LOC = ENDLESS_ROTATION_QUEUE_LOC + 8; //4 * 4
static const int ENDLESS_ROTATION_COMP_FUNC_LOC = ENDLESS_ROTATION_PLACEMENT_LIST_LOC + 4 * 4; //4 * 4

static const int REPLAY_NTE_DATA_BUFFER_LOC = ENDLESS_ROTATION_COMP_FUNC_LOC + 4 * 4; //0x14
static const int REPLAY_CREATE_SECTION_BUFFER_LOC = REPLAY_NTE_DATA_BUFFER_LOC + 0x14; //8
static const int REPLAY_CRYPTO_BUFFER_LOC = REPLAY_CREATE_SECTION_BUFFER_LOC + 8; //0x30

static const int CODE_MENU_WIIMOTE_CONVERSION_TABLE_LOC = REPLAY_CRYPTO_BUFFER_LOC + 0x30; //0x10 * 3

static const int P1_TAG_HEX_LOC = CODE_MENU_WIIMOTE_CONVERSION_TABLE_LOC + 0x10 * 3; //0x18
static const int P2_TAG_HEX_LOC = P1_TAG_HEX_LOC + 0x18; //0x18
static const int P3_TAG_HEX_LOC = P2_TAG_HEX_LOC + 0x18; //0x18
static const int P4_TAG_HEX_LOC = P3_TAG_HEX_LOC + 0x18; //0x18

static const int P1_STOP_LOAD_FLAG_PTR_LOC = P4_TAG_HEX_LOC + 0x18; //4
static const int P2_STOP_LOAD_FLAG_PTR_LOC = P1_STOP_LOAD_FLAG_PTR_LOC + 4; //4
static const int P3_STOP_LOAD_FLAG_PTR_LOC = P2_STOP_LOAD_FLAG_PTR_LOC + 4; //4
static const int P4_STOP_LOAD_FLAG_PTR_LOC = P3_STOP_LOAD_FLAG_PTR_LOC + 4; //4

static const int RANDOM_ALTS_RNG = P4_STOP_LOAD_FLAG_PTR_LOC + 4; //4
static const int RANDOM_ALTS_MATCH_START_FLAG = RANDOM_ALTS_RNG + 4; //4

static const int TEAM_SETTINGS_LOC = RANDOM_ALTS_MATCH_START_FLAG + 4; //4
static const int TAG_LOAD_FLAGS_LOC = TEAM_SETTINGS_LOC + 4; //4

static const int PREV_TAG_COSTUMES_SETTING_LOC = TAG_LOAD_FLAGS_LOC + 4; //4

static const int DOLPHIN_MOUNT_VF_LOC = PREV_TAG_COSTUMES_SETTING_LOC + 4; //4

static const int CODE_MENU_OLD_CAMERA_MATRIX_LOC = DOLPHIN_MOUNT_VF_LOC + 4; //4 * 12 = 0x30
static const int CODE_MENU_NEED_TO_SAVE_CAMERA_MATRIX_FLAG_LOC = CODE_MENU_OLD_CAMERA_MATRIX_LOC + 0x30; //4

static const int SHOULD_DISPLAY_HUD_FLAG_LOC = CODE_MENU_NEED_TO_SAVE_CAMERA_MATRIX_FLAG_LOC + 4; //4

static const int SHOULD_RESET_HITBOX_DISPLAY_FLAG_LOC = SHOULD_DISPLAY_HUD_FLAG_LOC + 4; //4
static const int SHOULD_RESET_STAGE_COLLISIONS_FLAG_LOC = SHOULD_RESET_HITBOX_DISPLAY_FLAG_LOC + 4; //4

static const int ALC_P1_LOC = SHOULD_RESET_STAGE_COLLISIONS_FLAG_LOC + 4; //4
static const int ALC_P2_LOC = ALC_P1_LOC + 4; //4
static const int ALC_P3_LOC = ALC_P2_LOC + 4; //4
static const int ALC_P4_LOC = ALC_P3_LOC + 4; //4

static const int BIG_HEAD_LOC = ALC_P4_LOC + 4; //4

static const int RANDOM_ANGLE_LOC = BIG_HEAD_LOC + 4; //4

static const int WAR_MODE_LOC = RANDOM_ANGLE_LOC + 4; //4

static const int BUFFER_P1_LOC = WAR_MODE_LOC + 4; //4
static const int BUFFER_P2_LOC = BUFFER_P1_LOC + 4; //4
static const int BUFFER_P3_LOC = BUFFER_P2_LOC + 4; //4
static const int BUFFER_P4_LOC = BUFFER_P3_LOC + 4; //4

static const int SCALE_LOC = BUFFER_P4_LOC + 4; //4

static const int SPEED_LOC = SCALE_LOC + 4; //4

static const int CSS_VER_LOC = SPEED_LOC + 4; //4

static const int THEME_LOC = CSS_VER_LOC + 4; //4

static const int DASH_ATTACK_ITEM_GRAB_LOC = THEME_LOC + 4; //4

static const int TRIP_TOGGLE_LOC = DASH_ATTACK_ITEM_GRAB_LOC + 4; //4
static const int TRIP_RATE_MULTIPLIER_LOC = TRIP_TOGGLE_LOC + 4; //4
static const int TRIP_INTERVAL_LOC = TRIP_RATE_MULTIPLIER_LOC + 4; //4

static const int BACKPLATE_COLOR_1_LOC = TRIP_INTERVAL_LOC + 4; //4
static const int BACKPLATE_COLOR_2_LOC = BACKPLATE_COLOR_1_LOC + 4; //4
static const int BACKPLATE_COLOR_3_LOC = BACKPLATE_COLOR_2_LOC + 4; //4
static const int BACKPLATE_COLOR_4_LOC = BACKPLATE_COLOR_3_LOC + 4; //4
static const int BACKPLATE_COLOR_C_LOC = BACKPLATE_COLOR_4_LOC + 4; //4
static const int BACKPLATE_COLOR_T_LOC = BACKPLATE_COLOR_C_LOC + 4; //4
static const int BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC = BACKPLATE_COLOR_T_LOC + 4; //4

static const int JUMPSQUAT_OVERRIDE_TOGGLE_LOC = BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC + 4; // 4
static const int JUMPSQUAT_OVERRIDE_FRAMES_LOC = JUMPSQUAT_OVERRIDE_TOGGLE_LOC + 4; // 4
static const int JUMPSQUAT_OVERRIDE_MIN_LOC = JUMPSQUAT_OVERRIDE_FRAMES_LOC + 4; // 4
static const int JUMPSQUAT_OVERRIDE_MAX_LOC = JUMPSQUAT_OVERRIDE_MIN_LOC + 4; // 4

static const int DRAW_SETTINGS_BUFFER_LOC = JUMPSQUAT_OVERRIDE_MAX_LOC + 4; //0x200

// Relocated Mem2 Constants, Old Range was 0x935CE300 - 0x935CE584
static const int MEM2_CONSTANTS_START = DRAW_SETTINGS_BUFFER_LOC + 0x200;
static const int MENU_BLOCK_PTRS = MEM2_CONSTANTS_START;
static const int MENU_BUTTON_STRING_LOC = MENU_BLOCK_PTRS + 4 * 4; // 0x60
static const int MENU_CONTROL_STRING_LOC = MENU_BUTTON_STRING_LOC + 0x60; // 0x60
static const int TAG_IN_USE_LOC = MENU_CONTROL_STRING_LOC + 0x60; // 4
static const int REPLACE_NAME_OLD_TIME_LOC = TAG_IN_USE_LOC + 4; // 4
static const int REPLACE_NAME_TIME_ADDRESS = REPLACE_NAME_OLD_TIME_LOC + 4; // 4
static const int DISABLE_DPAD_ASL_STORAGE = REPLACE_NAME_TIME_ADDRESS + 4; //4
static const int GCC_BUTTON_STORAGE_LOC = DISABLE_DPAD_ASL_STORAGE + 4; //8
static const int WIIMOTE_CONVERTED_BUTTON_STORAGE_LOC = GCC_BUTTON_STORAGE_LOC + 8; //8
static const int WIIMOTE_CONVERSION_TABLE = WIIMOTE_CONVERTED_BUTTON_STORAGE_LOC + 8; //16
static const int WIICHUCK_CONVERSION_TABLE = WIIMOTE_CONVERSION_TABLE + 16; //16
static const int CLASSIC_CONVERSION_TABLE = WIICHUCK_CONVERSION_TABLE + 16; //16
static const int KAPPA_ITEM_FLAG = CLASSIC_CONVERSION_TABLE + 16; //4
//4
// In-Game Value currently is 0x817ae800
// Heap 42 : MenuInstance: 0.48MB Used  0.00MB (  0%) adr S 81734d60 E 817ae860 SIZE 00079b00
// This makes sense actually, looks like it's Allocated in SetupCharacterBuffer() and stored here for later.
// The values at this location seem to be related to the character heaps. In a 2 player match
// Values: 81273184, 8179f840, 812bb5e4, 8179b5c0, 813006c4, 81797340, 81356a44, 817930c0
// Heap 27 : Fighter1Instance: 0.32MB Used  0.00MB(0 %) adr S 8123ab60 E 8128cb60 SIZE 00052000
// Heap 28 : Fighter2Instance: 0.32MB Used  0.00MB(0 %) adr S 8128cb60 E 812deb60 SIZE 00052000
// Heap 29 : Fighter3Instance: 0.32MB Used  0.00MB(0 %) adr S 812deb60 E 81330b60 SIZE 00052000 
// Heap 30 : Fighter4Instance: 0.32MB Used  0.00MB(0 %) adr S 81330b60 E 81382b60 SIZE 00052000
// Every 1st address is in one of the FighterInstance Heaps, every 2nd is in MenuInstance
// Those MenuInstance addresses seem consistent as well, actually.
static const int MAIN_BUFFER_PTR = KAPPA_ITEM_FLAG + 4;
static const int STRING_BUFFER = MAIN_BUFFER_PTR + 4; //0x100
static const int IASA_OVERLAY_MEM_PTR_LOC = STRING_BUFFER + 0x100; //4
static const int IASA_TRIGGER_OVERLAY_COMMAND_PTR_LOC = IASA_OVERLAY_MEM_PTR_LOC + 4; //4
static const int IASA_TERMINATE_OVERLAY_COMMAND_PTR_LOC = IASA_TRIGGER_OVERLAY_COMMAND_PTR_LOC + 4; //4
static const int IASA_STATE = IASA_TERMINATE_OVERLAY_COMMAND_PTR_LOC + 4; //4
static const int IS_IN_GAME_FLAG = IASA_STATE + 4; //4
static const int WRITE_SD_FILE_HEADER_LOC = IS_IN_GAME_FLAG + 4; //0x18
static const int ACTIVE_TAG_ID_BY_PORT = WRITE_SD_FILE_HEADER_LOC + 0x18; //4
static const int HEX_TO_ASCII_TABLE = ACTIVE_TAG_ID_BY_PORT + 0x4; //0x10
static const int COSTUME_PATH_ADDRESS_RESULT = HEX_TO_ASCII_TABLE + 0x10; //4
static const int CSTICK_TAUNT_SPECIAL_WORDS = (COSTUME_PATH_ADDRESS_RESULT + 4); // 0x18
static const int MEM2_CONSTANTS_END = CSTICK_TAUNT_SPECIAL_WORDS + 0x18;
static const int MEM2_CONSTANTS_LENGTH = MEM2_CONSTANTS_END - MEM2_CONSTANTS_START;

// Line Color Expansion
static struct
{
	enum COLORS
	{
		COLOR_WHITE,
		COLOR_YELLOW,
		COLOR_TEAL,
		COLOR_BLUE,
		COLOR_GREEN,
		COLOR_RED,
		COLOR_ORANGE,
		COLOR_PURPLE,
		COLOR_PINK,
		COLOR_GRAY,
		COLOR_LIGHT_GRAY,
		COLOR_BLACK,
		__COLOR_COUNT
	};

	constexpr unsigned int table_start() { return MEM2_CONSTANTS_END; };

	// See getArray() function for color value definitions!
	std::array<int, __COLOR_COUNT> COLORS = getArray();

	constexpr unsigned int table_size() { return COLORS.size() * 4; };
	constexpr unsigned int table_end() { return table_start() + table_size(); };

	constexpr unsigned int offset(enum COLORS colorIn) { return colorIn * 4; };

private:
	constexpr std::array<int, __COLOR_COUNT> getArray()
	{
		std::array<int, __COLOR_COUNT> result{};
		result[COLORS::COLOR_WHITE]         = 0xFFFFFFFF;
		result[COLORS::COLOR_YELLOW]        = 0xFFFF00FF;
		result[COLORS::COLOR_TEAL]          = 0x6DD0FFFF;
		result[COLORS::COLOR_BLUE]          = 0x0066FFFF;
		result[COLORS::COLOR_GREEN]         = 0x33CC33FF;
		result[COLORS::COLOR_RED]           = 0xFF0000FF;
		result[COLORS::COLOR_ORANGE]        = 0xFF9900FF;
		result[COLORS::COLOR_PURPLE]        = 0x6E0094FF;
		result[COLORS::COLOR_PINK]          = 0xFF60C0FF;
		result[COLORS::COLOR_GRAY]          = 0xFFFFFF80;
		result[COLORS::COLOR_LIGHT_GRAY]    = 0xFFFFFFB0;
		result[COLORS::COLOR_BLACK]         = 0x000000FF;
		return result;
	}
} LINE_COLOR_TABLE;
static const u8 NORMAL_LINE_COLOR_OFFSET = LINE_COLOR_TABLE.offset(LINE_COLOR_TABLE.COLOR_WHITE);
static const u8 HIGHLIGHTED_LINE_COLOR_OFFSET = LINE_COLOR_TABLE.offset(LINE_COLOR_TABLE.COLOR_YELLOW);
static const u8 CHANGED_LINE_COLOR_OFFSET = LINE_COLOR_TABLE.offset(LINE_COLOR_TABLE.COLOR_TEAL);
static const u8 CHANGED_AND_HIGHLIGHTED_LINE_COLOR_OFFSET = LINE_COLOR_TABLE.offset(LINE_COLOR_TABLE.COLOR_BLUE);
static const u8 COMMENT_LINE_COLOR_OFFSET = LINE_COLOR_TABLE.offset(LINE_COLOR_TABLE.COLOR_GREEN);
static const u8 UNSELECTABLE_LINE_COLOR_OFFSET = LINE_COLOR_TABLE.offset(LINE_COLOR_TABLE.COLOR_LIGHT_GRAY);

// Heap IDs, courtesy of SammiHusky
// See: https://github.com/Sammi-Husky/BrawlHeaders/blob/main/Brawl/Include/memory.h
enum HeapType {
	SystemFW = 0x1,
	System = 0x2,
	Effect = 0x3,
	RenderFifo = 0x4,
	Sound = 0x5,
	Network = 0x6,
	WiiPad = 0x7,
	IteamResource = 0x8,
	InfoResource = 0x9,
	CommonResource = 0xa,
	Replay = 0xb,
	Tmp = 0xc,
	Physics = 0xd,
	ItemInstance = 0xe,
	StageInstance = 0xf,
	StageCommonResource = 0x10,
	StageResource = 0x11,
	Fighter1Resource = 0x12,
	Fighter2Resource = 0x13,
	Fighter3Resource = 0x14,
	Fighter4Resource = 0x15,
	Fighter1Resource2 = 0x16,
	Fighter2Resource2 = 0x17,
	Fighter3Resource2 = 0x18,
	Fighter4Resource2 = 0x19,
	FighterEffect = 0x1a,
	Fighter1Instance = 0x1b,
	Fighter2Instance = 0x1c,
	Fighter3Instance = 0x1d,
	Fighter4Instance = 0x1e,
	FighterTechqniq = 0x1f,
	FighterKirbyResource1 = 0x20,
	FighterKirbyResource2 = 0x21,
	FighterKirbyResource3 = 0x22,
	AssistFigureResource = 0x23,
	ItemExtraResource = 0x24,
	EnemyInstance = 0x25,
	PokemonResource = 0x26,
	WeaponInstance = 0x27,
	InfoInstance = 0x28,
	InfoExtraResource = 0x29,
	MenuInstance = 0x2a,
	MenuResource = 0x2b,
	CopyFB = 0x2c,
	GameGlobal = 0x2d,
	GlobalMode = 0x2e,
	MeleeFont = 0x30,
	OverlayCommon = 0x32,
	OverlayStage = 0x33,
	OverlayMenu = 0x34,
	OverlayFighter1 = 0x35,
	OverlayFighter2 = 0x36,
	OverlayFighter3 = 0x37,
	OverlayFighter4 = 0x38,
	OverlayEnemy = 0x39,
	Thread = 0x3a,
};
static struct _heapCacheTable
{
	enum CachedHeaps
	{
		CACHED_REPLAY_HEAP = 0,
		__CACHED_COUNT
	};
private:
	static constexpr unsigned int idArraySize = CachedHeaps::__CACHED_COUNT + (4 - CachedHeaps::__CACHED_COUNT % 4);
public:

	constexpr unsigned int table_start() { return LINE_COLOR_TABLE.table_end(); }

	const std::array<int, CachedHeaps::__CACHED_COUNT> addressArray{};
	const std::array<unsigned char, idArraySize> idArray = getArray();

	constexpr unsigned int address_array_size() { return addressArray.size() * 4; }
	constexpr unsigned int address_array_offset() { return 0; }
	constexpr unsigned int id_array_size() { return idArray.size(); }
	constexpr unsigned int id_array_offset() { return address_array_size(); }

	constexpr unsigned int table_size() { return address_array_size() + id_array_size(); }
	constexpr unsigned int table_end() { return table_start() + table_size(); }

	constexpr unsigned int address_offset(unsigned int heapIn) { return heapIn * 4; }
	constexpr unsigned int header_relative_address_offset(unsigned int heapIn) { return table_start() + address_offset(heapIn); }

private:
	constexpr std::array<unsigned char, idArraySize> getArray()
	{
		std::array<unsigned char, idArraySize> result{};
		result[CachedHeaps::CACHED_REPLAY_HEAP] = HeapType::Replay;
		return result;
	}
} HEAP_ADDRESS_TABLE;

static const int END_OF_CODE_MENU_HEADER = HEAP_ADDRESS_TABLE.table_end();

#endif