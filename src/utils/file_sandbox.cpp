/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "file_sandbox.h"
#include "filesystem.h"
#if defined GBBASIC_OS_HTML
#	include <emscripten.h>
#	include <emscripten/bind.h>
#endif /* GBBASIC_OS_HTML */

/*
** {===========================================================================
** Utilities
*/

#if defined GBBASIC_OS_HTML
EM_JS(
	void, fileSandboxFree, (void* ptr), {
		_free(ptr);
	}
);
static void fileSandboxWait(int ms) {
	emscripten_sleep((unsigned)ms);
}
#endif /* GBBASIC_OS_HTML */

/* ===========================================================================} */

/*
** {===========================================================================
** File monitor
*/

#if defined GBBASIC_OS_HTML
EM_JS(
	void, fileMonitorInitializeJs, (), {
		if (!FS.trackingDelegate) {
			console.warn('FS debug is disabled.');

			return;
		}

		Module.__GBBASIC_FILE_PROTECTIONS__ = { };
		Module.fileMonitorProtect = ({ path }) => {
			let n = 0;
			if (path in Module.__GBBASIC_FILE_PROTECTIONS__)
				n = Module.__GBBASIC_FILE_PROTECTIONS__[path].count;
			const entry = {
				path,
				count: n + 1
			};
			Module.__GBBASIC_FILE_PROTECTIONS__[path] = entry;

			return entry;
		};
		Module.fileMonitorUnprotect = ({ path }) => {
			if (!(path in Module.__GBBASIC_FILE_PROTECTIONS__))
				return null;

			const entry = Module.__GBBASIC_FILE_PROTECTIONS__[path];
			delete Module.__GBBASIC_FILE_PROTECTIONS__[path];

			return entry;
		};
		Module.fileMonitorIsProtected = ({ path }) => {
			return path in Module.__GBBASIC_FILE_PROTECTIONS__;
		};

		let seed = 1;
		Module.__GBBASIC_FILE_MONITORS__ = { };
		Module.fileMonitorOnOpen = ({ handle, path, handler }) => {
			let id = 0;
			do { id = seed++; } while (id == 0);
			const entry = {
				id,
				handle,
				event: 'open',
				path,
				flags: undefined,
				handler
			};
			Module.__GBBASIC_FILE_MONITORS__[id] = entry;

			return entry;
		};
		Module.fileMonitorOnClose = ({ handle, path, handler }) => {
			let id = 0;
			do { id = seed++; } while (id == 0);
			const entry = {
				id,
				handle,
				event: 'close',
				path,
				flags: undefined,
				handler
			};
			Module.__GBBASIC_FILE_MONITORS__[id] = entry;

			return entry;
		};
		Module.removeFileMonitor = (cond) => {
			const matchedIds = [ ];
			for (const id in Module.__GBBASIC_FILE_MONITORS__) {
				const entry = Module.__GBBASIC_FILE_MONITORS__[id];
				if (cond(entry))
					matchedIds.push(entry.id);
			}
			for (const id of matchedIds) {
				delete Module.__GBBASIC_FILE_MONITORS__[id];
			}
		};
		const isPathMatch = (entry, path) => {
			return entry.path == path; // || path.startsWith(`${entry.path}.`);
		};
		FS.trackingDelegate['onOpenFile'] = (path, flags) => {
			const matchedIds = [ ];
			for (const id in Module.__GBBASIC_FILE_MONITORS__) {
				const entry = Module.__GBBASIC_FILE_MONITORS__[id];
				if (entry.event != 'open')
					continue;
				if (isPathMatch(entry, path))
					matchedIds.push(entry.id);
			}
			for (const id of matchedIds) {
				const entry = Module.__GBBASIC_FILE_MONITORS__[id];
				delete Module.__GBBASIC_FILE_MONITORS__[id];
				entry.handler({ ...entry, flags });
			}
		};
		FS.trackingDelegate['onCloseFile'] = (path) => {
			const matchedIds = [ ];
			for (const id in Module.__GBBASIC_FILE_MONITORS__) {
				const entry = Module.__GBBASIC_FILE_MONITORS__[id];
				if (entry.event != 'close')
					continue;
				if (isPathMatch(entry, path))
					matchedIds.push(entry.id);
			}
			for (const id of matchedIds) {
				const entry = Module.__GBBASIC_FILE_MONITORS__[id];
				delete Module.__GBBASIC_FILE_MONITORS__[id];
				entry.handler(entry);
			}
		};
		FS.trackingDelegate['onReadFile'] = (path, bytesRead) => {
			for (const id in Module.__GBBASIC_FILE_MONITORS__) {
				const entry = Module.__GBBASIC_FILE_MONITORS__[id];
				if (entry.flags == undefined) { // Has not been accessed yet.
					if (isPathMatch(entry, path))
						entry.flags = false; // For read.
				}
			}
		};
		FS.trackingDelegate['onWriteToFile'] = (path, bytesWritten) => {
			for (const id in Module.__GBBASIC_FILE_MONITORS__) {
				const entry = Module.__GBBASIC_FILE_MONITORS__[id];
				if (entry.flags == undefined || entry.flags == false) { // Has not been accessed or has been read.
					if (isPathMatch(entry, path))
						entry.flags = true; // For write.
				}
			}
		};
	}
);
EM_JS(
	void, fileMonitorDisposeJs, (), {
		if (!FS.trackingDelegate)
			return;

		Module.fileMonitorProtect = null;
		Module.fileMonitorUnprotect = null;
		Module.fileMonitorIsProtected = null;

		Module.fileMonitorOnOpen = null;
		Module.fileMonitorOnClose = null;
		Module.removeFileMonitor = null;

		Module.__GBBASIC_FILE_MONITORS__ = null;
		Module.__GBBASIC_FILE_PROTECTIONS__ = null;

		delete FS.trackingDelegate['onOpenFile'];
		delete FS.trackingDelegate['onCloseFile'];
		delete FS.trackingDelegate['onReadFile'];
		delete FS.trackingDelegate['onWriteToFile'];
	}
);
EM_JS(
	void, fileMonitorRemoveByHandleJs, (unsigned handle), {
		Module.removeFileMonitor((entry) => {
			return entry.handle == handle;
		});
	}
);

