#include "Label.h"

MoaError Label::Labels::Info::getLabelInfoMapSymbols(PIMoaMmValue mmValueInterfacePointer) {
	RETURN_NULL(mmValueInterfacePointer);

	Label::Info::MAP symbolsLabelInfoMap = {};
	SYMBOL_VARIANT symbolVariant = 0;

	for (Label::Info::MAP::iterator labelInfoMapIterator = labelInfoMap.begin(); labelInfoMapIterator != labelInfoMap.end(); labelInfoMapIterator++) {
		RETURN_ERR(getSymbol(labelInfoMapIterator->second.labelFormat, mmValueInterfacePointer));

		symbolVariant = labelInfoMapIterator->first;
		RETURN_ERR(getSymbol(symbolVariant, mmValueInterfacePointer));

		symbolsLabelInfoMap[symbolVariant] = labelInfoMapIterator->second;
	}

	labelInfoMap = symbolsLabelInfoMap;
	return kMoaErr_NoErr;
}

Label::Labels::Info::Info(PIMoaMmValue mmValueInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	MoaError err = getLabelInfoMapSymbols(mmValueInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Label Info Map Symbols");
	}
}

const Label::Info::MAP& Label::Labels::Info::get() const {
	return labelInfoMap;
}