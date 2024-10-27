#pragma once
#include "shared.h"

#define kExportFileRegKey_MixerSavedCallHandler MOADICT_RUNTIME_KEY_PREFIX "MixerSavedCallHandler"

namespace Mixer {
	#ifdef WINDOWS
	class Window {
		private:
		void create();
		void destroy();
		void duplicate(const Window &window);
		void move(Window &window);

		HWND handle = NULL;

		// these are fine to store statically
		// they will not need to outlive the module unloading
		static HMODULE moduleHandle;
		static ATOM registeredClass;

		static LRESULT onMixerSaved(WORD wParam, DWORD lParam);
		static LRESULT CALLBACK proc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		public:
		static const UINT WM_MIXER_SAVED = WM_USER;

		Window();
		~Window();
		Window(const Window &window);
		Window(Window &&window) noexcept;
		Window &operator=(const Window &window);
		Window &operator=(Window &&window) noexcept;
		HWND getHandle();
	};
	#endif

	unsigned int __stdcall thread(void* argList);
}