/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "device.h"
#include "editor_actor.h"
#include "editor_code.h"
#include "editor_font.h"
#include "editor_map.h"
#include "editor_music.h"
#include "editor_scene.h"
#include "editor_sfx.h"
#include "editor_tiles.h"
#include "operations.h"
#include "theme.h"
#include "resource/inline_resource.h"
#include "../compiler/compiler.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#if defined GBBASIC_OS_HTML
#	include <emscripten.h>
#endif /* GBBASIC_OS_HTML */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef OPERATIONS_GBBASIC_TIME_STAT_ENABLED
#	define OPERATIONS_GBBASIC_TIME_STAT_ENABLED
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

static void operationsHandlePrint(Workspace* ws, const char* msg) {
	ws->print(msg);
}
static void operationsHandleWarningOrError(Workspace* ws, const char* msg, bool isWarning) {
	if (isWarning)
		ws->warn(msg);
	else
		ws->error(msg);
}

/* ===========================================================================} */

/*
** {===========================================================================
** Operations
*/

promise::Promise Operations::always(Window*, Renderer*, Workspace*, bool ret) {
	return promise::newPromise(
		[ret] (promise::Defer df) -> void {
			df.resolve(ret);
		}
	);
}

promise::Promise Operations::always(Window*, Renderer*, Workspace*) {
	return promise::newPromise(
		[] (promise::Defer df) -> void {
			df.resolve();
		}
	);
}

promise::Promise Operations::never(Window*, Renderer*, Workspace*) {
	return promise::newPromise(
		[] (promise::Defer df) -> void {
			df.reject();
		}
	);
}

promise::Promise Operations::popupMessage(Window*, Renderer*, Workspace* ws, const char* content, bool deniable, bool cancelable) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::MessagePopupBox::ConfirmedHandler confirm = nullptr;
			ImGui::MessagePopupBox::DeniedHandler deny = nullptr;
			ImGui::MessagePopupBox::CanceledHandler cancel = nullptr;

			confirm = ImGui::MessagePopupBox::ConfirmedHandler(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			if (deniable) {
				deny = ImGui::MessagePopupBox::DeniedHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.resolve(false);
					},
					nullptr
				);
			}
			if (cancelable) {
				cancel = ImGui::MessagePopupBox::CanceledHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					},
					nullptr
				);
			}
			ws->messagePopupBox(
				content,
				confirm,
				deny,
				cancel
			);
		}
	);
}

promise::Promise Operations::popupMessage(Window*, Renderer*, Workspace* ws, const char* content, const char* confirmTxt, const char* denyTxt, const char* cancelTxt) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::MessagePopupBox::ConfirmedHandler confirm = nullptr;
			ImGui::MessagePopupBox::DeniedHandler deny = nullptr;
			ImGui::MessagePopupBox::CanceledHandler cancel = nullptr;

			confirm = ImGui::MessagePopupBox::ConfirmedHandler(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			if (denyTxt) {
				deny = ImGui::MessagePopupBox::DeniedHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.resolve(false);
					},
					nullptr
				);
			}
			if (cancelTxt) {
				cancel = ImGui::MessagePopupBox::CanceledHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					},
					nullptr
				);
			}
			ws->messagePopupBox(
				content,
				confirm,
				deny,
				cancel,
				confirmTxt,
				denyTxt,
				cancelTxt
			);
		}
	);
}

promise::Promise Operations::popupMessageExclusive(Window*, Renderer*, Workspace* ws, const char* content, bool deniable, bool cancelable) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::MessagePopupBox::ConfirmedHandler confirm = nullptr;
			ImGui::MessagePopupBox::DeniedHandler deny = nullptr;
			ImGui::MessagePopupBox::CanceledHandler cancel = nullptr;

			confirm = ImGui::MessagePopupBox::ConfirmedHandler(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			if (deniable) {
				deny = ImGui::MessagePopupBox::DeniedHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.resolve(false);
					},
					nullptr
				);
			}
			if (cancelable) {
				cancel = ImGui::MessagePopupBox::CanceledHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					},
					nullptr
				);
			}
			ws->messagePopupBox(
				content,
				confirm,
				deny,
				cancel,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				true
			);
		}
	);
}

promise::Promise Operations::popupMessageWithOption(Window*, Renderer*, Workspace* ws, const char* content, const char* optionBoolTxt, bool optionBoolValue, bool optionBoolValueReadonly, bool deniable, bool cancelable) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::MessagePopupBox::ConfirmedWithOptionHandler confirm = nullptr;
			ImGui::MessagePopupBox::DeniedHandler deny = nullptr;
			ImGui::MessagePopupBox::CanceledHandler cancel = nullptr;

			confirm = ImGui::MessagePopupBox::ConfirmedWithOptionHandler(
				[ws, df] (bool arg) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true, arg);
				},
				nullptr
			);
			if (deniable) {
				deny = ImGui::MessagePopupBox::DeniedHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.resolve(false, false);
					},
					nullptr
				);
			}
			if (cancelable) {
				cancel = ImGui::MessagePopupBox::CanceledHandler(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					},
					nullptr
				);
			}
			ws->messagePopupBoxWithOption(
				content,
				confirm,
				deny,
				cancel,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				optionBoolTxt, optionBoolValue, optionBoolValueReadonly
			);
		}
	);
}

promise::Promise Operations::popupInput(Window*, Renderer*, Workspace* ws, const char* content, const char* default_, unsigned flags) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::InputPopupBox::ConfirmedHandler confirm(
				[ws, df] (const char* name) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(name);
				},
				nullptr
			);
			ImGui::InputPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->inputPopupBox(
				content ? content : ws->theme()->dialogInput_Input().c_str(),
				default_ ? default_ : "", flags,
				confirm,
				cancel
			);
		}
	);
}

promise::Promise Operations::popupWait(Window*, Renderer*, Workspace* ws, const char* content) {
	const ImGui::PopupBox::Ptr reserved = ws->popupBox();

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::WaitingPopupBox::TimeoutHandler timeout(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			ws->waitingPopupBox(
				content,
				timeout
			);
		}
	)
		.then(
			[reserved] (void) -> void {
				// Do nothing.
			}
		);
}

promise::Promise Operations::popupWait(Window*, Renderer*, Workspace* ws, bool dim, const char* content, bool instantly, bool exclusive) {
	const ImGui::PopupBox::Ptr reserved = ws->popupBox();

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::WaitingPopupBox::TimeoutHandler timeout(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			ws->waitingPopupBox(
				dim, content,
				instantly, timeout,
				exclusive
			);
		}
	)
		.then(
			[reserved] (void) -> void {
				// Do nothing.
			}
		);
}

promise::Promise Operations::popupWait(Window*, Renderer*, Workspace* ws) {
	const ImGui::PopupBox::Ptr reserved = ws->popupBox();

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::WaitingPopupBox::TimeoutHandler timeout(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			ws->waitingPopupBox(
				false, "",
				true, timeout,
				false
			);
		}
	)
		.then(
			[reserved] (void) -> void {
				// Do nothing.
			}
		);
}

promise::Promise Operations::popupProjectCreating(Window*, Renderer*, Workspace* ws, const char* template_, const char* content, const char* default_, unsigned flags) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::ProjectCreatingPopupBox::ConfirmedHandler confirm(
				[ws, df] (int idx, const char* name) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(idx, name);
				},
				nullptr
			);
			ImGui::ProjectCreatingPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->projectCreatingPopupBox(
				template_ ? template_ : ws->theme()->dialogStarterKits_StarterKit().c_str(),
				content ? content : ws->theme()->dialogStarterKits_ProjectName().c_str(),
				default_ ? default_ : "", flags,
				confirm,
				cancel
			);
		}
	);
}

promise::Promise Operations::popupAssetsSorting(Window*, Renderer* rnd, Workspace* ws, AssetsBundle::Categories category) {
	typedef std::map<int, int> Changing;

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::AssetsSortingPopupBox::ConfirmedHandler confirm(
				[ws, df] (ImGui::AssetsSortingPopupBox::Orders orders) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					Project::Ptr &prj = ws->currentProject();
					if (!prj) {
						df.reject();

						return;
					}

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)AssetsBundle::Categories::FONT];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						for (Changing::iterator it = changing.begin(); it != changing.end(); ++it) {
							const int from = it->first;
							const int to = it->second;
							FontAssets::Entry* entry = prj->getFont(from);
							Editable* editor = entry->editor;
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)to);
						}

						FontAssets::Array fonts = prj->assets()->fonts.entries;
						prj->assets()->fonts.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->fonts.entries.push_back(fonts[idx]);
						}
						ws->clearFontPageNames();
					} while (false);

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)Workspace::Categories::CODE];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						for (Changing::iterator it = changing.begin(); it != changing.end(); ++it) {
							const int from = it->first;
							const int to = it->second;
							CodeAssets::Entry* entry = prj->getCode(from);
							Editable* editor = entry->editor;
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)to);
						}

						CodeAssets::Array code = prj->assets()->code.entries;
						prj->assets()->code.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->code.entries.push_back(code[idx]);
						}

						ws->needAnalyzing(true);
					} while (false);

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)AssetsBundle::Categories::TILES];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						for (Changing::iterator it = changing.begin(); it != changing.end(); ++it) {
							const int from = it->first;
							const int to = it->second;
							TilesAssets::Entry* entry = prj->getTiles(from);
							Editable* editor = entry->editor;
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)to);
						}

						TilesAssets::Array tiles = prj->assets()->tiles.entries;
						prj->assets()->tiles.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->tiles.entries.push_back(tiles[idx]);
						}
						ws->clearTilesPageNames();

						for (int i = 0; i < prj->mapPageCount(); ++i) {
							MapAssets::Entry* entry = prj->getMap(i);
							Changing::iterator it = changing.find(entry->ref);
							if (it != changing.end()) {
								entry->ref = it->second;
								entry->cleanup();
								Editable* editor = entry->editor;
								if (editor) {
									editor->post(Editable::UPDATE_REF_INDEX, ws, (Variant::Int)entry->ref);
									editor->post(Editable::CLEAR_UNDO_REDO_RECORDS);
								}
							}
						}
					} while (false);

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)AssetsBundle::Categories::MAP];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						for (Changing::iterator it = changing.begin(); it != changing.end(); ++it) {
							const int from = it->first;
							const int to = it->second;
							MapAssets::Entry* entry = prj->getMap(from);
							Editable* editor = entry->editor;
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)to);
						}

						MapAssets::Array maps = prj->assets()->maps.entries;
						prj->assets()->maps.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->maps.entries.push_back(maps[idx]);
						}
						ws->clearMapPageNames();

						for (int i = 0; i < prj->scenePageCount(); ++i) {
							SceneAssets::Entry* entry = prj->getScene(i);
							Changing::iterator it = changing.find(entry->refMap);
							if (it != changing.end()) {
								entry->refMap = it->second;
								entry->cleanup();
								Editable* editor = entry->editor;
								if (editor) {
									editor->post(Editable::UPDATE_REF_INDEX, ws, (Variant::Int)entry->refMap);
									editor->post(Editable::CLEAR_UNDO_REDO_RECORDS);
								}
							}
						}
					} while (false);

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)AssetsBundle::Categories::MUSIC];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						for (Changing::iterator it = changing.begin(); it != changing.end(); ++it) {
							const int from = it->first;
							const int to = it->second;
							MusicAssets::Entry* entry = prj->getMusic(from);
							Editable* editor = entry->editor;
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)to);
						}

						MusicAssets::Array music = prj->assets()->music.entries;
						prj->assets()->music.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->music.entries.push_back(music[idx]);
						}
						ws->clearMusicPageNames();
					} while (false);

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)AssetsBundle::Categories::SFX];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						Changing::iterator it = changing.find(prj->activeSfxIndex());
						if (it != changing.end()) {
							EditorSfx* editor = prj->sharedSfxEditor();
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)it->second);
						}

						SfxAssets::Array sfx = prj->assets()->sfx.entries;
						prj->assets()->sfx.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->sfx.entries.push_back(sfx[idx]);
						}
						ws->clearSfxPageNames();

						for (int i = 0; i < prj->sfxPageCount(); ++i) {
							SfxAssets::Entry* entry = prj->getSfx(i);
							Editable* editor = entry->editor;
							if (editor) {
								editor->post(Editable::CLEAR_UNDO_REDO_RECORDS);
							}
						}
					} while (false);

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)AssetsBundle::Categories::ACTOR];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						for (Changing::iterator it = changing.begin(); it != changing.end(); ++it) {
							const int from = it->first;
							const int to = it->second;
							ActorAssets::Entry* entry = prj->getActor(from);
							Editable* editor = entry->editor;
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)to);
						}

						ActorAssets::Array actors = prj->assets()->actors.entries;
						prj->assets()->actors.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->actors.entries.push_back(actors[idx]);
						}
						ws->clearActorPageNames();

						for (int i = 0; i < prj->scenePageCount(); ++i) {
							SceneAssets::Entry* entry = prj->getScene(i);
							int changed = 0;
							Map::Ptr layer = entry->data->actorLayer();
							const int w = layer->width();
							const int h = layer->height();
							for (int j = 0; j < h; ++j) {
								for (int i_ = 0; i_ < w; ++i_) {
									const UInt8 cel = (UInt8)layer->get(i_, j);
									if (cel == Scene::INVALID_ACTOR())
										continue;

									Changing::iterator it = changing.find(cel);
									if (it != changing.end()) {
										layer->set(i_, j, it->second);
										++changed;
									}
								}
							}
							if (changed > 0) {
								entry->cleanup();
								Editable* editor = entry->editor;
								if (editor) {
									editor->post(Editable::UPDATE_REF_INDEX, ws, (Variant::Int)entry->refMap);
									editor->post(Editable::CLEAR_UNDO_REDO_RECORDS);
								}
							}
						}
					} while (false);

					do {
						Changing changing;
						const ImGui::AssetsSortingPopupBox::Order &order = orders[(unsigned)AssetsBundle::Categories::SCENE];
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							if (idx != i)
								changing[idx] = i;
						}

						for (Changing::iterator it = changing.begin(); it != changing.end(); ++it) {
							const int from = it->first;
							const int to = it->second;
							SceneAssets::Entry* entry = prj->getScene(from);
							Editable* editor = entry->editor;
							if (editor)
								editor->post(Editable::UPDATE_INDEX, (Variant::Int)to);
						}

						SceneAssets::Array scenes = prj->assets()->scenes.entries;
						prj->assets()->scenes.entries.clear();
						for (int i = 0; i < (int)order.size(); ++i) {
							const int idx = order[i];
							prj->assets()->scenes.entries.push_back(scenes[idx]);
						}
						ws->clearScenePageNames();
					} while (false);

					prj->hasDirtyAsset(true);

					df.resolve(true);
				},
				nullptr
			);
			ImGui::AssetsSortingPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->assetsSortingPopupBox(
				rnd,
				category,
				confirm,
				cancel
			);
		}
	);
}

