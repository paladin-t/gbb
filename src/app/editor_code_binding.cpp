/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_code.h"
#include "editor_code_binding.h"
#include "editor_code_language_definition.h"
#include "project.h"
#include "theme.h"
#include "workspace.h"
#include "../utils/encoding.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include <SDL.h>

/*
** {===========================================================================
** Code binding editor
*/

class EditorCodeBindingImpl : public EditorCodeBinding, public ImGui::CodeEditor {
private:
	bool _opened = false;

	Renderer* _renderer = nullptr; // Foreign.
	Workspace* _workspace = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	struct {
		Editing::Tools::CodeIndex index;
		std::string pageText;
		std::string warning;

		int computedLine(EditorCodeBindingImpl* self) const {
			int ln = 0;
			if (index.line.isLeft())
				ln = index.line.left().get();
			else
				ln = self->getLineOfLabel(index.line.right().get());

			return ln;
		}
		bool set(Workspace* ws, const std::string &idx) {
			const bool result = index.set(idx);

			pageText = ws->theme()->status_Pg() + " " + Text::toString(index.page);

			return result;
		}
		void setPage(Workspace* ws, int pg) {
			index.setPage(pg);

			pageText = ws->theme()->status_Pg() + " " + Text::toString(index.page);
		}
		void setLine(int line) {
			index.setLine(line);
		}
		void setLabel(const std::string &lbl) {
			index.setLabel(lbl);
		}
		bool empty(void) const {
			return index.empty();
		}
		void clear(void) {
			index.clear();
			pageText.clear();
			warning.clear();
		}
	} _index;
	struct {
		unsigned revision = 0;
		bool filled = false;

		void clear(bool clearRevision) {
			if (clearRevision)
				revision = 0;
			filled = false;
		}
	} _inCodeDefinitions;
	ChangedHandler _changed = nullptr; // Foreign.

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	GotoHandler _gotoHandler = nullptr;
	std::string _gotoText;

	ImGui::Initializer _init;

public:
	EditorCodeBindingImpl(
		Window* wnd, Renderer* rnd,
		Workspace* ws,
		Theme* theme,
		ChangedHandler changed,
		const std::string &title,
		Project* prj,
		const std::string &index,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const GotoHandler &goto_,
		const char* confirmTxt, const char* cancelTxt, const char* gotoTxt
	) : _renderer(rnd),
		_workspace(ws),
		_theme(theme),
		_changed(changed),
		_title(title),
		_project(prj),
		_confirmedHandler(confirm), _canceledHandler(cancel), _gotoHandler(goto_)
	{
		const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();
		SetLanguageDefinition(EditorCodeLanguageDefinition::languageDefinition(krnl ? krnl->path().c_str() : nullptr));

		const int scale = rnd->scale() / wnd->scale();
		SetImePositionUpdatedHandler(
			[scale] (const ImVec2 &pos) -> ImVec2 {
				return ImVec2(pos.x * scale, pos.y * scale);
			}
		);

		if (confirmTxt)
			_confirmText = confirmTxt;
		if (cancelTxt)
			_cancelText = cancelTxt;
		if (gotoTxt)
			_gotoText = gotoTxt;

		open(index);
	}
	virtual ~EditorCodeBindingImpl() override {
		close();
	}

