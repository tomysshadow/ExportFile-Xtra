#pragma once
#define _WIN32_WINNT 0x0501
#include "scope_guard.hpp"
#include "MoaIDHash.h"
#include "MappedVector.h"
#include "IgnoreCaseComparer.h"
#include <memory>
#include <stdexcept>
#include <utility>
#include <string>
#include <variant>
#include <filesystem>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "mmtypes.h"
#include "moaxtra.h"
#include "moastdif.h"
#include "mmixscrp.h"

#pragma warning(push)
#pragma warning(disable : 5040)
#include "xmmvalue.h"
#pragma warning(pop)

#include "moafile.h"

#ifdef WINDOWS
#include <windows.h>
#include <cguid.h>
#include <atlbase.h>
#include <atlconv.h>
#endif

#include "resource.h"

typedef uint32_t CHUNK_ID;

#ifdef MACINTOSH
typedef ResID RESOURCE_ID;
#endif
#ifdef WINDOWS
typedef WORD RESOURCE_ID;
#endif

typedef std::variant<std::string, MoaMmSymbol> SYMBOL_VARIANT;

#ifdef WINDOWS
typedef std::vector<HANDLE> HANDLE_VECTOR;
typedef std::vector<HMODULE> MODULE_HANDLE_VECTOR;
#endif

//#define kWriterRegKey_AgentRegDict MOADICT_RUNTIME_KEY_PREFIX "%%AgentRegDict%%"

inline bool stringNullOrEmpty(const char* str) {
	return !str || !*str;
}

inline bool stringWhitespace(const char* str) {
	unsigned char space = *str;

	while (space && isspace(space)) {
		space = *++str;
	}
	return !space;
}

inline bool stringWhitespaceWide(const wchar_t* str) {
	wchar_t space = *str;

	while (space && iswspace(space)) {
		space = *++str;
	}
	return !space;
}

inline size_t stringSize(const char* str) {
	return strlen(str) + 1;
}

inline size_t stringSizeWide(const wchar_t* str) {
	return wcslen(str) + 1;
}

inline size_t stringSizeMax(const char* str, size_t sizeMax) {
	return strnlen_s(str, sizeMax) + 1;
}

inline size_t stringSizeMaxWide(const wchar_t* str, size_t sizeMax) {
	return wcsnlen_s(str, sizeMax) + 1;
}

inline bool stringTruncated(const char* str, size_t size) {
	return size <= strnlen_s(str, size) + 1;
}

inline bool stringTruncatedWide(const wchar_t* str, size_t size) {
	return size <= wcsnlen_s(str, size) + 1;
}

inline bool stringEquals(const char* str, const char* str2) {
	return !strcmp(str, str2);
}

inline bool stringEqualsWide(const wchar_t* str, const wchar_t* str2) {
	return !wcscmp(str, str2);
}

inline bool stringEqualsCaseInsensitive(const char* str, const char* str2) {
	return !_stricmp(str, str2);
}

inline bool stringEqualsCaseInsensitiveWide(const wchar_t* str, const wchar_t* str2) {
	return !_wcsicmp(str, str2);
}

inline bool memoryEquals(const void* mem, const void* mem2, size_t size) {
	return !memcmp(mem, mem2, size);
}

inline bool memoryShift(void* mem, size_t memSize, void* sourceMem, size_t sourceMemSize, size_t shift, bool direction) {
	#pragma warning(push)
	#pragma warning(disable : 4133)
	if (sourceMem < mem || (char*)sourceMem + sourceMemSize > (char*)mem + memSize) {
		return false;
	}

	size_t destinationSize = (char*)mem + memSize - sourceMem;
	char* destination = (char*)sourceMem;

	if (direction) {
		destination += shift;
	} else {
		destination -= shift;
	}

	if (destination < mem || destination + destinationSize > (char*)mem + memSize) {
		return false;
	}
	return !memmove_s(destination, destinationSize, sourceMem, sourceMemSize);
	#pragma warning(pop)
}

#define DIRECTOR_UTF8(productVersionMajor) ((productVersionMajor) >= 11)

#define FILESYSTEM_DIRECTOR_PATH(element, productVersionMajor) (DIRECTOR_UTF8(productVersionMajor) ? std::filesystem::u8path(element) : std::filesystem::path(element))
#define FILESYSTEM_DIRECTOR_STRING(path, productVersionMajor) (DIRECTOR_UTF8(productVersionMajor) ? (path).u8string() : (path).string())

#ifdef MACINTOSH
#define kCFStringEncodingDirector(productVersionMajor) (DIRECTOR_UTF8(productVersionMajor) ? kCFStringEncodingUTF8 : kCFStringEncodingASCII)
#endif
#ifdef WINDOWS
#define CP_DIRECTOR(productVersionMajor) (DIRECTOR_UTF8(productVersionMajor) ? CP_UTF8 : ATL::_AtlGetConversionACP())
#endif

