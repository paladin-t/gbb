/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_layered.h"
#include "commands_sfx.h"
#include "editor_sfx.h"
#include "theme.h"
#include "workspace.h"
#include "resource/inline_resource.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#include "../utils/marker.h"
#include "../utils/shapes.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_SFX_UNLOAD_SYMBOLS_ON_FINISH
#	define EDITOR_SFX_UNLOAD_SYMBOLS_ON_FINISH 1
#endif /* EDITOR_SFX_UNLOAD_SYMBOLS_ON_FINISH */

/* ===========================================================================} */

/*
** {===========================================================================
** Sfx editor
*/

class EditorSfxImpl : public EditorSfx {
private:
	struct ShapeRenderingJob {
		int page = -1;
		Sfx::Sound sound;
		Image::Ptr image = nullptr;

		Device::Ptr device = nullptr;
		SDL_AudioCVT converter;
		size_t converterBufferLength = 0;

		ShapeRenderingJob() {
		}
		ShapeRenderingJob(int pg, const Sfx::Sound &snd) : page(pg), sound(snd) {
		}

		void open(Workspace* ws) {
			device = Device::Ptr(Device::create(Device::CoreTypes::BINJGB, ws));

			SDL_AudioSpec devSpec;
			SDL_AudioSpec dstSpec;
			memset(&devSpec, 0, sizeof(SDL_AudioSpec));
			memset(&dstSpec, 0, sizeof(SDL_AudioSpec));
			memset(&converter, 0, sizeof(SDL_AudioCVT));

			memcpy(&devSpec, device->audioSpecification(), sizeof(SDL_AudioSpec));
			dstSpec = devSpec;
			dstSpec.freq = 11000;
			dstSpec.format = AUDIO_U8;
			dstSpec.channels = 1;
			dstSpec.silence = 127;

			const int ret = SDL_BuildAudioCVT(
				&converter,
				devSpec.format, devSpec.channels, devSpec.freq,
				dstSpec.format, dstSpec.channels, dstSpec.freq
			);
			(void)ret;
			if (converter.needed) {
				fprintf(
					stdout,
					"Need audio conversion for SFX shape rendering.\n"
					"  Source format: %d, channels: %d, frequency: %d\n"
					"  Target format: %d, channels: %d, frequency: %d\n",
					devSpec.format, devSpec.channels, devSpec.freq,
					dstSpec.format, dstSpec.channels, dstSpec.freq
				);
				converter.len = dstSpec.size;
			}
		}
		void close(void) {
			device = nullptr;
		}
	};
	struct ShapeRenderingBatch {
		typedef std::map<uintptr_t, ShapeRenderingJob> Dictionary;

		typedef std::function<void(uintptr_t, const ShapeRenderingJob &, int)> ConstEnumerator;
		typedef std::function<void(uintptr_t, ShapeRenderingJob &, int)> Enumerator;

		Dictionary dictionary;

		ShapeRenderingBatch() {
		}

		int count(void) const {
			return (int)dictionary.size();
		}
		bool empty(void) const {
			return dictionary.empty();
		}
		bool add(uintptr_t ptr, int page, const Sfx::Sound &snd) {
			Dictionary::const_iterator it = dictionary.find(ptr);
			if (it != dictionary.end())
				return false;

			dictionary.insert(std::make_pair(ptr, ShapeRenderingJob(page, snd)));

			return true;
		}
		void clear(void) {
			dictionary.clear();
		}

		int foreach(ConstEnumerator enumerator) const {
			int result = 0;
			for (Dictionary::const_iterator it = dictionary.begin(); it != dictionary.end(); ++it) {
				enumerator(it->first, it->second, result);
				++result;
			}

			return result;
		}
		int foreach(Enumerator enumerator) {
			int result = 0;
			for (Dictionary::iterator it = dictionary.begin(); it != dictionary.end(); ++it) {
				enumerator(it->first, it->second, result);
				++result;
			}

			return result;
		}
	};

private:
	int _opened = 0;

	Project* _project = nullptr; // Foreign.
	int _index = -1;
	CommandQueue* _commands = nullptr;
	struct {
		Workspace* workspace = nullptr;
		ShapeRenderingBatch shapeBatch;
		int inQueueCount = 0;

		void open(Workspace* ws) {
			workspace = ws;
		}
		void close(void) {
			inQueueCount = 0;
			shapeBatch.clear();
			workspace = nullptr;
		}

		void wait(void) {
			while (inQueueCount > 0)
				workspace->wait();

			shapeBatch.clear();
		}
	} _async;

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
		Marker<bool> shaping = Marker<bool>(false);

