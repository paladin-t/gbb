/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "web.h"
#if defined GBBASIC_OS_HTML
#	include "web_html.h"
#else /* GBBASIC_OS_HTML */
#endif /* GBBASIC_OS_HTML */
#include "web_civetweb.h"

/*
** {===========================================================================
** Macros and constants
*/

#if !GBBASIC_WEB_ENABLED
#	pragma message("Web disabled.")
#endif /* GBBASIC_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Fetch
*/

#if GBBASIC_WEB_ENABLED

Fetch* Fetch::create(void) {
#if defined GBBASIC_OS_HTML
	FetchHtml* result = new FetchHtml();

	return result;
#else /* GBBASIC_OS_HTML */
	return nullptr;
#endif /* GBBASIC_OS_HTML */
}

void Fetch::destroy(Fetch* ptr) {
#if defined GBBASIC_OS_HTML
	FetchHtml* impl = static_cast<FetchHtml*>(ptr);
	delete impl;
#else /* GBBASIC_OS_HTML */
	(void)ptr;
#endif /* GBBASIC_OS_HTML */
}

#endif /* GBBASIC_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Web
*/

#if GBBASIC_WEB_ENABLED

Web* Web::create(void) {
#if defined GBBASIC_OS_HTML
	WebHtml* result = new WebHtml();

	return result;
#else /* GBBASIC_OS_HTML */
	WebCivetWeb* result = new WebCivetWeb();

	return result;
#endif /* GBBASIC_OS_HTML */
}

void Web::destroy(Web* ptr) {
#if defined GBBASIC_OS_HTML
	WebHtml* impl = static_cast<WebHtml*>(ptr);
	delete impl;
#else /* GBBASIC_OS_HTML */
	WebCivetWeb* impl = static_cast<WebCivetWeb*>(ptr);
	delete impl;
#endif /* GBBASIC_OS_HTML */
}

#endif /* GBBASIC_WEB_ENABLED */

/* ===========================================================================} */
