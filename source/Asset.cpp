#include "Asset.h"

std::string& Asset::trimEllipsis(std::string &displayName) {
	// when displaying the type for the Cast window
	// Director will strip ellipsis off the end of the Display Name
	// (but ONLY if there are three periods or more)
	const std::string::size_type ELLIPSIS_SIZE = 3;

	std::string::size_type ellipsisIndex = displayName.find_last_not_of(".") + 1;

	if (ellipsisIndex > displayName.size() - ELLIPSIS_SIZE) {
		return displayName;
	}
	return displayName.erase(ellipsisIndex);
}

/*
MoaError Asset::Assets::Info::createIconAssetStream(std::optional<Stream>* streamOptionalPointer, PMoaVoid data, MoaUlong size, PIMoaCallback callbackInterfacePointer) {
	RETURN_NULL(data);
	RETURN_NULL(callbackInterfacePointer);

	bool createdNew = !streamOptionalPointer->has_value();

	if (createdNew) {
		*streamOptionalPointer = Asset::Stream(callbackInterfacePointer);
	}

	Stream &assetStream = streamOptionalPointer->value();

	if (!createdNew) {
		// optimization for the scenario where the icons are all the same size
		// (they aren't compressed, so they typically will be)
		// in that case we don't recreate the stream
		// just reset the position
		MoaStreamPosition end = 0;

		MoaError err = assetStream.getEnd(end);

		if (err == kMoaErr_NoErr
			&& end <= size) {
			RETURN_ERR(assetStream.resetPosition());
		} else {
			assetStream = Stream(callbackInterfacePointer);
		}
	}

	RETURN_ERR(assetStream.writeSafe(data, size));
	RETURN_ERR(assetStream.flush());
	return assetStream.resetPosition();
}
*/

void Asset::Assets::Info::destroy() {
	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
	releaseInterface((PPMoaVoid)&mmImageInterfacePointer);
}

void Asset::Assets::Info::duplicate(const Info &info) {
	setInterface((PPMoaVoid)&mmValueInterfacePointer, info.mmValueInterfacePointer);
	setInterface((PPMoaVoid)&mmImageInterfacePointer, info.mmImageInterfacePointer);

	iconValuesOptional = info.iconValuesOptional;
	//streamOptional = assetsInfo.streamOptional;

	assetInfoMap = info.assetInfoMap;
}

MoaError Asset::Assets::Info::findIconValue(IconValues::VARIANT &iconValuesVariant) {
	if (!mmImageInterfacePointer) {
		return kMoaErr_NoErr;
	}

	if (!iconValuesOptional.has_value()) {
		return kMoaErr_NoErr;
	}

	if (std::holds_alternative<MoaLong>(iconValuesVariant)) {
		const MoaLong &INDEX = std::get<MoaLong>(iconValuesVariant);

		IconValues::ICON_VALUE_MAP iconValueMap = iconValuesOptional.value().toIconValueMap();

		MoaRect rect = {};
		rect.left = 24 * INDEX;
		rect.right = rect.left + 16;
		rect.bottom = 16;

		MoaMmValue rectValue = kVoidMoaMmValueInitializer;
		RETURN_ERR(mmValueInterfacePointer->RectToValue(&rect, &rectValue));

		SCOPE_EXIT {
			releaseValue(rectValue, mmValueInterfacePointer);
		};

		MoaError err = kMoaErr_NoErr;

		MoaMmValue foundIconValue = kVoidMoaMmValueInitializer;
		IconValues::POINTER foundIconValues = std::make_shared<IconValues>(mmValueInterfacePointer, mmImageInterfacePointer);
		RETURN_NULL(foundIconValues);

		for (IconValues::ICON_VALUE_MAP::iterator iconValueMapIterator = iconValueMap.begin(); iconValueMapIterator != iconValueMap.end(); iconValueMapIterator++) {
			// if succeeded, release the value after setting the icon value
			// so it will be set to the found icon value
			SCOPE_EXIT {
				if (err == kMoaErr_NoErr) {
					releaseValue(foundIconValue, mmValueInterfacePointer);
				}
			};

			{
				// if failed, release the value before setting the icon value
				// so it will be set to void
				SCOPE_EXIT {
					if (err != kMoaErr_NoErr) {
						releaseValue(foundIconValue, mmValueInterfacePointer);
					}
				};

				// don't throw if this fails, value may be void
				err = mmImageInterfacePointer->Crop(&iconValueMapIterator->second, &rectValue, &foundIconValue);
			}

			RETURN_ERR(foundIconValues->setValue(iconValueMapIterator->first, foundIconValue));
		}

		iconValuesVariant = foundIconValues;
	}
	return kMoaErr_NoErr;
}

