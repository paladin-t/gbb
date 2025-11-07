/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "../gbbasic.h"
#include "exporter.h"
#include "settings.h"
#include "../compiler/kernel.h"
#include "../utils/assets.h"
#include "../utils/renderer.h"
#include "../utils/window.h"
#include "../../lib/imgui/imgui.h"
#include <stack>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WIDGETS_TEXT_BUFFER_SIZE
#	define WIDGETS_TEXT_BUFFER_SIZE 256
#endif /* WIDGETS_TEXT_BUFFER_SIZE */

#ifndef WIDGETS_BUTTON_WIDTH
#	define WIDGETS_BUTTON_WIDTH 64.0f
#endif /* WIDGETS_BUTTON_WIDTH */

#ifndef WIDGETS_TOOLTIP_PADDING
#	define WIDGETS_TOOLTIP_PADDING 8.0f
#endif /* WIDGETS_TOOLTIP_PADDING */

#ifndef WIDGETS_SELECTION_GUARD
#	define WIDGETS_SELECTION_GUARD(T) \
		ProcedureGuard<int> GBBASIC_UNIQUE_NAME(__SELECTIONGUARD__)( \
			[&] (void) -> int* { \
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4((T)->style()->selectedButtonColor)); \
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4((T)->style()->selectedButtonHoveredColor)); \
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4((T)->style()->selectedButtonActiveColor)); \
				return nullptr; \
			}, \
			[&] (int*) -> void { \
				ImGui::PopStyleColor(3); \
			} \
		)
#endif /* WIDGETS_SELECTION_GUARD */

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

class Project;
class Theme;
class Workspace;

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

bool operator == (const ImVec2 &left, const ImVec2 &right);
bool operator != (const ImVec2 &left, const ImVec2 &right);

/* ===========================================================================} */

/*
** {===========================================================================
** ImGui widgets
*/

/**
 * @brief Custom ImGui widgets.
 *   The functions follow the naming convention of the original ImGui library.
 */
namespace ImGui {

typedef std::pair<ImVec2, ImVec2> Rect;

typedef std::function<void(const ImVec2 &, bool, bool, const char*)> ButtonDrawer;

typedef std::function<void(void)> TabBarDropper;

typedef std::function<void(void)> ExporterMenuHandler;

class Initializer {
private:
	int _ticks = 0;

public:
	bool begin(void) const;
	bool end(void) const;
	void update(void);
	void reset(void);
};

class Hierarchy {
public:
	typedef std::function<bool(const std::string &)> BeginHandler;
	typedef std::function<void(void)> EndHandler;

private:
	BeginHandler _begin = nullptr;
	EndHandler _end = nullptr;

	std::stack<bool> _opened;
	int _dec = 0;
	Text::Array _inc;
	Text::Array _path;

public:
	Hierarchy(BeginHandler begin, EndHandler end);

	void prepare(void);
	void finish(void);

	bool with(Text::Array::const_iterator begin, Text::Array::const_iterator end);
};

class Bubble {
public:
	typedef std::shared_ptr<Bubble> Ptr;

	struct TimeoutHandler : public Handler<TimeoutHandler, void> {
		using Handler::Handler;
	};

private:
	bool _exclusive = false;
	std::string _content;
	unsigned long long _initialTime = 0;
	unsigned long long _timeoutTime = 0;

	TimeoutHandler _timeoutHandler = nullptr;

	Initializer _init;

public:
	Bubble(
		const std::string &content,
		const TimeoutHandler &timeout
	);
	Bubble(
		const std::string &content,
		const TimeoutHandler &timeout,
		bool exclusive_
	);
	~Bubble();

	bool exclusive(void) const;

	void update(Workspace* ws);
};

class PopupBox {
public:
	typedef std::shared_ptr<PopupBox> Ptr;

protected:
	bool _exclusive = false;

public:
	PopupBox();
	virtual ~PopupBox();

	virtual bool exclusive(void) const;

	virtual void dispose(void);

	virtual void update(Workspace* ws) = 0;
};

class WaitingPopupBox : public PopupBox {
public:
	struct TimeoutHandler : public Handler<TimeoutHandler, void> {
		using Handler::Handler;
	};

private:
	bool _dim = true;
	std::string _content;
	bool _instantTimeout = false;
	unsigned long long _timeoutTime = 0;