		void clear(void) {
			text.clear();
			filled = false;
			shaping = Marker<bool>(false);
		}
	} _status;
	float _statusWidth = 0.0f;
	struct {
		Text::Array warnings;
		std::string text;
		mutable RecursiveMutex lock;

		bool empty(void) const {
			LockGuard<decltype(lock)> guard(lock);

			return warnings.empty();
		}
		void clear(void) {
			LockGuard<decltype(lock)> guard(lock);

			warnings.clear();
			text.clear();
		}
		bool add(const std::string &txt) {
			LockGuard<decltype(lock)> guard(lock);

			if (std::find(warnings.begin(), warnings.end(), txt) != warnings.end())
				return false;

			warnings.push_back(txt);

			flush();

			return true;
		}
		bool remove(const std::string &txt) {
			LockGuard<decltype(lock)> guard(lock);

			Text::Array::iterator it = std::find(warnings.begin(), warnings.end(), txt);
			if (it == warnings.end())
				return false;

			warnings.erase(it);

			flush();

			return true;
		}
		void flush(void) {
			LockGuard<decltype(lock)> guard(lock);

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
	std::function<void(const Command*)> _refresh = nullptr;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _estimated;

	struct Ref : public Editor::Ref {
		void clear(void) {
			// Do nothing.
		}
	} _ref;
	std::function<void(int)> _setView = nullptr;
	std::function<void(BaseAssets::Entry*, int)> _pageAdded = nullptr;
	std::function<void(BaseAssets::Entry*, int)> _pageRemoved = nullptr;
	struct Tools {
		Math::Vec2i contentSize;
		int locateToPage = -1;
		std::string name;
		std::string soundString;
		std::string newSoundString;

		bool focused = false;
		int needToRefreshShapes = 0;
		bool isPlaying = false;
		int playingIndex = -1;
		double playerStopDelay = 0.0;
		double playerMaxDuration = 0.0;
		double playerPlayedTicks = 0.0;
		bool isPlayerConfigLoaded = false;
		std::string playerConfigText;
		std::string playerSymbolsText;
		std::string playerAliasesText;
		Editing::SymbolLocation playerIsPlayingLocation;
		bool hasPlayerPlayed = false;
		bool toShowImportMenu = false;
		Sfx::VgmOptions vgmImportOptions;
		Sfx::FxHammerOptions fxHammerImportOptions;

		void clear(void) {
			contentSize = Math::Vec2i();
			locateToPage = -1;
			name.clear();
			soundString.clear();
			newSoundString.clear();

			focused = false;
			needToRefreshShapes = 0;
			isPlaying = false;
			playingIndex = -1;
			playerStopDelay = 0.0;
			playerMaxDuration = 0.0;
			playerPlayedTicks = 0.0;
			isPlayerConfigLoaded = false;
			playerConfigText.clear();
			playerSymbolsText.clear();
			playerAliasesText.clear();
			playerIsPlayingLocation = Editing::SymbolLocation();
			hasPlayerPlayed = false;
			toShowImportMenu = false;
			vgmImportOptions = Sfx::VgmOptions();
			fxHammerImportOptions = Sfx::FxHammerOptions();
		}
	} _tools;

	const char* const* BYTE_DEC_NUMBERS = nullptr;

public:
	EditorSfxImpl(Window* wnd, Renderer* rnd, Workspace* ws, Project* prj) {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Sfx::AddPage>()
			->reg<Commands::Sfx::DeletePage>()
			->reg<Commands::Sfx::DeleteAllPages>()
			->reg<Commands::Sfx::MoveBackward>()
			->reg<Commands::Sfx::MoveForward>()
			->reg<Commands::Sfx::SetName>()
			->reg<Commands::Sfx::SetSound>()
			->reg<Commands::Sfx::Import>()
			->reg<Commands::Sfx::ImportAsNew>();

		_async.open(ws);

		BYTE_DEC_NUMBERS = ws->theme()->generic_ByteDec();

		_project = prj;

		_setView = std::bind(&EditorSfxImpl::changeView, this, wnd, rnd, ws, std::placeholders::_1);
		_pageAdded = std::bind(&EditorSfxImpl::pageAdded, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2);
		_pageRemoved = std::bind(&EditorSfxImpl::pageRemoved, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2);
	}
	virtual ~EditorSfxImpl() override {
		_project = nullptr;

		_setView = nullptr;
		_pageAdded = nullptr;
		_pageRemoved = nullptr;

		_async.close();

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window*, class Renderer*, class Workspace* ws, class Project* /* project */, int index, unsigned /* refCategory */, int /* refIndex */) override {
		auto initialize = [ws, this] (int index) -> void {
			int another = -1;
			if (sound(index)->name.empty() || !_project->canRenameSfx(index, sound(index)->name, &another)) {
				if (sound(index)->name.empty()) {
					const std::string msg = Text::format(ws->theme()->warning_SfxSfxNameIsEmptyAtPage(), { Text::toString(index) });
					warn(ws, msg, true);
				} else {
					const std::string msg = Text::format(ws->theme()->warning_SfxDuplicateSfxNameAtPages(), { sound(index)->name, Text::toString(index), Text::toString(another) });
					warn(ws, msg, true);
				}
				sound(index)->name = _project->getUsableSfxName(index); // Unique name.
				_project->hasDirtyAsset(true);
			}

			refreshSound();
		};

		if (_opened) {
			++_opened;

			_index = index;

			initialize(index);

			fprintf(stdout, "SFX editor (shared) opened: %d.\n", index);

			return;
		}
		++_opened;

		_index = index;

		_refresh = std::bind(&EditorSfxImpl::refresh, this, ws, std::placeholders::_1);

		++_tools.needToRefreshShapes;

		initialize(index);

		fprintf(stdout, "SFX editor opened: #%d.\n", index);
	}
	virtual void close(int index) override {
		SfxAssets::Entry* entry_ = entry(index);
		if (entry_) {
			entry_->shape = nullptr;
		}

		--_opened;
		if (_opened) {
			fprintf(stdout, "SFX editor (shared) closed: %d.\n", index);

			return;
		}

		fprintf(stdout, "SFX editor closed: #%d.\n", index);

		_index = -1;

		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;
		_estimated.clear();

		_ref.clear();
		_tools.clear();

		_async.wait();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		// Do nothing.
	}
	virtual void leave(class Workspace* ws) override {
		stopSfx(ws);
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
		// Do nothing.
	}
	virtual void cut(void) override {
		// Do nothing.
	}
	virtual bool pastable(void) const override {
		return false;
	}
	virtual void paste(void) override {
		// Do nothing.
	}
	virtual void del(bool) override {
		// Do nothing.
	}
	virtual bool selectable(void) const override {
		return false;
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

		const Commands::Layered::Layered* pcmd = Command::as<Commands::Layered::Layered>(cmd);
		const int index = pcmd->sub();

		_commands->redo(object(index), Variant((void*)_project));

		_refresh(cmd);

		_project->toPollEditor(true);
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		const Commands::Layered::Layered* pcmd = Command::as<Commands::Layered::Layered>(cmd);
		const int index = pcmd->sub();

		_commands->undo(object(index), Variant((void*)_project));

		_refresh(cmd);

		_project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
			}

			return Variant(true);
		case CLEAR_UNDO_REDO_RECORDS:
			_commands->clear();

			return Variant(true);
		case IMPORT:
			_tools.toShowImportMenu = true;

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

		if (!entry(_index) || !object(_index) || !sound(_index)) {
			if (_tools.toShowImportMenu) {
				_tools.toShowImportMenu = false;

				ImGui::OpenPopup("@Imp");
			}

			statusBarMenu(wnd, rnd, ws);

			return;
		}

		if (_tools.locateToPage != -1 && _tools.locateToPage != _project->activeSfxIndex()) {
			_project->activeSfxIndex(_tools.locateToPage);
			statusInvalidated();

			refreshSound();
		} else if (_index != _project->activeSfxIndex()) {
			_index = _project->activeSfxIndex();
			statusInvalidated();

			refreshSound();
		}

		refreshShapes(rnd, ws);
		updateShapeBatch(rnd, ws, this, &_async.shapeBatch);

		updateSfx(ws, delta);

		shortcuts(wnd, rnd, ws);

		const Ref::Splitter splitter = _ref.split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		bool toImport = false;
		bool toExport = false;

		bool cleared = false;

		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - statusBarHeight), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
		{
			SfxAssets &sfxAssets = _project->assets()->sfx;
			const bool changed = Editing::sfx(
				rnd, ws,
				&sfxAssets,
				splitter.first,
				&_index,
				&_tools.contentSize, &_tools.locateToPage,
				_tools.isPlaying ? &_tools.playingIndex : nullptr,
#if EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED
				_project->preferencesSfxShowSoundShape(),
#else /* EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED */
				false,
#endif /* EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED */
				BYTE_DEC_NUMBERS,
				[wnd, rnd, ws, this, &toImport, &toExport] (int idx, Editing::Operations op) -> void {
					switch (op) {
					case Editing::Operations::PLAY:
						ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, idx);

						playSfx(ws, idx);

						break;
					case Editing::Operations::STOP:
						ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, idx);

						stopSfx(ws);

						break;
					case Editing::Operations::IMPORT:
						toImport = true;

						ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, idx);

						break;
					case Editing::Operations::EXPORT:
						toExport = true;

						ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, idx);

						break;
					default:
						GBBASIC_ASSERT(false && "Impossible.");

						break;
					}
				}
			);
			if (changed) {
				_tools.locateToPage = _index;
				if (_index != _project->activeSfxIndex()) {
					_project->activeSfxIndex(_index);
					statusInvalidated();

					refreshSound();
				}
			}

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			if (!context(wnd, rnd, ws))
				cleared = true;
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		if (!cleared) {
			const float spwidth = _ref.windowWidth(splitter.second);
			if (
				Editing::Tools::namable(
					rnd, ws,
					sound(_index)->name, _tools.name,
					spwidth,
					nullptr, nullptr,
					ws->theme()->dialogPrompt_Name().c_str()
				)
			) {
				if (_project->canRenameSfx(_index, _tools.name, nullptr)) {
					Command* cmd = enqueue<Commands::Sfx::SetName>()
						->with(_tools.name)
						->with(_setView, _index)
						->exec(object(_index), Variant((void*)_project));

					_refresh(cmd);
				} else {
					_tools.name = sound(_index)->name;

					warn(ws, ws->theme()->warning_SfxNameIsAlreadyInUse(), true);
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::sound(
					rnd, ws,
					_tools.soundString, _tools.newSoundString,
					spwidth,
					nullptr, 0,
					nullptr, nullptr,
					ws->theme()->dialogPrompt_Data().c_str()
				)
			) {
				Sfx::Sound sound_ = *sound(_index);
				Command* cmd = enqueue<Commands::Sfx::SetSound>()
					->with(sound_)
					->with(_setView, _index)
					->exec(object(_index), Variant((void*)_project));

				_refresh(cmd);
			}

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows); // Ignore shortcuts when the window is not focused.
			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		if (toImport)
			ImGui::OpenPopup("@ImpOne");
		if (toExport)
			ImGui::OpenPopup("@XptOne");
		itemMenu(wnd, rnd, ws);

		refreshStatus(wnd, rnd, ws);
		renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);
	}

	virtual void statusInvalidated(void) override {
		_page.filled = false;
		_status.filled = false;
		_estimated.filled = false;
	}

	virtual void added(BaseAssets::Entry* entry, int index) override {
		_pageAdded(entry, index);
	}
	virtual void removed(BaseAssets::Entry* entry, int index) override {
		_pageRemoved(entry, index);
	}

	virtual void played(class Renderer*, class Workspace* ws) override {
		stopSfx(ws);
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
					index = _project->sfxPageCount() - 1;
				if (index != _index) {
					ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, index);
					_tools.locateToPage = index;
				}
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->sfxPageCount())
					index = 0;
				if (index != _index) {
					ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, index);
					_tools.locateToPage = index;
				}
			}
		}
	}

	bool context(Window* wnd, Renderer* rnd, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		bool result = true;

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Ctx")) {
			const bool canMoveBackward = _index >= 1;
			const bool canMoveForward = _index <= _project->sfxPageCount() - 2;

			if (ImGui::MenuItem(ws->theme()->menu_MoveBackward(), nullptr, nullptr, canMoveBackward)) {
				Command* cmd = enqueue<Commands::Sfx::MoveBackward>()
					->with(_index)
					->with(_setView, _index)
					->exec(object(_index), Variant((void*)_project));

				_refresh(cmd);
			}
			if (ImGui::MenuItem(ws->theme()->menu_MoveForward(), nullptr, nullptr, canMoveForward)) {
				Command* cmd = enqueue<Commands::Sfx::MoveForward>()
					->with(_index)
					->with(_setView, _index)
					->exec(object(_index), Variant((void*)_project));

				_refresh(cmd);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menu_Delete())) {
				const Sfx::Sound* snd = sound(_index);

				Command* cmd = enqueue<Commands::Sfx::DeletePage>()
					->with(wnd, rnd, ws, _index, *snd, true)
					->with(_setView, _index)
					->exec(object(_index), Variant((void*)_project));

				if (_refresh)
					_refresh(cmd);

				if (_project->sfxPageCount() == 0)
					result = false;
			}
			if (ImGui::MenuItem(ws->theme()->menu_DeleteAll())) {
				_async.wait();

				Sfx::Sound::Array snds;
				for (int i = 0; i < _project->sfxPageCount(); ++i) {
					const Sfx::Sound* snd = sound(i);
					snds.push_back(*snd);
				}

				Command* cmd = enqueue<Commands::Sfx::DeleteAllPages>()
					->with(wnd, rnd, ws, snds)
					->with(_setView, _index)
					->exec(object(_index), Variant((void*)_project));

				if (_refresh)
					_refresh(cmd);

				_project->hasDirtyAsset(true);

				result = false;
			}

			ImGui::EndPopup();
		}

		return result;
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
			const SfxAssets::Entry* entry_ = entry(_index);
			if (entry_) {
				_estimated.text = " " + Text::toScaledBytes(entry_->estimateFinalByteCount());
				_estimated.filled = true;
			}
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
				index = _project->sfxPageCount() - 1;
			if (index != _index) {
				ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, index);
				_tools.locateToPage = index;
			}
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
			if (index >= _project->sfxPageCount())
				index = 0;
			if (index != _index) {
				ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, index);
				_tools.locateToPage = index;
			}
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(style.FramePadding.x, 0));
		ImGui::SameLine();