#define RETURN_NULL(pointer) do { if (!(pointer)) { return kMoaErr_OutOfMem; } } while (0)

#define RETURN_NULL_BOOL(pointer) do { if (!(pointer)) { return false; } } while (0)

#define RETURN_ERR(err) do { MoaError _err = (err); if (_err != kMoaErr_NoErr) { return _err; } } while (0)

#define RETURN_ERR_BOOL(err) do { if ((err) != kMoaErr_NoErr) { return false; } } while (0)

inline MoaError errOrDefaultErr(MoaError err, MoaError defaultErr) {
	RETURN_ERR(err);
	return defaultErr;
}

#ifdef MACINTOSH
// I don't know if Mac has a GetLastError() equivalent
inline MoaError osErrOrDefaultErr(MoaError defaultErr) {
	return defaultErr;
}

inline MoaError osErr(long l) {
	if (l) {
		return kMoaErr_NoErr;
	}
	return osErrOrDefaultErr(kMoaErr_OutOfMem);
}

inline MoaError osErr(short n) {
	if (n) {
		return kMoaErr_NoErr;
	}
	return osErrOrDefaultErr(kMoaErr_OutOfMem);
}

inline MoaError osErr(char c) {
	if (c) {
		return kMoaErr_NoErr;
	}
	return osErrOrDefaultErr(kMoaErr_OutOfMem);
}

inline MoaError osErr(Handle h) {
	if (h) {
		return kMoaErr_NoErr;
	}
	return osErrOrDefaultErr(kMoaErr_OutOfMem);
}

inline MoaError fileErr(MoaError err) {
	return IS_ERROR(err) ? err : kMoaFileErr_IoError;
}
#endif
#ifdef WINDOWS
inline MoaError osErrOrDefaultErr(MoaError defaultErr, bool doserrno = false) {
	DWORD lastError = doserrno ? _doserrno : GetLastError();
	return errOrDefaultErr(HRESULT_FROM_WIN32(lastError), defaultErr);
}

inline MoaError osErr(DWORD dw, bool doserrno = false) {
	if (dw) {
		return kMoaErr_NoErr;
	}
	return osErrOrDefaultErr(kMoaErr_OutOfMem, doserrno);
}

inline MoaError osErr(WORD w, bool doserrno = false) {
	return osErr((DWORD)w, doserrno);
}

inline MoaError osErr(BYTE by, bool doserrno = false) {
	return osErr((DWORD)by, doserrno);
}

inline MoaError osErr(BOOL b, bool doserrno = false) {
	return osErr((DWORD)b, doserrno);
}

inline MoaError osErr(HANDLE h, bool doserrno = false) {
	if (h != NULL && h != INVALID_HANDLE_VALUE) {
		return kMoaErr_NoErr;
	}
	return osErrOrDefaultErr(kMoaErr_OutOfMem, doserrno);
}

inline MoaError fileErr(MoaError err) {
	return IS_ERROR(err) ? err : HRESULT_FROM_WIN32(err);
}
#endif

inline MoaUlong getInterfaceRefCount(PIMoaUnknown interfacePointer) {
	if (interfacePointer) {
		interfacePointer->AddRef();
		return interfacePointer->Release();
	}
	return 0;
}

inline bool lastInterfaceRef(PIMoaUnknown interfacePointer) {
	return getInterfaceRefCount(interfacePointer) == 1;
}

inline void releaseInterface(PPMoaVoid interfacePointerPointer) {
	if (!interfacePointerPointer) {
		throw std::invalid_argument("interfacePointerPointer must not be NULL");
	}

	PIMoaUnknown &interfacePointer = *(PIMoaUnknown*)interfacePointerPointer;

	if (!interfacePointer) {
		return;
	}

	interfacePointer->Release();
	interfacePointer = NULL;
}

inline PIMoaUnknown getInterface(PIMoaUnknown interfacePointer) {
	if (interfacePointer) {
		interfacePointer->AddRef();
	}
	return interfacePointer;
}

inline void setInterface(PPMoaVoid interfacePointerPointer, PIMoaUnknown valueInterfacePointer) {
	if (!interfacePointerPointer) {
		throw std::invalid_argument("interfacePointerPointer must not be NULL");
	}

	PIMoaUnknown &interfacePointer = *(PIMoaUnknown*)interfacePointerPointer;

	if (interfacePointer == valueInterfacePointer) {
		return;
	}

	releaseInterface((PPMoaVoid)&interfacePointer);

	if (valueInterfacePointer) {
		interfacePointer = valueInterfacePointer;
		interfacePointer->AddRef();
	}
}

