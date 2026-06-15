/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_i18n.h"
#include "editor_i18n.h"
#include "theme.h"
#include "workspace.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"

/*
** {===========================================================================
** Font editor
*/

class EditorI18nImpl : public EditorI18n {
private:
	bool _opened = false;

	Project* _project = nullptr; // Foreign.
	int _index = -1;
	CommandQueue* _commands = nullptr;

	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _page;
	struct {
		std::string info;

		void clear(void) {
			info.clear();
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
	std::function<void(const Command*)> _refresh = nullptr;

	struct Ref : public Editor::Ref {
		void clear(void) {
			// Do nothing.
		}
	} _ref;
	struct Tools {
		std::string namableText;

		bool focused = false;
		bool inputFieldFocused = false;

		void clear(void) {
			namableText.clear();

			focused = false;
			inputFieldFocused = false;
		}
	} _tools;

public:
	EditorI18nImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::I18n::AddItem>()
			->reg<Commands::I18n::DeleteItem>()
			->reg<Commands::I18n::AddLanguage>()
			->reg<Commands::I18n::DeleteLanguage>()
			->reg<Commands::I18n::ChangeContent>()
			->reg<Commands::I18n::SetName>()
			->reg<Commands::I18n::Import>();
	}
	virtual ~EditorI18nImpl() override {
		close(_index);

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window* /* wnd */, class Renderer* /* rnd */, class Workspace* ws, class Project* project, int index, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		_project = project;
		_index = index;

		_refresh = std::bind(&EditorI18nImpl::refresh, this, ws, std::placeholders::_1);

		_tools.namableText = entry()->name;

		// TODO: i18n.

		fprintf(stdout, "Font editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Font editor closed: #%d.\n", _index);

		_project = nullptr;
		_index = -1;

		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;

		_ref.clear();
		_tools.clear();

		// TODO: i18n.
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		// Do nothing.
	}
	virtual void leave(class Workspace*) override {
		// Do nothing.
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return false;
	}
	virtual void readonly(bool /* ro */) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		return _commands->hasUnsavedChanges();
	}
	virtual void markChangesSaved(void) override {
		_commands->markChangesSaved();
	}
	virtual void prepareForSaving(class Workspace*) override {
		// Do nothing.
	}

	virtual void copy(void) override {
		// TODO: i18n.
	}
	virtual void cut(void) override {
		// TODO: i18n.
	}
	virtual bool pastable(void) const override {
		// TODO: i18n.

		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		// TODO: i18n.
	}
	virtual void del(bool) override {
		// TODO: i18n.
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

		_refresh(cmd);

		_project->toPollEditor(true);
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		_refresh(cmd);

		_project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		(void)msg;
		(void)argc;
		(void)argv;

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		double /* delta */
	) override {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws);

		const Ref::Splitter splitter = _ref.split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		if (!entry() || !object()) {
			ImGui::BeginChild("@Blk", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			ImGui::EndChild();
			refreshStatus(wnd, rnd, ws);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}

		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - statusBarHeight), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
		{
			// TODO: i18n.
			(void)io;

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			bool inputFieldFocused = false;
			bool inputFieldFocused_ = false;

			const float spwidth = _ref.windowWidth(splitter.second);

			if (
				Editing::Tools::namable(
					rnd, ws,
					entry()->name, _tools.namableText,
					spwidth,
					nullptr, &inputFieldFocused_,
					ws->theme()->dialogPrompt_Name().c_str()
				)
			) {
				if (_project->canRenameTiles(_index, _tools.namableText, nullptr)) {
					Command* cmd = enqueue<Commands::I18n::SetName>()
						->with(_tools.namableText)
						->exec(object());

					_refresh(cmd);
				} else {
					_tools.namableText = entry()->name;

					warn(ws, ws->theme()->warning_TilesNameIsAlreadyInUse(), true);
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			_tools.inputFieldFocused = inputFieldFocused;

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows); // Ignore shortcuts when the window is not focused.
			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		refreshStatus(wnd, rnd, ws);
		renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);
	}