EM_JS(
	void, unuseFileJs, (const char* path), {
		const path_ = UTF8ToString(path);
		if (Module.fileMonitorIsProtected(path_)) {
			console.log(`[JS WEB] Ignored unusing web file "${path_}", which is protected.`);

			return;
		}

		// setTimeout(() => {
			try {
				FS.unlink(path_); // As "/Dir/File Name.ext".
				console.log(`[JS WEB] Unlinked web file "${path_}".`);
			} catch (_) { }
		// }, 1);
	}
);

void FileMonitor::initialize(void) {
	fileMonitorInitializeJs();

	fprintf(stdout, "File monitor initialized.\n");
}

void FileMonitor::dispose(void) {
	fileMonitorDisposeJs();

	fprintf(stdout, "File monitor disposed.\n");
}

void FileMonitor::unuse(const char* path) {
	if (!path)
		return;

	if (!Text::startsWith(path, "/Documents/", false))
		return;

	if (Path::fileExists(path))
		unuseFileJs(path);
}

void FileMonitor::unuse(const std::string &path) {
	if (!Text::startsWith(path, "/Documents/", false))
		return;

	if (Path::fileExists(path.c_str()))
		unuseFileJs(path.c_str());
}

void FileMonitor::dump(void) {
	DirectoryInfo::Ptr di = DirectoryInfo::make("/Documents");
	FileInfos::Ptr fileInfos = di->getFiles("*;*.*", true);
	if (fileInfos->count() == 0) {
		fprintf(stdout, "[JS WEB] No omissive local file found in MEMFS.\n");
	} else {
		IEnumerator::Ptr enumerator = fileInfos->enumerate();
		while (enumerator->next()) {
			Variant::Pair pair = enumerator->current();
			Object::Ptr val = (Object::Ptr)pair.second;
			if (!val)
				continue;
			FileInfo::Ptr fileInfo = Object::as<FileInfo::Ptr>(val);
			if (!fileInfo)
				continue;

			const std::string &filePath = fileInfo->fullPath();
			fprintf(stdout, "[JS WEB] Found local file \"%s\" in MEMFS.\n", filePath.c_str());
		}
	}

	EM_ASM({
		Object.entries(Module.__GBBASIC_FILE_PROTECTIONS__ || { }).forEach(([ key, entry ]) => {
			console.log(`[JS WEB] Found local protected file "${key}".`);
		});

		Object.entries(Module.__GBBASIC_FILE_MONITORS__ || { }).forEach(([ key, entry ]) => {
			console.log(`[JS WEB] Found local file monitor of "${entry.path}", id ${key}.`);
		});
	});
}
#else /* Platform macro. */
void FileMonitor::initialize(void) {
	// Do nothing.
}

void FileMonitor::dispose(void) {
	// Do nothing.
}

