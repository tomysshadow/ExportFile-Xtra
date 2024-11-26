#include "Media.h"

void Media::DirectorMedia::Content::destroy() {
	releaseInterface((PPMoaVoid)&readerInterfacePointer);
	releaseInterface((PPMoaVoid)&readerRegistryEntryDictInterfacePointer);
	releaseInterface((PPMoaVoid)&dataObjectInterfacePointer);
}

/*
void Media::DirectorMedia::Content::duplicate(const Content &content) {
	setInterface((PPMoaVoid)&readerInterfacePointer, content.readerInterfacePointer);
	setInterface((PPMoaVoid)&readerRegistryEntryDictInterfacePointer, content.readerRegistryEntryDictInterfacePointer);
	setInterface((PPMoaVoid)&dataObjectInterfacePointer, content.dataObjectInterfacePointer);

	formatPointer = content.formatPointer;
	streamOptional = content.streamOptional;
}
*/

Media::DirectorMedia::Content::Content() {
}

Media::DirectorMedia::Content::~Content() {
	destroy();
}

/*
Media::DirectorMedia::Content::Content(const Content &content) {
	duplicate(content);
}

Media::DirectorMedia::Content &Media::DirectorMedia::Content::operator=(const Content &content) {
	if (this == &content) {
		return *this;
	}

	duplicate(content);
	return *this;
}
*/

PIMoaReader Media::DirectorMedia::Content::getReaderInterfacePointer() const {
	return (PIMoaReader)getInterface(readerInterfacePointer);
}

PIMoaRegistryEntryDict Media::DirectorMedia::Content::getReaderRegistryEntryDictInterfacePointer() const {
	return (PIMoaRegistryEntryDict)getInterface(readerRegistryEntryDictInterfacePointer);
}

PIMoaDataObject Media::DirectorMedia::Content::getDataObjectInterfacePointer() const {
	return (PIMoaDataObject)getInterface(dataObjectInterfacePointer);
}

void Media::DirectorMedia::Content::setReaderInterfacePointer(PIMoaReader readerInterfacePointer) {
	setInterface((PPMoaVoid)&this->readerInterfacePointer, readerInterfacePointer);
}

void Media::DirectorMedia::Content::setReaderRegistryEntryDictInterfacePointer(PIMoaRegistryEntryDict readerRegistryEntryDictInterfacePointer) {
	setInterface((PPMoaVoid)&this->readerRegistryEntryDictInterfacePointer, readerRegistryEntryDictInterfacePointer);
}

void Media::DirectorMedia::Content::setDataObjectInterfacePointer(PIMoaDataObject dataObjectInterfacePointer) {
	setInterface((PPMoaVoid)&this->dataObjectInterfacePointer, dataObjectInterfacePointer);
}

Media::DirectorMedia::DirectorMedia(Label::Info::MAP::const_iterator labelInfoNotFound)
	: labelInfoMapIterator(labelInfoNotFound) {
}

void Media::MixerMedia::destroy() {
	releaseInterface((PPMoaVoid)&drMovieContextInterfacePointer);
	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
	releaseInterface((PPMoaVoid)&registryEntryDictInterfacePointer);
	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
}

void Media::MixerMedia::duplicate(const MixerMedia &mixerMedia) {
	setInterface((PPMoaVoid)&drMovieContextInterfacePointer, mixerMedia.drMovieContextInterfacePointer);
	setInterface((PPMoaVoid)&drCastMemInterfacePointer, mixerMedia.drCastMemInterfacePointer);
	setInterface((PPMoaVoid)&registryEntryDictInterfacePointer, mixerMedia.registryEntryDictInterfacePointer);
	setInterface((PPMoaVoid)&mmValueInterfacePointer, mixerMedia.mmValueInterfacePointer);

	isSavingSymbol = mixerMedia.isSavingSymbol;

	formatPointer = mixerMedia.formatPointer;

	#ifdef WINDOWS
	// note: destroy should not free the library from this module handle!
	moduleHandle = mixerMedia.moduleHandle;
	window = mixerMedia.window;
	#endif
}

Media::MixerMedia::MixerMedia(const ExportFileValueConverter &exportFileValueConverter, PIMoaDrMovie drMovieInterfacePointer, PIMoaMmValue mmValueInterfacePointer)
	: exportFileValueConverter(
		exportFileValueConverter
	),

	lingo(
		drMovieInterfacePointer,
		mmValueInterfacePointer
	),

	mmValueInterfacePointer(
		mmValueInterfacePointer
	) {
	if (!drMovieInterfacePointer) {
		throw std::invalid_argument("drMovieInterfacePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->AddRef();

	MoaError err = mmValueInterfacePointer->StringToSymbol("IsSaving", &isSavingSymbol);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Convert String To Symbol");
	}

	// may be null if Director version doesn't need it
	err = drMovieInterfacePointer->QueryInterface(&IID_IMoaDrMovieContext, (PPMoaVoid)&drMovieContextInterfacePointer);

	if (err != kMoaErr_NoErr) {
		drMovieContextInterfacePointer = NULL;
	}
}

Media::MixerMedia::MixerMedia(const ExportFileValueConverter &exportFileValueConverter, PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer)
	: exportFileValueConverter(
		exportFileValueConverter
	),

	lingo(
		drPlayerInterfacePointer,
		mmValueInterfacePointer
	),

	mmValueInterfacePointer(
		mmValueInterfacePointer
	) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->AddRef();

	MoaError err = mmValueInterfacePointer->StringToSymbol("IsSaving", &isSavingSymbol);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Convert String To Symbol");
	}

	PIMoaDrMovie drMovieInterfacePointer = lingo.getDrMovieInterfacePointer();

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&drMovieInterfacePointer);
	};

	if (!drMovieInterfacePointer) {
		throw std::logic_error("drMovieInterfacePointer must not be NULL");
	}

	// may be null if Director version doesn't need it
	err = drMovieInterfacePointer->QueryInterface(&IID_IMoaDrMovieContext, (PPMoaVoid)&drMovieContextInterfacePointer);

	if (err != kMoaErr_NoErr) {
		drMovieContextInterfacePointer = NULL;
	}
}

