/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_font.h"
#include "editor_font.h"
#include "theme.h"
#include "workspace.h"
#include "../utils/encoding.h"
#include "../utils/filesystem.h"
#include "../../lib/imgui_code_editor/imgui_code_editor.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include <SDL.h>

/*
** {===========================================================================
** Font editor
*/

class EditorFontImpl : public EditorFont, public ImGui::CodeEditor {
private:
	bool _opened = false;

	Project* _project = nullptr; // Foreign.
	int _index = -1;
	CommandQueue* _commands = nullptr;

	struct {
		float preferedHeight = 0.0f;
		bool noResizing = false;
		bool isResizing = false;
		bool isResetting = false;
		Math::Vec2i size = Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE);
		Math::Vec2i maxSize = Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE);
		bool trim = true;
		int offset = 0;
		bool isTwoBitsPerPixel = GBBASIC_FONT_DEFAULT_IS_2BPP;
		bool preferFullWord = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD;
		bool preferFullWordForNonAscii = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII;
		int enabledEffect = (int)Font::Effects::NONE;
		int shadowOffset = 1;
		float shadowStrength = 0.3f;
		UInt8 shadowDirection = (UInt8)Directions::DIRECTION_DOWN_RIGHT;
		int outlineOffset = 1;
		float outlineStrength = 0.3f;
		int thresholds[4];
		bool inverted = false;
		std::string content;
		Math::Vec2i frameSize;
		Image::Ptr image = nullptr;
		Texture::Ptr texture = nullptr;

		void clear(void) {
			preferedHeight = 0.0f;
			noResizing = false;
			isResizing = false;
			isResetting = false;
			image = nullptr;
			texture = nullptr;
		}
		void fill(Renderer* rnd, FontAssets::Entry* entry, const Math::Vec2i &frameSize_) {
			// Prepare.
			Font::Ptr &obj = entry->object;

			// Check whether need to refresh the preview objects.
			if (size != entry->size) {
				size = entry->size;
				texture = nullptr;
			}

			if (maxSize != entry->maxSize) {
				maxSize = entry->maxSize;
				texture = nullptr;
			}

			if (trim != entry->trim) {
				trim = entry->trim;
				texture = nullptr;
			}

			if (offset != entry->offset) {
				offset = entry->offset;
				texture = nullptr;
			}

			if (isTwoBitsPerPixel != entry->isTwoBitsPerPixel) {
				isTwoBitsPerPixel = entry->isTwoBitsPerPixel;
				texture = nullptr;
			}

			if (preferFullWord != entry->preferFullWord) {
				preferFullWord = entry->preferFullWord;
				texture = nullptr;
			}

			if (preferFullWordForNonAscii != entry->preferFullWordForNonAscii) {
				preferFullWordForNonAscii = entry->preferFullWordForNonAscii;
				texture = nullptr;
			}

			if (enabledEffect != entry->enabledEffect) {
				enabledEffect = entry->enabledEffect;
				texture = nullptr;
			}

			if (shadowOffset != entry->shadowOffset) {
				shadowOffset = entry->shadowOffset;
				texture = nullptr;
			}

			if (shadowStrength != entry->shadowStrength) {
				shadowStrength = entry->shadowStrength;
				texture = nullptr;
			}

			if (shadowDirection != entry->shadowDirection) {
				shadowDirection = entry->shadowDirection;
				texture = nullptr;
			}

			if (outlineOffset != entry->outlineOffset) {
				outlineOffset = entry->outlineOffset;
				texture = nullptr;
			}

			if (outlineStrength != entry->outlineStrength) {
				outlineStrength = entry->outlineStrength;
				texture = nullptr;
			}

			if (memcmp(thresholds, entry->thresholds, sizeof(thresholds)) != 0) {
				memcpy(thresholds, entry->thresholds, sizeof(thresholds));
				texture = nullptr;
			}

			if (inverted != entry->inverted) {
				inverted = entry->inverted;
				texture = nullptr;
			}

			if (content != entry->content) {
				content = entry->content;
				texture = nullptr;
			}

			if (frameSize != frameSize_) {
				frameSize = frameSize_;
				texture = nullptr;
			}

			if (texture)
				return;

			// Create an image object.
			Indexed::Ptr palette = nullptr;
			if (image) {
				palette = image->palette();
			} else {
				const Colour col0 = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_0);
				const Colour col1 = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_1);
				const Colour col2 = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_2);
				const Colour col3 = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_3);
				palette = Indexed::Ptr(Indexed::create(GBBASIC_PALETTE_PER_GROUP_COUNT));
				palette->set(0, &col0);
				palette->set(1, &col1);
				palette->set(2, &col2);
				palette->set(3, &col3);
				image = Image::Ptr(Image::create(palette));
			}
			image->fromBlank(frameSize.x, frameSize.y, GBBASIC_PALETTE_DEPTH);
			if (inverted) {
				for (int j = 0; j < frameSize.y; ++j) {
					for (int i = 0; i < frameSize.x; ++i) {
						image->set(i, j, 3);
					}
				}
			}

			// Bake the glyphs.
			struct Word {
				typedef std::vector<Word> Array;

				std::wstring text;
				Image::Ptr image = nullptr;

				Word() {
				}
			};

			auto appendWord = [] (Word::Array &words_, Word &word_) -> void {
				if (!word_.text.empty() || word_.image) {
					words_.push_back(word_);
					word_ = Word();
				}
			};
			auto appendChar = [entry] (Word &word_, wchar_t cp, const Image::Ptr &pixels) -> void {
				word_.text += cp;
				if (pixels) {
					if (word_.image) {
						const int w_ = word_.image->width();
						const int h_ = word_.image->height();
						const int x = word_.image->width() + entry->characterPadding.x;
						const int w = x + pixels->width();
						const int h = Math::max(word_.image->height(), pixels->height());
						word_.image->resize(w, h, false);
						for (int j = 0; j < h_; ++j) {
							for (int i = w_; i < w; ++i) {
								word_.image->set(i, j, Colour(0, 0, 0, 0));
							}
						}
						for (int j = h_; j < h; ++j) {
							for (int i = 0; i < w; ++i) {
								word_.image->set(i, j, Colour(0, 0, 0, 0));
							}
						}
						pixels->blit(word_.image.get(), x, 0, pixels->width(), pixels->height(), 0, 0);
					} else {
						word_.image = pixels;
					}
				}
			};

			Word::Array words_;
			Text::Array words;
			const Text::Array wordsWithoutSpaces = Text::split(content, " ");
			for (int i = 0; i < (int)wordsWithoutSpaces.size(); ++i) {
				const std::string &word = wordsWithoutSpaces[i];
				words.push_back(word);
				if (i != (int)wordsWithoutSpaces.size() - 1)
					words.push_back(" ");
			}
			for (const std::string &word : words) {
				Word word_;
				const std::wstring wstr = Unicode::toWide(word);
				const wchar_t* wptr = wstr.c_str();
				while (*wptr) {
					// Prepare.
					const wchar_t cp = *wptr++;
					if (!cp)
						break;

					// Process new line.
					if (cp == '\n') {
						appendWord(words_, word_);
						appendChar(word_, cp, nullptr);
						appendWord(words_, word_);

						continue;
					}

					// Bake a character to pixels.
					Image::Ptr pixels(Image::create());
					const Colour col(0, 255, 0, 255);
					Font::Options options;
					if (isTwoBitsPerPixel && !!enabledEffect && (!!shadowOffset || !!outlineOffset)) {
						options.enabledEffect = (Font::Effects)enabledEffect;
						options.shadowOffset = shadowOffset;
						options.shadowStrength = shadowStrength;
						options.shadowDirection = shadowDirection;
						options.outlineOffset = outlineOffset;
						options.outlineStrength = outlineStrength;
						for (int i = 0; i < GBBASIC_COUNTOF(thresholds); ++i) {
							if (thresholds[i] != 0 && thresholds[i] < options.effectThreshold)
								options.effectThreshold = thresholds[i];
						}
					}
					int w = 0;
					int h = 0;
					bool rendered = obj->render(cp, pixels.get(), &col, &w, &h, offset, &options);
					if (!rendered && cp == ' ') {
						rendered = obj->render('X', pixels.get(), &col, &w, &h, offset, &options);
						pixels->fromBlank(pixels->width(), pixels->height(), 0);
					}
					if (!rendered) {
						rendered = obj->render('?', pixels.get(), &col, &w, &h, offset, &options);
					}
					if (!rendered) {
						continue;
					}

					// Adjust the pixels.
					if (entry->offset != 0 || w > GBBASIC_FONT_MAX_SIZE || h > GBBASIC_FONT_MAX_SIZE) {
						Image::Ptr tmp(Image::create());
						w = Math::min(w, GBBASIC_FONT_MAX_SIZE);
						h = Math::min(h, GBBASIC_FONT_MAX_SIZE);
						tmp->fromBlank(w, h, 0);
						pixels->blit(tmp.get(), 0, 0, w, h, 0, 0 /* -entry->offset */);
						std::swap(pixels, tmp);
					}

					// Put the pixels to an image.
					if (preferFullWordForNonAscii) { // Full for all words.
						GBBASIC_ASSERT(preferFullWord && "Impossible.");
						if (cp == ' ') { // Met space.
							appendWord(words_, word_);
							appendChar(word_, cp, pixels);
							appendWord(words_, word_);
						} else { // Met non-space.
							appendChar(word_, cp, pixels);
						}
					} else if (preferFullWord) { // Full ASCII word.
						if (cp == ' ') { // Met space.
							appendWord(words_, word_);
							appendChar(word_, cp, pixels);
							appendWord(words_, word_);
						} else if (cp <= 255) { // Met ASCII.
							appendChar(word_, cp, pixels);
						} else { // Met non-ASCII.
							appendWord(words_, word_);
							appendChar(word_, cp, pixels);
							appendWord(words_, word_);
						}
					} else {
						appendChar(word_, cp, pixels);
						appendWord(words_, word_);
					}
				}
				appendWord(words_, word_);
			}

			// Blit the glyphs.
			auto newline = [entry] (int &x, int &y) -> void {
				x = entry->frameMargin.x;
				y += entry->characterPadding.y + entry->size.y;
			};
			auto advance = [this, entry] (int /* x */, int y) -> void {
				if (y + entry->characterPadding.y + entry->size.y + entry->frameMargin.y <= image->height())
					return;

				const int w = image->width();
				const int h = image->height() + entry->characterPadding.y + entry->size.y + entry->frameMargin.y;
				image->resize(w, h, false);
			};

			int x = entry->frameMargin.x;
			int y = entry->frameMargin.y;
			bool isNewline = true;
			for (const Word &word_ : words_) {
				// Process new line.
				if (word_.text == L"\n") {
					newline(x, y);
					isNewline = true;

					continue;
				}

				// Put the pixels to an image.
				const Image::Ptr &src = word_.image;
				Image::Ptr blk(Image::create(palette));
				blk->fromBlank(src->width(), src->height(), GBBASIC_PALETTE_DEPTH);
				for (int j = 0; j < src->height(); ++j) {
					for (int i = 0; i < src->width(); ++i) {
						Colour c;
						if (!src->get(i, j, c)) {
							GBBASIC_ASSERT(false && "Wrong data.");
						}
						const int bits = FontAssets::getBits(c, entry->isTwoBitsPerPixel, entry->thresholds, entry->inverted);
						switch (bits) {
						case 0b00: // Fall through.
						case 0b01: // Fall through.
						case 0b10: // Fall through.
						case 0b11:
							blk->set(i, j, bits);

							break;
						default:
							GBBASIC_ASSERT(false && "Impossible.");

							break;
						}
					}
				}

				// Blit the image.
				if (x + entry->characterPadding.x + src->width() > image->width()) {
					if (!isNewline) {
						newline(x, y);
						isNewline = true;
					}
				}
				advance(x, y);
				blk->blit(image.get(), x, y, src->width(), src->height(), 0, 0);
				x += entry->characterPadding.x + src->width();
				isNewline = false;
			}

			// Create a texture object.
			texture = Texture::Ptr(Texture::create());
			texture->fromImage(rnd, Texture::STATIC, image.get(), Texture::NEAREST);
			texture->blend(Texture::BLEND);
		}
	} _preview;
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
		std::string info;

		void clear(void) {
			text.clear();
			filled = false;
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
	struct {
		Math::Vec2i size = Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE);
		Math::Vec2i maxSize = Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE);
		bool trim = true;
		int offset = 0;
		bool isTwoBitsPerPixel = GBBASIC_FONT_DEFAULT_IS_2BPP;
		bool preferFullWord = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD;
		bool preferFullWordForNonAscii = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII;
		int enabledEffect = (int)Font::Effects::NONE;
		int shadowOffset = 1;
		float shadowStrength = 0.3f;
		UInt8 shadowDirection = (UInt8)Directions::DIRECTION_DOWN_RIGHT;
		int outlineOffset = 1;
		float outlineStrength = 0.3f;
		int thresholds[4];
		bool inverted = false;

		void clear(void) {
			size = Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE);
			maxSize = Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE);
			trim = true;
			offset = 0;
			isTwoBitsPerPixel = GBBASIC_FONT_DEFAULT_IS_2BPP;
			preferFullWord = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD;
			preferFullWordForNonAscii = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII;
			enabledEffect = (int)Font::Effects::NONE;
			shadowOffset = 1;
			shadowStrength = 0.3f;
			shadowDirection = (UInt8)Directions::DIRECTION_DOWN_RIGHT;
			outlineOffset = 1;
			outlineStrength = 0.3f;
			memset(thresholds, 0, sizeof(thresholds));
			inverted = false;
		}
	} _parameters;

	struct Ref : public Editor::Ref {
		void clear(void) {
			// Do nothing.
		}
	} _ref;
	struct Tools {
		int magnification = -1;
		std::string namableText;

		bool focused = false;
		bool inputFieldFocused = false;
		bool showGrids = true;
		bool gridsVisible = false;
		bool fineZooming = false;

		void clear(void) {
			magnification = -1;
			namableText.clear();

			focused = false;
			inputFieldFocused = false;
			showGrids = true;
			gridsVisible = false;
			fineZooming = false;
		}
	} _tools;

