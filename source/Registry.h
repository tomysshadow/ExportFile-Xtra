#pragma once
#include "shared.h"
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

		static MoaError getValueLong(ConstPMoaChar keyStringPointer, MoaLong &value, PIMoaRegistryEntryDict registryEntryDictInterfacePointer);
		static MoaError setValueLong(ConstPMoaChar keyStringPointer, MoaLong value, PIMoaRegistryEntryDict registryEntryDictInterfacePointer);
	};

	struct ExcludedTypeSymbolSet {
		PIMoaMmValue mmValueInterfacePointer = NULL;
		SYMBOL_SET* excludedTypeSymbolSetPointer = 0;
	};

	struct AssetInfoMap {
		PIMoaMmValue mmValueInterfacePointer = NULL;
		PIMoaCalloc callocInterfacePointer = NULL;

		PIMoaMmImage mmImageInterfacePointer = NULL;

		BitmapImporter* bitmapImporterPointer = 0;
		Asset::Info::MAP* assetInfoMapPointer = 0;
	};

	struct AgentHiddenReaderSet {
		size_t* agentMoaIDsHashPointer = 0;
		Agent::HIDDEN_READER_SET* agentHiddenReaderSetPointer = 0;
	};

	struct Reader {
		AgentHiddenReaderSet* registryEntriesAgentHiddenReaderSetPointer = 0;
		Entry::VARIANT* readerRegistryEntryVariantPointer = NULL;
	};
}