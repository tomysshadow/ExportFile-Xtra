#include "ValueConverter.h"

/*
#ifdef WINDOWS
#include <Objbase.h>
#endif
*/

void ValueConverter::destroy() {
	releaseInterface((PPMoaVoid)&drPlayerInterfacePointer);
	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
	releaseInterface((PPMoaVoid)&mmListInterfacePointer);
	releaseInterface((PPMoaVoid)&callocInterfacePointer);
}

void ValueConverter::duplicate(const ValueConverter &valueConverter) {
	setInterface((PPMoaVoid)&drPlayerInterfacePointer, valueConverter.drPlayerInterfacePointer);
	setInterface((PPMoaVoid)&mmValueInterfacePointer, valueConverter.mmValueInterfacePointer);
	setInterface((PPMoaVoid)&mmListInterfacePointer, valueConverter.mmListInterfacePointer);
	setInterface((PPMoaVoid)&callocInterfacePointer, valueConverter.callocInterfacePointer);

	symbols = valueConverter.symbols;
}

MoaError ValueConverter::getSymbols() {
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Hi", &symbols.Hi));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Lo", &symbols.Lo));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Member", &symbols.Member));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("String", &symbols.String));
	return kMoaErr_NoErr;
}

MoaError ValueConverter::appendToDictInterfacePointer(const MoaMmValue &propListValue, PROP_LIST_DICT_VALUE_MAP &propListDictValueMap, MoaDictTypeID dictTypeID, ConstPMoaChar keyStringPointer, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(keyStringPointer);
	RETURN_NULL(dictInterfacePointer);

	typedef std::map<MoaDictTypeID, MoaLong> DICT_TYPE_ID_VALUE_SIZE_MAP;

	const DICT_TYPE_ID_VALUE_SIZE_MAP dictTypeIDValueSizeMap = {
		{kMoaDictType_Dict, sizeof(PIMoaDict)},
		{kMoaDictType_PIMoaUnknown, sizeof(PIMoaUnknown)},
		{kMoaDictType_Long, sizeof(MoaLong)},
		{kMoaDictType_Float, sizeof(MoaFloat)},
		{kMoaDictType_Wide, sizeof(MoaWide)},
		{kMoaDictType_Bool, sizeof(MoaBool)},
		{kMoaDictType_MoaID, sizeof(MoaID)},
		{kMoaDictType_Double, sizeof(MoaDouble)},
		{kMoaDictType_MoaPoint, sizeof(MoaPoint)},
		{kMoaDictType_MoaRect, sizeof(MoaRect)}
	};

	PMoaVoid valueBuffer = NULL;
	MoaLong valueSize = 0;

	SCOPE_EXIT {
		freeMemory(valueBuffer, callocInterfacePointer);
	};

	DICT_TYPE_ID_VALUE_SIZE_MAP::const_iterator dictTypeIDValueSizeMapIterator = dictTypeIDValueSizeMap.find(dictTypeID);

	if (dictTypeIDValueSizeMapIterator != dictTypeIDValueSizeMap.end()) {
		valueSize = dictTypeIDValueSizeMapIterator->second;

		valueBuffer = callocInterfacePointer->NRAlloc(valueSize);
		RETURN_NULL(valueBuffer);
	}

	MoaMmValue value = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(value, mmValueInterfacePointer);
	};

	// use the property as long as it's in the property list
	// (even if the value is void)
	MoaError err = getProp(propListValue, keyStringPointer, value);

	if (err == kMoaDrErr_HandlerNotDefined) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR(err);

	switch (dictTypeID) {
		case kMoaDictType_Dict:
		{
			PROP_LIST_DICT_VALUE_MAP::iterator propListDictValueMapIterator = propListDictValueMap.find((const PMoaMmValue)&value);

			if (propListDictValueMapIterator == propListDictValueMap.end()) {
				// this interface does not need to be released
				PIMoaDict valueDictInterfacePointer = NULL;
				RETURN_ERR(dictInterfacePointer->MakeDict(&valueDictInterfacePointer, keyStringPointer));
				return concatToDictInterfacePointer(value, propListDictValueMap, valueDictInterfacePointer);
			} else {
				RETURN_NULL(propListDictValueMapIterator->second);
				*(PIMoaDict)valueBuffer = *propListDictValueMapIterator->second;
			}
		}
		break;
		case kMoaDictType_PIMoaUnknown:
		case kMoaDictType_Long:
		// it is fine to treat PIMoaUnknown as a standard integer here
		// (we intentionally do not null check it)
		RETURN_ERR(mmValueInterfacePointer->ValueToInteger(&value, (PMoaLong)valueBuffer));
		break;
		case kMoaDictType_Float:
		{
			MoaDouble valueDouble = 0;
			RETURN_ERR(mmValueInterfacePointer->ValueToFloat(&value, &valueDouble));
			*(MoaFloat*)valueBuffer = (MoaFloat)valueDouble;
		}
		break;
		case kMoaDictType_Wide:
		RETURN_ERR(toWide(value, *(MoaWide*)valueBuffer));
		break;
		case kMoaDictType_Bool:
		{
			MoaLong valueInteger = 0;
			RETURN_ERR(mmValueInterfacePointer->ValueToInteger(&value, &valueInteger));
			*(MoaBool*)valueBuffer = (MoaBool)valueInteger;
		}
		break;
		case kMoaDictType_MoaID:
		RETURN_ERR(toID(value, *(MoaID*)valueBuffer));
		break;
		case kMoaDictType_Bytes:
		{
			RETURN_ERR(toBytes(value, *(MoaByte**)&valueBuffer, valueSize));
			RETURN_NULL(valueBuffer);
		}
		break;
		case kMoaDictType_CString:
		RETURN_ERR(mmValueInterfacePointer->ValueStringLength(&value, &valueSize));
		valueSize++;

		valueBuffer = callocInterfacePointer->NRAlloc(valueSize);
		RETURN_NULL(valueBuffer);

		RETURN_ERR(mmValueInterfacePointer->ValueToString(&value, (PMoaChar)valueBuffer, valueSize));
		break;
		case kMoaDictType_Double:
		RETURN_ERR(mmValueInterfacePointer->ValueToFloat(&value, (MoaDouble*)valueBuffer));
		break;
		case kMoaDictType_MoaPoint:
		RETURN_ERR(mmValueInterfacePointer->ValueToPoint(&value, (PMoaPoint)valueBuffer));
		break;
		case kMoaDictType_MoaRect:
		RETURN_ERR(mmValueInterfacePointer->ValueToRect(&value, (PMoaRect)valueBuffer));
	}
	return dictInterfacePointer->Put(dictTypeID, valueBuffer, valueSize, keyStringPointer);
}

