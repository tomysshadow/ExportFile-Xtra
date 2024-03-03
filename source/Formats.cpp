#include "Formats.h"
#include "Media.h"

extern "C" {
	#include "drmedfix.h"
}

void Formats::Format::destroy() {
	releaseInterface((PPMoaVoid)&writeFileInterfacePointer);
	releaseInterface((PPMoaVoid)&swapFileInterfacePointer);
}

MoaError Formats::Format::getClosedFileStream(PIMoaFile fileInterfacePointer, PIMoaStream &streamInterfacePointer) const {
	RETURN_NULL(fileInterfacePointer);

	MoaError err = kMoaErr_NoErr;

	// because file is empty, initialize size to zero
	RETURN_ERR(fileInterfacePointer->GetStream(0, &streamInterfacePointer));
	RETURN_NULL(streamInterfacePointer);

	MAKE_SCOPE_EXIT(closeStreamInterfacePointerScopeExit) {
		err = errOrDefaultErr(closeStream(streamInterfacePointer), err);
	};

	// close the stream in case it's open already
	// (it shouldn't be)
	err = streamInterfacePointer->Close();

	if (err == kMoaStreamErr_StreamNotOpen) {
		err = kMoaErr_NoErr;
	}

	RETURN_ERR(err);
	closeStreamInterfacePointerScopeExit.dismiss();
	return err;
}

MoaError Formats::Format::getOpenFileStream(PIMoaFile fileInterfacePointer, PIMoaStream &writeStreamInterfacePointer) const {
	RETURN_NULL(fileInterfacePointer);

	MoaError err = kMoaErr_NoErr;

	RETURN_ERR(getClosedFileStream(fileInterfacePointer, writeStreamInterfacePointer));
	RETURN_NULL(writeStreamInterfacePointer);

	MAKE_SCOPE_EXIT(closeStreamInterfacePointerScopeExit) {
		err = errOrDefaultErr(closeStream(writeStreamInterfacePointer), err);
	};

	// because we just created the file, it should be empty at this stage
	// now, we must acquire these permissions, otherwise it's a fail
	RETURN_ERR(openStream(kMoaStreamOpenAccess_WriteOnly, false, writeStreamInterfacePointer));
	closeStreamInterfacePointerScopeExit.dismiss();
	return err;
}

MoaError Formats::Format::createTempFile(PIMoaFile tempFileInterfacePointer) {
	RETURN_NULL(tempFileInterfacePointer);

	// there already is one?
	if (swapFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	// this must be completely airtight
	// the swap file should never exist without a temp file
	MAKE_SCOPE_EXIT(releaseSwapFileInterfacePointerScopeExit) {
		releaseInterface((PPMoaVoid)&swapFileInterfacePointer);
	};

	RETURN_ERR(cloneFile(swapFileInterfacePointer, tempFileInterfacePointer));
	RETURN_NULL(swapFileInterfacePointer);

	// bugfix: SetFilename doesn't like paths without parents but doesn't return an error
	MoaChar pathnameSpec[MOA_MAX_PATHNAME] = "";
	RETURN_ERR(tempFileInterfacePointer->GetPathnameSpec(pathnameSpec, MOA_MAX_PATHNAME));

	bool hasParentPath = FILESYSTEM_DIRECTOR_PATH(pathnameSpec, productVersionMajor).has_parent_path();

	// folder paths must be 12 characters shorter than MAX_PATH in order
	// to allow an 8.3 filename to exist before MAX_PATH is reached
	// therefore, as long as our filename is an 8.3 filename, we will
	// never hit MAX_PATH
	// in order to ensure this, this prefix must be five characters or less
	// tilde prefix is generally recognized to mean a temporary file
	// "ExFi" gives some clue where the temporary file came from if a crash occurs
	// use extension of .tmp for a further clue to the file's purpose
	// unless other specific extension required
	const std::string PREFIX = "~ExFi";
	const std::string PERIOD_TEMP_FILE_EXTENSION = "." + TEMP_FILE_EXTENSION;

	std::string filename = PREFIX + PERIOD_TEMP_FILE_EXTENSION;

	MoaError err = hasParentPath
		? tempFileInterfacePointer->SetFilename(filename.c_str())
		: tempFileInterfacePointer->SetPathnameSpec(filename.c_str(), FALSE);

	err = tempFileInterfacePointer->CreateFile();

	if (err != kMoaFileErr_DuplicateSpec) {
		RETURN_ERR(err);
		releaseSwapFileInterfacePointerScopeExit.dismiss();
		return err;
	}

	// iterates through A to Z, AA to ZZ, AAA to ZZZ
	// we don't use SetNewTempSpec here because we want the name to end with a specific extension
	// getting a temp spec and then modifying it will make it invalid
	// we could create a temporary directory instead and put the file in it, but
	// we want to keep this name short, to not hit MAX_PATH
	// we also can't put this in the system temporary directory
	// because later we'll call SwapFile on this, which requires
	// that it be on the same volume as the file we're swapping with
	const size_t ABC_SIZE = 4;
	char abc[ABC_SIZE] = "";

	size_t index = 2;

	for (abc[0] = 'A'; abc[0] <= 'Z'; abc[0]++) {
		for (abc[1] = 'A'; abc[1] <= 'Z'; abc[1]++) {
			for (abc[2] = 'A'; abc[2] <= 'Z'; abc[2]++) {
				filename = PREFIX + (char*)&abc[index] + PERIOD_TEMP_FILE_EXTENSION;

				err = hasParentPath
					? tempFileInterfacePointer->SetFilename(filename.c_str())
					: tempFileInterfacePointer->SetPathnameSpec(filename.c_str(), FALSE);

				RETURN_ERR(err);

				err = tempFileInterfacePointer->CreateFile();

				if (err != kMoaFileErr_DuplicateSpec) {
					RETURN_ERR(err);
					releaseSwapFileInterfacePointerScopeExit.dismiss();
					return err;
				}
			}

			if (index > 1) {
				index = 1;
				abc[index]--;
			}
		}

		if (index > 0) {
			index = 0;
			abc[index]--;
		}
	}
	return kMoaFileErr_IoError;
}

MoaError Formats::Format::deleteTempFile(PIMoaFile tempFileInterfacePointer) {
	RETURN_NULL(tempFileInterfacePointer);

	// no more temp file, so release (don't delete!) swap file
	releaseInterface((PPMoaVoid)&swapFileInterfacePointer);

	MoaError err = tempFileInterfacePointer->Delete();

	if (err == kMoaErr_NoErr || err == kMoaErr_FileNotFound) {
		return err;
	}
	return kMoaFileErr_IoError;
}

MoaError Formats::Format::swapTempFile(PIMoaFile tempFileInterfacePointer) {
	RETURN_NULL(tempFileInterfacePointer);

	if (!swapFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	MoaError err = kMoaErr_NoErr;

	SCOPE_EXIT {
		err = errOrDefaultErr(deleteTempFile(tempFileInterfacePointer), err);
	};

	// this thankfully doesn't allow swapping with directories (that would be very bad)
	RETURN_ERR(tempFileInterfacePointer->SwapFile(swapFileInterfacePointer));
	return err;
}

#ifdef WINDOWS
MoaError Formats::Format::setTempFileAttributeHidden(bool hidden, PIMoaFile tempFileInterfacePointer) {
	RETURN_NULL(tempFileInterfacePointer);

	MoaError err = kMoaErr_NoErr;

	MAKE_SCOPE_EXIT(deleteTempFileInterfacePointerScopeExit) {
		err = errOrDefaultErr(deleteTempFile(tempFileInterfacePointer), err);
	};

	MoaSystemFileSpec sysSpec = "";
	const MoaLong SYS_SPEC_SIZE = sizeof(sysSpec);

	RETURN_ERR(tempFileInterfacePointer->GetSysSpec(sysSpec, SYS_SPEC_SIZE));

	RETURN_ERR(setFileAttributeHiddenWide(hidden, CA2W(sysSpec, CP_DIRECTOR(productVersionMajor))));
	deleteTempFileInterfacePointerScopeExit.dismiss();
	return err;
}
#endif

Formats::Format::Format(unsigned long productVersionMajor)
	: productVersionMajor(productVersionMajor) {
}

Formats::Format::Format(unsigned long productVersionMajor, const std::string &tempFileExtension, bool pathRelative)
	: productVersionMajor(productVersionMajor),
	TEMP_FILE_EXTENSION(tempFileExtension),
	PATH_RELATIVE(pathRelative) {
}

Formats::Format::~Format() {
	destroy();
}

MoaError Formats::Format::writeFile(bool agent, PIMoaFile writeFileInterfacePointer) {
	RETURN_NULL(writeFileInterfacePointer);

	setInterface((PPMoaVoid)&this->writeFileInterfacePointer, writeFileInterfacePointer);
	RETURN_NULL(this->writeFileInterfacePointer);

	// the format just needs to know we're going to write a file so it can cancel later
	// but if an agent is writing the file on our behalf we return here
	if (agent) {
		return kMoaErr_NoErr;
	}

	// the default implementation of this calls get
	MoaError err = kMoaErr_NoErr;

	PIMoaStream writeStreamInterfacePointer = NULL;
	RETURN_ERR(getOpenFileStream(this->writeFileInterfacePointer, writeStreamInterfacePointer));
	RETURN_NULL(writeStreamInterfacePointer);

	SCOPE_EXIT {
		err = errOrDefaultErr(closeStream(writeStreamInterfacePointer), err);
	};

	MoaUlong size = 0;
	RETURN_ERR(get(size, writeStreamInterfacePointer));
	return err;
}

MoaError Formats::Format::cancelFile() {
	if (!writeFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&writeFileInterfacePointer);
	};
	return writeFileInterfacePointer->Delete();
}

MoaError Formats::Format::swapFile(bool status) {
	// only used by classes inheriting from this one
	// this implementation returns a status
	if (!status) {
		return kMoaErr_BadParam;
	}

	// if there is no swap file then we're done here
	if (!swapFileInterfacePointer) {
		return kMoaStatus_OK;
	}

	if (!writeFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&writeFileInterfacePointer);
	};

	MoaError err = kMoaStatus_OK;

	#ifdef WINDOWS
	// here the file is done writing so we unhide it
	RETURN_ERR(setTempFileAttributeHidden(false, writeFileInterfacePointer));
	#endif
	
	RETURN_ERR(swapTempFile(writeFileInterfacePointer));
	return err;
}

