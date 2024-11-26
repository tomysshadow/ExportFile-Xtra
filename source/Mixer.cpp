#include "Mixer.h"
#include "Media.h"
#include "ValueConverter.h"
#include "Registry.h"
#include <chrono>
#include <thread>

#ifdef WINDOWS
#include <process.h>
#endif

// errors returned from this function go off into the void
// so we should only return the error when it is impossible to continue
// ideally, even if the file write fails, the mixerSaved handler is still called
// with the error code passed to it
MoaError mixerSavedContext(Media::MixerMedia &mixerMedia) {
	PIMoaDrCastMem drCastMemInterfacePointer = mixerMedia.getDrCastMemInterfacePointer();

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
	};

	// can't call the handler without the member argument
	if (!drCastMemInterfacePointer) {
		return kMoaErr_InternalError;
	}

	PIMoaMmValue mmValueInterfacePointer = mixerMedia.getMmValueInterfacePointer();

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
	};

	// can't release the member value without this interface
	if (!mmValueInterfacePointer) {
		return kMoaErr_InternalError;
	}

	MoaMmValue isSavingValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(isSavingValue, mmValueInterfacePointer);
	};

	MoaLong isSaving = FALSE;

	// if we fail to tell if the member is saving, the member probably got erased
	// we should report this as an error to the mixerSaved handler instead of returning
	MoaError err = drCastMemInterfacePointer->GetProp(mixerMedia.getIsSavingSymbol(), &isSavingValue);

	if (err == kMoaErr_NoErr) {
		err = mmValueInterfacePointer->ValueToInteger(&isSavingValue, &isSaving);

		if (err != kMoaErr_NoErr) {
			isSaving = FALSE;
		}
	}

	// still not done saving yet
	if (isSaving) {
		return kMoaStatus_OK;
	}

	if (!mixerMedia.formatPointer) {
		return kMoaErr_InternalError;
	}

	// if we fail to swap the files, this error should be reported to the mixerSaved handler
	err = errOrDefaultErr(fileErr(mixerMedia.formatPointer->swapFile(false)), err);

	// the rest of this code is for calling the mixerSaved handler
	// it's important to handle the files if at all possible to not leave junk on the disk
	// so this is done second
	PIMoaRegistryEntryDict registryEntryDictInterfacePointer = mixerMedia.getRegistryEntryDictInterfacePointer();

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&registryEntryDictInterfacePointer);
	};

	// we should never call the handler if we can't be sure if we're supposed to
	if (!registryEntryDictInterfacePointer) {
		return kMoaErr_InternalError;
	}

	MoaLong mixerSavedCallHandler = 0;
	RETURN_ERR(Registry::Entry::getValueLong(kExportFileRegKey_MixerSavedCallHandler, mixerSavedCallHandler, registryEntryDictInterfacePointer));

	if (mixerSavedCallHandler) {
		MoaMmValue memberValue = kVoidMoaMmValueInitializer;

		SCOPE_EXIT {
			releaseValue(memberValue, mmValueInterfacePointer);
		};

		// can't call the handler without the member value
		RETURN_ERR(mixerMedia.exportFileValueConverter.toValue(drCastMemInterfacePointer, memberValue));

		mixerMedia.lingo.callHandler(memberValue, err);
	}
	return kMoaStatus_False;
}

MoaError mixerSaved(Media::MixerMedia* mixerMediaPointer) {
	RETURN_NULL(mixerMediaPointer);

	// this must be done in here because it must be
	// destroyed on the same thread it was created on (on Windows)
	MAKE_SCOPE_EXIT(deleteMixerMediaPointerScopeExit) {
		delete mixerMediaPointer;
	};

	PIMoaDrMovieContext drMovieContextInterfacePointer = mixerMediaPointer->getDrMovieContextInterfacePointer();

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&drMovieContextInterfacePointer);
	};

	DrContextState drContextState = {};

	if (drMovieContextInterfacePointer) {
		RETURN_ERR(drMovieContextInterfacePointer->PushXtraContext(&drContextState));
	}

	MoaError err = kMoaErr_NoErr;

	SCOPE_EXIT {
		if (drMovieContextInterfacePointer) {
			err = errOrDefaultErr(drMovieContextInterfacePointer->PopXtraContext(&drContextState), err);
		}
	};
	
	err = mixerSavedContext(*mixerMediaPointer);

	if (err != kMoaStatus_OK) {
		return err;
	}

	deleteMixerMediaPointerScopeExit.dismiss();
	return err;
}