	EditorCodeBinding* open(const std::string &index) {
		if (_opened)
			return this;
		_opened = true;

		const std::string idx = Text::trim(index);
		if (!_index.set(_workspace, idx) || idx.empty()) {
			int pg = _project->preferencesCodePageForBindedRoutine();
			pg = Math::min(pg, _project->codePageCount() - 1);
			pg = Math::max(pg, 0);
			const Either<int, std::string> &ln = _project->preferencesCodeLineForBindedRoutine();
			std::string dest = "#" + Text::toString(pg);
			if (ln.isLeft())
				dest += ":" + Text::toString(ln.left().get() + 1);
			else
				dest += ":" + ln.right().get();

			_index.set(_workspace, dest);
		}

		refreshPage();

		SetReadOnly(true);

		SetStickyLineNumbers(true);

		SetHeadClickEnabled(true);

		DisableShortcut(All);

		SetErrorTipEnabled(false);

		SetTooltipEnabled(false);

		const Settings &settings = _workspace->settings();
		setIndentRule(settings.mainIndentRule);
		setColumnIndicator(settings.mainColumnIndicator);
		setShowSpaces(settings.mainShowWhiteSpaces);

		SetLineClickedHandler(std::bind(&EditorCodeBindingImpl::lineClicked, this, std::placeholders::_1, std::placeholders::_2));

		const int ln = _index.computedLine(this);
		if (ln == -1 || ln >= GetTotalLines()) {
			int pg = _project->preferencesCodePageForBindedRoutine();
			pg = Math::min(pg, _project->codePageCount() - 1);
			pg = Math::max(pg, 0);
			const std::string dest = "#" + Text::toString(pg);

			_index.set(_workspace, dest);

			SetCursorPosition(Coordinates(0, 0));
			EnsureCursorVisible();
		} else {
			SetCursorPosition(Coordinates(ln, 0));
			EnsureCursorVisible();
		}

		if (_workspace->needAnalyzing())
			_workspace->analyze(true);

		fprintf(stdout, "Code binding editor opened: #%s.\n", _index.index.destination.c_str());

		return this;
	}
	EditorCodeBinding* close(void) {
		if (!_opened)
			return this;
		_opened = false;

		fprintf(stdout, "Code binding editor closed: #%s.\n", _index.index.destination.c_str());

		SetLineClickedHandler(nullptr);

		_project = nullptr;
		_index.clear();
		_inCodeDefinitions.clear(true);

		return this;
	}

	virtual void addKeyword(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		def.Keys.insert(str);
	}
	virtual void addSymbol(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		def.Symbols.insert(str);
	}
	virtual void addIdentifier(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "GB BASIC function";
		def.Ids.insert(std::make_pair(std::string(str), id));
	}
	virtual void addPreprocessor(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "GB BASIC preprocessor";
		def.PreprocIds.insert(std::make_pair(std::string(str), id));
	}

	virtual const std::string toString(void) const override {
		return GetText("\n");
	}
	virtual void fromString(const char* txt, size_t /* len */) override {
		SetText(txt);
	}