Media::MixerMedia::~MixerMedia() {
	destroy();
}

Media::MixerMedia::MixerMedia(const MixerMedia &mixerMedia)
	: exportFileValueConverter(mixerMedia.exportFileValueConverter),
	lingo(mixerMedia.lingo) {
	duplicate(mixerMedia);
}

Media::MixerMedia &Media::MixerMedia::operator=(const MixerMedia &mixerMedia) {
	if (this == &mixerMedia) {
		return *this;
	}

	exportFileValueConverter = mixerMedia.exportFileValueConverter;

	lingo = mixerMedia.lingo;

	duplicate(mixerMedia);
	return *this;
}

PIMoaDrMovieContext Media::MixerMedia::getDrMovieContextInterfacePointer() const {
	return (PIMoaDrMovieContext)getInterface(drMovieContextInterfacePointer);
}

PIMoaDrCastMem Media::MixerMedia::getDrCastMemInterfacePointer() const {
	return (PIMoaDrCastMem)getInterface(drCastMemInterfacePointer);
}

PIMoaRegistryEntryDict Media::MixerMedia::getRegistryEntryDictInterfacePointer() const {
	return (PIMoaRegistryEntryDict)getInterface(registryEntryDictInterfacePointer);
}

PIMoaMmValue Media::MixerMedia::getMmValueInterfacePointer() const {
	return (PIMoaMmValue)getInterface(mmValueInterfacePointer);
}

MoaMmSymbol Media::MixerMedia::getIsSavingSymbol() const {
	return isSavingSymbol;
}

void Media::MixerMedia::setDrCastMemInterfacePointer(PIMoaDrCastMem drCastMemInterfacePointer) {
	setInterface((PPMoaVoid)&this->drCastMemInterfacePointer, drCastMemInterfacePointer);
}

void Media::MixerMedia::setRegistryEntryDictInterfacePointer(PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
	setInterface((PPMoaVoid)&this->registryEntryDictInterfacePointer, registryEntryDictInterfacePointer);
}

void Media::MixerMedia::Lingo::destroy() {
	releaseValue(defaultResultValue, mmValueInterfacePointer);
	releaseValue(defaultErrCodeValue, mmValueInterfacePointer);

	releaseInterface((PPMoaVoid)&drMovieInterfacePointer);
	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
}

void Media::MixerMedia::Lingo::duplicate(const Lingo &lingo) {
	setInterface((PPMoaVoid)&drMovieInterfacePointer, lingo.drMovieInterfacePointer);
	setInterface((PPMoaVoid)&mmValueInterfacePointer, lingo.mmValueInterfacePointer);

	setValue(defaultResultValue, lingo.defaultResultValue, mmValueInterfacePointer);
	setValue(defaultErrCodeValue, lingo.defaultErrCodeValue, mmValueInterfacePointer);

	symbols = lingo.symbols;
}

MoaError Media::MixerMedia::Lingo::getSymbols() {
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("MixerSaved", &symbols.MixerSaved));
	return kMoaErr_NoErr;
}

MoaError Media::MixerMedia::Lingo::getDefaultValues() {
	const MoaLong DEFAULT_RESULT_VALUE = IS_ERROR(kMoaErr_InternalError);
	const MoaLong DEFAULT_ERR_CODE_VALUE = HRESULT_CODE(kMoaErr_InternalError);

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(DEFAULT_RESULT_VALUE, &defaultResultValue));
	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(DEFAULT_ERR_CODE_VALUE, &defaultErrCodeValue));
	return kMoaErr_NoErr;
}

Media::MixerMedia::Lingo::Lingo(PIMoaDrMovie drMovieInterfacePointer, PIMoaMmValue mmValueInterfacePointer)
	: drMovieInterfacePointer(drMovieInterfacePointer),
	mmValueInterfacePointer(mmValueInterfacePointer) {
	if (!drMovieInterfacePointer) {
		throw std::invalid_argument("drMovieInterfacePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	drMovieInterfacePointer->AddRef();
	mmValueInterfacePointer->AddRef();

	MoaError err = getSymbols();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Symbols");
	}

	err = getDefaultValues();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Default Err Code Value");
	}
}

Media::MixerMedia::Lingo::Lingo(PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer)
	: mmValueInterfacePointer(mmValueInterfacePointer) {
	if (!drPlayerInterfacePointer) {
		throw std::invalid_argument("drPlayerInterfacePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	MoaError err = drPlayerInterfacePointer->GetActiveMovie(&drMovieInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Active Movie");
	}

	mmValueInterfacePointer->AddRef();

	err = getSymbols();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Symbols");
	}

	err = getDefaultValues();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Default Err Code Value");
	}
}

Media::MixerMedia::Lingo::~Lingo() {
	destroy();
}

Media::MixerMedia::Lingo::Lingo(const Lingo &lingo) {
	duplicate(lingo);
}

Media::MixerMedia::Lingo &Media::MixerMedia::Lingo::operator=(const Lingo &lingo) {
	if (this == &lingo) {
		return *this;
	}

	duplicate(lingo);
	return *this;
}