inline void moveInterface(PPMoaVoid interfacePointerPointer, PPMoaVoid valueInterfacePointerPointer) {
	if (!interfacePointerPointer) {
		throw std::invalid_argument("interfacePointerPointer must not be NULL");
	}

	if (!valueInterfacePointerPointer) {
		throw std::invalid_argument("valueInterfacePointerPointer must not be NULL");
	}

	PIMoaUnknown &valueInterfacePointer = *(PIMoaUnknown*)valueInterfacePointerPointer;
	*(PIMoaUnknown*)interfacePointerPointer = valueInterfacePointer;
	valueInterfacePointer = NULL;
}

inline MoaError openStream(MoaLong accessMode, bool setPosition, PIMoaStream streamInterfacePointer) {
	RETURN_NULL(streamInterfacePointer);

	if (setPosition) {
		// for general purpose use this might do a lot of seeking
		// so might as well try for cheap
		MoaError err = streamInterfacePointer->Open(accessMode, kMoaStreamSetPositionType_Cheap);

		if (err == kMoaStreamErr_BadSetPositionMode) {
			// if we can't get cheap then try expensive, it doesn't matter a tonne
			err = streamInterfacePointer->Open(accessMode, kMoaStreamSetPositionType_Expensive);
		}
		return err;
	}
	return streamInterfacePointer->Open(accessMode, kMoaStreamSetPositionType_None);
}

inline MoaError closeStream(PIMoaStream &streamInterfacePointer) {
	if (!streamInterfacePointer) {
		return kMoaErr_NoErr;
	}

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&streamInterfacePointer);
	};

	if (lastInterfaceRef(streamInterfacePointer)) {
		MoaError err = streamInterfacePointer->Close();

		if (err != kMoaStreamErr_StreamNotOpen) {
			return err;
		}
	}
	return kMoaErr_NoErr;
}

inline MoaError cloneFile(PIMoaFile &fileInterfacePointer, PIMoaFile valueFileInterfacePointer) {
	MoaError err = kMoaErr_NoErr;

	if (valueFileInterfacePointer) {
		err = valueFileInterfacePointer->Clone(&valueFileInterfacePointer);

		if (err != kMoaErr_NoErr) {
			valueFileInterfacePointer = NULL;
		}
	}

	setInterface((PPMoaVoid)&fileInterfacePointer, valueFileInterfacePointer);
	return err;
}

inline MoaError deleteFile(PIMoaFile &fileInterfacePointer) {
	if (!fileInterfacePointer) {
		return kMoaErr_NoErr;
	}

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&fileInterfacePointer);
	};

	if (lastInterfaceRef(fileInterfacePointer)) {
		MoaError err = fileInterfacePointer->Delete();

		if (err != kMoaErr_FileNotFound) {
			return err;
		}
	}
	return kMoaErr_NoErr;
}

inline MoaError clonePathName(PIMoaPathName &pathNameInterfacePointer, PIMoaPathName valuePathNameInterfacePointer) {
	MoaError err = kMoaErr_NoErr;

	if (valuePathNameInterfacePointer) {
		err = valuePathNameInterfacePointer->Clone(&valuePathNameInterfacePointer);

		if (err != kMoaErr_NoErr) {
			valuePathNameInterfacePointer = NULL;
		}
	}

	setInterface((PPMoaVoid)&pathNameInterfacePointer, valuePathNameInterfacePointer);
	return err;
}

inline void endUsingResources(MoaFileRef fileRef, XtraResourceCookie &saveCookie, PIMoaCallback callbackInterfacePointer) {
	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	if (!saveCookie) {
		return;
	}

	callbackInterfacePointer->MoaEndUsingResources(fileRef, saveCookie);
	saveCookie = NULL;
}

inline void freeMemory(PMoaVoid &pv, PIMoaCalloc callocInterfacePointer) {
	if (!callocInterfacePointer) {
		throw std::invalid_argument("callocInterfacePointer must not be NULL");
	}

	if (!pv) {
		return;
	}

	callocInterfacePointer->NRFree(pv);
	pv = NULL;
}

inline MoaError duplicateMemory(PMoaVoid &data, MoaUlong size, PIMoaCalloc callocInterfacePointer) {
	RETURN_NULL(data);

	if (!callocInterfacePointer) {
		throw std::invalid_argument("callocInterfacePointer must not be NULL");
	}

	PMoaVoid destination = callocInterfacePointer->NRAlloc(size);

	MAKE_SCOPE_EXIT(freeMemoryDestinationScopeExit) {
		freeMemory(destination, callocInterfacePointer);
	};

	RETURN_NULL(destination);

	if (memcpy_s(destination, size, data, size)) {
		return kMoaErr_OutOfMem;
	}

	data = destination;
	freeMemoryDestinationScopeExit.dismiss();
	return kMoaErr_NoErr;
}