public:
	EditorFontImpl() {
		SetLanguageDefinition(languageDefinition());

		SetPalette(getDarkPalette());

		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Font::SetContent>()
			->reg<Commands::Font::SetName>()
			->reg<Commands::Font::ResizeInt>()
			->reg<Commands::Font::ResizeVec2>()
			->reg<Commands::Font::ChangeMaxSizeVec2>()
			->reg<Commands::Font::SetTrim>()
			->reg<Commands::Font::SetOffset>()
			->reg<Commands::Font::Set2Bpp>()
			->reg<Commands::Font::SetEffect>()
			->reg<Commands::Font::SetShadowParameters>()
			->reg<Commands::Font::SetOutlineParameters>()
			->reg<Commands::Font::SetWordWrap>()
			->reg<Commands::Font::SetThresholds>()
			->reg<Commands::Font::Invert>()
			->reg<Commands::Font::SetArbitrary>();
	}
	virtual ~EditorFontImpl() override {
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

		_preview.preferedHeight = project->fontPreviewHeight();

		_refresh = std::bind(&EditorFontImpl::refresh, this, ws, std::placeholders::_1);

		_parameters.size = entry()->size;
		_parameters.maxSize = entry()->maxSize;
		_parameters.trim = entry()->trim;
		_parameters.offset = entry()->offset;
		_parameters.isTwoBitsPerPixel = entry()->isTwoBitsPerPixel;
		_parameters.preferFullWord = entry()->preferFullWord;
		_parameters.preferFullWordForNonAscii = entry()->preferFullWordForNonAscii;
		_parameters.enabledEffect = entry()->enabledEffect;
		_parameters.shadowOffset = entry()->shadowOffset;
		_parameters.shadowStrength = entry()->shadowStrength;
		_parameters.shadowDirection = entry()->shadowDirection;
		_parameters.outlineOffset = entry()->outlineOffset;
		_parameters.outlineStrength = entry()->outlineStrength;
		memcpy(_parameters.thresholds, entry()->thresholds, sizeof(entry()->thresholds));
		_parameters.inverted = entry()->inverted;

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		_tools.magnification = entry()->magnification;
		_tools.namableText = entry()->name;
		_tools.showGrids = _project->preferencesShowGrids();
		_tools.fineZooming = _project->preferencesFineZooming();
		_tools.gridsVisible = !caps.pressed();

		readonly(!entry()->isAsset);

		SetText(entry()->content);

		SetShowLineNumbers(false);

		SetStickyLineNumbers(true);

		SetHeadClickEnabled(false);

		DisableShortcut(All);

		SetTooltipEnabled(false);

		const Settings &settings = ws->settings();
		post(Editable::SET_INDENT_RULE, (Int)settings.mainIndentRule);
		post(Editable::SET_COLUMN_INDICATOR, (Int)settings.mainColumnIndicator);
		post(Editable::SET_SHOW_SPACES, (Int)settings.mainShowWhiteSpaces);

		SetColorizedHandler(std::bind(&EditorFontImpl::colorized, this, std::placeholders::_1));

		SetModifiedHandler(std::bind(&EditorFontImpl::modified, this, ws));

		fprintf(stdout, "Font editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Font editor closed: #%d.\n", _index);

		SetColorizedHandler(nullptr);

		SetModifiedHandler(nullptr);

		_project = nullptr;
		_index = -1;

		_preview.clear();
		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;
		_parameters.clear();

		_ref.clear();
		_tools.clear();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		// Do nothing.
	}
	virtual void leave(class Workspace*) override {
		_preview.clear();
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return IsReadOnly();
	}
	virtual void readonly(bool ro) override {
		SetReadOnly(ro);

		_status.clear();
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
		if (_tools.focused)
			return;

		Copy();
	}
	virtual void cut(void) override {
		if (ReadOnly) {
			copy();

			return;
		}

		if (_tools.focused)
			return;

		Cut();

		SetChangesCleared();

		ClearUndoRedoStack();
	}
	virtual bool pastable(void) const override {
		if (ReadOnly)
			return false;

		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (ReadOnly)
			return;

		if (_tools.focused)
			return;

		Paste();

		SetChangesCleared();

		ClearUndoRedoStack();
	}
	virtual void del(bool) override {
		if (ReadOnly)
			return;

		if (_tools.focused)
			return;

		Delete();

		SetChangesCleared();

		ClearUndoRedoStack();
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

		_commands->redo(object(), Variant((void*)entry()));

		_refresh(cmd);

		_project->toPollEditor(true);
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		_commands->undo(object(), Variant((void*)entry()));

		_refresh(cmd);

		_project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case SET_INDENT_RULE: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Settings::SPACE_2);
				switch ((Settings::IndentRules)rule) {
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

			return Variant(true);
		case SET_COLUMN_INDICATOR: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Settings::COL_80);
				switch ((Settings::ColumnIndicator)rule) {
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

			return Variant(true);
		case SET_SHOW_SPACES: {
				const bool show = unpack<bool>(argc, argv, 0, true);
				SetShowWhiteSpaces(show);
			}

			return Variant(true);
		case RESIZE: {
				const bool isInt = unpack<bool>(argc, argv, 0, true);
				const Variant::Int sizeX = unpack<Variant::Int>(argc, argv, 1, 0);
				const Variant::Int sizeY = unpack<Variant::Int>(argc, argv, 2, 0);
				const Math::Vec2i size(sizeX, sizeY);
				if (size == entry()->size)
					return Variant(true);

				if (isInt) {
					Command* cmd = enqueue<Commands::Font::ResizeInt>()
						->with((int)size.y)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				} else {
					Command* cmd = enqueue<Commands::Font::ResizeVec2>()
						->with(size)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				}
			}

			return Variant(true);
		case SELECT_ALL:
			if (_tools.focused)
				return Variant(false);

			SelectAll();

			return Variant(true);
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
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
		const char* title,
		float /* x */, float y, float width, float height,
		double /* delta */
	) override {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws);

		const Ref::Splitter splitter = _ref.split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		entry()->touch(); // Ensure the runtime resources are ready.

		const float height_ = height - statusBarHeight;
		if (!entry() || !object()) {
			ImGui::BeginChild("@Blk", ImVec2(width, height_), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			ImGui::EndChild();
			refreshStatus(wnd, rnd, ws);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}

		constexpr const int MAGNIFICATIONS[] = {
			1, 2, 4, 8
		};
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height_), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);
		{
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

			const float minH = 48.0f;
			const float maxH = height_ - minH - style.ScrollbarSize;
			const float previewWidth = splitter.first - style.ScrollbarSize;
			if (_preview.preferedHeight <= 0.0f) {
				_preview.preferedHeight = height_ * 0.5f;
				_preview.preferedHeight = Math::clamp(_preview.preferedHeight, minH, maxH);
				_project->fontPreviewHeight(height_ * 0.5f);
			} else {
				_preview.preferedHeight = Math::clamp(_preview.preferedHeight, minH, maxH);
			}
			_preview.fill(rnd, entry(), Math::Vec2i((int)previewWidth, (int)_preview.preferedHeight));

			if (_preview.noResizing && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				_preview.noResizing = false;
			}
			if (_preview.isResizing && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				_preview.isResizing = false;
			}
			if (_preview.isResetting && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				_preview.isResetting = false;
			}
			const float gripPaddingX = 16.0f;
			const float gripMarginY = ImGui::WindowResizingPadding().y;
			const bool isHoveringRect = ImGui::IsMouseHoveringRect(
				ImVec2(gripPaddingX, y + _preview.preferedHeight - gripMarginY),
				ImVec2(splitter.first - gripMarginY * 2.0f - style.ScrollbarSize, y + _preview.preferedHeight + gripMarginY),
				false
			);
			if (!_preview.isResizing) {
				if (!isHoveringRect && !ws->popupBox()) {
					_preview.noResizing = ImGui::IsMouseDown(ImGuiMouseButton_Left);
					_preview.isResizing = false;
				}
			}
			if (!_preview.noResizing) {
				if (isHoveringRect && !ws->popupBox()) {
					_preview.isResizing = ImGui::IsMouseDown(ImGuiMouseButton_Left);

					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						_preview.isResetting = true;

					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
				} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					_preview.isResizing = false;
				}
			}
			if (_preview.isResizing) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

				if (_preview.isResetting) {
					const float h = Math::clamp(height_ * 0.5f, minH, maxH);
					_preview.preferedHeight = h;
					_project->fontPreviewHeight(h);
				} else {
					const float curY = ImGui::GetMousePos().y;
					const float posY = curY - y;
					const float h = Math::clamp(posY, minH, maxH);
					_preview.preferedHeight = h;
					_project->fontPreviewHeight(h);
				}

				io.MouseDown[0] = io.MouseDown[1] = io.MouseDown[2] = false;
			}

			ImGui::BeginChildFrame(ImGui::GetID("@Prv"), ImVec2(splitter.first, _preview.preferedHeight), ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
			{
				if (_tools.magnification == -1) {
					const float PADDING = 32.0f;
					const float s = (splitter.first + PADDING) / _preview.texture->width();
					const float t = ((height - statusBarHeight) + PADDING) / _preview.texture->height();
					int m = 0;
					int n = 0;
					for (int i = 0; i < GBBASIC_COUNTOF(MAGNIFICATIONS); ++i) {
						if (s > MAGNIFICATIONS[i])
							m = i;
						if (t > MAGNIFICATIONS[i])
							n = i;
					}
					_tools.magnification = Math::min(m, n);
				}
				if (_tools.fineZooming) {
					_tools.magnification = Math::clamp(_tools.magnification, MAGNIFICATIONS[0], MAGNIFICATIONS[GBBASIC_COUNTOF(MAGNIFICATIONS) - 1]);
				} else {
					_tools.magnification = Math::clamp(_tools.magnification, 0, (int)GBBASIC_COUNTOF(MAGNIFICATIONS) - 1);
				}

				const int scale = _tools.fineZooming ? _tools.magnification : MAGNIFICATIONS[_tools.magnification];
				const float widgetWidth = (float)_preview.texture->width() * scale;
				const float widgetHeight = (float)_preview.texture->height() * scale;
				const ImVec2 curPos = ImGui::GetCursorScreenPos() - ImVec2(0.5f, 0.5f);
				ImGui::Image(
					(void*)_preview.texture->pointer(rnd),
					ImVec2(widgetWidth, widgetHeight),
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.0f)
				);

				if (scale > MAGNIFICATIONS[0] && (_tools.showGrids && _tools.gridsVisible)) {
					ImDrawList* drawList = ImGui::GetWindowDrawList();

					const float alpha = scale == MAGNIFICATIONS[1] ? 0.35f : 0.75f;
					drawList->AddRect(
						curPos,
						curPos + ImVec2(widgetWidth, widgetHeight),
						ImGui::GetColorU32(ImVec4(1, 1, 1, alpha))
					);
					const float w = (float)scale;
					const float h = (float)scale;
					for (float i = w; i < widgetWidth; i += w) {
						drawList->AddLine(
							curPos + ImVec2(i, 0),
							curPos + ImVec2(i, widgetHeight),
							ImGui::GetColorU32(ImVec4(1, 1, 1, alpha))
						);
					}
					for (float j = h; j < widgetHeight; j += h) {
						drawList->AddLine(
							curPos + ImVec2(0, j),
							curPos + ImVec2(widgetWidth, j),
							ImGui::GetColorU32(ImVec4(1, 1, 1, alpha))
						);
					}
				}

				ImGui::EndChildFrame();
			}

			ImFont* fontCode = ws->theme()->fontCode();
			if (fontCode && fontCode->IsLoaded()) {
				ImGui::PushFont(fontCode);
				SetFont(fontCode);
			}
			Render(title, ImVec2(splitter.first, height_ - _preview.preferedHeight));
			if (fontCode && fontCode->IsLoaded()) {
				SetFont(nullptr);
				ImGui::PopFont();
			}

			context(wnd, rnd, ws);

			refreshStatus(wnd, rnd, ws);

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height_), true, _ref.windowFlags());
		{
			const float spwidth = _ref.windowWidth(splitter.second);
			float xOffset = 0;
			const float mwidth = Editing::Tools::measure(
				rnd, ws,
				&xOffset,
				nullptr,
				-1.0f
			);

			const bool inputFieldFocused = _tools.inputFieldFocused;
			_tools.inputFieldFocused = false;

			auto canUseShortcuts = [ws, inputFieldFocused] (void) -> bool {
				return !inputFieldFocused && ws->canUseShortcuts();
			};

			bool inputFieldFocused_ = false;

			ImGui::PushID("@Ref");
			{
				const float unitSize = Editing::Tools::unitSize(rnd, ws, spwidth);

				VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
				VariableGuard<decltype(style.ItemInnerSpacing)> guardItemInnerSpacing(&style.ItemInnerSpacing, style.ItemInnerSpacing, ImVec2());

				const float curPosX = ImGui::GetCursorPosX();
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (withTtf() && !readonly()) {
					if (ImGui::ImageButton(ws->theme()->iconRef()->pointer(rnd), ImVec2(unitSize, unitSize), ImColor(IM_COL32_WHITE), false, ws->theme()->tooltip_ChangeRefFont().c_str())) {
						if (entry()->isAsset) {
							ImGui::FontResolverPopupBox::ConfirmedHandler confirm(
								[wnd, rnd, ws, this] (const char* path, const Math::Vec2i* size) -> void {
									resolveRef(wnd, rnd, ws, path, size, false, true);

									ws->popupBox(nullptr);
								},
								nullptr
							);
							ImGui::FontResolverPopupBox::CanceledHandler cancel(
								[ws] (void) -> void {
									ws->popupBox(nullptr);
								},
								nullptr
							);
							ws->showExternalFontBrowser(
								rnd,
								ws->theme()->generic_Path(),
#if GBBASIC_PSD_ENABLED
								GBBASIC_FONT_FILE_WITH_PSD_FILTER,
#else /* GBBASIC_PSD_ENABLED */
								GBBASIC_FONT_FILE_FILTER,
#endif /* GBBASIC_PSD_ENABLED */
								confirm,
								cancel,
								nullptr,
								nullptr
							);
						} else {
							ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
						}
					}
				} else {
					ImGui::Image(ws->theme()->iconRef()->pointer(rnd), ImVec2(unitSize, unitSize));
				}
				{
					VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2(0, 5.5f));

					ImGui::SameLine();
					ImGui::Dummy(ImVec2(style.ChildBorderSize, 0));
					ImGui::SameLine();
					const float prevWidth = ImGui::GetCursorPosX() - curPosX;
					ImGui::SetNextItemWidth(mwidth - prevWidth + xOffset);
					ImGui::InputText("", (char*)entry()->path.c_str(), entry()->path.length() + 1, ImGuiInputTextFlags_None);
				}
			}
			ImGui::PopID();

			ImGui::PushID("@Mag");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				if (_tools.fineZooming) {
					Editing::Tools::magnifiable(
						rnd, ws,
						&_tools.magnification,
						MAGNIFICATIONS[0], MAGNIFICATIONS[GBBASIC_COUNTOF(MAGNIFICATIONS) - 1],
						spwidth, canUseShortcuts(),
						&inputFieldFocused_
					);
				} else {
					Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, spwidth, canUseShortcuts());
				}
			}
			ImGui::PopID();

			ImGui::PushID("@Nm");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				if (
					Editing::Tools::namable(
						rnd, ws,
						entry()->name, _tools.namableText,
						spwidth,
						nullptr, &inputFieldFocused_,
						ws->theme()->dialogPrompt_Name().c_str()
					)
				) {
					if (entry()->isAsset) {
						if (_project->canRenameFont(_index, _tools.namableText, nullptr)) {
							Command* cmd = enqueue<Commands::Font::SetName>()
								->with(_tools.namableText)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);
						} else {
							_tools.namableText = entry()->name;

							warn(ws, ws->theme()->warning_FontNameIsAlreadyInUse(), true);
						}
					} else {
						_tools.namableText = entry()->name;

						ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
					}
				}
				_tools.inputFieldFocused |= inputFieldFocused_;
			}
			ImGui::PopID();

			ImGui::PushID("@Sz");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				if (withTtf()) {
					int size_ = (int)entry()->size.y;
					int newSize = _parameters.size.y;
					if (
						Editing::Tools::resizable(
							rnd, ws,
							size_,
							&newSize,
							GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE,
							spwidth,
							&inputFieldFocused_,
							ws->theme()->dialogPrompt_Size_InPixels().c_str(),
							ws->theme()->warning_FontSizeOutOfBounds().c_str(),
							std::bind(&EditorFontImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
						)
					) {
						_parameters.size.y = newSize;
						if (entry()->isAsset) {
							_parameters.size.y = Math::clamp(_parameters.size.y, GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE);
							if (_parameters.size.y != size_) {
								post(RESIZE, true, _parameters.size.x, _parameters.size.y);

								_warnings.clear();

								_project->preferencesFontSize(_parameters.size);
							}
						} else {
							_parameters.size = entry()->size;

							ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
						}
					} else {
						_parameters.size.y = newSize;
					}
					_tools.inputFieldFocused |= inputFieldFocused_;

					ImGui::PushID("@Max");
					{
						int sizeWidth = (int)entry()->maxSize.x;
						int sizeHeight = (int)entry()->maxSize.y;
						int newSizeWidth = _parameters.maxSize.x;
						int newSizeHeight = _parameters.maxSize.y;
						if (
							Editing::Tools::resizable(
								rnd, ws,
								sizeWidth, sizeHeight,
								&newSizeWidth, &newSizeHeight,
								GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE,
								spwidth,
								&inputFieldFocused_,
								ws->theme()->dialogPrompt_MaxSize().c_str(),
								ws->theme()->warning_FontMaxSizeOutOfBounds().c_str(),
								std::bind(&EditorFontImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
							)
						) {
							_parameters.maxSize.x = newSizeWidth;
							_parameters.maxSize.y = newSizeHeight;
							if (entry()->isAsset) {
								Command* cmd = enqueue<Commands::Font::ChangeMaxSizeVec2>()
									->with(_parameters.maxSize)
									->exec(object(), Variant((void*)entry()));

								_refresh(cmd);
							} else {

								ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
							}
						} else {
							_parameters.maxSize.x = newSizeWidth;
							_parameters.maxSize.y = newSizeHeight;
						}
						_tools.inputFieldFocused |= inputFieldFocused_;
					}
					ImGui::PopID();

					ImGui::PushID("@Trim");
					{
						ImGui::NewLine(1);
						if (
							Editing::Tools::togglable(
								rnd, ws,
								&_parameters.trim,
								spwidth,
								ws->theme()->dialogPrompt_Trim().c_str(),
								ws->theme()->tooltipFontAndI18n_FontTrim().c_str()
							)
						) {
							if (entry()->isAsset) {
								Command* cmd = enqueue<Commands::Font::SetTrim>()
									->with(_parameters.trim)
									->exec(object(), Variant((void*)entry()));

								_refresh(cmd);

								_warnings.clear();
							} else {
								_parameters.trim = entry()->trim;

								ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
							}
						}
					}
					ImGui::PopID();
				} else {
					int width_ = (int)entry()->size.x;
					int height_ = (int)entry()->size.y;
					int newWidth = (int)_parameters.size.x;
					int newHeight = (int)_parameters.size.y;
					if (
						Editing::Tools::resizable(
							rnd, ws,
							width_, height_,
							&newWidth, &newHeight,
							GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE,
							spwidth,
							&inputFieldFocused_,
							ws->theme()->dialogPrompt_Size().c_str(),
							ws->theme()->warning_FontSizeOutOfBounds().c_str(),
							std::bind(&EditorFontImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
						)
					) {
						_parameters.size = Math::Vec2i(newWidth, newHeight);
						if (entry()->isAsset) {
							_parameters.size.x = Math::clamp(_parameters.size.x, GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE);
							_parameters.size.y = Math::clamp(_parameters.size.y, GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE);
							if (_parameters.size != Math::Vec2i(width_, height_)) {
								post(RESIZE, false, _parameters.size.x, _parameters.size.y);

								_warnings.clear();

								_project->preferencesFontSize(_parameters.size);
							}
						} else {
							_parameters.size = entry()->size;

							ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
						}
					} else {
						_parameters.size = Math::Vec2i(newWidth, newHeight);
					}
					_tools.inputFieldFocused |= inputFieldFocused_;
				}
				if (entry()->enabledEffect == (int)Font::Effects::SHADOW) {
					const int enlargeX =
						(_parameters.shadowDirection == (UInt8)Directions::DIRECTION_LEFT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_RIGHT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP_LEFT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP_RIGHT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN_LEFT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN_RIGHT) ?
							_parameters.shadowOffset :
							0;
					const int enlargeY =
						(_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP_LEFT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP_RIGHT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN_LEFT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN_RIGHT) ?
							_parameters.shadowOffset :
							0;
					const bool bad =
						(enlargeX != 0 && (_parameters.size.x + enlargeX > GBBASIC_FONT_MAX_SIZE)) ||
						(enlargeY != 0 && (_parameters.size.y + enlargeY > GBBASIC_FONT_MAX_SIZE));
					if (bad)
						warn(ws, ws->theme()->warning_FontShadowOutOfBounds(), true);
				} else if (entry()->enabledEffect == (int)Font::Effects::OUTLINE) {
					const int enlarge = _parameters.outlineOffset * 2;
					const bool bad =
						(_parameters.size.x + enlarge > GBBASIC_FONT_MAX_SIZE) ||
						(_parameters.size.y + enlarge > GBBASIC_FONT_MAX_SIZE);
					if (bad)
						warn(ws, ws->theme()->warning_FontOutlineOutOfBounds(), true);
				}
			}
			ImGui::PopID();

			ImGui::PushID("@Off");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				const int offset_ = entry()->offset;
				if (
					Editing::Tools::resizable(
						rnd, ws,
						offset_,
						&_parameters.offset,
						-GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE,
						spwidth,
						&inputFieldFocused_,
						ws->theme()->dialogPrompt_Offset().c_str(),
						nullptr,
						nullptr
					)
				) {
					if (entry()->isAsset) {
						_parameters.offset = Math::clamp(_parameters.offset, -GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE);
						if (_parameters.offset != offset_) {
							Command* cmd = enqueue<Commands::Font::SetOffset>()
								->with(_parameters.offset)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);

							_project->preferencesFontOffset(_parameters.offset);
						}
					} else {
						_parameters.offset = entry()->offset;

						ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
					}
				}
				_tools.inputFieldFocused |= inputFieldFocused_;
			}
			ImGui::PopID();

			ImGui::PushID("@2bpp");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				if (
					Editing::Tools::togglable(
						rnd, ws,
						&_parameters.isTwoBitsPerPixel,
						spwidth,
						ws->theme()->dialogPrompt_2Bpp().c_str(),
						nullptr
					)
				) {
					if (entry()->isAsset) {
						Command* cmd = enqueue<Commands::Font::Set2Bpp>()
							->with(_parameters.isTwoBitsPerPixel)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);

						_warnings.clear();

						_project->preferencesFontIsTwoBitsPerPixel(_parameters.isTwoBitsPerPixel);
					} else {
						_parameters.isTwoBitsPerPixel = entry()->isTwoBitsPerPixel;

						ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
					}
				}
			}
			ImGui::PopID();

			ImGui::PushID("@Fx");
			if (entry()->isTwoBitsPerPixel) {
				Editing::Tools::separate(rnd, ws, spwidth);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(ws->theme()->dialogPrompt_Effect());

				const char* items[] = {
					ws->theme()->generic_None().c_str(),
					ws->theme()->windowFont_Shadow().c_str(),
					ws->theme()->windowFont_Outline().c_str()
				};
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(mwidth);
				_parameters.enabledEffect = Math::clamp(_parameters.enabledEffect, 0, (int)(GBBASIC_COUNTOF(items) - 1));

				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					if (ImGui::Combo("", &_parameters.enabledEffect, items, GBBASIC_COUNTOF(items))) {
						if (entry()->isAsset) {
							Command* cmd = enqueue<Commands::Font::SetEffect>()
								->with(_parameters.enabledEffect)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);

							_warnings.clear();
						} else {
							_parameters.enabledEffect = entry()->enabledEffect;

							ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
						}
					}
				} while (false);
				ImGui::SameLine();
				ImGui::NewLine();
				ImGui::NewLine(1);

				if (entry()->enabledEffect == (int)Font::Effects::SHADOW) { // Shadow.
					const int limit =
						(_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP_LEFT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_UP_RIGHT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN_LEFT ||
						_parameters.shadowDirection == (UInt8)Directions::DIRECTION_DOWN_RIGHT) ?
							GBBASIC_FONT_MAX_SIZE - _parameters.size.y :
							0;
					int offset_ = entry()->shadowOffset;
					float strength = entry()->shadowStrength;
					UInt8 dir = entry()->shadowDirection;
					const bool changed = Editing::Tools::offsetable(
						rnd, ws,
						offset_, &_parameters.shadowOffset, 1, 4, &limit,
						strength, &_parameters.shadowStrength, 0.0f, 1.0f,
						dir, &_parameters.shadowDirection,
						spwidth,
						&inputFieldFocused_,
						ws->theme()->warning_FontShadowOutOfBounds().c_str(),
						std::bind(&EditorFontImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
					);
					if (changed) {
						if (entry()->isAsset) {
							Command* cmd = enqueue<Commands::Font::SetShadowParameters>()
								->with(_parameters.shadowOffset, _parameters.shadowStrength, _parameters.shadowDirection)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);
						} else {
							_parameters.shadowOffset = entry()->shadowOffset;
							_parameters.shadowStrength = entry()->shadowStrength;
							_parameters.shadowDirection = entry()->shadowDirection;

							ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
						}
					}
					_tools.inputFieldFocused |= inputFieldFocused_;
				} else if (entry()->enabledEffect == (int)Font::Effects::OUTLINE) { // Outline.
					const int limit = (GBBASIC_FONT_MAX_SIZE - _parameters.size.y) / 2;
					int offset_ = entry()->outlineOffset;
					float strength = entry()->outlineStrength;
					const bool changed = Editing::Tools::offsetable(
						rnd, ws,
						offset_, &_parameters.outlineOffset, 1, 4, &limit,
						strength, &_parameters.outlineStrength, 0.0f, 1.0f,
						spwidth,
						&inputFieldFocused_,
						ws->theme()->warning_FontOutlineOutOfBounds().c_str(),
						std::bind(&EditorFontImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
					);
					if (changed) {
						if (entry()->isAsset) {
							Command* cmd = enqueue<Commands::Font::SetOutlineParameters>()
								->with(_parameters.outlineOffset, _parameters.outlineStrength)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);
						} else {
							_parameters.outlineOffset = entry()->outlineOffset;
							_parameters.outlineStrength = entry()->outlineStrength;

							ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
						}
					}
					_tools.inputFieldFocused |= inputFieldFocused_;
				}
			}
			ImGui::PopID();

			ImGui::PushID("@Fw");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(ws->theme()->dialogPrompt_WordWrap());

				const char* items[] = {
					ws->theme()->generic_None().c_str(),
					ws->theme()->windowFont_FullWordForAscii().c_str(),
					ws->theme()->windowFont_FullWordForAll().c_str()
				};
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(mwidth);
				int idx =
					_parameters.preferFullWordForNonAscii ? 2 :
					_parameters.preferFullWord ? 1 : 0;

				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					if (ImGui::Combo("", &idx, items, GBBASIC_COUNTOF(items))) {
						if (entry()->isAsset) {
							bool preferFullWord = 0;
							bool preferFullWordForNonAscii = 0;
							switch (idx) {
							case 0: // None.
								preferFullWord = 0;
								preferFullWordForNonAscii = 0;

								break;
							case 1: // ASCII.
								preferFullWord = 1;
								preferFullWordForNonAscii = 0;

								break;
							case 2: // All.
								preferFullWord = 1;
								preferFullWordForNonAscii = 1;

								break;
							}
							Command* cmd = enqueue<Commands::Font::SetWordWrap>()
								->with(preferFullWord, preferFullWordForNonAscii)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);

							_warnings.clear();
						} else {
							_parameters.preferFullWord = entry()->preferFullWord;
							_parameters.preferFullWordForNonAscii = entry()->preferFullWordForNonAscii;

							ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
						}
					}
				} while (false);
			}
			ImGui::PopID();

			ImGui::PushID("@Thr");
			{
				constexpr const int MIN[4] = { GBBASIC_FONT_MIN_THRESHOLD, GBBASIC_FONT_MIN_THRESHOLD, GBBASIC_FONT_MIN_THRESHOLD, GBBASIC_FONT_MIN_THRESHOLD };
				constexpr const int MAX[4] = { GBBASIC_FONT_MAX_THRESHOLD, GBBASIC_FONT_MAX_THRESHOLD, GBBASIC_FONT_MAX_THRESHOLD, GBBASIC_FONT_MAX_THRESHOLD };
				int thresholds_[4];
				memcpy(thresholds_, entry()->thresholds, sizeof(entry()->thresholds));
				Editing::Tools::separate(rnd, ws, spwidth);
				if (
					Editing::Tools::limited(
						rnd, ws,
						thresholds_,
						_parameters.thresholds,
						MIN, MAX,
						spwidth,
						&inputFieldFocused_,
						ws->theme()->dialogPrompt_Thresholds().c_str(),
						nullptr,
						nullptr
					)
				) {
					if (entry()->isAsset) {
						const Math::Vec4i thresholds_(
							_parameters.thresholds[0],
							_parameters.thresholds[1],
							_parameters.thresholds[2],
							_parameters.thresholds[3]
						);
						Command* cmd = enqueue<Commands::Font::SetThresholds>()
							->with(thresholds_)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);
					} else {
						memcpy(_parameters.thresholds, entry()->thresholds, sizeof(entry()->thresholds));

						ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
					}
				}
				_tools.inputFieldFocused |= inputFieldFocused_;
			}
			ImGui::PopID();

			ImGui::PushID("@Inv");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				if (
					Editing::Tools::togglable(
						rnd, ws,
						&_parameters.inverted,
						spwidth,
						ws->theme()->dialogPrompt_Inverted().c_str(),
						nullptr
					)
				) {
					if (entry()->isAsset) {
						Command* cmd = enqueue<Commands::Font::Invert>()
							->with(_parameters.inverted)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);

						_warnings.clear();
					} else {
						_parameters.inverted = entry()->inverted;

						ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
					}
				}
			}
			ImGui::PopID();

			ImGui::PushID("@Arb");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (entry()->isAsset) {
					if (ImGui::Button(ws->theme()->dialogPrompt_Arbitrary(), ImVec2(mwidth, 0))) {
						ws->showArbitraryEditor(
							rnd,
							_index,
							[this] (const std::initializer_list<Variant> &args) -> void {
								const Variant &arg = *args.begin();
								Object::Ptr obj = (Object::Ptr)arg;
								Font::Codepoints::Ptr ptr = Object::as<Font::Codepoints::Ptr>(obj);

								Command* cmd = enqueue<Commands::Font::SetArbitrary>()
									->with(*ptr)
									->exec(object(), Variant((void*)entry()));

								_refresh(cmd);
							}
						);
					}
				} else {
					if (ImGui::Button(ws->theme()->dialogPrompt_Arbitrary(), ImVec2(mwidth, 0))) {
						ws->messagePopupBox(ws->theme()->dialogPrompt_CannotModifyTheBuiltinAssetPage(), nullptr, nullptr, nullptr);
					}
				}
			}
			ImGui::PopID();

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows); // Ignore shortcuts when the window is not focused.
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);
	}

	virtual void statusInvalidated(void) override {
		_page.filled = false;
		_status.filled = false;
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
		_ref.windowResized();
	}

	virtual void focusLost(class Renderer*) override {
		// Do nothing.
	}
	virtual void focusGained(class Renderer*) override {
		// Do nothing.
	}

