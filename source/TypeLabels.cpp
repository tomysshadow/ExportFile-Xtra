#include "TypeLabels.h"
#include <vector>

MoaError TypeLabel::TypeLabels::getTypeLabelMapSymbols(PIMoaMmValue mmValueInterfacePointer) {
	RETURN_NULL(mmValueInterfacePointer);

	TypeLabel::MAP symbolTypeLabelMap = {};
	SYMBOL_VARIANT symbolVariant = 0;
	std::vector<SYMBOL_VARIANT> symbolVariantVector = {};

	for (TypeLabel::MAP::iterator typeLabelMapIterator = typeLabelMap.begin(); typeLabelMapIterator != typeLabelMap.end(); typeLabelMapIterator++) {
		Label::MAPPED_VECTOR &labelMappedVector = typeLabelMapIterator->second;

		symbolVariantVector = {};

		for (Label::MAPPED_VECTOR::CONST_ITERATOR labelMappedVectorIterator = labelMappedVector.cbegin(); labelMappedVectorIterator != labelMappedVector.cend(); labelMappedVectorIterator++) {
			symbolVariant = *labelMappedVectorIterator;
			RETURN_ERR(getSymbol(symbolVariant, mmValueInterfacePointer));

			symbolVariantVector.push_back(symbolVariant);
		}

		symbolVariant = typeLabelMapIterator->first;
		RETURN_ERR(getSymbol(symbolVariant, mmValueInterfacePointer));

		symbolTypeLabelMap[symbolVariant] = symbolVariantVector;
	}

	typeLabelMap = symbolTypeLabelMap;
	return kMoaErr_NoErr;
}

TypeLabel::TypeLabels::TypeLabels(PIMoaMmValue mmValueInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	MoaError err = getTypeLabelMapSymbols(mmValueInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Type Label Map Symbols");
	}
}

const TypeLabel::MAP& TypeLabel::TypeLabels::get() const {
	return typeLabelMap;
}