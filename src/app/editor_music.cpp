/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_music.h"
#include "editor_music.h"
#include "theme.h"
#include "workspace.h"
#include "resource/inline_resource.h"
#include "../compiler/compiler.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_MUSIC_UNLOAD_SYMBOLS_ON_FINISH
#	define EDITOR_MUSIC_UNLOAD_SYMBOLS_ON_FINISH 1
#endif /* EDITOR_MUSIC_UNLOAD_SYMBOLS_ON_FINISH */

static constexpr const char* const NOTE_NAMES[] = {
	"C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
	"C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
	"C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
	"C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
	"C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7",
	"C-8", "C#8", "D-8", "D#8", "E-8", "F-8", "F#8", "G-8", "G#8", "A-8", "A#8", "B-8",
	"LAST-NOTE",
	"n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a",
	"---"
};
static constexpr const char* const OCTAVE_NAMES[] = {
	"3rd Octave",
	"4th Octave",
	"5th Octave",
	"6th Octave",
	"7th Octave",
	"8th Octave"
};
static constexpr const char* const INSTRUMENT_NAMES[] = {
	"--", "01", "02", "03", "04", "05", "06", "07",
	"08", "09", "10", "11", "12", "13", "14", "15"
};
static constexpr const char* const BYTE_NAMES[] = {
	"00 - 0F", "10 - 1F", "20 - 2F", "30 - 3F", "40 - 4F", "50 - 5F", "60 - 6F", "70 - 7F",
	"80 - 8F", "90 - 9F", "A0 - AF", "B0 - BF", "C0 - CF", "D0 - DF", "E0 - EF", "F0 - FF",
};
static constexpr const char* const LINE_NUMBER_NAMES[] = {
	"00", "01", "02", "03", "04", "05", "06", "07",
	"08", "09", "10", "11", "12", "13", "14", "15",
	"16", "17", "18", "19", "20", "21", "22", "23",
	"24", "25", "26", "27", "28", "29", "30", "31",
	"32", "33", "34", "35", "36", "37", "38", "39",
	"40", "41", "42", "43", "44", "45", "46", "47",
	"48", "49", "50", "51", "52", "53", "54", "55",
	"56", "57", "58", "59", "60", "61", "62", "63"
};

static const ImVec4 ROW_COLORS[] = {
	ImVec4(1.00f, 1.00f, 1.00f, 0.05f),
	ImVec4(1.00f, 1.00f, 1.00f, 0.00f),
	ImVec4(1.00f, 1.00f, 1.00f, 0.00f),
	ImVec4(1.00f, 1.00f, 1.00f, 0.00f)
};
static const ImVec4 ROW_HOVERED_COLORS[] = {
	ImVec4(1.00f, 1.00f, 1.00f, 0.10f),
	ImVec4(1.00f, 1.00f, 1.00f, 0.10f),
	ImVec4(1.00f, 1.00f, 1.00f, 0.10f),
	ImVec4(1.00f, 1.00f, 1.00f, 0.10f)
};
static const ImVec4 ROW_ACTIVE_COLORS[] = {
	ImVec4(0.90f, 0.90f, 0.90f, 0.10f),
	ImVec4(0.90f, 0.90f, 0.90f, 0.10f),
	ImVec4(0.90f, 0.90f, 0.90f, 0.10f),
	ImVec4(0.90f, 0.90f, 0.90f, 0.10f)
};

/* ===========================================================================} */

/*
** {===========================================================================
** Music editor
*/

class EditorMusicImpl : public EditorMusic {
private:
	typedef std::function<bool(const Tracker::Cursor &, int &)> DataGetter;
	typedef std::function<bool(const Tracker::Cursor &, const int &)> DataSetter;

	typedef std::vector<const char*> TextPointers;

private:
	bool _opened = false;

	Project* _project = nullptr; // Foreign.
	int _index = -1;
	CommandQueue* _commands = nullptr;

	Tracker::Range _selection;
	struct {
		Tracker::ViewTypes index = Tracker::ViewTypes::TRACKER;
		Tracker::ViewTypes target = Tracker::ViewTypes::TRACKER;
	} _view;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _page;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _status;
	float _statusWidth = 0.0f;
	struct {
		Text::Array warnings;
		std::string text;

		bool empty(void) const {
			return warnings.empty();
		}
		void clear(void) {
			warnings.clear();
			text.clear();
		}
		bool add(const std::string &txt) {
			if (std::find(warnings.begin(), warnings.end(), txt) != warnings.end())
				return false;

			warnings.push_back(txt);

			flush();

			return true;
		}
		bool remove(const std::string &txt) {
			Text::Array::iterator it = std::find(warnings.begin(), warnings.end(), txt);
			if (it == warnings.end())
				return false;

			warnings.erase(it);

			flush();

			return true;
		}
		void flush(void) {
			text.clear();
			for (int i = 0; i < (int)warnings.size(); ++i) {
				const std::string &w = warnings[i];
				text += "* ";
				text += w;
				if (i != (int)warnings.size() - 1)
					text += "\n";
			}
		}
	} _warnings;
	std::function<void(const Command*, bool, bool)> _refresh = nullptr;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _estimated;
	struct {
		DataGetter getData = nullptr;
		DataSetter setData = nullptr;

		bool empty(void) const {
			return !getData || !setData;
		}
		void clear(void) {
			getData = nullptr;
			setData = nullptr;
		}
		void fill(DataGetter getData_, DataSetter setData_) {
			getData = getData_;
			setData = setData_;
		}
	} _binding;

	struct Ref : public Editor::Ref {
		void clear(void) {
			// Do nothing.
		}
	} _ref;
	std::function<void(int)> _setView = nullptr;
	std::function<void(int, int, int)> _setViewDetail = nullptr;
	std::function<void(Music::Instruments, int, int)> _setInstrument = nullptr;
	std::function<void(int)> _setWave = nullptr;
	std::function<int(void)> _getWave = nullptr;
	struct Tools {
		bool orderEditingInitialized = false;
		bool orderEditingFocused = false;
		bool editingOrders = false;
		Tracker::Location ordering;
		Tracker::Location target = Tracker::Location::INVALID();
		Tracker::Cursor active = Tracker::Cursor(0, 0, 0, Tracker::DataTypes::NOTE);
		Editing::NumberStroke instrumentNumberStroke = Editing::NumberStroke(Editing::NumberStroke::Bases::DEC, 0, 0, GBBASIC_MUSIC_INSTRUMENT_COUNT);
		Editing::NumberStroke effectCodeNumberStroke = Editing::NumberStroke(Editing::NumberStroke::Bases::HEX, 0, 0, GBBASIC_MUSIC_EFFECT_COUNT - 1);
		Editing::NumberStroke effectParametersNumberStroke = Editing::NumberStroke(Editing::NumberStroke::Bases::HEX, 0, 0, std::numeric_limits<Byte>::max());
		int magnification = -1;
		int ticksPerRow = 3;
		std::string name;
		Music::Channels channels;
		int instrumentIndexForTrackerView = -1;
		int instrumentIndexForPianoView = -1;
		Music::DutyInstrument dutyInstrument;
		Music::WaveInstrument waveInstrument;
		Music::NoiseInstrument noiseInstrument;
		std::array<int, GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT> instrumentWaveform;
		bool instrumentWaveformMouseDown = false;
		Music::NoiseInstrument::Macros instrumentMacros;
		bool instrumentMacrosMouseDown = false;
		int waveformCursor = 0;
		std::array<int, GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT> waveform;
		std::array<int, GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT> newWaveform;
		std::string waveformString;
		std::string newWaveformString;
		ImVec2 waveformMousePosition = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
		bool waveformMouseDown = false;

		bool editAreaFocused = false;
		bool viewPreferenceSet = false;
		ImVec2 safeDataCharacterSize = ImVec2(0, 0);
		bool definitionUnfolded = false;
		float definitionHeight = 0;
		Editing::Tools::Album definitionShadow;
		bool instrumentsUnfolded = false;
		float instrumentsHeight = 0;
		std::string toImportText;
		std::string toImportType;
		int popupCount = 0;

		std::function<void(const Tracker::Cursor*)> playMusic = nullptr;
		std::function<void(void)> stopPlaying = nullptr;
		bool isPlaying = false;
		Music::Ptr playingMusic = nullptr;
		double playerStopDelay = 0.0;
		double playerMaxDuration = 0.0;
		double playerPlayedTicks = 0.0;
		bool isPlayerConfigLoaded = false;
		std::string playerConfigText;
		std::string playerSymbolsText;
		std::string playerAliasesText;
		Editing::SymbolLocation playerIsPlayingLocation;
		Editing::SymbolLocation playerOrderCursorLocation;
		Editing::SymbolLocation playerLineCursorLocation;
		Editing::SymbolLocation playerTicksLocation;
		bool hasPlayerPlayed = false;
		int lastPlayingOrderIndex = -1;
		int lastPlayingLine = -1;
		int lastPlayingTicks = -1;
		int maxPlayingOrderIndex = -1;
		int maxPlayingLine = -1;
		int maxPlayingTicks = -1;
		int targetPlayingOrderIndex = -1;
		int targetPlayingLine = -1;

		Tools() {
			instrumentWaveform.fill(0);
			instrumentMacros.resize(15);
			waveform.fill(0);
			newWaveform.fill(0);
		}

		void clear(void) {
			orderEditingInitialized = false;
			orderEditingFocused = false;
			editingOrders = false;
			ordering = Tracker::Location();
			target = Tracker::Location::INVALID();
			active = Tracker::Cursor(0, 0, 0, Tracker::DataTypes::NOTE);
			instrumentNumberStroke = Editing::NumberStroke(Editing::NumberStroke::Bases::DEC, 0, 0, GBBASIC_MUSIC_INSTRUMENT_COUNT);
			effectCodeNumberStroke = Editing::NumberStroke(Editing::NumberStroke::Bases::HEX, 0, 0, GBBASIC_MUSIC_EFFECT_COUNT - 1);
			effectParametersNumberStroke = Editing::NumberStroke(Editing::NumberStroke::Bases::HEX, 0, 0, std::numeric_limits<Byte>::max());
			magnification = -1;
			ticksPerRow = 3;
			name.clear();
			channels = Music::Channels();
			instrumentIndexForTrackerView = -1;
			instrumentIndexForPianoView = -1;
			dutyInstrument = Music::DutyInstrument();
			waveInstrument = Music::WaveInstrument();
			noiseInstrument = Music::NoiseInstrument();
			instrumentWaveform.fill(0);
			instrumentWaveformMouseDown = false;
			instrumentMacros.resize(15);
			instrumentMacrosMouseDown = false;
			waveformCursor = 0;
			waveform.fill(0);
			newWaveform.fill(0);
			waveformString.clear();
			newWaveformString.clear();
			waveformMousePosition = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
			waveformMouseDown = false;

			editAreaFocused = false;
			viewPreferenceSet = false;
			safeDataCharacterSize = ImVec2(0, 0);
			definitionUnfolded = false;
			definitionHeight = 0;
			definitionShadow = Editing::Tools::Album();
			instrumentsUnfolded = false;
			instrumentsHeight = 0;
			toImportText.clear();
			toImportType.clear();
			popupCount = 0;

			playMusic = nullptr;
			stopPlaying = nullptr;
			isPlaying = false;
			playerStopDelay = 0.0;
			playerMaxDuration = 0.0;
			playerPlayedTicks = 0.0;
			playingMusic = nullptr;
			isPlayerConfigLoaded = false;
			playerConfigText.clear();
			playerSymbolsText.clear();
			playerAliasesText.clear();
			playerIsPlayingLocation = Editing::SymbolLocation();
			playerOrderCursorLocation = Editing::SymbolLocation();
			playerLineCursorLocation = Editing::SymbolLocation();
			playerTicksLocation = Editing::SymbolLocation();
			hasPlayerPlayed = false;
			lastPlayingOrderIndex = -1;
			lastPlayingLine = -1;
			lastPlayingTicks = -1;
			maxPlayingOrderIndex = -1;
			maxPlayingLine = -1;
			maxPlayingTicks = -1;
			targetPlayingOrderIndex = -1;
			targetPlayingLine = -1;
		}
	} _tools;

	const char* const* BYTE_NUMBERS = nullptr;
	const char* const* LINE_NUMBERS = nullptr;

	Text::Array INSTRUMENT_TOOLTIPS;
	const char* const* EFFECT_CODE_NAMES = nullptr;
	Text::Array EFFECT_CODE_TOOLTIPS;
	TextPointers EFFECT_CODE_TOOLTIPS_POINTERS;
	const char* const* EFFECT_PARAMETERS_NAMES = nullptr;
	Text::Array EFFECT_PARAMETERS_TOOLTIPS;

public:
	EditorMusicImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Music::SetNote>()
			->reg<Commands::Music::SetInstrument>()
			->reg<Commands::Music::SetEffectCode>()
			->reg<Commands::Music::SetEffectParameters>()
			->reg<Commands::Music::ChangeOrder>()
			->reg<Commands::Music::CutForTracker>()
			->reg<Commands::Music::PasteForTracker>()
			->reg<Commands::Music::DeleteForTracker>()
			->reg<Commands::Music::CutForInstrument>()
			->reg<Commands::Music::PasteForInstrument>()
			->reg<Commands::Music::DeleteForInstrument>()
			->reg<Commands::Music::CutForWave>()
			->reg<Commands::Music::PasteForWave>()
			->reg<Commands::Music::DeleteForWave>()
			->reg<Commands::Music::SetName>()
			->reg<Commands::Music::SetArtist>()
			->reg<Commands::Music::SetComment>()
			->reg<Commands::Music::SetTicksPerRow>()
			->reg<Commands::Music::SetDutyInstrument>()
			->reg<Commands::Music::SetWaveInstrument>()
			->reg<Commands::Music::SetNoiseInstrument>()
			->reg<Commands::Music::SetWave>()
			->reg<Commands::Music::Import>();
	}
	virtual ~EditorMusicImpl() override {
		close(_index);

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Project* project, int index, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		_project = project;
		_index = index;

		int another = -1;
		if (song()->name.empty() || !_project->canRenameMusic(_index, song()->name, &another)) {
			if (song()->name.empty()) {
				const std::string msg = Text::format(ws->theme()->warning_MusicMusicNameIsEmptyAtPage(), { Text::toString(_index) });
				warn(ws, msg, true);
			} else {
				const std::string msg = Text::format(ws->theme()->warning_MusicDuplicateMusicNameAtPages(), { song()->name, Text::toString(_index), Text::toString(another) });
				warn(ws, msg, true);
			}
			song()->name = _project->getUsableMusicName(_index); // Unique name.
			_project->hasDirtyAsset(true);
		}
		for (int i = 0; i < song()->instrumentCount(); ++i) {
			another = -1;
			Music::BaseInstrument* inst = song()->instrumentAt(i, nullptr);
			if (inst->name.empty() || !_project->canRenameMusicInstrument(_index, i, inst->name, &another)) {
				if (inst->name.empty()) {
					const std::string msg = Text::format(ws->theme()->warning_MusicMusicInstrumentNameIsEmptyAt(), { Text::toString(i) });
					warn(ws, msg, true);
				} else {
					const std::string msg = Text::format(ws->theme()->warning_MusicDuplicateMusicInstrumentNameAt(), { inst->name, Text::toString(i), Text::toString(another) });
					warn(ws, msg, true);
				}
				inst->name = _project->getUsableMusicInstrumentName(_index, i); // Unique name.
				_project->hasDirtyAsset(true);
			}
		}

		_refresh = std::bind(&EditorMusicImpl::refresh, this, ws, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

		_setView = std::bind(&EditorMusicImpl::changeView, this, wnd, rnd, ws, std::placeholders::_1);
		_setViewDetail = std::bind(&EditorMusicImpl::changeViewDetail, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		_setInstrument = std::bind(&EditorMusicImpl::changeInstrument, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		_setWave = std::bind(&EditorMusicImpl::changeWave, this, wnd, rnd, ws, std::placeholders::_1);
		_getWave = std::bind(&EditorMusicImpl::getWaveIndex, this);

		_tools.definitionShadow.artist = song()->artist;
		_tools.definitionShadow.comment = song()->comment;
		_tools.magnification = entry()->magnification;
		_tools.ticksPerRow = song()->ticksPerRow;
		_tools.name = song()->name;
		_tools.safeDataCharacterSize = safeDataCharacterSize();
		_tools.channels = song()->channels;
		_tools.playMusic = std::bind(&EditorMusicImpl::playMusic, this, ws, std::placeholders::_1);
		_tools.stopPlaying = std::bind(&EditorMusicImpl::stopMusic, this, ws);
		refreshWaveform();

		BYTE_NUMBERS = ws->theme()->generic_ByteHex();
		LINE_NUMBERS = LINE_NUMBER_NAMES;

		INSTRUMENT_TOOLTIPS = ws->theme()->tooltipAudio_MusicInstruments();
		EFFECT_CODE_NAMES = ws->theme()->generic_ByteHex();
		EFFECT_CODE_TOOLTIPS = ws->theme()->tooltipAudio_MusicEffects();
		for (int i = 0; i < (int)EFFECT_CODE_TOOLTIPS.size(); ++i)
			EFFECT_CODE_TOOLTIPS_POINTERS.push_back(EFFECT_CODE_TOOLTIPS[i].c_str());
		EFFECT_PARAMETERS_NAMES = ws->theme()->generic_ByteHex();
		EFFECT_PARAMETERS_TOOLTIPS = ws->theme()->tooltipAudio_MusicEffectParameters();

		fprintf(stdout, "Music editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Music editor closed: #%d.\n", _index);

		_tools.stopPlaying();

		_project = nullptr;
		_index = -1;

		_selection.clear();
		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;
		_estimated.clear();
		_binding.clear();

		_ref.clear();
		_setView = nullptr;
		_setViewDetail = nullptr;
		_setInstrument = nullptr;
		_setWave = nullptr;
		_getWave = nullptr;
		_tools.clear();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		// Do nothing.
	}
	virtual void leave(class Workspace* ws) override {
		stopMusic(ws);
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return false;
	}
	virtual void readonly(bool) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		return _commands->hasUnsavedChanges();
	}
	virtual void markChangesSaved(void) override {
		_commands->markChangesSaved();
	}

	virtual void copy(void) override {
		switch (_view.index) {
		case Tracker::ViewTypes::TRACKER: // Fall through.
		case Tracker::ViewTypes::PIANO:
			copyForTrackerOrPiano();

			break;
		case Tracker::ViewTypes::WAVES:
			copyForWaves();

			break;
		}
	}
	virtual void cut(void) override {
		switch (_view.index) {
		case Tracker::ViewTypes::TRACKER: // Fall through.
		case Tracker::ViewTypes::PIANO:
			cutForTrackerOrPiano();

			break;
		case Tracker::ViewTypes::WAVES:
			cutForWaves();

			break;
		}
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		switch (_view.index) {
		case Tracker::ViewTypes::TRACKER: // Fall through.
		case Tracker::ViewTypes::PIANO:
			pasteForTrackerOrPiano();

			break;
		case Tracker::ViewTypes::WAVES:
			pasteForWaves();

			break;
		}
	}
	virtual void del(bool) override {
		switch (_view.index) {
		case Tracker::ViewTypes::TRACKER: // Fall through.
		case Tracker::ViewTypes::PIANO:
			delForTrackerOrPiano();

			break;
		case Tracker::ViewTypes::WAVES:
			delForWaves();

			break;
		}
	}
	virtual bool selectable(void) const override {
		return true;
	}

	virtual const char* redoable(void) const override {
		const Command* cmd = _commands->redoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}
	virtual const char* undoable(void) const override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}

	virtual void redo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->redoable();
		if (!cmd)
			return;

		_commands->redo(object());

		_refresh(cmd, true, true);

		_project->toPollEditor(true);
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		_commands->undo(object());

		_refresh(cmd, false, true);

		_project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case SELECT_ALL:
			_selection.start(_tools.active.sequence, 0, 0, Tracker::DataTypes::NOTE);
			_selection.end(_tools.active.sequence, GBBASIC_MUSIC_CHANNEL_COUNT - 1, GBBASIC_MUSIC_PATTERN_COUNT - 1, Tracker::DataTypes::EFFECT_PARAMETERS);

			return Variant(true);
		case GET_CURSOR:
			return Variant(false);
		case SET_CURSOR:
			return Variant(false);
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
			}

			return Variant(true);
		case IMPORT: {
				const std::string txt = unpack<std::string>(argc, argv, 0, "");
				const std::string y = unpack<std::string>(argc, argv, 1, "");

				_tools.toImportText = txt;
				_tools.toImportType = y;
			}

			return Variant(true);
		default: // Do nothing.
			break;
		}

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		double delta
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		if (_binding.empty()) {
			_binding.fill(
				std::bind(&EditorMusicImpl::getData, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMusicImpl::setData, this, std::placeholders::_1, std::placeholders::_2)
			);
		}

		if (!_tools.toImportType.empty()) {
			if (_tools.toImportType == "C") {
				importFromCString(wnd, rnd, ws, _tools.toImportText);
			} else if (_tools.toImportType == "JSON") {
				importFromJsonString(wnd, rnd, ws, _tools.toImportText);
			}

			_tools.toImportText.clear();
			_tools.toImportType.clear();
		}

		updateMusic(ws, delta, true);

		shortcuts(wnd, rnd, ws);

		const float tabBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		const float headerHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		if (ImGui::BeginTabBar("@Ed")) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);

			ImGuiTabItemFlags flags = ImGuiTabItemFlags_NoTooltip;
			if ((_view.target == Tracker::ViewTypes::TRACKER && _view.index != Tracker::ViewTypes::TRACKER) || (!_tools.viewPreferenceSet && entry()->lastView != Tracker::ViewTypes::TRACKER)) {
				_tools.viewPreferenceSet = true;
				flags |= ImGuiTabItemFlags_SetSelected;
			}
			if (ImGui::BeginTabItem(ws->theme()->tabMusic_Tracker(), nullptr, flags, ws->theme()->style()->tabTextColor)) {
				if (!(_view.target != Tracker::ViewTypes::TRACKER && _view.index == Tracker::ViewTypes::TRACKER)) {
					_tools.viewPreferenceSet = true;

					_view.target = _view.index = Tracker::ViewTypes::TRACKER;

					entry()->lastView = Tracker::ViewTypes::TRACKER;

					updateAsTracker(wnd, rnd, ws, width, height, headerHeight, statusBarHeight + tabBarHeight, statusBarActived);
				}

				ImGui::EndTabItem();
			}

			flags = ImGuiTabItemFlags_NoTooltip;
			if ((_view.target == Tracker::ViewTypes::PIANO && _view.index != Tracker::ViewTypes::PIANO) || (!_tools.viewPreferenceSet && entry()->lastView == Tracker::ViewTypes::PIANO)) {
				_tools.viewPreferenceSet = true;
				flags |= ImGuiTabItemFlags_SetSelected;
			}
			if (ImGui::BeginTabItem(ws->theme()->tabMusic_Piano(), nullptr, flags, ws->theme()->style()->tabTextColor)) {
				if (!(_view.target != Tracker::ViewTypes::PIANO && _view.index == Tracker::ViewTypes::PIANO)) {
					_tools.viewPreferenceSet = true;

					_view.target = _view.index = Tracker::ViewTypes::PIANO;

					entry()->lastView = Tracker::ViewTypes::PIANO;

					updateAsPiano(wnd, rnd, ws, width, height, headerHeight, statusBarHeight + tabBarHeight, statusBarActived);
				}

				ImGui::EndTabItem();
			}

			flags = ImGuiTabItemFlags_NoTooltip;
			if (_view.target == Tracker::ViewTypes::WAVES && _view.index != Tracker::ViewTypes::WAVES) {
				_tools.viewPreferenceSet = true;
				flags |= ImGuiTabItemFlags_SetSelected;
			}
			if (ImGui::BeginTabItem(ws->theme()->tabMusic_Waves(), nullptr, flags, ws->theme()->style()->tabTextColor)) {
				if (!(_view.target != Tracker::ViewTypes::WAVES && _view.index == Tracker::ViewTypes::WAVES)) {
					_view.target = _view.index = Tracker::ViewTypes::WAVES;

					updateWaves(wnd, rnd, ws, width, height, headerHeight, statusBarHeight + tabBarHeight, statusBarActived);
				}

				ImGui::EndTabItem();
			}

			ImGui::PopStyleColor();

			ImGui::EndTabBar();
		}

		renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);
	}

	virtual void statusInvalidated(void) override {
		_page.filled = false;
		_status.filled = false;
		_estimated.filled = false;
	}

	virtual void added(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}
	virtual void removed(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}

	virtual void played(class Renderer*, class Workspace* ws) override {
		stopMusic(ws);
	}
	virtual void stopped(class Renderer*, class Workspace*) override {
		// Do nothing.
	}

	virtual void resized(class Renderer*, const Math::Vec2i &, const Math::Vec2i &) override {
		// Do nothing.
	}

	virtual void focusLost(class Renderer*) override {
		// Do nothing.
	}
	virtual void focusGained(class Renderer*) override {
		// Do nothing.
	}