promise::Promise Operations::popupExternalFontResolver(Window*, Renderer* rnd, Workspace* ws, const char* content) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			const Text::Array filter = GBBASIC_FONT_FILE_FILTER;
			ImGui::FontResolverPopupBox::ConfirmedHandler confirm(
				[ws, df] (const char* path, const Math::Vec2i* size) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(path, size, false, true);
				},
				nullptr
			);
			ImGui::FontResolverPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->showExternalFontBrowser(
				rnd,
				content,
				filter,
				confirm,
				cancel,
				nullptr,
				nullptr
			);
		}
	);
}

promise::Promise Operations::popupExternalMapResolver(Window*, Renderer* rnd, Workspace* ws, const char* content) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			const Text::Array filter = GBBASIC_IMAGE_FILE_FILTER;
			ImGui::MapResolverPopupBox::ConfirmedHandler confirm(
				[ws, df] (const int* index, const char* path, bool allowFlip) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(index, path, allowFlip);
				},
				nullptr
			);
			ImGui::MapResolverPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->showExternalMapBrowser(
				rnd,
				content,
				filter,
				true, true,
				confirm,
				cancel,
				nullptr,
				nullptr
			);
		}
	);
}

promise::Promise Operations::popupExternalSceneResolver(Window*, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			const Text::Array filter = GBBASIC_IMAGE_FILE_FILTER;
			ImGui::SceneResolverPopupBox::ConfirmedHandler confirm(
				[ws, df] (int index, bool useGravity) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(index, useGravity);
				},
				nullptr
			);
			ImGui::SceneResolverPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->showExternalSceneBrowser(
				rnd,
				confirm,
				cancel,
				nullptr,
				nullptr
			);
		}
	);
}

promise::Promise Operations::popupExternalFileResolver(Window*, Renderer* rnd, Workspace* ws, const char* content, const Text::Array &filter) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::FileResolverPopupBox::ConfirmedHandler_Path confirm(
				[ws, df] (const char* path) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(path, false, true);
				},
				nullptr
			);
			ImGui::FileResolverPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->showExternalFileBrowser(
				rnd,
				content,
				filter,
				true,
				confirm,
				cancel,
				nullptr,
				nullptr
			);
		}
	);
}

promise::Promise Operations::popupRomBuildSettings(Window*, Renderer* rnd, Workspace* ws, Exporter::Ptr ex) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (ex->packageArchived()) {
				df.resolve(false, nullptr, nullptr, false);

				return;
			}

			ImGui::RomBuildSettingsPopupBox::ConfirmedHandler confirm(
				[ws, df] (const char* cartType, const char* sramType, bool hasRtc) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.resolve(true, cartType, sramType, hasRtc);
				},
				nullptr
			);
			ImGui::RomBuildSettingsPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->showRomBuildSettings(
				rnd,
				confirm,
				cancel
			);
		}
	);
}

promise::Promise Operations::popupEmulatorBuildSettings(Window*, Renderer* rnd, Workspace* ws, Exporter::Ptr ex, const char* settings, const char* args, bool hasIcon) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (!ex->packageArchived()) {
				df.resolve(false);

				return;
			}

			ImGui::EmulatorBuildSettingsPopupBox::ConfirmedHandler confirm(
				[ws, df] (const char* settings, const char* args, Bytes::Ptr icon) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					ws->settings().exporterSettings = settings;
					ws->settings().exporterArgs = args;
					ws->settings().exporterIcon = icon;

					df.resolve(true);
				},
				nullptr
			);
			ImGui::EmulatorBuildSettingsPopupBox::CanceledHandler cancel(
				[ws, df] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->showEmulatorBuildSettings(
				rnd,
				settings ? settings : "", args, hasIcon,
				confirm,
				cancel
			);
		}
	);
}

