/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "i18n.h"
#include "text.h"

/*
** {===========================================================================
** Utilities
*/

static std::string i18nCsvQuote(const std::string &val) {
	bool needQuote = false;
	for (size_t i = 0; i < val.size(); ++i) {
		char c = val[i];
		if (c == ',' || c == '"' || c == '\r' || c == '\n') {
			needQuote = true;

			break;
		}
	}
	if (!needQuote)
		return val;

	std::string result;
	result += '"';
	for (size_t i = 0; i < val.size(); ++i) {
		const char c = val[i];
		if (c == '"')
			result += '"';
		result += c;
	}
	result += '"';

	return result;
}
static std::string i18nCsvUnquote(const std::string &val) {
	if (val.size() < 2 || val.front() != '"' || val.back() != '"')
		return val;

	std::string result;
	for (size_t i = 1; i < val.size() - 1; ++i) {
		char c = val[i];
		if (c == '"' && i + 1 < val.size() - 1 && val[i + 1] == '"') {
			result += '"';
			++i;
		} else {
			result += c;
		}
	}

	return result;
}

/* ===========================================================================} */

/*
** {===========================================================================
** I18n
*/

class I18nImpl : public I18n {
private:
	typedef std::vector<std::string> Languages;

	typedef std::vector<std::string> Item;
	typedef std::vector<Item> Items;

private:
	Languages _languages;
	Items _items;

public:
	I18nImpl() {
	}
	virtual ~I18nImpl() override {
		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(I18n** ptr, bool /* represented */) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		I18nImpl* result = static_cast<I18nImpl*>(I18n::create());
		result->_languages = _languages;
		result->_items = _items;

		*ptr = result;

		return true;
	}
	virtual bool clone(I18n** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		I18n* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;
		for (int i = 0; i < (int)_languages.size(); ++i)
			result = Math::hash(result, _languages[i]);
		for (int i = 0; i < (int)_items.size(); ++i) {
			for (int j = 0; j < (int)_items[i].size(); ++j)
				result = Math::hash(result, _items[i][j]);
		}

		return result;
	}
	virtual int compare(const I18n* other) const override {
		const I18nImpl* rhs = static_cast<const I18nImpl*>(other);

		if (_languages.size() != rhs->_languages.size())
			return (int)_languages.size() < (int)rhs->_languages.size() ? -1 : 1;
		for (int i = 0; i < (int)_languages.size(); ++i) {
			if (_languages[i] != rhs->_languages[i])
				return _languages[i] < rhs->_languages[i] ? -1 : 1;
		}

		if (_items.size() != rhs->_items.size())
			return (int)_items.size() < (int)rhs->_items.size() ? -1 : 1;
		for (int i = 0; i < (int)_items.size(); ++i) {
			if (_items[i].size() != rhs->_items[i].size())
				return (int)_items[i].size() < (int)rhs->_items[i].size() ? -1 : 1;
			for (int j = 0; j < (int)_items[i].size(); ++j) {
				if (_items[i][j] != rhs->_items[i][j])
					return _items[i][j] < rhs->_items[i][j] ? -1 : 1;
			}
		}

		return 0;
	}

	virtual void* pointer(void) override {
		return &_items;
	}

	virtual int languageCount(void) const override {
		return (int)_languages.size();
	}
	virtual bool addLanguage(int index, const std::string &lang) override {
		if (index < 0 || index > (int)_languages.size())
			return false;

		_languages.insert(_languages.begin() + index, lang);
		_languages.shrink_to_fit();
		for (int i = 0; i < (int)_items.size(); ++i) {
			_items[i].insert(_items[i].begin() + index, std::string());
			_items[i].shrink_to_fit();
		}

		return true;
	}
	virtual bool deleteLanguage(int index) override {
		if (index < 0 || index >= (int)_languages.size())
			return false;

		_languages.erase(_languages.begin() + index);
		for (int i = 0; i < (int)_items.size(); ++i)
			_items[i].erase(_items[i].begin() + index);

		return true;
	}
	virtual int getLanguageIndex(const std::string &lang) const override {
		int result = -1;
		for (int i = 0; i < (int)_languages.size(); ++i) {
			if (_languages[i] == lang) {
				result = i;

				break;
			}
		}

		return result;
	}