void Media::MixerMedia::Lingo::callHandler(MoaMmValue &memberValue, MoaError mixerSavedCallHandlerThreadErr) {
	MoaLong result = !IS_ERROR(mixerSavedCallHandlerThreadErr);

	MoaMmValue resultValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(resultValue, mmValueInterfacePointer);
	};

	MoaError err = mmValueInterfacePointer->IntegerToValue(result, &resultValue);

	MoaLong errCode = HRESULT_CODE(mixerSavedCallHandlerThreadErr);

	MoaMmValue errCodeValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(errCodeValue, mmValueInterfacePointer);
	};

	MoaError err2 = mmValueInterfacePointer->IntegerToValue(errCode, &errCodeValue);

	// default values so this will theoretically NEVER fail
	const MoaLong ARGS_SIZE = 3;
	MoaMmValue args[ARGS_SIZE] = { memberValue, err == kMoaErr_NoErr ? resultValue : defaultResultValue, err2 == kMoaErr_NoErr ? errCodeValue : defaultErrCodeValue };

	// don't care if this fails, handler may not be defined
	drMovieInterfacePointer->CallHandler(symbols.MixerSaved, ARGS_SIZE, args, NULL);
}

PIMoaDrMovie Media::MixerMedia::Lingo::getDrMovieInterfacePointer() const {
	return (PIMoaDrMovie)getInterface(drMovieInterfacePointer);
}

#ifdef WINDOWS
void Media::WinBMPMedia::destroy() {
	if (!unmapMappedView(mappedView)) {
		throw std::runtime_error("Failed to Unmap Mapped View");
	}

	if (!closeHandle(fileMapping)) {
		throw std::runtime_error("Failed to Close Handle");
	}
}

MoaError Media::WinBMPMedia::allocateSourceBitmap(PIMoaReceptorPixels receptorPixelsInterfacePointer, PIMoaStream readStreamInterfacePointer) {
	if (sourceBitmapFileHeaderOptional.has_value() && sourceBitmapInfoPointer) {
		return kMoaErr_NoErr;
	}

	MAKE_SCOPE_EXIT(sourceBitmapHeaderFileOptionalScopeExit) {
		sourceBitmapFileHeaderOptional = std::nullopt;
	};

	MAKE_SCOPE_EXIT(sourceBitmapInfoPointerScopeExit) {
		sourceBitmapInfoPointer = 0;
	};

	// first, the source bitmap file header
	// we read it here (but don't validate it yet, as we don't need to use it yet)
	// this has the offset to the bits which will be useful for later
	sourceBitmapFileHeaderOptional.emplace();
	BITMAPFILEHEADER &sourceBitmapFileHeader = sourceBitmapFileHeaderOptional.value();

	const MoaStreamCount SOURCE_BITMAP_FILE_HEADER_SIZE = sizeof(sourceBitmapFileHeader);

	RETURN_ERR(readStreamSafe(&sourceBitmapFileHeader, SOURCE_BITMAP_FILE_HEADER_SIZE, readStreamInterfacePointer));

	MoaStreamPosition sourceBitmapInfoPosition = 0;

	// this method shouldn't assume the stream was set to position zero
	RETURN_ERR(readStreamInterfacePointer->GetPosition(&sourceBitmapInfoPosition));

	// assume at first that there is no color table
	// I don't like writing the word color without a u by the way
	// I'm just doing so for consistency's sake
	sourceBitmapInfoSize = BITMAPINFO_SIZE;
	sourceBitmapInfoPointer = std::shared_ptr<char[]>(new char[sourceBitmapInfoSize]);
	RETURN_NULL(sourceBitmapInfoPointer);

	{
		// in its own scope because this might become invalid later
		// we need this ugly get call because this is technically a char pointer
		BITMAPINFOHEADER &sourceBitmapInfoHeader = ((PBITMAPINFO)sourceBitmapInfoPointer.get())->bmiHeader;

		const MoaStreamCount SOURCE_BITMAP_INFO_HEADER_SIZE = sizeof(sourceBitmapInfoHeader);

		RETURN_ERR(readStreamSafe(&sourceBitmapInfoHeader, SOURCE_BITMAP_INFO_HEADER_SIZE, readStreamInterfacePointer));

		// using the source bitmap info, see if there is a color table
		if (!getBitmapInfoColorsSize(sourceBitmapInfoHeader, true, sourceBitmapInfoColorsSize)) {
			return ERR_CANNOT_READ;
		}
	}

	if (sourceBitmapInfoColorsSize) {
		// if there's a color table, it isn't included in the size of the bitmap info header
		// so we need to reallocate this
		RETURN_ERR(readStreamInterfacePointer->SetPosition(sourceBitmapInfoPosition));

		sourceBitmapInfoSize += sourceBitmapInfoColorsSize;
		sourceBitmapInfoPointer = std::shared_ptr<char[]>(new char[sourceBitmapInfoSize]);
		RETURN_NULL(sourceBitmapInfoPointer);

		// read in the color table also
		RETURN_ERR(readStreamSafe((PMoaVoid)sourceBitmapInfoPointer.get(), sourceBitmapInfoSize, readStreamInterfacePointer));
	}

	sourceBitmapHeaderFileOptionalScopeExit.dismiss();
	sourceBitmapInfoPointerScopeExit.dismiss();
	return kMoaErr_NoErr;
}

bool Media::WinBMPMedia::rgbX(RGBTRIPLE* rgbTriplePointer, DWORD stride, DWORD imageSize) {
	if (!rgbTriplePointer) {
		return false;
	}

	RGBTRIPLE* endPointer = (RGBTRIPLE*)((LPBYTE)rgbTriplePointer + imageSize) - 1;

	if (!endPointer) {
		return false;
	}

	RGBTRIPLE* rowPointer = NULL;

	COLORREF colorRef = 0;
	MoaMmRGBTriple* mmRGBTriplePointer = NULL;

	while (rgbTriplePointer <= endPointer) {
		rowPointer = (RGBTRIPLE*)((LPBYTE)rgbTriplePointer + stride) - 1;

		if (!rowPointer) {
			return false;
		}

		while (rgbTriplePointer <= rowPointer) {
			colorRef = RGB(rgbTriplePointer->rgbtRed, rgbTriplePointer->rgbtGreen, rgbTriplePointer->rgbtBlue);

			mmRGBTriplePointer = (MoaMmRGBTriple*)rgbTriplePointer;

			if (!mmRGBTriplePointer) {
				return false;
			}

			WinToMoaRGB(colorRef, mmRGBTriplePointer);

			rgbTriplePointer = rgbTriplePointer + 1;

			if (!rgbTriplePointer) {
				return false;
			}
		}

		rgbTriplePointer = rowPointer + 1;

		if (!rgbTriplePointer) {
			return false;
		}
	}
	return true;
}

