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
	typedef std::vector<std::string> Row;
	typedef std::vector<Row> Table;

private:
	Table _table;

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
		result->_table = _table;

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
		for (int i = 0; i < (int)_table.size(); ++i) {
			for (int j = 0; j < (int)_table[i].size(); ++j)
				result = Math::hash(result, _table[i][j]);
		}

		return result;
	}
	virtual int compare(const I18n* other) const override {
		const I18nImpl* rhs = static_cast<const I18nImpl*>(other);

		if (_table.size() != rhs->_table.size())
			return (int)_table.size() < (int)rhs->_table.size() ? -1 : 1;

		for (int i = 0; i < (int)_table.size(); ++i) {
			if (_table[i].size() != rhs->_table[i].size())
				return (int)_table[i].size() < (int)rhs->_table[i].size() ? -1 : 1;

			for (int j = 0; j < (int)_table[i].size(); ++j) {
				if (_table[i][j] != rhs->_table[i][j])
					return _table[i][j] < rhs->_table[i][j] ? -1 : 1;
			}
		}

		return 0;
	}

	virtual void* pointer(void) override {
		return &_table;
	}

	virtual int languageCount(void) const override {
		if (_table.empty())
			return 0;

		return (int)_table.front().size();
	}
	virtual bool addLanguage(int col, const std::string &lang) override {
		if (col < 0 || col > languageCount())
			return false;

		if (_table.empty()) {
			_table.push_back(Row());
			_table.shrink_to_fit();
		}

		Row &head = _table.front();
		head.insert(head.begin() + col, lang);
		head.shrink_to_fit();
		for (int i = 1; i < (int)_table.size(); ++i) {
			_table[i].insert(_table[i].begin() + col, std::string());
			_table[i].shrink_to_fit();
		}

		return true;
	}
	virtual bool deleteLanguage(int col) override {
		if (col < 0 || col >= languageCount())
			return false;

		if (_table.empty())
			return false;

		Row &head = _table.front();
		head.erase(head.begin() + col);

		for (int i = 1; i < (int)_table.size(); ++i)
			_table[i].erase(_table[i].begin() + col);

		return true;
	}
	virtual int getLanguageIndex(const std::string &lang) const override {
		if (_table.empty())
			return -1;

		const Row &head = _table.front();
		int result = -1;
		for (int i = 0; i < (int)head.size(); ++i) {
			if (head[i] == lang) {
				result = i;

				break;
			}
		}

		return result;
	}
	virtual bool getLanguage(int col, std::string &lang) const override {
		lang.clear();

		if (_table.empty())
			return false;

		const Row &head = _table.front();
		if (col < 0 || col >= (int)head.size())
			return false;

		lang = head[col];

		return true;
	}
	virtual bool setLanguage(int col, const std::string &lang) override {
		if (_table.empty())
			return false;

		Row &head = _table.front();
		if (col < 0 || col >= (int)head.size())
			return false;

		head[col] = lang;

		return true;
	}

	virtual int itemCount(void) const override {
		if (_table.empty())
			return 0;

		return (int)_table.size() - 1;
	}
	virtual bool addItem(int row, const char* item) override {
		++row;
		if (row < 1 || row > (int)_table.size())
			return false;

		Row row_(languageCount());
		if (item && !row_.empty())
			row_[0] = item;

		_table.insert(_table.begin() + row, row_);
		_table.shrink_to_fit();

		return true;
	}
	virtual bool deleteItem(int row) override {
		++row;
		if (row < 1 || row >= (int)_table.size())
			return false;

		_table.erase(_table.begin() + row);

		return true;
	}
	virtual int getItemIndex(const std::string &item) const override {
		const int keyCol = getLanguageIndex(I18N_KEY_COLUMN_NAME);
		if (keyCol == -1)
			return -1;

		for (int i = 0; i < itemCount(); ++i) {
			const char* key_ = getContent(keyCol, i);
			if (!key_)
				continue;

			if (item == key_)
				return i;
		}

		return -1;
	}

	virtual bool swapLanguages(int l, int r) override {
		const int n = languageCount();
		if (l < 0 || l >= n)
			return false;
		if (r < 0 || r >= n)
			return false;

		if (l == r)
			return true;

		for (int i = 0; i < (int)_table.size(); ++i)
			std::swap(_table[i][l], _table[i][r]);

		return true;
	}
	virtual bool swapItems(int l, int r) override {
		++l;
		++r;
		if (l < 1 || l >= (int)_table.size())
			return false;
		if (r < 1 || r >= (int)_table.size())
			return false;

		if (l == r)
			return true;

		std::swap(_table[l], _table[r]);

		return true;
	}

	virtual const char* getContent(const std::string &lang, const std::string &key, int* lang_, int* item) const override {
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

		return getContent(langLoc, itemLoc);
	}
	virtual const char* getContent(int lang, int item) const override {
		++item;
		if (lang < 0 || lang >= languageCount())
			return nullptr;
		if (item < 1 || item >= (int)_table.size())
			return nullptr;

		if (lang >= (int)_table[item].size()) {
			GBBASIC_ASSERT(false && "Wrong data.");

			return nullptr;
		}

		return _table[item][lang].c_str();
	}
	virtual bool setContent(int lang, int item, const std::string &val) override {
		++item;
		if (lang < 0 || lang >= languageCount())
			return false;
		if (item < 1 || item >= (int)_table.size())
			return false;

		if (lang >= (int)_table[item].size()) {
			GBBASIC_ASSERT(false && "Wrong data.");

			return nullptr;
		}

		_table[item][lang] = val;

		return true;
	}

	virtual int columnCount(void) const override {
		if (_table.empty())
			return 0;

		const Row &row = _table.front();

		return (int)row.size();
	}
	virtual int rowCount(void) const override {
		if (_table.empty())
			return 0;

		return (int)_table.size();
	}
	virtual const char* get(int col, int row) const override {
		if (row < 0 || row >= (int)_table.size())
			return nullptr;

		const Row &row_ = _table[row];
		if (col < 0 || col >= (int)row_.size())
			return false;

		return _table[row][col].c_str();
	}
	virtual bool set(int col, int row, const std::string &val) override {
		if (row < 0 || row >= (int)_table.size())
			return false;

		Row &row_ = _table[row];
		if (col < 0 || col >= (int)row_.size())
			return false;

		_table[row][col] = val;

		return true;
	}

	virtual bool fromBlank(void) override {
		clear();

		addLanguage(0, I18N_KEY_COLUMN_NAME);
		addLanguage(1, I18N_ENGLISH_COLUMN_NAME);
		addItem(0, "hello");
		setContent(1, 0, "Hello");

		return true;
	}

	virtual bool toCsv(std::string &val) const override {
		val.clear();

		if (_table.empty())
			return true;

		for (int i = 0; i < (int)_table.size(); ++i) {
			for (int j = 0; j < (int)_table[i].size(); ++j) {
				if (j > 0)
					val += ',';
				val += i18nCsvQuote(_table[i][j]);
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
		Text::Array languages;
		for (int i = 0; i < (int)headers.size(); ++i) {
			const std::string name = i18nCsvUnquote(headers[i]);
			languages.push_back(name);
		}
		languages.shrink_to_fit();

		for (int i = 0; i < (int)lines.size(); ++i) {
			if (lines[i].empty())
				continue;

			const Text::Array fields = Text::split(lines[i], ',', '"');
			Row row(languages.size());
			for (int j = 0; j < (int)fields.size() && j < (int)languages.size(); ++j)
				row[j] = i18nCsvUnquote(fields[j]);
			row.shrink_to_fit();
			_table.push_back(row);
		}
		_table.shrink_to_fit();

		return true;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		for (int i = 0; i < languageCount(); ++i) {
			rapidjson::Value jkey;
			std::string name;
			getLanguage(i, name);
			jkey.SetString(name.c_str(), doc.GetAllocator());

			rapidjson::Value jarr;
			jarr.SetArray();
			for (int j = 1; j < (int)_table.size(); ++j) {
				rapidjson::Value jval;
				jval.SetString(_table[j][i].c_str(), (rapidjson::SizeType)_table[j][i].size(), doc.GetAllocator());
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

			addLanguage(languageCount(), key);
		}

		int itemCount = 0;
		for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it) {
			if (it->value.IsArray() && (int)it->value.Size() > itemCount)
				itemCount = (int)it->value.Size();
		}

		_table.resize(itemCount + 1);
		_table.shrink_to_fit();
		const int n = languageCount();
		for (int i = 1; i < (int)_table.size(); ++i) {
			_table[i].resize(n);
			_table[i].shrink_to_fit();
		}

		int langIdx = 0;
		for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it, ++langIdx) {
			const std::string key(it->name.GetString(), it->name.GetStringLength());

			const rapidjson::Value &jarr = it->value;
			if (!jarr.IsArray())
				continue;

			for (int j = 0; j < (int)jarr.Size() && j < itemCount; ++j) {
				std::string content;
				if (jarr[j].IsString())
					content = jarr[j].GetString();
				setContent(langIdx, j, content);
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
		_table.clear();
	}

	/**
	 * @param[out] lang_
	 * @param[out] item Starts from 0, already skipped the No. 0 row for languages.
	 */
	bool getLocation(const std::string &lang, const std::string &key, int &lang_, int &item) const {
		lang_ = -1;
		item = -1;

		const int keyCol = getLanguageIndex(I18N_KEY_COLUMN_NAME);
		if (keyCol == -1)
			return false;

		int keyRow = -1;
		for (int i = 0; i < itemCount(); ++i) {
			const char* key_ = getContent(keyCol, i);
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