#if EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED
		if (ImGui::Checkbox(ws->theme()->dialogPrompt_Shape(), &_project->preferencesSfxShowSoundShape())) {
			toggleShapes(_project->preferencesSfxShowSoundShape());

			_project->hasDirtyAsset(true);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipAudio_SfxShowSoundShape());
		}
		ImGui::SameLine();
#endif /* EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED */
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_status.text);
		ImGui::SameLine();
		ImGui::TextUnformatted(_estimated.text);
		ImGui::SameLine();
		do {
			_status.shaping = !!_async.inQueueCount;
			const Marker<bool>::Values shaping = _status.shaping.values();
			if (!shaping.first && shaping.second) {
				if (_statusWidth > 0)
					_statusWidth -= 13 + style.ItemInnerSpacing.x * 2;
			} else if (shaping.first && !shaping.second) {
				if (_statusWidth > 0)
					_statusWidth += 13 + style.ItemInnerSpacing.x * 2;
			}
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - _statusWidth);
			if (shaping.first) {
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::ImageButton(ws->theme()->iconWorking()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Refreshing().c_str());
				ImGui::PopStyleColor(2);
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
			}
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
			if (ImGui::ImageButton(ws->theme()->iconImport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_ImportAsNew().c_str())) {
				ImGui::OpenPopup("@Imp");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconExport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Export().c_str())) {
				ImGui::OpenPopup("@Xpt");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconMusic()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_Music().c_str())) {
				ws->category(Workspace::Categories::MUSIC);
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			do {
				WIDGETS_SELECTION_GUARD(ws->theme());

				if (ImGui::ImageButton(ws->theme()->iconSfx()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_Sfx().c_str())) {
					// Do nothing.
				}
			} while (false);
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
			const Text::Array &assetNames = ws->getSfxPageNames();
			const int n = _project->sfxPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg)) {
						ws->changePage(wnd, rnd, _project, Workspace::Categories::SFX, i);
						_tools.locateToPage = i;
					}
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				if (Platform::hasClipboardText()) {
					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					importFromJsonString(wnd, rnd, ws, txt, true);
				} else {
					ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);
				}
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
						pfd::opt::multiselect
					);
					if (open.result().empty())
						break;

					if (open.result().size() == 1) {
						std::string path = open.result().front();
						if (path.empty())
							break;

						Path::uniform(path);

						importFromJsonFile(wnd, rnd, ws, path, true);
					} else {
						for (int i = 0; i < (int)open.result().size(); ++i) {
							std::string path = open.result()[i];
							if (path.empty())
								continue;
							Path::uniform(path);

							importFromJsonFile(wnd, rnd, ws, path, true);
						}
					}
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_VgmFile())) {
				ImGui::FileResolverPopupBox::ConfirmedHandler_Path confirm(
					[wnd, rnd, ws, this] (const char* path) -> void {
						const std::string path_ = path;
						importFromVgmFile(wnd, rnd, ws, path_, true);

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CanceledHandler cancel(
					[ws] (void) -> void {
						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CustomHandler custom(
					[rnd, ws, this] (ImGui::FileResolverPopupBox* /* popup */, float width) -> void {
						ImGuiStyle &style = ImGui::GetStyle();

						constexpr const float GAP = 4;
						const float width_ = (width - GAP) * 0.5f;
						const float x0 = ImGui::GetCursorPosX();
						const float x1 = x0 + width * 0.5f;

						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr1x(), &_tools.vgmImportOptions.noNr1x);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr2x(), &_tools.vgmImportOptions.noNr2x);

						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr3x(), &_tools.vgmImportOptions.noNr3x);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr4x(), &_tools.vgmImportOptions.noNr4x);

						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr5x(), &_tools.vgmImportOptions.noNr5x);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoInit(), &_tools.vgmImportOptions.noInit);

						{
							const float oldX_ = ImGui::GetCursorPosX();
							ImGui::AlignTextToFramePadding();
							ImGui::TextUnformatted(ws->theme()->dialogPrompt_Delay());
							ImGui::SameLine();

							VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

							const float width__ = width_ - (ImGui::GetCursorPosX() - oldX_);
							ImGui::PushID("Dly");
							ImGui::IntegerModifier(rnd, ws->theme(), &_tools.vgmImportOptions.delay, 1, 16, width__);
							ImGui::PopID();
						}
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoWave(), &_tools.vgmImportOptions.noWave);
					},
					nullptr
				);
				ws->showExternalFileBrowser(
					rnd,
					ws->theme()->windowSfx(),
					ws->theme()->generic_Path(),
					GBBASIC_VGM_FILE_FILTER,
					true,
					confirm,
					cancel,
					nullptr,
					custom
				);
			}
			if (ImGui::MenuItem(ws->theme()->menu_WavFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_WAV_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;

					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					importFromWavFile(wnd, rnd, ws, path, true);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_FxHammerFile())) {
				ImGui::FileResolverPopupBox::ConfirmedHandler_PathVec2i confirm(
					[wnd, rnd, ws, this] (const char* path, const Math::Vec2i &index_) -> void {
						const std::string path_ = path;
						importFromFxHammerFile(wnd, rnd, ws, path_, index_, true);

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CanceledHandler cancel(
					[ws] (void) -> void {
						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::SelectedHandler select(
					[ws] (ImGui::FileResolverPopupBox* popup) -> bool {
						const std::string &path = popup->path();

						Bytes::Ptr bytes(Bytes::create());
						File::Ptr file(File::create());
						if (!file->open(path.c_str(), Stream::READ)) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return false;
						}
						if (!file->readBytes(bytes.get())) {
							file->close();

							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return false;
						}
						file->close();

						const int n = Sfx::countFxHammer(bytes.get());
						if (n < 1) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return false;
						}

						const int max = (int)n - 1;
						popup->vec2iMaxValue(Math::Vec2i(max, max));
						popup->vec2iValue(Math::Vec2i(0, max));

						return true;
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CustomHandler custom(
					[rnd, ws, this] (ImGui::FileResolverPopupBox* /* popup */, float width) -> void {
						ImGuiStyle &style = ImGui::GetStyle();

						constexpr const float GAP = 4;
						const float width_ = (width - GAP) * 0.5f;
						const float x0 = ImGui::GetCursorPosX();
						const float x1 = x0 + width * 0.5f;

						ImGui::Checkbox(ws->theme()->dialogPrompt_CutSound(), &_tools.fxHammerImportOptions.cutSound);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_UsePan(), &_tools.fxHammerImportOptions.usePan);

						{
							const float oldX_ = ImGui::GetCursorPosX();
							ImGui::AlignTextToFramePadding();
							ImGui::TextUnformatted(ws->theme()->dialogPrompt_Delay());
							ImGui::SameLine();

							VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

							const float width__ = width_ - (ImGui::GetCursorPosX() - oldX_);
							ImGui::PushID("Dly");
							ImGui::IntegerModifier(rnd, ws->theme(), &_tools.fxHammerImportOptions.delay, 1, 16, width__);
							ImGui::PopID();
						}
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_Optimize(), &_tools.fxHammerImportOptions.optimize);
					},
					nullptr
				);
				ws->showExternalFileBrowser(
					rnd,
					ws->theme()->windowSfx(),
					ws->theme()->generic__Path(),
					GBBASIC_FX_HAMMER_FILE_FILTER,
					true,
					Math::Vec2i(0, 0), Math::Vec2i(0, 0), Math::Vec2i(0, 0), ws->theme()->dialogPrompt_Index().c_str(), " -",
					confirm,
					cancel,
					select,
					custom,
					nullptr, nullptr
				);
			}
			if (ws->sfxCount() > 0) {
#if WORKSPACE_SFX_MENU_ENABLED
				if (ImGui::BeginMenu(ws->theme()->menu_Library())) {
					std::string path;
					if (ImGui::SfxMenu(ws->sfx(), path)) {
						importFromLibraryFile(wnd, rnd, ws, path, true);
					}

					ImGui::EndMenu();
				}
#else /* WORKSPACE_SFX_MENU_ENABLED */
				if (ImGui::MenuItem(ws->theme()->menu_FromLibrary())) {
					const std::string sfxDirectory = Path::absoluteOf(WORKSPACE_SFX_DIR);
					std::string defaultPath = sfxDirectory;
					DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(sfxDirectory.c_str());
					FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", false);
					if (fileInfos->count() != 0) {
						FileInfo::Ptr fileInfo = fileInfos->get(0);
						defaultPath = fileInfo->fullPath();
#	if defined GBBASIC_OS_WIN
						defaultPath = Text::replace(defaultPath, "/", "\\");
#	endif /* GBBASIC_OS_WIN */
					}

					do {
						pfd::open_file open(
							GBBASIC_TITLE,
							defaultPath,
							GBBASIC_JSON_FILE_FILTER,
							pfd::opt::multiselect
						);
						if (open.result().empty())
							break;

						if (open.result().size() == 1) {
							std::string path = open.result().front();
							if (path.empty())
								break;
							Path::uniform(path);

							if (!importFromLibraryFile(wnd, rnd, ws, path, true))
								break;
						} else {
							for (int i = 0; i < (int)open.result().size(); ++i) {
								std::string path = open.result()[i];
								if (path.empty())
									continue;
								Path::uniform(path);

								importFromLibraryFile(wnd, rnd, ws, path, true);
							}
						}
					} while (false);
				}
#endif /* WORKSPACE_SFX_MENU_ENABLED */
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::MenuItem(ws->theme()->menu_Code())) {
				do {
					std::string txt;
					if (!entry(_index)->serializeBasic(txt, _index))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					std::string txt;
					if (!entry(_index)->serializeJson(txt, true))
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
					const std::string &name = sound(_index)->name;
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						name.empty() ? "gbbasic-sfx.json" : Text::sanitizeFilename(name) + ".json",
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
					if (!entry(_index)->serializeJson(txt, false))
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
	void itemMenu(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		// Importing.
		if (ImGui::BeginPopup("@ImpOne")) {
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				if (Platform::hasClipboardText()) {
					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					importFromJsonString(wnd, rnd, ws, txt, false);
				} else {
					ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);
				}
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

					importFromJsonFile(wnd, rnd, ws, path, false);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_VgmFile())) {
				ImGui::FileResolverPopupBox::ConfirmedHandler_Path confirm(
					[wnd, rnd, ws, this] (const char* path) -> void {
						const std::string path_ = path;
						importFromVgmFile(wnd, rnd, ws, path_, false);

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CanceledHandler cancel(
					[ws] (void) -> void {
						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CustomHandler custom(
					[rnd, ws, this] (ImGui::FileResolverPopupBox* /* popup */, float width) -> void {
						ImGuiStyle &style = ImGui::GetStyle();

						constexpr const float GAP = 4;
						const float width_ = (width - GAP) * 0.5f;
						const float x0 = ImGui::GetCursorPosX();
						const float x1 = x0 + width * 0.5f;

						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr1x(), &_tools.vgmImportOptions.noNr1x);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr2x(), &_tools.vgmImportOptions.noNr2x);

						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr3x(), &_tools.vgmImportOptions.noNr3x);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr4x(), &_tools.vgmImportOptions.noNr4x);

						ImGui::Checkbox(ws->theme()->dialogPrompt_NoNr5x(), &_tools.vgmImportOptions.noNr5x);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoInit(), &_tools.vgmImportOptions.noInit);

						{
							const float oldX_ = ImGui::GetCursorPosX();
							ImGui::AlignTextToFramePadding();
							ImGui::TextUnformatted(ws->theme()->dialogPrompt_Delay());
							ImGui::SameLine();

							VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

							const float width__ = width_ - (ImGui::GetCursorPosX() - oldX_);
							ImGui::PushID("Dly");
							ImGui::IntegerModifier(rnd, ws->theme(), &_tools.vgmImportOptions.delay, 1, 16, width__);
							ImGui::PopID();
						}
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_NoWave(), &_tools.vgmImportOptions.noWave);
					},
					nullptr
				);
				ws->showExternalFileBrowser(
					rnd,
					ws->theme()->windowSfx(),
					ws->theme()->generic_Path(),
					GBBASIC_VGM_FILE_FILTER,
					true,
					confirm,
					cancel,
					nullptr,
					custom
				);
			}
			if (ImGui::MenuItem(ws->theme()->menu_WavFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_WAV_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;

					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					importFromWavFile(wnd, rnd, ws, path, false);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_FxHammerFile())) {
				ImGui::FileResolverPopupBox::ConfirmedHandler_PathInteger confirm(
					[wnd, rnd, ws, this] (const char* path, int index) -> void {
						const std::string path_ = path;
						importFromFxHammerFile(wnd, rnd, ws, path_, Math::Vec2i(index, index), false);

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CanceledHandler cancel(
					[ws] (void) -> void {
						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::SelectedHandler select(
					[ws] (ImGui::FileResolverPopupBox* popup) -> bool {
						const std::string &path = popup->path();

						Bytes::Ptr bytes(Bytes::create());
						File::Ptr file(File::create());
						if (!file->open(path.c_str(), Stream::READ)) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return false;
						}
						if (!file->readBytes(bytes.get())) {
							file->close();

							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return false;
						}
						file->close();

						const int n = Sfx::countFxHammer(bytes.get());
						if (n < 1) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return false;
						}

						const int max = (int)n - 1;
						popup->integerMaxValue(max);

						return true;
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CustomHandler custom(
					[rnd, ws, this] (ImGui::FileResolverPopupBox* /* popup */, float width) -> void {
						ImGuiStyle &style = ImGui::GetStyle();

						constexpr const float GAP = 4;
						const float width_ = (width - GAP) * 0.5f;
						const float x0 = ImGui::GetCursorPosX();
						const float x1 = x0 + width * 0.5f;

						ImGui::Checkbox(ws->theme()->dialogPrompt_CutSound(), &_tools.fxHammerImportOptions.cutSound);
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_UsePan(), &_tools.fxHammerImportOptions.usePan);

						{
							const float oldX_ = ImGui::GetCursorPosX();
							ImGui::AlignTextToFramePadding();
							ImGui::TextUnformatted(ws->theme()->dialogPrompt_Delay());
							ImGui::SameLine();

							VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

							const float width__ = width_ - (ImGui::GetCursorPosX() - oldX_);
							ImGui::PushID("Dly");
							ImGui::IntegerModifier(rnd, ws->theme(), &_tools.fxHammerImportOptions.delay, 1, 16, width__);
							ImGui::PopID();
						}
						ImGui::SameLine();

						ImGui::SetCursorPosX(x1);
						ImGui::Checkbox(ws->theme()->dialogPrompt_Optimize(), &_tools.fxHammerImportOptions.optimize);
					},
					nullptr
				);
				ws->showExternalFileBrowser(
					rnd,
					ws->theme()->windowSfx(),
					ws->theme()->generic__Path(),
					GBBASIC_FX_HAMMER_FILE_FILTER,
					true,
					0, 0, 0, ws->theme()->dialogPrompt_Index().c_str(),
					false,
					confirm,
					cancel,
					select,
					custom
				);
			}
			if (ws->sfxCount() > 0) {
#if WORKSPACE_SFX_MENU_ENABLED
				if (ImGui::BeginMenu(ws->theme()->menu_Library())) {
					std::string path;
					if (ImGui::SfxMenu(ws->sfx(), path)) {
						importFromLibraryFile(wnd, rnd, ws, path, false);
					}

					ImGui::EndMenu();
				}
#else /* WORKSPACE_SFX_MENU_ENABLED */
				if (ImGui::MenuItem(ws->theme()->menu_FromLibrary())) {
					const std::string sfxDirectory = Path::absoluteOf(WORKSPACE_SFX_DIR);
					std::string defaultPath = sfxDirectory;
					do {
						DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(sfxDirectory.c_str());
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

						importFromLibraryFile(wnd, rnd, ws, path, false);
					} while (false);
				}
#endif /* WORKSPACE_SFX_MENU_ENABLED */
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@XptOne")) {
			if (ImGui::MenuItem(ws->theme()->menu_Code())) {
				do {
					std::string txt;
					if (!entry(_index)->serializeBasic(txt, _index))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					std::string txt;
					if (!entry(_index)->serializeJson(txt, true))
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
					const std::string &name = sound(_index)->name;
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						name.empty() ? "gbbasic-sfx.json" : Text::sanitizeFilename(name) + ".json",
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
					if (!entry(_index)->serializeJson(txt, false))
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
	void changeView(Window*, Renderer*, Workspace*, int view) {
		if (_index == view)
			return;

		_index = view;
		if (_index != _tools.locateToPage) {
			_tools.locateToPage = _index;
		}
	}

	void pageAdded(Window* wnd, Renderer* rnd, Workspace* ws, BaseAssets::Entry* entry, int index) {
		const SfxAssets::Entry* entry_ = (SfxAssets::Entry*)entry;
		const Sfx::Ptr &sfx = entry_->data;
		const Sfx::Sound* snd = sfx->pointer();

		Command* cmd = enqueue<Commands::Sfx::AddPage>()
			->with(wnd, rnd, ws, index, *snd)
			->with(_setView, index)
			->exec(object(index), Variant((void*)_project));

		_refresh(cmd);
	}
	void pageRemoved(Window* wnd, Renderer* rnd, Workspace* ws, BaseAssets::Entry* entry, int index) {
		const SfxAssets::Entry* entry_ = (SfxAssets::Entry*)entry;
		const Sfx::Ptr &sfx = entry_->data;
		const Sfx::Sound* snd = sfx->pointer();

		Command* cmd = enqueue<Commands::Sfx::DeletePage>()
			->with(wnd, rnd, ws, index, *snd, false)
			->with(_setView, index)
			->exec(object(index), Variant((void*)_project));

		_refresh(cmd);
	}

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "SFX editor: ";
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
	void refresh(Workspace* ws, const Command* cmd) {
		const bool recalcSize =
			Command::is<Commands::Sfx::Import>(cmd) ||
			Command::is<Commands::Sfx::ImportAsNew>(cmd);
		const bool refreshIndex =
			Command::is<Commands::Sfx::AddPage>(cmd) ||
			Command::is<Commands::Sfx::DeletePage>(cmd);
		const bool refreshName =
			Command::is<Commands::Sfx::SetName>(cmd);
		const bool refreshSound_ =
			Command::is<Commands::Sfx::AddPage>(cmd) ||
			Command::is<Commands::Sfx::DeletePage>(cmd) ||
			Command::is<Commands::Sfx::SetSound>(cmd) ||
			Command::is<Commands::Sfx::Import>(cmd) ||
			Command::is<Commands::Sfx::ImportAsNew>(cmd);
		const bool refreshShapes_ =
			Command::is<Commands::Sfx::AddPage>(cmd) ||
			Command::is<Commands::Sfx::DeletePage>(cmd) ||
			Command::is<Commands::Sfx::SetSound>(cmd) ||
			Command::is<Commands::Sfx::Import>(cmd) ||
			Command::is<Commands::Sfx::ImportAsNew>(cmd);

		const Commands::Layered::Layered* pcmd = Command::as<Commands::Layered::Layered>(cmd);
		const int index = pcmd->sub();

		if (recalcSize) {
			_estimated.filled = false;
		}
		if (refreshIndex) {
			_index = _project->activeSfxIndex();
		}
		if (refreshName) {
			_tools.name = sound(index)->name;
			ws->clearSfxPageNames();
		}
		if (refreshSound_) {
			refreshSound();
		}
		if (refreshShapes_) {
			const Commands::Sfx::AddPage* addcmd = Command::as<Commands::Sfx::AddPage>(cmd);
			const Commands::Sfx::DeletePage* delcmd = Command::as<Commands::Sfx::DeletePage>(cmd);
			const Commands::Sfx::ImportAsNew* newcmd = Command::as<Commands::Sfx::ImportAsNew>(cmd);
			int idx = _index;
			if (addcmd)
				idx = addcmd->index();
			else if (delcmd)
				idx = delcmd->index();
			else if (newcmd)
				idx = newcmd->index();

			SfxAssets::Entry* entry_ = entry(idx);
			if (entry_) {
				entry_->shape = nullptr;

				++_tools.needToRefreshShapes;
			}
		}
	}

	SfxAssets::Entry* entry(int index) const {
		SfxAssets::Entry* entry = _project->getSfx(index);

		return entry;
	}
	Sfx::Ptr object(int index) const {
		SfxAssets::Entry* entry_ = entry(index);
		if (!entry_)
			return nullptr;

		return entry_->data;
	}
	int indexOfObject(uintptr_t ptr) const {
		const int n = _project->sfxPageCount();
		for (int i = 0; i < n; ++i) {
			const SfxAssets::Entry* entry_ = entry(i);
			const Sfx::Ptr &data_ = entry_->data;
			const uintptr_t ptr_ = (uintptr_t)data_.get();
			if (ptr_ == ptr)
				return i;
		}

		return -1;
	}
	const Sfx::Sound* sound(int index) const {
		Sfx::Ptr obj = object(index);
		if (!obj)
			return nullptr;

		return obj->pointer();
	}
	Sfx::Sound* sound(int index) {
		Sfx::Ptr obj = object(index);
		if (!obj)
			return nullptr;

		return obj->pointer();
	}

	bool importFromJsonString(Window* wnd, Renderer* rnd, Workspace* ws, const std::string &txt, bool asNew) {
		rapidjson::Document doc;
		if (!Json::fromString(doc, txt.c_str())) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		const rapidjson::Value* sound = nullptr;
		if (!Jpath::get(doc, sound, "sound") || !sound || !sound->IsObject()) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		if (asNew) {
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->fromJson(*sound)) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			const SfxAssets &assets_ = _project->assets()->sfx;
			const int index = assets_.count();

			Sfx::Sound* snd = newObj->pointer();
			int another = -1;
			if (snd->name.empty() || !_project->canRenameSfx(index, snd->name, &another)) {
				snd->name = _project->getUsableSfxName(snd->name, index); // Unique name.
			}

			Command* cmd = enqueue<Commands::Sfx::ImportAsNew>()
				->with(wnd, rnd, ws, index, newObj)
				->with(_setView, index)
				->exec(object(index), Variant((void*)_project));

			_refresh(cmd);
		} else {
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->fromJson(*sound)) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			Command* cmd = enqueue<Commands::Sfx::Import>()
				->with(newObj)
				->with(_setView, _index)
				->exec(object(_index), Variant((void*)_project));

			_refresh(cmd);
		}

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}
	bool importFromJsonFile(Window* wnd, Renderer* rnd, Workspace* ws, const std::string &path, bool asNew) {
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

		if (!importFromJsonString(wnd, rnd, ws, txt, asNew))
			return false;

		return true;
	}
	bool importFromVgmFile(Window* wnd, Renderer* rnd, Workspace* ws, const std::string &path, bool asNew) {
		Bytes::Ptr bytes(Bytes::create());
		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		if (!file->readBytes(bytes.get())) {
			file->close(); FileMonitor::unuse(path);

			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		file->close(); FileMonitor::unuse(path);

		if (asNew) {
			Sfx::Sound::Array sounds;
			const Sfx::VgmOptions &options = _tools.vgmImportOptions;
			if (!Sfx::fromVgm(bytes.get(), sounds, &options) || sounds.size() != 1) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->load(sounds.front())) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			const SfxAssets &assets_ = _project->assets()->sfx;
			const int index = assets_.count();

			std::string name;
			Path::split(path, &name, nullptr, nullptr);
			Sfx::Sound* snd = newObj->pointer();
			snd->name = name;
			int another = -1;
			if (snd->name.empty() || !_project->canRenameSfx(index, snd->name, &another)) {
				snd->name = _project->getUsableSfxName(snd->name, index); // Unique name.
			}

			Command* cmd = enqueue<Commands::Sfx::ImportAsNew>()
				->with(wnd, rnd, ws, index, newObj)
				->with(_setView, index)
				->exec(object(index), Variant((void*)_project));

			_refresh(cmd);
		} else {
			const Sfx::Sound* oldSnd = sound(_index);
			const std::string oldName = oldSnd->name;
			Sfx::Sound::Array sounds;
			const Sfx::VgmOptions &options = _tools.vgmImportOptions;
			if (!Sfx::fromVgm(bytes.get(), sounds, &options) || sounds.size() != 1) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			sounds.front().name = oldName;
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->load(sounds.front())) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			Command* cmd = enqueue<Commands::Sfx::Import>()
				->with(newObj)
				->with(_setView, _index)
				->exec(object(_index), Variant((void*)_project));

			_refresh(cmd);
		}

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}
	bool importFromWavFile(Window* wnd, Renderer* rnd, Workspace* ws, const std::string &path, bool asNew) {
		Bytes::Ptr bytes(Bytes::create());
		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		if (!file->readBytes(bytes.get())) {
			file->close(); FileMonitor::unuse(path);

			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		file->close(); FileMonitor::unuse(path);

		if (asNew) {
			Sfx::Sound::Array sounds;
			if (Sfx::fromWav(bytes.get(), sounds) != 1 || sounds.size() != 1) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->load(sounds.front())) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			const SfxAssets &assets_ = _project->assets()->sfx;
			const int index = assets_.count();

			std::string name;
			Path::split(path, &name, nullptr, nullptr);
			Sfx::Sound* snd = newObj->pointer();
			snd->name = name;
			int another = -1;
			if (snd->name.empty() || !_project->canRenameSfx(index, snd->name, &another)) {
				snd->name = _project->getUsableSfxName(snd->name, index); // Unique name.
			}

			Command* cmd = enqueue<Commands::Sfx::ImportAsNew>()
				->with(wnd, rnd, ws, index, newObj)
				->with(_setView, index)
				->exec(object(index), Variant((void*)_project));

			_refresh(cmd);
		} else {
			const Sfx::Sound* oldSnd = sound(_index);
			const std::string oldName = oldSnd->name;
			Sfx::Sound::Array sounds;
			if (Sfx::fromWav(bytes.get(), sounds) != 1 || sounds.size() != 1) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			sounds.front().name = oldName;
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->load(sounds.front())) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			Command* cmd = enqueue<Commands::Sfx::Import>()
				->with(newObj)
				->with(_setView, _index)
				->exec(object(_index), Variant((void*)_project));

			_refresh(cmd);
		}

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}
	bool importFromFxHammerFile(Window* wnd, Renderer* rnd, Workspace* ws, const std::string &path, const Math::Vec2i &index_, bool asNew) {
		Bytes::Ptr bytes(Bytes::create());
		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ)) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		if (!file->readBytes(bytes.get())) {
			file->close(); FileMonitor::unuse(path);

			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		file->close(); FileMonitor::unuse(path);

		if (asNew) {
			Sfx::Sound::Array sounds;
			const Sfx::FxHammerOptions &options = _tools.fxHammerImportOptions;
			if (!Sfx::fromFxHammer(bytes.get(), sounds, &options) || sounds.size() < 1) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			const int min = Math::min(index_.x, index_.y);
			const int max = Math::max(index_.x, index_.y);
			for (int i = min; i <= max; ++i) {
				Sfx::Sound &sound = sounds[i];
				Sfx::Ptr newObj(Sfx::create());
				if (!newObj->load(sound)) {
					ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

					return false;
				}

				const SfxAssets &assets_ = _project->assets()->sfx;
				const int index = assets_.count();

				std::string name;
				Path::split(path, &name, nullptr, nullptr);
				Sfx::Sound* snd = newObj->pointer();
				snd->name = name;
				int another = -1;
				if (snd->name.empty() || !_project->canRenameSfx(index, snd->name, &another)) {
					snd->name = _project->getUsableSfxName(snd->name, index); // Unique name.
				}

				Command* cmd = enqueue<Commands::Sfx::ImportAsNew>()
					->with(wnd, rnd, ws, index, newObj)
					->with(_setView, index)
					->exec(object(index), Variant((void*)_project));

				_refresh(cmd);
			}
		} else {
			const Sfx::Sound* oldSnd = sound(_index);
			const std::string oldName = oldSnd->name;
			Sfx::Sound::Array sounds;
			Sfx::FxHammerOptions options = _tools.fxHammerImportOptions;
			options.index = index_.x;
			if (!Sfx::fromFxHammer(bytes.get(), sounds, &options) || sounds.size() != 1) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			Sfx::Sound &sound = sounds.front();
			sound.name = oldName;
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->load(sound)) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			Command* cmd = enqueue<Commands::Sfx::Import>()
				->with(newObj)
				->with(_setView, _index)
				->exec(object(_index), Variant((void*)_project));

			_refresh(cmd);
		}

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}
	bool importFromLibraryFile(Window* wnd, Renderer* rnd, Workspace* ws, const std::string &path, bool asNew) {
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

		rapidjson::Document doc;
		if (!Json::fromString(doc, txt.c_str())) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		const rapidjson::Value* sound = nullptr;
		if (!Jpath::get(doc, sound, "sound") || !sound || !sound->IsObject()) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		if (asNew) {
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->fromJson(*sound)) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			const SfxAssets &assets_ = _project->assets()->sfx;
			const int index = assets_.count();

			Sfx::Sound* snd = newObj->pointer();
			int another = -1;
			if (snd->name.empty() || !_project->canRenameSfx(index, snd->name, &another)) {
				snd->name = _project->getUsableSfxName(snd->name, index); // Unique name.
			}

			Command* cmd = enqueue<Commands::Sfx::ImportAsNew>()
				->with(wnd, rnd, ws, index, newObj)
				->with(_setView, index)
				->exec(object(index), Variant((void*)_project));

			_refresh(cmd);
		} else {
			Sfx::Ptr newObj(Sfx::create());
			if (!newObj->fromJson(*sound)) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			Command* cmd = enqueue<Commands::Sfx::Import>()
				->with(newObj)
				->with(_setView, _index)
				->exec(object(_index), Variant((void*)_project));

			_refresh(cmd);
		}

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}

	void refreshSound(void) {
		const Sfx::Sound* snd = sound(_index);
		if (!snd)
			return;

		_tools.name = snd->name;

		std::string str;
		if (!snd->toString(str))
			return;

		_tools.soundString = _tools.newSoundString = str;
	}
	void toggleShapes(bool on) {
		_tools.contentSize = Math::Vec2i();

		if (on) {
			++_tools.needToRefreshShapes;

			return;
		}

		for (int i = 0; i < _project->sfxPageCount(); ++i) {
			SfxAssets::Entry* entry_ = entry(i);
			if (entry_) {
				entry_->shape = nullptr;
			}
		}
	}
	void refreshShapes(Renderer* rnd, Workspace* ws) {
		if (_tools.needToRefreshShapes == 0)
			return;

		if (_async.inQueueCount > 0)
			return;

		_tools.needToRefreshShapes = 0;

		if (!_project->preferencesSfxShowSoundShape())
			return;

		const int n = _project->sfxPageCount();
		for (int i = 0; i < n; ++i) {
			const SfxAssets::Entry* entry_ = entry(i);
			if (!entry_->shape) {
				const Sfx::Ptr &data_ = entry_->data;
				const uintptr_t ptr = (uintptr_t)data_.get();
				const Sfx::Sound* snd = data_->pointer();
				_async.shapeBatch.add(ptr, i, *snd);
			}
		}

		if (_async.shapeBatch.empty())
			return;

		setupShapeBatch(ws, this, &_async.shapeBatch);
		renderShapeBatch(rnd, ws, this, &_async.shapeBatch);
	}
	/**
	 * @param[in, out] batch
	 */
	static void setupShapeBatch(Workspace* ws, EditorSfxImpl*, ShapeRenderingBatch* batch) {
		// Open the jobs.
		batch->foreach(
			[ws] (uintptr_t /* ptr */, ShapeRenderingJob &job, int index) -> void {
				job.open(ws);

				fprintf(stdout, "Setup job %d for SFX shape at page %d.\n", index, job.page);
			}
		);
	}
	/**
	 * @param[in, out] batch
	 */
	static void renderShapeBatch(Renderer* rnd, Workspace* ws, EditorSfxImpl* self, ShapeRenderingBatch* batch) {
		// Prepare.
		constexpr const double SETUP_STEP = 1 / 1000.0;
		constexpr const double STEP = 1 / 60.0;
		constexpr const double MAX_RENDERING_DURATION = 5.0; // Up to 5 seconds per SFX.
		constexpr const int MAX_BUFFER_COUNT = 512 * 1024; // Up to 512KB buffer.

		// Render the sounds.
		auto operate = [self, ws, SETUP_STEP, STEP, MAX_RENDERING_DURATION, MAX_BUFFER_COUNT] (ShapeRenderingJob &job) -> void {
			// Prepare.
			Tools tools;
			tools.playerMaxDuration = 5.0; // Ignore any data longer than 5 seconds.

			Bytes::Ptr buffer(Bytes::create());
			UInt8 minValue = std::numeric_limits<UInt8>::max();
			UInt8 maxValue = std::numeric_limits<UInt8>::min();

			Device::AudioHandler callback = [&job, buffer, &minValue, &maxValue] (void* /* specification */, Bytes* buffer_, UInt32 len) -> bool {
				if (job.converter.needed) {
					const size_t newLen = len * job.converter.len_mult;
					if (job.converterBufferLength < newLen) {
						job.converterBufferLength = newLen;
						job.converter.buf = (Uint8*)SDL_realloc(job.converter.buf, newLen);
					}
					job.converter.len = (int)len;
					SDL_memcpy(job.converter.buf, buffer_->pointer(), len);
					SDL_ConvertAudio(&job.converter);
					for (int i = 0; i < job.converter.len_cvt; ++i) {
						const UInt8 b = job.converter.buf[i];
						buffer->writeByte(b);
						minValue = Math::min(minValue, b);
						maxValue = Math::max(maxValue, b);
					}
				} else {
					for (int i = 0; i < (int)len; ++i) {
						const UInt8 b = buffer_->get(i);
						buffer->writeByte(b);
						minValue = Math::min(minValue, b);
						maxValue = Math::max(maxValue, b);
					}
				}

				return true;
			};

			// Compile the SFX.
			const Sfx::Sound &snd = job.sound;
			SfxAssets::Entry entry_(snd);
			const Bytes::Ptr rom_ = compileSfx(ws, self, entry_, tools);
			tools.playerStopDelay = 0.001;
			tools.playerPlayedTicks = 0.0;
			tools.hasPlayerPlayed = false;

			// Open a device.
			if (rom_) {
				job.device->open(
					rom_,
					Device::DeviceTypes::CLASSIC, false,
					nullptr,
					nullptr,
					true, false, true
				);
				job.device->timeoutThreshold(DateTime::fromSeconds(1.0));
				job.device->writeRam((UInt16)tools.playerIsPlayingLocation.address, (UInt8)EDITOR_SFX_HALT_BANK);
				job.converter.buf = (Uint8*)SDL_malloc(job.converter.len * job.converter.len_mult);
				job.converterBufferLength = job.converter.len * job.converter.len_mult;
			}

			// Render the sound data to an image.
			if (!snd.empty()) {
				const long long start = DateTime::ticks();
				for (; ; ) {
					bool stop = false;
					UInt8 ply = 0;
					job.device->readRam((UInt16)tools.playerIsPlayingLocation.address, &ply);
					if (ply != EDITOR_SFX_HALT_BANK) { // Playing.
						if (!tools.hasPlayerPlayed) {
							// Do nothing.
						}
					} else { // Not playing or stopped.
						if (tools.hasPlayerPlayed) {
							stop = true;
						} else {
							const bool ret = job.device->update(nullptr, nullptr, SETUP_STEP, nullptr, nullptr, false, nullptr, nullptr);
							if (!ret) {
								const std::string msg = Text::format(ws->theme()->warning_SfxTimeoutWhenRenderingSoundShapeAtPage(), { Text::toString(job.page) });
								self->warn(ws, msg, true);

								break;
							}

							continue;
						}
					}

					tools.hasPlayerPlayed = true;

					const bool ret = job.device->update(nullptr, nullptr, STEP, nullptr, nullptr, false, nullptr, callback);
					if (!ret) {
						const std::string msg = Text::format(ws->theme()->warning_SfxTimeoutWhenRenderingSoundShapeAtPage(), { Text::toString(job.page) });
						self->warn(ws, msg, true);

						break;
					}

					if ((int)buffer->count() >= MAX_BUFFER_COUNT)
						stop |= true;

					const long long now = DateTime::ticks();
					const long long diff = now - start;
					const double secs = DateTime::toSeconds(diff);
					if (secs >= MAX_RENDERING_DURATION)
						stop |= true;

					if (stop) {
						if (tools.playerStopDelay > 0.0) {
							tools.playerStopDelay -= STEP;

							continue;
						}

						break;
					}

					if (tools.playerMaxDuration > 0.0) {
						tools.playerPlayedTicks += STEP;
						if (tools.playerPlayedTicks >= tools.playerMaxDuration) {
							break;
						}
					}
				}
			}

			// Close the device.
			if (rom_) {
				job.device->close(nullptr);
			}

			// Fill into the image.
			Image::Ptr img(Image::create());
			img->fromBlank(EDITING_SFX_SHAPE_IMAGE_WIDTH, EDITING_SFX_SHAPE_IMAGE_HEIGHT, 0);

			const ImU32 txtCol = ImGui::GetColorU32(ImGuiCol_Text);
			Colour col;
			col.fromRGBA(txtCol);
			Shapes::Plotter plot = [img, &col] (int x, int y) -> void {
				img->set(x, y, col);
			};

			if (snd.empty() || minValue == maxValue) {
				Shapes::line(0, EDITING_SFX_SHAPE_IMAGE_HEIGHT - 1, EDITING_SFX_SHAPE_IMAGE_WIDTH - 1, EDITING_SFX_SHAPE_IMAGE_HEIGHT - 1, plot);
			} else {
				const int step = (int)buffer->count() / EDITING_SFX_SHAPE_IMAGE_WIDTH;
				int oldx = 0, oldy = 0;
				for (int x = 0; x < EDITING_SFX_SHAPE_IMAGE_WIDTH; ++x) {
					const int i = x * step;
					double sum = 0.0;
					int c = 0;
					for (int k = 0; k < step; k += 2) {
						const UInt8 s = buffer->get(i + k);
						sum += s;
						++c;
					}
					const UInt8 sample = (UInt8)(sum / Math::max(c, 1));
					const float factor = (float)(sample - minValue) / (maxValue - minValue);
					const int y = (int)((1.0f - factor) * (EDITING_SFX_SHAPE_IMAGE_HEIGHT - 1));
					if (x == 0)
						plot(x, y);
					else
						Shapes::line(oldx, oldy, x, y, plot);
					oldx = x;
					oldy = y;
				}
			}

			job.image = img;

			// Finish.
			if (job.converter.buf)
				SDL_free(job.converter.buf);
		};

		batch->foreach(
			[rnd, ws, self, operate] (uintptr_t ptr, ShapeRenderingJob &job, int index) -> void {
				fprintf(stdout, "Queued job %d for SFX shape at page %d.\n", index, job.page);

				++self->_async.inQueueCount;

				ws->async(
					std::bind(
						[operate] (WorkTask* /* task */, ShapeRenderingJob* job, int index) -> uintptr_t { // On work thread.
							operate(*job);

							fprintf(stdout, "Operated job %d for SFX shape at page %d.\n", index, job->page);

							return 0;
						},
						std::placeholders::_1, &job, index
					),
					[rnd, self, ptr, &job, index] (WorkTask* /* task */, uintptr_t) -> void { // On main thread.
						const int index_ = self->indexOfObject(ptr);
						SfxAssets::Entry* entry_ = self->entry(index_);
						if (entry_) {
							entry_->shape = Texture::Ptr(Texture::create());
							entry_->shape->fromImage(rnd, Texture::STATIC, job.image.get(), Texture::NEAREST);
							entry_->shape->blend(Texture::BLEND);

							fprintf(stdout, "Committed job %d for SFX shape at page %d.\n", index, job.page);
						} else {
							fprintf(stdout, "Discarded job %d for missing SFX shape at page %d.\n", index, job.page);
						}

						job.close();
					},
					[self] (WorkTask* task, uintptr_t) -> void { // On main thread.
						task->disassociated(true);

						--self->_async.inQueueCount;
					}
				);
			}
		);
	}
	/**
	 * @param[in, out] batch
	 */
	static void updateShapeBatch(Renderer*, Workspace*, EditorSfxImpl* self, ShapeRenderingBatch* batch) {
		if (self->_async.inQueueCount > 0)
			return;

		batch->clear();
	}

	bool playSfx(Workspace* ws, int index) {
		// Prepare.
		if (ws->running())
			return true;

		if (_tools.isPlaying)
			stopSfx(ws);

		const Sfx::Sound* snd = sound(index);
		if (snd->empty()) // Empty.
			return true;

		// Start measuring performance.
		const long long start = DateTime::ticks();

		// Compile.
		SfxAssets::Entry entry_ = *entry(index);
		const Bytes::Ptr rom_ = compileSfx(ws, this, entry_, _tools);
		_tools.playerStopDelay = 0.5;

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
		device->writeRam((UInt16)_tools.playerIsPlayingLocation.address, (UInt8)EDITOR_SFX_HALT_BANK);

		_tools.isPlaying = true;
		_tools.playingIndex = index;

		// Finish.
		return true;
	}
	bool stopSfx(Workspace* ws) {
		// Prepare.
		if (!_tools.isPlaying)
			return true;

		// Reset the player states.
		const Device::Ptr &device = ws->audioDevice();
		if (device) {
			device->writeRam((UInt16)_tools.playerIsPlayingLocation.address, (UInt8)EDITOR_SFX_HALT_BANK);
		}

		// Close the device.
		ws->closeAudioDevice();

		// Stop playing.
		_tools.isPlaying = false;
		_tools.playingIndex = -1;
		_tools.playerStopDelay = 0.2;
		_tools.playerMaxDuration = 0.0;
		_tools.playerPlayedTicks = 0.0;
#if EDITOR_SFX_UNLOAD_SYMBOLS_ON_FINISH
		_tools.isPlayerConfigLoaded = false;
		_tools.playerConfigText.clear();
		_tools.playerSymbolsText.clear();
		_tools.playerAliasesText.clear();
		_tools.playerIsPlayingLocation = Editing::SymbolLocation();
#endif /* EDITOR_SFX_UNLOAD_SYMBOLS_ON_FINISH */
		_tools.hasPlayerPlayed = false;

		// Finish.
		return true;
	}
	bool updateSfx(Workspace* ws, double delta) {
		// Prepare.
		if (!_tools.isPlaying)
			return true;

		const Device::Ptr &device = ws->audioDevice();
		if (!device)
			return false;

		// Read the player states.
		bool stop = false;
		UInt8 ply = 0;
		device->readRam((UInt16)_tools.playerIsPlayingLocation.address, &ply);
		if (ply != EDITOR_SFX_HALT_BANK) { // Playing.
			if (!_tools.hasPlayerPlayed) {
				// Do nothing.
			}
		} else { // Not playing or stopped.
			if (_tools.hasPlayerPlayed)
				stop = true;
			else
				return true;
		}

		_tools.hasPlayerPlayed = true;

		// Stop playing if it reaches the end.
		do {
			if (!stop)
				break;

			if (_tools.playerStopDelay > 0.0) {
				_tools.playerStopDelay -= delta;
			} else {
				stopSfx(ws);

				return true;
			}
		} while (false);

		// Stop playing if it reaches the duration limit.
		if (_tools.playerMaxDuration > 0.0) {
			_tools.playerPlayedTicks += delta;
			if (_tools.playerPlayedTicks >= _tools.playerMaxDuration) {
				stopSfx(ws);

				return true;
			}
		}

		// Finish.
		return true;
	}
	/**
	 * @param[in, out] tools
	 */
	static Bytes::Ptr compileSfx(Workspace* ws, EditorSfxImpl* self, const SfxAssets::Entry &entry_, Tools &tools) {
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
			self->warn(ws, "No valid SFX player.", true);

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
			dict[EDITOR_SFX_PLAYER_IS_PLAYING_RAM_STUB]  = Editing::SymbolLocation();
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

			const Editing::SymbolLocation &isPlayingLoc  = dict[EDITOR_SFX_PLAYER_IS_PLAYING_RAM_STUB];
			tools.playerIsPlayingLocation                = isPlayingLoc;
			tools.playerIsPlayingLocation.address       += EDITOR_SFX_PLAYER_IS_PLAYING_RAM_OFFSET;
			tools.isPlayerConfigLoaded                   = true;
			tools.playerConfigText                       = configTxt;
			tools.playerSymbolsText                      = symTxt;
			tools.playerAliasesText                      = aliasesTxt;
		}

		// Compile.
		print_("Begin compiling for SFX playback.");

		AssetsBundle::Ptr assets(new AssetsBundle());
		std::string src;
		src += RES_CODE_PLAY_SFX;
		assets->code.add(src);
		assets->sfx.add(entry_);

		const Bytes::Ptr rom_ = Workspace::compile(
			config, tools.playerConfigText,
			rom,
			sym, tools.playerSymbolsText,
			aliases, tools.playerAliasesText,
			"SFX", assets,
			bootstrapBank,
			print_, warn_, error_
		);

		print_("End compiling for SFX playback.");

		if (!rom_)
			return nullptr;

		print_("Ok.");

		// Finish.
		return rom_;
	}
};

EditorSfx* EditorSfx::instance = nullptr;
int EditorSfx::refCount = 0;

EditorSfx* EditorSfx::create(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Project* prj) {
	if (refCount++ == 0) {
		GBBASIC_ASSERT(!instance && "Impossible.");

		EditorSfxImpl* result = new EditorSfxImpl(wnd, rnd, ws, prj);
		instance = result;
	}

	return instance;
}

EditorSfx* EditorSfx::destroy(EditorSfx* ptr) {
	GBBASIC_ASSERT(refCount > 0 && "Impossible.");
	GBBASIC_ASSERT(ptr == instance && "Impossible.");

	if (--refCount == 0) {
		EditorSfxImpl* impl = static_cast<EditorSfxImpl*>(ptr);
		delete impl;

		instance = nullptr;
	}

	return instance;
}

/* ===========================================================================} */