MoaError ValueConverter::appendToPropList(MoaMmValue &propListValue, bool appendInterfaceValue, DICT_VALUE_PROP_LIST_MAP &dictValuePropListMap, MoaDictTypeID dictTypeID, MoaLong valueSize, ConstPMoaChar keyStringPointer, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(keyStringPointer);
	RETURN_NULL(dictInterfacePointer);

	typedef std::map<MoaDictTypeID, MoaLong> DICT_TYPE_ID_VALUE_SIZE_MAP;

	const DICT_TYPE_ID_VALUE_SIZE_MAP dictTypeIDValueSizeMap = {
		{kMoaDictType_Dict, sizeof(PIMoaDict)},
		{kMoaDictType_PIMoaUnknown, sizeof(PIMoaUnknown)},
		{kMoaDictType_Long, sizeof(MoaLong)},
		{kMoaDictType_Float, sizeof(MoaFloat)},
		{kMoaDictType_Wide, sizeof(MoaWide)},
		{kMoaDictType_Bool, sizeof(MoaBool)},
		{kMoaDictType_MoaID, sizeof(MoaID)},
		{kMoaDictType_Double, sizeof(MoaDouble)},
		{kMoaDictType_MoaPoint, sizeof(MoaPoint)},
		{kMoaDictType_MoaRect, sizeof(MoaRect)}
	};

	DICT_TYPE_ID_VALUE_SIZE_MAP::const_iterator dictTypeIDValueSizeMapIterator = dictTypeIDValueSizeMap.find(dictTypeID);

	if (dictTypeIDValueSizeMapIterator != dictTypeIDValueSizeMap.end()
		&& dictTypeIDValueSizeMapIterator->second < valueSize) {
		return kMoaDictErr_BufferTooSmall;
	}

	PMoaVoid valueBuffer = callocInterfacePointer->NRAlloc(valueSize);

	SCOPE_EXIT {
		freeMemory(valueBuffer, callocInterfacePointer);
	};

	RETURN_NULL(valueBuffer);
	RETURN_ERR(dictInterfacePointer->Get(dictTypeID, valueBuffer, valueSize, keyStringPointer));

	MoaMmValue value = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(value, mmValueInterfacePointer);
	};

	bool appendValue = true;

	switch (dictTypeID) {
		case kMoaDictType_Dict:
		{
			PIMoaDict valueDictInterfacePointer = (PIMoaDict)valueBuffer;

			if (valueDictInterfacePointer) {
				// handle for self referential list
				// (must add a ref in that case because we release it below)
				DICT_VALUE_PROP_LIST_MAP::iterator dictValuePropListMapIterator = dictValuePropListMap.find(valueDictInterfacePointer);

				if (dictValuePropListMapIterator == dictValuePropListMap.end()) {
					RETURN_ERR(mmListInterfacePointer->NewPropListValue(&value));
					RETURN_ERR(concatToPropList(value, appendInterfaceValue, valueDictInterfacePointer));
				} else {
					RETURN_NULL(dictValuePropListMapIterator->second);
					value = *dictValuePropListMapIterator->second;
					mmValueInterfacePointer->ValueAddRef(&value);
				}
			}
		}
		break;
		case kMoaDictType_PIMoaUnknown:
		appendValue = appendInterfaceValue;

		if (appendValue) {
			PIMoaUnknown unknownInterfacePointer = (PIMoaUnknown)valueBuffer;

			if (unknownInterfacePointer) {
				unknownInterfacePointer->AddRef();
			}
		}
		// PIMoaUnknown's get cast to a MoaLong
		[[fallthrough]];
		case kMoaDictType_Long:
		RETURN_ERR(mmValueInterfacePointer->IntegerToValue(*(PMoaLong)valueBuffer, &value));
		break;
		case kMoaDictType_Float:
		RETURN_ERR(mmValueInterfacePointer->FloatToValue(*(MoaFloat*)valueBuffer, &value));
		break;
		case kMoaDictType_Wide:
		RETURN_ERR(toValue(*(MoaWide*)valueBuffer, value));
		break;
		case kMoaDictType_Bool:
		RETURN_ERR(mmValueInterfacePointer->IntegerToValue(*(MoaBool*)valueBuffer, &value));
		break;
		case kMoaDictType_MoaID:
		RETURN_ERR(toValue(*(MoaID*)valueBuffer, value));
		break;
		case kMoaDictType_Bytes:
		RETURN_ERR(toValue((MoaByte*)valueBuffer, valueSize, value));
		break;
		case kMoaDictType_CString:
		RETURN_ERR(mmValueInterfacePointer->StringToValue((PMoaChar)valueBuffer, &value));
		break;
		case kMoaDictType_Double:
		RETURN_ERR(mmValueInterfacePointer->FloatToValue(*(MoaDouble*)valueBuffer, &value));
		break;
		case kMoaDictType_MoaPoint:
		RETURN_ERR(mmValueInterfacePointer->PointToValue((PMoaPoint)valueBuffer, &value));
		break;
		case kMoaDictType_MoaRect:
		RETURN_ERR(mmValueInterfacePointer->RectToValue((PMoaRect)valueBuffer, &value));
	}

	if (appendValue) {
		RETURN_ERR(appendToPropList(keyStringPointer, value, propListValue));
	}
	return kMoaErr_NoErr;
}