	virtual void update(Workspace* ws) override {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		touchLanguageDefinition(ws);

		bool isOpen = true;
		bool toConfirm = false;
		bool toCancel = false;
		bool toGoto = false;

		if (_init.begin())
			ImGui::OpenPopup(_title);

		if (!_init.end()) {
			const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
			ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ws->analyzing()) {
			ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Always);
		} else {
			ImGui::SetNextWindowSizeConstraints(ImVec2(280, 180), ImVec2((float)_renderer->width(), (float)_renderer->height()));
			ImGui::SetNextWindowSize(ImVec2(280, Math::max(_renderer->height() * 0.85f, 180.0f)), ImGuiCond_Once);
		}
		if (ImGui::BeginPopupModal(_title, nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav)) {
			if (ws->analyzing()) {
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(_theme->dialogPrompt_Analyzing());

				const char* confirm = _confirmText.c_str();
				const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();
				const char* goto_ = _gotoText.empty() ? "Goto" : _gotoText.c_str();

				if (_confirmText.empty()) {
					confirm = "Ok";
				}

				ImGui::CentralizeButton(3);

				ImGui::BeginDisabled();
				{
					ImGui::Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0));

					ImGui::SameLine();
					ImGui::Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0));

					ImGui::SameLine();
					ImGui::Button(goto_, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
				}
				ImGui::EndDisabled();

				_init.reset();
			} else {
				do {
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

					if (ImGui::Button("<")) {
						int pg = _index.index.page - 1;
						if (pg < 0)
							pg = _project->codePageCount() - 1;
						if (pg != _index.index.page) {
							_index.setPage(_workspace, pg);
							refreshPage();
						}
					}
					ImGui::SameLine();
					if (ImGui::Button(_index.pageText)) {
						ImGui::OpenPopup("@Pg");

						_workspace->bubble(nullptr);
					}
					ImGui::SameLine();
					if (ImGui::Button(">")) {
						int pg = _index.index.page + 1;
						if (pg >= _project->codePageCount())
							pg = 0;
						if (pg != _index.index.page) {
							_index.setPage(_workspace, pg);
							refreshPage();
						}
					}
				} while (false);
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					if (ImGui::BeginPopup("@Pg")) {
						const Text::Array &assetNames = ws->getCodePageNames();
						const int n = _project->codePageCount();
						for (int i = 0; i < n; ++i) {
							const std::string &pg = assetNames[i];
							if (i == _index.index.page) {
								ImGui::MenuItem(pg, nullptr, true);
							} else {
								if (ImGui::MenuItem(pg)) {
									_index.setPage(_workspace, i);
									refreshPage();
								}
							}
						}

						ImGui::EndPopup();
					}
				} while (false);

				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - style.FramePadding.x + 1);
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
				if (_index.warning.empty())
					ImGui::InputText("", (char*)_index.index.destination.c_str(), _index.index.destination.length(), ImGuiInputTextFlags_ReadOnly);
				else
					ImGui::InputText("", (char*)_index.warning.c_str(), _index.warning.length(), ImGuiInputTextFlags_ReadOnly);
				ImGui::PopStyleColor();

				{
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

					const float width = ImGui::GetWindowWidth() - guardItemSpacing.previous().x * 2;
					const float height = ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - guardItemSpacing.previous().y * 4 - 16;
					ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0));
					{
						ImFont* fontCode = _theme->fontCode();
						if (fontCode && fontCode->IsLoaded()) {
							ImGui::PushFont(fontCode);
							SetFont(fontCode);
						}
						Render(_title.c_str(), ImVec2(width, height));
						if (fontCode && fontCode->IsLoaded()) {
							SetFont(nullptr);
							ImGui::PopFont();
						}
					}
					ImGui::PopStyleColor();
				}
				ImGui::NewLine(1);

				const char* confirm = _confirmText.c_str();
				const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();
				const char* goto_ = _gotoText.empty() ? "Goto" : _gotoText.c_str();

				if (_confirmText.empty()) {
					confirm = "Ok";
				}

				ImGui::CentralizeButton(3);

				if (ImGui::Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_Y)) {
					toConfirm = true;

					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
				if (ImGui::Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
					toCancel = true;

					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
				if (ImGui::Button(goto_, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
				toGoto = true;

				ImGui::CloseCurrentPopup();
			}
			}

			if (!_init.begin() && !_init.end())
				ImGui::CentralizeWindow();

			ImGui::EnsureWindowVisible();

			ImGui::EndPopup();
		}

		if (isOpen)
			_init.update();

		if (!isOpen)
			toCancel = true;

		if (toConfirm) {
			_init.reset();

			_project->preferencesCodePageForBindedRoutine(_index.index.page);
			_project->preferencesCodeLineForBindedRoutine(_index.index.line);

			if (_changed != nullptr) {
				_changed({ _index.index.destination });
			}
			if (!_confirmedHandler.empty()) {
				_confirmedHandler();

				return;
			}
		}
		if (toCancel) {
			_init.reset();

			if (!_canceledHandler.empty()) {
				_canceledHandler();

				return;
			}
		}
		if (toGoto) {
			_init.reset();

			if (!_gotoHandler.empty()) {
				const int ln = Math::max(0, _index.computedLine(this));
				_gotoHandler(_index.index.page, ln);

				return;
			}
		}
	}