RESOURCE_ID Asset::Assets::Info::getBaseResourceID(unsigned long productVersionMajor) {
	// 8, 9, 10, 11, 12
	const unsigned long MIN_PRODUCT_VERSION_MAJOR = 8;
	const unsigned long MAX_PRODUCT_VERSION_MAJOR = 12;

	const RESOURCE_ID ASSET_INFO_MAP_ICON_RESOURCES_COUNT = 6;

	productVersionMajor = min(MAX_PRODUCT_VERSION_MAJOR, max(MIN_PRODUCT_VERSION_MAJOR, productVersionMajor));
	return (RESOURCE_ID)((productVersionMajor - MIN_PRODUCT_VERSION_MAJOR) * ASSET_INFO_MAP_ICON_RESOURCES_COUNT);
}

MoaError Asset::Assets::Info::getAssetInfoMapSymbols() {
	Asset::Info::MAP symbolAssetInfoMap = {};
	IconValues::MAP symbolIconValuesMap = {};
	SYMBOL_VARIANT symbolVariant = 0;

	for (Asset::Info::MAP::iterator assetInfoMapIterator = assetInfoMap.begin(); assetInfoMapIterator != assetInfoMap.end(); assetInfoMapIterator++) {
		RETURN_ERR(getSymbol(assetInfoMapIterator->second.subType, mmValueInterfacePointer));

		IconValues::MAP &iconValuesMap = assetInfoMapIterator->second.iconValuesMap;

		for (IconValues::MAP::iterator iconValuesMapIterator = iconValuesMap.begin(); iconValuesMapIterator != iconValuesMap.end(); iconValuesMapIterator++) {
			symbolVariant = iconValuesMapIterator->first;
			RETURN_ERR(getSymbol(symbolVariant, mmValueInterfacePointer));

			symbolIconValuesMap[symbolVariant] = iconValuesMapIterator->second;
		}

		iconValuesMap = symbolIconValuesMap;

		symbolVariant = assetInfoMapIterator->first;
		RETURN_ERR(getSymbol(symbolVariant, mmValueInterfacePointer));

		symbolAssetInfoMap[symbolVariant] = assetInfoMapIterator->second;
	}

	assetInfoMap = symbolAssetInfoMap;
	return kMoaErr_NoErr;
}

MoaError Asset::Assets::Info::getAssetInfoMapIcon(BitmapImporter &bitmapImporter, Label::Labels::Info &labelsInfo, IconValues &iconValues, RESOURCE_ID resourceID, RESOURCE_ID baseResourceID, XtraResourceCookie myCookie) {
	// (NULL is a valid value for myCookie)
	//RETURN_NULL(myCookie);

	//Formats::Format::POINTER formatPointer = 0;

	baseResourceID += resourceID;

	#ifdef MACINTOSH
	// this has not been tested
	GlobalHandleLock<>::GlobalHandle iconGlobalHandle = GetResource('PICT', baseResourceID);
	RETURN_ERR(osErr(iconGlobalHandle));

	GlobalHandleLock<> iconGlobalHandleLock(iconGlobalHandle);

	/*
	formatPointer = Formats::FormatFactory::createMediaInfoFormat(
		Label::TYPE_MEDIA_INFO | Label::TYPE_MEDIA_INFO_GLOBAL_HANDLE,
		mmValueInterfacePointer,
		true,
		resourceHandle
	);
	*/
	#endif
	#ifdef WINDOWS
	HRSRC resourceHandle = FindResource((HINSTANCE)myCookie, MAKEINTRESOURCE(baseResourceID), RT_BITMAP);
	RETURN_ERR(osErr(resourceHandle));

	GlobalHandleLock<> resourceGlobalHandleLock((HINSTANCE)myCookie, resourceHandle);
	SIZE_T resourceGlobalHandleLockSize = resourceGlobalHandleLock.size();

	// the handle given to us by FindResource is only a pseudo-global handle
	// so we need to copy it to a real one here
	GlobalHandleLock<>::GlobalHandle iconGlobalHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, resourceGlobalHandleLockSize);
	RETURN_ERR(osErr(iconGlobalHandle));

	GlobalHandleLock<> iconGlobalHandleLock(iconGlobalHandle);

	if (memcpy_s(iconGlobalHandleLock.get(), iconGlobalHandleLock.size(), resourceGlobalHandleLock.get(), resourceGlobalHandleLockSize)) {
		return kMoaErr_OutOfMem;
	}

	// the way this used to be done was with NewImageFromStream
	// but it didn't set the color depth correctly
	/*
	formatPointer = Formats::FormatFactory::createMediaInfoFormat(
		Label::TYPE_MEDIA_INFO | Label::TYPE_MEDIA_INFO_WIN_DIB,
		(HINSTANCE)myCookie,
		resourceHandle,
		callocInterfacePointer
	);
	*/
	#endif
	return bitmapImporter.insertIntoIconValues(iconGlobalHandleLock.getGlobalHandle(), iconValues, resourceID);

	/*
	RETURN_NULL(formatPointer);

	PMoaVoid data = NULL;
	MoaUlong size = 0;
	RETURN_ERR(formatPointer->get(data, size));

	SCOPE_EXIT {
		freeMemory(data, callocInterfacePointer);
	};

	RETURN_NULL(data);

	RETURN_ERR(createIconAssetStream(&streamOptional, data, size, callbackInterfacePointer));

	if (!streamOptional.has_value()) {
		return false;
	}

	// we don't need to release this (AssetStream has us covered)
	PIMoaStream streamInterfacePointer = streamOptional.value().get();
	RETURN_NULL(streamInterfacePointer);

	// this expects the stream to still be open
	// (no big deal if this fails)
	MoaMmValue iconValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(iconValue, mmValueInterfacePointer);
	};
	*/

	///*RETURN_ERR(*/mmImageInterfacePointer->NewImageFromStream(streamInterfacePointer, NULL, &iconValue)/*)*/;
	//return kMoaErr_NoErr;
}