MoaError ValueConverter::concatToDictInterfacePointer(const MoaMmValue &propListValue, PROP_LIST_DICT_VALUE_MAP &propListDictValueMap, bool &empty, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(dictInterfacePointer);

	RETURN_ERR(testValueVoid(propListValue, empty, kMoaMmValueType_PropList));

	if (!empty) {
		RETURN_ERR(testListValueEmpty(propListValue, empty));
	}

	if (!empty) {
		propListDictValueMap[(const PMoaMmValue)&propListValue] = dictInterfacePointer;

		MoaUlong count = 0;
		RETURN_ERR(dictInterfacePointer->Count(&count));

		MoaDictTypeID dictTypeID = kMoaDictType_Bogus;
		ConstPMoaChar keyStringPointer = NULL;

		for (MoaUlong i = 0; i < count; i++) {
			RETURN_ERR(dictInterfacePointer->GetNth(i, &dictTypeID, NULL, &keyStringPointer));
			RETURN_ERR(appendToDictInterfacePointer(propListValue, propListDictValueMap, dictTypeID, keyStringPointer, dictInterfacePointer));
		}
	}
	return kMoaErr_NoErr;
}

MoaError ValueConverter::concatToDictInterfacePointer(const MoaMmValue &propListValue, PROP_LIST_DICT_VALUE_MAP &propListDictValueMap, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(dictInterfacePointer);

	bool empty = false;
	return concatToDictInterfacePointer(propListValue, propListDictValueMap, empty, dictInterfacePointer);
}