bool Media::WinBMPMedia::rgbaX(RGBQUAD* rgbQuadPointer, DWORD stride, DWORD imageSize) {
	if (!rgbQuadPointer) {
		return false;
	}

	RGBQUAD* endPointer = (RGBQUAD*)((LPBYTE)rgbQuadPointer + imageSize) - 1;

	if (!endPointer) {
		return false;
	}

	RGBQUAD* rowPointer = NULL;

	COLORREF colorRef = 0;
	MoaMmRGBTriple* mmRGBTriplePointer = NULL;

	while (rgbQuadPointer <= endPointer) {
		rowPointer = (RGBQUAD*)((LPBYTE)rgbQuadPointer + stride) - 1;

		if (!rowPointer) {
			return false;
		}

		while (rgbQuadPointer <= rowPointer) {
			colorRef = RGB(rgbQuadPointer->rgbRed, rgbQuadPointer->rgbGreen, rgbQuadPointer->rgbBlue);

			mmRGBTriplePointer = (MoaMmRGBTriple*)rgbQuadPointer;

			if (!mmRGBTriplePointer) {
				return false;
			}

			WinToMoaRGB(colorRef, mmRGBTriplePointer);

			rgbQuadPointer = rgbQuadPointer + 1;

			if (!rgbQuadPointer) {
				return false;
			}
		}

		rgbQuadPointer = rowPointer + 1;

		if (!rgbQuadPointer) {
			return false;
		}
	}
	return true;
}

DWORD Media::WinBMPMedia::getStride(ULONG absWidth, WORD bitCount) {
	return (((absWidth * bitCount) + 31) & ~31) >> 3;
}

bool Media::WinBMPMedia::getImageSize(MoaLong colorSpace, ULONG absWidth, ULONG absHeight, MoaLong &rowBytes, DWORD &imageSize) {
	rowBytes = 0;
	imageSize = 0;

	#if defined READ_RPCS_INDEXED_RGB || defined READ_RPCS_RGB16
	typedef std::map<MoaLong, WORD> BIT_COUNT_MAP;

	const BIT_COUNT_MAP _BIT_COUNT_MAP = {
		{ RPCS_INDEXED_RGB, 8 },
		{ RPCS_RGB16, 16 },
		{ RPCS_RGB, 24 },
		{ RPCS_RGBA, 32 }
	};

	BIT_COUNT_MAP::const_iterator foundBitCount = _BIT_COUNT_MAP.find(colorSpace);

	if (foundBitCount == _BIT_COUNT_MAP.end()) {
		return false;
	}

	WORD bitCount = foundBitCount->second;
	#else
	WORD bitCount = colorSpace == RPCS_RGBA ? 32 : 24;
	#endif

	DWORD stride = getStride(absWidth, bitCount);

	// (do not directly reuse rowBytes in calculations, it is a signed integer)
	rowBytes = stride;
	imageSize = absHeight * stride;
	return true;
}

bool Media::WinBMPMedia::getSamplesPerPixel(MoaLong colorSpace, MoaShort &samplesPerPixel) {
	samplesPerPixel = 0;

	#if defined READ_RPCS_INDEXED_RGB || defined READ_RPCS_RGB16
	typedef std::map<MoaLong, MoaShort> SAMPLES_PER_PIXEL_MAP;

	const SAMPLES_PER_PIXEL_MAP _SAMPLES_PER_PIXEL_MAP = {
		{ RPCS_INDEXED_RGB, 1 },
		{ RPCS_RGB16, 3 },
		{ RPCS_RGB, 3 },
		{ RPCS_RGBA, 4 }
	};

	SAMPLES_PER_PIXEL_MAP::const_iterator foundSamplesPerPixel = _SAMPLES_PER_PIXEL_MAP.find(colorSpace);

	if (foundSamplesPerPixel == _SAMPLES_PER_PIXEL_MAP.end()) {
		return false;
	}

	samplesPerPixel = foundSamplesPerPixel->second;
	#else
	samplesPerPixel = colorSpace == RPCS_RGBA ? 4 : 3;
	#endif
	return true;
}

int Media::WinBMPMedia::getCoordinate(int dimension) {
	int dimensionFlipped = -dimension;
	return max(0, dimensionFlipped);
}