MoaError Asset::Assets::Info::getAssetInfoMapIcons(BitmapImporter &bitmapImporter, Label::Labels::Info &labelsInfo, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer) {
	RETURN_NULL(callbackInterfacePointer);

	MoaError err = callbackInterfacePointer->QueryInterface(&IID_IMoaMmImage, (PPMoaVoid)&mmImageInterfacePointer);

	// it is fine if this interface doesn't exist
	// (pre-Director 8)
	if (err != kMoaErr_NoErr) {
		mmImageInterfacePointer = NULL;
	}

	if (!mmImageInterfacePointer) {
		return kMoaErr_NoErr;
	}

	iconValuesOptional.emplace(mmValueInterfacePointer, mmImageInterfacePointer);

	IconValues &iconValues = iconValuesOptional.value();
	IconValues::ICON_VALUE_MAP iconValueMap = iconValues.toIconValueMap();

	RESOURCE_ID baseResourceID = getBaseResourceID(productVersionMajor);

	XtraResourceCookie saveCookie = NULL;
	XtraResourceCookie myCookie = callbackInterfacePointer->MoaBeginUsingResources(gXtraFileRef, &saveCookie);

	SCOPE_EXIT {
		endUsingResources(gXtraFileRef, saveCookie, callbackInterfacePointer);
	};

	// reset the AssetStream when we're done using it
	// so we aren't holding onto an icon's stream pointlessly
	/*
	SCOPE_EXIT {
		streamOptional = std::nullopt;
	};
	*/

	for (IconValues::ICON_VALUE_MAP::iterator iconValueMapIterator = iconValueMap.begin(); iconValueMapIterator != iconValueMap.end(); iconValueMapIterator++) {
		RETURN_ERR(
			getAssetInfoMapIcon(
				bitmapImporter,
				labelsInfo,
				iconValues,
				iconValueMapIterator->first,
				baseResourceID,
				myCookie
			)
		);
	}
	return kMoaErr_NoErr;
}

Asset::Assets::Info::Info(BitmapImporter &bitmapImporter, Label::Labels::Info &labelsInfo, unsigned long productVersionMajor, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer)
	: mmValueInterfacePointer(mmValueInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}
	
	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->AddRef();

	MoaError err = getAssetInfoMapSymbols();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Asset Info Map Symbols");
	}

	err = getAssetInfoMapIcons(bitmapImporter, labelsInfo, productVersionMajor, callbackInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Asset Info Map Icons");
	}
}

Asset::Assets::Info::~Info() {
	destroy();
}

Asset::Assets::Info::Info(const Info &info) {
	duplicate(info);
}

Asset::Assets::Info &Asset::Assets::Info::operator=(const Info &info) {
	if (this == &info) {
		return *this;
	}

	duplicate(info);
	return *this;
}

std::optional<Asset::Info> Asset::Assets::Info::find(SYMBOL_VARIANT typeSymbol) {
	Asset::Info::MAP::iterator foundAssetInfo = assetInfoMap.find(typeSymbol);

	if (foundAssetInfo == assetInfoMap.cend()) {
		return std::nullopt;
	}

	MoaError err = kMoaErr_NoErr;

	for (IconValues::MAP::iterator iconValuesMapIterator = foundAssetInfo->second.iconValuesMap.begin(); iconValuesMapIterator != foundAssetInfo->second.iconValuesMap.end(); iconValuesMapIterator++) {
		err = findIconValue(iconValuesMapIterator->second);
		
		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Find Icon Value");
		}
	}
	return foundAssetInfo->second;
}

bool Asset::Assets::Info::insert(SYMBOL_VARIANT typeSymbol, const Asset::Info &assetInfo) {
	return assetInfoMap.insert({typeSymbol, assetInfo}).second;
}