MoaError ValueConverter::concatToPropList(MoaMmValue &propListValue, bool appendInterfaceValues, DICT_VALUE_PROP_LIST_MAP &dictValuePropListMap, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(dictInterfacePointer);

	dictValuePropListMap[dictInterfacePointer] = &propListValue;

	MoaUlong count = 0;
	RETURN_ERR(dictInterfacePointer->Count(&count));

	MoaDictTypeID dictTypeID = kMoaDictType_Bogus;
	MoaLong valueSize = 0;
	ConstPMoaChar keyStringPointer = NULL;

	for (MoaUlong i = 0; i < count; i++) {
		RETURN_ERR(dictInterfacePointer->GetNth(i, &dictTypeID, &valueSize, &keyStringPointer));
		RETURN_ERR(appendToPropList(propListValue, appendInterfaceValues, dictValuePropListMap, dictTypeID, valueSize, keyStringPointer, dictInterfacePointer));
	}
	return kMoaErr_NoErr;
}

ValueConverter::ValueConverter(PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaMmList mmListInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: drPlayerInterfacePointer(drPlayerInterfacePointer),
	mmValueInterfacePointer(mmValueInterfacePointer),
	mmListInterfacePointer(mmListInterfacePointer),
	callocInterfacePointer(callocInterfacePointer) {
	if (!drPlayerInterfacePointer) {
		throw std::invalid_argument("drPlayerInterfacePointer must not be NULL");
	}

	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	if (!mmListInterfacePointer) {
		throw std::invalid_argument("mmListInterfacePointer must not be NULL");
	}

	if (!callocInterfacePointer) {
		throw std::invalid_argument("callocInterfacePointer must not be NULL");
	}

	drPlayerInterfacePointer->AddRef();
	mmValueInterfacePointer->AddRef();
	mmListInterfacePointer->AddRef();
	callocInterfacePointer->AddRef();

	MoaError err = getSymbols();

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Symbols");
	}
}

ValueConverter::~ValueConverter() {
	destroy();
}

ValueConverter::ValueConverter(const ValueConverter &valueConverter) {
	duplicate(valueConverter);
}

ValueConverter &ValueConverter::operator=(const ValueConverter &valueConverter) {
	if (this == &valueConverter) {
		return *this;
	}

	duplicate(valueConverter);
	return *this;
}

MoaError ValueConverter::toString(const MoaMmValue &value, std::string &string, bool strict) {
	string = "";

	MoaMmValue stringValue = value;

	ConstPMoaChar stringPointer = NULL;
	MoaError err = mmValueInterfacePointer->ValueToStringPtr(&stringValue, &stringPointer);

	MoaMmValue resultValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(resultValue, mmValueInterfacePointer);
	};

	if (!strict
	&& err == kMoaMmErr_StringExpected) {
		RETURN_ERR(drPlayerInterfacePointer->CallHandler(symbols.String, 1, &stringValue, &resultValue));
		stringValue = resultValue;

		err = mmValueInterfacePointer->ValueToStringPtr(&stringValue, &stringPointer);
	}

	RETURN_ERR(err);
	RETURN_NULL(stringPointer);

	SCOPE_EXIT {
		err = errOrDefaultErr(mmValueInterfacePointer->ValueReleaseStringPtr(&stringValue), err);
	};

	string = stringPointer;
	return err;
}