private:
	void updateAsTracker(
		Window* wnd, Renderer* rnd,
		Workspace* ws,
		float /* width */, float height, float headerHeight, float paddingY,
		bool &statusBarActived
	) {
		// Prepare.
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const Ref::Splitter splitter = _ref.split();

		const int magnification = dataCharacterScale();

		shortcutsForTrackerOrPiano(wnd, rnd, ws);

		const float padding = 8.0f;
		const float dataWidth = _tools.safeDataCharacterSize.x * 8;

		// Render the header.
		const float headY = ImGui::GetCursorPosY();
		ImGui::BeginChild("@Hdr", ImVec2(splitter.first, headerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);
		{
			const ImU32 col = ws->theme()->style()->musicSideColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			const float headY_ = ImGui::GetCursorPosY();
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + padding, headY_));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->windowAudio_Duty1());
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + dataWidth + padding * 2, headY_));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->windowAudio_Duty2());
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + dataWidth * 2 + padding * 3, headY_));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->windowAudio_Wave());
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + dataWidth * 3 + padding * 4, headY_));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->windowAudio_Noise());
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();

		// Update and render the edit area for cells.
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - paddingY - headerHeight), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
		{
			// Prepare.
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

			ImGui::SetWindowFontScale((float)magnification);

			const ImVec2 wndStart = ImGui::GetWindowPos();
			const ImVec2 wndSize = ImGui::GetWindowSize();

			// Walk through the rows.
			{
				// Prepare.
				const float spaceHeight = ((height - paddingY) - _tools.safeDataCharacterSize.y) * 0.5f;
				auto getRect = [this, &dataWidth, &padding] (const ImVec2 &origin, int channel, int line, Tracker::DataTypes data) -> ImRect {
					ImVec2 min = origin;
					if (channel == 0)
						(void)min.x;
					else if (channel == 1)
						min.x += dataWidth + padding * 1;
					else if (channel == 2)
						min.x += dataWidth * 2 + padding * 2;
					else if (channel == 3)
						min.x += dataWidth * 3 + padding * 3;
					if (data == Tracker::DataTypes::NOTE)
						(void)min.x;
					else if (data == Tracker::DataTypes::INSTRUMENT)
						min.x += _tools.safeDataCharacterSize.x * 3;
					else if (data == Tracker::DataTypes::EFFECT_CODE)
						min.x += _tools.safeDataCharacterSize.x * 5;
					else if (data == Tracker::DataTypes::EFFECT_PARAMETERS)
						min.x += _tools.safeDataCharacterSize.x * 6;
					min.y += _tools.safeDataCharacterSize.y * line;

					ImVec2 max = min;
					if (data == Tracker::DataTypes::NOTE)
						max.x += _tools.safeDataCharacterSize.x * 3;
					else if (data == Tracker::DataTypes::INSTRUMENT)
						max.x += _tools.safeDataCharacterSize.x * 2;
					else if (data == Tracker::DataTypes::EFFECT_CODE)
						max.x += _tools.safeDataCharacterSize.x * 1;
					else if (data == Tracker::DataTypes::EFFECT_PARAMETERS)
						max.x += _tools.safeDataCharacterSize.x * 2;
					max.y += _tools.safeDataCharacterSize.y;

					return ImRect(min, max);
				};

				ImGuiMouseButton mouseButton = ImGuiMouseButton_Left;
				bool doubleClicked = false;

				ImVec2 cursorStart = ImVec2();
				ImVec2 cursorEnd = ImVec2();

				// Add top space to make the cursor at the middle of the window.
				ImGui::Dummy(ImVec2(1, spaceHeight));

				// Render the line numbers.
				auto beginLineSelection = [this] (int line) -> void {
					_selection.clear();
					_selection.start(_tools.active.sequence, 0, line, Tracker::DataTypes::NOTE);
				};
				auto endLineSelection = [this] (const ImVec2 &origin, int line) -> void {
					ImRect rect;
					rect.Min = origin + ImVec2(0, _tools.safeDataCharacterSize.y * line);
					rect.Max = origin + ImVec2(_tools.safeDataCharacterSize.x * 2, _tools.safeDataCharacterSize.y * (line + 1));

					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const Tracker::Cursor end(_tools.active.sequence, GBBASIC_MUSIC_CHANNEL_COUNT - 1, line, Tracker::DataTypes::EFFECT_PARAMETERS);
					if (_selection.startsWith(end)) {
						// Do nothing.
					} else {
						_selection.end(end);

						ImGuiContext &g = *ImGui::GetCurrentContext();
						g.NavId = 0;
					}
				};
				auto clearLineSelection = [this] (const ImVec2 &origin, int line) -> void {
					ImRect rect;
					rect.Min = origin + ImVec2(0, _tools.safeDataCharacterSize.y * line);
					rect.Max = origin + ImVec2(_tools.safeDataCharacterSize.x * 2, _tools.safeDataCharacterSize.y * (line + 1));

					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const bool same = _selection.endsWith(GBBASIC_MUSIC_CHANNEL_COUNT - 1, line, Tracker::DataTypes::EFFECT_PARAMETERS);
					if (same)
						return;

					_selection.clear();
				};
				auto playFromHere = [this, &mouseButton] (const ImVec2 &origin, int line) -> bool {
					ImRect rect;
					rect.Min = origin + ImVec2(0, _tools.safeDataCharacterSize.y * line);
					rect.Max = origin + ImVec2(_tools.safeDataCharacterSize.x * 2, _tools.safeDataCharacterSize.y * (line + 1));

					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return false;

					switch (mouseButton) {
					case ImGuiMouseButton_Middle:
						return true;
					default: // Do nothing.
						break;
					}

					return false;
				};

				const ImVec2 oldPos = ImGui::GetCursorScreenPos();
				ImGui::PushID("@Ln");
				for (int j = 0; j < GBBASIC_MUSIC_PATTERN_COUNT; ++j) {
					if (_tools.active.line == j)
						cursorStart = ImGui::GetCursorScreenPos();

					ImGui::PushID(j + 1);
					ImGui::PushStyleColor(ImGuiCol_Button, ROW_COLORS[j % 4]);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ROW_HOVERED_COLORS[j % 4]);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ROW_ACTIVE_COLORS[j % 4]);
					{
						const ImU32 col = ws->theme()->style()->musicSideColor;
						ImGui::PushStyleColor(ImGuiCol_Text, col);
						if (ImGui::AdjusterButton(LINE_NUMBERS[j], _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, false)) {
							if (io.KeyShift) {
								if (_selection.invalid() || _selection.first.channel != 0)
									beginLineSelection(_tools.active.line);
								endLineSelection(oldPos, j);
							} else {
								beginLineSelection(j);
							}
						}
						if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
							endLineSelection(oldPos, j);
						} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
							clearLineSelection(oldPos, j);
						}
						if (!_tools.isPlaying && playFromHere(oldPos, j)) {
							_tools.targetPlayingOrderIndex = _tools.active.sequence;
							_tools.targetPlayingLine = j;
							playMusic(ws, nullptr);
						}
						ImGui::PopStyleColor(1);
					}
					ImGui::PopStyleColor(3);
					ImGui::PopID();
				}
				ImGui::PopID();
				const ImVec2 startScreenPos = oldPos + ImVec2(_tools.safeDataCharacterSize.x * 2 + padding, 0);
				ImGui::SetCursorScreenPos(startScreenPos);

				// Iterate all the channels.
				bool openCtx = false;
				auto activated = [&mouseButton, &doubleClicked, &openCtx] (void) -> bool {
					bool active = false;
					switch (mouseButton) {
					case ImGuiMouseButton_Left:
						active = true;
						if (doubleClicked)
							openCtx = true;

						break;
					case ImGuiMouseButton_Right:
						active = true;
						openCtx = true;

						break;
					default: // Do nothing.
						break;
					}

					return active;
				};
				auto beginSelection = [this, &openCtx] (int channel, int line, Tracker::DataTypes data) -> void {
					if (openCtx || _tools.popupCount)
						return;

					_selection.clear();
					_selection.start(_tools.active.sequence, channel, line, data);
				};
				auto endSelection = [this, getRect, &openCtx] (const ImVec2 &origin, int channel, int line, Tracker::DataTypes data) -> void {
					if (_tools.popupCount)
						return;

					const ImRect rect = getRect(origin, channel, line, data);
					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const Tracker::Cursor end(_tools.active.sequence, channel, line, data);
					if (_selection.startsWith(end)) {
						// Do nothing.
					} else {
						_selection.end(end);

						ImGuiContext &g = *ImGui::GetCurrentContext();
						g.NavId = 0;
					}
				};
				auto clearSelection = [this, getRect] (const ImVec2 &origin, int channel, int line, Tracker::DataTypes data) -> void {
					if (_tools.popupCount)
						return;

					const ImRect rect = getRect(origin, channel, line, data);
					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const bool same = _selection.endsWith(channel, line, data);
					if (same)
						return;

					_selection.clear();
				};

				for (int i = 0; i < GBBASIC_MUSIC_CHANNEL_COUNT; ++i) {
					// Prepare.
					auto highlight = [this] (const char* idstr) -> void {
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						const ImGuiID id = window->GetID(idstr);
						ImGui::SetFocusID(id, window);
					};
					auto relocate = [this, highlight] (int i, int j, Tracker::DataTypes di, const char* idstr) -> void {
						if (_tools.target.channel == -1)
							return;
						if (_tools.target.channel != i)
							return;

						if (_tools.target.line == -1)
							return;
						if (_tools.target.line != j)
							return;

						if (_tools.active.data != di)
							return;

						ImGui::ScrollToItem(ImGuiScrollFlags_AlwaysCenterY);

						if (_selection.invalid())
							highlight(idstr);

						_tools.target = Tracker::Location::INVALID();
					};

					ImGui::BeginChildFrame(i + 1, ImVec2(dataWidth, _tools.safeDataCharacterSize.y * GBBASIC_MUSIC_PATTERN_COUNT), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

					// Walk through the columns.
					do {
						// Iterate all the notes.
						for (int j = 0; j < GBBASIC_MUSIC_PATTERN_COUNT; ++j) {
							// Prepare.
							const Music::Cell BLANK;
							const Music::Cell* cell_ = cell(_tools.active.sequence, i, j);
							if (!cell_)
								cell_ = &BLANK;

							// Update and render all the fields.
							const int note = cell_->note;
							const int instrument = cell_->instrument;
							const int effectCode = cell_->effectCode;
							const int effectParameters = cell_->effectParameters;

							ImGui::PushID(j + 1);
							ImGui::PushStyleColor(ImGuiCol_Button, ROW_COLORS[j % 4]);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ROW_HOVERED_COLORS[j % 4]);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ROW_ACTIVE_COLORS[j % 4]);
							ImGui::PushID("@Nt");
							{
								const bool highlight_ = _tools.active.channel == i && _tools.active.line == j && _tools.active.data == Tracker::DataTypes::NOTE;
								const ImU32 col = ws->theme()->style()->musicNoteColor;
								ImGui::PushStyleColor(ImGuiCol_Text, col);
								if (ImGui::AdjusterButton(NOTE_NAMES[note], 3, _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, highlight_)) {
									if (activated()) {
										if (io.KeyShift) {
											if (_selection.invalid())
												beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::NOTE);
											endSelection(startScreenPos, i, j, Tracker::DataTypes::NOTE);
										} else {
											beginSelection(i, j, Tracker::DataTypes::NOTE);
										}

										if (_selection.invalid()) {
											_tools.active.set(i, j, Tracker::DataTypes::NOTE);
											refreshShortcutCaches();
										}
									} else if (mouseButton == ImGuiMouseButton_Middle) {
										_selection.clear();
										_tools.active.set(i, j, Tracker::DataTypes::NOTE);
										refreshShortcutCaches();
									}

									highlight(NOTE_NAMES[note]);
								}
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::NOTE);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::NOTE);
								}
								ImGui::PopStyleColor(1);
								ImGui::SameLine();

								relocate(i, j, Tracker::DataTypes::NOTE, NOTE_NAMES[note]);
							}
							ImGui::PopID();
							ImGui::PushID("@In");
							{
								const bool highlight_ = _tools.active.channel == i && _tools.active.line == j && _tools.active.data == Tracker::DataTypes::INSTRUMENT;
								const ImU32 col = ws->theme()->style()->musicInstrumentColor;
								ImGui::PushStyleColor(ImGuiCol_Text, col);
								if (ImGui::AdjusterButton(INSTRUMENT_NAMES[instrument], 2, _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, highlight_)) {
									if (activated()) {
										if (io.KeyShift) {
											if (_selection.invalid())
												beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::INSTRUMENT);
											endSelection(startScreenPos, i, j, Tracker::DataTypes::INSTRUMENT);
										} else {
											beginSelection(i, j, Tracker::DataTypes::INSTRUMENT);
										}

										if (_selection.invalid()) {
											_tools.active.set(i, j, Tracker::DataTypes::INSTRUMENT);
											refreshShortcutCaches();
										}
									} else if (mouseButton == ImGuiMouseButton_Middle) {
										_selection.clear();
										_tools.active.set(i, j, Tracker::DataTypes::INSTRUMENT);
										refreshShortcutCaches();
									}

									highlight(INSTRUMENT_NAMES[instrument]);
								}
								if (instrument >= 0 && instrument < (int)INSTRUMENT_TOOLTIPS.size() && ImGui::IsItemHovered()) {
									VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

									ImGui::PushStyleColor(ImGuiCol_Text, ws->theme()->style()->builtin[ImGuiCol_Text]);
									ImGui::SetTooltip(INSTRUMENT_TOOLTIPS[instrument]);
									ImGui::PopStyleColor();
								}
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::INSTRUMENT);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::INSTRUMENT);
								}
								ImGui::PopStyleColor(1);
								ImGui::SameLine();

								relocate(i, j, Tracker::DataTypes::INSTRUMENT, INSTRUMENT_NAMES[instrument]);
							}
							ImGui::PopID();
							ImGui::PushID("@Fx");
							{
								char buf0[2];
								char buf1[3];
								if (effectCode == 0 && effectParameters == 0) {
									buf0[0] = '-'; buf0[1] = '\0';
									buf1[0] = '-'; buf1[1] = '-'; buf1[2] = '\0';
								} else {
									const char* ecn = EFFECT_CODE_NAMES[effectCode];
									const char* epn = EFFECT_PARAMETERS_NAMES[effectParameters];
									buf0[0] = ecn[1]; buf0[1] = '\0';
									buf1[0] = epn[0]; buf1[1] = epn[1]; buf1[2] = '\0';
								}
								bool highlight_ = _tools.active.channel == i && _tools.active.line == j && _tools.active.data == Tracker::DataTypes::EFFECT_CODE;
								const ImU32 col = ws->theme()->style()->musicEffectColor;
								ImGui::PushStyleColor(ImGuiCol_Text, col);
								if (ImGui::AdjusterButton(buf0, 1, _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, highlight_)) {
									if (activated()) {
										if (io.KeyShift) {
											if (_selection.invalid())
												beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::EFFECT_CODE);
											endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_CODE);
										} else {
											beginSelection(i, j, Tracker::DataTypes::EFFECT_CODE);
										}

										if (_selection.invalid()) {
											_tools.active.set(i, j, Tracker::DataTypes::EFFECT_CODE);
											refreshShortcutCaches();
										}
									} else if (mouseButton == ImGuiMouseButton_Middle) {
										_selection.clear();
										_tools.active.set(i, j, Tracker::DataTypes::EFFECT_CODE);
										refreshShortcutCaches();
									}

									highlight(buf0);
								}
								if (effectCode >= 0 && effectCode < (int)EFFECT_CODE_TOOLTIPS.size() && ImGui::IsItemHovered()) {
									VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

									ImGui::PushStyleColor(ImGuiCol_Text, ws->theme()->style()->builtin[ImGuiCol_Text]);
									ImGui::SetTooltip(EFFECT_CODE_TOOLTIPS[effectCode]);
									ImGui::PopStyleColor();
								}
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_CODE);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_CODE);
								}
								relocate(i, j, Tracker::DataTypes::EFFECT_CODE, buf0);
								ImGui::SameLine();
								highlight_ = _tools.active.channel == i && _tools.active.line == j && _tools.active.data == Tracker::DataTypes::EFFECT_PARAMETERS;
								if (ImGui::AdjusterButton(buf1, 2, _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, highlight_)) {
									if (activated()) {
										if (io.KeyShift) {
											if (_selection.invalid())
												beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::EFFECT_PARAMETERS);
											endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
										} else {
											beginSelection(i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
										}

										if (_selection.invalid()) {
											_tools.active.set(i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
											refreshShortcutCaches();
										}
									} else if (mouseButton == ImGuiMouseButton_Middle) {
										_selection.clear();
										_tools.active.set(i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
										refreshShortcutCaches();
									}

									highlight(buf1);
								}
								if (effectParameters >= 0 && effectParameters < (int)EFFECT_PARAMETERS_TOOLTIPS.size() && ImGui::IsItemHovered()) {
									VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

									ImGui::PushStyleColor(ImGuiCol_Text, ws->theme()->style()->builtin[ImGuiCol_Text]);
									ImGui::SetTooltip(EFFECT_PARAMETERS_TOOLTIPS[effectParameters]);
									ImGui::PopStyleColor();
								}
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
								}
								relocate(i, j, Tracker::DataTypes::EFFECT_PARAMETERS, buf1);
								ImGui::PopStyleColor(1);

								ImGui::SameLine();
								if (i == GBBASIC_MUSIC_CHANNEL_COUNT - 1 && _tools.active.line == j) {
									cursorEnd = ImGui::GetCursorScreenPos();
									cursorEnd.y += _tools.safeDataCharacterSize.y;
								}
								ImGui::NewLine();
							}
							ImGui::PopID();
							ImGui::PopStyleColor(3);
							ImGui::PopID();
						}
					} while (false);

					// Finish.
					ImGui::EndChildFrame();
					if (i != GBBASIC_MUSIC_CHANNEL_COUNT - 1) {
						ImGui::SameLine();

						ImGui::Dummy(ImVec2(padding, 1));
						ImGui::SameLine();
					}
				}

				// Update and render the context menues.
				if (openCtx) {
					switch (_tools.active.data) {
					case Tracker::DataTypes::NOTE:
						ImGui::OpenPopup("@Ctx4N");

						break;
					case Tracker::DataTypes::INSTRUMENT:
						ImGui::OpenPopup("@Ctx4I");

						break;
					case Tracker::DataTypes::EFFECT_CODE:
						ImGui::OpenPopup("@Ctx4C");

						break;
					case Tracker::DataTypes::EFFECT_PARAMETERS:
						ImGui::OpenPopup("@Ctx4P");

						break;
					default: // Do nothing.
						break;
					}
				}

				_tools.popupCount = 0;
				if (_selection.invalid()) {
					_tools.popupCount += (int)contextForTrackerOrPianoNotes(wnd, rnd, ws);
					_tools.popupCount += (int)contextForTrackerOrPianoInstruments(wnd, rnd, ws);
					_tools.popupCount += (int)contextForTrackerOrPianoEffectCodes(wnd, rnd, ws);
					_tools.popupCount += (int)contextForTrackerOrPianoEffectParameters(wnd, rnd, ws);
				} else {
					_tools.popupCount += (int)contextForTrackerOrPiano(wnd, rnd, ws);
				}

				// Render the cursors.
				drawList->PushClipRect(wndStart, wndStart + wndSize - ImVec2(style.ScrollbarSize, 0), true);
				{
					// Render the line cursor.
					if (_selection.invalid()) {
						drawList->AddRectFilled(cursorStart, cursorEnd, 0x40000000);
						drawList->AddRect(cursorStart, cursorEnd, 0x40a0a0a0);
					}

					// Render the selection cursor.
					if (!_selection.invalid()) {
						ImDrawList* drawList_ = ImGui::GetForegroundDrawList();
						drawList_->PushClipRect(wndStart, wndStart + wndSize - ImVec2(style.ScrollbarSize, 0), true);
						{
							const Tracker::Cursor minLoc = _selection.min();
							const Tracker::Cursor maxLoc = _selection.max();
							const ImRect minRect = getRect(startScreenPos, minLoc.channel, minLoc.line, minLoc.data);
							const ImRect maxRect = getRect(startScreenPos, maxLoc.channel, maxLoc.line, maxLoc.data);
							drawList->AddRect(minRect.Min - ImVec2(0.5f, 0.0f), maxRect.Max + ImVec2(0.5f, 0.0f), ImGui::GetColorU32(ImGuiCol_NavHighlight));
						}
						drawList_->PopClipRect();
					}
				}
				drawList->PopClipRect();

				// Add bottom space to make the cursor at the middle of the window.
				ImGui::Dummy(ImVec2(1, spaceHeight));
			}

			// Finish.
			refreshStatus(wnd, rnd, ws);

			const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
			_tools.editAreaFocused = focusingWnd;

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		}
		ImGui::EndChild();

		// Update and render the tools area for sequence orders.
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::SetCursorPosY(headY);
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - paddingY), true, _ref.windowFlags());
		{
			const float spwidth = _ref.windowWidth(splitter.second);
			int line = -1;
			if (
				Editing::Tools::orderable(
					rnd, ws,
					song()->channels, _tools.channels,
					spwidth,
					&_tools.orderEditingInitialized, &_tools.orderEditingFocused,
					Math::Vec2i((int)_tools.safeDataCharacterSize.x, (int)_tools.safeDataCharacterSize.y),
					&_tools.ordering.channel, &_tools.ordering.line,
					&_tools.editingOrders,
					ws->theme()->dialogPrompt_Orders().c_str(),
					BYTE_NUMBERS
				)
			) {
				if (song()->channels != _tools.channels) {
					Command* cmd = enqueue<Commands::Music::ChangeOrder>()
						->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
						->with(_tools.channels)
						->with(_setView, (int)_view.index)
						->exec(object());

					line = _tools.ordering.line;

					_refresh(cmd, true, false);
				}
			}
			if (line == -1)
				_tools.active.sequence = _tools.ordering.line;
			else
				_tools.active.sequence = _tools.ordering.line = line;
			ImGui::NewLine(1);

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::namable(
					rnd, ws,
					song()->name, _tools.name,
					spwidth,
					nullptr, nullptr,
					ws->theme()->dialogPrompt_Name().c_str()
				)
			) {
				if (_project->canRenameMusic(_index, _tools.name, nullptr)) {
					Command* cmd = enqueue<Commands::Music::SetName>()
						->with(_tools.name)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, false, false);
				} else {
					_tools.name = song()->name;

					warn(ws, ws->theme()->warning_MusicNameIsAlreadyInUse(), true);
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			int size_ = song()->ticksPerRow;
			if (
				Editing::Tools::resizable(
					rnd, ws,
					size_,
					&_tools.ticksPerRow,
					GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE,
					spwidth,
					nullptr,
					ws->theme()->dialogPrompt_TicksPerRow().c_str(),
					nullptr,
					nullptr
				)
			) {
				if (song()->ticksPerRow != _tools.ticksPerRow) {
					Command* cmd = enqueue<Commands::Music::SetTicksPerRow>()
						->with(_tools.ticksPerRow)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, true, false);
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::definable(
					rnd, ws,
					object().get(),
					&_tools.definitionShadow,
					&_tools.definitionUnfolded,
					spwidth, &_tools.definitionHeight,
					ws->theme()->dialogPrompt_Definition().c_str()
				)
			) {
				if (_tools.definitionShadow.artist != song()->artist) {
					Command* cmd = enqueue<Commands::Music::SetArtist>()
						->with(_tools.definitionShadow.artist)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, false, false);
				}
				if (_tools.definitionShadow.comment != song()->comment) {
					Command* cmd = enqueue<Commands::Music::SetComment>()
						->with(_tools.definitionShadow.comment)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, false, false);
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::instruments(
					rnd, ws,
					object().get(),
					&_tools.dutyInstrument,
					&_tools.waveInstrument,
					&_tools.noiseInstrument,
					&_tools.instrumentWaveform, &_tools.instrumentWaveformMouseDown,
					&_tools.instrumentMacros, &_tools.instrumentMacrosMouseDown,
					false,
					&_tools.instrumentsUnfolded,
					&_tools.instrumentIndexForTrackerView,
					spwidth, &_tools.instrumentsHeight,
					ws->theme()->windowAudio_Instruments().c_str(),
					[this] (Editing::Operations op) -> void {
						Music::Instruments type;
						const Music::BaseInstrument* inst = song()->instrumentAt(_tools.instrumentIndexForTrackerView, &type);
						(void)inst;
						int idx = -1;
						switch (type) {
						case Music::Instruments::SQUARE:
							idx = _tools.instrumentIndexForTrackerView;

							break;
						case Music::Instruments::WAVE:
							idx = _tools.instrumentIndexForTrackerView - (int)song()->dutyInstruments.size();

							break;
						case Music::Instruments::NOISE:
							idx = _tools.instrumentIndexForTrackerView - ((int)song()->dutyInstruments.size() + (int)song()->waveInstruments.size());

							break;
						}

						switch (op) {
						case Editing::Operations::COPY: // Copy.
							copyForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::CUT: // Cut.
							cutForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::PASTE: // Paste.
							pasteForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::RESET: // Delete.
							delForInstruments(type, _tools.active.channel, idx);

							break;
						default:
							GBBASIC_ASSERT(false && "Impossible.");

							break;
						}
					}
				)
			) {
				Music::Instruments type;
				const Music::BaseInstrument* inst = song()->instrumentAt(_tools.instrumentIndexForTrackerView, &type);
				(void)inst;
				switch (type) {
				case Music::Instruments::SQUARE: {
						Command* cmd = enqueue<Commands::Music::SetDutyInstrument>()
							->with(_tools.instrumentIndexForTrackerView, _tools.dutyInstrument)
							->with(_setView, (int)_view.index)
							->exec(object());

						_refresh(cmd, false, false);
					}

					break;
				case Music::Instruments::WAVE: {
						Command* cmd = enqueue<Commands::Music::SetWaveInstrument>()
							->with(_tools.instrumentIndexForTrackerView - (int)song()->dutyInstruments.size(), _tools.waveInstrument)
							->with(_setView, (int)_view.index)
							->exec(object());

						_refresh(cmd, false, false);
					}

					break;
				case Music::Instruments::NOISE: {
						Command* cmd = enqueue<Commands::Music::SetNoiseInstrument>()
							->with(_tools.instrumentIndexForTrackerView - ((int)song()->dutyInstruments.size() + (int)song()->waveInstruments.size()), _tools.noiseInstrument)
							->with(_setView, (int)_view.index)
							->exec(object());

						_refresh(cmd, false, false);
					}

					break;
				}
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	void updateAsPiano(
		Window* wnd, Renderer* rnd,
		Workspace* ws,
		float /* width */, float height, float headerHeight, float paddingY,
		bool &statusBarActived
	) {
		// Prepare.
		constexpr const char* const NOTES[] = {
			"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
		};
		constexpr const char* const OCTAVES[] = {
			"3rd Octave",
			"4th Octave",
			"5th Octave",
			"6th Octave",
			"7th Octave",
			"8th Octave"
		};
		constexpr const char* const CODES[] = {
			"-", "1", "2", "3", "4", "5", "6", "7",
			"8", "9", "A", "B", "C", "D", "E", "F"
		};

		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const Ref::Splitter splitter = _ref.split();

		const int magnification = dataCharacterScale();

		shortcutsForTrackerOrPiano(wnd, rnd, ws);

		const float padding = 8.0f;
		const float pianoWidth = _tools.safeDataCharacterSize.x * 12;
		const float octaveWidth = _tools.safeDataCharacterSize.x * 6;
		const float instrumentWidth = _tools.safeDataCharacterSize.x * 2;
		const float effectCodeWidth = _tools.safeDataCharacterSize.x * 16;
		const float effectParamsWidth = _tools.safeDataCharacterSize.x * 2;

		if (!_selection.invalid()) {
			if (_selection.first.channel < _selection.second.channel) {
				_selection.second.channel = _selection.first.channel;
				_selection.second.data = Tracker::DataTypes::EFFECT_PARAMETERS;
			} else if (_selection.second.channel < _selection.first.channel) {
				_selection.second.channel = _selection.first.channel;
				_selection.second.data = Tracker::DataTypes::NOTE;
			}
		}

		// Render the header.
		const float headY = ImGui::GetCursorPosY();
		ImGui::BeginChild("@Hdr", ImVec2(splitter.first, headerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);
		{
			const ImU32 col = ws->theme()->style()->musicSideColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			const float headY_ = ImGui::GetCursorPosY();
			float stepping = 0;
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + stepping + padding, headY_)); stepping += pianoWidth;
			int index = 0;
			if (ImGui::PianoButtons(pianoWidth, headerHeight, &index, (const char**)NOTES, ImGui::ColorConvertFloat4ToU32(ws->theme()->style()->builtin[ImGuiCol_Text]))) {
				const int note_ = entry()->lastOctave * 12 + index;
				Command* cmd = enqueue<Commands::Music::SetNote>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(note_)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);

				if (++_tools.active.line >= GBBASIC_MUSIC_PATTERN_COUNT)
					_tools.active.line = GBBASIC_MUSIC_PATTERN_COUNT - 1;
				active(_tools.active.channel, _tools.active.line);
			}
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + stepping + padding * 2, headY_)); stepping += octaveWidth;
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("OCTAVE");
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + stepping + padding * 3, headY_)); stepping += instrumentWidth;
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("IN");
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + stepping + padding * 4, headY_)); stepping += effectCodeWidth;
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("EFFECT");
			ImGui::SetCursorPos(ImVec2(_tools.safeDataCharacterSize.x * 2 + stepping + padding * 5, headY_)); stepping += effectParamsWidth;
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("PA");
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();

		// Update and render the edit area for cells.
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - paddingY - headerHeight), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
		{
			// Prepare.
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

			ImGui::SetWindowFontScale((float)magnification);

			const ImVec2 wndStart = ImGui::GetWindowPos();
			const ImVec2 wndSize = ImGui::GetWindowSize();

			// Walk through the rows.
			{
				// Prepare.
				const float spaceHeight = ((height - paddingY) - _tools.safeDataCharacterSize.y) * 0.5f;
				auto getRect = [this, &padding, pianoWidth, octaveWidth, instrumentWidth, effectCodeWidth, effectParamsWidth] (const ImVec2 &origin, int /* channel */, int line, Tracker::DataTypes data) -> ImRect {
					ImVec2 min = origin;
					if (data == Tracker::DataTypes::NOTE)
						(void)min.x;
					else if (data == Tracker::DataTypes::INSTRUMENT)
						min.x += pianoWidth + padding + octaveWidth + padding;
					else if (data == Tracker::DataTypes::EFFECT_CODE)
						min.x += pianoWidth + padding + octaveWidth + padding + instrumentWidth + padding;
					else if (data == Tracker::DataTypes::EFFECT_PARAMETERS)
						min.x += pianoWidth + padding + octaveWidth + padding + instrumentWidth + padding + effectCodeWidth + padding;
					min.y += _tools.safeDataCharacterSize.y * line;

					ImVec2 max = min;
					if (data == Tracker::DataTypes::NOTE)
						max.x += pianoWidth + padding + octaveWidth;
					else if (data == Tracker::DataTypes::INSTRUMENT)
						max.x += instrumentWidth;
					else if (data == Tracker::DataTypes::EFFECT_CODE)
						max.x += effectCodeWidth;
					else if (data == Tracker::DataTypes::EFFECT_PARAMETERS)
						max.x += effectParamsWidth;
					max.y += _tools.safeDataCharacterSize.y;

					return ImRect(min, max);
				};

				ImGuiMouseButton mouseButton = ImGuiMouseButton_Left;
				bool doubleClicked = false;

				ImVec2 cursorStart = ImVec2();
				ImVec2 cursorEnd = ImVec2();

				// Add top space to make the cursor at the middle of the window.
				ImGui::Dummy(ImVec2(1, spaceHeight));

				// Render the line numbers.
				auto beginLineSelection = [this] (int line) -> void {
					_selection.clear();
					_selection.start(_tools.active.sequence, _tools.active.channel, line, Tracker::DataTypes::NOTE);
				};
				auto endLineSelection = [this] (const ImVec2 &origin, int line) -> void {
					ImRect rect;
					rect.Min = origin + ImVec2(0, _tools.safeDataCharacterSize.y * line);
					rect.Max = origin + ImVec2(_tools.safeDataCharacterSize.x * 2, _tools.safeDataCharacterSize.y * (line + 1));

					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const Tracker::Cursor end(_tools.active.sequence, _tools.active.channel, line, Tracker::DataTypes::EFFECT_PARAMETERS);
					if (_selection.startsWith(end)) {
						// Do nothing.
					} else {
						_selection.end(end);

						ImGuiContext &g = *ImGui::GetCurrentContext();
						g.NavId = 0;
					}
				};
				auto clearLineSelection = [this] (const ImVec2 &origin, int line) -> void {
					ImRect rect;
					rect.Min = origin + ImVec2(0, _tools.safeDataCharacterSize.y * line);
					rect.Max = origin + ImVec2(_tools.safeDataCharacterSize.x * 2, _tools.safeDataCharacterSize.y * (line + 1));

					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const bool same = _selection.endsWith(_tools.active.channel, line, Tracker::DataTypes::EFFECT_PARAMETERS);
					if (same)
						return;

					_selection.clear();
				};
				auto playFromHere = [this, &mouseButton] (const ImVec2 &origin, int line) -> bool {
					ImRect rect;
					rect.Min = origin + ImVec2(0, _tools.safeDataCharacterSize.y * line);
					rect.Max = origin + ImVec2(_tools.safeDataCharacterSize.x * 2, _tools.safeDataCharacterSize.y * (line + 1));

					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return false;

					switch (mouseButton) {
					case ImGuiMouseButton_Middle:
						return true;
					default: // Do nothing.
						break;
					}

					return false;
				};

				const ImVec2 oldPos = ImGui::GetCursorScreenPos();
				ImGui::PushID("@Ln");
				for (int j = 0; j < GBBASIC_MUSIC_PATTERN_COUNT; ++j) {
					if (_tools.active.line == j)
						cursorStart = ImGui::GetCursorScreenPos();

					ImGui::PushID(j + 1);
					ImGui::PushStyleColor(ImGuiCol_Button, ROW_COLORS[j % 4]);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ROW_HOVERED_COLORS[j % 4]);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ROW_ACTIVE_COLORS[j % 4]);
					{
						const ImU32 col = ws->theme()->style()->musicSideColor;
						ImGui::PushStyleColor(ImGuiCol_Text, col);
						if (ImGui::AdjusterButton(LINE_NUMBERS[j], _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, false)) {
							if (io.KeyShift) {
								if (_selection.invalid() || _selection.first.channel != 0)
									beginLineSelection(_tools.active.line);
								endLineSelection(oldPos, j);
							} else {
								beginLineSelection(j);
							}
						}
						if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
							endLineSelection(oldPos, j);
						} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
							clearLineSelection(oldPos, j);
						}
						if (!_tools.isPlaying && playFromHere(oldPos, j)) {
							_tools.targetPlayingOrderIndex = _tools.active.sequence;
							_tools.targetPlayingLine = j;
							playMusic(ws, nullptr);
						}
						ImGui::PopStyleColor(1);
					}
					ImGui::PopStyleColor(3);
					ImGui::PopID();
				}
				ImGui::PopID();
				const ImVec2 startScreenPos = oldPos + ImVec2(_tools.safeDataCharacterSize.x * 2 + padding, 0);
				ImGui::SetCursorScreenPos(startScreenPos);

				// Iterate all the channels.
				bool openCtx = false;
				auto activated = [&mouseButton, &doubleClicked, &openCtx] (void) -> bool {
					bool active = false;
					switch (mouseButton) {
					case ImGuiMouseButton_Left:
						active = true;
						if (doubleClicked)
							openCtx = true;

						break;
					case ImGuiMouseButton_Right:
						active = true;
						openCtx = true;

						break;
					default: // Do nothing.
						break;
					}

					return active;
				};
				auto beginSelection = [this, &openCtx] (int channel, int line, Tracker::DataTypes data) -> void {
					if (openCtx)
						return;

					_selection.clear();
					_selection.start(_tools.active.sequence, channel, line, data);
				};
				auto endSelection = [this, getRect, &openCtx] (const ImVec2 &origin, int channel, int line, Tracker::DataTypes data) -> void {
					const ImRect rect = getRect(origin, channel, line, data);
					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const Tracker::Cursor end(_tools.active.sequence, channel, line, data);
					if (_selection.startsWith(end)) {
						// Do nothing.
					} else {
						_selection.end(end);

						ImGuiContext &g = *ImGui::GetCurrentContext();
						g.NavId = 0;
					}
				};
				auto clearSelection = [this, getRect] (const ImVec2 &origin, int channel, int line, Tracker::DataTypes data) -> void {
					const ImRect rect = getRect(origin, channel, line, data);
					if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
						return;

					const bool same = _selection.endsWith(channel, line, data);
					if (same)
						return;

					_selection.clear();
				};

				do {
					// Prepare.
					auto highlight = [this] (const char* idstr) -> void {
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						const ImGuiID id = window->GetID(idstr);
						ImGui::SetFocusID(id, window);
					};
					auto relocate = [this, highlight] (int i, int j, Tracker::DataTypes di, const char* idstr) -> void {
						if (_tools.target.channel == -1)
							return;
						if (_tools.target.channel != i)
							return;

						if (_tools.target.line == -1)
							return;
						if (_tools.target.line != j)
							return;

						if (_tools.active.data != di)
							return;

						ImGui::ScrollToItem(ImGuiScrollFlags_AlwaysCenterY, 1);

						if (_selection.invalid())
							highlight(idstr);

						_tools.target = Tracker::Location::INVALID();
					};

					int i = _tools.active.channel;
					const float dataWidth = _tools.safeDataCharacterSize.x * (12 + 6 + 2 + 16 + 2) + padding * 4;
					ImGui::BeginChildFrame(i + 1, ImVec2(dataWidth, _tools.safeDataCharacterSize.y * GBBASIC_MUSIC_PATTERN_COUNT), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

					// Walk through the columns.
					do {
						// Iterate all the notes.
						for (int j = 0; j < GBBASIC_MUSIC_PATTERN_COUNT; ++j) {
							// Prepare.
							const Music::Cell BLANK;
							const Music::Cell* cell_ = cell(_tools.active.sequence, i, j);
							if (!cell_)
								cell_ = &BLANK;

							// Update and render all the fields.
							const int note = cell_->note;
							const int instrument = cell_->instrument;
							const int effectCode = cell_->effectCode;
							const int effectParameters = cell_->effectParameters;

							ImGui::PushID(j + 1);
							ImGui::PushStyleColor(ImGuiCol_Button, ROW_COLORS[j % 4]);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ROW_HOVERED_COLORS[j % 4]);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ROW_ACTIVE_COLORS[j % 4]);
							ImGui::PushID("@Nt");
							{
								const ImU32 col = ws->theme()->style()->musicNoteColor;
								const std::div_t div = std::div(note, 12);
								int idx = note == ___ ? -1 : div.rem;
								bool changedNote = false;
								bool clickedNote = false;
								ImGui::PushStyleColor(ImGuiCol_Text, col);
								ImGui::PushID("@N");
								do {
									clickedNote = ImGui::BulletButtons(pianoWidth, _tools.safeDataCharacterSize.y, 12, &changedNote, &idx, &mouseButton, "-", col, (const char**)NOTES, ImGui::ColorConvertFloat4ToU32(ws->theme()->style()->builtin[ImGuiCol_Text]));
									if (clickedNote) {
										if (activated()) {
											if (io.KeyShift) {
												if (_selection.invalid())
													beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::NOTE);
												endSelection(startScreenPos, i, j, Tracker::DataTypes::NOTE);
											} else {
												beginSelection(i, j, Tracker::DataTypes::NOTE);
											}

											if (_selection.invalid()) {
												_tools.active.set(i, j, Tracker::DataTypes::NOTE);
												refreshShortcutCaches();
											}
										} else if (mouseButton == ImGuiMouseButton_Middle) {
											_selection.clear();
											_tools.active.set(i, j, Tracker::DataTypes::NOTE);
											refreshShortcutCaches();
										}

										highlight(NOTE_NAMES[note]);
									} else if (changedNote) {
										const int note_ = note == ___ ? entry()->lastOctave * 12 + idx : div.quot * 12 + idx;
										Command* cmd = enqueue<Commands::Music::SetNote>()
											->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
											->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
											->with(note_)
											->with(_setView, (int)_view.index)
											->exec(object());

										_refresh(cmd, true, false);
									}
								} while (false);
								ImGui::PopID();
								ImGui::SameLine();
								ImGui::Dummy(ImVec2(padding, 0));
								ImGui::SameLine();
								idx = note == ___ ? -1 : div.quot;
								bool changedOct = false;
								bool clickedOct = false;
								ImGui::PushID("@O");
								do {
									clickedOct = ImGui::BulletButtons(octaveWidth, _tools.safeDataCharacterSize.y, 6, &changedOct, &idx, &mouseButton, "-", col, (const char**)OCTAVES, ImGui::ColorConvertFloat4ToU32(ws->theme()->style()->builtin[ImGuiCol_Text]));
									if (clickedNote || changedNote)
										break;
									if (clickedOct) {
										if (activated()) {
											if (io.KeyShift) {
												if (_selection.invalid())
													beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::NOTE);
												endSelection(startScreenPos, i, j, Tracker::DataTypes::NOTE);
											} else {
												beginSelection(i, j, Tracker::DataTypes::NOTE);
											}

											if (_selection.invalid()) {
												_tools.active.set(i, j, Tracker::DataTypes::NOTE);
												refreshShortcutCaches();
											}
										} else if (mouseButton == ImGuiMouseButton_Middle) {
											_selection.clear();
											_tools.active.set(i, j, Tracker::DataTypes::NOTE);
											refreshShortcutCaches();
										}

										highlight(NOTE_NAMES[note]);
									} else if (changedOct) {
										const int note_ = note == ___ ? idx * 12 + entry()->lastNote : idx * 12 + div.rem;
										Command* cmd = enqueue<Commands::Music::SetNote>()
											->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
											->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
											->with(note_)
											->with(_setView, (int)_view.index)
											->exec(object());

										_refresh(cmd, true, false);
									}
								} while (false);
								ImGui::PopID();
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::NOTE);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::NOTE);
								}
								ImGui::PopStyleColor(1);
								ImGui::SameLine();

								relocate(i, j, Tracker::DataTypes::NOTE, NOTE_NAMES[note]);
							}
							ImGui::PopID();
							ImGui::Dummy(ImVec2(padding, 0));
							ImGui::SameLine();
							ImGui::PushID("@In");
							{
								const bool highlight_ = _tools.active.channel == i && _tools.active.line == j && _tools.active.data == Tracker::DataTypes::INSTRUMENT;
								const ImU32 col = ws->theme()->style()->musicInstrumentColor;
								ImGui::PushStyleColor(ImGuiCol_Text, col);
								if (ImGui::AdjusterButton(INSTRUMENT_NAMES[instrument], 2, _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, highlight_)) {
									if (activated()) {
										if (io.KeyShift) {
											if (_selection.invalid())
												beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::INSTRUMENT);
											endSelection(startScreenPos, i, j, Tracker::DataTypes::INSTRUMENT);
										} else {
											beginSelection(i, j, Tracker::DataTypes::INSTRUMENT);
										}

										if (_selection.invalid()) {
											_tools.active.set(i, j, Tracker::DataTypes::INSTRUMENT);
											refreshShortcutCaches();
										}
									} else if (mouseButton == ImGuiMouseButton_Middle) {
										_selection.clear();
										_tools.active.set(i, j, Tracker::DataTypes::INSTRUMENT);
										refreshShortcutCaches();
									}

									highlight(INSTRUMENT_NAMES[instrument]);
								}
								if (instrument >= 0 && instrument < (int)INSTRUMENT_TOOLTIPS.size() && ImGui::IsItemHovered()) {
									VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

									ImGui::PushStyleColor(ImGuiCol_Text, ws->theme()->style()->builtin[ImGuiCol_Text]);
									ImGui::SetTooltip(INSTRUMENT_TOOLTIPS[instrument]);
									ImGui::PopStyleColor();
								}
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::INSTRUMENT);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::INSTRUMENT);
								}
								ImGui::PopStyleColor(1);
								ImGui::SameLine();

								relocate(i, j, Tracker::DataTypes::INSTRUMENT, INSTRUMENT_NAMES[instrument]);
							}
							ImGui::PopID();
							ImGui::Dummy(ImVec2(padding, 0));
							ImGui::SameLine();
							ImGui::PushID("@Fx");
							{
								char buf0[2];
								char buf1[3];
								if (effectCode == 0 && effectParameters == 0) {
									buf0[0] = '-'; buf0[1] = '\0';
									buf1[0] = '-'; buf1[1] = '-'; buf1[2] = '\0';
								} else {
									const char* ecn = EFFECT_CODE_NAMES[effectCode];
									const char* epn = EFFECT_PARAMETERS_NAMES[effectParameters];
									buf0[0] = ecn[1]; buf0[1] = '\0';
									buf1[0] = epn[0]; buf1[1] = epn[1]; buf1[2] = '\0';
								}
								bool highlight_ = _tools.active.channel == i && _tools.active.line == j && _tools.active.data == Tracker::DataTypes::EFFECT_CODE;
								const ImU32 col = ws->theme()->style()->musicEffectColor;
								int idx = effectCode;
								bool changedCode = false;
								bool clickedCode = false;
								ImGui::PushStyleColor(ImGuiCol_Text, col);
								do {
									clickedCode = ImGui::BulletButtons(effectCodeWidth, _tools.safeDataCharacterSize.y, 16, &changedCode, &idx, &mouseButton, (const char**)CODES, col, (const char**)&EFFECT_CODE_TOOLTIPS_POINTERS.front(), ImGui::ColorConvertFloat4ToU32(ws->theme()->style()->builtin[ImGuiCol_Text]));
									// ImGui::AdjusterButton(buf0, 1, _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, highlight_);
									if (clickedCode) {
										if (activated()) {
											if (io.KeyShift) {
												if (_selection.invalid())
													beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::EFFECT_CODE);
												endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_CODE);
											} else {
												beginSelection(i, j, Tracker::DataTypes::EFFECT_CODE);
											}

											if (_selection.invalid()) {
												_tools.active.set(i, j, Tracker::DataTypes::EFFECT_CODE);
												refreshShortcutCaches();
											}
										} else if (mouseButton == ImGuiMouseButton_Middle) {
											_selection.clear();
											_tools.active.set(i, j, Tracker::DataTypes::EFFECT_CODE);
											refreshShortcutCaches();
										}

										highlight(buf0);
									} else if (changedCode) {
										Command* cmd = enqueue<Commands::Music::SetEffectCode>()
											->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
											->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
											->with(idx)
											->with(_setView, (int)_view.index)
											->exec(object());

										_refresh(cmd, true, false);
									}
								} while (false);
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_CODE);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_CODE);
								}
								relocate(i, j, Tracker::DataTypes::EFFECT_CODE, buf0);
								ImGui::SameLine();
								ImGui::Dummy(ImVec2(padding, 0));
								ImGui::SameLine();
								highlight_ = _tools.active.channel == i && _tools.active.line == j && _tools.active.data == Tracker::DataTypes::EFFECT_PARAMETERS;
								if (ImGui::AdjusterButton(buf1, 2, _tools.safeDataCharacterSize, &mouseButton, &doubleClicked, highlight_)) {
									if (activated()) {
										if (io.KeyShift) {
											if (_selection.invalid())
												beginSelection(_tools.active.channel, _tools.active.line, Tracker::DataTypes::EFFECT_PARAMETERS);
											endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
										} else {
											beginSelection(i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
										}

										if (_selection.invalid()) {
											_tools.active.set(i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
											refreshShortcutCaches();
										}
									} else if (mouseButton == ImGuiMouseButton_Middle) {
										_selection.clear();
										_tools.active.set(i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
										refreshShortcutCaches();
									}

									highlight(buf1);
								}
								if (effectParameters >= 0 && effectParameters < (int)EFFECT_PARAMETERS_TOOLTIPS.size() && ImGui::IsItemHovered()) {
									VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

									ImGui::PushStyleColor(ImGuiCol_Text, ws->theme()->style()->builtin[ImGuiCol_Text]);
									ImGui::SetTooltip(EFFECT_PARAMETERS_TOOLTIPS[effectParameters]);
									ImGui::PopStyleColor();
								}
								if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
									endSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
								} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
									clearSelection(startScreenPos, i, j, Tracker::DataTypes::EFFECT_PARAMETERS);
								}
								relocate(i, j, Tracker::DataTypes::EFFECT_PARAMETERS, buf1);
								ImGui::PopStyleColor(1);

								ImGui::SameLine();
								if (_tools.active.line == j) {
									cursorEnd = ImGui::GetCursorScreenPos();
									cursorEnd.y += _tools.safeDataCharacterSize.y;
								}
								ImGui::NewLine();
							}
							ImGui::PopID();
							ImGui::PopStyleColor(3);
							ImGui::PopID();
						}
					} while (false);

					// Finish.
					ImGui::EndChildFrame();

					ImGui::Dummy(ImVec2(padding, 1));
				} while (false);

				// Update and render the context menues.
				if (openCtx) {
					switch (_tools.active.data) {
					case Tracker::DataTypes::NOTE:
						ImGui::OpenPopup("@Ctx4N");

						break;
					case Tracker::DataTypes::INSTRUMENT:
						ImGui::OpenPopup("@Ctx4I");

						break;
					case Tracker::DataTypes::EFFECT_CODE:
						ImGui::OpenPopup("@Ctx4C");

						break;
					case Tracker::DataTypes::EFFECT_PARAMETERS:
						ImGui::OpenPopup("@Ctx4P");

						break;
					default: // Do nothing.
						break;
					}
				}

				if (_selection.invalid()) {
					contextForTrackerOrPianoNotes(wnd, rnd, ws);
					contextForTrackerOrPianoInstruments(wnd, rnd, ws);
					contextForTrackerOrPianoEffectCodes(wnd, rnd, ws);
					contextForTrackerOrPianoEffectParameters(wnd, rnd, ws);
				} else {
					contextForTrackerOrPiano(wnd, rnd, ws);
				}

				// Render the cursors.
				drawList->PushClipRect(wndStart, wndStart + wndSize - ImVec2(style.ScrollbarSize, 0), true);
				{
					// Render the line cursor.
					if (_selection.invalid()) {
						drawList->AddRectFilled(cursorStart, cursorEnd, 0x40000000);
						drawList->AddRect(cursorStart, cursorEnd, 0x40a0a0a0);
					}

					// Render the selection cursor.
					if (!_selection.invalid()) {
						ImDrawList* drawList_ = ImGui::GetForegroundDrawList();
						drawList_->PushClipRect(wndStart, wndStart + wndSize - ImVec2(style.ScrollbarSize, 0), true);
						{
							const Tracker::Cursor minLoc = _selection.min();
							const Tracker::Cursor maxLoc = _selection.max();
							const ImRect minRect = getRect(startScreenPos, minLoc.channel, minLoc.line, minLoc.data);
							const ImRect maxRect = getRect(startScreenPos, maxLoc.channel, maxLoc.line, maxLoc.data);
							drawList_->AddRect(minRect.Min - ImVec2(0.5f, 0.0f), maxRect.Max + ImVec2(0.5f, 0.0f), ImGui::GetColorU32(ImGuiCol_NavHighlight));
						}
						drawList_->PopClipRect();
					}
				}
				drawList->PopClipRect();

				// Add bottom space to make the cursor at the middle of the window.
				ImGui::Dummy(ImVec2(1, spaceHeight));
			}

			// Finish.
			refreshStatus(wnd, rnd, ws);

			const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
			_tools.editAreaFocused = focusingWnd;

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		}
		ImGui::EndChild();

		// Update and render the tools area for sequence orders.
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::SetCursorPosY(headY);
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - paddingY), true, _ref.windowFlags());
		{
			const float spwidth = _ref.windowWidth(splitter.second);
			int line = -1;
			if (
				Editing::Tools::orderable(
					rnd, ws,
					song()->channels, _tools.channels,
					spwidth,
					&_tools.orderEditingInitialized, &_tools.orderEditingFocused,
					Math::Vec2i((int)_tools.safeDataCharacterSize.x, (int)_tools.safeDataCharacterSize.y),
					&_tools.ordering.channel, &_tools.ordering.line,
					&_tools.editingOrders,
					ws->theme()->dialogPrompt_Orders().c_str(),
					BYTE_NUMBERS
				)
			) {
				if (song()->channels != _tools.channels) {
					Command* cmd = enqueue<Commands::Music::ChangeOrder>()
						->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
						->with(_tools.channels)
						->with(_setView, (int)_view.index)
						->exec(object());

					line = _tools.ordering.line;

					_refresh(cmd, true, false);
				}
			}
			if (line == -1)
				_tools.active.sequence = _tools.ordering.line;
			else
				_tools.active.sequence = _tools.ordering.line = line;
			ImGui::NewLine(1);

			Editing::Tools::separate(rnd, ws, spwidth);
			int channel = _tools.active.channel;
			int changedToChannel = -1;
			if (
				Editing::Tools::channels(
					rnd, ws,
					&channel,
					spwidth,
					true
				)
			) {
				_tools.active.channel = channel;
				changedToChannel = channel;
				if (!_selection.invalid()) {
					_selection.second.channel = _selection.first.channel = channel;
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::namable(
					rnd, ws,
					song()->name, _tools.name,
					spwidth,
					nullptr, nullptr,
					ws->theme()->dialogPrompt_Name().c_str()
				)
			) {
				if (_project->canRenameMusic(_index, _tools.name, nullptr)) {
					Command* cmd = enqueue<Commands::Music::SetName>()
						->with(_tools.name)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, false, false);
				} else {
					_tools.name = song()->name;

					warn(ws, ws->theme()->warning_MusicNameIsAlreadyInUse(), true);
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			int size_ = song()->ticksPerRow;
			if (
				Editing::Tools::resizable(
					rnd, ws,
					size_,
					&_tools.ticksPerRow,
					GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE,
					spwidth,
					nullptr,
					ws->theme()->dialogPrompt_TicksPerRow().c_str(),
					nullptr,
					nullptr
				)
			) {
				if (song()->ticksPerRow != _tools.ticksPerRow) {
					Command* cmd = enqueue<Commands::Music::SetTicksPerRow>()
						->with(_tools.ticksPerRow)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, true, false);
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::definable(
					rnd, ws,
					object().get(),
					&_tools.definitionShadow,
					&_tools.definitionUnfolded,
					spwidth, &_tools.definitionHeight,
					ws->theme()->dialogPrompt_Definition().c_str()
				)
			) {
				if (_tools.definitionShadow.artist != song()->artist) {
					Command* cmd = enqueue<Commands::Music::SetArtist>()
						->with(_tools.definitionShadow.artist)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, false, false);
				}
				if (_tools.definitionShadow.comment != song()->comment) {
					Command* cmd = enqueue<Commands::Music::SetComment>()
						->with(_tools.definitionShadow.comment)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, false, false);
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			bool changedInst = false;
			switch (_tools.active.channel) {
			case 0: // Duty 0.
				// Fall through.
			case 1: // Duty 1.
				changedInst = Editing::Tools::instruments(
					rnd, ws,
					object().get(),
					&_tools.dutyInstrument,
					nullptr,
					nullptr,
					nullptr, nullptr,
					nullptr, nullptr,
					changedToChannel != -1,
					&_tools.instrumentsUnfolded,
					&_tools.instrumentIndexForPianoView,
					spwidth, &_tools.instrumentsHeight,
					ws->theme()->windowAudio_Instruments().c_str(),
					[this] (Editing::Operations op) -> void {
						const Music::Instruments type = Music::Instruments::SQUARE;
						const int idx = _tools.instrumentIndexForPianoView;
						switch (op) {
						case Editing::Operations::COPY: // Copy.
							copyForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::CUT: // Cut.
							cutForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::PASTE: // Paste.
							pasteForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::RESET: // Delete.
							delForInstruments(type, _tools.active.channel, idx);

							break;
						default:
							GBBASIC_ASSERT(false && "Impossible.");

							break;
						}
					}
				);

				break;
			case 2: // Wave.
				changedInst = Editing::Tools::instruments(
					rnd, ws,
					object().get(),
					nullptr,
					&_tools.waveInstrument,
					nullptr,
					&_tools.instrumentWaveform, &_tools.instrumentWaveformMouseDown,
					nullptr, nullptr,
					changedToChannel != -1,
					&_tools.instrumentsUnfolded,
					&_tools.instrumentIndexForPianoView,
					spwidth, &_tools.instrumentsHeight,
					ws->theme()->windowAudio_Instruments().c_str(),
					[this] (Editing::Operations op) -> void {
						const Music::Instruments type = Music::Instruments::WAVE;
						const int idx = _tools.instrumentIndexForPianoView;
						switch (op) {
						case Editing::Operations::COPY: // Copy.
							copyForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::CUT: // Cut.
							cutForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::PASTE: // Paste.
							pasteForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::RESET: // Delete.
							delForInstruments(type, _tools.active.channel, idx);

							break;
						default:
							GBBASIC_ASSERT(false && "Impossible.");

							break;
						}
					}
				);

				break;
			case 3: // Noise.
				changedInst = Editing::Tools::instruments(
					rnd, ws,
					object().get(),
					nullptr,
					nullptr,
					&_tools.noiseInstrument,
					nullptr, nullptr,
					&_tools.instrumentMacros, &_tools.instrumentMacrosMouseDown,
					changedToChannel != -1,
					&_tools.instrumentsUnfolded,
					&_tools.instrumentIndexForPianoView,
					spwidth, &_tools.instrumentsHeight,
					ws->theme()->windowAudio_Instruments().c_str(),
					[this] (Editing::Operations op) -> void {
						const Music::Instruments type = Music::Instruments::NOISE;
						const int idx = _tools.instrumentIndexForPianoView;
						switch (op) {
						case Editing::Operations::COPY: // Copy.
							copyForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::CUT: // Cut.
							cutForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::PASTE: // Paste.
							pasteForInstruments(type, _tools.active.channel, idx);

							break;
						case Editing::Operations::RESET: // Delete.
							delForInstruments(type, _tools.active.channel, idx);

							break;
						default:
							GBBASIC_ASSERT(false && "Impossible.");

							break;
						}
					}
				);

				break;
			}
			if (changedInst) {
				switch (_tools.active.channel) {
				case 0: // Duty 0.
					// Fall through.
				case 1: { // Duty 1.
						Command* cmd = enqueue<Commands::Music::SetDutyInstrument>()
							->with(_tools.instrumentIndexForPianoView, _tools.dutyInstrument)
							->with(_setView, (int)_view.index)
							->exec(object());

						_refresh(cmd, false, false);
					}

					break;
				case 2: { // Wave.
						Command* cmd = enqueue<Commands::Music::SetWaveInstrument>()
							->with(_tools.instrumentIndexForPianoView, _tools.waveInstrument)
							->with(_setView, (int)_view.index)
							->exec(object());

						_refresh(cmd, false, false);
					}

					break;
				case 3: { // Noise.
						Command* cmd = enqueue<Commands::Music::SetNoiseInstrument>()
							->with(_tools.instrumentIndexForPianoView, _tools.noiseInstrument)
							->with(_setView, (int)_view.index)
							->exec(object());

						_refresh(cmd, false, false);
					}

					break;
				}
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	void updateWaves(Window* wnd, Renderer* rnd,
		Workspace* ws,
		float /* width */, float height, float headerHeight, float paddingY,
		bool &statusBarActived
	) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		const Ref::Splitter splitter = _ref.split();

		shortcutsForWaveforms(wnd, rnd, ws);

		// Render the header.
		const float headY = ImGui::GetCursorPosY();
		ImGui::BeginChild("@Hdr", ImVec2(splitter.first, headerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			const char* items[] = {
				ws->theme()->dialogPrompt_Waveform_0().c_str(),
				ws->theme()->dialogPrompt_Waveform_1().c_str(),
				ws->theme()->dialogPrompt_Waveform_2().c_str(),
				ws->theme()->dialogPrompt_Waveform_3().c_str(),
				ws->theme()->dialogPrompt_Waveform_4().c_str(),
				ws->theme()->dialogPrompt_Waveform_5().c_str(),
				ws->theme()->dialogPrompt_Waveform_6().c_str(),
				ws->theme()->dialogPrompt_Waveform_7().c_str(),
				ws->theme()->dialogPrompt_Waveform_8().c_str(),
				ws->theme()->dialogPrompt_Waveform_9().c_str(),
				ws->theme()->dialogPrompt_Waveform_10().c_str(),
				ws->theme()->dialogPrompt_Waveform_11().c_str(),
				ws->theme()->dialogPrompt_Waveform_12().c_str(),
				ws->theme()->dialogPrompt_Waveform_13().c_str(),
				ws->theme()->dialogPrompt_Waveform_14().c_str(),
				ws->theme()->dialogPrompt_Waveform_15().c_str()
			};
			int index = _tools.waveformCursor;
			ImGui::SetNextItemWidth(splitter.first);
			if (ImGui::Combo("", &index, items, GBBASIC_COUNTOF(items))) {
				_tools.waveformCursor = index;

				refreshWaveform();
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip("(Shift+Tab/Tab)");
			}
		}
		ImGui::EndChild();

		// Update and render the edit area for wave.
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - paddingY - headerHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
		{
			if (
				ImGui::LineGraph(
					ImVec2(splitter.first, height - paddingY - headerHeight),
					ImGui::GetColorU32(ImGuiCol_Text), ImGui::GetColorU32(ImGuiCol_TextDisabled), ws->theme()->style()->graphColor, ws->theme()->style()->graphDrawingColor,
					Math::Vec2i(1, 1),
					&_tools.waveform.front(), &_tools.newWaveform.front(),
					0x0, 0xf,
					(int)_tools.newWaveform.size(),
					false,
					&_tools.waveformMousePosition,
					&_tools.waveformMouseDown,
					"%d: %X"
				)
			) {
				Music::Wave wave_;
				for (int i = 0; i < (int)wave_.size(); ++i)
					wave_[i] = (UInt8)_tools.newWaveform[i];
				Command* cmd = enqueue<Commands::Music::SetWave>()
					->with(wave_, _setWave, _getWave)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, false, false);
			}

			refreshStatus(wnd, rnd, ws);

			const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
			_tools.editAreaFocused = focusingWnd;

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		}
		ImGui::EndChild();

		// Update and render the tools area for waves.
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::SetCursorPosY(headY);
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - paddingY), true, _ref.windowFlags());
		{
			const float spwidth = _ref.windowWidth(splitter.second);
			if (
				Editing::Tools::waveform(
					rnd, ws,
					_tools.waveformString, _tools.newWaveformString,
					spwidth,
					nullptr, nullptr,
					ws->theme()->dialogPrompt_Data().c_str()
				)
			) {
				if (_tools.waveformString != _tools.newWaveformString) {
					const Music::Wave wave_ = parseWaveform(_tools.newWaveformString);
					Command* cmd = enqueue<Commands::Music::SetWave>()
						->with(wave_, _setWave, _getWave)
						->with(_setView, (int)_view.index)
						->exec(object());

					_refresh(cmd, false, false);
				}
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	void copyForTrackerOrPiano(void) {
		typedef std::vector<int> Data;

		auto toString = [] (const Tracker::Range &area, const Data &data) -> std::string {
			rapidjson::Document doc;
			const Math::Vec2i size = area.size();
			const std::string tracker = "tracker";
			Jpath::set(doc, doc, tracker, "type");
			Jpath::set(doc, doc, size.x, "width");
			Jpath::set(doc, doc, size.y, "height");
			Jpath::set(doc, doc, (int)area.min().data, "offset");
			for (int k = 0; k < (int)data.size(); ++k) {
				Jpath::set(doc, doc, data[k], "data", k);
			}

			std::string buf;
			Json::toString(doc, buf);

			return buf;
		};

		if (!_binding.getData || !_binding.setData)
			return;

		Tracker::Range area = _selection;
		if (area.invalid()) {
			area.first = area.second = _tools.active;
			if (area.invalid())
				return;
		}
		Data vec;

		int k = 0;
		for (int ln = area.min().line; ln <= area.max().line; ++ln) {
			const Tracker::Cursor a(area.min().sequence, area.min().channel, ln, area.min().data);
			const Tracker::Cursor b(area.max().sequence, area.max().channel, ln, area.max().data);
			for (Tracker::Cursor c = a; c <= b && c.line == ln; c = c.nextData()) {
				int val = 0;
				getData(c, val);
				vec.push_back(val);

				++k;
			}
		}

		const std::string buf = toString(area, vec);
		const std::string osstr = Unicode::toOs(buf);

		Platform::setClipboardText(osstr.c_str());
	}
	void cutForTrackerOrPiano(void) {
		if (!_binding.getData || !_binding.setData)
			return;

		copyForTrackerOrPiano();

		Tracker::Range area = _selection;
		if (area.invalid()) {
			area.first = area.second = _tools.active;
			if (area.invalid())
				return;
		}

		Command* cmd = enqueue<Commands::Music::CutForTracker>()
			->with(_binding.getData, _binding.setData)
			->with(
				area,
				[] (const Tracker::Cursor &c, int &val) -> bool {
					if (c.data == Tracker::DataTypes::NOTE)
						val = ___;
					else
						val = 0;

					return true;
				}
			)
			->with(_tools.active.sequence, _tools.active.channel, _tools.active.line)
			->exec(object());

		_refresh(cmd, true, false);
	}
	void pasteForTrackerOrPiano(void) {
		typedef std::vector<int> Data;

		auto fromString = [] (const std::string &buf, Tracker::Range &area, Data &vec) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			std::string type;
			int width = -1, height = -1;
			int offset = -1;
			if (!Jpath::get(doc, type, "type") || type != "tracker")
				return false;
			if (!Jpath::get(doc, width, "width"))
				return false;
			if (!Jpath::get(doc, height, "height"))
				return false;
			if (!Jpath::get(doc, offset, "offset"))
				return false;

			constexpr const int _ = 0;
			area.first = Tracker::Cursor(_, _, _, (Tracker::DataTypes)offset);
			area.second = area.first;
			for (int j = 0; j < height - 1; ++j)
				area.second = area.second.nextLine();
			for (int i = 0; i < width - 1; ++i)
				area.second = area.second.nextData();
			int k = 0;
			for (int j = 0; j < height; ++j) {
				for (int i = 0; i < width; ++i) {
					int val = 0;
					if (!Jpath::get(doc, val, "data", k++))
						return false;
					vec.push_back(val);
				}
			}

			return true;
		};

		const std::string osstr = Platform::getClipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Tracker::Range area;
		Data vec;
		if (!fromString(buf, area, vec))
			return;

		Tracker::Range dstArea = _selection;
		if (dstArea.invalid()) {
			dstArea.first = dstArea.second = _tools.active;
			if (dstArea.invalid())
				return;
		}
		Command* cmd = enqueue<Commands::Music::PasteForTracker>()
			->with(_binding.getData, _binding.setData)
			->with(dstArea, vec, area)
			->with(_tools.active.sequence, _tools.active.channel, _tools.active.line)
			->exec(object());

		_refresh(cmd, true, false);
	}
	void delForTrackerOrPiano(void) {
		if (!_binding.getData || !_binding.setData)
			return;

		const Tracker::Range &area = _selection;
		if (area.invalid())
			return;

		Command* cmd = enqueue<Commands::Music::DeleteForTracker>()
			->with(_binding.getData, _binding.setData)
			->with(
				area,
				[] (const Tracker::Cursor &c, int &val) -> bool {
					if (c.data == Tracker::DataTypes::NOTE)
						val = ___;
					else
						val = 0;

					return true;
				}
			)
			->with(_tools.active.sequence, _tools.active.channel, _tools.active.line)
			->exec(object());

		_refresh(cmd, true, false);
	}

	void copyForInstruments(Music::Instruments type, int /* group */, int index) {
		auto dutyInstrumentToString = [] (const Music::DutyInstrument &inst) -> std::string {
			rapidjson::Document doc;
			if (!inst.toJson(doc, doc))
				return "";

			std::string buf;
			if (!Json::toString(doc, buf, false))
				return "";

			return buf;
		};
		auto waveInstrumentToString = [] (const Music::WaveInstrument &inst) -> std::string {
			rapidjson::Document doc;
			if (!inst.toJson(doc, doc))
				return "";

			std::string buf;
			if (!Json::toString(doc, buf, false))
				return "";

			return buf;
		};
		auto noiseInstrumentToString = [] (const Music::NoiseInstrument &inst) -> std::string {
			rapidjson::Document doc;
			if (!inst.toJson(doc, doc))
				return "";

			std::string buf;
			if (!Json::toString(doc, buf, false))
				return "";

			return buf;
		};

		std::string buf;
		switch (type) {
		case Music::Instruments::SQUARE: {
				const Music::DutyInstrument &inst = song()->dutyInstruments[index];

				buf = dutyInstrumentToString(inst);
			}

			break;
		case Music::Instruments::WAVE: {
				const Music::WaveInstrument &inst = song()->waveInstruments[index];

				buf = waveInstrumentToString(inst);
			}

			break;
		case Music::Instruments::NOISE: {
				const Music::NoiseInstrument &inst = song()->noiseInstruments[index];

				buf = noiseInstrumentToString(inst);
			}

			break;
		}

		const std::string osstr = Unicode::toOs(buf);

		Platform::setClipboardText(osstr.c_str());
	}
	void cutForInstruments(Music::Instruments type, int group, int index) {
		copyForInstruments(type, group, index);

		switch (type) {
		case Music::Instruments::SQUARE: {
				const Music::DutyInstrument inst;

				Command* cmd = enqueue<Commands::Music::CutForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		case Music::Instruments::WAVE: {
				const Music::WaveInstrument inst;

				Command* cmd = enqueue<Commands::Music::CutForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		case Music::Instruments::NOISE: {
				const Music::NoiseInstrument inst;

				Command* cmd = enqueue<Commands::Music::CutForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		}
	}
	void pasteForInstruments(Music::Instruments type, int group, int index) {
		auto dutyFromString = [] (const std::string &buf, Music::DutyInstrument &inst) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			if (!inst.fromJson(doc))
				return false;

			return true;
		};
		auto waveFromString = [] (const std::string &buf, Music::WaveInstrument &inst) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			if (!inst.fromJson(doc))
				return false;

			return true;
		};
		auto noiseFromString = [] (const std::string &buf, Music::NoiseInstrument &inst) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			if (!inst.fromJson(doc))
				return false;

			return true;
		};

		const std::string osstr = Platform::getClipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		switch (type) {
		case Music::Instruments::SQUARE: {
				Music::DutyInstrument inst;
				if (!dutyFromString(buf, inst))
					return;

				Command* cmd = enqueue<Commands::Music::PasteForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		case Music::Instruments::WAVE: {
				Music::WaveInstrument inst;
				if (!waveFromString(buf, inst))
					return;

				Command* cmd = enqueue<Commands::Music::DeleteForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		case Music::Instruments::NOISE: {
				Music::NoiseInstrument inst;
				if (!noiseFromString(buf, inst))
					return;

				Command* cmd = enqueue<Commands::Music::DeleteForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		}
	}
	void delForInstruments(Music::Instruments type, int group, int index) {
		switch (type) {
		case Music::Instruments::SQUARE: {
				const Music::DutyInstrument inst;

				Command* cmd = enqueue<Commands::Music::DeleteForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		case Music::Instruments::WAVE: {
				const Music::WaveInstrument inst;

				Command* cmd = enqueue<Commands::Music::DeleteForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		case Music::Instruments::NOISE: {
				const Music::NoiseInstrument inst;

				Command* cmd = enqueue<Commands::Music::DeleteForInstrument>()
					->with(_setInstrument, group, index)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			break;
		}
	}

	void copyForWaves(void) {
		auto toString = [] (const Music::Wave &wave) -> std::string {
			rapidjson::Document doc;
			const int n = (int)wave.size();
			Jpath::set(doc, doc, n, "count");
			for (int i = 0; i < (int)wave.size(); ++i) {
				Jpath::set(doc, doc, wave[i], "data", i);
			}

			std::string buf;
			Json::toString(doc, buf);

			return buf;
		};

		const Music::Wave &wave = song()->waves[_tools.waveformCursor];

		const std::string buf = toString(wave);
		const std::string osstr = Unicode::toOs(buf);

		Platform::setClipboardText(osstr.c_str());
	}
	void cutForWaves(void) {
		copyForWaves();

		Command* cmd = enqueue<Commands::Music::CutForWave>()
			->with(_setWave, _getWave)
			->with(_setView, (int)_view.index)
			->exec(object());

		_refresh(cmd, true, false);
	}
	void pasteForWaves(void) {
		auto fromString = [] (const std::string &buf, Music::Wave &wave) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			int count = 0;
			if (!Jpath::get(doc, count, "count"))
				return false;

			for (int i = 0; i < count && i < (int)wave.size(); ++i) {
				Jpath::get(doc, wave[i], "data", i);
			}

			return true;
		};

		const std::string osstr = Platform::getClipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Music::Wave wave;
		if (!fromString(buf, wave))
			return;

		Command* cmd = enqueue<Commands::Music::PasteForWave>()
			->with(wave)
			->with(_setWave, _getWave)
			->with(_setView, (int)_view.index)
			->exec(object());

		_refresh(cmd, true, false);
	}
	void delForWaves(void) {
		Command* cmd = enqueue<Commands::Music::DeleteForWave>()
			->with(_setWave, _getWave)
			->with(_setView, (int)_view.index)
			->exec(object());

		_refresh(cmd, true, false);
	}

	void shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
		if (!ws->canUseShortcuts())
			return;

#if defined GBBASIC_OS_APPLE
		const Editing::Shortcut pgUp(SDL_SCANCODE_PAGEUP, false, false, false, false, false, true);
		const Editing::Shortcut pgDown(SDL_SCANCODE_PAGEDOWN, false, false, false, false, false, true);
#else /* GBBASIC_OS_APPLE */
		const Editing::Shortcut pgUp(SDL_SCANCODE_PAGEUP, true);
		const Editing::Shortcut pgDown(SDL_SCANCODE_PAGEDOWN, true);
#endif /* GBBASIC_OS_APPLE */
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (pgUp.pressed()) {
				int index = _index - 1;
				if (index < 0)
					index = _project->musicPageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::MUSIC, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->musicPageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::MUSIC, index);
			}
		}
	}
	void shortcutsForTrackerOrPiano(Window*, Renderer*, Workspace* ws) {
		// Prepare.
		ImGuiIO &io = ImGui::GetIO();

		if (!ws->canUseShortcuts())
			return;

		if (!_tools.editAreaFocused)
			return;

		if (io.KeyCtrl || io.KeyAlt || io.KeySuper)
			return;

		const bool multipleChannels = _view.index == Tracker::ViewTypes::TRACKER;

		const bool  home   = ImGui::IsKeyPressed(SDL_SCANCODE_HOME);
		const bool  end    = ImGui::IsKeyPressed(SDL_SCANCODE_END);
		const bool  pgUp   = ImGui::IsKeyPressed(SDL_SCANCODE_PAGEUP);
		const bool  pgDown = ImGui::IsKeyPressed(SDL_SCANCODE_PAGEDOWN);
		const bool  up     = ImGui::IsKeyPressed(SDL_SCANCODE_UP);
		const bool  down   = ImGui::IsKeyPressed(SDL_SCANCODE_DOWN);
		const bool  left   = ImGui::IsKeyPressed(SDL_SCANCODE_LEFT);
		const bool  right  = ImGui::IsKeyPressed(SDL_SCANCODE_RIGHT);
		const bool  tab    = ImGui::IsKeyPressed(SDL_SCANCODE_TAB);
		const bool  shift  = io.KeyShift;

		// Sequence operations.
		if (home && shift) {
			const int index = 0; // Goto the first sequence.
			if (index != _tools.active.sequence) {
				_tools.active.sequence = index;
				_tools.ordering.line = _tools.active.sequence;
			}
		} else if (end && shift) {
			const int index = sequenceCount() - 1; // Goto the last sequence.
			if (index != _tools.active.sequence) {
				_tools.active.sequence = index;
				_tools.ordering.line = _tools.active.sequence;
			}
		}

		if (pgUp && shift) {
			int index = _tools.active.sequence - 1; // Goto the previous sequence.
			if (index < 0)
				index = sequenceCount() - 1;
			if (index != _tools.active.sequence) {
				_tools.active.sequence = index;
				_tools.ordering.line = _tools.active.sequence;
				_selection.clear();
			}
		} else if (pgDown && shift) {
			int index = _tools.active.sequence + 1; // Goto the next sequence.
			if (index >= sequenceCount())
				index = 0;
			if (index != _tools.active.sequence) {
				_tools.active.sequence = index;
				_tools.ordering.line = _tools.active.sequence;
				_selection.clear();
			}
		}

		// Cursor operations.
		if (home && !shift) {
			const int index = 0; // Move the cursor to the top.
			_tools.active.line = index;
			active(_tools.active.channel, index);
		} else if (end && !shift) {
			const int index = GBBASIC_MUSIC_PATTERN_COUNT - 1; // Move the cursor to the bottom.
			_tools.active.line = index;
			active(_tools.active.channel, index);
		}

		if (pgUp && !shift) {
			int index = _tools.active.line - 4; // Move the cursor to the previous page.
			if (index < 0)
				index = GBBASIC_MUSIC_PATTERN_COUNT - 1;
			_tools.active.line = index;
			active(_tools.active.channel, index);
		} else if (pgDown && !shift) {
			int index = _tools.active.line + 4; // Move the cursor to the next page.
			if (index >= GBBASIC_MUSIC_PATTERN_COUNT)
				index = 0;
			_tools.active.line = index;
			active(_tools.active.channel, index);
		}

		if (up && !shift) {
			int index = _tools.active.line - 1; // Move the cursor to the previous line.
			if (index < 0)
				index = GBBASIC_MUSIC_PATTERN_COUNT - 1;
			_tools.active.line = index;
			active(_tools.active.channel, index);
		} else if (down && !shift) {
			int index = _tools.active.line + 1; // Move the cursor to the next line.
			if (index >= GBBASIC_MUSIC_PATTERN_COUNT)
				index = 0;
			_tools.active.line = index;
			active(_tools.active.channel, index);
		} else if (up && shift) {
			if (_selection.first == Tracker::Cursor::INVALID())
				_selection.start(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);

			const Tracker::Range sel = _selection;
			const int index = Math::max(_tools.active.line - 1, 0); // Move the cursor to the previous line and make selection.
			_tools.active.line = index;
			active(_tools.active.channel, index);
			_selection = sel;

			_selection.end(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);
		} else if (down && shift) {
			if (_selection.first == Tracker::Cursor::INVALID())
				_selection.start(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);

			const Tracker::Range sel = _selection;
			const int index = Math::min(_tools.active.line + 1, GBBASIC_MUSIC_PATTERN_COUNT - 1); // Move the cursor to the next line and make selection.
			_tools.active.line = index;
			active(_tools.active.channel, index);
			_selection = sel;

			_selection.end(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);
		}

		if (left && !shift) {
			int index = (int)_tools.active.data - 1; // Move the cursor to the previous data column.
			int channel = _tools.active.channel;
			if (index < 0) {
				if (multipleChannels) {
					index = (int)Tracker::DataTypes::COUNT - 1;
					if (--channel < 0)
						channel = GBBASIC_MUSIC_CHANNEL_COUNT - 1;
				} else {
					index = 0;
				}
			}
			if ((int)_tools.active.data != index || _tools.active.channel != channel) {
				_tools.active.data = (Tracker::DataTypes)index;
				_tools.active.channel = channel;
				active(channel, _tools.active.line);
			}
		} else if (right && !shift) {
			int index = (int)_tools.active.data + 1; // Move the cursor to the next data column.
			int channel = _tools.active.channel;
			if (index >= (int)Tracker::DataTypes::COUNT) {
				if (multipleChannels) {
					index = 0;
					if (++channel >= GBBASIC_MUSIC_CHANNEL_COUNT)
						channel = 0;
				} else {
					index = (int)Tracker::DataTypes::COUNT - 1;
				}
			}
			if ((int)_tools.active.data != index || _tools.active.channel != channel) {
				_tools.active.data = (Tracker::DataTypes)index;
				_tools.active.channel = channel;
				active(channel, _tools.active.line);
			}
		} else if (left && shift) {
			if (_selection.first == Tracker::Cursor::INVALID())
				_selection.start(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);

			const Tracker::Range sel = _selection;
			int index = (int)_tools.active.data - 1; // Move the cursor to the previous data column and make selection.
			int channel = _tools.active.channel;
			if (index < 0) {
				if (multipleChannels) {
					index = (int)Tracker::DataTypes::COUNT - 1;
					if (--channel < 0) {
						index = (int)Tracker::DataTypes::NOTE;
						channel = 0;
					}
				} else {
					index = 0;
				}
			}
			if ((int)_tools.active.data != index || _tools.active.channel != channel) {
				_tools.active.data = (Tracker::DataTypes)index;
				_tools.active.channel = channel;
				active(channel, _tools.active.line);
				_selection = sel;

				_selection.end(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);
			}
		} else if (right && shift) {
			if (_selection.first == Tracker::Cursor::INVALID())
				_selection.start(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);

			const Tracker::Range sel = _selection;
			int index = (int)_tools.active.data + 1; // Move the cursor to the next data column and make selection.
			int channel = _tools.active.channel;
			if (index >= (int)Tracker::DataTypes::COUNT) {
				if (multipleChannels) {
					index = 0;
					if (++channel >= GBBASIC_MUSIC_CHANNEL_COUNT) {
						index = (int)Tracker::DataTypes::EFFECT_PARAMETERS;
						channel = GBBASIC_MUSIC_CHANNEL_COUNT - 1;
					}
				} else {
					index = (int)Tracker::DataTypes::COUNT - 1;
				}
			}
			if ((int)_tools.active.data != index || _tools.active.channel != channel) {
				_tools.active.data = (Tracker::DataTypes)index;
				_tools.active.channel = channel;
				active(channel, _tools.active.line);
				_selection = sel;

				_selection.end(_tools.active.sequence, _tools.active.channel, _tools.active.line, _tools.active.data);
			}
		}

		if (multipleChannels) {
			if (tab && !shift) {
				const int index = (int)_tools.active.data;
				int channel = _tools.active.channel;
				if (++channel >= GBBASIC_MUSIC_CHANNEL_COUNT) // Move the cursor to the next channel.
					channel = 0;
				_tools.active.data = (Tracker::DataTypes)index;
				_tools.active.channel = channel;
				active(channel, _tools.active.line);
			} else if (tab && shift) {
				const int index = (int)_tools.active.data;
				int channel = _tools.active.channel;
				if (--channel < 0) // Move the cursor to the previous channel.
					channel = GBBASIC_MUSIC_CHANNEL_COUNT - 1;
				_tools.active.data = (Tracker::DataTypes)index;
				_tools.active.channel = channel;
				active(channel, _tools.active.line);
			}
		}

		// Data editing operations.
		bool nextLine = false;
		switch (_tools.active.data) {
		case Tracker::DataTypes::NOTE: {
				const Music::Cell BLANK;
				const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
				if (!cell_)
					cell_ = &BLANK;

				const int note = shortcutsForTrackerOrPianoNotes(cell_->note);
				if (note == EDITING_MUSIC_NO_INPUT_VALUE) {
					break;
				} else if (note == EDITING_MUSIC_NEWLINE_INPUT_VALUE) {
					nextLine = true;

					break;
				}

				if (note == cell_->note)
					break;

				Command* cmd = enqueue<Commands::Music::SetNote>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(note)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);

				if (!io.KeyShift)
					nextLine = true;
			}

			break;
		case Tracker::DataTypes::INSTRUMENT: {
				const int inst = shortcutsForTrackerOrPianoInstruments();
				if (inst == EDITING_MUSIC_NO_INPUT_VALUE) {
					break;
				} else if (inst == EDITING_MUSIC_NEWLINE_INPUT_VALUE) {
					nextLine = true;

					break;
				}

				const Music::Cell BLANK;
				const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
				if (!cell_)
					cell_ = &BLANK;

				if (inst == cell_->instrument)
					break;

				Command* cmd = enqueue<Commands::Music::SetInstrument>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);

				if (io.KeyShift)
					nextLine = true;
			}

			break;
		case Tracker::DataTypes::EFFECT_CODE: {
				const int code = shortcutsForTrackerOrPianoEffectCodes();
				if (code == EDITING_MUSIC_NO_INPUT_VALUE) {
					break;
				} else if (code == EDITING_MUSIC_NEWLINE_INPUT_VALUE) {
					nextLine = true;

					break;
				}

				const Music::Cell BLANK;
				const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
				if (!cell_)
					cell_ = &BLANK;

				if (code == cell_->effectCode)
					break;

				Command* cmd = enqueue<Commands::Music::SetEffectCode>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(code)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);

				if (io.KeyShift)
					nextLine = true;
			}

			break;
		case Tracker::DataTypes::EFFECT_PARAMETERS: {
				const int params = shortcutsForTrackerOrPianoEffectParameters();
				if (params == EDITING_MUSIC_NO_INPUT_VALUE) {
					break;
				} else if (params == EDITING_MUSIC_NEWLINE_INPUT_VALUE) {
					nextLine = true;

					break;
				}

				const Music::Cell BLANK;
				const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
				if (!cell_)
					cell_ = &BLANK;

				if (params == cell_->effectParameters)
					break;

				Command* cmd = enqueue<Commands::Music::SetEffectParameters>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(params)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);

				if (io.KeyShift)
					nextLine = true;
			}

			break;
		default:
			// Do nothing.

			break;
		}
		if (nextLine) {
			if (++_tools.active.line >= GBBASIC_MUSIC_PATTERN_COUNT)
				_tools.active.line = GBBASIC_MUSIC_PATTERN_COUNT - 1;
			active(_tools.active.channel, _tools.active.line);
		}
	}
	int shortcutsForTrackerOrPianoNotes(int oldNote) const {
		ImGuiIO &io = ImGui::GetIO();

		if (io.KeyCtrl || io.KeyShift || io.KeySuper)
			return EDITING_MUSIC_NO_INPUT_VALUE;

		const bool  del          = ImGui::IsKeyPressed(SDL_SCANCODE_DELETE);
		const bool  backspace    = ImGui::IsKeyPressed(SDL_SCANCODE_BACKSPACE);
		const bool  z            = ImGui::IsKeyPressed(SDL_SCANCODE_Z);
		const bool  s            = ImGui::IsKeyPressed(SDL_SCANCODE_S);
		const bool  x            = ImGui::IsKeyPressed(SDL_SCANCODE_X);
		const bool  d            = ImGui::IsKeyPressed(SDL_SCANCODE_D);
		const bool  c            = ImGui::IsKeyPressed(SDL_SCANCODE_C);
		const bool  v            = ImGui::IsKeyPressed(SDL_SCANCODE_V);
		const bool  g            = ImGui::IsKeyPressed(SDL_SCANCODE_G);
		const bool  b            = ImGui::IsKeyPressed(SDL_SCANCODE_B);
		const bool  h            = ImGui::IsKeyPressed(SDL_SCANCODE_H);
		const bool  n            = ImGui::IsKeyPressed(SDL_SCANCODE_N);
		const bool  j            = ImGui::IsKeyPressed(SDL_SCANCODE_J);
		const bool  m            = ImGui::IsKeyPressed(SDL_SCANCODE_M);
		const bool  q            = ImGui::IsKeyPressed(SDL_SCANCODE_Q);
		const bool _2            = ImGui::IsKeyPressed(SDL_SCANCODE_2);
		const bool  w            = ImGui::IsKeyPressed(SDL_SCANCODE_W);
		const bool _3            = ImGui::IsKeyPressed(SDL_SCANCODE_3);
		const bool  e            = ImGui::IsKeyPressed(SDL_SCANCODE_E);
		const bool  r            = ImGui::IsKeyPressed(SDL_SCANCODE_R);
		const bool _5            = ImGui::IsKeyPressed(SDL_SCANCODE_5);
		const bool  t            = ImGui::IsKeyPressed(SDL_SCANCODE_T);
		const bool _6            = ImGui::IsKeyPressed(SDL_SCANCODE_6);
		const bool  y            = ImGui::IsKeyPressed(SDL_SCANCODE_Y);
		const bool _7            = ImGui::IsKeyPressed(SDL_SCANCODE_7);
		const bool  u            = ImGui::IsKeyPressed(SDL_SCANCODE_U);
		const bool  i            = ImGui::IsKeyPressed(SDL_SCANCODE_I);
		const bool _9            = ImGui::IsKeyPressed(SDL_SCANCODE_9);
		const bool  o            = ImGui::IsKeyPressed(SDL_SCANCODE_O);
		const bool _0            = ImGui::IsKeyPressed(SDL_SCANCODE_0);
		const bool  p            = ImGui::IsKeyPressed(SDL_SCANCODE_P);
		const bool  leftbracket  = ImGui::IsKeyPressed(SDL_SCANCODE_LEFTBRACKET);
		const bool  rightbracket = ImGui::IsKeyPressed(SDL_SCANCODE_RIGHTBRACKET);
		const bool  minus        = ImGui::IsKeyPressed(SDL_SCANCODE_MINUS);
		const bool  plus         = ImGui::IsKeyPressed(SDL_SCANCODE_EQUALS);
		const bool  enter        = ImGui::IsKeyPressed(SDL_SCANCODE_RETURN);
		const bool  normal       = !io.KeyAlt;

		if (enter)
			return EDITING_MUSIC_NEWLINE_INPUT_VALUE;

		int note = EDITING_MUSIC_NO_INPUT_VALUE;
		if (del || backspace) {
			note = ___;
		} else if (z) {
			note = normal ? C_3 : C_6;
		} else if (s) {
			note = normal ? Cs3 : Cs6;
		} else if (x) {
			note = normal ? D_3 : D_6;
		} else if (d) {
			note = normal ? Ds3 : Ds6;
		} else if (c) {
			note = normal ? E_3 : E_6;
		} else if (v) {
			note = normal ? F_3 : F_6;
		} else if (g) {
			note = normal ? Fs3 : Fs6;
		} else if (b) {
			note = normal ? G_3 : G_6;
		} else if (h) {
			note = normal ? Gs3 : Gs6;
		} else if (n) {
			note = normal ? A_3 : A_6;
		} else if (j) {
			note = normal ? As3 : As6;
		} else if (m) {
			note = normal ? B_3 : B_6;
		} else if (q) {
			note = normal ? C_4 : C_7;
		} else if (_2) {
			note = normal ? Cs4 : Cs7;
		} else if (w) {
			note = normal ? D_4 : D_7;
		} else if (_3) {
			note = normal ? Ds4 : Ds7;
		} else if (e) {
			note = normal ? E_4 : E_7;
		} else if (r) {
			note = normal ? F_4 : F_7;
		} else if (_5) {
			note = normal ? Fs4 : Fs7;
		} else if (t) {
			note = normal ? G_4 : G_7;
		} else if (_6) {
			note = normal ? Gs4 : Gs7;
		} else if (y) {
			note = normal ? A_4 : A_7;
		} else if (_7) {
			note = normal ? As4 : As7;
		} else if (u) {
			note = normal ? B_4 : B_7;
		} else if (i) {
			note = normal ? C_5 : C_8;
		} else if (_9) {
			note = normal ? Cs5 : Cs8;
		} else if (o) {
			note = normal ? D_5 : D_8;
		} else if (_0) {
			note = normal ? Ds5 : Ds8;
		} else if (p) {
			note = normal ? E_5 : E_8;
		} else if (leftbracket) {
			note = normal ? F_5 : F_8;
		} else if (rightbracket) {
			note = normal ? Fs5 : Fs8;
		} else if (minus) {
			if (oldNote == ___) {
				// Do nothing.
			} else {
				note = oldNote - 1;
				if (note < 0)
					note = ___;
			}
		} else if (plus) {
			if (oldNote == ___) {
				note = C_3;
			} else {
				note = oldNote + 1;
				if (note >= LAST_NOTE)
					note = LAST_NOTE - 1;
			}
		}

		return note;
	}
	int shortcutsForTrackerOrPianoInstruments(void) {
		const bool enter = ImGui::IsKeyPressed(SDL_SCANCODE_RETURN);
		if (enter)
			return EDITING_MUSIC_NEWLINE_INPUT_VALUE;

		int val = 0;
		if (!_tools.instrumentNumberStroke.filled(&val))
			return EDITING_MUSIC_NO_INPUT_VALUE;

		return val;
	}
	int shortcutsForTrackerOrPianoEffectCodes(void) {
		const bool enter = ImGui::IsKeyPressed(SDL_SCANCODE_RETURN);
		if (enter)
			return EDITING_MUSIC_NEWLINE_INPUT_VALUE;

		int val = 0;
		if (!_tools.effectCodeNumberStroke.filled(&val))
			return EDITING_MUSIC_NO_INPUT_VALUE;

		return val;
	}
	int shortcutsForTrackerOrPianoEffectParameters(void) {
		const bool enter = ImGui::IsKeyPressed(SDL_SCANCODE_RETURN);
		if (enter)
			return EDITING_MUSIC_NEWLINE_INPUT_VALUE;

		int val = 0;
		if (!_tools.effectParametersNumberStroke.filled(&val))
			return EDITING_MUSIC_NO_INPUT_VALUE;

		return val;
	}
	void shortcutsForWaveforms(Window*, Renderer*, Workspace* ws) {
		if (!ws->canUseShortcuts())
			return;

		const Editing::Shortcut shiftTab(SDL_SCANCODE_TAB, false, true);
		const Editing::Shortcut tab(SDL_SCANCODE_TAB);
		if (shiftTab.pressed()) {
			if (--_tools.waveformCursor < 0)
				_tools.waveformCursor = GBBASIC_MUSIC_WAVEFORM_COUNT - 1;

			refreshWaveform();
		} else if (tab.pressed()) {
			if (++_tools.waveformCursor >= GBBASIC_MUSIC_WAVEFORM_COUNT)
				_tools.waveformCursor = 0;

			refreshWaveform();
		}
	}
	void refreshShortcutCaches(void) {
		const Music::Cell BLANK;
		const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
		if (!cell_)
			cell_ = &BLANK;
		_tools.instrumentNumberStroke.value = cell_->instrument;
		_tools.effectCodeNumberStroke.value = cell_->effectCode;
		_tools.effectParametersNumberStroke.value = cell_->effectParameters;
	}

	bool contextForTrackerOrPiano(Window*, Renderer*, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		bool result = false;
		if (ImGui::BeginPopup("@Ctx")) {
			if (ImGui::MenuItem(ws->theme()->menu_Cut())) {
				cut();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Copy())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Paste(), nullptr, nullptr, pastable())) {
				paste();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Delete())) {
				del(false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_SelectAll())) {
				post(Editable::SELECT_ALL);
			}

			ImGui::EndPopup();

			result = true;
		}

		return result;
	}
	bool contextForTrackerOrPianoNotes(Window*, Renderer*, Workspace*) {
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		bool result = false;
		if (ImGui::BeginPopup("@Ctx4N")) {
			int note = EDITING_MUSIC_NO_INPUT_VALUE;
			for (int i = 0; i < GBBASIC_COUNTOF(OCTAVE_NAMES); ++i) {
				if (ImGui::BeginMenu(OCTAVE_NAMES[i])) {
					for (int j = 0; j < 12; ++j) {
						const int k = i * 12 + j;
						if (ImGui::MenuItem(NOTE_NAMES[k])) {
							note = C_3 + i * 12 + j;
						}
					}

					ImGui::EndMenu();
				}
			}
			if (ImGui::MenuItem("---")) {
				note = ___;
			}

			ImGui::EndPopup();

			const Music::Cell BLANK;
			const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
			if (!cell_)
				cell_ = &BLANK;

			if (note != cell_->note && note != EDITING_MUSIC_NO_INPUT_VALUE) {
				Command* cmd = enqueue<Commands::Music::SetNote>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(note)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			result = true;
		}

		return result;
	}
	bool contextForTrackerOrPianoInstruments(Window*, Renderer*, Workspace*) {
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		bool result = false;
		if (ImGui::BeginPopup("@Ctx4I")) {
			int inst = EDITING_MUSIC_NO_INPUT_VALUE;
			for (int i = 0; i < GBBASIC_COUNTOF(INSTRUMENT_NAMES); ++i) {
				if (ImGui::MenuItem(INSTRUMENT_NAMES[i])) {
					inst = i;
				}
			}

			ImGui::EndPopup();

			const Music::Cell BLANK;
			const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
			if (!cell_)
				cell_ = &BLANK;

			if (inst != cell_->instrument && inst != EDITING_MUSIC_NO_INPUT_VALUE) {
				Command* cmd = enqueue<Commands::Music::SetInstrument>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(inst)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			result = true;
		}

		return result;
	}
	bool contextForTrackerOrPianoEffectCodes(Window*, Renderer*, Workspace*) {
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		bool result = false;
		if (ImGui::BeginPopup("@Ctx4C")) {
			int code = EDITING_MUSIC_NO_INPUT_VALUE;
			for (int i = 0; i < 16 /* used count of `EFFECT_CODE_NAMES` */; ++i) {
				if (i == 0) {
					if (ImGui::MenuItem("-")) {
						code = 0;
					}
				} else {
					if (ImGui::MenuItem(EFFECT_CODE_NAMES[i])) {
						code = i;
					}
				}

				if (i < (int)EFFECT_CODE_TOOLTIPS.size() && ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(EFFECT_CODE_TOOLTIPS[i]);
				}
			}

			ImGui::EndPopup();

			const Music::Cell BLANK;
			const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
			if (!cell_)
				cell_ = &BLANK;

			if (code != cell_->effectCode && code != EDITING_MUSIC_NO_INPUT_VALUE) {
				Command* cmd = enqueue<Commands::Music::SetEffectCode>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(code)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			result = true;
		}

		return result;
	}
	bool contextForTrackerOrPianoEffectParameters(Window*, Renderer*, Workspace*) {
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		bool result = false;
		if (ImGui::BeginPopup("@Ctx4P")) {
			int params = EDITING_MUSIC_NO_INPUT_VALUE;
			constexpr const int M = GBBASIC_COUNTOF(BYTE_NAMES);
			const int N = 256 /* used count of `EFFECT_PARAMETERS_NAMES` */ / M;
			for (int i = 0; i < M; ++i) {
				if (ImGui::BeginMenu(BYTE_NAMES[i])) {
					for (int j = 0; j < N; ++j) {
						if (i == 0 && j == 0) {
							if (ImGui::MenuItem("--")) {
								params = 0;
							}
						} else {
							if (ImGui::MenuItem(EFFECT_PARAMETERS_NAMES[j + N * i])) {
								params = j + N * i;
							}
						}
					}

					ImGui::EndMenu();
				}
			}

			ImGui::EndPopup();

			const Music::Cell BLANK;
			const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.active.channel, _tools.active.line);
			if (!cell_)
				cell_ = &BLANK;

			if (params != cell_->effectParameters && params != EDITING_MUSIC_NO_INPUT_VALUE) {
				Command* cmd = enqueue<Commands::Music::SetEffectParameters>()
					->with(std::bind(&EditorMusicImpl::cell, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
					->with(_setViewDetail, _tools.active.sequence, _tools.active.channel, _tools.active.line)
					->with(params)
					->with(_setView, (int)_view.index)
					->exec(object());

				_refresh(cmd, true, false);
			}

			result = true;
		}

		return result;
	}

	int dataCharacterScale(void) {
		const int scale = Math::clamp(entry()->magnification, 1, 4);
		if (scale == _tools.magnification)
			return _tools.magnification;

		_tools.magnification = scale;
		_tools.safeDataCharacterSize = safeDataCharacterSize();

		return _tools.magnification;
	}
	ImVec2 safeDataCharacterSize(void) const {
		ImVec2 result;
		char buf[2] = { '\0', '\0' };
		for (char ch = '0'; ch <= '9'; ++ch) {
			buf[0] = ch;
			const ImVec2 size = ImGui::CalcTextSize(buf, buf + 1);
			result.x = Math::max(result.x, size.x);
			result.y = Math::max(result.y, size.y);
		}
		for (char ch = 'a'; ch <= 'z'; ++ch) {
			buf[0] = ch;
			const ImVec2 size = ImGui::CalcTextSize(buf, buf + 1);
			result.x = Math::max(result.x, size.x);
			result.y = Math::max(result.y, size.y);
		}
		for (char ch = 'A'; ch <= 'Z'; ++ch) {
			buf[0] = ch;
			const ImVec2 size = ImGui::CalcTextSize(buf, buf + 1);
			result.x = Math::max(result.x, size.x);
			result.y = Math::max(result.y, size.y);
		}

		return result * (float)_tools.magnification;
	}

	void refreshInstrumentNames(Music::Ptr newObj) const {
		for (int i = 0; i < (int)newObj->pointer()->dutyInstruments.size(); ++i) {
			Music::DutyInstrument &inst = newObj->pointer()->dutyInstruments[i];
			inst.name = "Duty" + Text::toString(i + 1);
		}
		for (int i = 0; i < (int)newObj->pointer()->waveInstruments.size(); ++i) {
			Music::WaveInstrument &inst = newObj->pointer()->waveInstruments[i];
			inst.name = "Wave" + Text::toString(i + 1);
		}
		for (int i = 0; i < (int)newObj->pointer()->noiseInstruments.size(); ++i) {
			Music::NoiseInstrument &inst = newObj->pointer()->noiseInstruments[i];
			inst.name = "Noise" + Text::toString(i + 1);
		}
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
		if (!_status.filled) {
			if (readonly()) {
				_status.text += " " + ws->theme()->status_Readonly();
			}
			_status.filled = true;
		}
		if (!_estimated.filled) {
			_estimated.text = " " + Text::toScaledBytes(entry()->estimateFinalByteCount());
			_estimated.filled = true;
		}
	}
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, float width, float height, bool actived) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (actived || EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			const ImVec2 pos = ImGui::GetCursorPos();
			ImGui::Dummy(
				ImVec2(width - style.ChildBorderSize, height - style.ChildBorderSize),
				ImGui::GetStyleColorVec4(ImGuiCol_Button)
			);
			ImGui::SetCursorPos(pos);
		}

		if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		}
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		if (ImGui::Button("<", ImVec2(0, height))) {
			int index = _index - 1;
			if (index < 0)
				index = _project->musicPageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::MUSIC, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_PreviousPage());
		}
		ImGui::SameLine();
		if (ImGui::Button(_page.text.c_str(), ImVec2(0, height))) {
			ImGui::OpenPopup("@Pg");
		}
		ImGui::SameLine();
		if (ImGui::Button(">", ImVec2(0, height))) {
			int index = _index + 1;
			if (index >= _project->musicPageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::MUSIC, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		if (_tools.isPlaying) {
			if (ImGui::ImageButton(ws->theme()->iconStopPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_StopMusic().c_str())) {
				stopMusic(ws);
			}
		} else {
			if (ws->running()) {
				ImGui::BeginDisabled();
				ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_PlayMusic().c_str());
				ImGui::EndDisabled();
			} else {
				if (ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_PlayMusic().c_str())) {
					playMusic(ws, nullptr);
				}
			}
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(style.FramePadding.x, 0));
		ImGui::SameLine();
		if (ImGui::Checkbox(ws->theme()->dialogPrompt_Stroke(), &_project->preferencesMusicPreviewStroke())) {
			if (!_project->preferencesMusicPreviewStroke()) {
				stopMusic(ws);
				ws->destroyAudioDevice();
			}

			_project->hasDirtyAsset(true);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipAudio_MusicPlayOnStroke());
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_status.text);
		ImGui::SameLine();
		ImGui::TextUnformatted(_estimated.text);
		ImGui::SameLine();
		do {
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - _statusWidth);
			if (!_warnings.empty()) {
				const ImVec4 col = ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->warningColor);
				ImGui::PushStyleColor(ImGuiCol_Text, col);

				if (ImGui::ImageButton(ws->theme()->iconWarning()->pointer(rnd), ImVec2(13, 13), col, false, ws->theme()->tooltip_Warning().c_str())) {
					ImGui::OpenPopupTooltip("Wrn");
				}
				if (ImGui::PopupTooltip("Wrn", _warnings.text, ws->theme()->generic_Dismiss().c_str()))
					_warnings.clear();
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();

				ImGui::PopStyleColor();
			}
			if (ImGui::ImageButton(ws->theme()->iconImport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Import().c_str())) {
				ImGui::OpenPopup("@Imp");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconExport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Export().c_str())) {
				ImGui::OpenPopup("@Xpt");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			do {
				WIDGETS_SELECTION_GUARD(ws->theme());

				if (ImGui::ImageButton(ws->theme()->iconMusic()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_Music().c_str())) {
					// Do nothing.
				}
			} while (false);
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconSfx()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_Sfx().c_str())) {
				ws->category(Workspace::Categories::SFX);
			}
			width_ += ImGui::GetItemRectSize().x;
			width_ += style.FramePadding.x;
			_statusWidth = width_;
		} while (false);
		if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			ImGui::PopStyleColor(3);
		}

		statusBarMenu(wnd, rnd, ws);
	}
	void statusBarMenu(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		// Paging.
		if (ImGui::BeginPopup("@Pg")) {
			const Text::Array &assetNames = ws->getMusicPageNames();
			const int n = _project->musicPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::MUSIC, i);
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::MenuItem(ws->theme()->menu_C())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					importFromCString(wnd, rnd, ws, txt);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_C_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					importFromCFile(wnd, rnd, ws, path);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					importFromJsonString(wnd, rnd, ws, txt);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_JsonFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_JSON_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					importFromJsonFile(wnd, rnd, ws, path);
				} while (false);
			}
			if (ws->musicCount() > 0) {
#if WORKSPACE_MUSIC_MENU_ENABLED
				if (ImGui::BeginMenu(ws->theme()->menu_Library())) {
					std::string path;
					if (ImGui::MusicMenu(ws->music(), path)) {
						importFromLibraryFile(wnd, rnd, ws, path);
					}

					ImGui::EndMenu();
				}
#else /* WORKSPACE_MUSIC_MENU_ENABLED */
				if (ImGui::MenuItem(ws->theme()->menu_FromLibrary())) {
					const std::string musicDirectory = Path::absoluteOf(WORKSPACE_MUSIC_DIR);
					std::string defaultPath = musicDirectory;
					do {
						DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(musicDirectory.c_str());
						FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", false);
						if (fileInfos->count() == 0)
							break;

						FileInfo::Ptr fileInfo = fileInfos->get(0);
						defaultPath = fileInfo->fullPath();
#if defined GBBASIC_OS_WIN
						defaultPath = Text::replace(defaultPath, "/", "\\");
#endif /* GBBASIC_OS_WIN */
					} while (false);

					do {
						pfd::open_file open(
							GBBASIC_TITLE,
							defaultPath,
							GBBASIC_JSON_FILE_FILTER,
							pfd::opt::none
						);
						if (open.result().empty())
							break;

						std::string path = open.result().front();
						Path::uniform(path);
						if (path.empty())
							break;

						importFromLibraryFile(wnd, rnd, ws, path);
					} while (false);
				}
#endif /* WORKSPACE_MUSIC_MENU_ENABLED */
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::MenuItem(ws->theme()->menu_Code())) {
				do {
					std::string txt;
					if (!entry()->serializeBasic(txt, _index))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_C())) {
				do {
					std::string txt;
					if (!entry()->serializeC(txt))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						"gbbasic-music.c",
						GBBASIC_C_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || ext != "c")
						path += ".c";

					std::string txt;
					if (!entry()->serializeC(txt))
						break;

					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeString(txt);
					file->close();

#if !defined GBBASIC_OS_HTML
					FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
					std::string path_ = fileInfo->parentPath();
					path_ = Unicode::toOs(path_);
					Platform::browse(path_.c_str());
#endif /* Platform macro. */

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					std::string txt;
					if (!entry()->serializeJson(txt, true))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_JsonFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						song()->name.empty() ? "gbbasic-music.json" : Text::sanitizeFilename(song()->name) + ".json",
						GBBASIC_JSON_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || ext != "json")
						path += ".json";

					std::string txt;
					if (!entry()->serializeJson(txt, false))
						break;

					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeString(txt);
					file->close();