	bool _excuted = false;
	TimeoutHandler _timeoutHandler = nullptr;

	Initializer _init;

public:
	WaitingPopupBox(
		const std::string &content,
		const TimeoutHandler &timeout
	);
	WaitingPopupBox(
		bool dim, const std::string &content,
		bool instantly, const TimeoutHandler &timeout,
		bool exclusive_ = false
	);
	virtual ~WaitingPopupBox() override;

	virtual void dispose(void) override;

	virtual void update(Workspace* ws) override;
};

class MessagePopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void> {
		using Handler::Handler;
	};
	struct ConfirmedWithOptionHandler : public Handler<ConfirmedWithOptionHandler, void, bool> {
		using Handler::Handler;
	};
	struct DeniedHandler : public Handler<DeniedHandler, void> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	std::string _title;
	std::string _content;

	ConfirmedHandler _confirmedHandler = nullptr;
	ConfirmedWithOptionHandler _confirmedWithOptionHandler = nullptr;
	std::string _confirmText;
	std::string _confirmTooltips;
	DeniedHandler _deniedHandler = nullptr;
	std::string _denyText;
	std::string _denyTooltips;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	std::string _cancelTooltips;
	std::string _optionBooleanText;
	bool _optionBooleanValue = false;
	bool _optionBooleanValueReadonly = false;

	Initializer _init;

public:
	MessagePopupBox(
		const std::string &title,
		const std::string &content,
		const ConfirmedHandler &confirm, const DeniedHandler &deny, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* denyTxt /* nullable */, const char* cancelTxt /* nullable */,
		const char* confirmTooltips /* nullable */, const char* denyTooltips /* nullable */, const char* cancelTooltips /* nullable */,
		bool exclusive_ = false
	);
	MessagePopupBox(
		const std::string &title,
		const std::string &content,
		const ConfirmedWithOptionHandler &confirm, const DeniedHandler &deny, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* denyTxt /* nullable */, const char* cancelTxt /* nullable */,
		const char* confirmTooltips /* nullable */, const char* denyTooltips /* nullable */, const char* cancelTooltips /* nullable */,
		const char* optionBoolTxt /* nullable */, bool optionBoolValue, bool optionBoolValueReadonly,
		bool exclusive_ = false
	);
	virtual ~MessagePopupBox() override;

	virtual void update(Workspace* ws) override;
};

class InputPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const char*> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	std::string _title;
	std::string _content;
	std::string _default;
	unsigned _flags = 0;
	char _buffer[WIDGETS_TEXT_BUFFER_SIZE]; // Fixed size.

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	InputPopupBox(
		const std::string &title,
		const std::string &content, const std::string &default_, unsigned flags,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~InputPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class StarterKitsPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, int, const char*> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	std::string _template;
	int _templateCursor = 0;
	std::string _content;
	std::string _default;
	unsigned _flags = 0;
	char _buffer[WIDGETS_TEXT_BUFFER_SIZE]; // Fixed size.

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	StarterKitsPopupBox(
		Theme* theme,
		const std::string &title,
		const std::string &template_,
		const std::string &content, const std::string &default_, unsigned flags,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~StarterKitsPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class SortAssetsPopupBox : public PopupBox {
public:
	typedef std::vector<int> Order;
	typedef std::map<unsigned, Order> Orders;

	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, Orders> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	unsigned _category;
	Orders _orders;
	Orders _ordersShadow;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	SortAssetsPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		Project* project,
		unsigned category,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~SortAssetsPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class RomBuildSettingsPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const char*, const char*, bool> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	std::string _cartType;
	std::string _sramType;
	bool _hasRtc = false;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	RomBuildSettingsPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		Project* project,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~RomBuildSettingsPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class EmulatorBuildSettingsPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const char*, const char*, Bytes::Ptr> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Input* _input = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Exporter::Settings _settings;
	bool _hasArgs = false;
	char _argsBuffer[WIDGETS_TEXT_BUFFER_SIZE]; // Fixed size.
	bool _hasIcon = false;
	Bytes::Ptr _icon = nullptr;
	Texture::Ptr _iconTexture = nullptr;
	Texture::Ptr _invalidIconTexture = nullptr;
	bool _invalidIconSize = false;
	bool _activeClassicPaletteShowColorPicker = false;
	int _activeClassicPaletteIndex = -1;
	int _activeGamepadIndex = -1;
	int _activeButtonIndex = -1;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	EmulatorBuildSettingsPopupBox(
		Renderer* rnd,
		Input* input, Theme* theme,
		const std::string &title,
		const std::string &settings, const char* args, bool hasIcon,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~EmulatorBuildSettingsPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class SearchPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const char*> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Settings &_settings; // Foreign.
	std::string _content;
	std::string _default;
	unsigned _flags = 0;
	char _buffer[WIDGETS_TEXT_BUFFER_SIZE]; // Fixed size.

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	SearchPopupBox(
		Theme* theme,
		const std::string &title,
		Settings &settings,
		const std::string &content, const std::string &default_, unsigned flags,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~SearchPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class EditorPopupBox : public PopupBox {
public:
	typedef std::function<void(const std::initializer_list<Variant> &)> ChangedHandler;

	typedef std::function<const std::string &(void)> TextGetter;
	typedef std::function<void(const std::string &)> TextSetter;
};

class ActorSelectorPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, int> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	int _selection = -1;

	ConfirmedHandler _confirmedHandler = nullptr;
	CanceledHandler _canceledHandler = nullptr;

	Initializer _init;

public:
	ActorSelectorPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		Project* project,
		int index,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel
	);
	virtual ~ActorSelectorPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class FontResolverPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const char*, const Math::Vec2i*> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct SelectedHandler : public Handler<SelectedHandler, bool, FontResolverPopupBox*> {
		using Handler::Handler;
	};
	struct CustomHandler : public Handler<CustomHandler, void, FontResolverPopupBox*, float> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	bool _withSize = false;
	Math::Vec2i _vec2i;
	Math::Vec2i _vec2iMin;
	Math::Vec2i _vec2iMax;
	std::string _vec2iText;
	std::string _vec2iXText;
	std::string _content;
	std::string _path;
	bool _exists = false;
	Text::Array _filter;
	bool _requireExisting = true;
	std::string _browseText;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	SelectedHandler _selectedHandler = nullptr;
	CustomHandler _customHandler = nullptr;

	Initializer _init;

public:
	FontResolverPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		const Math::Vec2i &vec2i, const Math::Vec2i &vec2iMin, const Math::Vec2i &vec2iMax, const char* vec2iTxt /* nullable */, const char* vec2iXTxt /* nullable */,
		const std::string &content, const std::string &default_,
		const Text::Array &filter,
		bool requireExisting,
		const char* browseTxt /* nullable */,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */,
		const SelectedHandler &selected /* nullable */,
		const CustomHandler &custom /* nullable */
	);

	virtual ~FontResolverPopupBox() override;

	const std::string &path(void) const;
	bool exists(void) const;
	const Math::Vec2i &vec2iValue(void) const;
	void vec2iValue(const Math::Vec2i &val);
	const Math::Vec2i &vec2iMinValue(void) const;
	void vec2iMinValue(const Math::Vec2i &val);
	const Math::Vec2i &vec2iMaxValue(void) const;
	void vec2iMaxValue(const Math::Vec2i &val);

	virtual void update(Workspace* ws) override;
};

class MapResolverPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const int*, const char*, bool> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct SelectedHandler : public Handler<SelectedHandler, bool, MapResolverPopupBox*> {
		using Handler::Handler;
	};
	struct CustomHandler : public Handler<CustomHandler, void, MapResolverPopupBox*, float> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	bool _withTilesIndex = true;
	std::string _withIndexText;
	std::string _withImageText;
	int _index = 0;
	int _indexMin = 0;
	int _indexMax = 0;
	std::string _indexText;
	std::string _path;
	std::string _content;
	bool _requireExisting = true;
	std::string _browseText;
	Text::Array _filter;
	bool _exists = false;
	bool _allowFlip = true;
	std::string _allowFlipText;
	std::string _allowFlipTooltipText;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	SelectedHandler _selectedHandler = nullptr;
	CustomHandler _customHandler = nullptr;

	Initializer _init;

public:
	MapResolverPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		const char* withIndexTxt /* nullable */, const char* withImageTxt /* nullable */,
		int index, int indexMin, int indexMax, const char* indexTxt /* nullable */,
		const std::string &content, const std::string &default_,
		const Text::Array &filter,
		bool requireExisting,
		const char* browseTxt /* nullable */,
		bool allowFlip,
		const char* allowFlipTxt /* nullable */, const char* allowFlipTooltipTxt /* nullable */,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */,
		const SelectedHandler &selected /* nullable */,
		const CustomHandler &custom /* nullable */
	);

	virtual ~MapResolverPopupBox() override;

	const std::string &path(void) const;
	bool exists(void) const;
	int indexValue(void) const;
	void indexValue(int val);
	int indexMinValue(void) const;
	void indexMinValue(int val);
	int indexMaxValue(void) const;
	void indexMaxValue(int val);

	virtual void update(Workspace* ws) override;
};

class SceneResolverPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, int, bool> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct SelectedHandler : public Handler<SelectedHandler, bool, SceneResolverPopupBox*> {
		using Handler::Handler;
	};
	struct CustomHandler : public Handler<CustomHandler, void, SceneResolverPopupBox*, float> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	int _index = 0;
	int _indexMin = 0;
	int _indexMax = 0;
	std::string _indexText;
	bool _useGravity = false;
	std::string _useGravityText;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	SelectedHandler _selectedHandler = nullptr;
	CustomHandler _customHandler = nullptr;

	Initializer _init;

public:
	SceneResolverPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		Project* project,
		int index, int indexMin, int indexMax, const char* indexTxt /* nullable */,
		bool useGravity, const char* useGravityText,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */,
		const SelectedHandler &selected /* nullable */,
		const CustomHandler &custom /* nullable */
	);

	virtual ~SceneResolverPopupBox() override;

	int indexValue(void) const;
	void indexValue(int val);
	int indexMinValue(void) const;
	void indexMinValue(int val);
	int indexMaxValue(void) const;
	void indexMaxValue(int val);

	virtual void update(Workspace* ws) override;
};

class FileResolverPopupBox : public PopupBox {
public:
	struct ConfirmedHandler_Path : public Handler<ConfirmedHandler_Path, void, const char*> {
		using Handler::Handler;
	};
	struct ConfirmedHandler_PathBoolean : public Handler<ConfirmedHandler_PathBoolean, void, const char*, bool> {
		using Handler::Handler;
	};
	struct ConfirmedHandler_PathInteger : public Handler<ConfirmedHandler_PathInteger, void, const char*, int> {
		using Handler::Handler;
	};
	struct ConfirmedHandler_PathVec2i : public Handler<ConfirmedHandler_PathVec2i, void, const char*, const Math::Vec2i &> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct SelectedHandler : public Handler<SelectedHandler, bool, FileResolverPopupBox*> {
		using Handler::Handler;
	};
	struct CustomHandler : public Handler<CustomHandler, void, FileResolverPopupBox*, float> {
		using Handler::Handler;
	};
	struct Vec2iResolver : public Handler<Vec2iResolver, bool, const char*, Math::Vec2i*> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	std::string _content;
	bool _toSave = false;
	std::string _path;
	bool _exists = false;
	Text::Array _filter;
	bool _requireExisting = true;
	std::string _browseText;
	bool _boolean = true;
	std::string _booleanText;
	int _integer;
	int _integerMin;
	int _integerMax;
	std::string _inteterText;
	Math::Vec2i _vec2i;
	Math::Vec2i _vec2iMin;
	Math::Vec2i _vec2iMax;
	std::string _vec2iText;
	std::string _vec2iXText;
	bool _toResolveVec2i = false;

	ConfirmedHandler_Path _confirmedHandler_Path = nullptr;
	ConfirmedHandler_PathBoolean _confirmedHandler_PathBool = nullptr;
	ConfirmedHandler_PathInteger _confirmedHandler_PathInteger = nullptr;
	ConfirmedHandler_PathVec2i _confirmedHandler_PathVec2i = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	SelectedHandler _selectedHandler = nullptr;
	CustomHandler _customHandler = nullptr;
	Vec2iResolver _vec2iResolver = nullptr;
	std::string _vec2iResolvingText;

