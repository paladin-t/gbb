/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_CODE_LANGUAGE_DEFINITION_H__
#define __EDITOR_CODE_LANGUAGE_DEFINITION_H__

#include "../../lib/imgui_code_editor/imgui_code_editor.h"

/*
** {===========================================================================
** Code language definition
*/

class EditorCodeLanguageDefinition {
public:
	static ImGui::CodeEditor::LanguageDefinition languageDefinition(const char* kernelConfigPath);
};

/* ===========================================================================} */

#endif /* __EDITOR_CODE_LANGUAGE_DEFINITION_H__ */
