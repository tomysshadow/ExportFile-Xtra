#include "BitmapImporter.h"

void BitmapImporter::destroy() {
	MoaError err = kMoaErr_NoErr;

	if (memberIndex) {
		err = drCastInterfacePointer->DeleteCastMem(memberIndex);

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Delete Cast Member");
		}
	}

	if (castIndex) {
		err = drMovieInterfacePointer->RemoveCast(castIndex);

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Remove Cast");
		}
	}

	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
	releaseInterface((PPMoaVoid)&drUtilsInterfacePointer);
	releaseInterface((PPMoaVoid)&mmImageInterfacePointer);
	releaseInterface((PPMoaVoid)&drMovieInterfacePointer);
	releaseInterface((PPMoaVoid)&drCastInterfacePointer);
	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
}

void BitmapImporter::duplicate(const BitmapImporter &bitmapImporter) {
	setInterface((PPMoaVoid)&mmValueInterfacePointer, bitmapImporter.mmValueInterfacePointer);
	setInterface((PPMoaVoid)&drUtilsInterfacePointer, bitmapImporter.drUtilsInterfacePointer);
	setInterface((PPMoaVoid)&mmImageInterfacePointer, bitmapImporter.mmImageInterfacePointer);
	setInterface((PPMoaVoid)&drMovieInterfacePointer, bitmapImporter.drMovieInterfacePointer);
	setInterface((PPMoaVoid)&drCastInterfacePointer, bitmapImporter.drCastInterfacePointer);
	setInterface((PPMoaVoid)&drCastMemInterfacePointer, bitmapImporter.drCastMemInterfacePointer);

	formatSymbol = bitmapImporter.formatSymbol;

	symbols = bitmapImporter.symbols;

	// it isn't enough to copy castIndex and memberIndex
	// (the old member gets deleted in the event of a copy)
	if (!createMember()) {
		throw std::runtime_error("Failed to Create Member");
	}
}

void BitmapImporter::move(BitmapImporter &bitmapImporter) {
	memberIndex = bitmapImporter.memberIndex;
	bitmapImporter.memberIndex = NULL;

	castIndex = bitmapImporter.castIndex;
	bitmapImporter.castIndex = NULL;

	moveInterface((PPMoaVoid)&mmValueInterfacePointer, (PPMoaVoid)&bitmapImporter.mmValueInterfacePointer);
	moveInterface((PPMoaVoid)&drUtilsInterfacePointer, (PPMoaVoid)&bitmapImporter.drUtilsInterfacePointer);
	moveInterface((PPMoaVoid)&mmImageInterfacePointer, (PPMoaVoid)&bitmapImporter.mmImageInterfacePointer);
	moveInterface((PPMoaVoid)&drMovieInterfacePointer, (PPMoaVoid)&bitmapImporter.drMovieInterfacePointer);
	moveInterface((PPMoaVoid)&drCastInterfacePointer, (PPMoaVoid)&bitmapImporter.drCastInterfacePointer);
	moveInterface((PPMoaVoid)&drCastMemInterfacePointer, (PPMoaVoid)&bitmapImporter.drCastMemInterfacePointer);

	formatSymbol = bitmapImporter.formatSymbol;

	symbols = bitmapImporter.symbols;
}

MoaError BitmapImporter::getSymbols() {
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Bitmap", &symbols.Bitmap));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Field", &symbols.Field));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Image", &symbols.Image));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Picture", &symbols.Picture));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Type", &symbols.Type));
	return kMoaErr_NoErr;
}

MoaError BitmapImporter::getFormatSymbol(const Label::Labels::Info &labelsInfo) {
	const Label::Info::MAP &LABEL_INFO_MAP = labelsInfo.get();

	Label::Info::MAP::const_iterator foundLabelInfo = LABEL_INFO_MAP.find(symbols.Image);

	if (foundLabelInfo == LABEL_INFO_MAP.cend()) {
		return kMoaDrErr_LabelNotFound;
	}

	const Label::Info &LABEL_INFO = foundLabelInfo->second;

	if (!std::holds_alternative<MoaMmSymbol>(LABEL_INFO.labelFormat)) {
		return kMoaDrErr_MediaFormatNotSupported;
	}

	formatSymbol = std::get<MoaMmSymbol>(LABEL_INFO.labelFormat);
	return kMoaErr_NoErr;
}