	Initializer _init;

public:
	FileResolverPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		const std::string &content, const std::string &default_,
		const Text::Array &filter,
		bool requireExisting,
		const char* browseTxt /* nullable */,
		const ConfirmedHandler_Path &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */,
		const SelectedHandler &selected /* nullable */,
		const CustomHandler &custom /* nullable */
	);
	FileResolverPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		const std::string &content, const std::string &default_,
		const Text::Array &filter,
		bool requireExisting,
		const char* browseTxt /* nullable */,
		bool bool_, const char* boolTxt /* nullable */,
		const ConfirmedHandler_PathBoolean &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */,
		const SelectedHandler &selected /* nullable */,
		const CustomHandler &custom /* nullable */
	);
	FileResolverPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		const std::string &content, const std::string &default_,
		const Text::Array &filter,
		bool requireExisting,
		const char* browseTxt /* nullable */,
		int integer, int min, int max, const char* intTxt /* nullable */,
		const ConfirmedHandler_PathInteger &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */,
		const SelectedHandler &selected /* nullable */,
		const CustomHandler &custom /* nullable */
	);
	FileResolverPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		const std::string &content, const std::string &default_,
		const Text::Array &filter,
		bool requireExisting,
		const char* browseTxt /* nullable */,
		const Math::Vec2i &vec2i, const Math::Vec2i &vec2iMin, const Math::Vec2i &vec2iMax, const char* vec2iTxt /* nullable */, const char* vec2iXTxt /* nullable */,
		const ConfirmedHandler_PathVec2i &confirm, const CanceledHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */,
		const SelectedHandler &selected /* nullable */,
		const CustomHandler &custom /* nullable */,
		const Vec2iResolver &resolveVec2i /* nullable */, const char* resolveVec2iTxt /* nullable */
	);

	virtual ~FileResolverPopupBox() override;

	bool toSave(void) const;
	FileResolverPopupBox* toSave(bool val);

	const std::string &path(void) const;
	bool exists(void) const;
	bool booleanValue(void) const;
	void booleanValue(bool val);
	int integerValue(void) const;
	void integerValue(int val);
	int integerMinValue(void) const;
	void integerMinValue(int val);
	int integerMaxValue(void) const;
	void integerMaxValue(int val);
	const Math::Vec2i &vec2iValue(void) const;
	void vec2iValue(const Math::Vec2i &val);
	const Math::Vec2i &vec2iMinValue(void) const;
	void vec2iMinValue(const Math::Vec2i &val);
	const Math::Vec2i &vec2iMaxValue(void) const;
	void vec2iMaxValue(const Math::Vec2i &val);

	virtual void update(Workspace* ws) override;
};

class ProjectPropertyPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, Project*> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct AppliedHandler : public Handler<AppliedHandler, void, Project*> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	float _superFeaturesBeginX = 0.0f;
	float _superFeaturesEndX = 0.0f;
	float _superFeaturesEnabledWidth = 72.0f;
	Texture::Ptr _borderTexture = nullptr;
	std::string _borderError;
	Semaphore _borderTextureIsBeingGenerated;
	bool _activeSuperPaletteShowColorPicker = false;
	int _activeSuperPaletteIndex = -1;
	Text::Array _superPaletteNames;
	Project* _project = nullptr; // Foreign.
	Project* _projectShadow = nullptr;
	char _buffer[WIDGETS_TEXT_BUFFER_SIZE]; // Fixed size.

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	AppliedHandler _appliedHandler = nullptr;
	std::string _applyText;

	Initializer _init;

public:
	ProjectPropertyPopupBox(
		Renderer* rnd,
		Theme* theme,
		const std::string &title,
		Project* project,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* applyTxt /* nullable */
	);
	virtual ~ProjectPropertyPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class PreferencesPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, Settings &> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct AppliedHandler : public Handler<AppliedHandler, void, Settings &> {
		using Handler::Handler;
	};

	typedef std::function<bool(void)> BoolGetter;