MoaError ValueConverter::toWide(const MoaMmValue &value, MoaWide &wide) {
	wide = {};

	MoaMmValue hiValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(hiValue, mmValueInterfacePointer);
	};

	RETURN_ERR(getProp(value, symbols.Hi, hiValue));
	RETURN_ERR(mmValueInterfacePointer->ValueToInteger(&hiValue, &wide.hi));

	MoaMmValue loValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(loValue, mmValueInterfacePointer);
	};

	RETURN_ERR(getProp(value, symbols.Lo, loValue));

	MoaLong lo = 0;
	RETURN_ERR(mmValueInterfacePointer->ValueToInteger(&loValue, &lo));
	wide.lo = lo;
	return kMoaErr_NoErr;
}

MoaError ValueConverter::toID(const MoaMmValue &value, MoaID &id) {
	const MoaLong ID_SIZE = sizeof(id);

	unsigned char* dataPointer = (unsigned char*)&id;
	RETURN_NULL(dataPointer);

	MoaLong integer = 0;

	for (MoaLong i = 1; i <= ID_SIZE; i++) {
		RETURN_ERR(getAt(value, i, integer));
		*dataPointer++ = (unsigned char)integer;
	}
	return kMoaErr_NoErr;
}

MoaError ValueConverter::toBytes(const MoaMmValue &value, MoaByte* &bytesPointer, MoaLong &valueSize) {
	bytesPointer = NULL;

	MAKE_SCOPE_EXIT(freeMemoryBytesPointerScopeExit) {
		freeMemory(*(PMoaVoid*)bytesPointer, callocInterfacePointer);
	};

	valueSize = mmListInterfacePointer->CountElements(&value);

	if (valueSize == -1) {
		return kMoaDrErr_HandlerNotDefined;
	}

	bytesPointer = (MoaByte*)callocInterfacePointer->NRAlloc(valueSize);
	RETURN_NULL(bytesPointer);

	// copy so we don't add to the main pointer we want to free later
	MoaByte* bytePointer = bytesPointer;
	RETURN_NULL(bytePointer);

	MoaLong integer = 0;

	for (MoaLong i = 1; i <= valueSize; i++) {
		RETURN_ERR(getAt(value, i, integer));
		*bytePointer++ = (MoaByte)integer;
	}

	freeMemoryBytesPointerScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::toValue(const SYMBOL_VARIANT &symbolVariant, MoaMmValue &value) {
	MoaMmSymbol symbol = 0;

	RETURN_ERR(getSymbol(symbolVariant, symbol, mmValueInterfacePointer));
	return mmValueInterfacePointer->SymbolToValue(symbol, &value);
}

MoaError ValueConverter::toValue(const MoaWide &wide, MoaMmValue &value) {
	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewPropListValue(&value));

	MoaMmValue hiValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(hiValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(wide.hi, &hiValue));
	RETURN_ERR(appendToPropList(symbols.Hi, hiValue, value));

	MoaMmValue loValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(loValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(wide.lo, &loValue));
	RETURN_ERR(appendToPropList(symbols.Lo, loValue, value));

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::toValue(const MoaID &id, MoaMmValue &value) {
	/*
	#ifdef MACINTOSH
	// hmmm
	#endif
	#ifdef WINDOWS
	LPOLESTR iidString = NULL;
	RETURN_ERR(StringFromIID(id, &iidString))
	RETURN_ERR(mmValueInterfacePointer->StringToValue(CW2A(iidString, CP_DIRECTOR(productVersionMajor)), &value))
	#endif
	*/

	const MoaLong ID_SIZE = sizeof(id);

	unsigned char* dataPointer = (unsigned char*)&id;
	RETURN_NULL(dataPointer);

	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewListValue(&value));

	for (MoaLong i = 1; i <= ID_SIZE; i++) {
		RETURN_ERR(appendToList(*dataPointer++, value));
	}

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::toValue(const MoaByte* bytesPointer, MoaLong valueSize, MoaMmValue &value) {
	RETURN_NULL(bytesPointer);

	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewListValue(&value));

	for (MoaLong i = 1; i <= valueSize; i++) {
		RETURN_ERR(appendToList(*bytesPointer++, value));
	}

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::toValue(PIMoaDrCastMem drCastMemInterfacePointer, MoaMmValue &value) {
	value = kVoidMoaMmValueInitializer;

	RETURN_NULL(drCastMemInterfacePointer);
	return drCastMemInterfacePointer->GetProp(symbols.Member, &value);
}

MoaError ValueConverter::testStringValueEmpty(const MoaMmValue &testStringValue, bool &empty) {
	empty = true;

	MoaLong stringLength = 0;
	RETURN_ERR(mmValueInterfacePointer->ValueStringLength(&testStringValue, &stringLength));

	empty = !stringLength;
	return kMoaErr_NoErr;
}

MoaError ValueConverter::testListValueEmpty(const MoaMmValue &testListValue, bool &empty) {
	empty = true;

	MoaLong elements = mmListInterfacePointer->CountElements(&testListValue);

	if (elements == -1) {
		return kMoaDrErr_HandlerNotDefined;
	}

	empty = !elements;
	return kMoaErr_NoErr;
}

MoaError ValueConverter::testValueVoid(const MoaMmValue &testValue, bool &voidP, MoaMmValueType testValueType) {
	voidP = true;

	MoaMmValueType valueType = kMoaMmValueType_Void;
	RETURN_ERR(getValueType(testValue, voidP, valueType));

	if (!voidP && testValueType != kMoaMmValueType_Void && valueType != testValueType) {
		return kMoaMmErr_ValueTypeMismatch;
	}
	return kMoaErr_NoErr;
}

MoaError ValueConverter::testPropFound(MoaMmSymbol propertySymbol, bool &found, PIMoaDrCastMem drCastMemInterfacePointer) {
	found = false;

	RETURN_NULL(drCastMemInterfacePointer);

	MoaMmValue propertyValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(propertyValue, mmValueInterfacePointer);
	};

	MoaError err = drCastMemInterfacePointer->GetProp(propertySymbol, &propertyValue);

	if (err != kMoaMmErr_PropertyNotFound) {
		RETURN_ERR(err);
	}

	found = err == kMoaErr_NoErr;
	return kMoaErr_NoErr;
}

