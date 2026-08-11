#pragma once
#include "utils.h"
#include "BitmapImporter.h"
#include "Asset.h"
#include <unordered_set>

#include "mmiimage.h"

namespace Registry {
	using SymbolSet = std::unordered_set<MoaMmSymbol>;

	struct Entry {
		using Variant = std::variant<Entry, PIMoaRegistryEntryDict>;

		MoaClassID* classIDPointer;
		MoaInterfaceID* interfaceIDPointer;

		static MoaError getValueLong(ConstPMoaChar keyStringPointer, MoaLong &value,
			PIMoaRegistryEntryDict registryEntryDictInterfacePointer);

		static MoaError setValueLong(ConstPMoaChar keyStringPointer, MoaLong value,
			PIMoaRegistryEntryDict registryEntryDictInterfacePointer);
	};

	struct ExcludedTypeSymbolSet {
		PIMoaMmValue mmValueInterfacePointer = NULL;
		SymbolSet* excludedTypeSymbolSetPointer = nullptr;
	};

	struct AssetInfoMap {
		PIMoaMmValue mmValueInterfacePointer = NULL;
		PIMoaCalloc callocInterfacePointer = NULL;

		PIMoaMmImage mmImageInterfacePointer = NULL;

		BitmapImporter* bitmapImporterPointer = nullptr;
		Asset::Info::Map* assetInfoMapPointer = nullptr;
	};

	struct AgentHiddenReaderSet {
		size_t* agentMoaIDsHashPointer = nullptr;
		Agent::HiddenReaderSet* agentHiddenReaderSetPointer = nullptr;
	};

	struct Reader {
		AgentHiddenReaderSet* registryEntriesAgentHiddenReaderSetPointer = nullptr;
		Entry::Variant* readerRegistryEntryVariantPointer = NULL;
	};
}