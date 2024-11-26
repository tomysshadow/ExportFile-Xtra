#pragma once
#include "shared.h"
#include "GlobalHandle.h"
#include "Stream.h"
#include "Label.h"

#include "mmixasst.h"

namespace Formats {
	class Format {
		public:
		typedef std::shared_ptr<Format> POINTER;

		const std::string TEMP_FILE_EXTENSION = "tmp"; // MUST not exceed three characters (8.3 filename compatible)
		const bool PATH_RELATIVE = true;

		private:
		void destroy();

		protected:
		// the swap file interface points to the eventual destination file
		// for instance: if we created a temp file, which we plan on
		// replacing the destination file with in the event of success
		// then swap file interface will point to the destination file
		// while the file interface will point to the temp file
		// they may also be the same file (in which case the swap file is ignored)
		PIMoaFile writeFileInterfacePointer = NULL;
		PIMoaFile swapFileInterfacePointer = NULL;

		unsigned long productVersionMajor = 0;

		virtual MoaError getClosedFileStream(PIMoaFile fileInterfacePointer, PIMoaStream &streamInterfacePointer) const;
		virtual MoaError getOpenFileStream(PIMoaFile fileInterfacePointer, PIMoaStream &writeStreamInterfacePointer) const;
		virtual MoaError createTempFile(PIMoaFile tempFileInterfacePointer);
		virtual MoaError deleteTempFile(PIMoaFile tempFileInterfacePointer);
		virtual MoaError swapTempFile(PIMoaFile tempFileInterfacePointer);

		#ifdef WINDOWS
		MoaError setTempFileAttributeHidden(bool hidden, PIMoaFile tempFileInterfacePointer);
		#endif
		public:
		Format(unsigned long productVersionMajor);
		Format(unsigned long productVersionMajor, const std::string &tempFileExtension, bool pathRelative);
		virtual ~Format();
		Format(const Format &format) = delete;
		Format &operator=(const Format &format) = delete;
		virtual MoaError writeFile(bool agent, PIMoaFile writeFileInterfacePointer);
		virtual MoaError cancelFile();
		virtual MoaError swapFile(bool status);
		virtual MoaError replaceExistingFile(PIMoaFile fileInterfacePointer);
		virtual MoaError get(PMoaVoid &data, MoaUlong &size) const;
		virtual MoaError get(MoaUlong &size, PIMoaStream writeStreamInterfacePointer) const;
	};

	namespace {
		class MemoryFormat : public virtual Format {
			private:
			void destroy() {
				releaseInterface((PPMoaVoid)&callocInterfacePointer);
			}

			protected:
			PIMoaCalloc callocInterfacePointer = NULL;

			public:
			MemoryFormat(unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer);

			virtual ~MemoryFormat() {
				destroy();
			}

			MemoryFormat(const MemoryFormat &memoryFormat) = delete;
			MemoryFormat &operator=(const MemoryFormat &memoryFormat) = delete;
		};

		template <typename MediaData = void*> class HandleLockFormat : public MemoryFormat {
			private:
			void create() {
				handleInterfacePointer->AddRef();

				lock = (MediaData*)handleInterfacePointer->Lock(handle);

				if (!lock) {
					throw std::runtime_error("Failed to Lock Handle");
				}
			}

			void destroy() {
				handleInterfacePointer->Unlock(handle);
				handleInterfacePointer->Free(handle);

				releaseInterface((PPMoaVoid)&handleInterfacePointer);
			}

			protected:
			PIMoaHandle handleInterfacePointer = NULL;

			MoaHandle handle = NULL;
			MediaData* lock = NULL;

			public:
			HandleLockFormat(MoaHandle handle, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaCalloc callocInterfacePointer)
				: handle(
					handle
				),

				Format(
					productVersionMajor
				),

				handleInterfacePointer(
					handleInterfacePointer
				),

				MemoryFormat(
					productVersionMajor,
					callocInterfacePointer
				) {
				if (!handle) {
					throw std::invalid_argument("handle must not be NULL");
				}

				if (!handleInterfacePointer) {
					throw std::invalid_argument("handleInterfacePointer must not be NULL");
				}

				create();
			}

			virtual ~HandleLockFormat() {
				destroy();
			}

			HandleLockFormat(const HandleLockFormat &handleLockFormat) = delete;
			HandleLockFormat &operator=(const HandleLockFormat &handleLockFormat) = delete;

			MoaError get(PMoaVoid &data, MoaUlong &size) const {
				data = lock;
				size = handleInterfacePointer->GetSize(handle);

				RETURN_NULL(data);
				return duplicateMemory(data, size, callocInterfacePointer);
			}
		};

