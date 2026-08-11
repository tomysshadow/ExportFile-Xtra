#pragma once
#include "utils.h"
#include <map>
#include <unordered_map>

class ValueConverter {
	private:
	void destroy();
	void duplicate(const ValueConverter &valueConverter);

	protected:
	using PropListDictValueMap = std::map<
		const PMoaMmValue,
		PIMoaDict,
		std::less<>
	>;

	using DictValuePropListMap = std::unordered_map<
		PIMoaDict,
		PMoaMmValue
	>;

	PIMoaDrPlayer drPlayerInterfacePointer = NULL;
	PIMoaMmValue mmValueInterfacePointer = NULL;
	PIMoaMmList mmListInterfacePointer = NULL;
	PIMoaCalloc callocInterfacePointer = NULL;

	struct Symbols {
		MoaMmSymbol Hi = 0;
		MoaMmSymbol Lo = 0;
		MoaMmSymbol Member = 0;
		MoaMmSymbol String = 0;
	};

	Symbols symbols;

	MoaError getSymbols();

	MoaError appendToDictInterfacePointer(
		const MoaMmValue &propListValue,
		PropListDictValueMap &propListDictValueMap,
		MoaDictTypeID dictTypeID,
		ConstPMoaChar keyStringPointer,
		PIMoaDict dictInterfacePointer
	);

	MoaError appendToPropList(
		MoaMmValue &propListValue,
		bool appendInterfaceValue,
		DictValuePropListMap &dictValuePropListMap,
		MoaDictTypeID dictTypeID,
		MoaLong valueSize,
		ConstPMoaChar keyStringPointer,
		PIMoaDict dictInterfacePointer
	);

	MoaError concatToDictInterfacePointer(
		const MoaMmValue &propListValue,
		PropListDictValueMap &propListDictValueMap,
		bool &empty,
		PIMoaDict dictInterfacePointer
	);

	MoaError concatToDictInterfacePointer(
		const MoaMmValue &propListValue,
		PropListDictValueMap &propListDictValueMap,
		PIMoaDict dictInterfacePointer
	);

	MoaError concatToPropList(
		MoaMmValue &propListValue,
		bool appendInterfaceValues,
		DictValuePropListMap &dictValuePropListMap,
		PIMoaDict dictInterfacePointer
	);

	public:
	ValueConverter(
		PIMoaDrPlayer drPlayerInterfacePointer,
		PIMoaMmValue mmValueInterfacePointer,
		PIMoaMmList mmListInterfacePointer,
		PIMoaCalloc callocInterfacePointer
	);

	~ValueConverter();
	ValueConverter(const ValueConverter &valueConverter);
	ValueConverter &operator=(const ValueConverter &valueConverter);

	MoaError toString(
		const MoaMmValue &value, std::string &string, bool strict = true
	);

	MoaError toWide(const MoaMmValue &value, MoaWide &wide);
	MoaError toID(const MoaMmValue &value, MoaID &id);

	MoaError toBytes(
		const MoaMmValue &value, MoaByte* &bytesPointer, MoaLong &valueSize
	);

	MoaError toValue(
		const SymbolVariant &symbolVariant, MoaMmValue &value
	);

	MoaError toValue(
		const MoaWide &wide, MoaMmValue &value
	);

	MoaError toValue(
		const MoaID &id, MoaMmValue &value
	);

	MoaError toValue(
		const MoaByte* bytesPointer, MoaLong valueSize, MoaMmValue &value
	);

	MoaError toValue(
		PIMoaDrCastMem drCastMemInterfacePointer, MoaMmValue &value
	);

	MoaError testStringValueEmpty(
		const MoaMmValue &testStringValue, bool &empty
	);

	MoaError testListValueEmpty(
		const MoaMmValue &testListValue, bool &empty
	);

	MoaError testValueVoid(
		const MoaMmValue &testValue, bool &voidP,
		MoaMmValueType testValueType = kMoaMmValueType_Void
	);

	MoaError testPropFound(
		MoaMmSymbol propertySymbol, bool &found,
		PIMoaDrCastMem drCastMemInterfacePointer
	);

	MoaError appendToList(
		const MoaMmValue &value, MoaMmValue &listValue
	);

	MoaError appendToList(
		const SymbolVariant &symbolVariant, MoaMmValue &listValue
	);

	MoaError appendToList(
		MoaLong integer, MoaMmValue &listValue
	);

	MoaError appendToList(
		ConstPMoaChar stringPointer, MoaMmValue &listValue
	);

	MoaError appendToPropList(
		const MoaMmValue &propertyValue,
		const MoaMmValue &value,
		MoaMmValue &propListValue
	);

	MoaError appendToPropList(
		MoaMmSymbol propertySymbol,
		const MoaMmValue &value,
		MoaMmValue &propListValue
	);

	MoaError appendToPropList(
		MoaMmSymbol propertySymbol,
		ConstPMoaChar stringPointer,
		MoaMmValue &propListValue
	);

	MoaError appendToPropList(
		ConstPMoaChar propertyStringPointer,
		const MoaMmValue &value,
		MoaMmValue &propListValue
	);

	MoaError appendToPropList(
		ConstPMoaChar propertyStringPointer,
		ConstPMoaChar stringPointer,
		MoaMmValue &propListValue
	);

	MoaError appendToPropList(
		SymbolVariant propertySymbolVariant,
		const MoaMmValue &value,
		MoaMmValue &propListValue
	);

	MoaError appendToPropList(
		SymbolVariant propertySymbolVariant,
		ConstPMoaChar stringPointer,
		MoaMmValue &propListValue
	);

	MoaError concatToDictInterfacePointer(
		const MoaMmValue &propListValue,
		bool &empty, PIMoaDict dictInterfacePointer
	);

	MoaError concatToDictInterfacePointer(
		const MoaMmValue &propListValue,
		PIMoaDict dictInterfacePointer
	);

	MoaError concatToPropList(
		MoaMmValue &propListValue,
		bool appendInterfaceValues, PIMoaDict dictInterfacePointer
	);

	MoaError getValueType(
		const MoaMmValue &value, bool &voidP, MoaMmValueType &valueType
	);

	MoaError getAt(
		const MoaMmValue &listValue, MoaLong index, MoaMmValue &value
	);

	MoaError getAt(
		const MoaMmValue &listValue, MoaLong index, MoaLong &integer
	);

	MoaError getProp(
		const MoaMmValue &propListValue,
		MoaMmSymbol propertySymbol,
		MoaMmValue &value
	);

	MoaError getProp(
		const MoaMmValue &propListValue,
		ConstPMoaChar propertyStringPointer,
		MoaMmValue &value
	);

	MoaError getProp(
		const MoaMmValue &propListValue,
		SymbolVariant propertySymbolVariant,
		MoaMmValue &value
	);

	MoaError getAProp(
		const MoaMmValue &propListValue,
		MoaMmSymbol propertySymbol,
		MoaMmValue &value
	);

	MoaError getAProp(
		const MoaMmValue &propListValue,
		ConstPMoaChar propertyStringPointer,
		MoaMmValue &value
	);

	MoaError getAProp(
		const MoaMmValue &propListValue,
		SymbolVariant propertySymbolVariant,
		MoaMmValue &value
	);
};