MoaError Formats::Format::replaceExistingFile(PIMoaFile fileInterfacePointer) {
	RETURN_NULL(fileInterfacePointer);

	RETURN_ERR(createTempFile(fileInterfacePointer));

	#ifdef WINDOWS
	RETURN_ERR(setTempFileAttributeHidden(true, fileInterfacePointer));
	#endif
	return kMoaErr_NoErr;
}

MoaError Formats::Format::get(PMoaVoid &data, MoaUlong &size) const {
	return kMoaErr_NotImplemented;
}

MoaError Formats::Format::get(MoaUlong &size, PIMoaStream writeStreamInterfacePointer) const {
	RETURN_NULL(writeStreamInterfacePointer);

	PMoaVoid data = NULL;
	RETURN_ERR(get(data, size));
	return writeStreamSafe(data, size, writeStreamInterfacePointer);
}

Formats::MemoryFormat::MemoryFormat(unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer)
	: Format(productVersionMajor),
	callocInterfacePointer(callocInterfacePointer) {
	if (!callocInterfacePointer) {
		throw std::invalid_argument("callocInterfacePointer must not be NULL");
	}

	callocInterfacePointer->AddRef();
}

#ifdef WINDOWS
MoaError Formats::WinDIBFormat::getBitmapFileHeader(BITMAPFILEHEADER &bitmapFileHeader) const {
	bitmapFileHeader = {};

	// http://learn.microsoft.com/en-us/windows/win32/gdi/storing-an-image
	// http://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
	bitmapFileHeader.bfType = Media::WinBMPMedia::TYPE; // BM
	//bitmapFileHeader.bfReserved1 = 0;
	//bitmapFileHeader.bfReserved2 = 0;

	PBITMAPINFO bitmapInfoPointer = globalHandleLockPointer->get();
	RETURN_NULL(bitmapInfoPointer);

	BITMAPINFOHEADER &bitmapInfoHeader = bitmapInfoPointer->bmiHeader;

	DWORD bitmapInfoColorsSize = 0;
	
	if (!Media::WinBMPMedia::getBitmapInfoColorsSize(bitmapInfoHeader, false, bitmapInfoColorsSize)) {
		return kMoaErr_BadParam;
	}

	bitmapFileHeader.bfOffBits = Media::WinBMPMedia::BITMAPFILEHEADER_SIZE + bitmapInfoHeader.biSize + bitmapInfoColorsSize;
	bitmapFileHeader.bfSize = Media::WinBMPMedia::BITMAPFILEHEADER_SIZE + globalHandleLockPointer->size();
	return kMoaErr_NoErr;
}

Formats::WinDIBFormat::WinDIBFormat(GlobalHandleLock<BITMAPINFO>::GlobalHandle mediaData, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer)
	: GlobalHandleLockFormat(
		mediaData,
		productVersionMajor,
		callocInterfacePointer
	),

	Format(
		productVersionMajor
	) {
}

Formats::WinDIBFormat::WinDIBFormat(HMODULE moduleHandle, HRSRC resourceHandle, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer)
	: GlobalHandleLockFormat(
		moduleHandle,
		resourceHandle,
		productVersionMajor,
		callocInterfacePointer
	),

	Format(
		productVersionMajor
	) {
}

Formats::WinDIBFormat::~WinDIBFormat() {
}