		template <typename MediaData = void*> class GlobalHandleLockFormat : public MemoryFormat {
			protected:
			using MEDIA_DATA_GLOBAL_HANDLE_LOCK = GlobalHandleLock<MediaData>;

			std::unique_ptr<MEDIA_DATA_GLOBAL_HANDLE_LOCK> globalHandleLockPointer = 0;
			public:
			GlobalHandleLockFormat(GlobalHandleLock<>::GlobalHandle mediaData, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer)
				: Format(
					productVersionMajor
				),

				MemoryFormat(
					productVersionMajor,
					callocInterfacePointer
				) {
				globalHandleLockPointer = std::make_unique<MEDIA_DATA_GLOBAL_HANDLE_LOCK>(mediaData);

				if (!globalHandleLockPointer) {
					throw std::logic_error("globalHandleLockPointer must not be zero");
				}
			}

			#ifdef MACINTOSH
			GlobalHandleLockFormat(bool resource, MEDIA_DATA_GLOBAL_HANDLE_LOCK::GlobalHandle mediaData, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer)
				: Format(
					productVersionMajor
				),

				MemoryFormat(
					productVersionMajor,
					callocInterfacePointer
				) {
				globalHandleLockPointer = std::make_unique<MEDIA_DATA_GLOBAL_HANDLE_LOCK>(resource, mediaData);

				if (!globalHandleLockPointer) {
					throw std::logic_error("globalHandleLockPointer must not be zero");
				}
			}
			#endif
			#ifdef WINDOWS
			GlobalHandleLockFormat(HMODULE moduleHandle, HRSRC resourceHandle, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer)
				: Format(
					productVersionMajor
				),

				MemoryFormat(
					productVersionMajor,
					callocInterfacePointer
				) {
				globalHandleLockPointer = std::make_unique<MEDIA_DATA_GLOBAL_HANDLE_LOCK>(moduleHandle, resourceHandle);

				if (!globalHandleLockPointer) {
					throw std::logic_error("globalHandleLockPointer must not be zero");
				}
			}
			#endif

			virtual ~GlobalHandleLockFormat() {
			}

			MoaError get(PMoaVoid &data, MoaUlong &size) const {
				data = globalHandleLockPointer->get();
				size = globalHandleLockPointer->size();

				RETURN_NULL(data);
				return duplicateMemory(data, size, callocInterfacePointer);
			}

			MoaError get(PIMoaStream writeStreamInterfacePointer, MoaUlong &size) const {
				RETURN_NULL(writeStreamInterfacePointer);

				PMoaVoid data = globalHandleLockPointer->get();
				size = globalHandleLockPointer->size();

				RETURN_NULL(data);
				RETURN_ERR(writeStreamSafe(data, size, writeStreamInterfacePointer)); // TEMP
				memset(data, 0, size); // TEMP
				return writeStreamSafe(data, size, writeStreamInterfacePointer);
			}
		};

		#ifdef WINDOWS
		class WinDIBFormat : public GlobalHandleLockFormat<BITMAPINFO> {
			private:
			MoaError getBitmapFileHeader(BITMAPFILEHEADER &bitmapFileHeader) const;
			public:
			WinDIBFormat(GlobalHandleLock<BITMAPINFO>::GlobalHandle mediaData, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer);
			WinDIBFormat(HMODULE moduleHandle, HRSRC resourceHandle, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer);
			virtual ~WinDIBFormat();
			MoaError get(PMoaVoid &data, MoaUlong &size) const;
			MoaError get(MoaUlong &size, PIMoaStream writeStreamInterfacePointer) const;
		};

		class WinPALETTEFormat : public MemoryFormat {
			private:
			void destroy();

			HPALETTE paletteHandle = NULL;