private:
	Renderer* _renderer = nullptr; // Foreign.
	Input* _input = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Settings &_settings; // Foreign.
	Settings _settingsShadow;
	BoolGetter _getBorderlessWritable = nullptr;
	BoolGetter _getBorderless = nullptr;
	bool _activeClassicPaletteShowColorPicker = false;
	int _activeClassicPaletteIndex = -1;
	int _activeGamepadIndex = -1;
	int _activeButtonIndex = -1;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	AppliedHandler _appliedHandler = nullptr;
	std::string _applyText;

	Initializer _init;

public:
	PreferencesPopupBox(
		Renderer* rnd,
		Input* input, Theme* theme,
		const std::string &title,
		Settings &settings,
		BoolGetter getBorderlessWritable, BoolGetter getBorderless,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* applyTxt /* nullable */
	);
	virtual ~PreferencesPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class ActivitiesPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	std::string _title;
	std::string _activities;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;

	Initializer _init;

public:
	ActivitiesPopupBox(
		Renderer* rnd,
		const std::string &title,
		Workspace* ws,
		const ConfirmedHandler &confirm,
		const char* confirmTxt /* nullable */
	);
	virtual ~ActivitiesPopupBox() override;

	virtual void update(Workspace* ws) override;
};

class AboutPopupBox : public PopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void> {
		using Handler::Handler;
	};

private:
	Renderer* _renderer = nullptr; // Foreign.
	std::string _title;
	std::string _desc;
	std::string _specs;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;

	Initializer _init;

public:
	AboutPopupBox(
		Renderer* rnd,
		const std::string &title,
		const ConfirmedHandler &confirm,
		const char* confirmTxt /* nullable */,
		const char* licenses /* nullable */
	);
	virtual ~AboutPopupBox() override;

	virtual void update(Workspace* ws) override;
};

ImVec2 GetMousePosOnCurrentItem(const ImVec2* ref_pos = nullptr);

void PushID(const std::string &str_id);

Rect LastItemRect(void);

void Dummy(const ImVec2 &size, ImU32 col);
void Dummy(const ImVec2 &size, const ImVec4 &col);

void NewLine(float height);

void ScrollToItem(int flags, float offset_y);

ImVec2 WindowResizingPadding(void);
bool Begin(const std::string &name, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
void CentralizeWindow(void);
void EnsureWindowVisible(void);

void OpenPopup(const std::string &str_id, ImGuiPopupFlags popup_flags = ImGuiPopupFlags_None);
bool BeginPopupModal(const std::string &name, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

float TitleBarHeight(void);
/**
 * @brief Adds custom buttons aside the close button, layouts from right to left.
 */
bool TitleBarCustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip = nullptr);

ImVec2 ScrollbarSize(bool horizontal);

ImVec2 CustomButtonAutoPosition(void);

bool CustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip = nullptr);

void CustomAddButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomRemoveButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomRenameButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomClearButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomMinButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomMaxButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomCloseButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomPlayButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomStopButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);

bool LineGraph(const ImVec2 &size, ImU32 border_col, ImU32 grid_col, ImU32 content_col, ImU32 drawing_col, const Math::Vec2i &grid_step, const int* old_values, int* new_values /* nullable */, int min, int max, int count, bool circular, ImVec2* prev_pos /* nullable */, bool* mouse_down /* nullable */, const char* tooltip_fmt /* nullable */);
bool BarGraph(const ImVec2 &size, ImU32 border_col, ImU32 grid_col, ImU32 content_col, ImU32 drawing_col, const Math::Vec2i &grid_step, const int* old_values, int* new_values /* nullable */, int min, int max, int count, float base, ImVec2* prev_pos /* nullable */, bool* mouse_down /* nullable */, const char* tooltip_fmt /* nullable */);

void TextUnformatted(const std::string &text);

bool Url(const char* label, const char* link, bool adj = false);

void OpenPopupTooltip(const char* id);
void PopupTooltip(const char* id, const std::string &text);
bool PopupTooltip(const char* id, const std::string &text, const char* clear_txt /* nullable */);
void PopupHelpTooltip(const std::string &text);
void SetTooltip(const std::string &text);
void SetHelpTooltip(const std::string &text);

bool Checkbox(const std::string &label, bool* v);