MoaError BitmapImporter::createMemberInCast(MoaDrCastIndex movieCastIndex) {
	RETURN_NULL(movieCastIndex);
	RETURN_NULL(drMovieInterfacePointer);

	MAKE_SCOPE_EXIT(releaseDrCastInterfacePointerScopeExit) {
		memberIndex = NULL;

		releaseInterface((PPMoaVoid)&drCastInterfacePointer);
	};

	RETURN_ERR(drMovieInterfacePointer->GetNthCast(movieCastIndex, &drCastInterfacePointer));
	RETURN_NULL(drCastInterfacePointer);

	RETURN_ERR(drCastInterfacePointer->GetLastFreeMemberIndex(&memberIndex));
	RETURN_NULL(memberIndex);

	RETURN_ERR(drCastInterfacePointer->CreateCastMem(memberIndex, symbols.Bitmap));
	releaseDrCastInterfacePointerScopeExit.dismiss();

	RETURN_ERR(drCastInterfacePointer->GetCastMem(memberIndex, &drCastMemInterfacePointer));
	RETURN_NULL(drCastMemInterfacePointer);
	return kMoaErr_NoErr;
}

MoaError BitmapImporter::createMember() {
	MoaLong castCount = 0;
	RETURN_ERR(drMovieInterfacePointer->GetCastCount(&castCount));

	MoaError err = kMoaErr_NoErr;

	for (MoaLong i = 1; i <= castCount; i++) {
		err = createMemberInCast(i);

		if (err == kMoaErr_NoErr) {
			return kMoaErr_NoErr;
		}
	}

	// no free slots
	MoaChar castName[] = "BitmapImporter"; // can't be const
	RETURN_ERR(drMovieInterfacePointer->NewCast(castName, FALSE, &castIndex));
	RETURN_NULL(castIndex);
	return createMemberInCast(castIndex);
}

MoaError BitmapImporter::getMemberImageValue(GlobalHandleLock<>::GlobalHandle mediaData, MoaMmValue &memberStageImageValue, bool color) {
	// this holds for the duration of this scope, then releases, the global handle
	GlobalHandleLock<> globalHandleLock(mediaData);
	memberStageImageValue = kVoidMoaMmValueInitializer;

	MoaDrImageAuxInfo imageAuxInfo = {};
	imageAuxInfo.pixelDepth = color ? 32 : 1;

	MoaDrMediaInfo mediaInfo = {};

	// SetMedia, not AttachMedia - we own the Global Handle and release it ourselves
	RETURN_ERR(
		drUtilsInterfacePointer->NewMediaInfo(
			symbols.Image,
			formatSymbol,
			globalHandleLock.getGlobalHandle(),

			// Color Depth: 32-bit for Color, 1-bit for BW
			// Trim White Space: Disabled
			kMoaDrImgMediaOpts_AuxInfo
			| kMoaDrImgMediaFlags_DontTrimWhiteEdges,

			&imageAuxInfo,
			&mediaInfo
		)
	);

	RETURN_ERR(drCastMemInterfacePointer->SetMedia(&mediaInfo));
	return drCastMemInterfacePointer->GetProp(symbols.Image, &memberStageImageValue);
}

BitmapImporter::BitmapImporter(const Label::Labels::Info &labelsInfo, PIMoaDrMovie drMovieInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaDrUtils drUtilsInterfacePointer, PIMoaMmImage mmImageInterfacePointer)
	: drMovieInterfacePointer(drMovieInterfacePointer),
	mmValueInterfacePointer(mmValueInterfacePointer),
	drUtilsInterfacePointer(drUtilsInterfacePointer),
	mmImageInterfacePointer(mmImageInterfacePointer) {
	if (!drMovieInterfacePointer) {
		throw std::invalid_argument("drMovieInterfacePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	if (!drUtilsInterfacePointer) {
		throw std::invalid_argument("drUtilsInterfacePointer must not be NULL");
	}

	drMovieInterfacePointer->AddRef();
	mmValueInterfacePointer->AddRef();
	drUtilsInterfacePointer->AddRef();

	if (mmImageInterfacePointer) {
		mmImageInterfacePointer->AddRef();
	}

	MoaError err = getSymbols();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Symbols");
	}

	err = getFormatSymbol(labelsInfo);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Format Symbol");
	}

	err = createMember();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Create Member");
	}
}

BitmapImporter::BitmapImporter(const Label::Labels::Info &labelsInfo, PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaDrUtils drUtilsInterfacePointer, PIMoaMmImage mmImageInterfacePointer)
	: mmValueInterfacePointer(mmValueInterfacePointer),
	drUtilsInterfacePointer(drUtilsInterfacePointer),
	mmImageInterfacePointer(mmImageInterfacePointer) {
	if (!drPlayerInterfacePointer) {
		throw std::invalid_argument("drPlayerInterfacePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	if (!drUtilsInterfacePointer) {
		throw std::invalid_argument("drUtilsInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->AddRef();
	drUtilsInterfacePointer->AddRef();

	if (mmImageInterfacePointer) {
		mmImageInterfacePointer->AddRef();
	}

	MoaError err = getSymbols();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Symbols");
	}

	err = getFormatSymbol(labelsInfo);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Format Symbol");
	}

	err = drPlayerInterfacePointer->GetActiveMovie(&drMovieInterfacePointer);
	
	if (err != kMoaErr_NoErr
	|| !drMovieInterfacePointer) {
		throw std::runtime_error("Failed to Get Active Movie");
	}

	err = createMember();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Create Member");
	}
}