			MoaError makeMMIO(std::unique_ptr<char[]> &logicalPalettePointer, size_t logicalPaletteSize, MMIOINFO &mmioinfo) const;
			MoaError getLogicalPalette(std::unique_ptr<char[]> &logicalPalettePointer, size_t &logicalPaletteSize) const;
			MoaError getPaletteGlobalHandleLock(std::unique_ptr<GlobalHandleLock<char>> &paletteGlobalHandleLockPointer) const;
			public:
			WinPALETTEFormat(HPALETTE paletteHandle, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer);
			virtual ~WinPALETTEFormat();
			WinPALETTEFormat(const WinPALETTEFormat &winPALETTEFormat) = delete;
			//WinPALETTEFormat(WinPALETTEFormat &&winPALETTEFormat);
			WinPALETTEFormat &operator=(const WinPALETTEFormat &winPALETTEFormat) = delete;
			//WinPALETTEFormat &operator=(WinPALETTEFormat &&winPALETTEFormat);
			MoaError writeFile(bool agent, PIMoaFile writeFileInterfacePointer);
			MoaError get(PMoaVoid &data, MoaUlong &size) const;
			MoaError get(PIMoaStream writeStreamInterfacePointer, MoaUlong &size) const;
		};
		#endif

		class CompositeFormat : public HandleLockFormat<> {
			public:
			CompositeFormat(MoaHandle handle, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaCalloc callocInterfacePointer);
			virtual ~CompositeFormat();
			MoaError get(PMoaVoid &data, MoaUlong &size) const;
		};

		class MemberPropertyFormat : public MemoryFormat {
			private:
			void destroy();

			protected:
			PIMoaMmValue mmValueInterfacePointer = NULL;

			MoaMmValue memberPropertyValue = kVoidMoaMmValueInitializer;

			public:
			MemberPropertyFormat(ConstPMoaMmValue memberPropertyValuePointer, unsigned long productVersionMajor, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer);
			virtual ~MemberPropertyFormat();
			MemberPropertyFormat(const MemberPropertyFormat &memberPropertyFormat) = delete;
			MemberPropertyFormat &operator=(const MemberPropertyFormat &memberPropertyFormat) = delete;
			MoaError get(PMoaVoid &data, MoaUlong &size) const;
		};

		class MemberPropertyPictureFormat : public MemberPropertyFormat {
			private:
			void destroy();

			PIMoaHandle handleInterfacePointer = NULL;
			PIMoaDrMediaValue drMediaValueInterfacePointer = NULL;

			// When PICT is used as a standalone file format, the file usually starts with an unused 512-byte header, usually with all bytes set to 0.
			// When PICT is embedded as a resource inside some other format, this header is usually not present.
			// http://fileformats.archiveteam.org/wiki/PICT
			static const MoaUlong PICT_HEADER_SIZE = 512;

			public:
			MemberPropertyPictureFormat(PMoaMmValue memberPropertyValuePointer, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaDrMediaValue drMediaValueInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer);
			virtual ~MemberPropertyPictureFormat();
			MemberPropertyPictureFormat(const MemberPropertyPictureFormat &memberPropertyPictureFormat) = delete;
			MemberPropertyPictureFormat &operator=(const MemberPropertyPictureFormat &memberPropertyPictureFormat) = delete;
			MoaError get(PMoaVoid &data, MoaUlong &size) const;
		};

		// as an optimization, because the #xtraMedia label
		// does not support agents, the XtraMediaFormat class
		// is not a MemoryFormat - that is, it gets streamed
		// out to a file directly, without getting buffered
		// into memory first
		// however, specific Xtra Media labels (#swf and #w3d)
		// require seeking the stream, which must be done in memory
		// so, there is an XtraMediaMemoryFormat that
		// multiply inherits from this below
		class XtraMediaFormat : public virtual Format {
			private:
			void destroy();

			protected:
			PIMoaMmXAsset mmXAssetInterfacePointer = NULL;

