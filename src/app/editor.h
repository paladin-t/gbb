/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "../utils/editable.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED
#	define EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED 1
#endif /* EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED */

#ifndef EDITOR_DEBOUNCE_INTERVAL
#	define EDITOR_DEBOUNCE_INTERVAL 3.0
#endif /* EDITOR_DEBOUNCE_INTERVAL */

/* ===========================================================================} */

/*
** {===========================================================================
** Editor
*/

class Editor : public Editable {
protected:
	struct Ref {
	public:
		typedef std::pair<float, float> Splitter;

	private:
		bool _verticalScrollBarVisible = false;
		int _refreshingTicks = 0;

	public:
		float windowWidth(float exp);
		int windowFlags(void) const;
		void windowResized(void);

		Splitter split(void);
	};

	struct Debounce {
	private:
		long long _interval = 0;

		long long _lastModificationTimestamp = 0;
		bool _manuallyTriggered = false;

	public:
		Debounce();
		Debounce(double intervalSeconds);
		~Debounce();

		void trigger(void);
		void modified(void);
		bool fire(void);

		void clear(void);
	};
};

/* ===========================================================================} */

#endif /* __EDITOR_H__ */