void FileMonitor::unuse(const char* path) {
	(void)path;

	// Do nothing.
}

void FileMonitor::unuse(const std::string &path) {
	(void)path;

	// Do nothing.
}

void FileMonitor::dump(void) {
	// Do nothing.
}
#endif /* Platform macro. */

/* ===========================================================================} */

/*
** {===========================================================================
** Open file in JavaScript
*/

#if defined GBBASIC_OS_HTML
static std::string fileDialogOpenDefaultPath;
static bool fileDialogOpenFileShown = false;
static Text::Array* fileDialogOpenFileResult = nullptr;

extern "C" {

EMSCRIPTEN_KEEPALIVE int onOpenFileLoaded(const char* name, uint8_t* buffer, size_t size) {
	// if (fileDialogOpenDefaultPath.empty())
	fileDialogOpenDefaultPath = "transfer.tmp";
	if (name && *name != '\0')
		fileDialogOpenDefaultPath = name;

	const std::string path = Path::combine("/Documents", fileDialogOpenDefaultPath.c_str());
	File::Ptr file(File::create());
	if (!file->open(path.c_str(), Stream::WRITE)) {
		fileDialogOpenFileShown = false;

		return -1;
	}
	file->writeBytes((Byte*)buffer, size); // Write from local harddrive to MEMFS.
	file->close();

	fileDialogOpenFileResult->push_back(path); // As "/Dir/File Name.ext".
	fileDialogOpenFileShown = false;

	fileDialogOpenDefaultPath.clear();

	return 0;
}
EMSCRIPTEN_KEEPALIVE int onOpenFileCanceled(void) {
	fileDialogOpenFileShown = false;

	return 0;
}

}

EM_JS(
	void, openFileJs, (const char* filters, bool multiSelect), {
		const fileSelector = document.createElement('input');
		fileSelector.setAttribute('type', 'file');
		fileSelector.setAttribute('accept', UTF8ToString(filters));
		if (multiSelect) {
			fileSelector.setAttribute('multiple', '');
		}
		fileSelector.onchange = (evt) => {
			const fileReader = new FileReader();
			const name_ = evt.target.files[0].name;
			fileReader.onload = (event) => { // Loaded from local harddrive.
				const name = stringToNewUTF8(name_);
				const uint8Arr = new Uint8Array(event.target.result);
				console.log(`[JS WEB] Uploaded local file "${name_}" in ${uint8Arr.length} bytes.`);
				const numBytes = uint8Arr.length * uint8Arr.BYTES_PER_ELEMENT;
				const dataPtr = _malloc(numBytes);
				const dataOnHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numBytes);
				dataOnHeap.set(uint8Arr);
				const ret = ccall('onOpenFileLoaded', 'number', [ 'number', 'number', 'number' ], [ name, dataOnHeap.byteOffset, uint8Arr.length ]);
				console.log(`[JS WEB] Synced local file "${name_}" in MEMFS.`);
				_free(dataPtr);
				_free(name);
			};
			fileReader.readAsArrayBuffer(evt.target.files[0]);
		};
		fileSelector.addEventListener('cancel', (evt) => {
			const ret = ccall('onOpenFileCanceled', 'number', [ ], [ ]);
			console.log('[JS WEB] Canceled to open local file.');
		});
		fileSelector.click();
	}
);

static void openFile(const Text::Array &filters, bool multiSelect, Text::Array &ret) {
	std::string filters_;
	for (int i = 0; i < (int)filters.size(); i += 2) {
		std::string f = filters[i + 1];
		f = Text::replace(f, "*", "");
		f = Text::replace(f, " ", ",");
		filters_ += f;
		if (i < (int)filters.size() - 1)
			filters_ += ",";
	}

	fileDialogOpenFileShown = true;
	fileDialogOpenFileResult = &ret;

	openFileJs(filters_.c_str(), multiSelect);

	constexpr const int STEP = 10;
	while (fileDialogOpenFileShown) {
		fileSandboxWait(STEP);
	}

	fileDialogOpenFileResult = nullptr;
}
#endif /* GBBASIC_OS_HTML */

/* ===========================================================================} */

/*
** {===========================================================================
** Save file in JavaScript
*/