promise::Promise Operations::fileNew(Window* wnd, Renderer* rnd, Workspace* ws, const char* fontConfigPath) {
	auto next = [wnd, rnd, ws, fontConfigPath] (promise::Defer df, int idx, std::string name) -> void {
		Project::Ptr templatePrj = nullptr;
		do {
			if (idx < 0)
				break;

			const EntryWithPath::Array &starterKits = ws->starterKits();
			const EntryWithPath &entry = starterKits[idx];
			templatePrj = Project::Ptr(new Project(wnd, rnd, ws));
			if (!templatePrj->open(entry.path().c_str())) {
				templatePrj = nullptr;

				break;
			}

			Texture::Ptr attribtex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
			Texture::Ptr propstex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
			Texture::Ptr actorstex(ws->theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
			templatePrj->attributesTexture(attribtex);
			templatePrj->propertiesTexture(propstex);
			templatePrj->actorsTexture(actorstex);

			templatePrj->behaviourSerializer(std::bind(&Workspace::serializeKernelBehaviour, ws, std::placeholders::_1));
			templatePrj->behaviourParser(std::bind(&Workspace::parseKernelBehaviour, ws, std::placeholders::_1));

			if (!templatePrj->load(false, fontConfigPath, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
				templatePrj->close(true);
				templatePrj = nullptr;

				break;
			}
			templatePrj->path().clear(); // Clear the path.
		} while (false);

		const long long now = DateTime::now();
		Project::Ptr prj(new Project(wnd, rnd, ws));
		if (templatePrj) {
			*prj = *templatePrj;
			templatePrj->unload();
			templatePrj->close(true);
			templatePrj = nullptr;

			prj->title(name);
			prj->created(now);
			prj->modified(now);
			prj->order(PROJECT_DEFAULT_ORDER);
		} else {
			prj->title(name);
			Bytes::Ptr bytes(Bytes::create());
			bytes->writeBytes((Byte*)RES_ICON_PROJECT_DEFAULT, sizeof(RES_ICON_PROJECT_DEFAULT));
			std::string iconCode;
			if (Base64::fromBytes(iconCode, bytes.get()))
				prj->iconCode(iconCode);
			prj->cartridgeType(PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED);
			prj->sramType(PROJECT_SRAM_TYPE_32KB);
			prj->hasRtc(false);
			prj->caseInsensitive(true);
			prj->strictOn(true);
			prj->description("");
			prj->author("");
			prj->genre("");
			prj->version("1.0.0");
			prj->url("");
			prj->created(now);
			prj->modified(now);
			prj->assets(AssetsBundle::Ptr(new AssetsBundle()));

			prj->addCodePage("");

			prj->addGlobalFontPages(fontConfigPath, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2));
		}
		{
			const PaletteAssets &paletteData = prj->touchPalette();

			prj->activeFontIndex(0);

			prj->activeMajorCodeIndex(0);
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			prj->isMinorCodeEditorEnabled(false);
			prj->activeMinorCodeIndex(-1);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

			prj->synchronizeSuperPalettes(paletteData);
		}
		prj->hasDirtyInformation(true);
		prj->hasDirtyAsset(true);

		if (ws->showRecentProjects()) {
			ws->projects().push_back(prj);

			ws->sortProjects();

			const Project::Array &projects = ws->projects();
			for (int i = 0; i < (int)projects.size(); ++i) {
				const Project::Ptr &prj_ = projects[i];
				if (prj_.get() == prj.get()) {
					ws->recentProjectSelectedIndex(i);

					break;
				}
			}
		}

		++ws->activities().created;
		switch (prj->contentType()) {
		case Project::ContentTypes::BASIC: ++ws->activities().createdBasic; break;
		default: GBBASIC_ASSERT(false && "Impossible.");                    break;
		}

		ws->bubble(ws->theme()->dialogPrompt_CreatedProject(), nullptr);

		df.resolve(prj);
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (bool /* ok */) -> promise::Promise {
						return popupProjectCreating(wnd, rnd, ws, ws->theme()->dialogStarterKits_StarterKit().c_str(), ws->theme()->dialogStarterKits_ProjectName().c_str(), GBBASIC_NONAME_PROJECT_NAME, ImGuiInputTextFlags_None)
							.then(
								[ws, next, df] (int idx, const char* name) -> promise::Promise {
									WORKSPACE_AUTO_CLOSE_POPUP(ws)

									const std::string name_ = name;
									return promise::newPromise(std::bind(next, std::placeholders::_1, idx, name_))
										.then(
											[df] (Project::Ptr prj) -> void {
												df.resolve(prj);
											}
										)
										.fail(
											[df] (void) -> void {
												df.reject();
											}
										);
								}
							)
							.fail(
								[ws, df] (void) -> void {
									WORKSPACE_AUTO_CLOSE_POPUP(ws)

									df.reject();
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileOpen(Window* wnd, Renderer* rnd, Workspace* ws, Project::Ptr &prj, bool allowBin, const char* fontConfigPath) {
	return promise::newPromise(
		[&, allowBin] (promise::Defer df) -> void {
			const std::string fontConfigPath_ = fontConfigPath;

			auto next = [wnd, rnd, ws, prj, allowBin, fontConfigPath_] (promise::Defer df) -> void {
#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

				Texture::Ptr attribtex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
				Texture::Ptr propstex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
				Texture::Ptr actorstex(ws->theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
				prj->attributesTexture(attribtex);
				prj->propertiesTexture(propstex);
				prj->actorsTexture(actorstex);

				prj->behaviourSerializer(std::bind(&Workspace::serializeKernelBehaviour, ws, std::placeholders::_1));
				prj->behaviourParser(std::bind(&Workspace::parseKernelBehaviour, ws, std::placeholders::_1));

				ws->clear();

				ws->print(GBBASIC_TITLE " v" GBBASIC_VERSION_STRING);
				ws->print("");
				ws->print("Ready.");

				if (!prj->loaded() && !prj->load(allowBin, fontConfigPath_.c_str(), std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

					prj->attributesTexture(nullptr);
					prj->propertiesTexture(nullptr);
					prj->actorsTexture(nullptr);

					prj->behaviourSerializer(nullptr);
					prj->behaviourParser(nullptr);

					ws->clear();

					return;
				}

				ws->currentProject(prj);
				if (prj->contentType() == Project::ContentTypes::BASIC) {
					ws->category(Workspace::Categories::CODE);
				} else {
					ws->category(Workspace::Categories::CONSOLE);
					ws->categoryBeforeCompiling(Workspace::Categories::CONSOLE);
				}
				ws->tabsWidth(0.0f);

				ws->refreshWindowTitle(wnd);

				df.resolve(true);

				FileMonitor::unuse(prj->path());

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long end = DateTime::ticks();
				const long long diff = end - start;
				const double secs = DateTime::toSeconds(diff);
				fprintf(stdout, "Project opened in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
				ws->codeEditorMinorWidth(prj->minorCodeEditorWidth());
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

				ws->needAnalyzing(true);
			};

			if (ws->showRecentProjects()) {
				popupWait(wnd, rnd, ws, true, ws->theme()->dialogPrompt_Opening().c_str(), true, false)
					.then(
						[next, df] (void) -> promise::Promise {
							return promise::newPromise(std::bind(next, df));
						}
					);
			} else {
				std::bind(next, df)();
			}
		}
	);
}

promise::Promise Operations::fileClose(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (!prj) {
				df.resolve(false);

				return;
			}

			fileAskSave(wnd, rnd, ws, true)
				.then(
					[wnd, rnd, ws, df] (bool ok) -> void {
						Project::Ptr &prj = ws->currentProject();
						if (!prj->close(false)) {
							df.reject();

							return;
						}

						if (!prj->exists()) { // Not saved or doesn't exist on disk.
							int tobeRemoved = -1;
							for (int i = 0; i < (int)ws->projects().size(); ++i) {
								const Project::Ptr &prj_ = ws->projects()[i];
								if (prj_ == prj) { // Find the temporary project to be removed.
									tobeRemoved = i;

									break;
								}
							}
							if (tobeRemoved >= 0) {
								ws->projects().erase(ws->projects().begin() + tobeRemoved); // Remove the temporary project.
							}
						}

						ws->joinCompiling();

						ws->join();

						ws->clearAnalyzingResult();

						prj->attributesTexture(nullptr);
						prj->propertiesTexture(nullptr);
						prj->actorsTexture(nullptr);

						prj->behaviourSerializer(nullptr);
						prj->behaviourParser(nullptr);

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
						ws->destroyMinorCodeEditor();
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
						ws->codeEditorMinorWidth(0.0f);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

						ws->closeSearchResult();
						ws->currentProject(nullptr);
						ws->popupBox(nullptr);
						ws->category(Workspace::Categories::HOME);
						ws->categoryOfAudio(Workspace::Categories::MUSIC);
						ws->categoryBeforeCompiling(Workspace::Categories::HOME);
						ws->tabsWidth(0.0f);
						ws->consoleHasWarning(false);
						ws->consoleHasError(false);

						ws->clear();

						if (ws->showRecentProjects()) {
							ws->sortProjects();
						}

						ws->refreshWindowTitle(wnd);

						FileMonitor::dump();

						df.resolve(ok);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileAskSave(Window* wnd, Renderer* rnd, Workspace* ws, bool deniable) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (!prj) {
				df.resolve(false);

				return;
			}
			if (!prj->dirty()) {
				df.resolve(true);

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_SaveTheCurrentProject().c_str(), deniable, true)
				.then(
					[wnd, rnd, ws, df] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (ws->showRecentProjects()) {
								fileSave(wnd, rnd, ws, false)
									.then(
										[df] (bool ok) -> void {
											df.resolve(ok);
										}
									)
									.fail(
										[df] (void) -> void {
											df.reject();
										}
									);
							} else {
								fileSaveForNotepad(wnd, rnd, ws, false)
									.then(
										[df] (bool ok) -> void {
											df.resolve(ok);
										}
									)
									.fail(
										[df] (void) -> void {
											df.reject();
										}
									);
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileSave(Window* wnd, Renderer* rnd, Workspace* ws, bool saveAs) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (!prj->dirty() && !saveAs) {
				df.resolve(false);

				return;
			}

			const std::string &title = prj->title();
			std::string path;
			if (!saveAs)
				path = prj->path();
			if (path.empty()) {
				pfd::save_file save(
					ws->theme()->generic_SaveTo(),
					Text::sanitizeFilename(title) + "." GBBASIC_RICH_PROJECT_EXT,
					GBBASIC_PROJECT_FILE_FILTER
				);
				path = save.result();
				Path::uniform(path);
				if (path.empty()) {
					df.reject();

					return;
				}
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || ext != GBBASIC_RICH_PROJECT_EXT) {
					path += "." GBBASIC_RICH_PROJECT_EXT;
				}
			}
			if (!ws->canWriteProjectTo(path.c_str())) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations().c_str());

				return;
			}

			auto next = [wnd, rnd, ws, path] (promise::Defer df) -> void {
#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

				Project::Ptr &prj = ws->currentProject();
				if (!prj->save(path.c_str(), true, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
					df.reject();

					popupMessageExclusive(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveProjectSeeTheConsoleForDetails().c_str());

					return;
				}

				int tobeRemoved = -1;
				for (int i = 0; i < (int)ws->projects().size(); ++i) {
					const Project::Ptr &prj_ = ws->projects()[i];
					if (prj_->path() == path && prj_ != prj) { // Find if there's any project to be overwrite.
						tobeRemoved = i;

						break;
					}
				}
				if (tobeRemoved >= 0) {
					ws->projects().erase(ws->projects().begin() + tobeRemoved); // Remove the old project.
				}

				df.resolve(true);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long end = DateTime::ticks();
				const long long diff = end - start;
				const double secs = DateTime::toSeconds(diff);
				fprintf(stdout, "Project saved in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
			};

			popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
				.then(
					[next, df] (void) -> promise::Promise {
						return promise::newPromise(std::bind(next, df));
					}
				);
		}
	);
}

promise::Promise Operations::fileSaveForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, bool saveAs) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (!prj->dirty() && !saveAs) {
				df.resolve(false);

				return;
			}

			const std::string &title = prj->title();
			std::string path;
			if (!saveAs)
				path = prj->path();
			if (path.empty()) {
				prj->fileSync()->initialize(title + "." GBBASIC_RICH_PROJECT_EXT, FileSync::Filters::PROJECT);
				if (!prj->fileSync()->requestSave()) {
					df.reject();

					return;
				}
				path = prj->fileSync()->path();
			}
			if (!ws->canWriteProjectTo(path.c_str())) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations().c_str());

				return;
			}

			auto next = [wnd, rnd, ws, path] (promise::Defer df) -> void {
#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

				Project::Ptr &prj = ws->currentProject();
				if (!prj->save(path.c_str(), true, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
					df.reject();

					popupMessageExclusive(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveProjectSeeTheConsoleForDetails().c_str());

					return;
				}

				int tobeRemoved = -1;
				for (int i = 0; i < (int)ws->projects().size(); ++i) {
					const Project::Ptr &prj_ = ws->projects()[i];
					if (prj_->path() == path && prj_ != prj) { // Find if there's any project to be overwrite.
						tobeRemoved = i;

						break;
					}
				}
				if (tobeRemoved >= 0) {
					ws->projects().erase(ws->projects().begin() + tobeRemoved); // Remove the old project.
				}

				df.resolve(true);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long end = DateTime::ticks();
				const long long diff = end - start;
				const double secs = DateTime::toSeconds(diff);
				fprintf(stdout, "Project saved in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

#if defined GBBASIC_OS_HTML
				EM_ASM({
					do {
						if (Module.__GBBASIC_DOWNLOAD_TIPS_SHOWN_TIMES__ == undefined)
							Module.__GBBASIC_DOWNLOAD_TIPS_SHOWN_TIMES__ = 0;

						/*if (Module.__GBBASIC_DOWNLOAD_TIPS_SHOWN_TIMES__ >= 3) // Only prompt 3 times.
							break;*/

						if (typeof isFileSyncDisabled != 'function')
							break;

						if (isFileSyncDisabled())
							break;

						if (typeof showTips == 'function') {
							showTips({
								content: 'Saved in memory, click the download button to preserve your project.'
							});
						}
						++Module.__GBBASIC_DOWNLOAD_TIPS_SHOWN_TIMES__;
					} while (false);
				});
#endif /* Platform macro. */
			};

			popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
				.then(
					[next, df] (void) -> promise::Promise {
						return promise::newPromise(std::bind(next, df));
					}
				);
		}
	);
}

promise::Promise Operations::fileRename(Window* wnd, Renderer* rnd, Workspace* ws, Project::Ptr &prj, const char* fontConfigPath) {
	const std::string title = prj->title();

	return popupInput(wnd, rnd, ws, ws->theme()->dialogInput_ProjectName().c_str(), title.c_str(), ImGuiInputTextFlags_None)
		.then(
			[wnd, rnd, ws, prj, fontConfigPath, title] (const char* title_) -> promise::Promise {
				WORKSPACE_AUTO_CLOSE_POPUP(ws)

				const std::string title__ = title_;

				return promise::newPromise(
					[wnd, rnd, ws, prj, fontConfigPath, title, title__] (promise::Defer df) -> void {
						if (title == title__) {
							df.resolve(true);

							return;
						}

						if (prj->loaded()) {
							df.reject();

							return;
						}
						if (!prj->exists()) {
							df.reject();

							popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_TheProjectIsNotSavedYet().c_str());

							return;
						}
						if (!prj->rename(title__.c_str())) {
							popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_FileNameHasNotBeenChanged().c_str());
						}

						Texture::Ptr attribtex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
						Texture::Ptr propstex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
						Texture::Ptr actorstex(ws->theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
						prj->attributesTexture(attribtex);
						prj->propertiesTexture(propstex);
						prj->actorsTexture(actorstex);

						prj->behaviourSerializer(std::bind(&Workspace::serializeKernelBehaviour, ws, std::placeholders::_1));
						prj->behaviourParser(std::bind(&Workspace::parseKernelBehaviour, ws, std::placeholders::_1));

						if (!prj->load(false, fontConfigPath, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
							df.reject();

							popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

							return;
						}

						prj->attributesTexture(nullptr);
						prj->propertiesTexture(nullptr);
						prj->actorsTexture(nullptr);

						prj->behaviourSerializer(nullptr);
						prj->behaviourParser(nullptr);

						prj->title(title__);
						if (!prj->save(nullptr, false, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
							df.reject();

							popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveProject().c_str());

							return;
						}
						prj->unload();

						ws->sortProjects();

						const Project::Array &projects = ws->projects();
						for (int i = 0; i < (int)projects.size(); ++i) {
							const Project::Ptr &prj_ = projects[i];
							if (prj_.get() == prj.get()) {
								ws->recentProjectSelectedIndex(i);

								break;
							}
						}

						ws->bubble(ws->theme()->dialogPrompt_RenamedProject(), nullptr);

						df.resolve(true);
					}
				);
			}
		);
}

promise::Promise Operations::fileRemove(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			const std::string &path = prj->path();
			if (!Path::fileExists(path.c_str())) {
				for (int i = 0; i < (int)ws->projects().size(); ++i) {
					const Project::Ptr &prj_ = ws->projects()[i];
					if (prj == prj_) {
						ws->projects().erase(ws->projects().begin() + i);
						ws->recentProjectFocusableCount(ws->recentProjectFocusableCount() - 1);
						if (ws->recentProjectSelectedIndex() == i) {
							if (!ws->projects().empty()) {
								ws->recentProjectFocusingIndex(
									Math::clamp(
										ws->recentProjectFocusedIndex(),
										0,
										ws->recentProjectFocusableCount() - 1
									)
								);
							}
							ws->recentProjectSelectedIndex(
								Math::clamp(
									ws->recentProjectSelectedIndex(),
									0,
									ws->recentProjectFocusableCount() - 1
								)
							);
						}

						break;
					}
				}

				ws->bubble(ws->theme()->dialogPrompt_RemovedProject(), nullptr);

				df.resolve(true);

				return;
			}

			const bool writable = ws->canWriteProjectTo(path.c_str());
			popupMessageWithOption(
				wnd, rnd, ws,
				ws->theme()->dialogAsk_RemoveTheCurrentProject().c_str(),
				ws->theme()->dialogAsk_RemoveFromDisk().c_str(), writable && ws->settings().recentRemoveFromDisk, !writable,
				true, false
			)
				.then(
					[wnd, rnd, ws, prj, df, path] (bool ok, bool boolValue) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (!ok) {
							df.reject();

							return;
						}

						projectRemoveSram(wnd, rnd, ws, prj, true);

						if (boolValue && !Path::removeFile(path.c_str(), true)) {
							df.reject();

							popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotRemoveProject().c_str());

							return;
						}

#if defined GBBASIC_DEBUG
						do {
							std::string name;
							std::string dir;
							Path::split(path, &name, nullptr, &dir);
							name += "." GBBASIC_ROM_DUMP_EXT;
							const std::string newPath = Path::combine(dir.c_str(), name.c_str());
							if (Path::fileExists(newPath.c_str()))
								Path::removeFile(newPath.c_str(), true);
						} while (false);
#endif /* GBBASIC_DEBUG */

						for (int i = 0; i < (int)ws->projects().size(); ++i) {
							const Project::Ptr &prj_ = ws->projects()[i];
							if (prj == prj_) {
								ws->projects().erase(ws->projects().begin() + i);
								ws->recentProjectFocusableCount(ws->recentProjectFocusableCount() - 1);
								if (ws->recentProjectSelectedIndex() == i) {
									if (!ws->projects().empty()) {
										ws->recentProjectFocusingIndex(
											Math::clamp(
												ws->recentProjectFocusedIndex(),
												0,
												ws->recentProjectFocusableCount() - 1
											)
										);
									}
									ws->recentProjectSelectedIndex(
										Math::clamp(
											ws->recentProjectSelectedIndex(),
											0,
											ws->recentProjectFocusableCount() - 1
										)
									);
								}

								break;
							}
						}

						ws->bubble(ws->theme()->dialogPrompt_RemovedProject(), nullptr);

						df.resolve(true);
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileRemoveReference(Window*, Renderer*, Workspace* ws, const Project::Ptr &prj) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			for (int i = 0; i < (int)ws->projects().size(); ++i) {
				const Project::Ptr &prj_ = ws->projects()[i];
				if (prj == prj_) {
					ws->projects().erase(ws->projects().begin() + i);
					ws->recentProjectFocusableCount(ws->recentProjectFocusableCount() - 1);
					if (ws->recentProjectSelectedIndex() == i) {
						if (!ws->projects().empty()) {
							ws->recentProjectFocusingIndex(
								Math::clamp(
									ws->recentProjectFocusedIndex(),
									0,
									ws->recentProjectFocusableCount() - 1
								)
							);
						}
						ws->recentProjectSelectedIndex(
							Math::clamp(
								ws->recentProjectSelectedIndex(),
								0,
								ws->recentProjectFocusableCount() - 1
							)
						);
					}

					break;
				}
			}

			df.resolve(true);
		}
	);
}

promise::Promise Operations::fileClear(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_ClearAllRecentProjects().c_str(), true, false)
				.then(
					[ws, df] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							ws->projects().clear();
							ws->projectIndices().clear();

							ws->bubble(ws->theme()->dialogPrompt_ClearedProjects(), nullptr);

							df.resolve(true);

							fprintf(stdout, "Cleared all recent projects.\n");
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileAdd(Window* wnd, Renderer* rnd, Workspace* ws, const char* path) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

			Project::Ptr prj(new Project(wnd, rnd, ws));
			if (!prj->open(path)) {
				// Cannot resolve the title due to failed to open the project.
				prj->title("Missing...");
			}

			ws->projects().push_back(prj);

			df.resolve(prj);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			std::string title = prj->title();
			title = Unicode::toOs(title);

			const long long end = DateTime::ticks();
			const long long diff = end - start;
			const double secs = DateTime::toSeconds(diff);
			fprintf(stdout, "Project %s opened in %gs.\n", title.c_str(), secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
		}
	);
}

promise::Promise Operations::fileDragAndDropFile(Window* wnd, Renderer* rnd, Workspace* ws, const char* path_) {
	const std::string path__ = path_;

	auto next = [wnd, rnd, ws, path__] (promise::Defer df) -> void {
		auto import = [wnd, rnd, ws, df] (const std::string &path) -> bool {
			const std::string abspath = Path::absoluteOf(path);
			for (int i = 0; i < (int)ws->projects().size(); ++i) {
				const Project::Ptr &prj_ = ws->projects()[i];
				const std::string abspath_ = Path::absoluteOf(prj_->path());
				if (abspath == abspath_) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_ProjectAlreadyExists().c_str());

					return false;
				}
			}

			Project::Ptr prj(new Project(wnd, rnd, ws));
			if (!prj->open(path.c_str())) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

				return false;
			}

			if (ws->isUnderExamplesDirectory(path.c_str())) {
				prj->isExample(true);
			}

			ws->projects().push_back(prj);

			ws->sortProjects();

			const Project::Array &projects = ws->projects();
			for (int i = 0; i < (int)projects.size(); ++i) {
				const Project::Ptr &prj_ = projects[i];
				if (prj_.get() == prj.get()) {
					ws->recentProjectSelectedIndex(i);

					break;
				}
			}

			++ws->activities().opened;
			switch (prj->contentType()) {
			case Project::ContentTypes::BASIC: ++ws->activities().openedBasic; break;
			case Project::ContentTypes::ROM:   ++ws->activities().openedRom;   break;
			default: GBBASIC_ASSERT(false && "Impossible.");                   break;
			}

			ws->bubble(ws->theme()->dialogPrompt_ImportedProject(), nullptr);

			df.resolve(prj);

			return true;
		};

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

		std::string path = path__;
		if (path.empty())
			return;
		Path::uniform(path);

		if (!import(path))
			return;

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project imported in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (void) -> promise::Promise {
						return popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Importing().c_str())
							.then(
								[next, df] (void) -> promise::Promise {
									return promise::newPromise(std::bind(next, df));
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileDragAndDropFileForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, const char* path_) {
	const std::string path__ = path_;

	auto next = [wnd, rnd, ws, path__] (promise::Defer df) -> void {
		auto import = [wnd, rnd, ws, df] (const std::string &path) -> bool {
			Project::Ptr prj(new Project(wnd, rnd, ws));
			if (!prj->open(path.c_str())) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

				return false;
			}

			if (ws->isUnderExamplesDirectory(path.c_str())) {
				prj->isExample(true);
			}

			ws->bubble(ws->theme()->dialogPrompt_OpenedProject(), nullptr);

			df.resolve(prj);

			return true;
		};

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

		std::string path = path__;
		if (path.empty())
			return;
		Path::uniform(path);

		if (!import(path))
			return;

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project opened in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (void) -> promise::Promise {
						return promise::newPromise(std::bind(next, df));
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileImport(Window* wnd, Renderer* rnd, Workspace* ws) {
	auto next = [wnd, rnd, ws] (promise::Defer df) -> void {
		pfd::open_file open(
			ws->theme()->generic_Open(),
			"",
			GBBASIC_PROJECT_FILE_FILTER,
			pfd::opt::multiselect
		);
		if (open.result().empty() || open.result().front().empty()) {
			df.reject();

			return;
		}

		auto import = [wnd, rnd, ws, df] (const std::string &path, int total) -> bool {
			const std::string abspath = Path::absoluteOf(path);
			for (int i = 0; i < (int)ws->projects().size(); ++i) {
				const Project::Ptr &prj_ = ws->projects()[i];
				const std::string abspath_ = Path::absoluteOf(prj_->path());
				if (abspath == abspath_) {
					if (total == 1) {
						df.reject();

						popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_ProjectAlreadyExists().c_str());
					} else {
						const std::string msg = Text::format("Project \"{0}\" already exists.", { prj_->title() });
						ws->warn(msg.c_str());
					}

					return false;
				}
			}

			Project::Ptr prj(new Project(wnd, rnd, ws));
			if (!prj->open(path.c_str())) {
				if (total == 1) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());
				} else {
					const std::string msg = Text::format("Cannot import project at \"{0}\".", { path });
					ws->warn(msg.c_str());
				}

				return false;
			}

			if (ws->isUnderExamplesDirectory(path.c_str())) {
				prj->isExample(true);
			}

			ws->projects().push_back(prj);

			ws->sortProjects();

			const Project::Array &projects = ws->projects();
			for (int i = 0; i < (int)projects.size(); ++i) {
				const Project::Ptr &prj_ = projects[i];
				if (prj_.get() == prj.get()) {
					ws->recentProjectSelectedIndex(i);

					break;
				}
			}

			++ws->activities().opened;
			switch (prj->contentType()) {
			case Project::ContentTypes::BASIC: ++ws->activities().openedBasic; break;
			case Project::ContentTypes::ROM:   ++ws->activities().openedRom;   break;
			default: GBBASIC_ASSERT(false && "Impossible.");                   break;
			}

			ws->bubble(ws->theme()->dialogPrompt_ImportedProject(), nullptr);

			if (total == 1)
				df.resolve(prj);

			return true;
		};

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

		if (open.result().size() == 1) {
			std::string path = open.result().front();
			if (path.empty())
				return;
			Path::uniform(path);

			if (!import(path, 1))
				return;
		} else {
			for (int i = 0; i < (int)open.result().size(); ++i) {
				std::string path = open.result()[i];
				if (path.empty())
					continue;
				Path::uniform(path);

				import(path, (int)open.result().size());
			}
		}

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project imported in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (void) -> promise::Promise {
						return popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Importing().c_str())
							.then(
								[next, df] (void) -> promise::Promise {
									return promise::newPromise(std::bind(next, df));
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileImportStringForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, const Text::Ptr &content) {
	const Text::Ptr content_ = content;

	auto next = [wnd, rnd, ws, content_] (promise::Defer df) -> void {
		Project::Ptr prj(new Project(wnd, rnd, ws));
		prj->fileSync()->initialize("", FileSync::Filters::PROJECT);

		auto import = [wnd, rnd, ws, prj, df] (const std::string &path, int total) -> bool {
			if (!prj->open(path.c_str())) {
				if (total == 1) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());
				} else {
					ws->warn("Cannot open project.");
				}

				return false;
			}

			ws->bubble(ws->theme()->dialogPrompt_OpenedProject(), nullptr);

			if (total == 1)
				df.resolve(prj);

			return true;
		};

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

		const std::string dir = Path::documentDirectory();
		const std::string path = Path::combine(dir.c_str(), "Program." GBBASIC_RICH_PROJECT_EXT);
		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::Accesses::WRITE)) {
			df.reject();

			popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

			return;
		}
		file->writeString(content_->text());
		file->close();
		file.reset();

		content_->clear();

		if (!import(path, 1))
			return;

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project opened in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (void) -> promise::Promise {
						return popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Importing().c_str())
							.then(
								[next, df] (void) -> promise::Promise {
									return promise::newPromise(std::bind(next, df));
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileImportForNotepad(Window* wnd, Renderer* rnd, Workspace* ws) {
	auto next = [wnd, rnd, ws] (promise::Defer df) -> void {
		Project::Ptr prj(new Project(wnd, rnd, ws));
		prj->fileSync()->initialize("", FileSync::Filters::PROJECT);
		if (!prj->fileSync()->requestOpen()) {
			df.reject();

			return;
		}
		const std::string path = prj->fileSync()->path();

		auto import = [wnd, rnd, ws, prj, df] (const std::string &path, int total) -> bool {
			if (!prj->open(path.c_str())) {
				if (total == 1) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());
				} else {
					const std::string msg = Text::format("Cannot open project at \"{0}\".", { path });
					ws->warn(msg.c_str());
				}

				return false;
			}

			if (ws->isUnderExamplesDirectory(path.c_str())) {
				prj->isExample(true);
			}

			ws->bubble(ws->theme()->dialogPrompt_OpenedProject(), nullptr);

			if (total == 1)
				df.resolve(prj);

			return true;
		};

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

		if (!import(path, 1))
			return;

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project opened in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (void) -> promise::Promise {
						return popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Importing().c_str())
							.then(
								[next, df] (void) -> promise::Promise {
									return promise::newPromise(std::bind(next, df));
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileImportExamples(Window* wnd, Renderer* rnd, Workspace* ws, const char* path_) {
	const std::string preferedPath = path_ ? path_ : "";

	auto next = [wnd, rnd, ws, preferedPath] (promise::Defer df) -> void {
		const std::string exampleDirectory = Path::absoluteOf(WORKSPACE_EXAMPLE_PROJECTS_DIR);
		std::string defaultPath = exampleDirectory;
		do {
			DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(exampleDirectory.c_str());
			FileInfos::Ptr fileInfos = dirInfo->getFiles("*." GBBASIC_RICH_PROJECT_EXT, false);
			if (fileInfos->count() == 0)
				break;

			FileInfo::Ptr fileInfo = fileInfos->get(0);
			defaultPath = fileInfo->fullPath();
#if defined GBBASIC_OS_WIN
			defaultPath = Text::replace(defaultPath, "/", "\\");
#endif /* GBBASIC_OS_WIN */
		} while (false);

		auto import = [wnd, rnd, ws] (const std::string &path, int total) -> Project::Ptr {
			const std::string abspath = Path::absoluteOf(path);
			for (int i = 0; i < (int)ws->projects().size(); ++i) {
				const Project::Ptr &prj_ = ws->projects()[i];
				const std::string abspath_ = Path::absoluteOf(prj_->path());
				if (abspath == abspath_) {
					if (total == 1) {
						popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_ProjectAlreadyExists().c_str());
					} else {
						const std::string msg = Text::format("Project \"{0}\" already exists.", { prj_->title() });
						ws->warn(msg.c_str());
					}

					return nullptr;
				}
			}

			Project::Ptr prj(new Project(wnd, rnd, ws));
			if (!prj->open(path.c_str())) {
				if (total == 1) {
					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());
				} else {
					const std::string msg = Text::format("Cannot import project at \"{0}\".", { path });
					ws->warn(msg.c_str());
				}

				return nullptr;
			}

			if (ws->isUnderExamplesDirectory(path.c_str())) {
				prj->isExample(true);
			}

			if (ws->showRecentProjects()) {
				ws->projects().push_back(prj);

				ws->sortProjects();

				if (total == 1) {
					const Project::Array &projects = ws->projects();
					for (int i = 0; i < (int)projects.size(); ++i) {
						const Project::Ptr &prj_ = projects[i];
						if (prj_.get() == prj.get()) {
							ws->recentProjectSelectedIndex(i);

							break;
						}
					}
				}

				++ws->activities().opened;
				switch (prj->contentType()) {
				case Project::ContentTypes::BASIC: ++ws->activities().openedBasic; break;
				case Project::ContentTypes::ROM:   ++ws->activities().openedRom;   break;
				default: GBBASIC_ASSERT(false && "Impossible.");                   break;
				}
			}

			ws->bubble(ws->theme()->dialogPrompt_ImportedProject(), nullptr);

			return prj;
		};

		const bool preferedPathExists = preferedPath.empty() || !Path::fileExists(preferedPath.c_str());
		if (preferedPathExists) {
			pfd::open_file open(
				ws->theme()->generic_Open(),
				defaultPath,
				GBBASIC_PROJECT_FILE_FILTER,
				pfd::opt::multiselect
			);
			if (open.result().empty() || open.result().front().empty()) {
				df.reject();

				return;
			}

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

			if (open.result().size() == 1) {
				std::string path = open.result().front();
				if (path.empty())
					return;
				Path::uniform(path);

				Project::Ptr prj = import(path, 1);
				if (!prj) {
					df.reject();

					return;
				}

				df.resolve(prj);
			} else {
				Project::Array projects;
				for (int i = 0; i < (int)open.result().size(); ++i) {
					std::string path = open.result()[i];
					if (path.empty())
						continue;
					Path::uniform(path);

					Project::Ptr prj = import(path, (int)open.result().size());
					if (prj)
						projects.push_back(prj);
				}

				if (projects.empty()) {
					df.reject();

					return;
				}

				df.resolve(projects);
			}

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long end = DateTime::ticks();
			const long long diff = end - start;
			const double secs = DateTime::toSeconds(diff);
			fprintf(stdout, "Project imported in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
		} else {
#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

			std::string path = preferedPath;
			if (path.empty())
				return;
			Path::uniform(path);

			Project::Ptr prj = import(path, 1);
			if (!prj) {
				df.reject();

				return;
			}

			df.resolve(prj);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long end = DateTime::ticks();
			const long long diff = end - start;
			const double secs = DateTime::toSeconds(diff);
			fprintf(stdout, "Project imported in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
		}
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (void) -> promise::Promise {
						return popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Importing().c_str())
							.then(
								[next, df] (void) -> promise::Promise {
									return promise::newPromise(std::bind(next, df));
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileImportExampleForNotepad(Window* wnd, Renderer* rnd, Workspace* ws) {
	auto next = [wnd, rnd, ws] (promise::Defer df) -> void {
		const std::string exampleDirectory = Path::absoluteOf(WORKSPACE_EXAMPLE_PROJECTS_DIR);
		std::string defaultPath = exampleDirectory;
		do {
			DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(exampleDirectory.c_str());
			FileInfos::Ptr fileInfos = dirInfo->getFiles("*." GBBASIC_RICH_PROJECT_EXT, false);
			if (fileInfos->count() == 0)
				break;

			FileInfo::Ptr fileInfo = fileInfos->get(0);
			defaultPath = fileInfo->fullPath();
#if defined GBBASIC_OS_WIN
			defaultPath = Text::replace(defaultPath, "/", "\\");
#endif /* GBBASIC_OS_WIN */
		} while (false);

		Project::Ptr prj(new Project(wnd, rnd, ws));
		prj->fileSync()->initialize(defaultPath, FileSync::Filters::PROJECT);
		if (!prj->fileSync()->requestOpen()) {
			df.reject();

			return;
		}
		const std::string path = prj->fileSync()->path();

		auto import = [wnd, rnd, ws, prj] (const std::string &path, int total) -> Project::Ptr {
			if (!prj->open(path.c_str())) {
				if (total == 1) {
					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());
				} else {
					const std::string msg = Text::format("Cannot open project at \"{0}\".", { path });
					ws->warn(msg.c_str());
				}

				return nullptr;
			}

			if (ws->isUnderExamplesDirectory(path.c_str())) {
				prj->isExample(true);
			}

			ws->bubble(ws->theme()->dialogPrompt_OpenedProject(), nullptr);

			return prj;
		};

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

		Project::Ptr prj_ = import(path, 1);
		if (!prj_) {
			df.reject();

			return;
		}

		df.resolve(prj_);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project opened in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, next, df] (void) -> promise::Promise {
						return promise::newPromise(std::bind(next, df));
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileExportForNotepad(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj) {
	auto next = [wnd, rnd, ws, prj] (promise::Defer df, std::string name, std::string path) -> void {
		Project::Ptr prj_(new Project(wnd, rnd, ws));
		*prj_ = *prj;

		prj_->save(path.c_str(), true, nullptr);

		df.resolve(prj_);
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> promise::Promise {
			const std::string &title = prj->title();
			pfd::save_file save(
				ws->theme()->generic_SaveTo(),
				Text::sanitizeFilename(title) + "." GBBASIC_RICH_PROJECT_EXT,
				GBBASIC_PROJECT_FILE_FILTER
			);
			std::string path = save.result();
			Path::uniform(path);
			if (path.empty()) {
				df.reject();

				return promise::newPromise(
					[] (promise::Defer df) -> void {
						df.reject();
					}
				);
			}
			std::string ext;
			Path::split(path, nullptr, &ext, nullptr);
			Text::toLowerCase(ext);
			if (ext.empty() || ext != GBBASIC_RICH_PROJECT_EXT) {
				path += "." GBBASIC_RICH_PROJECT_EXT;
			}
			if (!ws->canWriteProjectTo(path.c_str())) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations().c_str());

				return promise::newPromise(
					[] (promise::Defer df) -> void {
						df.reject();
					}
				);
			}

			const std::string abspath = Path::absoluteOf(path);
			for (int i = 0; i < (int)ws->projects().size(); ++i) {
				const Project::Ptr &prj_ = ws->projects()[i];
				const std::string abspath_ = Path::absoluteOf(prj_->path());
				if (abspath == abspath_) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveProject().c_str());

					return promise::newPromise(
						[] (promise::Defer df) -> void {
							df.reject();
						}
					);
				}
			}

			return popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
				.then(
					[next, df, title, path] (void) -> promise::Promise {
						return promise::newPromise(std::bind(next, df, title, path));
					}
				);
		}
	);
}

promise::Promise Operations::fileDuplicate(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj, const char* fontConfigPath) {
	auto next = [wnd, rnd, ws, prj, fontConfigPath] (promise::Defer df, std::string name, std::string path) -> void {
		const std::string &src = prj->path();
		if (!Path::copyFile(src.c_str(), path.c_str())) {
			df.reject();

			return;
		}

		Project::Ptr prj_(new Project(wnd, rnd, ws));
		if (!prj_->open(path.c_str())) {
			df.reject();

			popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

			return;
		}

		Texture::Ptr attribtex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		Texture::Ptr propstex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		Texture::Ptr actorstex(ws->theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
		prj_->attributesTexture(attribtex);
		prj_->propertiesTexture(propstex);
		prj_->actorsTexture(actorstex);

		prj->behaviourSerializer(std::bind(&Workspace::serializeKernelBehaviour, ws, std::placeholders::_1));
		prj->behaviourParser(std::bind(&Workspace::parseKernelBehaviour, ws, std::placeholders::_1));

		if (!prj_->load(false, fontConfigPath, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
			df.reject();

			popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

			return;
		}

		prj_->title(name);
		prj_->order(PROJECT_DEFAULT_ORDER);
		prj_->hasDirtyInformation(true);
		prj_->save(path.c_str(), true, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2));
		prj_->unload();

		ws->projects().push_back(prj_);

		ws->sortProjects();

		const Project::Array &projects = ws->projects();
		for (int i = 0; i < (int)projects.size(); ++i) {
			const Project::Ptr &prj__ = projects[i];
			if (prj__.get() == prj_.get()) {
				ws->recentProjectSelectedIndex(i);

				break;
			}
		}

		++ws->activities().created;
		switch (prj->contentType()) {
		case Project::ContentTypes::BASIC: ++ws->activities().createdBasic; break;
		default: GBBASIC_ASSERT(false && "Impossible.");                    break;
		}

		ws->bubble(ws->theme()->dialogPrompt_DuplicatedProject(), nullptr);

		df.resolve(prj_);
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, prj, next, df] (bool /* ok */) -> promise::Promise {
						const std::string title = prj->title() + " Copy";
						pfd::save_file save(
							ws->theme()->generic_SaveTo(),
							Text::sanitizeFilename(title) + "." GBBASIC_RICH_PROJECT_EXT,
							GBBASIC_PROJECT_FILE_FILTER
						);
						std::string path = save.result();
						Path::uniform(path);
						if (path.empty()) {
							df.reject();

							return promise::newPromise(
								[] (promise::Defer df) -> void {
									df.reject();
								}
							);
						}
						std::string ext;
						Path::split(path, nullptr, &ext, nullptr);
						Text::toLowerCase(ext);
						if (ext.empty() || ext != GBBASIC_RICH_PROJECT_EXT) {
							path += "." GBBASIC_RICH_PROJECT_EXT;
						}
						if (!ws->canWriteProjectTo(path.c_str())) {
							df.reject();

							popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations().c_str());

							return promise::newPromise(
								[] (promise::Defer df) -> void {
									df.reject();
								}
							);
						}

						const std::string abspath = Path::absoluteOf(path);
						for (int i = 0; i < (int)ws->projects().size(); ++i) {
							const Project::Ptr &prj_ = ws->projects()[i];
							const std::string abspath_ = Path::absoluteOf(prj_->path());
							if (abspath == abspath_) {
								df.reject();

								popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveProject().c_str());

								return promise::newPromise(
									[] (promise::Defer df) -> void {
										df.reject();
									}
								);
							}
						}

						return popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
							.then(
								[next, df, title, path] (void) -> promise::Promise {
									return promise::newPromise(std::bind(next, df, title, path));
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fileBrowseExamples(Window*, Renderer*, Workspace*) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (!Path::directoryExists(WORKSPACE_EXAMPLE_PROJECTS_DIR)) {
				df.reject();

				return;
			}

			std::string path = WORKSPACE_EXAMPLE_PROJECTS_DIR;
			path = Path::absoluteOf(path);
			path = Unicode::toOs(path);
			Platform::browse(path.c_str());

			df.resolve(true);
		}
	);
}

promise::Promise Operations::fileBrowse(Window*, Renderer*, Workspace*, const Project::Ptr &prj) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (!prj || !prj->exists()) {
				df.reject();

				return;
			}

			const std::string &path = prj->path();
			std::string path_;
			Path::split(path, nullptr, nullptr, &path_);
			path_ = Unicode::toOs(path_);
			Platform::browse(path_.c_str());

			df.resolve(true);
		}
	);
}

promise::Promise Operations::fileSaveStreamed(Window* wnd, Renderer* rnd, Workspace* ws, const Bytes::Ptr &bytes) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			pfd::save_file save(
				ws->theme()->generic_SaveTo(),
				"",
				GBBASIC_ANY_FILE_FILTER
			);
			std::string path = save.result();
			Path::uniform(path);
			if (path.empty()) {
				df.reject();

				return;
			}

			auto next = [wnd, rnd, ws, bytes, path] (promise::Defer df) -> void {
				File::Ptr file(File::create());
				if (!file->open(path.c_str(), Stream::WRITE)) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveData().c_str());

					return;
				}
				if (bytes && !bytes->empty())
					file->writeBytes(bytes.get());
				file->close();
				file = nullptr;

				ws->bubble(ws->theme()->dialogPrompt_SavedData(), nullptr);

				df.resolve(true);
			};

			popupWait(wnd, rnd, ws, true, ws->theme()->dialogPrompt_Saving().c_str(), true, false)
				.then(
					[next, df] (void) -> promise::Promise {
						return promise::newPromise(std::bind(next, df));
					}
				);
		}
	);
}

