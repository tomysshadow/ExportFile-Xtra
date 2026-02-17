#pragma once
#include "utils.h"
#include "GlobalHandle.h"
#include "Stream.h"
#include "Label.h"

#include "mmixasst.h"

namespace Formats {
	class Format : NonCopyable {
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
		virtual MoaError writeFile(bool agent, PIMoaFile writeFileInterfacePointer);
		virtual MoaError cancelFile();
		virtual MoaError swapFile(bool status);
		virtual MoaError replaceExistingFile(PIMoaFile fileInterfacePointer);
		virtual MoaError get(PMoaVoid &data, MoaUlong &size) const;
		virtual MoaError get(MoaUlong &size, PIMoaStream writeStreamInterfacePointer) const;
	};

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