#if defined GBBASIC_OS_HTML
EM_JS(
	const char*, saveFileJs, (const char* defaultPath), {
		const defaultPath_ = UTF8ToString(defaultPath);
		const preferedPath = defaultPath_ && defaultPath_.length > 0 ? `/Documents/${defaultPath_}` : '/Documents/data';

		Module.fileMonitorOnClose({ // Begin monitor for file close.
			path: preferedPath, // As "/Dir/File Name.ext".
			handler: ({ path }) => {
				console.log(`[JS WEB] Saved web file "${path}" in MEMFS.`);
				const mime = 'application/octet-stream';
				const content = FS.readFile(path);
				const a = document.createElement('a');
				if (defaultPath_ == '') {
					let fileName = path;
					const i = fileName.lastIndexOf('/');
					if (i >= 0)
						fileName = fileName.substr(i + 1);
					a.download = fileName;
				} else {
					a.download = defaultPath_;
				}
				a.href = URL.createObjectURL(new Blob([ content ], { type: mime })); // Create a download URL.
				a.style.display = 'none';
				document.body.appendChild(a);
				a.click(); // Download.
				console.log(`[JS WEB] Downloaded web file "${path}" to local.`);
				{
					document.body.removeChild(a);
					URL.revokeObjectURL(a.href);
					FS.unlink(path); // Unlink the file on MEMFS.
					console.log(`[JS WEB] Unlinked web file "${path}".`);
				}
			}
		});

		const stringOnWasmHeap = stringToNewUTF8(preferedPath);

		return stringOnWasmHeap;
	}
);

static void saveFile(const std::string &defaultPath, std::string &ret) {
	const char* path = saveFileJs(defaultPath.c_str());
	ret = path;
	fileSandboxFree((void*)path); // Assign the path to save to.
}
#endif /* GBBASIC_OS_HTML */

/* ===========================================================================} */

/*
** {===========================================================================
** File dialog
*/

#if defined GBBASIC_OS_HTML
namespace pfd {

open_file::open_file(
	std::string const &title,
	std::string const &default_path,
	std::vector<std::string> const &filters,
	opt options
) {
	(void)title;
	(void)options;

	fileDialogOpenDefaultPath = default_path;

	openFile(filters, /* options == opt::multiselect */ false, _result);
}

open_file::open_file(
	std::string const &title,
	std::string const &default_path,
	std::vector<std::string> const &filters,
	bool allow_multiselect
) {
	(void)title;
	(void)allow_multiselect;

	fileDialogOpenDefaultPath = default_path;

	openFile(filters, /* allow_multiselect */ false, _result);
}

std::vector<std::string> open_file::result() {
	return _result;
}

save_file::save_file(
	std::string const &title,
	std::string const &default_path,
	std::vector<std::string> const &filters,
	opt options
) {
	(void)title;
	(void)default_path;
	(void)filters;
	(void)options;

	saveFile(default_path, _result);
}

save_file::save_file(
	std::string const &title,
	std::string const &default_path,
	std::vector<std::string> const &filters,
	bool confirm_overwrite
) {
	(void)title;
	(void)default_path;
	(void)filters;
	(void)confirm_overwrite;

	saveFile(default_path, _result);
}

std::string save_file::result() {
	return _result;
}

select_folder::select_folder(
	std::string const &title,
	std::string const &default_path,
	opt options
) {
	(void)title;
	(void)default_path;
	(void)options;

	fprintf(stderr, "Not implemented.\n");
}

std::string select_folder::result() {
	return std::string();
}

notify::notify(
	std::string const &title,
	std::string const &message,
	icon _icon
) {
	(void)title;
	(void)message;
	(void)_icon;

	fprintf(stderr, "Not implemented.\n");
}

}
#endif /* Platform macro. */

/* ===========================================================================} */

/*
** {===========================================================================
** File sync
*/

#if defined GBBASIC_OS_HTML
EM_JS(
	bool, fileSyncSupported, (), {
		if (typeof FileSystemHandle == 'undefined')
			return false;
		if (typeof FileSystemFileHandle == 'undefined')
			return false;
		if (typeof FileSystemWritableFileStream == 'undefined')
			return false;
		if (!window.showOpenFilePicker || !window.showSaveFilePicker)
			return false;

		if (typeof isFileSyncDisabled == 'function') {
			if (isFileSyncDisabled())
				return false;
		}

		if (typeof isInCrossOriginIframe == 'function') {
			if (isInCrossOriginIframe())
				return false;
		}

		return true;
	}
);

static bool fileSyncBusy = false;
static Text::Array* fileSyncRequestResult = nullptr;

extern "C" {

EMSCRIPTEN_KEEPALIVE int onFileRequestResolved(const char* name) {
	const std::string path = Path::combine("/Documents", name);

	fileSyncRequestResult->push_back(path); // As "/Dir/File Name.ext".
	fileSyncBusy = false;

	return 0;
}
EMSCRIPTEN_KEEPALIVE int onFileRequestRejected(void) {
	fileSyncBusy = false;

	return 0;
}

}