MoaError Formats::WinDIBFormat::get(PMoaVoid &data, MoaUlong &size) const {
	PBITMAPINFO globalHandleLockData = globalHandleLockPointer->get();
	size_t globalHandleLockSize = globalHandleLockPointer->size();

	RETURN_NULL(globalHandleLockData);

	BITMAPFILEHEADER bitmapFileHeader = {};
	RETURN_ERR(getBitmapFileHeader(bitmapFileHeader));

	size = Media::WinBMPMedia::BITMAPFILEHEADER_SIZE + globalHandleLockSize;
	data = callocInterfacePointer->NRAlloc(size);

	MAKE_SCOPE_EXIT(freeMemoryDataScopeExit) {
		freeMemory(data, callocInterfacePointer);
	};

	RETURN_NULL(data);

	if (memcpy_s(data, size, &bitmapFileHeader, Media::WinBMPMedia::BITMAPFILEHEADER_SIZE)) {
		return kMoaErr_OutOfMem;
	}

	if (memcpy_s((BITMAPFILEHEADER*)data + 1, size - Media::WinBMPMedia::BITMAPFILEHEADER_SIZE, globalHandleLockData, globalHandleLockSize)) {
		return kMoaErr_OutOfMem;
	}

	freeMemoryDataScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError Formats::WinDIBFormat::get(MoaUlong &size, PIMoaStream writeStreamInterfacePointer) const {
	RETURN_NULL(writeStreamInterfacePointer);

	PBITMAPINFO globalHandleLockData = globalHandleLockPointer->get();
	size_t globalHandleLockSize = globalHandleLockPointer->size();

	RETURN_NULL(globalHandleLockData);

	BITMAPFILEHEADER bitmapFileHeader = {};
	RETURN_ERR(getBitmapFileHeader(bitmapFileHeader));

	size = Media::WinBMPMedia::BITMAPFILEHEADER_SIZE + globalHandleLockSize;
	RETURN_ERR(writeStreamSafe((PMoaVoid)&bitmapFileHeader, Media::WinBMPMedia::BITMAPFILEHEADER_SIZE, writeStreamInterfacePointer));
	return writeStreamSafe(globalHandleLockData, globalHandleLockSize, writeStreamInterfacePointer);
}

void Formats::WinPALETTEFormat::destroy() {
	if (!deleteGDIObject(*(HGDIOBJ*)&paletteHandle)) {
		throw std::runtime_error("Failed to Delete GDI Object");
	}
}

MoaError Formats::WinPALETTEFormat::makeMMIO(std::unique_ptr<char[]> &logicalPalettePointer, size_t logicalPaletteSize, MMIOINFO &mmioinfo) const {
	RETURN_NULL(logicalPalettePointer);

	if (!logicalPaletteSize) {
		return kMoaErr_BadParam;
	}

	MoaError err = kMoaErr_NoErr;

	HMMIO mmioHandle = mmioOpen(NULL, &mmioinfo, MMIO_WRITE | MMIO_DENYREAD | MMIO_DENYWRITE);

	SCOPE_EXIT {
		err = errOrDefaultErr(osErr(closeMMIOHandle(mmioHandle)), err);
	};

	RETURN_ERR(osErr(mmioHandle));

	MMCKINFO mmckinfoParent = {};
	mmckinfoParent.fccType = mmioFOURCC('P', 'A', 'L', ' ');

	RETURN_ERR(osErr(mmioCreateChunk(mmioHandle, &mmckinfoParent, MMIO_CREATERIFF) == MMSYSERR_NOERROR));

	SCOPE_EXIT {
		err = errOrDefaultErr(osErr(mmioAscend(mmioHandle, &mmckinfoParent, 0) == MMSYSERR_NOERROR), err);
	};

	MMCKINFO mmckinfoSubchunk = {};
	mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmckinfoSubchunk.cksize = logicalPaletteSize;

	RETURN_ERR(osErr(mmioCreateChunk(mmioHandle, &mmckinfoSubchunk, 0) == MMSYSERR_NOERROR));

	SCOPE_EXIT {
		err = errOrDefaultErr(osErr(mmioAscend(mmioHandle, &mmckinfoSubchunk, 0) == MMSYSERR_NOERROR), err);
	};

	RETURN_ERR(osErr(mmioWrite(mmioHandle, logicalPalettePointer.get(), logicalPaletteSize) != -1));
	return err;
}

MoaError Formats::WinPALETTEFormat::getLogicalPalette(std::unique_ptr<char[]> &logicalPalettePointer, size_t &logicalPaletteSize) const {
	UINT numEntries = GetPaletteEntries(paletteHandle, 0, 0, NULL);
	RETURN_ERR(osErr((DWORD)numEntries));

	logicalPaletteSize = offsetof(LOGPALETTE, palPalEntry[numEntries]);
	logicalPalettePointer = std::unique_ptr<char[]>(new char[logicalPaletteSize]);
	RETURN_NULL(logicalPalettePointer);

	LOGPALETTE &logicalPalette = *(LPLOGPALETTE)logicalPalettePointer.get();
	logicalPalette.palVersion = 0x300;

	logicalPalette.palNumEntries = GetPaletteEntries(paletteHandle, 0, numEntries, logicalPalette.palPalEntry);
	RETURN_ERR(osErr(logicalPalette.palNumEntries));

	if (logicalPalette.palNumEntries != numEntries) {
		return kMoaErr_InternalError;
	}
	return kMoaErr_NoErr;
}

MoaError Formats::WinPALETTEFormat::getPaletteGlobalHandleLock(std::unique_ptr<GlobalHandleLock<char>> &paletteGlobalHandleLockPointer) const {
	std::unique_ptr<char[]> logicalPalettePointer = NULL;
	size_t logicalPaletteSize = 0;

	RETURN_ERR(getLogicalPalette(logicalPalettePointer, logicalPaletteSize));
	
	if (!logicalPalettePointer) {
		return kMoaErr_InternalError;
	}

	if (!logicalPaletteSize) {
		return kMoaErr_BadParam;
	}

	GlobalHandleLock<char>::GlobalHandle paletteGlobalHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, logicalPaletteSize);
	RETURN_ERR(osErr(paletteGlobalHandle));

	paletteGlobalHandleLockPointer = std::make_unique<GlobalHandleLock<char>>(paletteGlobalHandle);
	RETURN_NULL(paletteGlobalHandleLockPointer);

	// RIFF is word-aligned
	const DWORD WORD_SIZE = sizeof(WORD);

	MMIOINFO mmioinfo = {};
	mmioinfo.pchBuffer = paletteGlobalHandleLockPointer->get();
	mmioinfo.cchBuffer = logicalPaletteSize;
	mmioinfo.fccIOProc = FOURCC_MEM;
	*mmioinfo.adwInfo = WORD_SIZE;
	return makeMMIO(logicalPalettePointer, logicalPaletteSize, mmioinfo);
}

Formats::WinPALETTEFormat::WinPALETTEFormat(HPALETTE paletteHandle, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer)
	: paletteHandle(
		paletteHandle
	),

	Format(
		productVersionMajor
	),

	MemoryFormat(
		productVersionMajor,
		callocInterfacePointer
	) {
	if (!paletteHandle) {
		throw std::invalid_argument("paletteHandle must not be NULL");
	}
}

