/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __OPERATIONS_H__
#define __OPERATIONS_H__

#include "../gbbasic.h"
#include "workspace.h"
#include "../../lib/promise_cpp/include/promise-cpp/promise.hpp"

/*
** {===========================================================================
** Operations
*/

/**
 * @brief Asynchronized and synchronized operations for workspace.
 */
class Operations {
public:
	static promise::Promise always(Window* wnd, Renderer* rnd, Workspace* ws, bool ret);
	static promise::Promise always(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise never(Window* wnd, Renderer* rnd, Workspace* ws);

	static promise::Promise popupMessage(Window* wnd, Renderer* rnd, Workspace* ws, const char* content, bool deniable = false, bool cancelable = false);
	static promise::Promise popupMessage(Window* wnd, Renderer* rnd, Workspace* ws, const char* content, const char* confirmTxt, const char* denyTxt, const char* cancelTxt);
	static promise::Promise popupMessageExclusive(Window* wnd, Renderer* rnd, Workspace* ws, const char* content, bool deniable = false, bool cancelable = false);
	static promise::Promise popupMessageWithOption(Window* wnd, Renderer* rnd, Workspace* ws, const char* content, const char* optionBoolTxt, bool optionBoolValue, bool optionBoolValueReadonly = false, bool deniable = false, bool cancelable = false);
	static promise::Promise popupInput(Window* wnd, Renderer* rnd, Workspace* ws, const char* content, const char* default_ = nullptr, unsigned flags = 0);
	static promise::Promise popupWait(Window* wnd, Renderer* rnd, Workspace* ws, const char* content);
	static promise::Promise popupWait(Window* wnd, Renderer* rnd, Workspace* ws, bool dim, const char* content, bool instantly, bool exclusive = false);
	static promise::Promise popupWait(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise popupStarterKits(Window* wnd, Renderer* rnd, Workspace* ws, const char* template_, const char* content, const char* default_ = nullptr, unsigned flags = 0);
	static promise::Promise popupSortAssets(Window* wnd, Renderer* rnd, Workspace* ws, AssetsBundle::Categories category);
	static promise::Promise popupExternalFontResolver(Window* wnd, Renderer* rnd, Workspace* ws, const char* content);
	static promise::Promise popupExternalMapResolver(Window* wnd, Renderer* rnd, Workspace* ws, const char* content);
	static promise::Promise popupExternalSceneResolver(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise popupExternalFileResolver(Window* wnd, Renderer* rnd, Workspace* ws, const char* content, const Text::Array &filter);
	static promise::Promise popupRomBuildSettings(Window* wnd, Renderer* rnd, Workspace* ws, Exporter::Ptr ex);
	static promise::Promise popupEmulatorBuildSettings(Window* wnd, Renderer* rnd, Workspace* ws, Exporter::Ptr ex, const char* settings, const char* args, bool hasIcon);

	static promise::Promise fileNew(Window* wnd, Renderer* rnd, Workspace* ws, const char* fontConfigPath);
	static promise::Promise fileOpen(Window* wnd, Renderer* rnd, Workspace* ws, Project::Ptr &prj, bool allowBin, const char* fontConfigPath);
	static promise::Promise fileClose(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise fileAskSave(Window* wnd, Renderer* rnd, Workspace* ws, bool deniable);
	static promise::Promise fileSave(Window* wnd, Renderer* rnd, Workspace* ws, bool saveAs);                                              // Shows save file dialog if necessary.
	static promise::Promise fileSaveForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, bool saveAs);                                    // Shows save file dialog if necessary.
	static promise::Promise fileRename(Window* wnd, Renderer* rnd, Workspace* ws, Project::Ptr &prj, const char* fontConfigPath);
	static promise::Promise fileRemove(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj);
	static promise::Promise fileRemoveReference(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj);
	static promise::Promise fileClear(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise fileAdd(Window* wnd, Renderer* rnd, Workspace* ws, const char* path);
	static promise::Promise fileDragAndDropFile(Window* wnd, Renderer* rnd, Workspace* ws, const char* path);
	static promise::Promise fileDragAndDropFileForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, const char* path);
	static promise::Promise fileImport(Window* wnd, Renderer* rnd, Workspace* ws);                                                         // Shows open file dialog if necessary.
	static promise::Promise fileImportStringForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, const Text::Ptr &content);
	static promise::Promise fileImportForNotepad(Window* wnd, Renderer* rnd, Workspace* ws);                                               // Shows open file dialog if necessary.
	static promise::Promise fileImportExamples(Window* wnd, Renderer* rnd, Workspace* ws, const char* path /* nullable */);                // Shows open file dialog if necessary.
	static promise::Promise fileImportExampleForNotepad(Window* wnd, Renderer* rnd, Workspace* ws);                                        // Shows open file dialog if necessary.
	static promise::Promise fileExportForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj);                      // Shows save file dialog if necessary.
	static promise::Promise fileDuplicate(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj, const char* fontConfigPath); // Shows save file dialog if necessary.
	static promise::Promise fileBrowseExamples(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise fileBrowse(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj);
	static promise::Promise fileSaveStreamed(Window* wnd, Renderer* rnd, Workspace* ws, const Bytes::Ptr &bytes);

	static promise::Promise editSortAssets(Window* wnd, Renderer* rnd, Workspace* ws, AssetsBundle::Categories category);
	static promise::Promise fontAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise fontDuplicatePage(Window* wnd, Renderer* rnd, Workspace* ws, int index);
	static promise::Promise fontRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise codeAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise codeRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise tilesAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise tilesRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise mapAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise mapRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise musicAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise musicRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise sfxAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise sfxRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise actorAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise actorRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise sceneAddPage(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise sceneRemovePage(Window* wnd, Renderer* rnd, Workspace* ws);

	static promise::Promise projectCompile(Window* wnd, Renderer* rnd, Workspace* ws, const char* cartType /* nullable */, const char* sramType /* nullable */, bool* hasRtc /* nullable */, const char* fontConfigPath, bool useInRam);
	static promise::Promise projectDump(Window* wnd, Renderer* rnd, Workspace* ws, Bytes::Ptr rom, const char* path);
	static promise::Promise projectBuild(Window* wnd, Renderer* rnd, Workspace* ws, Bytes::Ptr rom, Exporter::Ptr ex);                     // Shows save file dialog if necessary.
	static promise::Promise projectRun(Window* wnd, Renderer* rnd, Workspace* ws, Bytes::Ptr rom, Bytes::Ptr sram, bool traceless);
	static promise::Promise projectStop(Window* wnd, Renderer* rnd, Workspace* ws);
	static promise::Promise projectReload(Window* wnd, Renderer* rnd, Workspace* ws, Project::Ptr &prj, const char* fontConfigPath);
	static promise::Promise projectLoadSram(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj, Bytes::Ptr sram);
	static promise::Promise projectSaveSram(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj, const Bytes::Ptr sram, bool immediate);
	static promise::Promise projectRemoveSram(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj, bool toTrashBin);
};

/* ===========================================================================} */

#endif /* __OPERATIONS_H__ */