EMSCRIPTEN_BINDINGS(FileSyncFilters) {
	emscripten::enum_<FileSync::Filters>("FileSyncFilters")
		.value("PROJECT",   FileSync::Filters::PROJECT)
		.value("CODE",      FileSync::Filters::CODE)
		.value("C",         FileSync::Filters::C)
		.value("JSON",      FileSync::Filters::JSON)
		.value("IMAGE",     FileSync::Filters::IMAGE)
		.value("FONT",      FileSync::Filters::FONT)
		.value("VGM",       FileSync::Filters::VGM)
		.value("WAV",       FileSync::Filters::WAV)
		.value("FX_HAMMER", FileSync::Filters::FX_HAMMER)
		.value("ANY",       FileSync::Filters::ANY)
	;
};

EM_JS(
	void, requestFileHandleJs, (unsigned handle, unsigned filters, bool save), {
		let fileHandle = null;
		let options = null;
		switch (filters) {
		case Module.FileSyncFilters.PROJECT.value:
			options = {
				types: [
					{
						description: 'Project files',
						accept: {
							'text/plain': [ '.gbb', '.bas' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.CODE.value:
			options = {
				types: [
					{
						description: 'Code files',
						accept: {
							'text/plain': [ '.bas', '.gbb', '.txt' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.C.value:
			options = {
				types: [
					{
						description: 'C files',
						accept: {
							'text/plain': [ '.c', '.h' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.JSON.value:
			options = {
				types: [
					{
						description: 'JSON files',
						accept: {
							'application/json': [ '.json' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.IMAGE.value:
			options = {
				types: [
					{
						description: 'Image files',
						accept: {
							'image/*': [ '.png', '.jpg', '.bmp', '.tga' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.FONT.value:
			options = {
				types: [
					{
						description: 'Font/image files',
						accept: {
							'image/*': [ '.ttf', '.png', '.jpg', '.bmp', '.tga' ]
						}
					},
					{
						description: 'Font files',
						accept: {
							'font/ttf': [ '.ttf' ]
						}
					},
						{
						description: 'Image files',
						accept: {
							'image/*': [ '.png', '.jpg', '.bmp', '.tga' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.VGM.value:
			options = {
				types: [
					{
						description: 'VGM files',
						accept: {
							'application/octet-stream': [ '.vgm' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.WAV.value:
			options = {
				types: [
					{
						description: 'WAV files',
						accept: {
							'audio/wav': [ '.wav' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.FX_HAMMER.value:
			options = {
				types: [
					{
						description: 'FxHammer files',
						accept: {
							'application/octet-stream': [ '.sav' ]
						}
					}
				]
			};

			break;
		case Module.FileSyncFilters.ANY.value: // Fall through.
		default:
			options = {
				types: [ ]
			};

			break;
		}
		const picker = save ? window.showSaveFilePicker : window.showOpenFilePicker;
		picker(options)
			.then((fileHandles) => {
				if (Array.isArray(fileHandles))
					[fileHandle] = fileHandles;
				else
					fileHandle = fileHandles;
				if (save)
					console.log(`[JS HDD] Requested local file "${fileHandle.name}" for save.`);
				else
					console.log(`[JS HDD] Requested local file "${fileHandle.name}" for open.`);
				if (!Module.__GBBASIC_FILE_REQUESTS__)
					Module.__GBBASIC_FILE_REQUESTS__ = { };
				Module.__GBBASIC_FILE_REQUESTS__[handle] = fileHandle;
				const fileName = stringToNewUTF8(fileHandle.name); // As "File Name.ext".
				const ret = ccall('onFileRequestResolved', 'number', [ 'number' ], [ fileName ]); // As "/Dir/File Name.ext".
				console.log(`[JS HDD] Synced local file path "${fileHandle.name}".`);
				_free(fileName);
			})
			.catch((err) => {
				fileHandle = -1;
				const ret = ccall('onFileRequestRejected', 'number', [ ], [ ]);
				if (err.name == 'AbortError')
					console.log('[JS HDD] Canceled to request file due to user abort.');
				else
					console.warn('[JS HDD] Failed to request file.', err);
			});
	}
);
EM_JS(
	void, releaseFileHandleJs, (unsigned handle), {
		if (!Module.__GBBASIC_FILE_REQUESTS__)
			return;

		if (handle in Module.__GBBASIC_FILE_REQUESTS__)
			delete Module.__GBBASIC_FILE_REQUESTS__[handle];
	}
);

extern "C" {

EMSCRIPTEN_KEEPALIVE int onSyncFileForOpenLoaded(const char* name, uint8_t* buffer, size_t size) {
	const std::string path = Path::combine("/Documents", name);
	File::Ptr file(File::create());
	if (!file->open(path.c_str(), Stream::WRITE)) {
		fileSyncBusy = false;

		return -1;
	}
	file->writeBytes((Byte*)buffer, size);
	file->close();

	fileSyncBusy = false;

	return 0;
}

}

EM_JS(
	void, monitorFileJs, (unsigned handle, const char* fullPath_), {
		const fileHandle = Module.__GBBASIC_FILE_REQUESTS__[handle];
		const fullPath = UTF8ToString(fullPath_); // As "/Dir/File Name.ext".

		let onOpen = null;
		onOpen = () => {
			Module.fileMonitorOnOpen({
				handle,
				path: fullPath,
				handler: async ({ path, flags }) => {
					console.log(`[JS HDD] Be awared of web file "${path}" opened in MEMFS.`);
					Module.fileMonitorProtect({ path });
					console.log(`[JS HDD] Retain web file "${path}" in MEMFS.`);

					onOpen();
				}
			});
		};

		let onClose = null;
		onClose = () => {
			Module.fileMonitorOnClose({
				handle,
				path: fullPath,
				handler: async ({ path, flags }) => {
					if (flags) {
						console.log(`[JS HDD] Be awared of web file "${path}" closed after writing in MEMFS.`);
						const mime = 'application/octet-stream';
						const content = FS.readFile(path); // As "/Dir/File Name.ext".
						const blob = new Blob([ content ], { type: mime });
						try {
							const stream = await fileHandle.createWritable();
							await stream.write(blob);
							await stream.close();
						} catch (_) {
							if (typeof showTips == 'function') {
								showTips({
									content: 'No permission to write to local file or user denied.',
									type: 'error'
								});
							}
						}
						{
							FS.unlink(path); // As "/Dir/File Name.ext".
							console.log(`[JS HDD] Unlinked web file "${path}".`);
						}
						console.log(`[JS HDD] Synced web file "${path}" to local.`);
					} else {
						console.log(`[JS HDD] Be awared of web file "${path}" closed in MEMFS.`);
					}

					Module.fileMonitorUnprotect({ path });
					console.log(`[JS HDD] Release web file "${path}" in MEMFS.`);

					onClose();
				}
			});
		};

		onOpen();
		onClose();
	}
);

extern "C" {

EMSCRIPTEN_KEEPALIVE int monitorFile(unsigned handle, const char* fullPath_) {
	monitorFileJs(handle, fullPath_);

	return 0;
}

}

EM_JS(
	void, syncFileForOpenJs, (unsigned handle), {
		const fileHandle = Module.__GBBASIC_FILE_REQUESTS__[handle];
		setTimeout(async () => {
			const name = stringToNewUTF8(fileHandle.name);
			const file = await fileHandle.getFile();
			const buffer = await file.arrayBuffer();
			const uint8Arr = new Uint8Array(buffer);
			console.log(`[JS HDD] Loaded local file "${fileHandle.name}" for open in ${uint8Arr.length} bytes.`);
			const numBytes = uint8Arr.length * uint8Arr.BYTES_PER_ELEMENT;
			const dataPtr = _malloc(numBytes);
			const dataOnHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numBytes);
			dataOnHeap.set(uint8Arr);
			const ret = ccall('onSyncFileForOpenLoaded', 'number', [ 'number', 'number', 'number' ], [ name, dataOnHeap.byteOffset, uint8Arr.length ]);
			console.log(`[JS HDD] Synced local file "${fileHandle.name}" for open in MEMFS.`);
			_free(dataPtr);
			_free(name);

			const fullPath = `/Documents/${fileHandle.name}`; // As "/Dir/File Name.ext".
			const fullPath_ = stringToNewUTF8(fullPath);
			ccall('monitorFile', 'number', [ 'number', 'number' ], [ handle, fullPath_ ]);
			_free(fullPath_);
		}, 0);
	}
);
EM_JS(
	void, syncFileForSaveJs, (unsigned handle, const char* overriddenPath), {
		const fullPath_ = overriddenPath; // As "/Dir/File Name.ext".
		ccall('monitorFile', 'number', [ 'number', 'number' ], [ handle, fullPath_ ]);
	}
);
#else /* Platform macro. */
static bool fileSyncSupported(void) {
	return true;
}
#endif /* Platform macro. */
static Text::Array fileSyncGetFilters(FileSync::Filters y) {
	Text::Array filters;
	switch (y) {
	case FileSync::Filters::PROJECT:
		filters = GBBASIC_PROJECT_FILE_FILTER;

		break;
	case FileSync::Filters::ANY: // Fall through.
	default:
		filters = { "All files (*.*)", "*" };

		break;
	}

	return filters;
}

static unsigned fileSyncHandleSeed = 1;

class FileSyncImpl : public FileSync {
public:
	FileSyncImpl() {
		filters(Filters::ANY);

		access(Stream::Accesses::READ);
		handle(0);
	}
	virtual ~FileSyncImpl() override {
		dispose();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void initialize(const std::string &defaultPath_, Filters filters_) override {
		dispose();

		defaultPath(defaultPath_);
		filters(filters_);
	}
	virtual void dispose(void) override {
		close();

		defaultPath().clear();
		filters(Filters::ANY);

		path("");
		access(Stream::Accesses::READ);
		if (handle()) {
#if defined GBBASIC_OS_HTML
			fileMonitorRemoveByHandleJs(handle());
			if (supported())
				releaseFileHandleJs(handle()); // Release handle.
#endif /* Platform macro. */
			handle(0);
		}
	}

	virtual bool supported(void) const override {
		return fileSyncSupported();
	}

	virtual bool requestOpen(void) override {
		if (handle())
			return false;

#if defined GBBASIC_OS_HTML
		if (supported()) {
			// Request file handle.
			if (path().empty()) {
				Text::Array ret;

				fileSyncBusy = true;
				fileSyncRequestResult = &ret;
				{
					unsigned handle_ = 0;
					do {
						handle_ = fileSyncHandleSeed++;
					} while (handle_ == 0);
					requestFileHandleJs(handle_, (unsigned)filters(), false);

					constexpr const int STEP = 10;
					while (fileSyncBusy) {
						fileSandboxWait(STEP);
					}

					handle(handle_); // Retain handle.
				}
				fileSyncRequestResult = nullptr;

				if (ret.empty()) {
					releaseFileHandleJs(handle()); // Release handle.
					handle(0);

					return false;
				}

				std::string path_ = ret.front(); // As "/Dir/File Name.ext".
				if (path_.empty()) {
					releaseFileHandleJs(handle()); // Release handle.
					handle(0);

					return false;
				}
				Path::uniform(path_);

				path(path_); // As "/Dir/File Name.ext".
			}

			// Sync for open.
			do {
				fileSyncBusy = true;
				{
					syncFileForOpenJs(handle()); // Sync file for open.

					constexpr const int STEP = 10;
					while (fileSyncBusy) {
						fileSandboxWait(STEP);
					}
				}
			} while (false);
		} else {
			if (path().empty()) {
				pfd::open_file open(
					GBBASIC_TITLE,
					defaultPath(),
					fileSyncGetFilters(filters()),
					pfd::opt::none
				);
				if (open.result().size() != 1)
					return false;

				std::string path_ = open.result().front();
				if (path_.empty())
					return false;
				Path::uniform(path_);

				unsigned handle_ = 0;
				do {
					handle_ = fileSyncHandleSeed++;
				} while (handle_ == 0);
				handle(handle_); // Retain handle.

				path(path_);
			}
		}
#else /* Platform macro. */
		if (path().empty()) {
			pfd::open_file open(
				GBBASIC_TITLE,
				defaultPath(),
				fileSyncGetFilters(filters()),
				pfd::opt::none
			);
			if (open.result().size() != 1)
				return false;

			std::string path_ = open.result().front();
			if (path_.empty())
				return false;
			Path::uniform(path_);

			unsigned handle_ = 0;
			do {
				handle_ = fileSyncHandleSeed++;
			} while (handle_ == 0);
			handle(handle_); // Retain handle.

			path(path_);
		}
#endif /* Platform macro. */

		if (path().empty())
			return false;

		access(Stream::READ);

		return true;
	}
	virtual bool requestSave(bool confirmOverwrite) override {
		if (pointer())
			return false;

#if defined GBBASIC_OS_HTML
		if (supported()) {
			// Request file handle.
			if (path().empty()) {
				Text::Array ret;

				fileSyncBusy = true;
				fileSyncRequestResult = &ret;
				{
					unsigned handle_ = 0;
					do {
						handle_ = fileSyncHandleSeed++;
					} while (handle_ == 0);
					requestFileHandleJs(handle_, (unsigned)filters(), true);

					constexpr const int STEP = 10;
					while (fileSyncBusy) {
						fileSandboxWait(STEP);
					}

					handle(handle_); // Retain handle.
				}
				fileSyncRequestResult = nullptr;

				if (ret.empty()) {
					releaseFileHandleJs(handle()); // Release handle.
					handle(0);

					return false;
				}

				std::string path_ = ret.front(); // As "/Dir/File Name.ext".
				if (path_.empty()) {
					releaseFileHandleJs(handle()); // Release handle.
					handle(0);

					return false;
				}
				Path::uniform(path_);

				fullfillPath(path_);

				path(path_); // As "/Dir/File Name.ext".
			}

			// Sync for save.
			syncFileForSaveJs(handle(), path().c_str()); // Sync file for save.
		} else {
			if (path().empty()) {
				pfd::save_file save(
					GBBASIC_TITLE,
					Text::sanitizeFilename(defaultPath()),
					fileSyncGetFilters(filters()),
					confirmOverwrite ? pfd::opt::none : pfd::opt::force_overwrite
				);
				std::string path_ = save.result();
				Path::uniform(path_);
				if (path_.empty())
					return false;

				unsigned handle_ = 0;
				do {
					handle_ = fileSyncHandleSeed++;
				} while (handle_ == 0);
				handle(handle_); // Retain handle.

				fullfillPath(path_);

				path(path_);
			}
		}
#else /* Platform macro. */
		if (path().empty()) {
			pfd::save_file save(
				GBBASIC_TITLE,
				Text::sanitizeFilename(defaultPath()),
				fileSyncGetFilters(filters()),
				confirmOverwrite ? pfd::opt::none : pfd::opt::force_overwrite
			);
			std::string path_ = save.result();
			Path::uniform(path_);
			if (path_.empty())
				return false;

			unsigned handle_ = 0;
			do {
				handle_ = fileSyncHandleSeed++;
			} while (handle_ == 0);
			handle(handle_); // Retain handle.

			fullfillPath(path_);

			path(path_);
		}
#endif /* Platform macro. */

		if (path().empty())
			return false;

		access(Stream::WRITE);

		return true;
	}

	virtual File::Ptr open(void) override {
		if (pointer())
			return nullptr;

		if (path().empty())
			return nullptr;

		File::Ptr file(File::create());
		if (!file->open(path().c_str(), access()))
			return nullptr;

		pointer(file);

		return pointer();
	}
	virtual bool close(void) override {
		if (!pointer())
			return false;

		if (!pointer()->close())
			return false;

		pointer(nullptr);

		FileMonitor::unuse(path());

		return true;
	}

private:
	bool fullfillPath(std::string &path) const {
		switch (filters()) {
		case FileSync::Filters::PROJECT: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || ext != GBBASIC_RICH_PROJECT_EXT) {
					path += "." GBBASIC_RICH_PROJECT_EXT;

					return true;
				}
			}

			break;
		case FileSync::Filters::CODE: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || (ext != "bas" && ext != "gbb")) {
					path += ".bas";

					return true;
				}
			}

			break;
		case FileSync::Filters::C: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || (ext != "c" && ext != "h")) {
					path += ".c";

					return true;
				}
			}

			break;
		case FileSync::Filters::JSON: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || ext != "json") {
					path += ".json";

					return true;
				}
			}

			break;
		case FileSync::Filters::IMAGE: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || (ext != "png" && ext != "jpg" && ext != "bmp" && ext != "tga")) {
					path += ".png";

					return true;
				}
			}

			break;
		case FileSync::Filters::FONT: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || ext != "ttf") {
					path += ".ttf";

					return true;
				}
			}

			break;
		case FileSync::Filters::VGM: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || ext != "vgm") {
					path += ".vgm";

					return true;
				}
			}

			break;
		case FileSync::Filters::WAV: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || ext != "wav") {
					path += ".wav";

					return true;
				}
			}

			break;
		case FileSync::Filters::FX_HAMMER: {
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				Text::toLowerCase(ext);
				if (ext.empty() || ext != "sav") {
					path += ".sav";

					return true;
				}
			}

			break;
		case FileSync::Filters::ANY: // Fall through.
		default:
			// Do nothing.

			break;
		}

		return false;
	}
};

FileSync* FileSync::create(void) {
	FileSyncImpl* result = new FileSyncImpl();

	return result;
}

void FileSync::destroy(FileSync* ptr) {
	FileSyncImpl* impl = static_cast<FileSyncImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