private:
	void refreshPage(void) {
		int pg = 0;
		if (!_index.empty())
			pg = _index.index.page;

		CodeAssets::Entry* entry = _project->getCode(pg);
		if (entry) {
			EditorCode* editor = (EditorCode*)entry->editor;
			std::string txt;
			if (editor)
				txt = editor->toString();
			else
				entry->toString(txt, nullptr);

			fromString(txt.c_str(), txt.length());

			ColorizeRange(0, GetTotalLines());
		}

		const int ln = Math::max(0, _index.computedLine(this));
		SetProgramPointer(ln);
	}
	int getLineNumberOfLine(int ln) const {
		const std::vector<std::pair<std::string, PaletteIndex> > words = GetWordsAtLine(ln, false, true, false);
		if (words.size() < 1)
			return -1;

		auto foo = words.front();
		if (!(words.front().second == PaletteIndex::Number))
			return -1;

		int lno = -1;
		if (!Text::fromString(words.front().first, lno))
			return -1;

		return lno;
	}
	std::string getLabelOfLine(int ln) const {
		const std::vector<std::pair<std::string, PaletteIndex> > words = GetWordsAtLine(ln, false, true, false);
		if (words.size() != 2)
			return "";

		if (!(words.front().second == PaletteIndex::Symbol)) // It could be `PaletteIndex::Identifier` if static analyzing is not complete, and a line number will be used instead.
			return "";
		if (!(words.back().second == PaletteIndex::Punctuation && words.back().first == ":"))
			return "";

		return words.front().first;
	}
	int getLineOfLabel(const std::string &lbl) const {
		const std::vector<std::string> lines = GetTextLines(false, true);
		for (int i = 0; i < (int)lines.size(); ++i) {
			std::string ln = lines[i];
			ln = Text::trim(ln);
			ln = Text::replace(ln, " ", "");
			ln = Text::replace(ln, "\t", "");
			if (ln == lbl + ":")
				return i;
		}

		return -1;
	}

	void setIndentRule(Settings::IndentRules rule) {
		switch (rule) {
		case Settings::SPACE_2:
			SetIndentWithTab(false);
			SetTabSize(2);

			break;
		case Settings::SPACE_4:
			SetIndentWithTab(false);
			SetTabSize(4);

			break;
		case Settings::SPACE_8:
			SetIndentWithTab(false);
			SetTabSize(8);

			break;
		case Settings::TAB_2:
			SetIndentWithTab(true);
			SetTabSize(2);

			break;
		case Settings::TAB_4:
			SetIndentWithTab(true);
			SetTabSize(4);

			break;
		case Settings::TAB_8:
			SetIndentWithTab(true);
			SetTabSize(8);

			break;
		}
	}
	void setColumnIndicator(Settings::ColumnIndicator rule) {
		switch (rule) {
		case Settings::COL_NONE:
			SetSafeColumnIndicatorOffset(0);

			break;
		case Settings::COL_40:
			SetSafeColumnIndicatorOffset(40);

			break;
		case Settings::COL_60:
			SetSafeColumnIndicatorOffset(60);

			break;
		case Settings::COL_80:
			SetSafeColumnIndicatorOffset(80);

			break;
		case Settings::COL_100:
			SetSafeColumnIndicatorOffset(100);

			break;
		case Settings::COL_120:
			SetSafeColumnIndicatorOffset(120);

			break;
		}
	}
	void setShowSpaces(bool show) {
		SetShowWhiteSpaces(show);
	}

	void lineClicked(int ln, bool /* doubleClicked */) {
		const int lno = getLineNumberOfLine(ln);
		const std::string lbl = getLabelOfLine(ln);

		if (lno >= 0)
			_index.warning = _workspace->theme()->warning_RoutineExplicitLineNumberIsNotSupported();
		else
			_index.warning.clear();

		if (lbl.empty())
			_index.setLine(ln);
		else
			_index.setLabel(lbl);

		refreshPage();
	}

	void touchLanguageDefinition(Workspace* ws) {
		// Prepare.
		if (_inCodeDefinitions.filled)
			return;

		// Ignore if not changed.
		const unsigned revision = ws->getLanguageDefinitionRevision();
		if (_inCodeDefinitions.revision == revision) {
			_inCodeDefinitions.filled = true;

			return;
		}
		_inCodeDefinitions.revision = revision;

		// Initialize with default definition.
		const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();
		SetLanguageDefinition(EditorCodeLanguageDefinition::languageDefinition(krnl ? krnl->path().c_str() : nullptr));

		// Initialize with macros.
		Text::Set added;
		const GBBASIC::Macro::List* macros = ws->getMacroDefinitions();
		if (macros && !macros->empty()) {
			for (const GBBASIC::Macro &macro : *macros) {
				const std::string &name = macro.name;
				if (added.find(name) != added.end())
					continue;

				added.insert(name);

				addPreprocessor(name.c_str());
			}

			Colorize();
		}

		// Initialize with destinitions.
		added.clear();
		const Text::Array* destinitions = ws->getDestinitions();
		if (destinitions && !destinitions->empty()) {
			for (const std::string &dest : *destinitions) {
				if (added.find(dest) != added.end())
					continue;

				added.insert(dest);

				addSymbol(dest.c_str());
			}

			Colorize();
		}

		// Finish.
		_inCodeDefinitions.filled = true;
	}
};

EditorCodeBinding* EditorCodeBinding::create(
	Window* wnd, Renderer* rnd,
	class Workspace* ws,
	class Theme* theme,
	ChangedHandler changed,
	const std::string &title,
	class Project* prj,
	const std::string &index,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel, const GotoHandler &goto_,
	const char* confirmTxt, const char* cancelTxt, const char* gotoTxt
) {
	EditorCodeBindingImpl* result = new EditorCodeBindingImpl(
		wnd, rnd, ws,
		theme,
		changed,
		title,
		prj,
		index,
		confirm, cancel, goto_,
		confirmTxt, cancelTxt, gotoTxt
	);

	return result;
}

void EditorCodeBinding::destroy(EditorCodeBinding* ptr) {
	EditorCodeBindingImpl* impl = static_cast<EditorCodeBindingImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