MoaError ValueConverter::appendToList(const MoaMmValue &value, MoaMmValue &listValue) {
	// method to handle for a const input value
	MoaMmValue modifiableValue = value;

	SCOPE_EXIT {
		const size_t MOA_MM_VALUE_SIZE = sizeof(MoaMmValue);

		if (!memoryEquals(&modifiableValue, &value, MOA_MM_VALUE_SIZE)) {
			releaseValue(modifiableValue, mmValueInterfacePointer);
		}
	};
	return mmListInterfacePointer->AppendValueToList(&listValue, &modifiableValue);
}

MoaError ValueConverter::appendToList(const SYMBOL_VARIANT &symbolVariant, MoaMmValue &listValue) {
	MoaMmValue symbolValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(symbolValue, mmValueInterfacePointer);
	};

	MoaMmSymbol symbol = 0;

	RETURN_ERR(getSymbol(symbolVariant, symbol, mmValueInterfacePointer));
	RETURN_ERR(mmValueInterfacePointer->SymbolToValue(symbol, &symbolValue));
	return mmListInterfacePointer->AppendValueToList(&listValue, &symbolValue);
}

MoaError ValueConverter::appendToList(MoaLong integer, MoaMmValue &listValue) {
	MoaMmValue integerValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(integerValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(integer, &integerValue));
	return mmListInterfacePointer->AppendValueToList(&listValue, &integerValue);
}

MoaError ValueConverter::appendToList(ConstPMoaChar stringPointer, MoaMmValue &listValue) {
	RETURN_NULL(stringPointer);

	MoaMmValue stringValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(stringValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(stringPointer, &stringValue));
	return mmListInterfacePointer->AppendValueToList(&listValue, &stringValue);
}