inline void releaseValue(MoaMmValue &value, PIMoaMmValue mmValueInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->ValueRelease(&value);
	value = kVoidMoaMmValueInitializer;
}

inline void getValue(MoaMmValue &get, const MoaMmValue &value, PIMoaMmValue mmValueInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	get = value;
	mmValueInterfacePointer->ValueAddRef(&get);
}

inline void setValue(MoaMmValue &set, const MoaMmValue &value, PIMoaMmValue mmValueInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	releaseValue(set, mmValueInterfacePointer);

	set = value;
	mmValueInterfacePointer->ValueAddRef(&set);
}

inline void moveValue(MoaMmValue &move, MoaMmValue &value) {
	move = value;
	value = kVoidMoaMmValueInitializer;
}

inline MoaError releaseMedia(MoaDrMediaInfo &mediaInfo, PIMoaDrUtils drUtilsInterfacePointer) {
	if (!drUtilsInterfacePointer) {
		throw std::invalid_argument("drUtilsInterfacePointer must not be NULL");
	}

	if (mediaInfo.mediaData) {
		RETURN_ERR(drUtilsInterfacePointer->MediaRelease(&mediaInfo));
	}

	mediaInfo.mediaData = NULL;
	return kMoaErr_NoErr;
}

#ifdef WINDOWS
inline bool closeHandle(HANDLE &handle) {
	if (handle && handle != INVALID_HANDLE_VALUE) {
		if (!CloseHandle(handle)) {
			return false;
		}
	}

	handle = NULL;
	return true;
}

inline bool closeProcess(HANDLE &process) {
	if (process) {
		if (!CloseHandle(process)) {
			return false;
		}
	}

	process = NULL;
	return true;
}

inline bool closeThread(HANDLE &thread) {
	return closeProcess(thread);
}

inline bool closeFind(HANDLE &find) {
	if (find && find != INVALID_HANDLE_VALUE) {
		if (!FindClose(find)) {
			return false;
		}
	}

	find = NULL;
	return true;
}

inline bool closeMMIOHandle(HMMIO &mmioHandle) {
	if (mmioHandle) {
		if (mmioClose(mmioHandle, MMIO_FHOPEN)) {
			return false;
		}
	}

	mmioHandle = NULL;
	return true;
}

inline bool freeLibrary(HMODULE &moduleHandle) {
	if (moduleHandle) {
		if (!FreeLibrary(moduleHandle)) {
			return false;
		}
	}

	moduleHandle = NULL;
	return true;
}

inline bool destroyWindow(HWND &windowHandle) {
	if (windowHandle) {
		if (!DestroyWindow(windowHandle)) {
			return false;
		}
	}

	windowHandle = NULL;
	return true;
}

inline bool deleteGDIDC(HDC &dcHandle) {
	if (dcHandle) {
		if (!DeleteDC(dcHandle)) {
			return false;
		}
	}

	dcHandle = NULL;
	return true;
}

inline bool deleteGDIObject(HGDIOBJ &gdiObjectHandle) {
	if (gdiObjectHandle) {
		if (!DeleteObject(gdiObjectHandle)) {
			return false;
		}
	}

	gdiObjectHandle = NULL;
	return true;
}

inline bool deselectGDIObject(HDC dcHandle, HGDIOBJ &replacedGDIObjectHandle) {
	if (dcHandle && replacedGDIObjectHandle && replacedGDIObjectHandle != HGDI_ERROR) {
		HGDIOBJ gdiObjectHandle = SelectObject(dcHandle, replacedGDIObjectHandle);

		if (!gdiObjectHandle || gdiObjectHandle == HGDI_ERROR) {
			return false;
		}
	}

	replacedGDIObjectHandle = NULL;
	return true;
}

inline bool unmapMappedView(LPVOID &mappedView) {
	if (mappedView) {
		if (!UnmapViewOfFile(mappedView)) {
			return false;
		}
	}

	mappedView = NULL;
	return true;
}
#endif

MoaError getSymbol(SYMBOL_VARIANT &symbolVariant, PIMoaMmValue mmValueInterfacePointer);
MoaError getSymbol(const SYMBOL_VARIANT &symbolVariant, MoaMmSymbol &symbol, PIMoaMmValue mmValueInterfacePointer);
MoaError readStreamSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead, PIMoaStream readStreamInterfacePointer);
MoaError writeStreamSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite, PIMoaStream writeStreamInterfacePointer);
MoaError readStreamPartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead, MoaStreamCount &numberOfBytesRead, PIMoaStream readStreamInterfacePointer);
MoaError writeStreamPartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite, MoaStreamCount &numberOfBytesWritten, PIMoaStream writeStreamInterfacePointer);

#ifdef WINDOWS
MoaError setFileAttributeHiddenWide(bool hidden, LPCWSTR pathWideStringPointer);
#endif