promise::Promise Operations::editSortAssets(Window* wnd, Renderer* rnd, Workspace* ws, AssetsBundle::Categories category) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (!prj) {
				df.resolve(false);

				return;
			}

			fileAskSave(wnd, rnd, ws, false)
				.then(
					[wnd, rnd, ws, category, df] (bool ok) -> promise::Promise {
						if (!ok)
							return never(wnd, rnd, ws);

						return popupAssetsSorting(wnd, rnd, ws, category)
							.then(
								[ws, df] (bool ok) -> void {
									WORKSPACE_AUTO_CLOSE_POPUP(ws)

									df.resolve(ok);
								}
							)
							.fail(
								[ws, df] (void) -> void {
									WORKSPACE_AUTO_CLOSE_POPUP(ws)

									df.reject();
								}
							);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fontAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->fontPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			popupExternalFontResolver(wnd, rnd, ws, ws->theme()->generic_Path().c_str())
				.then(
					[wnd, rnd, ws, df, prj] (const char* path, const Math::Vec2i* size, bool copy, bool embed) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (!path) {
							df.reject();

							return;
						}

						std::string content;
						content.assign((const char*)RES_TEXT_FONT_CONTENT, GBBASIC_COUNTOF(RES_TEXT_FONT_CONTENT));
						prj->addFontPage(copy, embed, path, size, content, true);
						prj->hasDirtyAsset(true);

						FontAssets::Entry* entry = prj->getFont(prj->fontPageCount() - 1);
						if (entry->bitmapData.empty()) { // Is TTF.
							int size_ = -1;
							int offset = 0;
							if (FontAssets::calculateBestFit(*entry, &size_, &offset)) {
								// Do nothing.
							} else {
								size_ = prj->preferencesFontSize().y > 0 ? prj->preferencesFontSize().y : GBBASIC_FONT_DEFAULT_SIZE;
								offset = prj->preferencesFontOffset();
							}
							size_ = Math::clamp(size_, GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MAX_SIZE);
							offset = Math::clamp(offset, -GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE);
							if (entry->size.y != size_ || entry->offset != offset) {
								entry->size = Math::Vec2i(-1, size_);
								entry->offset = offset;
								entry->cleanup();
								entry->touch();
							}
						} else { // Is bitmap-based.
							entry->offset = 0;
						}
						entry->isTwoBitsPerPixel = prj->preferencesFontIsTwoBitsPerPixel();
						entry->preferFullWord = prj->preferencesFontPreferFullWord();
						entry->preferFullWordForNonAscii = prj->preferencesFontPreferFullWordForNonAscii();

						ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::FONT);

						df.resolve(true);
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::fontDuplicatePage(Window* wnd, Renderer* rnd, Workspace* ws, int index) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->fontPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			FontAssets::Entry* entry = prj->getFont(index);
			if (!entry) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_InvalidData().c_str());

				return;
			}

			prj->duplicateFontPage(entry);
			prj->hasDirtyAsset(true);

			ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::FONT);

			df.resolve(true);
		}
	);
}