MoaError ValueConverter::appendToPropList(const MoaMmValue &propertyValue, const MoaMmValue &value, MoaMmValue &propListValue) {
	const size_t MOA_MM_VALUE_SIZE = sizeof(MoaMmValue);

	// because AppendValueToPropList has a non-const value input
	// we technically have to check for a different output
	// (even though I don't think it ever does)
	MoaMmValue modifiablePropertyValue = propertyValue;

	SCOPE_EXIT {
		if (!memoryEquals(&modifiablePropertyValue, &propertyValue, MOA_MM_VALUE_SIZE)) {
			releaseValue(modifiablePropertyValue, mmValueInterfacePointer);
		}
	};

	MoaMmValue modifiableValue = value;

	SCOPE_EXIT {
		if (!memoryEquals(&modifiableValue, &value, MOA_MM_VALUE_SIZE)) {
			releaseValue(modifiableValue, mmValueInterfacePointer);
		}
	};
	return mmListInterfacePointer->AppendValueToPropList(&propListValue, &modifiablePropertyValue, &modifiableValue);
}

MoaError ValueConverter::appendToPropList(MoaMmSymbol propertySymbol, const MoaMmValue &value, MoaMmValue &propListValue) {
	MoaMmValue propertyValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(propertyValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->SymbolToValue(propertySymbol, &propertyValue));
	return appendToPropList(propertyValue, value, propListValue);
}

MoaError ValueConverter::appendToPropList(MoaMmSymbol propertySymbol, ConstPMoaChar stringPointer, MoaMmValue &propListValue) {
	RETURN_NULL(stringPointer);

	MoaMmValue value = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(stringPointer, &value));
	return appendToPropList(propertySymbol, value, propListValue);
}

MoaError ValueConverter::appendToPropList(ConstPMoaChar propertyStringPointer, const MoaMmValue &value, MoaMmValue &propListValue) {
	RETURN_NULL(propertyStringPointer);

	MoaMmValue propertyValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(propertyValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(propertyStringPointer, &propertyValue));
	return appendToPropList(propertyValue, value, propListValue);
}

MoaError ValueConverter::appendToPropList(ConstPMoaChar propertyStringPointer, ConstPMoaChar stringPointer, MoaMmValue &propListValue) {
	RETURN_NULL(propertyStringPointer);
	RETURN_NULL(stringPointer);

	MoaMmValue value = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(stringPointer, &value));
	return appendToPropList(propertyStringPointer, value, propListValue);
}

MoaError ValueConverter::appendToPropList(SYMBOL_VARIANT propertySymbolVariant, const MoaMmValue &value, MoaMmValue &propListValue) {
	MoaMmSymbol propertySymbol = 0;

	RETURN_ERR(getSymbol(propertySymbolVariant, propertySymbol, mmValueInterfacePointer));
	return appendToPropList(propertySymbol, value, propListValue);
}

MoaError ValueConverter::appendToPropList(SYMBOL_VARIANT propertySymbolVariant, ConstPMoaChar stringPointer, MoaMmValue &propListValue) {
	RETURN_NULL(stringPointer);

	MoaMmSymbol propertySymbol = 0;

	RETURN_ERR(getSymbol(propertySymbolVariant, propertySymbol, mmValueInterfacePointer));
	return appendToPropList(propertySymbol, stringPointer, propListValue);
}

MoaError ValueConverter::concatToDictInterfacePointer(const MoaMmValue &propListValue, bool &empty, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(dictInterfacePointer);

	PROP_LIST_DICT_VALUE_MAP propListDictValueMap = {};
	return concatToDictInterfacePointer(propListValue, propListDictValueMap, empty, dictInterfacePointer);
}

MoaError ValueConverter::concatToDictInterfacePointer(const MoaMmValue &propListValue, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(dictInterfacePointer);

	bool empty = false;
	return concatToDictInterfacePointer(propListValue, empty, dictInterfacePointer);
}

