#include "Registry.h"

MoaError Registry::Entry::getValueLong(ConstPMoaChar keyStringPointer, MoaLong &value, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
	RETURN_NULL(keyStringPointer);
	RETURN_NULL(registryEntryDictInterfacePointer);

	const MoaLong VALUE_SIZE = sizeof(value);

	MoaLong defaultValue = value;

	MoaError err = registryEntryDictInterfacePointer->Get(kMoaDictType_Long, &value, VALUE_SIZE, keyStringPointer);

	if (err != kMoaErr_NoErr) {
		// discard whatever value was given out in the case of an error
		value = defaultValue;
		RETURN_ERR(registryEntryDictInterfacePointer->Put(kMoaDictType_Long, &value, VALUE_SIZE, keyStringPointer));
	}
	return kMoaErr_NoErr;
}

MoaError Registry::Entry::setValueLong(ConstPMoaChar keyStringPointer, MoaLong value, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
	RETURN_NULL(keyStringPointer);
	RETURN_NULL(registryEntryDictInterfacePointer);

	const MoaLong VALUE_SIZE = sizeof(value);
	return registryEntryDictInterfacePointer->Put(kMoaDictType_Long, &value, VALUE_SIZE, keyStringPointer);
}