bool Media::WinBMPMedia::getColorTableIndexedRGB(BITMAPINFO &bitmapInfo, MoaPixelFormat &pixelFormat) {
	//#ifdef READ_RPCS_INDEXED_RGB
	if (pixelFormat.cs.colorSpace != RPCS_INDEXED_RGB) {
		return false;
	}

	DWORD colorsUsed = 0;

	if (!getBitmapInfoColorsUsedRGB(bitmapInfo.bmiHeader, false, colorsUsed)) {
		return false;
	}

	const MoaLong ENTRY_SIZE = sizeof(TMoaCTableEntry);

	if (!ENTRY_SIZE) {
		return false;
	}

	const MoaLong ENTRIES_SIZE = sizeof(pixelFormat.cs.colorTable.Entry);
	const MoaLong NUM_ENTRIES_ALLOC = ENTRIES_SIZE / ENTRY_SIZE;

	pixelFormat.cs.colorTable.Header.iNumEntries = colorsUsed;
	pixelFormat.cs.colorTable.Header.iNumEntriesAlloc = NUM_ENTRIES_ALLOC;

	// the math works out such that there should be enough space allocated for every entry
	// but maybe the source bitmap is invalid here
	if (pixelFormat.cs.colorTable.Header.iNumEntries > pixelFormat.cs.colorTable.Header.iNumEntriesAlloc) {
		return false;
	}

	for (DWORD i = 0; i < colorsUsed; i++) {
		TMoaCTableEntry &cTableEntry = pixelFormat.cs.colorTable.Entry[i];
		RGBQUAD &colors = bitmapInfo.bmiColors[i];

		// the documentation dictates that, for 8-bit indexed images
		// both the upper and lower byte of the word should be the same value
		// (for platform reasons, I suppose?)
		// I'm guessing the upper byte is otherwise used for paletted RGBA
		// which we don't provide anyway
		#define SET_COLOR_COMPONENT(colorComponent, color) do {\
			colorComponent = ((MoaUshort)(color) << 8) | (color);\
		} while(0)

		SET_COLOR_COMPONENT(cTableEntry.Red, colors.rgbRed);
		SET_COLOR_COMPONENT(cTableEntry.Gre, colors.rgbGreen);
		SET_COLOR_COMPONENT(cTableEntry.Blu, colors.rgbBlue);
	}
	//#endif
	return true;
}

bool Media::WinBMPMedia::getBitmapInfoColorsUsedRGB(const BITMAPINFOHEADER &bitmapInfoHeader, bool allocation, DWORD &colorsUsed) {
	colorsUsed = 0;

	if (!validateBitmapInfoHeader(bitmapInfoHeader)) {
		return false;
	}

	if (!allocation && bitmapInfoHeader.biCompression != BI_RGB) {
		return false;
	}

	if (bitmapInfoHeader.biBitCount <= 8) {
		colorsUsed = bitmapInfoHeader.biClrUsed;

		if (!colorsUsed) {
			colorsUsed = (DWORD)pow(2, bitmapInfoHeader.biBitCount);
		}
	}
	return true;
}

bool Media::WinBMPMedia::validateBitmapFileHeader(const BITMAPFILEHEADER &bitmapFileHeader, DWORD end) {
	return bitmapFileHeader.bfType == TYPE
	&& end > bitmapFileHeader.bfOffBits; // must actually be greater, so not empty
}

bool Media::WinBMPMedia::validateBitmapInfoHeader(const BITMAPINFOHEADER &bitmapInfoHeader) {
	return bitmapInfoHeader.biSize >= BITMAPINFOHEADER_SIZE // may be larger for newer versions
	&& (
		bitmapInfoHeader.biCompression != BI_BITFIELDS // if compression is bitfields, must be 16-bit or 32-bit
		|| bitmapInfoHeader.biBitCount == 16
		|| bitmapInfoHeader.biBitCount == 32
	);
}

bool Media::WinBMPMedia::getBitmapInfoColorsSize(const BITMAPINFOHEADER &bitmapInfoHeader, bool allocation, DWORD &colorsSize) {
	colorsSize = 0;

	if (!validateBitmapInfoHeader(bitmapInfoHeader)) {
		return false;
	}

	// for allocations, we ignore the compression if the bits per pixel is less than or equal to eight
	// this is because some components assume there is always a color table in that case
	// and allocating more memory than we actually need prevents out of bounds references in those components
	// however, in reality there isn't actually a color table there, so otherwise we don't add to the size
	if ((allocation || bitmapInfoHeader.biCompression == BI_RGB) && bitmapInfoHeader.biBitCount <= 8) {
		DWORD colorsUsed = 0;
		
		if (!getBitmapInfoColorsUsedRGB(bitmapInfoHeader, allocation, colorsUsed)) {
			return false;
		}

		const DWORD RGBQUAD_SIZE = sizeof(RGBQUAD);

		colorsSize = colorsUsed * RGBQUAD_SIZE;
		return true;
	}

	if (bitmapInfoHeader.biCompression == BI_BITFIELDS) {
		const DWORD DWORD_SIZE = sizeof(DWORD);
		const DWORD COLOR_MASKS_SIZE = DWORD_SIZE * 3;

		colorsSize = COLOR_MASKS_SIZE;
	}
	return true;
}

Media::WinBMPMedia::WinBMPMedia() {
}

Media::WinBMPMedia::~WinBMPMedia() {
	destroy();
}