	virtual void statusInvalidated(void) override {
		_page.filled = false;

		// TODO: i18n.
	}

	virtual void added(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}
	virtual void removed(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}

	virtual void played(class Renderer*, class Workspace*) override {
		// Do nothing.
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
	bool shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
		if (_tools.inputFieldFocused || !ws->canUseShortcuts()) {
			return true;
		}

		// TODO: i18n.
		(void)wnd;
		(void)rnd;

		return true;
	}

	void context(Window*, Renderer*, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		// TODO: i18n.
		(void)style;
		(void)ws;
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
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
				index = _project->i18nPageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, index);
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
			if (index >= _project->i18nPageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		do {
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - _statusWidth);
			if (wndWidth >= 430) {
				if (_status.info.empty()) {
					const int ln = object()->itemCount();
					const int col = object()->languageCount();
					_status.info = Text::format(
						ws->theme()->tooltipI18n_Info(),
						{
							Text::toString(ln),
							Text::toString(col)
						}
					);
				}

				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::ImageButton(ws->theme()->iconInfo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, _status.info.empty() ? nullptr : _status.info.c_str());
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
				if (ImGui::PopupTooltip("Wrn", _warnings.text, ws->theme()->generic_Dismiss().c_str())) {
					_warnings.clear();
				}
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
			if (ImGui::ImageButton(ws->theme()->iconFont()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_FontAndI18n_Font().c_str())) {
				ws->category(Workspace::Categories::FONT);
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			do {
				WIDGETS_SELECTION_GUARD(ws->theme());

				if (ImGui::ImageButton(ws->theme()->iconI18n()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_FontAndI18n_I18n().c_str())) {
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
			const Text::Array &assetNames = ws->getI18nPageNames();
			const int n = _project->i18nPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, i);
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
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

					std::string txt;
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::READ)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readString(txt)) {
						file->close(); FileMonitor::unuse(path);

						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_Csv())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseCsv(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CsvFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_CSV_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					std::string txt;
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::READ)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readString(txt)) {
						file->close(); FileMonitor::unuse(path);

						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseCsv(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
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
						entry()->name.empty() ? "gbbasic-i18n.json" : Text::sanitizeFilename(entry()->name) + ".json",
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
					if (!entry()->serializeJson(txt, true))
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
			if (ImGui::MenuItem(ws->theme()->menu_Csv())) {
				do {
					std::string txt;
					if (!entry()->serializeCsv(txt))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CsvFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						entry()->name.empty() ? "gbbasic-i18n.csv" : Text::sanitizeFilename(entry()->name) + ".csv",
						GBBASIC_CSV_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || ext != "csv")
						path += ".csv";

					std::string txt;
					if (!entry()->serializeCsv(txt))
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

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "I18n editor: ";
				msg_ += msg;
				if (msg.back() != '.')
					msg_ += '.';
				ws->warn(msg_.c_str());
			}
		} else {
			_warnings.remove(msg);
		}
	}

	void modified(void) {
		_warnings.clear();
	}

	template<typename T> T* enqueue(void) {
		T* result = _commands->enqueue<T>();

		_project->toPollEditor(true);

		return result;
	}
	void refresh(Workspace* ws, const Command* cmd) {
		const bool refillName =
			Command::is<Commands::I18n::SetName>(cmd);

		if (refillName) {
			_tools.namableText = entry()->name;
			ws->clearTilesPageNames();
		}

		// TODO: i18n.
	}

	I18nAssets::Entry* entry(void) const {
		I18nAssets::Entry* entry = _project->getI18n(_index);

		return entry;
	}
	I18n::Ptr &object(void) const {
		return entry()->data;
	}
};

EditorI18n* EditorI18n::create(void) {
	EditorI18nImpl* result = new EditorI18nImpl();

	return result;
}

void EditorI18n::destroy(EditorI18n* ptr) {
	EditorI18nImpl* impl = static_cast<EditorI18nImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
