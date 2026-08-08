#pragma once
#include "utils.h"
#include "BitmapImporter.h"
#include "Asset.h"
#include <unordered_set>

#include "mmiimage.h"

namespace Registry {
	typedef std::unordered_set<MoaMmSymbol> SYMBOL_SET;

	struct Entry {
		typedef std::variant<Entry, PIMoaRegistryEntryDict> VARIANT;

		MoaClassID* classIDPointer;
		MoaInterfaceID* interfaceIDPointer;

		static MoaError getValueLong(ConstPMoaChar keyStringPointer, MoaLong &value,
			PIMoaRegistryEntryDict registryEntryDictInterfacePointer);

		static MoaError setValueLong(ConstPMoaChar keyStringPointer, MoaLong value,
			PIMoaRegistryEntryDict registryEntryDictInterfacePointer);
	};

	struct ExcludedTypeSymbolSet {
		PIMoaMmValue mmValueInterfacePointer = NULL;
		SYMBOL_SET* excludedTypeSymbolSetPointer = nullptr;
	};

	struct AssetInfoMap {
		PIMoaMmValue mmValueInterfacePointer = NULL;
		PIMoaCalloc callocInterfacePointer = NULL;

		PIMoaMmImage mmImageInterfacePointer = NULL;

		BitmapImporter* bitmapImporterPointer = nullptr;
		Asset::Info::MAP* assetInfoMapPointer = nullptr;
	};

	struct AgentHiddenReaderSet {
		size_t* agentMoaIDsHashPointer = nullptr;
		Agent::HIDDEN_READER_SET* agentHiddenReaderSetPointer = nullptr;
	};

	struct Reader {
		AgentHiddenReaderSet* registryEntriesAgentHiddenReaderSetPointer = nullptr;
		Entry::VARIANT* readerRegistryEntryVariantPointer = NULL;
	};
}