MoaError Media::WinBMPMedia::getPixelFormat(PIMoaReceptorPixels receptorPixelsInterfacePointer) {
	RETURN_NULL(receptorPixelsInterfacePointer);

	if (pixelFormatOptional.has_value()) {
		return kMoaErr_NoErr;
	}

	if (!sourceBitmapInfoPointer) {
		return kMoaErr_InternalError;
	}

	BITMAPINFO &sourceBitmapInfo = *(PBITMAPINFO)sourceBitmapInfoPointer.get();
	BITMAPINFOHEADER &sourceBitmapInfoHeader = sourceBitmapInfo.bmiHeader;

	if (!validateBitmapInfoHeader(sourceBitmapInfoHeader)) {
		return ERR_CANNOT_READ;
	}

	MAKE_SCOPE_EXIT(pixelFormatOptionalScopeExit) {
		pixelFormatOptional = std::nullopt;
	};

	pixelFormatOptional.emplace();
	MoaPixelFormat &pixelFormat = pixelFormatOptional.value();

	MoaLong colorSpaceCount = 1;

	// default list if bitdepth not recognized (default to 24-bit)
	MoaLong COLOR_SPACES[] = { RPCS_RGB, RPCS_RGB };

	if (sourceBitmapInfoHeader.biCompression == BI_RGB) {
		if (sourceBitmapInfoHeader.biBitCount == 32) {
			// RGBA available
			colorSpaceCount++;
			COLOR_SPACES[0] = RPCS_RGBA;
		} else {
			// the RPCS_INDEXED_RGB mode works, but
			// PNG Writer incorrectly misinterprets the data as 24-bit data anyway
			// so we don't provide it
			// (this doesn't prevent creating 8-bit bitmaps, but the Writer Xtra must downscale it itself)
			#ifdef READ_RPCS_INDEXED_RGB
			if (sourceBitmapInfoHeader.biBitCount <= 8) {
				colorSpaceCount++;
				COLOR_SPACES[0] = RPCS_INDEXED_RGB;
			}
			#endif
		}
	}

	// no thumbnail size, only the canonical size available
	const MoaLong DIMENSION_COUNT = 1;

	ULONG absSourceWidth = abs(sourceBitmapInfoHeader.biWidth);
	pixelFormat.dim.pixels.x = absSourceWidth;

	ULONG absSourceHeight = abs(sourceBitmapInfoHeader.biHeight);
	pixelFormat.dim.pixels.y = absSourceHeight;

	// both should ideally be provided
	// (PNG Writer just gives up if there's no TOP_DOWN option)
	const MoaLong DIRECTIONS_SIZE = 2;
	MoaLong DIRECTIONS[DIRECTIONS_SIZE] = { BOTTOM_UP, TOP_DOWN };

	RETURN_ERR(
		receptorPixelsInterfacePointer->InquireFormat(
			colorSpaceCount,
			COLOR_SPACES,
			DIMENSION_COUNT,
			&pixelFormat.dim.pixels,
			FALSE,
			DIRECTIONS_SIZE,
			DIRECTIONS
		)
	);

	// the colors duke, the colors!!!
	size_t colorsBitmapInfoSize = sourceBitmapInfoSize;
	std::shared_ptr<char[]> colorsBitmapInfoPointer(new char[colorsBitmapInfoSize]);
	RETURN_NULL(colorsBitmapInfoPointer);

	DWORD colorsSize = sourceBitmapInfoColorsSize;

	{
		BITMAPINFO &colorsBitmapInfo = *(PBITMAPINFO)colorsBitmapInfoPointer.get();

		// start with the same bitmap info as the source
		if (memcpy_s(&colorsBitmapInfo, colorsBitmapInfoSize, &sourceBitmapInfo, sourceBitmapInfoSize)) {
			return kMoaErr_OutOfMem;
		}

		BITMAPINFOHEADER &colorsBitmapInfoHeader = colorsBitmapInfo.bmiHeader;

		if (!validateBitmapInfoHeader(colorsBitmapInfoHeader)) {
			return ERR_CANNOT_READ;
		}

		// the requested color space is filled in here by InquireFormat
		pixelFormat.cs.colorSpace = *COLOR_SPACES;

		colorsBitmapInfoHeader.biCompression = BI_RGB;

		#ifdef READ_RPCS_INDEXED_RGB
		if (pixelFormat.cs.colorSpace == RPCS_INDEXED_RGB) {
			// indexed RGB requested
			// (1-bit, 2-bit, and 4-bit are generally expected to be rounded up to 8-bit)
			colorsBitmapInfoHeader.biBitCount = 8;

			// the bit count used for the implicit calculation may be changing
			// correct it if necessary
			if (colorsBitmapInfoHeader.biBitCount != sourceBitmapInfoHeader.biBitCount && !colorsBitmapInfoHeader.biClrUsed) {
				colorsBitmapInfoHeader.biClrUsed = (DWORD)pow(2, sourceBitmapInfoHeader.biBitCount);
			}
		} else {
		#endif
		colorsBitmapInfoHeader.biClrUsed = 0;
		colorsBitmapInfoHeader.biClrImportant = 0;
		colorsBitmapInfoHeader.biBitCount = pixelFormat.cs.colorSpace == RPCS_RGBA ? 32 : 24;
		#ifdef READ_RPCS_INDEXED_RGB
		}
		#endif

		// the colors size may have changed now
		if (!getBitmapInfoColorsSize(colorsBitmapInfoHeader, true, colorsSize)) {
			return ERR_CANNOT_READ;
		}

		if (colorsSize == sourceBitmapInfoColorsSize) {
			bitmapInfoSize = colorsBitmapInfoSize;
			bitmapInfoPointer = colorsBitmapInfoPointer;
		} else {
			bitmapInfoSize = BITMAPINFO_SIZE + colorsSize;
			bitmapInfoPointer = std::shared_ptr<char[]>(new char[bitmapInfoSize]);

			((PBITMAPINFO)bitmapInfoPointer.get())->bmiHeader = colorsBitmapInfoHeader;
		}

		RETURN_NULL(bitmapInfoPointer);

		colorsBitmapInfoPointer = 0;
	}

	BITMAPINFOHEADER &bitmapInfoHeader = ((PBITMAPINFO)bitmapInfoPointer.get())->bmiHeader;

	direction = *DIRECTIONS;

	// note: don't flip absSourceHeight directly, it's unsigned
	bitmapInfoHeader.biHeight = absSourceHeight;

	if (direction == TOP_DOWN) {
		bitmapInfoHeader.biHeight = -bitmapInfoHeader.biHeight;
	}

	#ifdef READ_RPCS_INDEXED_RGB
	if (pixelFormat.cs.colorSpace == RPCS_INDEXED_RGB) {
		if (!getColorTableIndexedRGB()) {
			return kMoaErr_InternalError;
		}
	}
	#endif

	if (!getImageSize(pixelFormat.cs.colorSpace, absSourceWidth, absSourceHeight, pixelFormat.dim.rowBytes, imageSize)) {
		return ERR_CANNOT_READ;
	}

	// only correct the header if necessary
	// this is an optimization because if the headers match, we
	// don't need to copy the image, so we want to make the minimum
	// number of required corrections for it to still be right
	// if the old size was zero it can be left implicit, but
	// if it was defined, we must correct it to be valid
	if (bitmapInfoHeader.biSizeImage) {
		bitmapInfoHeader.biSizeImage = imageSize;
	}

	if (!getSamplesPerPixel(pixelFormat.cs.colorSpace, pixelFormat.cs.samplesPerPixel)) {
		return ERR_CANNOT_READ;
	}

	#if defined READ_RPCS_RGB16
	pixelFormat.cs.colorSpace == RPCS_RGB16 ? 5 : 8
	#else
	pixelFormat.cs.bitsPerSample = 8;
	#endif

	pixelFormat.cs.transparentIdx = -1;
	pixelFormat.cs.flags = pixelFormat.cs.colorSpace == RPCS_RGBA ? RPCSFLAGS_TRANSPARENT : RPCSFLAGS_NONE;

	// can't be an integer divide: must be able to round down OR up to nearest number
	const double METERS_TO_INCHES = 0.0254;

	pixelFormat.dim.resolution.x = (MoaCoord)round(METERS_TO_INCHES * bitmapInfoHeader.biXPelsPerMeter);
	pixelFormat.dim.resolution.y = (MoaCoord)round(METERS_TO_INCHES * bitmapInfoHeader.biYPelsPerMeter);

	pixelFormatOptionalScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError Media::WinBMPMedia::getMappedView(PIMoaReceptorPixels receptorPixelsInterfacePointer, PIMoaStream readStreamInterfacePointer) {
	//RETURN_NULL(receptorPixelsInterfacePointer); // may be NULL
	RETURN_NULL(readStreamInterfacePointer);

	// don't recreate the mapped view if it already exists
	if (fileMapping) {
		return kMoaErr_NoErr;
	}

	if (mappedView) {
		return kMoaErr_NoErr;
	}

	// stream may not necessarily start at position zero
	RETURN_ERR(readStreamInterfacePointer->SetPosition(0));

	// this is important to do first for CanRead
	RETURN_ERR(allocateSourceBitmap(receptorPixelsInterfacePointer, readStreamInterfacePointer));

	// get in and validate the source bitmap file header
	if (!sourceBitmapFileHeaderOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	BITMAPFILEHEADER &sourceBitmapFileHeader = sourceBitmapFileHeaderOptional.value();

	MoaStreamPosition end = 0;
	RETURN_ERR(readStreamInterfacePointer->GetEnd(&end));

	if (!validateBitmapFileHeader(sourceBitmapFileHeader, end)) {
		return ERR_CANNOT_READ;
	}

	if (!sourceBitmapInfoPointer) {
		return kMoaErr_InternalError;
	}

	BITMAPINFO &sourceBitmapInfo = *(PBITMAPINFO)sourceBitmapInfoPointer.get();
	BITMAPINFOHEADER &sourceBitmapInfoHeader = sourceBitmapInfo.bmiHeader;

	if (!validateBitmapInfoHeader(sourceBitmapInfoHeader)) {
		return ERR_CANNOT_READ;
	}

	// code for CanRead jumps off here
	if (!receptorPixelsInterfacePointer) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR(getPixelFormat(receptorPixelsInterfacePointer));

	if (!bitmapInfoPointer) {
		return kMoaErr_InternalError;
	}

	BITMAPINFO &bitmapInfo = *(PBITMAPINFO)bitmapInfoPointer.get();
	BITMAPINFOHEADER &bitmapInfoHeader = bitmapInfo.bmiHeader;

	if (!validateBitmapInfoHeader(bitmapInfoHeader)) {
		return ERR_CANNOT_READ;
	}

	// next we'll read in the bitmap bits
	RETURN_ERR(readStreamInterfacePointer->SetPosition(sourceBitmapFileHeader.bfOffBits));

	if (!pixelFormatOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	MoaPixelFormat &pixelFormat = pixelFormatOptional.value();

	DWORD stride = pixelFormat.dim.rowBytes;

	// this value should've been set explicitly by getPixelFormat
	if (!imageSize) {
		return kMoaErr_InternalError;
	}

	// we create an extra row on the end of the file
	// several Writers crash unless there are some padding bytes on the end of this buffer
	// unfortunately, because GDI's default behaviour creates an image of the exact correct size
	// we have to do this ourselves
	DWORD fileMappingSize = stride + imageSize;

	fileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, fileMappingSize, NULL);
	RETURN_ERR(osErr(fileMapping));

	MoaError err = kMoaErr_NoErr;

	// if the bitmap has the same header as the source bitmap
	// and does not require flipping
	// then we can just use the source, and don't need to copy it
	// (this usually happens for RGBA)
	bool sourceBitmap = bitmapInfoSize == sourceBitmapInfoSize
		&& memoryEquals(bitmapInfoPointer.get(), sourceBitmapInfoPointer.get(), bitmapInfoSize);

	if (!sourceBitmap) {
		HDC compatibleDCHandle = CreateCompatibleDC(NULL);

		SCOPE_EXIT {
			err = errOrDefaultErr(osErr(deleteGDIDC(compatibleDCHandle)), err);
		};

		RETURN_ERR(osErr(compatibleDCHandle));

		// note: it is important this bitmap lives in this scope
		// this way, it'll be deleted (and its internal mapped view closed)
		// before we create our own mapped view to the same file mapping
		// so we don't use double the memory that we need
		HBITMAP bitmapHandle = NULL;

		SCOPE_EXIT {
			err = errOrDefaultErr(osErr(deleteGDIObject(*(HGDIOBJ*)&bitmapHandle)), err);
		};

		{
			// we don't use this bitsPointer, we use a mapped view later instead
			// so we throw it away in this scope
			PVOID bitsPointer = NULL;

			bitmapHandle = CreateDIBSection(
				compatibleDCHandle,
				&bitmapInfo,
				DIB_RGB_COLORS,
				&bitsPointer,
				fileMapping,
				0
			);

			RETURN_ERR(osErr(bitmapHandle));
			//RETURN_NULL(bitsPointer); // doesn't matter, bitsPointer goes unused
		}

		HGDIOBJ replacedGDIObject = SelectObject(compatibleDCHandle, bitmapHandle);

		SCOPE_EXIT {
			err = errOrDefaultErr(osErr(deselectGDIObject(compatibleDCHandle, replacedGDIObject)), err);
		};

		RETURN_ERR(osErr(replacedGDIObject));

		// the Source Compatible DC (explicitly the same device as the Compatible DC)
		HDC sourceCompatibleDCHandle = CreateCompatibleDC(compatibleDCHandle);

		SCOPE_EXIT {
			err = errOrDefaultErr(osErr(deleteGDIDC(sourceCompatibleDCHandle)), err);
		};

		RETURN_ERR(osErr(sourceCompatibleDCHandle));

		PVOID sourceBitsPointer = NULL;

		HBITMAP sourceBitmapHandle = CreateDIBSection(
			sourceCompatibleDCHandle,
			&sourceBitmapInfo,
			DIB_RGB_COLORS,
			&sourceBitsPointer,
			NULL,
			0
		);

		SCOPE_EXIT {
			err = errOrDefaultErr(osErr(deleteGDIObject(*(HGDIOBJ*)&sourceBitmapHandle)), err);
		};

		RETURN_ERR(osErr(sourceBitmapHandle));
		RETURN_NULL(sourceBitsPointer);
	
		// get the source bits size as determined by GDI
		// we can calculate this ourselves using the stride, but it's kind of sketch
		// I don't like relying on the fact that GDI and my code will
		// definitely calculate it the same way, when it's implementation is
		// never explicitly stated in the documentation
		// (particularly for compressed bitmaps)
		// the secret sauce here is that sourceBitsPointer must actually be
		// a mapped view, since it originates from a file mapping
		// even if we pass NULL, like we do for the source
		// it creates its own internal file mapping
		// so because it is a mapped view, we can use VirtualQuery to get the size
		MEMORY_BASIC_INFORMATION memoryBasicInformation = {};
		const SIZE_T MEMORY_BASIC_INFORMATION_SIZE = sizeof(memoryBasicInformation);

		RETURN_ERR(
			osErr(
				VirtualQuery(
					sourceBitsPointer,
					&memoryBasicInformation,
					MEMORY_BASIC_INFORMATION_SIZE
				)

				== MEMORY_BASIC_INFORMATION_SIZE
				&& memoryBasicInformation.State == MEM_COMMIT
				&& memoryBasicInformation.Protect
				&& memoryBasicInformation.BaseAddress == sourceBitsPointer
				&& memoryBasicInformation.RegionSize
			)
		);

		// partial reads are allowed here
		// the stream may also be smaller than the buffer GDI created
		// (particularly if it's compressed, GDI's size estimate is - I'd hope - the upper limit)
		// it may also have EOF Extra Data that isn't actually part of the bitmap
		// but that's fine, GDI's calculated size is authoritative
		MoaStreamCount numberOfBytesRead = 0;
		RETURN_ERR(readStreamPartial(sourceBitsPointer, memoryBasicInformation.RegionSize, numberOfBytesRead, readStreamInterfacePointer));

		HGDIOBJ sourceReplacedGDIObject = SelectObject(sourceCompatibleDCHandle, sourceBitmapHandle);

		SCOPE_EXIT {
			err = errOrDefaultErr(osErr(deselectGDIObject(sourceCompatibleDCHandle, sourceReplacedGDIObject)), err);
		};

		RETURN_ERR(osErr(sourceReplacedGDIObject));

		// this is where the main magic happens
		// this call is multipurpose
		// it'll flip the image from bottom up to top down or vice versa
		// (whichever format the Receptor requested)
		// and convert the color depth as desired
		// it's also hardware accelerated when possible! This is great for batch conversions
		// important: we use the source x/y for both arguments
		// because the height is already flipped in the bitmap info as necessary
		// if we didn't use the source height here, they would cancel
		int sourceX = getCoordinate(sourceBitmapInfoHeader.biWidth);
		int sourceY = getCoordinate(sourceBitmapInfoHeader.biHeight);

		RETURN_ERR(
			osErr(
				BitBlt(
					compatibleDCHandle,
					sourceX,
					sourceY,
					sourceBitmapInfoHeader.biWidth,
					sourceBitmapInfoHeader.biHeight,
					sourceCompatibleDCHandle,
					sourceX,
					sourceY,
					SRCCOPY
				)
			)
		);

		RETURN_ERR(osErr(GdiFlush()));
	}

	// important to return any GDI errors from the scope exits
	RETURN_ERR(err);

	mappedView = MapViewOfFile(fileMapping, FILE_MAP_WRITE, 0, 0, fileMappingSize);
	RETURN_ERR(osErr(mappedView));

	if (sourceBitmap) {
		// if we're using the source bitmap directly, here we read it into the mapped view after the fact
		// this should be a safe read instead of partial, because imageSize should be calculated exactly right
		// because the header should've been vetted by getPixelFormat already
		RETURN_ERR(readStreamSafe(mappedView, imageSize, readStreamInterfacePointer));
	}

	bool result = true;

	// deal with endianness-imposed problems
	switch (pixelFormat.cs.colorSpace) {
		case RPCS_RGB:
		result = rgbX((RGBTRIPLE*)mappedView, stride, imageSize);
		break;
		case RPCS_RGBA:
		result = rgbaX((RGBQUAD*)mappedView, stride, imageSize);
	}

	if (!result) {
		return kMoaErr_OutOfMem;
	}
	return kMoaErr_NoErr;
}
#endif