promise::Promise Operations::fontRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			const int page = prj->activeFontIndex();

			if (prj->fontPageCount() == 0) {
				df.reject();

				return;
			}

			FontAssets::Entry* entry = prj->getFont(page);
			if (!entry->isAsset) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotRemoveTheBuiltinAssetPage().c_str());

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeFontPage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::FONT, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::codeAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->codePageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			prj->addCodePage("");
			prj->hasDirtyAsset(true);

			ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::CODE);

			df.resolve(true);
		}
	);
}

promise::Promise Operations::codeRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			int page = prj->activeMajorCodeIndex();
			if (!prj->isMajorCodeEditorActive() && prj->isMinorCodeEditorEnabled()) {
				page = prj->activeMinorCodeIndex();
			}

			if (prj->codePageCount() == 0) {
				df.reject();

				return;
			}
			if (prj->codePageCount() == 1) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotRemoveTheOnlyAssetPage().c_str());

				return;
			}

			BaseAssets::Entry* entry = prj->getCode(page);
			EditorCode* editor = (EditorCode*)entry->editor;
			std::string txt;
			if (editor) {
				txt = editor->toString();
			}
			const bool canRemoveDirectly = (!editor || txt.empty()) && (editor && !editor->hasUnsavedChanges());
			if (canRemoveDirectly) {
				if (prj->removeCodePage(page)) {
					prj->hasDirtyAsset(true);

					ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::CODE, page);

					df.resolve(true);

					return;
				} else {
					df.reject();

					return;
				}
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeCodePage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::CODE, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::tilesAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->tilesPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			std::string str;
			TilesAssets::Entry default_(rnd, prj->paletteGetter());
			default_.toString(str, nullptr);

			prj->addTilesPage(str, true);
			prj->hasDirtyAsset(true);

			ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::TILES);

			df.resolve(true);
		}
	);
}