private:
	void shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		if (_tools.gridsVisible == caps.pressed()) {
			_tools.showGrids = !caps.pressed();
			_tools.gridsVisible = !caps.pressed();

			_project->preferencesShowGrids(_tools.showGrids);
			_project->hasDirtyEditor(true);
		}

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
					index = _project->fontPageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::FONT, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->fontPageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::FONT, index);
			}
		}
	}

	void context(Window*, Renderer*, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			if (!HasSelection())
				SelectWordUnderMouse();

			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Ctx")) {
			if (ImGui::MenuItem(ws->theme()->menu_Cut(), nullptr, nullptr, !readonly())) {
				cut();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Copy())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Paste(), nullptr, nullptr, pastable())) {
				paste();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Delete(), nullptr, nullptr, !readonly())) {
				del(false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_SelectAll())) {
				post(SELECT_ALL);
			}

			ImGui::EndPopup();
		}
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
		if (!_status.filled) {
			if (readonly()) {
				_status.text = " " + ws->theme()->status_Readonly();
			}
			_status.info = Text::format(
				ws->theme()->tooltipFontAndI18n_FontInfo(),
				{
					Text::toString(object()->glyphCount())
				}
			);
			_status.filled = true;
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
				index = _project->fontPageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::FONT, index);
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
			if (index >= _project->fontPageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::FONT, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_status.text);
		ImGui::SameLine();
		do {
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - _statusWidth);
			if (wndWidth >= 430) {
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
				if (ImGui::PopupTooltip("Wrn", _warnings.text, ws->theme()->generic_Dismiss().c_str()))
					_warnings.clear();
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();

				ImGui::PopStyleColor();
			}
			if (readonly()) {
#if !defined GBBASIC_OS_HTML
				if (ImGui::ImageButton(ws->theme()->iconBrowse()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Browse().c_str())) {
					if (entry()->isAsset) {
						std::string finalPath = Path::combine(entry()->directory.c_str(), entry()->path.c_str());
						const bool pathExists = Path::fileExists(entry()->path.c_str());
						const bool dirPathExists = Path::fileExists(finalPath.c_str());
						if (entry()->directory.empty()) {
							if (pathExists) // Is absolute path.
								finalPath = entry()->path;
						} else {
							if (pathExists && !dirPathExists) // Is absolute path.
								finalPath = entry()->path;
						}
						FileInfo::Ptr fileInfo = FileInfo::make(finalPath.c_str());
						const std::string dir = fileInfo->parentPath();
						const std::string osstr = Unicode::toOs(dir);

						Platform::browse(osstr.c_str());
					} else {
						std::string dir = WORKSPACE_FONT_DIR;
						dir = Path::absoluteOf(dir);
						const std::string osstr = Unicode::toOs(dir);

						Platform::browse(osstr.c_str());
					}
				}
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
#endif /* Platform macro. */
			}
			if (ImGui::ImageButton(ws->theme()->iconExport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Export().c_str())) {
				ImGui::OpenPopup("@Xpt");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconViews()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_View().c_str())) {
				ImGui::OpenPopup("@Views");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			do {
				WIDGETS_SELECTION_GUARD(ws->theme());

				if (ImGui::ImageButton(ws->theme()->iconFont()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipFontAndI18n_Font().c_str())) {
					// Do nothing.
				}
			} while (false);
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconI18n()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipFontAndI18n_I18n().c_str())) {
				ws->category(Workspace::Categories::I18N);
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
			const Text::Array &assetNames = ws->getFontPageNames();
			const int n = _project->fontPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::FONT, i);
				}
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::MenuItem(ws->theme()->menu_Code())) {
				std::string txt;
				if (!entry()->serializeBasic(txt, _index))
					return;

				const std::string osstr = Unicode::toOs(txt);

				Platform::setClipboardText(osstr.c_str());

				ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_Mapping())) {
				do {
					std::string txt;
					if (!entry()->serializeArbitraryMappingInCsv(txt))
						break;

					const std::string osstr = Unicode::toOs(txt);

					Platform::setClipboardText(osstr.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboardCsv());
			}
			if (ImGui::MenuItem(ws->theme()->menu_MappingFile())) {
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
					if (!entry()->serializeArbitraryMappingInCsv(txt))
						break;

					const std::string osstr = Unicode::toOs(txt);
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeString(osstr);
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
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_InCsvFormat());
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menu_Duplicate())) {
				ws->duplicateFontFrom(wnd, rnd, _index);
			}

			ImGui::EndPopup();
		}

		// Views.
		if (ImGui::BeginPopup("@Views")) {
			if (_tools.magnification == 0) {
				ImGui::MenuItem(ws->theme()->menu_Grids(), "Caps", false, false);
			} else {
				const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
				if (caps.pressed()) {
					ImGui::MenuItem(ws->theme()->menu_Grids(), "Caps", true);
				} else {
					if (ImGui::MenuItem(ws->theme()->menu_Grids(), "Caps", &_tools.showGrids)) {
						_project->preferencesShowGrids(_tools.showGrids);

						_project->hasDirtyEditor(true);
					}
				}
			}
			if (ImGui::MenuItem(ws->theme()->menu_FineZooming(), nullptr, &_tools.fineZooming)) {
				_tools.magnification = -1;

				_project->preferencesFineZooming(_tools.fineZooming);

				_project->hasDirtyEditor(true);
			}

			ImGui::EndPopup();
		}
	}

	void resolveRef(Window*, Renderer*, Workspace*, const char* path, const Math::Vec2i* size /* nullable */, bool copy, bool embed) {
		FontAssets::Entry entry_;
		_project->makeFontEntry(entry_, copy, embed, path, size, entry()->content, false);

		entry()->copying = entry_.copying;
		entry()->directory = entry_.directory;
		entry()->path = entry_.path;

		entry()->cleanup();

		_status.clear();

		_project->hasDirtyAsset(true);

		_preview.clear();
	}

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "Font editor: ";
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
		const bool refillName =
			Command::is<Commands::Font::SetName>(cmd);
		const bool refillContent =
			Command::is<Commands::Font::SetContent>(cmd);
		const bool toReset =
			Command::is<Commands::Font::ResizeInt>(cmd) ||
			Command::is<Commands::Font::ResizeVec2>(cmd) ||
			Command::is<Commands::Font::ChangeMaxSizeVec2>(cmd) ||
			Command::is<Commands::Font::SetTrim>(cmd) ||
			Command::is<Commands::Font::SetOffset>(cmd) ||
			Command::is<Commands::Font::Set2Bpp>(cmd) ||
			Command::is<Commands::Font::SetEffect>(cmd) ||
			Command::is<Commands::Font::SetShadowParameters>(cmd) ||
			Command::is<Commands::Font::SetOutlineParameters>(cmd) ||
			Command::is<Commands::Font::SetThresholds>(cmd) ||
			Command::is<Commands::Font::Invert>(cmd);
		const bool toReWrap =
			Command::is<Commands::Font::SetWordWrap>(cmd);

		if (refillName) {
			_tools.namableText = entry()->name;
			ws->clearFontPageNames();
		}
		if (refillContent) {
			SetText(entry()->content);
		}
		if (toReset) {
			_parameters.size = entry()->size;
			_parameters.maxSize = entry()->maxSize;
			_parameters.trim = entry()->trim;
			_parameters.offset = entry()->offset;
			_parameters.isTwoBitsPerPixel = entry()->isTwoBitsPerPixel;
			_parameters.preferFullWord = entry()->preferFullWord;
			_parameters.preferFullWordForNonAscii = entry()->preferFullWordForNonAscii;
			_parameters.enabledEffect = entry()->enabledEffect;
			_parameters.shadowOffset = entry()->shadowOffset;
			_parameters.shadowStrength = entry()->shadowStrength;
			_parameters.shadowDirection = entry()->shadowDirection;
			_parameters.outlineOffset = entry()->outlineOffset;
			_parameters.outlineStrength = entry()->outlineStrength;
			memcpy(_parameters.thresholds, entry()->thresholds, sizeof(entry()->thresholds));
			_parameters.inverted = entry()->inverted;

			entry()->cleanup();
		}
		if (toReWrap) {
			_parameters.preferFullWord = entry()->preferFullWord;
			_parameters.preferFullWordForNonAscii = entry()->preferFullWordForNonAscii;
		}
	}

	FontAssets::Entry* entry(void) const {
		FontAssets::Entry* entry = _project->getFont(_index);

		return entry;
	}
	Font::Ptr &object(void) const {
		return entry()->object;
	}
	bool withTtf(void) const {
		return entry()->bitmapData.empty();
	}

	void colorized(bool) {
		// Do nothing.
	}
	void modified(Workspace* ws) {
		const std::string str = GetText("\n");
		const std::wstring wstr = Unicode::toWide(str);
		if (wstr.length() > GBBASIC_FONT_CONTENT_MAX_SIZE) {
			warn(ws, ws->theme()->warning_FontPreviewContentIsTooLong(), true);

			return;
		}

		enqueue<Commands::Font::SetContent>()
			->with(str)
			->exec(object(), Variant((void*)entry()));

		SetChangesCleared();

		ClearUndoRedoStack();
	}

	LanguageDefinition languageDefinition(void) const {
		LanguageDefinition langDef;

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[x][0-9a-f]+", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([e][+-]?[0-9]+)?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[_]*[a-z_][a-z0-9_][A-Z_][A-Z0-9_]*[$]?", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\*\\/\\+\\-\\^\\(\\)\\[\\]\\=\\<\\>\\,\\;\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart.clear();
		langDef.CommentEnd.clear();

		langDef.CaseSensitive = true;

		langDef.Name = "Plain Text";

		return langDef;
	}

	const Palette &getDarkPalette(void) {
		static const Palette plt = {
			0xff4aa657, // None.
			0xff4aa657, // Keyword.
			0xff4aa657, // Number.
			0xff4aa657, // String.
			0xff4aa657, // Char literal.
			0xff4aa657, // Punctuation.
			0xff4aa657, // Preprocessor.
			0xff5ac8c8, // Symbol.
			0xff4aa657, // Identifier.
			0xff4aa657, // Known identifier.
			0xff4aa657, // Preproc identifier.
			0xff4aa657, // Comment (single line).
			0xff4aa657, // Comment (multi-line).
			0x90909090, // Space.
			0xff2c2c2c, // Background.
			0xffe0e0e0, // Cursor.
			0x80a06020, // Selection.
			0x804d00ff, // Error marker.
			0x8005f0fa, // Warning marker.
			0xe00020f0, // Breakpoint.
			0xe000f0f0, // Program pointer.
			0xffaf912b, // Line number.
			0x40000000, // Current line fill.
			0x40808080, // Current line fill (inactive).
			0x40a0a0a0, // Current line edge.
			0xff84f2ef, // Line edited.
			0xff307457, // Line edited saved.
			0xfffa955f  // Line edited reverted.
		};

		return plt;
	}
};

EditorFont* EditorFont::create(void) {
	EditorFontImpl* result = new EditorFontImpl();

	return result;
}

void EditorFont::destroy(EditorFont* ptr) {
	EditorFontImpl* impl = static_cast<EditorFontImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
