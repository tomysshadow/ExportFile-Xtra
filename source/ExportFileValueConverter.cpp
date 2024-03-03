#include "ExportFileValueConverter.h"

MoaError ExportFileValueConverter::getSymbols() {
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Agent", &symbols.Agent));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("AgentOptions", &symbols.AgentOptions));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("AlternatePathExtension", &symbols.AlternatePathExtension));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Basename", &symbols.Basename));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("BW", &symbols.BW));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Color", &symbols.Color));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Current", &symbols.Current));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Dirname", &symbols.Dirname));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Filename", &symbols.Filename));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Extension", &symbols.Extension));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("PathExtensions", &symbols.PathExtensions));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Hidden", &symbols.Hidden));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("IncrementFilename", &symbols.IncrementFilename));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Interface", &symbols.Interface));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Label", &symbols.Label));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Linked", &symbols.Linked));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Location", &symbols.Location));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Mask", &symbols.Mask));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Member", &symbols.Member));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Name", &symbols.Name));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("NewFolder", &symbols.NewFolder));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Options", &symbols.Options));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("Path", &symbols.Path));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("PathInfo", &symbols.PathInfo));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("ReplaceExistingFile", &symbols.ReplaceExistingFile));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("WriterClassID", &symbols.WriterClassID));
	RETURN_ERR(mmValueInterfacePointer->StringToSymbol("WriterClassIDs", &symbols.WriterClassIDs));
	return kMoaErr_NoErr;
}

ExportFileValueConverter::ExportFileValueConverter(PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaMmList mmListInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: ValueConverter(
		drPlayerInterfacePointer,
		mmValueInterfacePointer,
		mmListInterfacePointer,
		callocInterfacePointer
	) {
	MoaError err = getSymbols();
	
	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Symbols");
	}
}

MoaError ExportFileValueConverter::toAsset(PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmXAsset &mmXAssetInterfacePointer) {
	mmXAssetInterfacePointer = NULL;

	RETURN_NULL(drCastMemInterfacePointer);

	MoaMmValue interfaceValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(interfaceValue, mmValueInterfacePointer);
	};

	// this is already AddRef'd for us
	// do not error if it's NULL (just means this member doesn't have an interface)
	RETURN_ERR(drCastMemInterfacePointer->GetProp(symbols.Interface, &interfaceValue));
	return mmValueInterfacePointer->ValueToInteger(&interfaceValue, (PMoaLong)&mmXAssetInterfacePointer);
}