promise::Promise Operations::tilesRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			const int page = prj->activeTilesIndex();

			if (prj->tilesPageCount() == 0) {
				df.reject();

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeTilesPage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::TILES, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::mapAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->mapPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			popupExternalMapResolver(wnd, rnd, ws, ws->theme()->generic_Path().c_str())
				.then(
					[wnd, rnd, ws, df, prj] (const int* index, const char* path, bool allowFlip) -> void {
						if (index) { // From tiles asset.
							if (prj->tilesPageCount() == 0) {
								df.reject();

								popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CreateAPageOfTilesFirst().c_str(), ws->theme()->generic_Goto().c_str(), ws->theme()->generic_Later().c_str(), nullptr)
									.then(
										[wnd, rnd, ws] (bool ok) -> void {
											WORKSPACE_AUTO_CLOSE_POPUP(ws)

											if (ok) {
												ws->category(Workspace::Categories::TILES);
											}
										}
									);

								return;
							}

							WORKSPACE_AUTO_CLOSE_POPUP(ws)

							std::string str;
							const int ref = Math::clamp(*index, 0, prj->tilesPageCount() - 1);
							Texture::Ptr attribtex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
							MapAssets::Entry default_(ref, prj->tilesGetter(), attribtex);
							default_.toString(str, nullptr);

							std::string preferedName;
							const TilesAssets::Entry* tilesEntry = prj->getTiles(ref);
							if (tilesEntry) {
								if (!Text::startsWith(tilesEntry->name, "tiles", true))
									preferedName = tilesEntry->name;
							}

							prj->addMapPage(str, true, preferedName.empty() ? nullptr : preferedName.c_str());
							prj->hasDirtyAsset(true);

							ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::MAP);

							df.resolve(true);
						} else if (path) { // From image file.
							if (!path) {
								df.reject();

								popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_InvalidFile().c_str());

								return;
							}

							if (prj->tilesPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
								df.reject();

								popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMoreTilesPage().c_str());

								return;
							}

							const std::string path_ = path;

							auto next = [wnd, rnd, ws, prj, path_, allowFlip] (promise::Defer df) -> void {
								// Load the image.
								if (!Path::fileExists(path_.c_str())) {
									df.reject();

									popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_InvalidFile().c_str());

									return;
								}

								File::Ptr file(File::create());
								if (!file->open(path_.c_str(), Stream::READ)) {
									df.reject();

									popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_InvalidFile().c_str());

									return;
								}

								Bytes::Ptr bytes(Bytes::create());
								if (file->readBytes(bytes.get()) == 0) {
									file->close();

									df.reject();

									popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_InvalidFile().c_str());

									return;
								}
								file->close();

								Image::Ptr img(Image::create());
								if (!img->fromBytes(bytes.get())) {
									df.reject();

									popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_InvalidFile().c_str());

									return;
								}

								// Load from the image.
								Texture::Ptr attribtex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
								TilesAssets::Entry tiles(rnd, prj->paletteGetter());
								MapAssets::Entry map("", prj->tilesGetter(), attribtex);
								const bool loaded = MapAssets::parseImage(
									tiles, map,
									img.get(), allowFlip,
									true,
									prj->paletteGetter(),
									prj->tilesGetter(), prj->tilesPageCount(),
									std::bind(operationsHandlePrint, ws, std::placeholders::_1), std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2)
								);
								if (!loaded) {
									df.reject();

									popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_InvalidFile().c_str());

									return;
								}

								WORKSPACE_AUTO_CLOSE_POPUP(ws)

								// Add tiles asset.
								if (tiles.data) {
									std::string tilesStr;
									tiles.toString(tilesStr, nullptr);

									prj->addTilesPage(tilesStr, true);

									ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::TILES);
								}

								// Add map asset.
								std::string mapStr;
								map.toString(mapStr, nullptr);

								std::string preferedName;
#if !defined GBBASIC_OS_HTML
								std::string name;
								Path::split(path_, &name, nullptr, nullptr);
								if (!Text::startsWith(name, "tiles", true))
									preferedName = name;
#endif /* Platform macro. */

								prj->addMapPage(mapStr, true, preferedName.empty() ? nullptr : preferedName.c_str());

								ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::MAP);

								// Resolve.
								prj->hasDirtyAsset(true);

								df.resolve(true);
							};

							popupWait(wnd, rnd, ws, true, ws->theme()->dialogPrompt_Opening().c_str(), true, false)
								.then(
									[next, df] (void) -> promise::Promise {
										return promise::newPromise(std::bind(next, df));
									}
								);
						} else {
							WORKSPACE_AUTO_CLOSE_POPUP(ws)

							df.reject();
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::mapRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			const int page = prj->activeMapIndex();

			if (prj->mapPageCount() == 0) {
				df.reject();

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeMapPage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::MAP, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::musicAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->musicPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			std::string str;
			MusicAssets::Entry default_;
			default_.toString(str, nullptr);

			prj->addMusicPage(str, true);
			prj->hasDirtyAsset(true);

			ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::MUSIC);

			df.resolve(true);
		}
	);
}

promise::Promise Operations::musicRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			const int page = prj->activeMusicIndex();

			if (prj->musicPageCount() == 0) {
				df.reject();

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeMusicPage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::MUSIC, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::sfxAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	ImGuiIO &io = ImGui::GetIO();

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->sfxPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			const bool shift = io.KeyShift;
#if GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CTRL
			const bool modifier = io.KeyCtrl;
#elif GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CMD
			const bool modifier = io.KeySuper;
#endif /* GBBASIC_MODIFIER_KEY */

			std::string str;
			SfxAssets::Entry default_;
			default_.toString(str, nullptr);

			prj->addSfxPage(str, true, shift, modifier);
			prj->hasDirtyAsset(true);

			ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::SFX);

			df.resolve(true);
		}
	);
}

promise::Promise Operations::sfxRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			const int page = prj->activeSfxIndex();

			if (prj->sfxPageCount() == 0) {
				df.reject();

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeSfxPage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::SFX, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::actorAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->actorPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			std::string str;
			ActorAssets::Entry default_(rnd, prj->preferencesActorUses8x16Sprites(), prj->paletteGetter(), prj->behaviourSerializer(), prj->behaviourParser());
			default_.toString(str, nullptr);

			prj->addActorPage(str, true);
			prj->hasDirtyAsset(true);

			ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::ACTOR);

			df.resolve(true);
		}
	);
}

