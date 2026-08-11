#include "TypeLabels.h"
#include <vector>

namespace TypeLabel {
	MoaError TypeLabels::getTypeLabelMapSymbols(PIMoaMmValue mmValueInterfacePointer) {
		RETURN_NULL(mmValueInterfacePointer);

		Map symbolTypeLabelMap = {};
		SymbolVariant symbolVariant = 0;
		std::vector<SymbolVariant> symbolVariantVector = {};

		for (
			auto typeLabelMapIterator = typeLabelMap.begin();
			typeLabelMapIterator != typeLabelMap.end();
			typeLabelMapIterator++
		) {
			Label::MappedVector &labelMappedVector = typeLabelMapIterator->second;

			symbolVariantVector = {};

			for (
				auto labelMappedVectorIterator = labelMappedVector.cbegin();
				labelMappedVectorIterator != labelMappedVector.cend();
				labelMappedVectorIterator++
			) {
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

	TypeLabels::TypeLabels(PIMoaMmValue mmValueInterfacePointer) {
		if (!mmValueInterfacePointer) {
			throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
		}

		MoaError err = getTypeLabelMapSymbols(mmValueInterfacePointer);

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("failed to get type label map symbols");
		}
	}

	const Map& TypeLabels::get() const {
		return typeLabelMap;
	}
}