#if !defined GBBASIC_OS_HTML
					FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
					std::string path_ = fileInfo->parentPath();
					path_ = Unicode::toOs(path_);
					Platform::browse(path_.c_str());
#endif /* Platform macro. */

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}
	}

	void changeView(Window* wnd, Renderer* rnd, Workspace* ws, int view) {
		if (_view.index == (Tracker::ViewTypes)view)
			return;

		_view.target = (Tracker::ViewTypes)view;
		_status.clear();
		refreshStatus(wnd, rnd, ws);
	}
	void changeViewDetail(Window* wnd, Renderer* rnd, Workspace* ws, int seq, int ch, int ln) {
		if (_tools.active.sequence == seq && _tools.active.channel == ch && _tools.active.line == ln)
			return;

		_tools.active.sequence = seq;
		_tools.active.channel = ch;
		_tools.active.line = ln;
		_status.clear();
		refreshStatus(wnd, rnd, ws);
	}
	void changeInstrument(Window*, Renderer*, Workspace*, Music::Instruments type, int group, int index) {
		int idx = -1;
		switch (type) {
		case Music::Instruments::SQUARE:
			idx = index;

			break;
		case Music::Instruments::WAVE:
			idx = index + (int)song()->dutyInstruments.size();

			break;
		case Music::Instruments::NOISE:
			idx = index + ((int)song()->dutyInstruments.size() + (int)song()->waveInstruments.size());

			break;
		}

		if (_tools.instrumentIndexForTrackerView != idx)
			_tools.instrumentIndexForTrackerView = idx;

		if (_tools.instrumentIndexForPianoView != index)
			_tools.instrumentIndexForPianoView = index;

		if (_view.index == Tracker::ViewTypes::PIANO)
			_tools.active.channel = group;
	}
	void changeWave(Window* wnd, Renderer* rnd, Workspace* ws, int wave) {
		if (_tools.waveformCursor == wave)
			return;

		_tools.waveformCursor = wave;
		_status.clear();
		refreshStatus(wnd, rnd, ws);
	}
	int getWaveIndex(void) const {
		return _tools.waveformCursor;
	}

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "Music editor: ";
				msg_ += msg;
				if (msg.back() != '.')
					msg_ += '.';
				ws->warn(msg_.c_str());
			}
		} else {
			_warnings.remove(msg);
		}
	}

	template<typename T> T* enqueue(void) {
		T* result = _commands->enqueue<T>();

		_project->toPollEditor(true);

		return result;
	}
	void refresh(Workspace* ws, const Command* cmd, bool play, bool activate) {
		const bool changedNote =
			Command::is<Commands::Music::SetNote>(cmd);
		const bool relocate =
			Command::is<Commands::Music::SetNote>(cmd) ||
			Command::is<Commands::Music::SetInstrument>(cmd) ||
			Command::is<Commands::Music::SetEffectCode>(cmd) ||
			Command::is<Commands::Music::SetEffectParameters>(cmd) ||
			Command::is<Commands::Music::ChangeOrder>(cmd) ||
			Command::is<Commands::Music::CutForTracker>(cmd) ||
			Command::is<Commands::Music::PasteForTracker>(cmd) ||
			Command::is<Commands::Music::DeleteForTracker>(cmd);
		const bool playable =
			Command::is<Commands::Music::SetNote>(cmd) ||
			Command::is<Commands::Music::SetInstrument>(cmd) ||
			Command::is<Commands::Music::SetEffectCode>(cmd) ||
			Command::is<Commands::Music::SetEffectParameters>(cmd);
		const bool refillTicksPerRow =
			Command::is<Commands::Music::SetTicksPerRow>(cmd);
		const bool refillName =
			Command::is<Commands::Music::SetName>(cmd);
		const bool refillDefinition =
			Command::is<Commands::Music::SetArtist>(cmd) ||
			Command::is<Commands::Music::SetComment>(cmd) ||
			Command::is<Commands::Music::Import>(cmd);
		const bool refillChannels =
			Command::is<Commands::Music::ChangeOrder>(cmd);
		const bool recalcSize =
			Command::is<Commands::Music::ChangeOrder>(cmd) ||
			Command::is<Commands::Music::Import>(cmd);
		const bool refillInstrument =
			Command::is<Commands::Music::SetInstrument>(cmd) ||
			Command::is<Commands::Music::CutForInstrument>(cmd) ||
			Command::is<Commands::Music::PasteForInstrument>(cmd) ||
			Command::is<Commands::Music::DeleteForInstrument>(cmd) ||
			Command::is<Commands::Music::SetDutyInstrument>(cmd) ||
			Command::is<Commands::Music::SetWaveInstrument>(cmd) ||
			Command::is<Commands::Music::SetNoiseInstrument>(cmd) ||
			Command::is<Commands::Music::Import>(cmd);
		const bool refillWave =
			Command::is<Commands::Music::CutForWave>(cmd) ||
			Command::is<Commands::Music::PasteForWave>(cmd) ||
			Command::is<Commands::Music::DeleteForWave>(cmd) ||
			Command::is<Commands::Music::SetWave>(cmd) ||
			Command::is<Commands::Music::Import>(cmd);
		const bool refillAll =
			Command::is<Commands::Music::Import>(cmd);

		bool toStop = true;
		if (changedNote) {
			const Commands::Music::Playable* pcmd = Command::as<Commands::Music::Playable>(cmd);
			const std::div_t div = std::div(pcmd->note(), 12);
			entry()->lastNote = div.rem;
			entry()->lastOctave = div.quot;
		}
		if (relocate) {
			if (Command::is<Commands::Music::Playable>(cmd)) {
				const Commands::Music::Playable* pcmd = Command::as<Commands::Music::Playable>(cmd);
				active(pcmd->sequenceIndex(), pcmd->channelIndex(), pcmd->lineIndex());
			} else if (Command::is<Commands::Music::BlitForTracker>(cmd)) {
				const Commands::Music::BlitForTracker* pcmd = Command::as<Commands::Music::BlitForTracker>(cmd);
				active(pcmd->sequenceIndex(),pcmd->channelIndex(), pcmd->lineIndex());
			}
		}
		if (playable) {
			if (play) {
				_tools.playMusic(&_tools.active);
				toStop = false;
			}
			if (activate) {
				const Commands::Music::Playable* pcmd = Command::as<Commands::Music::Playable>(cmd);
				_tools.active.line = pcmd->lineIndex();
			}
		}
		if (refillTicksPerRow) {
			_tools.ticksPerRow = song()->ticksPerRow;
		}
		if (refillName) {
			_tools.name = song()->name;
			ws->clearMusicPageNames();
		}
		if (refillDefinition) {
			_tools.definitionShadow.artist = song()->artist;
			_tools.definitionShadow.comment = song()->comment;
		}
		if (refillChannels) {
			_tools.channels = song()->channels;
		}
		if (recalcSize) {
			_estimated.filled = false;
		}
		if (refillInstrument) {
			refreshInstruments();
		}
		if (refillWave) {
			refreshWaveform();
		}
		if (refillAll) {
			_tools.definitionShadow.artist = song()->artist;
			_tools.definitionShadow.comment = song()->comment;
			_tools.instrumentNumberStroke.clear();
			_tools.effectCodeNumberStroke.clear();
			_tools.effectParametersNumberStroke.clear();
			_tools.ticksPerRow = song()->ticksPerRow;
			_tools.name = song()->name;
			_tools.channels = song()->channels;
		}

		if (toStop) {
			_tools.stopPlaying();
		}
	}

	MusicAssets::Entry* entry(void) const {
		MusicAssets::Entry* entry = _project->getMusic(_index);

		return entry;
	}
	Music::Ptr &object(void) const {
		return entry()->data;
	}
	const Music::Song* song(void) const {
		return object()->pointer();
	}
	Music::Song* song(void) {
		return object()->pointer();
	}
	Music::Cell* cell(int sequenceIndex, int channelIndex, int cellIndex) {
		Music::Orders &orders = song()->orders;
		const Music::Channels &channels = song()->channels;
		GBBASIC_ASSERT(orders.size() == GBBASIC_MUSIC_CHANNEL_COUNT && "Wrong data.");
		GBBASIC_ASSERT(channels.size() == GBBASIC_MUSIC_CHANNEL_COUNT && "Wrong data.");

		const Music::Sequence &seq = channels[channelIndex];
		GBBASIC_ASSERT(seq.size() <= GBBASIC_MUSIC_SEQUENCE_MAX_COUNT && "Wrong data.");
		if (seq.empty())
			return nullptr;

		Music::Order &order = orders[channelIndex];
		GBBASIC_ASSERT(order.size() == GBBASIC_MUSIC_ORDER_COUNT && "Wrong data.");
		if (order.empty())
			return nullptr;

		const int orderIndex = seq[sequenceIndex];

		Music::Pattern &pattern = order[orderIndex];
		GBBASIC_ASSERT(pattern.size() == GBBASIC_MUSIC_PATTERN_COUNT && "Wrong data.");

		if (cellIndex < 0 || cellIndex >= (int)pattern.size())
			return nullptr;
		Music::Cell &cell = pattern[cellIndex];

		return &cell;
	}
	void active(int sequence, int channel, int line) {
		const Music::Channels &channels = song()->channels;
		sequence = Math::clamp(sequence, 0, (int)channels[channel].size() - 1);
		_tools.ordering.line = _tools.active.sequence = sequence;
		_tools.target = Tracker::Location(channel, line);

		const Music::Cell BLANK;
		const Music::Cell* cell_ = cell(_tools.active.sequence, _tools.target.channel, _tools.target.line);
		if (!cell_)
			cell_ = &BLANK;
		_tools.instrumentNumberStroke.value = cell_->instrument;
		_tools.effectCodeNumberStroke.value = cell_->effectCode;
		_tools.effectParametersNumberStroke.value = cell_->effectParameters;

		_selection.clear();
	}
	void active(int channel, int line) {
		_tools.target = Tracker::Location(channel, line);

		refreshShortcutCaches();

		_selection.clear();
	}
	int sequenceCount(void) const {
		const Music::Channels &channels = song()->channels;
		GBBASIC_ASSERT(channels.size() == GBBASIC_MUSIC_CHANNEL_COUNT && "Wrong data.");

		const Music::Sequence &seq = channels.front();
		GBBASIC_ASSERT(seq.size() <= GBBASIC_MUSIC_SEQUENCE_MAX_COUNT && "Wrong data.");
		if (seq.empty())
			return 0;

		return (int)seq.size();
	}

	bool getData(const Tracker::Cursor &c, int &val) {
		const Music::Cell* cell_ = cell(c.sequence, c.channel, c.line);
		if (!cell_)
			return false;

		switch (c.data) {
		case Tracker::DataTypes::NOTE:
			val = cell_->note;

			break;
		case Tracker::DataTypes::INSTRUMENT:
			val = cell_->instrument;

			break;
		case Tracker::DataTypes::EFFECT_CODE:
			val = cell_->effectCode;

			break;
		case Tracker::DataTypes::EFFECT_PARAMETERS:
			val = cell_->effectParameters;

			break;
		default:
			// Do nothing.

			break;
		}

		return true;
	}
	bool setData(const Tracker::Cursor &c, const int &val) {
		Music::Cell* cell_ = cell(c.sequence, c.channel, c.line);
		if (!cell_)
			return false;

		switch (c.data) {
		case Tracker::DataTypes::NOTE:
			cell_->note = val;

			break;
		case Tracker::DataTypes::INSTRUMENT:
			cell_->instrument = val;

			break;
		case Tracker::DataTypes::EFFECT_CODE:
			cell_->effectCode = val;

			break;
		case Tracker::DataTypes::EFFECT_PARAMETERS:
			cell_->effectParameters = val;

			break;
		default:
			// Do nothing.

			break;
		}

		return true;
	}

	bool importFromCString(Window*, Renderer*, Workspace* ws, const std::string &txt) {
		auto parse = [ws, this, txt] (void) -> bool {
			Music::Ptr newObj = nullptr;
			if (
				!entry()->parseC(
					newObj, "Clipboard", txt,
					[this, ws] (const char* msg) -> void {
						std::string msg_ = "Music editor: ";
						msg_ += msg;
						if (msg_.back() != '\n' && msg_.back() != '.')
							msg_ += '.';
						ws->warn(msg_.c_str());
					}
				)
			) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			newObj->pointer()->name = _project->getUsableMusicName(newObj->pointer()->name, _index);

			refreshInstrumentNames(newObj);

			Command* cmd = enqueue<Commands::Music::Import>()
				->with(newObj)
				->with(_setView, (int)_view.index)
				->exec(object());

			_refresh(cmd, false, false);

			ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

			return true;
		};

		ImGui::WaitingPopupBox::TimeoutHandler timeout(
			[ws, this, parse] (void) -> void {
				const bool ret = parse();

				Workspace* ws_ = ws;

				ws->popupBox(nullptr);

				if (!ret)
					ws_->messagePopupBox(ws_->theme()->dialogPrompt_InvalidDataSeeTheConsoleWindowForDetails(), nullptr, nullptr, nullptr);
			},
			nullptr
		);
		ws->waitingPopupBox(
			true, ws->theme()->dialogPrompt_Importing(),
			true, timeout,
			true
		);

		return true;
	}
	bool importFromCFile(Window*, Renderer*, Workspace* ws, const std::string &path) {
		std::string txt;
		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		if (!file->readString(txt)) {
			file->close(); FileMonitor::unuse(path);

			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		file->close(); FileMonitor::unuse(path);

		auto parse = [ws, this, txt, path] (void) -> bool {
			Music::Ptr newObj = nullptr;
			if (
				!entry()->parseC(
					newObj, path.c_str(), txt,
					[ws, this] (const char* msg) -> void {
						std::string msg_ = "Music editor: ";
						msg_ += msg;
						if (msg_.back() != '\n' && msg_.back() != '.')
							msg_ += '.';
						ws->warn(msg_.c_str());
					}
				)
			) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			newObj->pointer()->name = _project->getUsableMusicName(newObj->pointer()->name, _index);

			refreshInstrumentNames(newObj);

			Command* cmd = enqueue<Commands::Music::Import>()
				->with(newObj)
				->with(_setView, (int)_view.index)
				->exec(object());

			_refresh(cmd, false, false);

			ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

			return true;
		};

		ImGui::WaitingPopupBox::TimeoutHandler timeout(
			[ws, this, parse] (void) -> void {
				const bool ret = parse();

				Workspace* ws_ = ws;

				ws->popupBox(nullptr);

				if (!ret)
					ws_->messagePopupBox(ws_->theme()->dialogPrompt_InvalidDataSeeTheConsoleWindowForDetails(), nullptr, nullptr, nullptr);
			},
			nullptr
		);
		ws->waitingPopupBox(
			true, ws->theme()->dialogPrompt_Importing(),
			true, timeout,
			true
		);

		return true;
	}
	bool importFromJsonString(Window*, Renderer*, Workspace* ws, const std::string &txt) {
		Music::Ptr newObj = nullptr;
		if (!entry()->parseJson(newObj, txt)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		newObj->pointer()->name = _project->getUsableMusicName(newObj->pointer()->name, _index);

		refreshInstrumentNames(newObj);

		Command* cmd = enqueue<Commands::Music::Import>()
			->with(newObj)
			->with(_setView, (int)_view.index)
			->exec(object());

		_refresh(cmd, false, false);

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}
	bool importFromJsonFile(Window*, Renderer*, Workspace* ws, const std::string &path) {
		std::string txt;
		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		if (!file->readString(txt)) {
			file->close(); FileMonitor::unuse(path);

			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		file->close(); FileMonitor::unuse(path);

		Music::Ptr newObj = nullptr;
		if (!entry()->parseJson(newObj, txt)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		newObj->pointer()->name = _project->getUsableMusicName(newObj->pointer()->name, _index);

		refreshInstrumentNames(newObj);

		Command* cmd = enqueue<Commands::Music::Import>()
			->with(newObj)
			->with(_setView, (int)_view.index)
			->exec(object());

		_refresh(cmd, false, false);

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}
	bool importFromLibraryFile(Window*, Renderer*, Workspace* ws, const std::string &path) {
		std::string txt;
		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		if (!file->readString(txt)) {
			file->close(); FileMonitor::unuse(path);

			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		file->close(); FileMonitor::unuse(path);

		Music::Ptr newObj = nullptr;
		if (!entry()->parseJson(newObj, txt)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		newObj->pointer()->name = _project->getUsableMusicName(newObj->pointer()->name, _index);

		refreshInstrumentNames(newObj);

		Command* cmd = enqueue<Commands::Music::Import>()
			->with(newObj)
			->with(_setView, (int)_view.index)
			->exec(object());

		_refresh(cmd, false, false);

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}

	void refreshInstruments(void) {
		Music::Instruments type;
		int idx = -1;

		if (_tools.instrumentIndexForTrackerView != -1) {
			const Music::BaseInstrument* inst = song()->instrumentAt(_tools.instrumentIndexForTrackerView, &type);
			(void)inst;
			switch (type) {
			case Music::Instruments::SQUARE:
				idx = _tools.instrumentIndexForTrackerView;

				break;
			case Music::Instruments::WAVE:
				idx = _tools.instrumentIndexForTrackerView - (int)song()->dutyInstruments.size();

				break;
			case Music::Instruments::NOISE:
				idx = _tools.instrumentIndexForTrackerView - ((int)song()->dutyInstruments.size() + (int)song()->waveInstruments.size());

				break;
			}
		} else if (_tools.instrumentIndexForPianoView != -1) {
			type = (Music::Instruments)_tools.active.channel;
			idx = _tools.instrumentIndexForPianoView;
		} else {
			return;
		}

		if (idx == -1)
			return;

		switch (type) {
		case Music::Instruments::SQUARE: {
				const Music::DutyInstrument &inst = song()->dutyInstruments[idx];

				_tools.dutyInstrument = inst;
			}

			break;
		case Music::Instruments::WAVE: {
				const Music::WaveInstrument &inst = song()->waveInstruments[idx];

				_tools.waveInstrument = inst;
			}

			break;
		case Music::Instruments::NOISE: {
				const Music::NoiseInstrument &inst = song()->noiseInstruments[idx];

				_tools.noiseInstrument = inst;
			}

			break;
		}
	}

	std::string serializeWaveform(const Music::Wave &wave) const {
		std::string result;
		for (int i = 0; i < (int)wave.size(); ++i) {
			const std::string point = Text::toHex(wave[i], 1, '0', true);
			result += point;
		}

		return result;
	}
	Music::Wave parseWaveform(const std::string &str) const {
		Music::Wave result;
		result.fill(0);
		for (int i = 0; i < (int)str.length() && i < (int)result.size(); ++i) {
			const std::string point = std::string("0x") + str[i];
			int val = 0;
			Text::fromString(point, val);
			result[i] = (UInt8)val;
		}

		return result;
	}
	void refreshWaveform(void) {
		const Music::Wave &wave = song()->waves[_tools.waveformCursor];
		for (int i = 0; i < (int)wave.size(); ++i) {
			_tools.waveform[i] = wave[i];
			_tools.newWaveform[i] = wave[i];
		}

		_tools.waveformString = _tools.newWaveformString = serializeWaveform(song()->waves[_tools.waveformCursor]);
	}

	bool playMusic(Workspace* ws, const Tracker::Cursor* active) {
		// Prepare.
		if (ws->running())
			return true;

		if (active) {
			if (!_project->preferencesMusicPreviewStroke())
				return true;
		}

		if (_tools.isPlaying)
			stopMusic(ws);

		// Start measuring performance.
		const long long start = DateTime::ticks();

		// Compose the song.
		int sequence = _tools.active.sequence;
		MusicAssets::Entry entry_;
		if (active) {
			Music::Cell cell_ = *cell(active->sequence, active->channel, active->line);
			if (cell_.instrument == 0) {
				for (int i = active->line - 1; i >= 0; --i) {
					const Music::Cell* cellp = cell(active->sequence, active->channel, i);
					if (cellp->instrument) {
						cell_.instrument = cellp->instrument;

						break;
					}
				}
			}
			if (cell_.instrument == 0) {
				for (int i = active->line + 1; i < GBBASIC_MUSIC_PATTERN_COUNT; ++i) {
					const Music::Cell* cellp = cell(active->sequence, active->channel, i);
					if (cellp->instrument) {
						cell_.instrument = cellp->instrument;

						break;
					}
				}
			}
			if (cell_.instrument == 0) {
				cell_.instrument = 1;
			}
			cell_.effectCode = 0;
			cell_.effectParameters = 0;
			Music::Song* song_ = entry_.data->pointer();
			song_->dutyInstruments = entry()->data->pointer()->dutyInstruments;
			for (int i = 0; i < (int)song_->dutyInstruments.size(); ++i) {
				Music::DutyInstrument &inst = song_->dutyInstruments[i];
				inst.initialVolume = MUSIC_DUTY_INSTRUMENT_MAX_INITIAL_VOLUME;
			}
			song_->waveInstruments = entry()->data->pointer()->waveInstruments;
			for (int i = 0; i < (int)song_->waveInstruments.size(); ++i) {
				Music::WaveInstrument &inst = song_->waveInstruments[i];
				inst.volume = 1;
			}
			song_->noiseInstruments = entry()->data->pointer()->noiseInstruments;
			for (int i = 0; i < (int)song_->noiseInstruments.size(); ++i) {
				Music::NoiseInstrument &inst = song_->noiseInstruments[i];
				inst.initialVolume = MUSIC_NOISE_INSTRUMENT_MAX_INITIAL_VOLUME;
			}
			song_->waves = entry()->data->pointer()->waves;
			song_->ticksPerRow = entry()->data->pointer()->ticksPerRow;
			Music::Orders &orders = song_->orders;
			Music::Order &order = orders[active->channel];
			Music::Pattern &pattern = order[0]; // Rather than `active->sequence`.
			pattern[0] = cell_;
			pattern[1] = Music::Cell(___, 0, 0xc, 0x00);
			sequence = 0;
		} else {
			entry_ = *entry();
		}

		// Compile.
		const Bytes::Ptr rom_ = compileMusic(ws, this, entry_, _tools, sequence);
		if (active) {
			_tools.playerStopDelay = 1.5;
			_tools.playerMaxDuration = 3.0;
		} else {
			_tools.playerStopDelay = 0.5;
		}

		if (!rom_)
			return false;

		// Finish measuring performance.
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		const std::string time = Text::toString(secs, 6, 0, ' ', std::ios::fixed);

		const std::string msg = "Completed in " + time + "s.";
		fprintf(stdout, "%s\n", msg.c_str());

		// Run and play.
		ws->createAudioDevice();
		ws->openAudioDevice(rom_);

		const Device::Ptr &device = ws->audioDevice();
		device->writeRam((UInt16)_tools.playerIsPlayingLocation.address,   (UInt8)EDITOR_MUSIC_HALT_BANK);
		device->writeRam((UInt16)_tools.playerOrderCursorLocation.address, (UInt8)0xff);
		device->writeRam((UInt16)_tools.playerLineCursorLocation.address,  (UInt8)0xff);
		device->writeRam((UInt16)_tools.playerTicksLocation.address,       (UInt8)0x00);

		_tools.isPlaying = true;
		_tools.playingMusic = entry_.data;

		// Finish.
		return true;
	}
	bool stopMusic(Workspace* ws) {
		// Prepare.
		if (!_tools.isPlaying)
			return true;

		// Reset the player states.
		const Device::Ptr &device = ws->audioDevice();
		if (device) {
			device->writeRam((UInt16)_tools.playerIsPlayingLocation.address,   (UInt8)EDITOR_MUSIC_HALT_BANK);
			device->writeRam((UInt16)_tools.playerOrderCursorLocation.address, (UInt8)0xff);
			device->writeRam((UInt16)_tools.playerLineCursorLocation.address,  (UInt8)0xff);
			device->writeRam((UInt16)_tools.playerTicksLocation.address,       (UInt8)0x00);
		}

		// Close the device.
		ws->closeAudioDevice();

		// Stop playing.
		_tools.isPlaying = false;
		_tools.playerStopDelay = 0.0;
		_tools.playerMaxDuration = 0.0;
		_tools.playerPlayedTicks = 0.0;
		_tools.playingMusic = nullptr;
#if EDITOR_MUSIC_UNLOAD_SYMBOLS_ON_FINISH
		_tools.isPlayerConfigLoaded = false;
		_tools.playerConfigText.clear();
		_tools.playerSymbolsText.clear();
		_tools.playerAliasesText.clear();
		_tools.playerIsPlayingLocation = Editing::SymbolLocation();
		_tools.playerOrderCursorLocation = Editing::SymbolLocation();
		_tools.playerLineCursorLocation = Editing::SymbolLocation();
		_tools.playerTicksLocation = Editing::SymbolLocation();
#endif /* EDITOR_MUSIC_UNLOAD_SYMBOLS_ON_FINISH */
		_tools.hasPlayerPlayed = false;
		_tools.lastPlayingOrderIndex = -1;
		_tools.lastPlayingLine = -1;
		_tools.lastPlayingTicks = -1;
		_tools.maxPlayingOrderIndex = -1;
		_tools.maxPlayingLine = -1;
		_tools.maxPlayingTicks = -1;
		_tools.targetPlayingOrderIndex = -1;
		_tools.targetPlayingLine = -1;

		// Finish.
		return true;
	}
	bool updateMusic(Workspace* ws, double delta, bool updatePlayingCursor) {
		// Prepare.
		if (!_tools.isPlaying)
			return true;

		const Device::Ptr &device = ws->audioDevice();
		if (!device)
			return false;

		// Read the player states.
		UInt8 ply = 0;
		device->readRam((UInt16)_tools.playerIsPlayingLocation.address, &ply);
		if (ply != EDITOR_MUSIC_HALT_BANK) { // Playing.
			if (!_tools.hasPlayerPlayed) {
				if (_tools.targetPlayingOrderIndex != -1 && _tools.targetPlayingLine != -1) {
					// Not implemented.
					// device->writeRam((UInt16)_tools.playerOrderCursorLocation.address, (UInt8)(_tools.targetPlayingOrderIndex * 2));
					// device->writeRam((UInt16)_tools.playerLineCursorLocation.address,  (UInt8)_tools.targetPlayingLine);

					_tools.targetPlayingOrderIndex = -1;
					_tools.targetPlayingLine = -1;
				}
			}
		} else { // Not playing or stopped.
			if (_tools.hasPlayerPlayed)
				return false;
			else
				return true;
		}
		UInt8 ord = 0;
		UInt8 ln = 0;
		UInt8 ticks = 0;
		device->readRam((UInt16)_tools.playerOrderCursorLocation.address, &ord);
		device->readRam((UInt16)_tools.playerLineCursorLocation.address,  &ln);
		device->readRam((UInt16)_tools.playerTicksLocation.address,       &ticks);

		ord >>= 1;
		const Music::Song* song = _tools.playingMusic->pointer();
		const Music::Channels &channels = song->channels;
		const Music::Sequence &seq = channels[_tools.active.channel];
		if ((int)ord >= (int)seq.size())
			return true;

		_tools.hasPlayerPlayed = true;

		// Update the playing cursor.
		if (ord != _tools.lastPlayingOrderIndex || ln != _tools.lastPlayingLine) {
			_tools.lastPlayingOrderIndex = ord;
			_tools.lastPlayingLine = ln;

			if (updatePlayingCursor && _tools.playingMusic == object()) {
				_tools.ordering.line = ord;
				_tools.active.sequence = ord;
				_tools.active.set(_tools.active.channel, ln, Tracker::DataTypes::NOTE);
				_tools.target = Tracker::Location(_tools.active.channel, ln);

				_selection.start(_tools.active.sequence, 0, ln, Tracker::DataTypes::NOTE);
				_selection.end(_tools.active.sequence, GBBASIC_MUSIC_CHANNEL_COUNT - 1, ln, Tracker::DataTypes::EFFECT_PARAMETERS);
			}
		}

		// Stop playing if it reaches the end.
		if (ticks != _tools.lastPlayingTicks) {
			_tools.lastPlayingTicks = ticks;

			do {
				if (_tools.maxPlayingOrderIndex == -1 || _tools.maxPlayingLine == -1 || _tools.maxPlayingTicks == -1)
					break;

				const bool stop =
					ord > _tools.maxPlayingOrderIndex ||
					(
						ord == _tools.maxPlayingOrderIndex &&
						(
							ln > _tools.maxPlayingLine ||
							(
								ln == _tools.maxPlayingLine && ticks >= _tools.maxPlayingTicks
							)
						)
					);
				if (!stop)
					break;

				if (_tools.playerStopDelay > 0.0) {
					_tools.playerStopDelay -= delta;
				} else {
					stopMusic(ws);

					return true;
				}
			} while (false);
		}

		// Stop playing if it reaches the duration limit.
		if (_tools.playerMaxDuration > 0.0) {
			_tools.playerPlayedTicks += delta;
			if (_tools.playerPlayedTicks >= _tools.playerMaxDuration) {
				stopMusic(ws);

				return true;
			}
		}

		// Finish.
		return true;
	}
	/**
	 * @param[in, out] tools
	 */
	static Bytes::Ptr compileMusic(Workspace* ws, EditorMusicImpl* self, const MusicAssets::Entry &entry_, Tools &tools, int sequence) {
		// Prepare.
		auto print_ = [ws] (const std::string &msg) -> void {
			ws->print(msg.c_str());
		};
		auto warn_ = [ws] (const std::string &msg) -> void {
			ws->warn(msg.c_str());
		};
		auto error_ = [ws] (const std::string &msg) -> void {
			ws->error(msg.c_str());
		};

		// Get the kernel.
		if (ws->kernels().empty()) {
			self->warn(ws, "No valid music player.", true);

			return nullptr;
		}

		const GBBASIC::Kernel::Ptr &krnl = ws->kernels().front();
		if (!krnl) {
			self->warn(ws, "No valid kernel.", true);

			return nullptr;
		}

		std::string dir;
		Path::split(krnl->path(), nullptr, nullptr, &dir);
		const std::string config = krnl->path();
		const std::string rom = Path::combine(dir.c_str(), krnl->kernelRom().c_str());
		const std::string sym = Path::combine(dir.c_str(), krnl->kernelSymbols().c_str());
		const std::string aliases = Path::combine(dir.c_str(), krnl->kernelAliases().c_str());
		const int bootstrapBank = krnl->bootstrapBank();

		// Find the music stop.
		int maxOrd = -1;
		int maxLn = -1;
		int maxTicks = -1;
		if (entry_.data->pointer()->findStop(maxOrd, maxLn, maxTicks)) {
			tools.maxPlayingOrderIndex = maxOrd;
			tools.maxPlayingLine = maxLn;
			tools.maxPlayingTicks = maxTicks;
		} else {
			tools.maxPlayingOrderIndex = -1;
			tools.maxPlayingLine = -1;
			tools.maxPlayingTicks = -1;
		}

		// Load and parse the symbols.
		if (!tools.isPlayerConfigLoaded) {
			std::string configTxt;
			File::Ptr file(File::create());
			if (!file->open(config.c_str(), Stream::READ)) {
				self->warn(ws, "No valid config.", true);

				return nullptr;
			}
			if (!file->readString(configTxt)) {
				file->close();

				self->warn(ws, "No valid config.", true);

				return nullptr;
			}
			file->close();

			Editing::SymbolTable::Dictionary dict;
			dict[EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_STUB]   = Editing::SymbolLocation();
			dict[EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_STUB] = Editing::SymbolLocation();
			dict[EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_STUB]  = Editing::SymbolLocation();
			dict[EDITOR_MUSIC_PLAYER_TICKS_RAM_STUB]        = Editing::SymbolLocation();
			std::string symTxt;
			std::string aliasesTxt;
			Editing::SymbolTable symbols;
			const bool loaded = symbols.load(
				dict,
				sym, symTxt,
				aliases, aliasesTxt,
				[self, ws] (const char* msg) -> void {
					self->warn(ws, msg, true);
				}
			);
			if (!loaded) {
				self->warn(ws, "No valid symbol.", true);

				return nullptr;
			}

			const Editing::SymbolLocation &isPlayingLoc     = dict[EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_STUB];
			const Editing::SymbolLocation &ordCursorLoc     = dict[EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_STUB];
			const Editing::SymbolLocation &lnCursorLoc      = dict[EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_STUB];
			const Editing::SymbolLocation &ticksLoc         = dict[EDITOR_MUSIC_PLAYER_TICKS_RAM_STUB];
			tools.playerIsPlayingLocation                   = isPlayingLoc;
			tools.playerIsPlayingLocation.address          += EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_OFFSET;
			tools.playerOrderCursorLocation                 = ordCursorLoc;
			tools.playerOrderCursorLocation.address        += EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_OFFSET;
			tools.playerLineCursorLocation                  = lnCursorLoc;
			tools.playerLineCursorLocation.address         += EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_OFFSET;
			tools.playerTicksLocation                       = ticksLoc;
			tools.playerTicksLocation.address              += EDITOR_MUSIC_PLAYER_TICKS_RAM_OFFSET;
			tools.isPlayerConfigLoaded                      = true;
			tools.playerConfigText                          = configTxt;
			tools.playerSymbolsText                         = symTxt;
			tools.playerAliasesText                         = aliasesTxt;
		}

		// Compile.
		print_("Begin compiling for music playback.");

		AssetsBundle::Ptr assets(new AssetsBundle());
		std::string src = RES_CODE_PLAY_MUSIC;
		if (sequence > 0) {
			src += Text::format(
				RES_CODE_SET_MUSIC_POSITION,
				{
					Text::toString(sequence)
				}
			);
		}
		assets->code.add(src);
		assets->music.add(entry_);

		const Bytes::Ptr rom_ = Workspace::compile(
			config, tools.playerConfigText,
			rom,
			sym, tools.playerSymbolsText,
			aliases, tools.playerAliasesText,
			"Music", assets,
			bootstrapBank,
			print_, warn_, error_
		);

		print_("End compiling for music playback.");

		if (!rom_)
			return nullptr;

		print_("Ok.");

		// Finish.
		return rom_;
	}
};

EditorMusic* EditorMusic::create(void) {
	EditorMusicImpl* result = new EditorMusicImpl();

	return result;
}

void EditorMusic::destroy(EditorMusic* ptr) {
	EditorMusicImpl* impl = static_cast<EditorMusicImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