promise::Promise Operations::actorRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			const int page = prj->activeActorIndex();

			if (prj->actorPageCount() == 0) {
				df.reject();

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeActorPage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::ACTOR, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::sceneAddPage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			if (prj->scenePageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotAddMorePage().c_str());

				return;
			}

			popupExternalSceneResolver(wnd, rnd, ws)
				.then(
					[wnd, rnd, ws, df, prj] (int index, bool useGravity) -> void {
						if (prj->mapPageCount() == 0) {
							df.reject();

							popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CreateAPageOfMapFirst().c_str(), ws->theme()->generic_Goto().c_str(), ws->theme()->generic_Later().c_str(), nullptr)
								.then(
									[wnd, rnd, ws] (bool ok) -> void {
										WORKSPACE_AUTO_CLOSE_POPUP(ws)

										if (ok) {
											ws->category(Workspace::Categories::MAP);
										}
									}
								);

							return;
						}

						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						std::string str;
						const int ref = Math::clamp(index, 0, prj->mapPageCount() - 1);
						Texture::Ptr propstex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
						Texture::Ptr actorstex(ws->theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
						SceneAssets::Entry default_(ref, prj->mapGetter(), prj->actorGetter(), propstex, actorstex);
						if (useGravity) {
							default_.definition.gravity = 10;
							default_.definition.jump_gravity = 50;
							default_.definition.jump_max_count = 1;
							default_.definition.jump_max_ticks = 10;
							default_.definition.climb_velocity = 20;
						}
						default_.toString(str, nullptr);

						std::string preferedName;
						const MapAssets::Entry* mapEntry = prj->getMap(ref);
						if (mapEntry) {
							if (!Text::startsWith(mapEntry->name, "map", true))
								preferedName = mapEntry->name;
						}

						prj->addScenePage(str, true, preferedName.empty() ? nullptr : preferedName.c_str());
						prj->hasDirtyAsset(true);

						prj->preferencesSceneUseGravity(useGravity);

						ws->pageAdded(wnd, rnd, prj.get(), Workspace::Categories::SCENE);

						df.resolve(true);
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::sceneRemovePage(Window* wnd, Renderer* rnd, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			const int page = prj->activeSceneIndex();

			if (prj->scenePageCount() == 0) {
				df.reject();

				return;
			}

			popupMessage(wnd, rnd, ws, ws->theme()->dialogAsk_RemoveTheCurrentAssetPage().c_str(), true, false)
				.then(
					[wnd, rnd, ws, df, prj, page] (bool ok) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						if (ok) {
							if (prj->removeScenePage(page)) {
								prj->hasDirtyAsset(true);

								ws->pageRemoved(wnd, rnd, prj.get(), Workspace::Categories::SCENE, page);

								df.resolve(true);

								return;
							} else {
								df.reject();

								return;
							}
						} else {
							df.resolve(false);
						}
					}
				)
				.fail(
					[ws, df] (void) -> void {
						WORKSPACE_AUTO_CLOSE_POPUP(ws)

						df.reject();
					}
				);
		}
	);
}

promise::Promise Operations::projectCompile(Window* wnd, Renderer* rnd, Workspace* ws, const char* cartType_, const char* sramType_, bool* hasRtc_, const char* fontConfigPath, bool useInRam) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			// Prepare.
			const Project::Ptr &prj = ws->currentProject();
			const std::string &path = prj->path();
			if (!useInRam) {
				if (path.empty() || !Path::fileExists(path.c_str())) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_TheProjectIsNotSavedYet().c_str());

					return;
				}
			}

			// Get the project.
			Project::Ptr prj_ = nullptr;
			if (useInRam) {
				// Use the in-RAM project.
				prj_ = prj;

				// Replace with the content in unsaved minor editor if available.
				if (prj->minorCodeEditor() && prj->minorCodeEditor()->hasUnsavedChanges()) {
					const std::string &code = prj->minorCodeEditor()->toString();
					const int idx = prj->activeMinorCodeIndex();
					CodeAssets::Entry* entry = prj_->getCode(idx);
					entry->data = code;
				}
			} else {
				// Create a new project.
				prj_ = Project::Ptr(new Project(wnd, rnd, ws));
				if (!prj_->open(path.c_str())) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

					return;
				}

				// Load the program from disk.
				if (!prj_->load(false, fontConfigPath, std::bind(operationsHandleWarningOrError, ws, std::placeholders::_1, std::placeholders::_2))) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotOpenProject().c_str());

					return;
				}
			}

			// Find an active kernel.
			const GBBASIC::Kernel::Ptr krnl = ws->activeKernel();
			if (!krnl) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotFindValidKernel().c_str());

				return;
			}

			// Clear any error markers.
			for (int i = 0; i < prj->codePageCount(); ++i) {
				CodeAssets::Entry* entry = prj->getCode(i);
				Editable* editor = entry->editor;
				if (editor)
					editor->post(Editable::CLEAR_ERRORS);
			}

			// Switch to the console.
			switch (ws->category()) {
			case Workspace::Categories::PALETTE: // Fall through.
			case Workspace::Categories::CONSOLE: // Fall through.
			case Workspace::Categories::EMULATOR: // Fall through.
			case Workspace::Categories::HOME: // Do nothing.
				break;
			default:
				ws->categoryBeforeCompiling(ws->category());

				break;
			}
			ws->category(Workspace::Categories::CONSOLE);
			ws->tabsWidth(0.0f);
			ws->consoleHasWarning(false);
			ws->consoleHasError(false);

			// Compile the new loaded project.
			std::string dir;
			Path::split(krnl->path(), nullptr, nullptr, &dir);
			const std::string rom = Path::combine(dir.c_str(), krnl->kernelRom().c_str());
			const std::string sym = Path::combine(dir.c_str(), krnl->kernelSymbols().c_str());
			const std::string aliases = Path::combine(dir.c_str(), krnl->kernelAliases().c_str());
			const int bootstrap = krnl->bootstrapBank();
			const std::string cartType =
				cartType_ ? cartType_ :
				(prj_->cartridgeType().empty() ? PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED :
					prj_->cartridgeType());
			const std::string sramType =
				sramType_ ? sramType_ :
				(prj_->sramType().empty() ? PROJECT_SRAM_TYPE_32KB :
					prj_->sramType());
			const bool hasRtc =
				hasRtc_ ? *hasRtc_ :
					prj_->hasRtc();
			const bool caseInsensitive = prj_->caseInsensitive();
			const bool strictOn = prj_->strictOn();

			Text::Dictionary arguments;
			arguments[COMPILER_ROM_OPTION_KEY]                              = rom;
			arguments[COMPILER_SYM_OPTION_KEY]                              = sym;
			arguments[COMPILER_ALIASES_OPTION_KEY]                          = aliases;
			arguments[COMPILER_FONT_OPTION_KEY]                             = WORKSPACE_FONT_DEFAULT_CONFIG_FILE;
#if defined GBBASIC_DEBUG
			arguments[COMPILER_AST_OPTION_KEY]                              = "stdout";
#else /* GBBASIC_DEBUG */
			arguments[COMPILER_AST_OPTION_KEY]                              = ws->settings().debugShowAstEnabled ? "stdout" : "none";
#endif /* GBBASIC_DEBUG */
			arguments[COMPILER_TITLE_OPTION_KEY]                            = prj_->title();
			arguments[COMPILER_CART_TYPE_OPTION_KEY]                        = cartType;
			arguments[COMPILER_RAM_TYPE_OPTION_KEY]                         = sramType;
			arguments[COMPILER_RTC_OPTION_KEY]                              = Text::toString(hasRtc);
			arguments[COMPILER_CASE_INSENSITIVE_OPTION_KEY]                 = Text::toString(caseInsensitive);
			arguments[COMPILER_STRICT_ON_OPTION_KEY]                        = Text::toString(strictOn);
			// Do not: `arguments[COMPILER_EXPLICIT_LINE_NUMBER_OPTION_KEY] = "";`.
			arguments[COMPILER_DECLARATION_REQUIRED_OPTION_KEY]             = "true";
			arguments[COMPILER_BOOTSTRAP_OPTION_KEY]                        = Text::toString(bootstrap);
			if (!krnl->memoryHeapSize().empty())
				arguments[COMPILER_HEAP_SIZE_OPTION_KEY]                    = krnl->memoryHeapSize();
			if (!krnl->memoryStackSize().empty())
				arguments[COMPILER_STACK_SIZE_OPTION_KEY]                   = krnl->memoryStackSize();
			arguments[COMPILER_OPTIMIZE_CODE_OPTION_KEY]                    = "true";
			arguments[COMPILER_OPTIMIZE_ASSETS_OPTION_KEY]                  = "true";
			arguments[WORKSPACE_OPTION_APPLICATION_DO_NOT_QUIT_KEY]         = "";

			ws->compile(wnd, rnd, prj_, arguments);

			// Finish.
			df.resolve(true);
		}
	);
}

promise::Promise Operations::projectDump(Window*, Renderer*, Workspace*, Bytes::Ptr rom, const char* path) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (!rom) {
				df.reject();

				return;
			}
			if (!path) {
				df.reject();

				return;
			}

			File::Ptr file(File::create());
			if (!file->open(path, Stream::WRITE)) {
				df.reject();

				return;
			}
			std::string hex;
			std::string txt;
			for (int i = 0; i < (int)rom->count(); ++i) {
				Byte b = rom->get(i);
				char ch = (char)b;
				if (ch == '\r' || ch == '\n')
					ch = ' ';
				txt += ch;
				std::string h = Text::toHex(b, 2, '0', true);
				if (i % 16 == 0) {
					std::string lno = "0x" + Text::toHex(i, 8, '0', true);
					hex += lno;
					hex += " ";
				}
				hex += h;
				if (i % 16 == 15) {
					hex += " ";
					hex += txt;
					txt = "";
					hex += "\r\n";
				} else {
					hex += " ";
				}
			}
			file->writeString(hex);
			file->close();

			df.resolve(true);
		}
	);
}

promise::Promise Operations::projectBuild(Window* wnd, Renderer* rnd, Workspace* ws, Bytes::Ptr rom, Exporter::Ptr ex) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Project::Ptr &prj = ws->currentProject();
			std::string cartType;
			if (prj)
				cartType = prj->cartridgeType();
			const bool isClassic =
				cartType == PROJECT_CARTRIDGE_TYPE_CLASSIC ||
				cartType == PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED ||
				cartType == PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION ||
				cartType == PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION;
			const std::string ext0 = ex->extensions().front();
			const std::string ext1 = ex->extensions().back();
			std::string path;
			if (!ext0.empty() && !ext1.empty()) {
				std::string title;
				if (prj) {
					title = prj->title();
					if (isClassic)
						title += "." + ext0;
					else
						title += "." + ext1;
				}

				pfd::save_file save(
					ws->theme()->generic_SaveTo(),
					Text::sanitizeFilename(title),
					ex->filter()
				);
				path = save.result();
				Path::uniform(path);
				if (path.empty()) {
					df.reject();

					return;
				}

				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || !(ext == ext0 || ext == ext1)) {
					if (isClassic)
						path += "." + ext0;
					else
						path += "." + ext1;
				}
				if (!ws->canWriteProjectTo(path.c_str())) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations().c_str());

					return;
				}
			}

			auto next = [wnd, rnd, ws, rom, ex, path] (promise::Defer df) -> void {
				const long long start = DateTime::ticks();

				std::string exported;
				std::string hosted;
				const bool ret = ex->run(
					path.c_str(), rom,
					&exported, &hosted,
					ws->settings().exporterSettings, ws->settings().exporterArgs, ws->settings().exporterIcon,
					[ws] (const std::string &msg, int lv) -> void {
						switch (lv) {
						case 0: // Print.
							ws->print(msg.c_str());

							break;
						case 1: // Warn.
							ws->warn(msg.c_str());

							break;
						case 2: // Error.
							ws->error(msg.c_str());

							break;
						}
					}
				);
				if (!ret) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotExportProject().c_str());

					return;
				}

				GBBASIC_ASSERT(!ex->messageEnabled() && "Impossible.");

				if (path.empty()) {
					const std::string msg = ws->theme()->dialogPrompt_TheRomIsEncodedInTheUrlOfTheOpenedBrowserPageMakeSureYourBrowserSupportsLongUrl();
					popupMessage(wnd, rnd, ws, msg.c_str(), false, false)
						.then(
							[ws, ex, exported] (bool /* ok */) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)
							}
						);
				} else if (!hosted.empty()) { // Has been hosted locally.
					const std::string msg = Text::format(
						ws->theme()->dialogAsk_ProjectHasBeenBuiltAndIsBeingHostedAt_StopHostingAndBrowseThePackage(),
						{ hosted }
					);
					popupMessage(wnd, rnd, ws, msg.c_str(), true, false)
						.then(
							[ws, ex, exported] (bool ok) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)

								ws->print("Local server has been stopped for preview. To maintain access for other players, consider");
								ws->print("setting up a dedicated web server or uploading the game to a cloud hosting platform.");
								ex->reset();

								if (ok) {
									FileInfo::Ptr fileInfo = FileInfo::make(exported.c_str());
									std::string path_ = fileInfo->parentPath();
									path_ = Unicode::toOs(path_);
									Platform::browse(path_.c_str());
								}
							}
						)
						.fail(
							[ws] (void) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)
							}
						);
				} else { // Has outputed a package.
#if !defined GBBASIC_OS_HTML
					const std::string msg = ws->theme()->dialogAsk_BrowseTheExportedFile();
					popupMessage(wnd, rnd, ws, msg.c_str(), true, false)
						.then(
							[ws, ex, exported] (bool ok) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)

								if (ok) {
									FileInfo::Ptr fileInfo = FileInfo::make(exported.c_str());
									std::string path_ = fileInfo->parentPath();
									path_ = Unicode::toOs(path_);
									Platform::browse(path_.c_str());
								}
							}
						)
						.fail(
							[ws] (void) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)
							}
						);