unsigned int __declspec(noinline) threadNoInline(void* argList) {
	if (!argList) {
		throw std::invalid_argument("argList must not be NULL");
	}

	Media::MixerMedia* mixerMediaPointer = (Media::MixerMedia*)argList;

	if (!mixerMediaPointer) {
		throw std::logic_error("mixerMediaPointer must not be zero");
	}

	#ifdef WINDOWS
	HMODULE moduleHandle = mixerMediaPointer->moduleHandle;

	if (!moduleHandle) {
		throw std::logic_error("moduleHandle must not be NULL");
	}

	HWND windowHandle = mixerMediaPointer->window.getHandle();

	if (!windowHandle) {
		throw std::logic_error("windowHandle must not be NULL");
	}
	#endif

	// this doesn't really need to be precise, just needs to be some kinda small amount of time
	const std::chrono::milliseconds MILLISECONDS(25);

	unsigned int result = 0;
	MoaError err = kMoaErr_NoErr;

	for (;;) {
		#ifdef MACINTOSH
		err = mixerSaved(mixerMediaPointer);
		#endif
		#ifdef WINDOWS
		err = SendMessage(windowHandle, Mixer::Window::WM_MIXER_SAVED, 0, (LPARAM)mixerMediaPointer);
		#endif

		if (FAILED(err)) {
			result = 1;
		}

		if (err != kMoaStatus_OK) {
			break;
		}

		std::this_thread::sleep_for(MILLISECONDS);
	}

	#ifdef MACINTOSH
	// ???
	#endif
	#ifdef WINDOWS
	FreeLibraryAndExitThread(moduleHandle, result);
	#endif
	return result;
}

#ifdef WINDOWS
void Mixer::Window::create() {
	if (!moduleHandle) {
		// shouldn't increment the refcount, this object won't outlive the module
		if (!GetModuleHandleEx(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
			| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,

			(LPCSTR)&moduleHandle,
			&moduleHandle
		)

		|| !moduleHandle) {
			throw std::runtime_error("Failed to Get Module Handle Ex");
		}
	}

	const TCHAR className[] = TEXT("Mixer");

	if (!registeredClass) {
		WNDCLASSEX windowClassEx = {};
		const UINT WINDOW_CLASS_EX_SIZE = sizeof(windowClassEx);

		windowClassEx.cbSize = WINDOW_CLASS_EX_SIZE;
		windowClassEx.lpfnWndProc = proc;
		windowClassEx.hInstance = moduleHandle;
		windowClassEx.lpszClassName = className;

		registeredClass = RegisterClassEx(&windowClassEx);

		if (!registeredClass) {
			throw std::runtime_error("Failed to Register Class");
		}
	}

	handle = CreateWindowEx(
		NULL,
		className,
		TEXT("Mixer"),
		WS_CHILD,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		HWND_MESSAGE,
		NULL,
		moduleHandle,
		NULL
	);

	if (!handle) {
		throw std::runtime_error("Failed to Create Window Ex");
	}
}

void Mixer::Window::destroy() {
	if (!destroyWindow(handle)) {
		throw std::runtime_error("Failed to Destroy Window");
	}

	// errors here are ignored in case another instance has a window open
	if (registeredClass) {
		if (UnregisterClass((LPCSTR)registeredClass, moduleHandle)) {
			registeredClass = NULL;
		}
	}
}

void Mixer::Window::duplicate(const Window &window) {
	if (!destroyWindow(handle)) {
		throw std::runtime_error("Failed to Destroy Window");
	}

	create();
}

void Mixer::Window::move(Window &window) {
	handle = window.handle;
	window.handle = NULL;
}

HMODULE Mixer::Window::moduleHandle = NULL;
ATOM Mixer::Window::registeredClass = NULL;

LRESULT Mixer::Window::onMixerSaved(WORD wParam, DWORD lParam) {
	RETURN_NULL(lParam);
	return mixerSaved((Media::MixerMedia*)lParam);
}

LRESULT CALLBACK Mixer::Window::proc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_MIXER_SAVED) {
		return onMixerSaved(wParam, lParam);
	}
	return DefWindowProc(windowHandle, message, wParam, lParam);
}

Mixer::Window::Window() {
	create();
}

Mixer::Window::~Window() {
	destroy();
}

Mixer::Window::Window(const Window &window) {
	duplicate(window);
}

Mixer::Window::Window(Window &&window) noexcept {
	*this = std::move(window);
}

Mixer::Window &Mixer::Window::operator=(const Window &window) {
	if (this == &window) {
		return *this;
	}

	duplicate(window);
	return *this;
}

Mixer::Window &Mixer::Window::operator=(Window &&window) noexcept {
	if (this == &window) {
		return *this;
	}

	destroy();
	move(window);
	return *this;
}

HWND Mixer::Window::getHandle() {
	return handle;
}
#endif

unsigned int __stdcall Mixer::thread(void* argList) {
	// do not create any C++ objects here
	// (unwinding interferes with _endthreadex)
	unsigned int result = threadNoInline(argList);

	#ifdef WINDOWS
	_endthreadex(result);
	#endif
	return result;
}