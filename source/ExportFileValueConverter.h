#pragma once
#include "shared.h"
#include "ValueConverter.h"
#include "Args.h"
#include "Path.h"
#include "Label.h"
#include "Agent.h"
#include "Options.h"
#include "IconValues.h"

#include "mmixasst.h"

class ExportFileValueConverter : public ValueConverter {
	protected:
	struct Symbols {
		MoaMmSymbol Agent = 0;
		MoaMmSymbol AgentOptions = 0;
		MoaMmSymbol AlternatePathExtension = 0;
		MoaMmSymbol Basename = 0;
		MoaMmSymbol BW = 0;
		MoaMmSymbol Color = 0;
		MoaMmSymbol Current = 0;
		MoaMmSymbol Dirname = 0;
		MoaMmSymbol Filename = 0;
		MoaMmSymbol Extension = 0;
		MoaMmSymbol PathExtensions = 0;
		MoaMmSymbol Hidden = 0;
		MoaMmSymbol IncrementFilename = 0;
		MoaMmSymbol Interface = 0;
		MoaMmSymbol Label = 0;
		MoaMmSymbol Linked = 0;
		MoaMmSymbol Location = 0;
		MoaMmSymbol Mask = 0;
		MoaMmSymbol Member = 0;
		MoaMmSymbol Name = 0;
		MoaMmSymbol NewFolder = 0;
		MoaMmSymbol Options = 0;
		MoaMmSymbol Path = 0;
		MoaMmSymbol PathInfo = 0;
		MoaMmSymbol ReplaceExistingFile = 0;
		MoaMmSymbol WriterClassID = 0;
		MoaMmSymbol WriterClassIDs = 0;
	};

	Symbols symbols;

	MoaError getSymbols();

	public:
	ExportFileValueConverter(PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaMmList mmListInterfacePointer, PIMoaCalloc callocInterfacePointer);
	MoaError toAsset(PIMoaDrCastMem drCastMemInterfacePointer, PIMoaMmXAsset &mmXAssetInterfacePointer);
	
	using ValueConverter::toValue;
	MoaError toValue(Args &args, MoaMmValue &value);
	MoaError toValue(Path::Info &pathInfo, MoaMmValue &value, std::string &path);
	MoaError toValue(Path::Info &pathInfo, MoaMmValue &value);
	MoaError toValue(const Label::MAPPED_VECTOR &labelMappedVector, MoaMmValue &value);
	MoaError toValue(const Agent::Info::MAP &agentInfoMap, MoaMmValue &value);
	MoaError toValue(const Options &options, MoaMmValue &value);
	MoaError toValue(const IconValues::ICON_VALUE_MAP &iconValueMap, MoaMmValue &value, PIMoaDrCastMem drCastMemInterfacePointer);

	MoaError testMemberLinked(MoaLong &linked, PIMoaDrCastMem drCastMemInterfacePointer);

	using ValueConverter::appendToPropList;
	MoaError appendToPropList(ConstPMoaChar propertyStringPointer, const Agent::Info &agentInfo, MoaMmValue &propListValue);
};