#endif /* Platform macro. */
				}

				ws->bubble(ws->theme()->dialogPrompt_ExportedProject(), nullptr);

				df.resolve(true);

				const long long end = DateTime::ticks();
				const long long diff = end - start;
				const double secs = DateTime::toSeconds(diff);
				const std::string msg = "Project built in " + Text::toString(secs) + "s.";
				ws->print(msg.c_str());
				fprintf(stdout, "Project built in %gs.\n", secs);
			};

			popupWait(wnd, rnd, ws, true, ws->theme()->dialogPrompt_Saving().c_str(), true, false)
				.then(
					[next, df] (void) -> promise::Promise {
						return promise::newPromise(std::bind(next, df));
					}
				);
		}
	);
}

promise::Promise Operations::projectRun(Window*, Renderer*, Workspace* ws, Bytes::Ptr rom, Bytes::Ptr sram, bool traceless) {
	return promise::newPromise(
		[&, rom, sram, traceless] (promise::Defer df) -> void {
			if (!traceless)
				ws->print("Begin running.");

			ws->category(Workspace::Categories::EMULATOR);
			ws->tabsWidth(0.0f);

			ws->canvasDevice(Device::Ptr(Device::create(Device::CoreTypes::BINJGB, ws)));
			for (int i = 0; i < GBBASIC_COUNTOF(ws->settings().deviceClassicPalette); ++i)
				ws->canvasDevice()->classicPalette(i, ws->settings().deviceClassicPalette[i]);
			const bool suc = ws->canvasDevice()->open(
				rom,
				(Device::DeviceTypes)ws->settings().deviceType, ws->settings().devicePreferSgb,
				ws->input(),
				sram,
				true, true, traceless
			);
			if (!suc) {
				ws->canvasDevice()->close(nullptr);
				ws->canvasDevice(nullptr);

				df.reject();

				return;
			}

			ws->canvasDevice()->speed(ws->settings().emulatorSpeed);

			const bool cgb = ws->canvasDevice()->cartridgeHasCgbSupport();
			const bool sgb = ws->canvasDevice()->cartridgeHasSgbSupport();
			const bool ext = ws->canvasDevice()->cartridgeHasExtSupport();
			const Device::DeviceTypes dev = ws->canvasDevice()->deviceType();
			const Device::DeviceTypes edev = ws->canvasDevice()->enabledDeviceType();
			const int sram_ = ws->canvasDevice()->cartridgeSramSize(nullptr);
			const int rtc = ws->canvasDevice()->cartridgeHasRtc();
			const std::string cartFlag = std::string(cgb ? "COLOR" : "GRAY");
			const std::string sgbFlag = std::string(sgb ? "[SGB]" : "");
			const std::string devFlag = std::string(
				edev == Device::DeviceTypes::SUPER            ? "SUPER" :
				edev == Device::DeviceTypes::SUPER_EXTENDED   ? "SUPER[EXT]" :
				dev  == Device::DeviceTypes::CLASSIC          ? "GRAY" :
				dev  == Device::DeviceTypes::COLORED          ? "COLOR" :
				dev  == Device::DeviceTypes::CLASSIC_EXTENDED ? "GRAY[EXT]" :
				dev  == Device::DeviceTypes::COLORED_EXTENDED ? "COLOR[EXT]" :
				                                                "UNKNOWN"
			);
			ws->canvasCartridgeStatusText(
				cartFlag + " " +
				std::string(sram_ ? "SRAM[YES]" : "SRAM[NO]") + " " +
				std::string(rtc ? "RTC[YES]" : "RTC[NO]")
			);
			ws->canvasDeviceStatusText(
				devFlag
			);
			ws->canvasStatusTooltip(
				Text::format(
					ws->theme()->tooltipEmulator_StatusNote(),
					{
						cartFlag + sgbFlag,
						std::string(ext ? "YES" : "NO"),
						devFlag,
						std::string(sram_ ? "YES" : "NO"),
						std::string(rtc ? "YES" : "NO")
					}
				)
			);

			const Project::Ptr &prj = ws->currentProject();

			if (!traceless) {
				ws->activities().playingStartTimestamp = DateTime::ticks();

				++ws->activities().played;
				switch (prj->contentType()) {
				case Project::ContentTypes::BASIC: ++ws->activities().playedBasic; break;
				case Project::ContentTypes::ROM:   ++ws->activities().playedRom;   break;
				default: GBBASIC_ASSERT(false && "Impossible.");                   break;
				}
			}

			df.resolve(true);
		}
	);
}

promise::Promise Operations::projectStop(Window*, Renderer*, Workspace* ws) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			Bytes::Ptr sram = nullptr;
			if (!ws->running()) {
				df.resolve(false, sram);

				return;
			}

			if (ws->categoryBeforeCompiling() != Workspace::Categories::EMULATOR)
				ws->category(ws->categoryBeforeCompiling());
			ws->tabsWidth(0.0f);

			const bool traceless = ws->canvasDevice()->traceless();
			sram = Bytes::Ptr(Bytes::create());
			ws->canvasDevice()->close(sram);
			ws->canvasDevice(nullptr);
			ws->canvasCartridgeStatusText().clear();
			ws->canvasDeviceStatusText().clear();
			ws->canvasStatusTooltip().clear();
			ws->canvasCursorMode(Device::CursorTypes::POINTER);

			if (ws->canvasTexture())
				ws->canvasTexture(nullptr);
			if (ws->canvasTextureForBorderFrame())
				ws->canvasTextureForBorderFrame(nullptr);

			if (!traceless)
				ws->print("End running.");

			const Project::Ptr &prj = ws->currentProject();

			const long long now = DateTime::ticks();
			const long long diff = now - ws->activities().playingStartTimestamp;
			const double secs = DateTime::toSeconds(diff);
			const long long isecs = (long long)secs;
			fprintf(stdout, "Played for %gs.\n", secs);
			ws->activities().playingStartTimestamp = 0;

			ws->activities().playedTime += isecs;
			switch (prj->contentType()) {
			case Project::ContentTypes::BASIC: ws->activities().playedTimeBasic += isecs; break;
			case Project::ContentTypes::ROM:   ws->activities().playedTimeRom += isecs;   break;
			default: GBBASIC_ASSERT(false && "Impossible.");                              break;
			}

			df.resolve(true, sram);
		}
	);
}

promise::Promise Operations::projectReload(Window* wnd, Renderer* rnd, Workspace* ws, Project::Ptr &prj, const char* fontConfigPath) {
	const std::string fontConfigPath_ = fontConfigPath;
	const std::string path = prj->path();
	const Workspace::Categories cat = ws->category();
	const int page = ws->currentAssetPage();

	auto next = [wnd, rnd, ws, prj, fontConfigPath_, path, cat, page] (promise::Defer df) -> void {
		Project::Ptr prj_ = prj;
		fileOpen(wnd, rnd, ws, prj_, true, fontConfigPath_.c_str())
			.then(
				[wnd, rnd, ws, df, path, cat, page] (bool arg) -> void {
					ws->category(cat);
					ws->changePage(wnd, rnd, ws->currentProject().get(), cat, page);

					std::string msg = "Reloaded project file: \"";
					msg += path;
					msg += "\".";
					ws->print(msg.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ReloadedProject(), nullptr);

					df.resolve(arg);
				}
			)
			.fail(
				[df] (void) -> void {
					df.reject();
				}
			);
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Reloading().c_str())
				.then(
					[wnd, rnd, ws, next, df] (void) -> void {
						ws->skipFrame(3);

						fileClose(wnd, rnd, ws)
							.then(
								[next, df] (void) -> promise::Promise {
									return promise::newPromise(std::bind(next, df));
								}
							)
							.fail(
								[df] (void) -> void {
									df.reject();
								}
							);
					}
				);
		}
	);
}

promise::Promise Operations::projectLoadSram(Window*, Renderer*, Workspace* ws, const Project::Ptr &prj, Bytes::Ptr sram) {
	sram->clear();

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (!prj) {
				df.reject();

				return;
			}

			const std::string sramType = prj->sramType();
			if (sramType.empty() || sramType == "0") {
				df.resolve(true);

				fprintf(stdout, "Ignore SRAM loading.\n");

				return;
			}

			const std::string path = prj->sramPath(ws);
			if (path.empty()) {
				df.resolve(false);

				const std::string msg = "No SRAM file.";
				ws->print(msg.c_str());

				return;
			}

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

			if (!Path::fileExists(path.c_str())) {
				df.resolve(false);

				const std::string msg = "No SRAM file found at \"" + path + "\".";
				ws->print(msg.c_str());

				return;
			}

			File::Ptr file(File::create());
			if (!file->open(path.c_str(), Stream::READ)) {
				df.resolve(false);

				const std::string msg = "Cannot load SRAM at \"" + path + "\".";
				ws->warn(msg.c_str());

				return;
			}
			file->readBytes(sram.get());
			file->close();

			const std::string msg = "SRAM loaded at \"" + path + "\".";
			ws->print(msg.c_str());

			df.resolve(true);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long end = DateTime::ticks();
			const long long diff = end - start;
			const double secs = DateTime::toSeconds(diff);
			fprintf(stdout, "SRAM loaded in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
		}
	);
}

promise::Promise Operations::projectSaveSram(Window* wnd, Renderer* rnd, Workspace* ws, const Project::Ptr &prj, const Bytes::Ptr sram, bool immediate) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (!prj) {
				df.reject();

				return;
			}

			const std::string sramType = prj->sramType();
			if (sramType.empty() || sramType == "0") {
				df.resolve(true);

				fprintf(stdout, "Ignore SRAM saving.\n");

				return;
			}

			const std::string path = prj->sramPath(ws);
			if (path.empty()) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveSramState().c_str());

				return;
			}
			if (!ws->canWriteSramTo(path.c_str())) {
				df.reject();

				popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations().c_str());

				return;
			}

			auto next = [wnd, rnd, ws, sram, path] (promise::Defer df) -> void {
#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

				File::Ptr file(File::create());
				if (!file->open(path.c_str(), Stream::WRITE)) {
					df.reject();

					popupMessage(wnd, rnd, ws, ws->theme()->dialogPrompt_CannotSaveSramState().c_str());

					return;
				}
				file->writeBytes(sram.get());
				file->close();

#if defined GBBASIC_OS_HTML
				EM_ASM({
					FS.syncfs(
						(err) => {
							if (err)
								Module.printErr(err);

							Module.print("Filesystem synced.");
						}
					);
				});
#endif /* GBBASIC_OS_HTML */

				const std::string msg = "SRAM saved to \"" + path + "\".";
				ws->print(msg.c_str());

				df.resolve(true);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
				const long long end = DateTime::ticks();
				const long long diff = end - start;
				const double secs = DateTime::toSeconds(diff);
				fprintf(stdout, "SRAM saved in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
			};

			if (immediate) {
				always(wnd, rnd, ws, true)
					.then(
						[next, df] (void) -> promise::Promise {
							return promise::newPromise(std::bind(next, df));
						}
					);
			} else {
				popupWait(wnd, rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
					.then(
						[next, df] (void) -> promise::Promise {
							return promise::newPromise(std::bind(next, df));
						}
					);
			}
		}
	);
}

promise::Promise Operations::projectRemoveSram(Window*, Renderer*, Workspace* ws, const Project::Ptr &prj, bool toTrashBin) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			if (!prj) {
				df.reject();

				return;
			}

			const std::string path = prj->sramPath(ws);
			if (path.empty()) {
				df.resolve(false);

				return;
			}

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long start = DateTime::ticks();
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */

			Path::removeFile(path.c_str(), toTrashBin);

			df.resolve(true);

#if defined OPERATIONS_GBBASIC_TIME_STAT_ENABLED
			const long long end = DateTime::ticks();
			const long long diff = end - start;
			const double secs = DateTime::toSeconds(diff);
			fprintf(stdout, "SRAM removed in %gs.\n", secs);
#endif /* OPERATIONS_GBBASIC_TIME_STAT_ENABLED */
		}
	);
}

/* ===========================================================================} */