	virtual int itemCount(void) const override {
		return (int)_items.size();
	}
	virtual bool addItem(int index) override {
		if (index < 0 || index > (int)_items.size())
			return false;

		_items.insert(_items.begin() + index, Item(_languages.size()));
		_items.shrink_to_fit();

		return true;
	}
	virtual bool deleteItem(int index) override {
		if (index < 0 || index >= (int)_items.size())
			return false;

		_items.erase(_items.begin() + index);

		return true;
	}

	virtual bool swapLanguages(int l, int r) override {
		if (l < 0 || l >= (int)_languages.size())
			return false;
		if (r < 0 || r >= (int)_languages.size())
			return false;

		if (l == r)
			return true;

		std::swap(_languages[l], _languages[r]);
		for (int i = 0; i < (int)_items.size(); ++i)
			std::swap(_items[i][l], _items[i][r]);

		return true;
	}
	virtual bool swapItems(int l, int r) override {
		if (l < 0 || l >= (int)_items.size())
			return false;
		if (r < 0 || r >= (int)_items.size())
			return false;

		if (l == r)
			return true;

		std::swap(_items[l], _items[r]);

		return true;
	}

	virtual const char* get(const std::string &lang, const std::string &key, int* lang_, int* item) const override {
		if (lang_)
			*lang_ = -1;
		if (item)
			*item = -1;

		int langLoc = -1;
		int itemLoc = -1;
		if (!getLocation(lang, key, langLoc, itemLoc)) {
			if (lang_)
				*lang_ = langLoc;
			if (item)
				*item = itemLoc;

			return nullptr;
		}

		if (lang_)
			*lang_ = langLoc;
		if (item)
			*item = itemLoc;

		return get(langLoc, itemLoc);
	}
	virtual const char* get(int lang, int item) const override {
		if (lang < 0 || lang >= (int)_languages.size())
			return nullptr;
		if (item < 0 || item >= (int)_items.size())
			return nullptr;

		if (lang >= (int)_items[item].size()) {
			GBBASIC_ASSERT(false && "Wrong data.");

			return nullptr;
		}

		return _items[item][lang].c_str();
	}
	virtual bool set(int lang, int item, const std::string &val) override {
		if (lang < 0 || lang >= (int)_languages.size())
			return false;
		if (item < 0 || item >= (int)_items.size())
			return false;

		if (lang >= (int)_items[item].size()) {
			GBBASIC_ASSERT(false && "Wrong data.");

			return nullptr;
		}

		_items[item][lang] = val;

		return true;
	}

	virtual bool fromBlank(void) override {
		clear();

		_languages.push_back(I18N_ENGLISH_COLUMN_NAME);
		_languages.shrink_to_fit();
		_items.push_back(Item(1));
		_items.shrink_to_fit();

		return true;
	}

