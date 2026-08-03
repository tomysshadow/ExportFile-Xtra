#include "Registry.h"

namespace Registry {
	MoaError Entry::getValueLong(ConstPMoaChar keyStringPointer, MoaLong &value, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
		RETURN_NULL(keyStringPointer);
		RETURN_NULL(registryEntryDictInterfacePointer);

		MoaLong defaultValue = value;

		MoaError err = registryEntryDictInterfacePointer->Get(kMoaDictType_Long, &value, sizeof(value), keyStringPointer);

		if (err != kMoaErr_NoErr) {
			// discard whatever value was given out in the case of an error
			value = defaultValue;
			RETURN_ERR(registryEntryDictInterfacePointer->Put(kMoaDictType_Long, &value, sizeof(value), keyStringPointer));
		}
		return kMoaErr_NoErr;
	}

	MoaError Entry::setValueLong(ConstPMoaChar keyStringPointer, MoaLong value, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
		RETURN_NULL(keyStringPointer);
		RETURN_NULL(registryEntryDictInterfacePointer);

		return registryEntryDictInterfacePointer->Put(kMoaDictType_Long, &value, sizeof(value), keyStringPointer);
	}
}