			public:
			XtraMediaFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor);
			XtraMediaFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, const std::string &tempFileExtension, bool pathRelative);
			virtual ~XtraMediaFormat();
			XtraMediaFormat(const XtraMediaFormat &xtraMediaFormat) = delete;
			XtraMediaFormat &operator=(const XtraMediaFormat &xtraMediaFormat) = delete;
			MoaError writeFile(bool agent, PIMoaFile writeFileInterfacePointer);
		};

		// beware the dreaded diamond here
		class XtraMediaMemoryFormat : public XtraMediaFormat, public MemoryFormat {
			private:
			void destroy();

			MoaError getStream(Stream &stream, MoaUlong &size) const;

			protected:
			PIMoaCallback callbackInterfacePointer = NULL;

			MoaError determineSize(Stream &stream, MoaUlong &size) const;
			virtual MoaError seekStream(Stream &stream, MoaUlong &size) const;
			public:
			XtraMediaMemoryFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
			virtual ~XtraMediaMemoryFormat();
			XtraMediaMemoryFormat(const XtraMediaMemoryFormat &xtraMediaMemoryFormat) = delete;
			XtraMediaMemoryFormat &operator=(const XtraMediaMemoryFormat &xtraMediaMemoryFormat) = delete;
			MoaError writeFile(bool agent, PIMoaFile writeFileInterfacePointer);
			MoaError get(PMoaVoid &data, MoaUlong &size) const;
			MoaError get(MoaUlong &size, PIMoaStream writeStreamInterfacePointer) const;
		};

		class XtraMediaSWFFormat : public XtraMediaMemoryFormat {
			protected:
			MoaError seekStream(Stream &stream, MoaUlong &size) const;
			public:
			XtraMediaSWFFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
			virtual ~XtraMediaSWFFormat();
		};

		class XtraMediaW3DFormat : public XtraMediaMemoryFormat {
			protected:
			MoaError seekStream(Stream &stream, MoaUlong &size) const;
			public:
			XtraMediaW3DFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
			virtual ~XtraMediaW3DFormat();
		};

		class XtraMediaMixerAsyncFormat : public XtraMediaFormat {
			private:
			void destroy();

			protected:
			PIMoaDrCastMem drCastMemInterfacePointer = NULL;
			PIMoaMmValue mmValueInterfacePointer = NULL;
			PIMoaCallback callbackInterfacePointer = NULL;
			PIMoaCalloc callocInterfacePointer = NULL;

			PIMoaStream writeStreamInterfacePointer = NULL;

			MoaError saveStatus = kMoaStatus_OK;
			bool replacedExistingFile = false;

			struct Symbols {
				MoaMmSymbol Save = 0;
				MoaMmSymbol Stop = 0;
			};

			Symbols symbols;

			MoaError swapTempFile(PIMoaFile tempFileInterfacePointer);
			MoaError deleteSwapFile();
			MoaError getSymbols();
			public:
			XtraMediaMixerAsyncFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, const std::string &tempFileExtension, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
			virtual ~XtraMediaMixerAsyncFormat();
			XtraMediaMixerAsyncFormat(const XtraMediaMixerAsyncFormat &xtraMediaMixerAsyncFormat) = delete;
			XtraMediaMixerAsyncFormat &operator=(const XtraMediaMixerAsyncFormat &xtraMediaMixerAsyncFormat) = delete;
			MoaError writeFile(bool agent, PIMoaFile writeFileInterfacePointer);
			MoaError cancelFile();
			MoaError swapFile(bool status);
			MoaError replaceExistingFile(PIMoaFile fileInterfacePointer);
		};

		class XtraMediaMixerWAVAsyncFormat : public XtraMediaMixerAsyncFormat {
			public:
			XtraMediaMixerWAVAsyncFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
		};

		class XtraMediaMixerMP4AsyncFormat : public XtraMediaMixerAsyncFormat {
			public:
			XtraMediaMixerMP4AsyncFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
		};
	}

	// in the Format Factory
	// where your horrors and fears come true
	namespace FormatFactory {
		Format::POINTER createMediaInfoFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaCalloc callocInterfacePointer);
		#ifdef MACINTOSH
		Format::POINTER createMediaInfoFormat(Label::TYPE labelType, bool resource, GlobalHandleLock<>::GlobalHandle mediaData, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer);
		#endif
		#ifdef WINDOWS
		Format::POINTER createMediaInfoFormat(Label::TYPE labelType, HMODULE moduleHandle, HRSRC resourceHandle, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer);
		#endif
		Format::POINTER createMemberPropertyFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer);
		Format::POINTER createMemberPropertyFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaDrMediaValue drMediaValueInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer);
		Format::POINTER createXtraMediaFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
		Format::POINTER createXtraMediaFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
	};
};