BitmapImporter::~BitmapImporter() {
	destroy();
}

BitmapImporter::BitmapImporter(const BitmapImporter &bitmapImporter) {
	duplicate(bitmapImporter);
}

BitmapImporter::BitmapImporter(BitmapImporter &&bitmapImporter) noexcept {
	*this = std::move(bitmapImporter);
}

BitmapImporter &BitmapImporter::operator=(const BitmapImporter &bitmapImporter) {
	if (this == &bitmapImporter) {
		return *this;
	}

	duplicate(bitmapImporter);
	return *this;
}

BitmapImporter &BitmapImporter::operator=(BitmapImporter &&bitmapImporter) noexcept {
	if (this == &bitmapImporter) {
		return *this;
	}

	destroy();
	move(bitmapImporter);
	return *this;
}

MoaError BitmapImporter::toImageValue(GlobalHandleLock<>::GlobalHandle mediaData, MoaMmValue &stageImageValue, bool color) {
	stageImageValue = kVoidMoaMmValueInitializer;
	
	MoaMmValue memberStageImageValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(memberStageImageValue, mmValueInterfacePointer);
	};

	RETURN_ERR(getMemberImageValue(mediaData, memberStageImageValue, color));

	if (!mmImageInterfacePointer) {
		return kMoaErr_BadClass;
	}
	return mmImageInterfacePointer->Duplicate(&memberStageImageValue, &stageImageValue);
}

// more efficient method for Icon Values specifically (avoids a copy of the image)
MoaError BitmapImporter::insertIntoIconValues(GlobalHandleLock<>::GlobalHandle mediaData, IconValues &iconValues, RESOURCE_ID resourceID) {
	MoaMmValue memberStageImageValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(memberStageImageValue, mmValueInterfacePointer);
	};

	RETURN_ERR(getMemberImageValue(mediaData, memberStageImageValue, resourceID == IDB_ASSET_INFO_MAP_ICON_COLOR || resourceID == IDB_ASSET_INFO_MAP_ICON_COLOR_LINKED));
	return iconValues.setValue(resourceID, memberStageImageValue);
}

MoaError BitmapImporter::getMedia(MoaDrMediaInfo &mediaInfo, PIMoaDrCastMem imageDrCastMemInterfacePointer) {
	RETURN_NULL(imageDrCastMemInterfacePointer);

	if (mediaInfo.labelSymbol != symbols.Image) {
		return kMoaDrErr_MediaLabelNotSupported;
	}

	if (mediaInfo.formatSymbol != formatSymbol) {
		return kMoaDrErr_MediaFormatNotSupported;
	}

	MoaMmValue typeValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(typeValue, mmValueInterfacePointer);
	};

	RETURN_ERR(imageDrCastMemInterfacePointer->GetProp(symbols.Type, &typeValue));

	MoaMmSymbol typeSymbol = 0;
	RETURN_ERR(mmValueInterfacePointer->ValueToSymbol(&typeValue, &typeSymbol));

	if (typeSymbol != symbols.Bitmap) {
		// handle built-in member types that don't have an image property but do have a picture property
		// (or our Director version is old enough to predate images)
		MoaMmSymbol memberPropertySymbol = !mmImageInterfacePointer
			|| typeSymbol == symbols.Picture
			|| typeSymbol == symbols.Field
			? symbols.Picture
			: symbols.Image;

		MoaMmValue memberPropertyValue = kVoidMoaMmValueInitializer;

		SCOPE_EXIT {
			releaseValue(memberPropertyValue, mmValueInterfacePointer);
		};

		MoaError err = imageDrCastMemInterfacePointer->GetProp(memberPropertySymbol, &memberPropertyValue);

		if (err == kMoaErr_NoErr) {
			err = drCastMemInterfacePointer->SetProp(memberPropertySymbol, &memberPropertyValue);

			if (err == kMoaErr_NoErr) {
				return drCastMemInterfacePointer->GetMedia(&mediaInfo);
			}
		}
	}
	return imageDrCastMemInterfacePointer->GetMedia(&mediaInfo);
}

MoaError BitmapImporter::getProp(MoaMmValue &memberPropertyValue, PIMoaDrCastMem pictureDrCastMemInterfacePointer) {
	// release this as necessary
	releaseValue(memberPropertyValue, mmValueInterfacePointer);

	RETURN_NULL(pictureDrCastMemInterfacePointer);

	MoaMmValue imageValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(imageValue, mmValueInterfacePointer);
	};

	RETURN_ERR(pictureDrCastMemInterfacePointer->GetProp(symbols.Image, &imageValue));
	RETURN_ERR(drCastMemInterfacePointer->SetProp(symbols.Image, &imageValue));
	return drCastMemInterfacePointer->GetProp(symbols.Picture, &memberPropertyValue);
}