	virtual bool toCsv(std::string &val) const override {
		val.clear();

		if (_languages.empty())
			return true;

		for (int i = 0; i < (int)_languages.size(); ++i) {
			if (i > 0)
				val += ',';
			const std::string &name = _languages[i];
			val += i18nCsvQuote(name);
		}
		val += "\r\n";

		for (int i = 0; i < (int)_items.size(); ++i) {
			for (int j = 0; j < (int)_languages.size(); ++j) {
				if (j > 0)
					val += ',';
				val += i18nCsvQuote(_items[i][j]);
			}
			val += "\r\n";
		}

		return true;
	}
	virtual bool fromCsv(const std::string &val) override {
		clear();

		if (val.empty())
			return true;

		std::string val_ = Text::replace(val, "\r\n", "\n");
		val_ = Text::replace(val_, "\r", "\n");
		const Text::Array lines = Text::split(val_, "\n");
		if (lines.empty())
			return true;

		const Text::Array headers = Text::split(lines[0], ',', '"');
		for (int i = 0; i < (int)headers.size(); ++i) {
			const std::string name = i18nCsvUnquote(headers[i]);
			_languages.push_back(name);
		}
		_languages.shrink_to_fit();

		for (int i = 1; i < (int)lines.size(); ++i) {
			if (lines[i].empty())
				continue;

			const Text::Array fields = Text::split(lines[i], ',', '"');
			Item row(_languages.size());
			for (int j = 0; j < (int)fields.size() && j < (int)_languages.size(); ++j)
				row[j] = i18nCsvUnquote(fields[j]);
			row.shrink_to_fit();
			_items.push_back(row);
		}
		_items.shrink_to_fit();

		return true;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		for (int i = 0; i < (int)_languages.size(); ++i) {
			rapidjson::Value jkey;
			const std::string &name = _languages[i];
			jkey.SetString(name.c_str(), doc.GetAllocator());

			rapidjson::Value jarr;
			jarr.SetArray();
			for (int j = 0; j < (int)_items.size(); ++j) {
				rapidjson::Value jval;
				jval.SetString(_items[j][i].c_str(), (rapidjson::SizeType)_items[j][i].size(), doc.GetAllocator());
				jarr.PushBack(jval, doc.GetAllocator());
			}

			val.AddMember(jkey, jarr, doc.GetAllocator());
		}

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		rapidjson::Value jval;
		if (!toJson(jval, val))
			return false;

		val.Swap(jval);

		return true;
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		clear();

		if (!val.IsObject())
			return false;

		for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it) {
			const std::string key(it->name.GetString(), it->name.GetStringLength());
			if (!it->value.IsArray())
				return false;

			_languages.push_back(key);
		}
		_languages.shrink_to_fit();

		int itemCount = 0;
		for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it) {
			if (it->value.IsArray() && (int)it->value.Size() > itemCount)
				itemCount = (int)it->value.Size();
		}

		_items.resize(itemCount);
		_items.shrink_to_fit();
		for (int i = 0; i < itemCount; ++i) {
			_items[i].resize(_languages.size());
			_items[i].shrink_to_fit();
		}

		for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it) {
			const std::string key(it->name.GetString(), it->name.GetStringLength());
			int langIdx = -1;
			for (int i = 0; i < (int)_languages.size(); ++i) {
				if (_languages[i] == key) {
					langIdx = i;

					break;
				}
			}
			if (langIdx < 0)
				continue;

			const rapidjson::Value &jarr = it->value;
			if (!jarr.IsArray())
				continue;

			for (int j = 0; j < (int)jarr.Size() && j < itemCount; ++j) {
				if (jarr[j].IsString())
					_items[j][langIdx] = jarr[j].GetString();
			}
		}

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}

private:
	void clear(void) {
		_languages.clear();
		_items.clear();
	}

	bool getLocation(const std::string &lang, const std::string &key, int &lang_, int &item) const {
		lang_ = -1;
		item = -1;

		const int keyCol = getLanguageIndex(I18N_KEY_COLUMN_NAME);
		if (keyCol == -1)
			return false;
		int keyRow = -1;
		for (int i = 0; i < itemCount(); ++i) {
			const char* key_ = get(keyCol, i);
			if (!key_)
				continue;

			if (key == key_) {
				keyRow = i;

				break;
			}
		}

		lang_ = getLanguageIndex(lang);
		if (lang_ == -1)
			return false;

		if (keyRow == -1)
			return false;

		item = keyRow;

		return true;
	}
};

I18n* I18n::create(void) {
	I18nImpl* result = new I18nImpl();

	return result;
}

void I18n::destroy(I18n* ptr) {
	I18nImpl* impl = static_cast<I18nImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