MoaError ValueConverter::concatToPropList(MoaMmValue &propListValue, bool appendInterfaceValues, PIMoaDict dictInterfacePointer) {
	RETURN_NULL(dictInterfacePointer);

	DICT_VALUE_PROP_LIST_MAP dictValuePropListMap = {};
	return concatToPropList(propListValue, appendInterfaceValues, dictValuePropListMap, dictInterfacePointer);
}

MoaError ValueConverter::getValueType(const MoaMmValue &value, bool &voidP, MoaMmValueType &valueType) {
	voidP = true;

	RETURN_ERR(mmValueInterfacePointer->ValueType(&value, &valueType));

	voidP = valueType == kMoaMmValueType_Void;
	return kMoaErr_NoErr;
}

MoaError ValueConverter::getAt(const MoaMmValue &listValue, MoaLong index, MoaMmValue &value) {
	MAKE_SCOPE_EXIT(valueScopeExit) {
		value = kVoidMoaMmValueInitializer;
	};

	MoaError err = mmListInterfacePointer->GetValueByIndex(&listValue, index, &value);

	if (err == kMoaDrErr_HandlerNotDefined) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR(err);
	valueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::getAt(const MoaMmValue &listValue, MoaLong index, MoaLong &integer) {
	MAKE_SCOPE_EXIT(integerScopeExit) {
		integer = 0;
	};

	MoaMmValue integerValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(integerValue, mmValueInterfacePointer);
	};

	MoaError err = mmListInterfacePointer->GetValueByIndex(&listValue, index, &integerValue);

	if (err == kMoaDrErr_HandlerNotDefined) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR(err);
	RETURN_ERR(mmValueInterfacePointer->ValueToInteger(&integerValue, &integer));
	integerScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::getProp(const MoaMmValue &propListValue, MoaMmSymbol propertySymbol, MoaMmValue &value) {
	MoaMmValue propertyValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(propertyValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->SymbolToValue(propertySymbol, &propertyValue));
	return mmListInterfacePointer->GetValueByProperty(&propListValue, &propertyValue, &value);
}

MoaError ValueConverter::getProp(const MoaMmValue &propListValue, ConstPMoaChar propertyStringPointer, MoaMmValue &value) {
	RETURN_NULL(propertyStringPointer);

	MoaMmValue propertyValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(propertyValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(propertyStringPointer, &propertyValue));
	return mmListInterfacePointer->GetValueByProperty(&propListValue, &propertyValue, &value);
}

MoaError ValueConverter::getProp(const MoaMmValue &propListValue, SYMBOL_VARIANT propertySymbolVariant, MoaMmValue &value) {
	MoaMmSymbol propertySymbol = 0;

	RETURN_ERR(getSymbol(propertySymbolVariant, propertySymbol, mmValueInterfacePointer));
	return getProp(propListValue, propertySymbol, value);
}

MoaError ValueConverter::getAProp(const MoaMmValue &propListValue, MoaMmSymbol propertySymbol, MoaMmValue &value) {
	MAKE_SCOPE_EXIT(valueScopeExit) {
		value = kVoidMoaMmValueInitializer;
	};

	MoaError err = getProp(propListValue, propertySymbol, value);

	// if the value isn't found we don't throw, instead the value is set to void
	if (err == kMoaDrErr_HandlerNotDefined) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR(err);
	valueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::getAProp(const MoaMmValue &propListValue, ConstPMoaChar propertyStringPointer, MoaMmValue &value) {
	MAKE_SCOPE_EXIT(valueScopeExit) {
		value = kVoidMoaMmValueInitializer;
	};

	RETURN_NULL(propertyStringPointer);

	MoaError err = getProp(propListValue, propertyStringPointer, value);

	if (err == kMoaDrErr_HandlerNotDefined) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR(err);
	valueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ValueConverter::getAProp(const MoaMmValue &propListValue, SYMBOL_VARIANT propertySymbolVariant, MoaMmValue &value) {
	MAKE_SCOPE_EXIT(valueScopeExit) {
		value = kVoidMoaMmValueInitializer;
	};

	MoaError err = getProp(propListValue, propertySymbolVariant, value);

	if (err == kMoaDrErr_HandlerNotDefined) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR(err);
	valueScopeExit.dismiss();
	return kMoaErr_NoErr;
}