MoaError ExportFileValueConverter::toValue(Args &args, MoaMmValue &value) {
	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewPropListValue(&value));

	MoaMmValue memberValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(memberValue, mmValueInterfacePointer);
	};

	PIMoaDrCastMem drCastMemInterfacePointer = args.getDrCastMemInterfacePointer();

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
	};

	if (!drCastMemInterfacePointer) {
		return kMoaErr_InternalError;
	}

	RETURN_ERR(toValue(drCastMemInterfacePointer, memberValue));
	RETURN_ERR(appendToPropList(symbols.Member, memberValue, value));

	if (!args.pathInfoOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	MoaMmValue pathInfoValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(pathInfoValue, mmValueInterfacePointer);
	};

	std::string path = "";
	RETURN_ERR(toValue(args.pathInfoOptional.value(), pathInfoValue, path));

	if (path.empty()) {
		return kMoaErr_InternalError;
	}

	RETURN_ERR(appendToPropList(symbols.Path, path.c_str(), value));
	RETURN_ERR(appendToPropList(symbols.PathInfo, pathInfoValue, value));

	MoaMmValue labelValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(labelValue, mmValueInterfacePointer);
	};

	RETURN_ERR(toValue(args.labelSymbolVariant, labelValue));
	RETURN_ERR(appendToPropList(symbols.Label, labelValue, value));

	if (!args.agentStringOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	MoaMmValue agentValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(agentValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(args.agentStringOptional.value().c_str(), &agentValue));
	RETURN_ERR(appendToPropList(symbols.Agent, agentValue, value));

	if (!args.optionsOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	MoaMmValue optionsPropListValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(optionsPropListValue, mmValueInterfacePointer);
	};

	RETURN_ERR(toValue(args.optionsOptional.value(), optionsPropListValue));
	RETURN_ERR(appendToPropList(symbols.Options, optionsPropListValue, value));

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::toValue(Path::Info &pathInfo, MoaMmValue &value, std::string &path) {
	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewPropListValue(&value));

	std::optional<std::string> dirnameOptional = std::nullopt;
	std::optional<std::string> basenameOptional = std::nullopt;
	std::optional<std::string> extensionOptional = std::nullopt;
	std::optional<std::string> filenameOptional = std::nullopt;

	if (!pathInfo.getElementOptionals(dirnameOptional, basenameOptional, extensionOptional, filenameOptional, path)) {
		return kMoaErr_BadParam;
	}

	if (!dirnameOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	RETURN_ERR(appendToPropList(symbols.Dirname, dirnameOptional.value().c_str(), value));

	if (!basenameOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	RETURN_ERR(appendToPropList(symbols.Basename, basenameOptional.value().c_str(), value));

	if (!extensionOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	RETURN_ERR(appendToPropList(symbols.Extension, extensionOptional.value().c_str(), value));

	if (!filenameOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	RETURN_ERR(appendToPropList(symbols.Filename, filenameOptional.value().c_str(), value));

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::toValue(Path::Info &pathInfo, MoaMmValue &value) {
	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	// this overload doesn't care about the path string
	std::string path = "";
	RETURN_ERR(toValue(pathInfo, value, path));

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::toValue(const Label::MAPPED_VECTOR &labelMappedVector, MoaMmValue &value) {
	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewListValue(&value));

	for (Label::MAPPED_VECTOR::CONST_ITERATOR labelMappedVectorIterator = labelMappedVector.cbegin(); labelMappedVectorIterator != labelMappedVector.cend(); labelMappedVectorIterator++) {
		RETURN_ERR(appendToList(*labelMappedVectorIterator, value));
	}

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::toValue(const Agent::Info::MAP &agentInfoMap, MoaMmValue &value) {
	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewPropListValue(&value));

	for (Agent::Info::MAP::const_iterator agentInfoMapIterator = agentInfoMap.begin(); agentInfoMapIterator != agentInfoMap.end(); agentInfoMapIterator++) {
		RETURN_ERR(appendToPropList(agentInfoMapIterator->first.c_str(), agentInfoMapIterator->second, value));
	}

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::toValue(const Options &options, MoaMmValue &value) {
	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewPropListValue(&value));

	MoaMmValue incrementFilenameValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(incrementFilenameValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(options.incrementFilename != FALSE, &incrementFilenameValue));
	RETURN_ERR(appendToPropList(symbols.IncrementFilename, incrementFilenameValue, value));

	MoaMmValue replaceExistingFileValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(replaceExistingFileValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(options.replaceExistingFile != FALSE, &replaceExistingFileValue));
	RETURN_ERR(appendToPropList(symbols.ReplaceExistingFile, replaceExistingFileValue, value));

	MoaMmValue newFolderValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(newFolderValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(options.newFolder != FALSE, &newFolderValue));
	RETURN_ERR(appendToPropList(symbols.NewFolder, newFolderValue, value));

	MoaMmValue alternatePathExtensionValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(alternatePathExtensionValue, mmValueInterfacePointer);
	};

	// no != FALSE here, this is not an on/off value
	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(options.alternatePathExtension, &alternatePathExtensionValue));
	RETURN_ERR(appendToPropList(symbols.AlternatePathExtension, alternatePathExtensionValue, value));

	MoaMmValue locationValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(locationValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->SymbolToValue(options.locationSymbol, &locationValue));
	RETURN_ERR(appendToPropList(symbols.Location, locationValue, value));

	MoaMmValue writerClassIDValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(writerClassIDValue, mmValueInterfacePointer);
	};

	RETURN_ERR(toValue(options.writerClassID, writerClassIDValue));
	RETURN_ERR(appendToPropList(symbols.WriterClassID, writerClassIDValue, value));

	MoaMmValue agentOptionsValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(agentOptionsValue, mmValueInterfacePointer);
	};

	options.getAgentOptionsValue(agentOptionsValue);
	RETURN_ERR(appendToPropList(symbols.AgentOptions, agentOptionsValue, value));

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::toValue(const IconValues::ICON_VALUE_MAP &iconValueMap, MoaMmValue &value, PIMoaDrCastMem drCastMemInterfacePointer) {
	RETURN_NULL(drCastMemInterfacePointer);

	MoaLong linked = FALSE;
	RETURN_ERR(testMemberLinked(linked, drCastMemInterfacePointer));

	value = kVoidMoaMmValueInitializer;

	MAKE_SCOPE_EXIT(releaseValueScopeExit) {
		releaseValue(value, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewPropListValue(&value));

	IconValues::ICON_VALUE_MAP::const_iterator foundIconValue = {};

	#define FIND_ICON_VALUE(propertySymbol, resourceID, resourceIDLinked) do {\
		foundIconValue = iconValueMap.find(linked ? (resourceIDLinked) : (resourceID));\
		\
		if (foundIconValue != iconValueMap.cend()) {\
			RETURN_ERR(appendToPropList((propertySymbol), foundIconValue->second, value));\
		}\
	} while (0)

	FIND_ICON_VALUE(symbols.Color, IDB_ASSET_INFO_MAP_ICON_COLOR, IDB_ASSET_INFO_MAP_ICON_COLOR_LINKED);
	FIND_ICON_VALUE(symbols.BW, IDB_ASSET_INFO_MAP_ICON_BW, IDB_ASSET_INFO_MAP_ICON_BW_LINKED);
	FIND_ICON_VALUE(symbols.Mask, IDB_ASSET_INFO_MAP_ICON_MASK, IDB_ASSET_INFO_MAP_ICON_MASK_LINKED);

	releaseValueScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::testMemberLinked(MoaLong &linked, PIMoaDrCastMem drCastMemInterfacePointer) {
	MAKE_SCOPE_EXIT(linkedScopeExit) {
		linked = FALSE;
	};

	RETURN_NULL(drCastMemInterfacePointer);

	MoaMmValue linkedValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(linkedValue, mmValueInterfacePointer);
	};

	MoaError err = drCastMemInterfacePointer->GetProp(symbols.Linked, &linkedValue);

	if (err != kMoaMmErr_PropertyNotFound) {
		RETURN_ERR(err);
		RETURN_ERR(mmValueInterfacePointer->ValueToInteger(&linkedValue, &linked));
		linkedScopeExit.dismiss();
		return kMoaErr_NoErr;
	}

	// Linked property doesn't exist pre-Director 7
	// but must be checked if it's present (linked members can have an empty filename)
	MoaMmValue filenameValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(filenameValue, mmValueInterfacePointer);
	};

	RETURN_ERR(drCastMemInterfacePointer->GetProp(symbols.Filename, &filenameValue));

	bool filenameEmpty = true;
	RETURN_ERR(testStringValueEmpty(filenameValue, filenameEmpty));

	linked = !filenameEmpty;

	linkedScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError ExportFileValueConverter::appendToPropList(ConstPMoaChar propertyStringPointer, const Agent::Info &agentInfo, MoaMmValue &propListValue) {
	RETURN_NULL(propertyStringPointer);

	MoaMmValue propertyValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(propertyValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(propertyStringPointer, &propertyValue));

	MoaMmValue agentInfoPropListValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(agentInfoPropListValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewPropListValue(&agentInfoPropListValue));

	MoaMmValue nameValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(nameValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->StringToValue(agentInfo.name.c_str(), &nameValue));
	RETURN_ERR(appendToPropList(symbols.Name, nameValue, agentInfoPropListValue));

	MoaMmValue pathExtensionsValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(pathExtensionsValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewListValue(&pathExtensionsValue));

	for (Path::EXTENSION_MAPPED_VECTOR::CONST_ITERATOR pathExtensionsIterator = agentInfo.pathExtensions.cbegin(); pathExtensionsIterator != agentInfo.pathExtensions.cend(); pathExtensionsIterator++) {
		RETURN_ERR(appendToList(pathExtensionsIterator->c_str(), pathExtensionsValue));
	}

	RETURN_ERR(appendToPropList(symbols.PathExtensions, pathExtensionsValue, agentInfoPropListValue));

	MoaMmValue hiddenValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(hiddenValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmValueInterfacePointer->IntegerToValue(agentInfo.hidden != FALSE, &hiddenValue));
	RETURN_ERR(appendToPropList(symbols.Hidden, hiddenValue, agentInfoPropListValue));

	MoaMmValue writerClassIDsValue = kVoidMoaMmValueInitializer;

	SCOPE_EXIT {
		releaseValue(writerClassIDsValue, mmValueInterfacePointer);
	};

	RETURN_ERR(mmListInterfacePointer->NewListValue(&writerClassIDsValue));

	MoaMmValue writerClassIDValue = kVoidMoaMmValueInitializer;

	for (Agent::Info::WRITER_VECTOR::const_iterator writerVectorIterator = agentInfo.writerVector.begin(); writerVectorIterator != agentInfo.writerVector.end(); writerVectorIterator++) {
		SCOPE_EXIT {
			releaseValue(writerClassIDValue, mmValueInterfacePointer);
		};

		RETURN_ERR(toValue(writerVectorIterator->classID, writerClassIDValue));
		RETURN_ERR(mmListInterfacePointer->AppendValueToList(&writerClassIDsValue, &writerClassIDValue));
	}

	RETURN_ERR(appendToPropList(symbols.WriterClassIDs, writerClassIDsValue, agentInfoPropListValue));
	return mmListInterfacePointer->AppendValueToPropList(&propListValue, &propertyValue, &agentInfoPropListValue);
}