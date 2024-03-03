#include "IconValues.h"

void IconValues::destroy() {
	// may be NULL in the event of a move
	if (mmValueInterfacePointer) {
		for (ICON_VALUE_MAP::iterator iconValueMapIterator = iconValueMap.begin(); iconValueMapIterator != iconValueMap.end(); iconValueMapIterator++) {
			releaseValue(iconValueMapIterator->second, mmValueInterfacePointer);
		}
	}

	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
	releaseInterface((PPMoaVoid)&mmImageInterfacePointer);
}

void IconValues::duplicate(const IconValues &iconValues) {
	setInterface((PPMoaVoid)&mmValueInterfacePointer, iconValues.mmValueInterfacePointer);
	setInterface((PPMoaVoid)&mmImageInterfacePointer, iconValues.mmImageInterfacePointer);

	MoaError err = kMoaErr_NoErr;

	iconValueMap = iconValues.iconValueMap;

	for (ICON_VALUE_MAP::iterator iconValueMapIterator = iconValueMap.begin(); iconValueMapIterator != iconValueMap.end(); iconValueMapIterator++) {
		// handler not defined error is fine, may be void
		err = mmImageInterfacePointer->Duplicate(&iconValueMapIterator->second, &iconValueMapIterator->second);

		if (err != kMoaErr_NoErr && err != kMoaDrErr_HandlerNotDefined) {
			throw std::runtime_error("Failed to Duplicate Image");
		}
	}
}

void IconValues::move(IconValues &iconValues) {
	iconValueMap = iconValues.iconValueMap;
	iconValues.iconValueMap = {};

	moveInterface((PPMoaVoid)&mmValueInterfacePointer, (PPMoaVoid)&iconValues.mmValueInterfacePointer);
	moveInterface((PPMoaVoid)&mmImageInterfacePointer, (PPMoaVoid)&iconValues.mmImageInterfacePointer);
}

IconValues::IconValues(PIMoaMmValue mmValueInterfacePointer, PIMoaMmImage mmImageInterfacePointer)
	: mmValueInterfacePointer(mmValueInterfacePointer),
	mmImageInterfacePointer(mmImageInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	if (!mmImageInterfacePointer) {
		throw std::invalid_argument("mmImageInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->AddRef();
	mmImageInterfacePointer->AddRef();
}

IconValues::~IconValues() {
	destroy();
}

IconValues::IconValues(const IconValues &iconValues) {
	duplicate(iconValues);
}

IconValues::IconValues(IconValues &&iconValues) noexcept {
	*this = std::move(iconValues);
}

IconValues &IconValues::operator=(const IconValues &iconValues) {
	if (this == &iconValues) {
		return *this;
	}

	duplicate(iconValues);
	return *this;
}

IconValues &IconValues::operator=(IconValues &&iconValues) noexcept {
	if (this == &iconValues) {
		return *this;
	}

	destroy();
	move(iconValues);
	return *this;
}

MoaError IconValues::setValue(RESOURCE_ID resourceID, const MoaMmValue &value) {
	ICON_VALUE_MAP::iterator foundIconValue = iconValueMap.find(resourceID);

	if (foundIconValue == iconValueMap.end()) {
		return kMoaErr_BadParam;
	}

	releaseValue(foundIconValue->second, mmValueInterfacePointer);

	MoaError err = mmImageInterfacePointer->Duplicate(&value, &foundIconValue->second);

	if (err != kMoaErr_NoErr && err != kMoaDrErr_HandlerNotDefined) {
		RETURN_ERR(err);
	}
	return kMoaErr_NoErr;
}

const IconValues::ICON_VALUE_MAP& IconValues::toIconValueMap() const {
	return iconValueMap;
}