Formats::WinPALETTEFormat::~WinPALETTEFormat() {
	destroy();
}

MoaError Formats::WinPALETTEFormat::writeFile(bool agent, PIMoaFile writeFileInterfacePointer) {
	RETURN_NULL(writeFileInterfacePointer);

	setInterface((PPMoaVoid)&this->writeFileInterfacePointer, writeFileInterfacePointer);
	RETURN_NULL(this->writeFileInterfacePointer);

	if (agent) {
		return kMoaErr_NoErr;
	}

	std::unique_ptr<char[]> logicalPalettePointer = NULL;
	size_t logicalPaletteSize = 0;

	RETURN_ERR(getLogicalPalette(logicalPalettePointer, logicalPaletteSize));
	
	if (!logicalPalettePointer) {
		return kMoaErr_InternalError;
	}

	if (!logicalPaletteSize) {
		return kMoaErr_BadParam;
	}

	MoaSystemFileSpec sysSpec = "";
	const MoaLong SYS_SPEC_SIZE = sizeof(sysSpec);

	RETURN_ERR(this->writeFileInterfacePointer->GetSysSpec(sysSpec, SYS_SPEC_SIZE));

	CA2W sysSpecWide(sysSpec, CP_DIRECTOR(productVersionMajor));

	MoaError err = kMoaErr_NoErr;

	// we use CreateFile here to circumvent MMIO's 128 character filename limit
	// the file should already be created, so we use TRUNCATE_EXISTING instead
	// of CREATE_ALWAYS because it doesn't complain about hidden files
	HANDLE file = CreateFileW(sysSpecWide, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	SCOPE_EXIT {
		err = errOrDefaultErr(osErr(closeHandle(file)), err);
	};

	RETURN_ERR(osErr(file));

	// this implementation calls makeMMIO instead of get
	MMIOINFO mmioinfo = {};
	mmioinfo.fccIOProc = FOURCC_DOS;
	*mmioinfo.adwInfo = (DWORD)file;

	RETURN_ERR(makeMMIO(logicalPalettePointer, logicalPaletteSize, mmioinfo));
	return err;
}

MoaError Formats::WinPALETTEFormat::get(PMoaVoid &data, MoaUlong &size) const {
	std::unique_ptr<GlobalHandleLock<char>> paletteGlobalHandleLockPointer = 0;
	RETURN_ERR(getPaletteGlobalHandleLock(paletteGlobalHandleLockPointer));
	
	if (!paletteGlobalHandleLockPointer) {
		return kMoaErr_InternalError;
	}

	data = paletteGlobalHandleLockPointer->get();
	size = paletteGlobalHandleLockPointer->size();

	RETURN_NULL(data);
	return duplicateMemory(data, size, callocInterfacePointer);
}

MoaError Formats::WinPALETTEFormat::get(PIMoaStream writeStreamInterfacePointer, MoaUlong &size) const {
	RETURN_NULL(writeStreamInterfacePointer);

	std::unique_ptr<GlobalHandleLock<char>> paletteGlobalHandleLockPointer = 0;
	RETURN_ERR(getPaletteGlobalHandleLock(paletteGlobalHandleLockPointer));
	
	if (!paletteGlobalHandleLockPointer) {
		return kMoaErr_InternalError;
	}

	PMoaVoid data = paletteGlobalHandleLockPointer->get();
	size = paletteGlobalHandleLockPointer->size();

	RETURN_NULL(data);
	return writeStreamSafe(data, size, writeStreamInterfacePointer);
}
#endif

Formats::CompositeFormat::CompositeFormat(MoaHandle handle, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: HandleLockFormat(
		handle,
		productVersionMajor,
		handleInterfacePointer,
		callocInterfacePointer
	),
	
	Format(
		productVersionMajor
	) {
}

Formats::CompositeFormat::~CompositeFormat() {
}

MoaError Formats::CompositeFormat::get(PMoaVoid &data, MoaUlong &size) const {
	RETURN_ERR(HandleLockFormat::get(data, size));
	RETURN_NULL(data);

	#ifdef WINDOWS
	DrFixMediaHeader((const char*)data, size);
	#endif
	return kMoaErr_NoErr;
}

void Formats::MemberPropertyFormat::destroy() {
	releaseValue(memberPropertyValue, mmValueInterfacePointer);

	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
}

Formats::MemberPropertyFormat::MemberPropertyFormat(ConstPMoaMmValue memberPropertyValuePointer, unsigned long productVersionMajor, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: Format(
		productVersionMajor
	),
	
	mmValueInterfacePointer(
		mmValueInterfacePointer
	),

	MemoryFormat(
		productVersionMajor,
		callocInterfacePointer
	) {
	if (!memberPropertyValuePointer) {
		throw std::invalid_argument("memberPropertyValuePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->AddRef();

	memberPropertyValue = *memberPropertyValuePointer;
	mmValueInterfacePointer->ValueAddRef(&memberPropertyValue);
}

Formats::MemberPropertyFormat::~MemberPropertyFormat() {
	destroy();
}

MoaError Formats::MemberPropertyFormat::get(PMoaVoid &data, MoaUlong &size) const {
	// this does not include the null terminator
	MoaLong stringLength = 0;
	RETURN_ERR(mmValueInterfacePointer->ValueStringLength(&memberPropertyValue, &stringLength));

	// we don't want the output file to include the null terminator either
	// but the buffer must be long enough to recieve the string with it
	size = stringLength;
	MoaUlong bufferLength = size + 1;
	data = callocInterfacePointer->NRAlloc(bufferLength);

	MAKE_SCOPE_EXIT(freeMemoryDataScopeExit) {
		freeMemory(data, callocInterfacePointer);
	};

	RETURN_NULL(data);
	RETURN_ERR(mmValueInterfacePointer->ValueToString(&memberPropertyValue, (PMoaChar)data, bufferLength));
	freeMemoryDataScopeExit.dismiss();
	return kMoaErr_NoErr;
}

void Formats::MemberPropertyPictureFormat::destroy() {
	releaseInterface((PPMoaVoid)&handleInterfacePointer);
	releaseInterface((PPMoaVoid)&drMediaValueInterfacePointer);
}

Formats::MemberPropertyPictureFormat::MemberPropertyPictureFormat(PMoaMmValue memberPropertyValuePointer, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaDrMediaValue drMediaValueInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: MemberPropertyFormat(
		memberPropertyValuePointer,
		productVersionMajor,
		mmValueInterfacePointer,
		callocInterfacePointer
	),
	
	Format(
		productVersionMajor
	),

	handleInterfacePointer(
		handleInterfacePointer
	),

	drMediaValueInterfacePointer(
		drMediaValueInterfacePointer
	) {
	if (!handleInterfacePointer) {
		throw std::invalid_argument("handleInterfacePointer must not be NULL");
	}

	if (!drMediaValueInterfacePointer) {
		throw std::invalid_argument("drMediaValueInterfacePointer must not be NULL");
	}

	handleInterfacePointer->AddRef();
	drMediaValueInterfacePointer->AddRef();
}

Formats::MemberPropertyPictureFormat::~MemberPropertyPictureFormat() {
	destroy();
}

MoaError Formats::MemberPropertyPictureFormat::get(PMoaVoid &data, MoaUlong &size) const {
	MoaHandle memberPropertyHandle = NULL;
	RETURN_ERR(drMediaValueInterfacePointer->ValueToPicture(&memberPropertyValue, &memberPropertyHandle));
	RETURN_NULL(memberPropertyHandle);

	PMoaVoid memberPropertyHandleLock = handleInterfacePointer->Lock(memberPropertyHandle);
	RETURN_NULL(memberPropertyHandleLock);

	// important: we don't own, do not free! (will cause crash)
	SCOPE_EXIT {
		handleInterfacePointer->Unlock(memberPropertyHandle);
	};

	MoaLong memberPropertyHandleSize = handleInterfacePointer->GetSize(memberPropertyHandle);

	size = PICT_HEADER_SIZE + memberPropertyHandleSize;
	data = callocInterfacePointer->NRAlloc(size);

	MAKE_SCOPE_EXIT(freeMemoryDataScopeExit) {
		freeMemory(data, callocInterfacePointer);
	};

	RETURN_NULL(data);

	memset(data, 0, PICT_HEADER_SIZE);

	if (memcpy_s((void*)((const char*)data + PICT_HEADER_SIZE), size - PICT_HEADER_SIZE, memberPropertyHandleLock, memberPropertyHandleSize)) {
		return kMoaErr_OutOfMem;
	}

	freeMemoryDataScopeExit.dismiss();
	return kMoaErr_NoErr;
}

void Formats::XtraMediaFormat::destroy() {
	releaseInterface((PPMoaVoid)&mmXAssetInterfacePointer);
}

Formats::XtraMediaFormat::XtraMediaFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor)
	: mmXAssetInterfacePointer(mmXAssetInterfacePointer),
	Format(productVersionMajor) {
	if (!mmXAssetInterfacePointer) {
		throw std::invalid_argument("mmXAssetInterfacePointer must not be NULL");
	}

	mmXAssetInterfacePointer->AddRef();
}

Formats::XtraMediaFormat::XtraMediaFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, const std::string &tempFileExtension, bool pathRelative)
	: mmXAssetInterfacePointer(
		mmXAssetInterfacePointer
	),

	Format(
		productVersionMajor,
		tempFileExtension,
		pathRelative
	) {
	if (!mmXAssetInterfacePointer) {
		throw std::invalid_argument("mmXAssetInterfacePointer must not be NULL");
	}

	mmXAssetInterfacePointer->AddRef();
}

Formats::XtraMediaFormat::~XtraMediaFormat() {
	destroy();
}

MoaError Formats::XtraMediaFormat::writeFile(bool agent, PIMoaFile writeFileInterfacePointer) {
	RETURN_NULL(writeFileInterfacePointer);

	setInterface((PPMoaVoid)&this->writeFileInterfacePointer, writeFileInterfacePointer);
	RETURN_NULL(this->writeFileInterfacePointer);

	if (agent) {
		return kMoaErr_NoErr;
	}

	MoaUlong size = 0;
	RETURN_ERR(mmXAssetInterfacePointer->GetStreamOutMediaSize(&size));

	MoaError err = kMoaErr_NoErr;

	if (size) {
		PIMoaStream streamInterfacePointer = NULL;
		RETURN_ERR(getClosedFileStream(this->writeFileInterfacePointer, streamInterfacePointer));
		RETURN_NULL(streamInterfacePointer);

		SCOPE_EXIT {
			err = errOrDefaultErr(closeStream(streamInterfacePointer), err);
		};

		RETURN_ERR(mmXAssetInterfacePointer->StreamOutMedia(streamInterfacePointer));
	}
	return err;
}

void Formats::XtraMediaMemoryFormat::destroy() {
	releaseInterface((PPMoaVoid)&callbackInterfacePointer);
}

MoaError Formats::XtraMediaMemoryFormat::getStream(Stream &stream, MoaUlong &size) const {
	RETURN_ERR(mmXAssetInterfacePointer->GetStreamOutMediaSize(&size));

	// StreamOutMedia should only be called if size is not zero
	// (docs say greater than zero or equal to -1, but this is unsigned, so that's every non-zero value?)
	if (size) {
		PIMoaStream streamInterfacePointer = stream.getStreamInterfacePointer();

		SCOPE_EXIT {
			releaseInterface((PPMoaVoid)&streamInterfacePointer);
		};
		
		if (!streamInterfacePointer) {
			return kMoaErr_InternalError;
		}

		RETURN_ERR(mmXAssetInterfacePointer->StreamOutMedia(streamInterfacePointer));
		RETURN_ERR(stream.resetPosition());
	}
	return seekStream(stream, size);
}

MoaError Formats::XtraMediaMemoryFormat::determineSize(Stream &stream, MoaUlong &size) const {
	// get the determinate size if it's unknown
	if (size == -1) {
		RETURN_ERR(stream.getEnd(size));
	}
	return kMoaErr_NoErr;
}

MoaError Formats::XtraMediaMemoryFormat::seekStream(Stream &stream, MoaUlong &size) const {
	return determineSize(stream, size);
}

Formats::XtraMediaMemoryFormat::XtraMediaMemoryFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: XtraMediaFormat(
		mmXAssetInterfacePointer,
		productVersionMajor
	),

	Format(
		productVersionMajor
	),

	callbackInterfacePointer(
		callbackInterfacePointer
	),

	MemoryFormat(
		productVersionMajor,
		callocInterfacePointer
	) {
	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	callbackInterfacePointer->AddRef();
}

Formats::XtraMediaMemoryFormat::~XtraMediaMemoryFormat() {
	destroy();
}

MoaError Formats::XtraMediaMemoryFormat::writeFile(bool agent, PIMoaFile writeFileInterfacePointer) {
	RETURN_NULL(writeFileInterfacePointer);

	setInterface((PPMoaVoid)&this->writeFileInterfacePointer, writeFileInterfacePointer);
	RETURN_NULL(this->writeFileInterfacePointer);

	if (agent) {
		return kMoaErr_NoErr;
	}

	// this implementation calls getStream instead of get
	Stream stream(callbackInterfacePointer);
	MoaUlong size = 0;

	RETURN_ERR(getStream(stream, size));

	MoaError err = kMoaErr_NoErr;

	PIMoaStream writeStreamInterfacePointer = NULL;
	RETURN_ERR(getOpenFileStream(this->writeFileInterfacePointer, writeStreamInterfacePointer));
	RETURN_NULL(writeStreamInterfacePointer);

	SCOPE_EXIT {
		err = errOrDefaultErr(closeStream(writeStreamInterfacePointer), err);
	};

	RETURN_ERR(stream.copy(writeStreamInterfacePointer, size));
	return err;
}

MoaError Formats::XtraMediaMemoryFormat::get(PMoaVoid &data, MoaUlong &size) const {
	Stream stream(callbackInterfacePointer);

	MoaError err = getStream(stream, size);

	if (err != kMoaErr_NoErr) {
		return kMoaMmErr_StreamOutFailed;
	}

	data = callocInterfacePointer->NRAlloc(size);

	MAKE_SCOPE_EXIT(freeMemoryDataScopeExit) {
		freeMemory(data, callocInterfacePointer);
	};

	RETURN_NULL(data);

	err = stream.readSafe(data, size);

	if (err != kMoaErr_NoErr) {
		return kMoaMmErr_StreamOutFailed;
	}

	freeMemoryDataScopeExit.dismiss();
	return err;
}

MoaError Formats::XtraMediaMemoryFormat::get(MoaUlong &size, PIMoaStream writeStreamInterfacePointer) const {
	RETURN_NULL(writeStreamInterfacePointer);

	Stream stream(callbackInterfacePointer);
	RETURN_ERR(getStream(stream, size));
	return stream.copy(writeStreamInterfacePointer, size);
}

MoaError Formats::XtraMediaSWFFormat::seekStream(Stream &stream, MoaUlong &size) const {
	// this is (as close as possible) a recreation of the behaviour
	// of the Flash Asset's StreamInMedia method
	// first, read a ChunkID
	CHUNK_ID chunkID = 0;
	const MoaStreamCount CHUNK_ID_SIZE = sizeof(chunkID);

	RETURN_ERR(stream.readSafe(&chunkID, CHUNK_ID_SIZE));

	// little endian, so we need to byteswap it on Mac
	#ifdef MACINTOSH
	chunkID = mem_XLong(chunkID);
	#endif

	// mask it to get just three bytes
	chunkID &= 0x00FFFFFF;

	// check if the ChunkID is SWF or SWC
	const CHUNK_ID SWF_CHUNK_ID = 0x00535746;
	const CHUNK_ID SWC_CHUNK_ID = 0x00535743;

	if (chunkID == SWF_CHUNK_ID || chunkID == SWC_CHUNK_ID) {
		// if it is, the entire stream is the SWF
		RETURN_ERR(stream.resetPosition());
		return determineSize(stream, size);
	}

	// this value is 1 if the SWF is embedded, FLLK if it's linked
	// it is unused when reading the asset stream
	uint32_t unused = 0;
	const MoaStreamCount UNUSED_SIZE = sizeof(unused);

	RETURN_ERR(stream.readSafe(&unused, UNUSED_SIZE));

	// the SWF size
	const MoaStreamCount SIZE_SIZE = sizeof(size);

	RETURN_ERR(stream.readSafe(&size, SIZE_SIZE));

	// big endian, so we need to byteswap it on Windows
	#ifdef WINDOWS
	size = mem_XLong(size);
	#endif
	return kMoaErr_NoErr;
}

Formats::XtraMediaSWFFormat::XtraMediaSWFFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: XtraMediaMemoryFormat(
		mmXAssetInterfacePointer,
		productVersionMajor,
		callbackInterfacePointer,
		callocInterfacePointer
	),

	Format(
		productVersionMajor
	) {
}

Formats::XtraMediaSWFFormat::~XtraMediaSWFFormat() {
}

MoaError Formats::XtraMediaW3DFormat::seekStream(Stream &stream, MoaUlong &size) const {
	// this is (as close as possible) a recreation of the behaviour
	// of the Shockwave 3D Asset's StreamInMedia method
	// first, read a ChunkID
	CHUNK_ID chunkID = 0;
	const MoaStreamCount CHUNK_ID_SIZE = sizeof(chunkID);

	RETURN_ERR(stream.readSafe(&chunkID, CHUNK_ID_SIZE));

	// big endian, so we need to byteswap it on Windows
	#ifdef WINDOWS
	chunkID = mem_XLong(chunkID);
	#endif

	// check if the ChunkID is 3DEM
	// it may also be 3DMD if the W3D is linked
	// however, this is not checked when reading the asset stream
	// it doesn't need to, because the minimum W3D size is 16 bytes
	// if the W3D size is less than 16 bytes, it is invalid
	// the 3DMD ChunkID is likely just a placeholder
	// so that the stream is not empty
	const CHUNK_ID THREEDEM_CHUNK_ID = 0x3344454D;
	const uint32_t MIN_W3D_SIZE = 16;

	if (chunkID == THREEDEM_CHUNK_ID) {
		// the next two values are unused when reading the asset stream
		// I suspect they are only used for integration with 3DS Max
		// the size and constant value 0x20000000 look suspiciously similar to
		// the ID tags used in the 3DS model format
		uint32_t unused = 0;
		const MoaStreamCount UNUSED_SIZE = sizeof(unused);

		RETURN_ERR(stream.readSafe(&unused, UNUSED_SIZE));
		RETURN_ERR(stream.readSafe(&unused, UNUSED_SIZE));

		// the W3D size
		const MoaStreamCount SIZE_SIZE = sizeof(size);

		RETURN_ERR(stream.readSafe(&size, SIZE_SIZE));

		#ifdef WINDOWS
		size = mem_XLong(size);
		#endif
		return size < MIN_W3D_SIZE ? kMoaMmErr_StreamOutFailed : kMoaErr_NoErr;
	}

	// if it isn't, the entire stream is the W3D
	RETURN_ERR(stream.resetPosition());
	RETURN_ERR(determineSize(stream, size));
	return size < MIN_W3D_SIZE ? kMoaMmErr_StreamOutFailed : kMoaErr_NoErr;
}

Formats::XtraMediaW3DFormat::XtraMediaW3DFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: XtraMediaMemoryFormat(
		mmXAssetInterfacePointer,
		productVersionMajor,
		callbackInterfacePointer,
		callocInterfacePointer
	),

	Format(
		productVersionMajor
	) {
}

Formats::XtraMediaW3DFormat::~XtraMediaW3DFormat() {
}

void Formats::XtraMediaMixerAsyncFormat::destroy() {
	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
	releaseInterface((PPMoaVoid)&callbackInterfacePointer);
	releaseInterface((PPMoaVoid)&callocInterfacePointer);

	MoaError err = closeStream(writeStreamInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Close Stream");
	}
}

MoaError Formats::XtraMediaMixerAsyncFormat::swapTempFile(PIMoaFile tempFileInterfacePointer) {
	RETURN_NULL(tempFileInterfacePointer);

	if (!swapFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	MoaError err = kMoaErr_NoErr;

	SCOPE_EXIT {
		err = errOrDefaultErr(deleteTempFile(tempFileInterfacePointer), err);
	};

	// this thankfully doesn't allow swapping with directories (that would be very bad)
	SCOPE_EXIT {
		err = errOrDefaultErr(tempFileInterfacePointer->SwapFile(swapFileInterfacePointer), err);
	};

	RETURN_ERR(closeStream(writeStreamInterfacePointer));
	return err;
}

MoaError Formats::XtraMediaMixerAsyncFormat::deleteSwapFile() {
	if (!swapFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&swapFileInterfacePointer);
	};

	MoaError err = kMoaErr_NoErr;

	// don't delete an existing file but do delete e.g. an incremented file
	SCOPE_EXIT {
		if (!replacedExistingFile) {
			err = errOrDefaultErr(swapFileInterfacePointer->Delete(), err);
		}
	};
	
	RETURN_ERR(closeStream(writeStreamInterfacePointer));
	return err;
}

MoaError Formats::XtraMediaMixerAsyncFormat::getSymbols() {
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Save", &symbols.Save));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Stop", &symbols.Stop));
	return kMoaErr_NoErr;
}

// for mixers an absolute path is required
Formats::XtraMediaMixerAsyncFormat::XtraMediaMixerAsyncFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, const std::string &tempFileExtension, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: XtraMediaFormat(
		mmXAssetInterfacePointer,
		productVersionMajor,
		tempFileExtension,
		false
	),

	Format(
		productVersionMajor,
		tempFileExtension,
		false
	),

	drCastMemInterfacePointer(
		drCastMemInterfacePointer
	),

	mmValueInterfacePointer(
		mmValueInterfacePointer
	),

	callbackInterfacePointer(
		callbackInterfacePointer
	),

	callocInterfacePointer(
		callocInterfacePointer
	) {
	if (!drCastMemInterfacePointer) {
		throw std::invalid_argument("drCastMemInterfacePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	if (!callocInterfacePointer) {
		throw std::invalid_argument("callocInterfacePointer must not be NULL");
	}

	drCastMemInterfacePointer->AddRef();
	mmValueInterfacePointer->AddRef();
	callbackInterfacePointer->AddRef();
	callocInterfacePointer->AddRef();

	MoaError err = getSymbols();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Symbols");
	}
}

Formats::XtraMediaMixerAsyncFormat::~XtraMediaMixerAsyncFormat() {
	destroy();
}

MoaError Formats::XtraMediaMixerAsyncFormat::writeFile(bool agent, PIMoaFile writeFileInterfacePointer) {
	RETURN_NULL(writeFileInterfacePointer);

	// invalid for a mixer
	if (agent) {
		return kMoaErr_BadParam;
	}

	// we need to create a temporary file
	// with the correct extension so the mixer saves to
	// the specified format
	// regardless of if we're replacing an existing file or not
	if (!replacedExistingFile) {
		RETURN_ERR(fileErr(createTempFile(writeFileInterfacePointer)));
	}

	setInterface((PPMoaVoid)&this->writeFileInterfacePointer, writeFileInterfacePointer);
	RETURN_NULL(this->writeFileInterfacePointer);

	if (!swapFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	// should not already exist
	if (writeStreamInterfacePointer) {
		return kMoaErr_InternalError;
	}

	// we don't actually want to write to this stream, just hold it open
	RETURN_ERR(getOpenFileStream(swapFileInterfacePointer, writeStreamInterfacePointer));
	RETURN_NULL(writeStreamInterfacePointer);

	// CallFunction demands that the first argument be left empty
	const MoaLong ARGS_SIZE = 2;
	MoaMmValue args[ARGS_SIZE] = { kVoidMoaMmValueInitializer, kVoidMoaMmValueInitializer };

	MoaChar pathnameSpec[MOA_MAX_PATHNAME] = "";
	RETURN_ERR(this->writeFileInterfacePointer->GetPathnameSpec(pathnameSpec, MOA_MAX_PATHNAME));

	MoaMmValue &pathValue = args[1];

	SCOPE_EXIT {
		releaseValue(pathValue, mmValueInterfacePointer);
	};

	// we set the path on the file interface as the final step
	// so we don't delete the wrong file in the event of an error in makeFile
	RETURN_ERR(mmValueInterfacePointer->StringToValue(pathnameSpec, &pathValue));
	RETURN_ERR(drCastMemInterfacePointer->CallFunction(symbols.Save, ARGS_SIZE, args, NULL));

	// this is so that we don't redundantly call Stop if we aren't saving
	// which has the different behaviour of stopping the mixer's playback
	saveStatus = kMoaStatus_False;

	#ifdef WINDOWS
	//if (tempFile) {
	// set the temporary file as hidden
	// this must be done after calling save, which errors if the file is hidden
	// (probably because CreateFile also does)
	// on other platforms this is not necessary, the tilde marks the file as hidden (I think)
	// this is Windows specific so we don't need to seperately call GetSysSpec
	RETURN_ERR(setFileAttributeHiddenWide(true, CA2W(pathnameSpec, CP_DIRECTOR(productVersionMajor))));
	//}
	#endif
	return kMoaErr_NoErr;
}

MoaError Formats::XtraMediaMixerAsyncFormat::cancelFile() {
	MoaError err = kMoaErr_NoErr;

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&writeFileInterfacePointer);
	};

	// delete our file if we managed to create one
	// (we can always do this here because this is never called on the mixer thread)
	SCOPE_EXIT {
		if (writeFileInterfacePointer) {
			err = errOrDefaultErr(writeFileInterfacePointer->Delete(), err);
		}
	};

	SCOPE_EXIT {
		err = errOrDefaultErr(deleteSwapFile(), err);
	};

	// don't call Stop if we didn't call Save first
	if (saveStatus == kMoaStatus_False) {
		const MoaLong ARGS_SIZE = 1;
		MoaMmValue args[ARGS_SIZE] = { kVoidMoaMmValueInitializer };

		RETURN_ERR(drCastMemInterfacePointer->CallFunction(symbols.Stop, ARGS_SIZE, args, NULL));

		saveStatus = kMoaStatus_OK;
	}
	return err;
}

MoaError Formats::XtraMediaMixerAsyncFormat::swapFile(bool status) {
	// return a status
	if (status) {
		return saveStatus;
	}

	// now we expect the swap file to exist
	if (!swapFileInterfacePointer) {
		return kMoaErr_InternalError;
	}

	// we're on the mixer thread, don't return a status
	MoaError err = kMoaErr_NoErr;

	// delete the swap file if we fail to swap with it
	// (Format::swapFile handles deleting the temp file)
	MAKE_SCOPE_EXIT(deleteSwapFileInterfacePointerScopeExit) {
		err = errOrDefaultErr(deleteSwapFile(), err);
	};

	RETURN_ERR(Format::swapFile(swapFileInterfacePointer));
	deleteSwapFileInterfacePointerScopeExit.dismiss();
	return err;
}

MoaError Formats::XtraMediaMixerAsyncFormat::replaceExistingFile(PIMoaFile fileInterfacePointer) {
	RETURN_NULL(fileInterfacePointer);

	// don't set the file as hidden yet!
	RETURN_ERR(createTempFile(fileInterfacePointer));

	// we always create a temporary file, but need this for bookkeeping of what to delete
	replacedExistingFile = true;
	return kMoaErr_NoErr;
}

Formats::XtraMediaMixerWAVAsyncFormat::XtraMediaMixerWAVAsyncFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: XtraMediaMixerAsyncFormat(
		mmXAssetInterfacePointer,
		productVersionMajor,
		"wav",
		drCastMemInterfacePointer,
		mmValueInterfacePointer,
		callbackInterfacePointer,
		callocInterfacePointer
	),

	Format(
		productVersionMajor,
		"wav",
		false
	) {
}

Formats::XtraMediaMixerMP4AsyncFormat::XtraMediaMixerMP4AsyncFormat(PIMoaMmXAsset mmXAssetInterfacePointer, unsigned long productVersionMajor, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: XtraMediaMixerAsyncFormat(
		mmXAssetInterfacePointer,
		productVersionMajor,
		"mp4",
		drCastMemInterfacePointer,
		mmValueInterfacePointer,
		callbackInterfacePointer,
		callocInterfacePointer
	),

	Format(
		productVersionMajor,
		"mp4",
		false
	) {
}

Formats::Format::POINTER Formats::FormatFactory::createMediaInfoFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaCalloc callocInterfacePointer) {
	if (!(labelType & Label::TYPE_MEDIA_INFO)) {
		return 0;
	}

	if (labelType & Label::TYPE_MEDIA_INFO_GLOBAL_HANDLE) {
		return std::make_shared<GlobalHandleLockFormat<>>((GlobalHandleLock<>::GlobalHandle)mediaData, productVersionMajor, callocInterfacePointer);
	}

	#ifdef WINDOWS
	if (labelType & Label::TYPE_MEDIA_INFO_WIN_DIB) {
		return std::make_shared<WinDIBFormat>((GlobalHandleLock<BITMAPINFO>::GlobalHandle)mediaData, productVersionMajor, callocInterfacePointer);
	}

	if (labelType & Label::TYPE_MEDIA_INFO_WIN_PALETTE) {
		return std::make_shared<WinPALETTEFormat>((HPALETTE)mediaData, productVersionMajor, callocInterfacePointer);
	}
	#endif

	if (labelType & Label::TYPE_MEDIA_INFO_COMPOSITE) {
		return std::make_shared<CompositeFormat>((MoaHandle)mediaData, productVersionMajor, handleInterfacePointer, callocInterfacePointer);
	}
	return std::make_shared<HandleLockFormat<>>((MoaHandle)mediaData, productVersionMajor, handleInterfacePointer, callocInterfacePointer);
}

#ifdef MACINTOSH
Formats::Format::POINTER Formats::FormatFactory::createMediaInfoFormat(Label::TYPE labelType, bool resource, GlobalHandleLock<>::GlobalHandle mediaData, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer) {
	if (!(labelType & Label::TYPE_MEDIA_INFO)) {
		return 0;
	}

	if (labelType & Label::TYPE_MEDIA_INFO_GLOBAL_HANDLE) {
		return std::make_shared<GlobalHandleLockFormat<>>(resource, mediaData, productVersionMajor, callocInterfacePointer);
	}
	return 0;
}
#endif

#ifdef WINDOWS
Formats::Format::POINTER Formats::FormatFactory::createMediaInfoFormat(Label::TYPE labelType, HMODULE moduleHandle, HRSRC resourceHandle, unsigned long productVersionMajor, PIMoaCalloc callocInterfacePointer) {
	if (!(labelType & Label::TYPE_MEDIA_INFO)) {
		return 0;
	}

	if (labelType & Label::TYPE_MEDIA_INFO_GLOBAL_HANDLE) {
		return std::make_shared<GlobalHandleLockFormat<>>(moduleHandle, resourceHandle, productVersionMajor, callocInterfacePointer);
	}

	if (labelType & Label::TYPE_MEDIA_INFO_WIN_DIB) {
		return std::make_shared<WinDIBFormat>(moduleHandle, resourceHandle, productVersionMajor, callocInterfacePointer);
	}
	return 0;
}
#endif

Formats::Format::POINTER Formats::FormatFactory::createMemberPropertyFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer) {
	if (!(labelType & Label::TYPE_MEMBER_PROPERTY)) {
		return 0;
	}
	return std::make_shared<MemberPropertyFormat>((PMoaMmValue)mediaData, productVersionMajor, mmValueInterfacePointer, callocInterfacePointer);
}