bool Combo(
	const char* label,
	int* current_item,
	const char* const items[], int items_count,
	const char* const tooltips[],
	int popup_max_height_in_items = -1
);
bool Combo(
	const char* label,
	int* current_item,
	bool (* items_getter)(void* /* data */, int /* idx */, const char** /* out_text */), void* data, int items_count,
	bool (* tooltips_getter)(void* /* data */, int /* idx */, const char** /* out_text */), void* tooltips,
	int popup_max_height_in_items = -1
);
bool Combo(const std::string &label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
bool Combo(const std::string &label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
bool Combo(const std::string &label, int* current_item, bool (* items_getter)(void* /* data */, int /* idx */, const char** /* out_text */), void* data, int items_count, int popup_max_height_in_items = -1);

void Indicator(const ImVec2 &min, const ImVec2 &max, float thickness = 3);
void Indicator(const char* label, const ImVec2 &pos);

bool IntegerModifier(Renderer* rnd, Theme* theme, int* val, int min, int max, float width, const char* fmt = nullptr, const char* tooltip = nullptr);

bool ByteMatrice(Renderer* rnd, Theme* theme, Byte* val, bool* set /* nullable */, int* bit /* nullable */, Byte editable, float width, const char* tooltipLnHigh = nullptr, const char* tooltipLnLow = nullptr, const char** tooltipBits = nullptr);
bool ByteMatrice(Renderer* rnd, Theme* theme, Byte* val, Byte editable, float width, const char* tooltipLnHigh = nullptr, const char* tooltipLnLow = nullptr, const char** tooltipBits = nullptr);

bool ProgressBar(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", bool readonly = false);
bool ProgressBar(const char* label, float* v, float* v_text, float v_min, float v_max, const char* format = "%.3f", bool readonly = false);

bool Button(const std::string &label, const ImVec2 &size = ImVec2(0, 0));
bool Button(const char* label, const ImVec2 &size, ImGuiMouseButton* mouse_button /* nullable */, bool* double_clicked /* nullable */, bool highlight);
bool Button(const std::string &label, const ImVec2 &size, ImGuiMouseButton* mouse_button /* nullable */, bool* double_clicked /* nullable */, bool highlight);

void CentralizeButton(int count = 1, float width = WIDGETS_BUTTON_WIDTH);

bool ColorButton(const char* desc_id, const ImVec4 &col, ImGuiColorEditFlags flags, const ImVec2 &size, const char* tooltip /* nullable */);

bool ImageButton(ImTextureID texture_id, const ImVec2 &size, const ImVec4 &tint_col, bool selected = false, const char* tooltip = nullptr);

bool AdjusterButton(const char* label, size_t label_len, const ImVec2* sizeptr /* nullable */, const ImVec2 &charSize, ImGuiMouseButton* mouse_button /* nullable */, bool* double_clicked /* nullable */, bool highlight);
bool AdjusterButton(const char* label, size_t label_len, const ImVec2 &charSize, ImGuiMouseButton* mouse_button /* nullable */, bool* double_clicked /* nullable */, bool highlight);
bool AdjusterButton(const std::string &label, const ImVec2* sizeptr /* nullable */, const ImVec2 &charSize, ImGuiMouseButton* mouse_button /* nullable */, bool* double_clicked /* nullable */, bool highlight);
bool AdjusterButton(const std::string &label, const ImVec2 &charSize, ImGuiMouseButton* mouse_button /* nullable */, bool* double_clicked /* nullable */, bool highlight);

bool BulletButtons(float width, float height, int count, bool* changed /* nullable */, UInt32 &mask, ImGuiMouseButton* mouse_button /* nullable */, const char* non_selected_text, ImU32 col, const char** tooltips, ImU32 tooltip_col);
bool BulletButtons(float width, float height, int count, bool* changed /* nullable */, UInt32 &mask, ImGuiMouseButton* mouse_button /* nullable */, const char** texts, ImU32 col, const char** tooltips, ImU32 tooltip_col);
bool BulletButtons(float width, float height, int count, bool* changed /* nullable */, int* index /* nullable */, ImGuiMouseButton* mouse_button /* nullable */, const char* non_selected_text, ImU32 col, const char** tooltips, ImU32 tooltip_col);
bool BulletButtons(float width, float height, int count, bool* changed /* nullable */, int* index /* nullable */, ImGuiMouseButton* mouse_button /* nullable */, const char** texts, ImU32 col, const char** tooltips, ImU32 tooltip_col);

bool PianoButtons(float width, float height, int* index /* nullable */, const char** tooltips, ImU32 tooltip_col);

void NineGridsImage(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool horizontal = true, bool normal = true);

bool BeginMenu(const std::string &label, bool enabled = true);
bool BeginMenu(ImGuiID id, ImTextureID texture_id, const ImVec2 &size, bool enabled = true, ImU32 col = IM_COL32_WHITE);
bool MenuItem(const std::string &label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
bool MenuItem(const std::string &label, const char* shortcut, bool* selected, bool enabled = true);
bool MenuBarButton(const std::string &label, const ImVec2 &size = ImVec2(0, 0), const char* tooltip = nullptr);
bool MenuBarImageButton(ImTextureID texture_id, const ImVec2 &size, const ImVec4& tint_col, const char* tooltip = nullptr);
void MenuBarTextUnformatted(const std::string &text);

float ColorPickerMinWidthForInput(void);

float TabBarHeight(void);
bool BeginTabItem(const std::string &str_id, const std::string &label, bool* p_open = nullptr, ImGuiTabItemFlags flags = ImGuiTabItemFlags_None);
bool BeginTabItem(const std::string &label, bool* p_open = nullptr, ImGuiTabItemFlags flags = ImGuiTabItemFlags_None);
bool BeginTabItem(const std::string &label, bool* p_open, ImGuiTabItemFlags flags, ImU32 col);
void TabBarTabListPopupButton(TabBarDropper dropper);

bool BeginTable(const std::string &str_id, int column, ImGuiTableFlags flags = ImGuiTableFlags_None, const ImVec2 &outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f);
void TableSetupColumn(const std::string &label, ImGuiTableColumnFlags flags = ImGuiTableFlags_None, float init_width_or_weight = 0.0f, ImU32 user_id = 0);

/**
 * @brief Uses specific textures instead of the default arrow or bullet for
 *   node head.
 */
bool TreeNode(ImTextureID texture_id, ImTextureID open_tex_id, const std::string &label, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None, ImGuiButtonFlags button_flags = ImGuiButtonFlags_None, ImU32 col = IM_COL32_WHITE);
/**
 * @brief Uses checkbox instead of the default arrow or bullet for node head.
 */
bool TreeNode(bool* checked, const std::string &label, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None, ImGuiButtonFlags button_flags = ImGuiButtonFlags_None);

bool CollapsingHeader(const char* label, float width, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None);

bool Selectable(const std::string &label, bool selected = false, ImGuiSelectableFlags flags = ImGuiSelectableFlags_None, const ImVec2 &size = ImVec2(0, 0));
bool Selectable(const std::string &label, bool* p_selected, ImGuiSelectableFlags flags = ImGuiSelectableFlags_None, const ImVec2 &size = ImVec2(0, 0));

bool ExporterMenu(
	const Exporter::Array &exporters,
	const char* menu,
	Exporter* &selected,
	ExporterMenuHandler prev /* nullable */, ExporterMenuHandler post /* nullable */
);

bool ExampleMenu(
	const EntryWithPath::List &examples,
	std::string &selected
);

bool MusicMenu(
	const Entry::Dictionary &music,
	std::string &selected
);

bool SfxMenu(
	const Entry::Dictionary &sfx,
	std::string &selected
);

bool DocumentMenu(
	const Entry::Dictionary &documents,
	std::string &selected
);

bool LinkMenu(
	const Entry::Dictionary &links,
	std::string &url, std::string &message
);

void ConfigGamepads(
	Input* input,
	Input::Gamepad* pads, int pad_count,
	int* active_pad_index /* nullable */, int* active_btn_index /* nullable */,
	const char* label_wait /* nullable */
);
void ConfigOnscreenGamepad(
	bool* enabled,
	bool* swap_ab,
	float* scale, float* padding_x, float* padding_y,
	const char* label_enabled /* nullable */,
	const char* label_swap_ab /* nullable */,
	const char* label_scale /* nullable */, const char* label_padding_x /* nullable */, const char* label_padding_y /* nullable */
);

}

/* ===========================================================================} */

#endif /* __WIDGETS_H__ */