Formats::Format::POINTER Formats::FormatFactory::createMemberPropertyFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaHandle handleInterfacePointer, PIMoaDrMediaValue drMediaValueInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCalloc callocInterfacePointer) {
	if (!(labelType & Label::TYPE_MEMBER_PROPERTY)) {
		return 0;
	}

	if (labelType & Label::TYPE_MEMBER_PROPERTY_PICTURE) {
		return std::make_shared<MemberPropertyPictureFormat>((PMoaMmValue)mediaData, productVersionMajor, handleInterfacePointer, drMediaValueInterfacePointer, mmValueInterfacePointer, callocInterfacePointer);
	}
	return 0;
}

Formats::Format::POINTER Formats::FormatFactory::createXtraMediaFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer) {
	if (!(labelType & Label::TYPE_XTRA_MEDIA)) {
		return 0;
	}

	if (labelType & Label::TYPE_XTRA_MEDIA_SWF) {
		return std::make_shared<XtraMediaSWFFormat>((PIMoaMmXAsset)mediaData, productVersionMajor, callbackInterfacePointer, callocInterfacePointer);
	}

	if (labelType & Label::TYPE_XTRA_MEDIA_W3D) {
		return std::make_shared<XtraMediaW3DFormat>((PIMoaMmXAsset)mediaData, productVersionMajor, callbackInterfacePointer, callocInterfacePointer);
	}
	return std::make_shared<XtraMediaFormat>((PIMoaMmXAsset)mediaData, productVersionMajor);
}

Formats::Format::POINTER Formats::FormatFactory::createXtraMediaFormat(Label::TYPE labelType, PMoaVoid mediaData, unsigned long productVersionMajor, PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer) {
	if (!(labelType & Label::TYPE_XTRA_MEDIA)) {
		return 0;
	}

	if (labelType & Label::TYPE_XTRA_MEDIA_MIXER_ASYNC) {
		if (labelType & Label::TYPE_XTRA_MEDIA_MIXER_WAV_ASYNC) {
			return std::make_shared<XtraMediaMixerWAVAsyncFormat>((PIMoaMmXAsset)mediaData, productVersionMajor, drCastMemInterfacePointer, mmValueInterfacePointer, callbackInterfacePointer, callocInterfacePointer);
		}

		if (labelType & Label::TYPE_XTRA_MEDIA_MIXER_MP4_ASYNC) {
			return std::make_shared<XtraMediaMixerMP4AsyncFormat>((PIMoaMmXAsset)mediaData, productVersionMajor, drCastMemInterfacePointer, mmValueInterfacePointer, callbackInterfacePointer, callocInterfacePointer);
		}
	}
	return 0;
}