/*
ADOBE SYSTEMS INCORPORATED
Copyright 1994 - 2008 Adobe Macromedia Software LLC
All Rights Reserved

NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the 
terms of the Adobe license agreement accompanying it.  If you have received this file from a 
source other than Adobe, then your use, modification, or distribution of it requires the prior 
written permission of Adobe.
*/

#define _USING_V110_SDK71_
#define _CRT_NO_VA_START_VALIDATION

#define INITGUID 1

#include "script.h"
#include "Mixer.h"
#include "BitmapImporter.h"
#include "Formats.h"

#include "mmtypes.h"
#include "moaxtra.h"
#include "moastdif.h"
#include "mmixscrp.h"
#include "mmixasst.h"
#include "mmiimage.h"
#include "driservc.h"

#pragma warning(push)
#pragma warning(disable : 5040)
#include "xmmvalue.h"
#pragma warning(pop)

#include "drivalue.h"
#include "mixsvc.h"
#include "mixpix.h"
#include "moafile.h"
#include "xclassver.h"
#include "moatry.h"

#ifdef WINDOWS
#include <Shlobj.h>
#include <process.h>
#endif

/* Begin Xtra */
/*******************************************************************************
* SCRIPTING XTRA MESSAGE TABLE DESCRIPTION.
*
* the general format is:
* xtra <nameOfXtra>
* new object me [ args ... ]
* <otherHandlerDefintions>
* --
* the first line must give the name of the Scripting xtra.
* the remaining lines give the names of the handlers that this xtra implements
* along with the required types of the arguments for each handler.
*
* -- Pre-defined handler new
* the new handler will be called when a child object is created,
* the first argument is always the child object and you defined any remaining arguments.
* the new handler is the place to initialize any memory required by the child object.
*
* -- Simple Handler Definitions
* Each handler definition line is format like this:
* <handlerName> <argType1> <argName1>, <argType2> <argName2> ...
* the first word is the handler name. Following this are types description for
* the argument to the handler, each separated by a comma.
* the argument name <argName>, may be omited.
* Permited argument types are:
* 	integer
* 	float
* 	string
* 	symbol
* 	object
* 	any
* 	*
* For integer, float, string, symbol, and object, the type of the argument must
* match. the type any means allow any type. the asterisk (*) means any number and
* any type of arguments.
*
* the first argument is the child object and is always declared to be of type object.
*
* -- Global Handlers
* An asterisk (*) preceeding the handler name signifies a global handler.
* This handler is at the global scope level and can be called from any
* movie.
*
* -- Xtra level handlers
* A plus (+) preceeding the handler name signifies an Xtra level handler.
* This kind of handler can be called directly from the Xtra reference,
* without creating a child object.
*
* the enumerated list that follows must correspond directly with the msgTable
* (i.e. they must be in the same order).
*
*******************************************************************************/

/* This is the list of handlers for the Xtra. The versionInfo string is combined
/*	with the msgTable string in the Register method to create a single string that
/* is used when registering the Xtra as a Scripting Xtra. */

static char versionInfo[] = "xtra ExportFile -- version %s.%s.%s\n";
static char msgTable[] = {
	"new object me\n" /* standard first handler entry in all message tables */
	"* exportFileStatus -- returns the error code of the last method called\n"
	"* exportFileOut object member, * path, label, agent, options -- exports the content of a member out to a file\n"
	"-- Tell Export File Properties\n"
	"* tellExportFileMixerSaved integer mixerSavedCallHandler -- turns the mixerSaved handler on (TRUE) or off (FALSE)\n"
	"-- Get Export File Properties\n"
	"* getExportFileLabelList object member, * options -- gets a list of labels\n"
	"* getExportFileAgentPropList object member, * path, label, options -- gets a property list of agents\n"
	"* getExportFileDefaultPath object member, * info, label, agent, options -- gets the default path\n"
	"* getExportFileDefaultLabel object member, * path, options -- gets the default label\n"
	"* getExportFileDefaultAgent object member, * path, label, options -- gets the default agent\n"
	"* getExportFileDefaultOptions object member, * path, label, agent -- gets the default options\n"
	"* getExportFileDisplayName object member, * path, label, agent, options -- gets a display name\n"
	"* getExportFileTypePropList -- gets a property list of types\n"
	"* getExportFileIconPropList object member -- gets a property list with any combination of #color, #bw, and #mask properties (Director 8 or newer)\n"
	"-- Alternate Syntax\n"
	"+ Status object me -- alternate syntax for exportFileStatus\n"
	"+ Out object me, object member, * path, label, agent, options -- alternate syntax for exportFileOut\n"
	"+ callTell object me, symbol propertyName, * -- alternate syntax for Tell Export File Properties\n"
	"+ callGet object me, symbol propertyName, * -- alternate syntax for Get Export File Properties"
};

STDMETHODIMP_(MoaError) GetType_TStdXtra(PMoaMmSymbol typeSymbolPointer, PIMoaMmValue mmValueInterfacePointer, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
	moa_try

	ThrowNull(typeSymbolPointer);
	ThrowNull(mmValueInterfacePointer);
	ThrowNull(registryEntryDictInterfacePointer);

	MoaChar type[kMoaMmMaxPropName] = "";

	ThrowErr(registryEntryDictInterfacePointer->Get(kMoaMmDictType_SymbolString, type, kMoaMmMaxPropName, kMoaMmDictKey_SymbolString));
	ThrowErr(mmValueInterfacePointer->StringToSymbol(type, typeSymbolPointer));

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) GetAssetInfoExtensions_TStdXtra(Asset::Info* assetInfoPointer, Registry::AssetInfoMap* registryEntriesAssetInfoMapPointer, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
	PMoaVoid fileExtListPointer = NULL;
	
	moa_try

	ThrowNull(assetInfoPointer);
	ThrowNull(registryEntriesAssetInfoMapPointer);
	ThrowNull(registryEntryDictInterfacePointer);

	Registry::AssetInfoMap &registryEntriesAssetInfoMap = *registryEntriesAssetInfoMapPointer;
	ThrowNull(registryEntriesAssetInfoMap.callocInterfacePointer);

	// extensions
	MoaLong fileExtListSize = 0;

	MoaError err = registryEntryDictInterfacePointer->GetInfo(NULL, &fileExtListSize, kAgentRegKey_FileExts);

	if (err == kMoaErr_NoErr) {
		fileExtListPointer = registryEntriesAssetInfoMap.callocInterfacePointer->NRAlloc(fileExtListSize);
		ThrowNull(fileExtListPointer);

		err = registryEntryDictInterfacePointer->Get(kMoaDictType_CString, fileExtListPointer, fileExtListSize, kAgentRegKey_FileExts);

		if (err == kMoaErr_NoErr) {
			// error intentionally not returned here so one Xtra doesn't break it for everyone
			// instead we just clear the path extensions
			if (!Path::filterPatternExtensions((PMoaChar)fileExtListPointer, assetInfoPointer->pathExtensions)) {
				assetInfoPointer->pathExtensions.clear();
			}
		}
	}

	moa_catch
	moa_catch_end

	if (registryEntriesAssetInfoMapPointer) {
		freeMemory(fileExtListPointer, registryEntriesAssetInfoMapPointer->callocInterfacePointer);
	}

	moa_try_end
}

STDMETHODIMP_(MoaError) GetAssetInfoIcon_TStdXtra(MoaDictTypeID dictTypeID, ConstPMoaChar keyStringPointer, IconValues::POINTER iconValuesPointer, RESOURCE_ID resourceID, Registry::AssetInfoMap* registryEntriesAssetInfoMapPointer, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
	moa_try

	ThrowNull(keyStringPointer);
	ThrowNull(iconValuesPointer);
	ThrowNull(registryEntriesAssetInfoMapPointer);
	ThrowNull(registryEntryDictInterfacePointer);

	Registry::AssetInfoMap &registryEntriesAssetInfoMap = *registryEntriesAssetInfoMapPointer;
	ThrowNull(registryEntriesAssetInfoMap.bitmapImporterPointer);

	MoaLong iconSize = 0;
	
	ThrowErr(registryEntryDictInterfacePointer->GetInfo(NULL, &iconSize, keyStringPointer));

	#ifdef MACINTOSH
	// no idea
	#endif
	#ifdef WINDOWS
	GlobalHandleLock<>::GlobalHandle iconGlobalHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, iconSize);
	#endif

	ThrowNull(iconGlobalHandle);

	{
		GlobalHandleLock<> iconGlobalHandleLock(iconGlobalHandle);

		// NewImageFromStream doesn't allow setting the color depth, so this is done with BitmapImporter instead
		ThrowErr(registryEntryDictInterfacePointer->Get(dictTypeID, iconGlobalHandleLock.get(), iconSize, keyStringPointer));
		ThrowErr(registryEntriesAssetInfoMap.bitmapImporterPointer->insertIntoIconValues(iconGlobalHandleLock.getGlobalHandle(), *iconValuesPointer, resourceID));
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) GetAssetInfoIcons_TStdXtra(Asset::Info* assetInfoPointer, Registry::AssetInfoMap* registryEntriesAssetInfoMapPointer, PIMoaRegistryEntryDict registryEntryDictInterfacePointer) {
	moa_try

	ThrowNull(assetInfoPointer);
	ThrowNull(registryEntriesAssetInfoMapPointer);
	ThrowNull(registryEntryDictInterfacePointer);

	Registry::AssetInfoMap &registryEntriesAssetInfoMap = *registryEntriesAssetInfoMapPointer;
	ThrowNull(registryEntriesAssetInfoMap.mmValueInterfacePointer);

	if (registryEntriesAssetInfoMap.mmImageInterfacePointer) {
		IconValues::POINTER iconValuesPointer = std::make_shared<IconValues>(registryEntriesAssetInfoMap.mmValueInterfacePointer, registryEntriesAssetInfoMap.mmImageInterfacePointer);
		ThrowNull(iconValuesPointer);

		#define SET_ICON_VALUE(dictTypeID, keyStringPointer, resourceID) (GetAssetInfoIcon_TStdXtra(\
			(dictTypeID),\
			(keyStringPointer),\
			iconValuesPointer,\
			(resourceID),\
			&registryEntriesAssetInfoMap,\
			registryEntryDictInterfacePointer\
		))

		MoaError err = SET_ICON_VALUE(kMoaMmDictType_ColorIcon, kMoaMmDictKey_ColorIcon, IDB_ASSET_INFO_MAP_ICON_COLOR);
		MoaError err2 = SET_ICON_VALUE(kMoaMmDictType_BWIcon, kMoaMmDictKey_BWIcon, IDB_ASSET_INFO_MAP_ICON_BW);

		// if neither icon value is set, don't add them to the map
		// this will cause getExportFileIconPropList to fall back to the default Xtra Media icon
		if (err == kMoaErr_NoErr
		|| err2 == kMoaErr_NoErr) {
			assetInfoPointer->iconValuesMap.insert({ "", iconValuesPointer });
		}
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) MoaCacheRegistryEntryEnumProc_RegistryEntryDict_TStdXtra(PIMoaRegistryEntryDict registryEntryDictInterfacePointer, ConstPMoaClassID classIDPointer, ConstPMoaInterfaceID interfaceIDPointer, PMoaVoid refCon) {
	bool result = false;
	
	moa_try

	ThrowNull(registryEntryDictInterfacePointer);
	ThrowNull(classIDPointer);
	ThrowNull(interfaceIDPointer);
	ThrowNull(refCon);

	Registry::Entry::VARIANT &registryEntryVariant = *(Registry::Entry::VARIANT*)refCon;

	if (std::holds_alternative<PIMoaRegistryEntryDict>(registryEntryVariant)) {
		result = true;
		Throw(kMoaStatus_False);
	}

	if (std::holds_alternative<Registry::Entry>(registryEntryVariant)) {
		const Registry::Entry &REGISTRY_ENTRY = std::get<Registry::Entry>(registryEntryVariant);
		ThrowNull(REGISTRY_ENTRY.classIDPointer);
		ThrowNull(REGISTRY_ENTRY.interfaceIDPointer);

		MoaLong equalID = MoaEqualID(classIDPointer, REGISTRY_ENTRY.classIDPointer)
			&& MoaEqualID(interfaceIDPointer, REGISTRY_ENTRY.interfaceIDPointer);

		if (equalID) {
			registryEntryVariant = (PIMoaRegistryEntryDict)NULL;
			setInterface((PPMoaVoid)&std::get<PIMoaRegistryEntryDict>(registryEntryVariant), registryEntryDictInterfacePointer);
			result = true;
			Throw(kMoaStatus_False);
		}
	} else {
		Throw(kMoaErr_InternalError);
	}

	moa_catch

	// the false status is treated as an error that goes to the catch block
	// so we need a redundant result boolean
	if (!result) {
		if (refCon) {
			Registry::Entry::VARIANT &registryEntryVariant = *(Registry::Entry::VARIANT*)refCon;

			if (std::holds_alternative<PIMoaRegistryEntryDict>(registryEntryVariant)) {
				releaseInterface((PPMoaVoid)&std::get<PIMoaRegistryEntryDict>(registryEntryVariant));
			}
		}
	}

	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) MoaCacheRegistryEntryEnumProc_ExcludedTypeSymbolSet_TStdXtra(PIMoaRegistryEntryDict registryEntryDictInterfacePointer, ConstPMoaClassID classIDPointer, ConstPMoaInterfaceID interfaceIDPointer, PMoaVoid refCon) {
	moa_try

	ThrowNull(registryEntryDictInterfacePointer);
	ThrowNull(classIDPointer);
	ThrowNull(interfaceIDPointer);
	ThrowNull(refCon);

	Registry::ExcludedTypeSymbolSet &registryEntriesExcludedTypeSymbolSet = *(Registry::ExcludedTypeSymbolSet*)refCon;
	ThrowNull(registryEntriesExcludedTypeSymbolSet.mmValueInterfacePointer);
	ThrowNull(registryEntriesExcludedTypeSymbolSet.excludedTypeSymbolSetPointer);

	bool equalID = MoaEqualID(interfaceIDPointer, &IID_IMoaMmXAsset);

	if (!equalID) {
		MoaMmSymbol typeSymbol = 0;

		MoaError err = GetType_TStdXtra(&typeSymbol, registryEntriesExcludedTypeSymbolSet.mmValueInterfacePointer, registryEntryDictInterfacePointer);

		if (err == kMoaErr_NoErr) {
			registryEntriesExcludedTypeSymbolSet.excludedTypeSymbolSetPointer->insert(typeSymbol);
		}
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) MoaCacheRegistryEntryEnumProc_AssetMoaIDsHash_TStdXtra(PIMoaRegistryEntryDict registryEntryDictInterfacePointer, ConstPMoaClassID classIDPointer, ConstPMoaInterfaceID interfaceIDPointer, PMoaVoid refCon) {
	std::hash<MoaClassID> moaClassIDHash;
	std::hash<MoaInterfaceID> moaInterfaceIDHash;
	
	moa_try

	ThrowNull(registryEntryDictInterfacePointer);
	ThrowNull(classIDPointer);
	ThrowNull(interfaceIDPointer);
	ThrowNull(refCon);

	size_t &assetMoaIDsHash = *(size_t*)refCon;

	bool equalID = MoaEqualID(interfaceIDPointer, &IID_IMoaMmXAsset);

	if (equalID) {
		assetMoaIDsHash += moaClassIDHash(*classIDPointer) ^ moaInterfaceIDHash(*interfaceIDPointer);
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) MoaCacheRegistryEntryEnumProc_AssetInfoMap_TStdXtra(PIMoaRegistryEntryDict registryEntryDictInterfacePointer, ConstPMoaClassID classIDPointer, ConstPMoaInterfaceID interfaceIDPointer, PMoaVoid refCon) {
	Asset::Info assetInfo;

	moa_try

	ThrowNull(registryEntryDictInterfacePointer);
	ThrowNull(classIDPointer);
	ThrowNull(interfaceIDPointer);
	ThrowNull(refCon);

	Registry::AssetInfoMap &registryEntriesAssetInfoMap = *(Registry::AssetInfoMap*)refCon;
	ThrowNull(registryEntriesAssetInfoMap.mmValueInterfacePointer);
	ThrowNull(registryEntriesAssetInfoMap.assetInfoMapPointer);
	ThrowNull(registryEntriesAssetInfoMap.bitmapImporterPointer);

	bool equalID = MoaEqualID(interfaceIDPointer, &IID_IMoaMmXAsset);

	if (equalID) {
		MoaMmSymbol typeSymbol = 0;

		MoaError err = GetType_TStdXtra(&typeSymbol, registryEntriesAssetInfoMap.mmValueInterfacePointer, registryEntryDictInterfacePointer);

		if (err == kMoaErr_NoErr) {
			// extensions
			ThrowErr(GetAssetInfoExtensions_TStdXtra(&assetInfo, &registryEntriesAssetInfoMap, registryEntryDictInterfacePointer));

			// icons
			ThrowErr(GetAssetInfoIcons_TStdXtra(&assetInfo, &registryEntriesAssetInfoMap, registryEntryDictInterfacePointer));

			(*registryEntriesAssetInfoMap.assetInfoMapPointer)[typeSymbol] = assetInfo;
		}
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) MoaCacheRegistryEntryEnumProc_AgentHiddenReaderSet_TStdXtra(PIMoaRegistryEntryDict registryEntryDictInterfacePointer, ConstPMoaClassID classIDPointer, ConstPMoaInterfaceID interfaceIDPointer, PMoaVoid refCon) {
	std::hash<MoaClassID> moaClassIDHash;
	std::hash<MoaInterfaceID> moaInterfaceIDHash;

	moa_try

	ThrowNull(registryEntryDictInterfacePointer);
	ThrowNull(classIDPointer);
	ThrowNull(interfaceIDPointer);
	ThrowNull(refCon);

	Registry::AgentHiddenReaderSet &registryEntriesAgentHiddenReaderSet = *(Registry::AgentHiddenReaderSet*)refCon;
	ThrowNull(registryEntriesAgentHiddenReaderSet.agentMoaIDsHashPointer);
	ThrowNull(registryEntriesAgentHiddenReaderSet.agentHiddenReaderSetPointer);

	MoaError err = kMoaErr_NoErr;

	// if the type is a reader
	bool equalID = MoaEqualID(interfaceIDPointer, &IID_IMoaReader);

	if (equalID) {
		MoaBool hidden = FALSE;
		const MoaLong HIDDEN_SIZE = sizeof(hidden);

		err = registryEntryDictInterfacePointer->Get(kMoaDictType_Bool, &hidden, HIDDEN_SIZE, kReaderRegKey_Hidden);

		// default to FALSE in case of error
		if (err == kMoaErr_NoErr
		&& hidden) {
			registryEntriesAgentHiddenReaderSet.agentHiddenReaderSetPointer->insert(*classIDPointer);
		}
	}

	// if the type is either a reader OR writer
	equalID = equalID ? TRUE : MoaEqualID(interfaceIDPointer, &IID_IMoaWriter);

	// do not XOR with the agent/asset MoaID hashes, add to them - only XOR the individual GUIDs
	// the same Xtra can have multiple interfaces (such as both Reader and Writer)
	// so the same Xtra's ClassID can appear here twice
	// we don't want them to cancel to zero
	if (equalID) {
		*registryEntriesAgentHiddenReaderSet.agentMoaIDsHashPointer += moaClassIDHash(*classIDPointer) ^ moaInterfaceIDHash(*interfaceIDPointer);
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) GetProductVersionMajor_TStdXtra(TStdXtra* This) {
	PIMoaAppInfo appInfoInterfacePointer = NULL;

	PMoaVoid productVersionStringPointer = NULL;

	moa_try

	ThrowNull(This);

	MoaError err = This->pCallback->QueryInterface(&IID_IMoaAppInfo, (PPMoaVoid)&appInfoInterfacePointer);
	
	if (err == kMoaErr_NoErr
	&& appInfoInterfacePointer) {
		MoaUlong productVersionStringSize = 8;

		do {
			if (productVersionStringPointer) {
				productVersionStringSize += productVersionStringSize;
			}

			freeMemory(productVersionStringPointer, This->pCalloc);

			productVersionStringPointer = This->pCalloc->NRAlloc(productVersionStringSize);
			ThrowNull(productVersionStringPointer);

			err = appInfoInterfacePointer->GetInfo(kMoaAppInfo_ProductVersion, (PMoaChar)productVersionStringPointer, productVersionStringSize);
		} while (err == kMoaErr_BadParam);

		ThrowErr(err);

		This->productVersionMajor = strtoul((PMoaChar)productVersionStringPointer, 0, 10);
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&appInfoInterfacePointer);

	if (This) {
		freeMemory(productVersionStringPointer, This->pCalloc);
	}

	moa_try_end
}

STDMETHODIMP_(MoaError) GetSymbols_TStdXtra(TStdXtra* This) {
	moa_try

	ThrowNull(This);

	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("AgentOptions", &This->symbols.AgentOptions));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("AgentPropList", &This->symbols.AgentPropList));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("AlternatePathExtension", &This->symbols.AlternatePathExtension));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Basename", &This->symbols.Basename));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Bitmap", &This->symbols.Bitmap));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Composite", &This->symbols.Composite));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Current", &This->symbols.Current));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("DefaultAgent", &This->symbols.DefaultAgent));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("DefaultLabel", &This->symbols.DefaultLabel));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("DefaultOptions", &This->symbols.DefaultOptions));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("DefaultPath", &This->symbols.DefaultPath));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Dirname", &This->symbols.Dirname));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("DisplayName", &This->symbols.DisplayName));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Documents", &This->symbols.Documents));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Empty", &This->symbols.Empty));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Extension", &This->symbols.Extension));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Filename", &This->symbols.Filename));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("IconPropList", &This->symbols.IconPropList));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Image", &This->symbols.Image));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("IncrementFilename", &This->symbols.IncrementFilename));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("LabelList", &This->symbols.LabelList));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Lingo", &This->symbols.Lingo));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Loaded", &This->symbols.Loaded));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Location", &This->symbols.Location));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("MixerSaved", &This->symbols.MixerSaved));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Music", &This->symbols.Music));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Name", &This->symbols.Name));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("NewFolder", &This->symbols.NewFolder));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Picture", &This->symbols.Picture));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Pictures", &This->symbols.Pictures));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("ReplaceExistingFile", &This->symbols.ReplaceExistingFile));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("SampleSize", &This->symbols.SampleSize));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Script", &This->symbols.Script));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("ScriptSyntax", &This->symbols.ScriptSyntax));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("ScriptText", &This->symbols.ScriptText));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Sound", &This->symbols.Sound));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Type", &This->symbols.Type));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("TypePropList", &This->symbols.TypePropList));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("Videos", &This->symbols.Videos));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("WriterClassID", &This->symbols.WriterClassID));
	ThrowErr(This->mmValueInterfacePointer->StringToSymbol("XtraMedia", &This->symbols.XtraMedia));

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(MoaError) MoaCreate_TStdXtra(TStdXtra* This) {
	moa_try

	ThrowNull(This);

	This->cacheInterfacePointer = This->pCallback->MoaGetCache();
	ThrowNull(This->cacheInterfacePointer);

	ThrowErr(This->pCallback->QueryInterface(&IID_IMoaDrPlayer, (PPMoaVoid)&This->drPlayerInterfacePointer));
	ThrowNull(This->drPlayerInterfacePointer);

	ThrowErr(This->pCallback->QueryInterface(&IID_IMoaMmValue, (PPMoaVoid)&This->mmValueInterfacePointer));
	ThrowNull(This->mmValueInterfacePointer);

	ThrowErr(This->pCallback->QueryInterface(&IID_IMoaDrValue, (PPMoaVoid)&This->drValueInterfacePointer));
	ThrowNull(This->drValueInterfacePointer);

	ThrowErr(This->pCallback->QueryInterface(&IID_IMoaMmList, (PPMoaVoid)&This->mmListInterfacePointer));
	ThrowNull(This->mmListInterfacePointer);

	ThrowErr(This->pCallback->QueryInterface(&IID_IMoaDrUtils, (PPMoaVoid)&This->drUtilsInterfacePointer));
	ThrowNull(This->drUtilsInterfacePointer);

	ThrowErr(This->pCallback->QueryInterface(&IID_IMoaHandle, (PPMoaVoid)&This->handleInterfacePointer));
	ThrowNull(This->handleInterfacePointer);

	MoaError err = This->pCallback->QueryInterface(&IID_IMoaMmImage, (PPMoaVoid)&This->mmImageInterfacePointer);

	if (err != kMoaErr_NoErr) {
		This->mmImageInterfacePointer = NULL;
	}

	err = This->pCallback->QueryInterface(&IID_IMoaDrMediaValue, (PPMoaVoid)&This->drMediaValueInterfacePointer);

	if (err != kMoaErr_NoErr) {
		This->drMediaValueInterfacePointer = NULL;
	}

	This->registryEntryVariantPointer = new Registry::Entry::VARIANT({});
	ThrowNull(This->registryEntryVariantPointer);

	Registry::Entry::VARIANT &registryEntryVariant = *This->registryEntryVariantPointer;

	Registry::Entry &registryEntry = std::get<Registry::Entry>(registryEntryVariant);
	registryEntry.classIDPointer = (MoaClassID*)&CLSID_TStdXtra;
	registryEntry.interfaceIDPointer = (MoaInterfaceID*)&IID_IMoaMmXScript;

	This->exportFileValueConverterPointer = new ExportFileValueConverter(
		This->drPlayerInterfacePointer,
		This->mmValueInterfacePointer,
		This->mmListInterfacePointer,
		This->pCalloc
	);

	ThrowNull(This->exportFileValueConverterPointer);

	This->labelsInfoPointer = new Label::Labels::Info(This->mmValueInterfacePointer);
	ThrowNull(This->labelsInfoPointer);

	This->typeLabelsPointer = new TypeLabel::TypeLabels(This->mmValueInterfacePointer);
	ThrowNull(This->typeLabelsPointer);

	// this is not done here
	//This->assetsInfoPointer = new Asset::Assets::Info(...);
	//ThrowNull(This->assetsInfoPointer);

	This->labelAgentInfoMapPointer = new Label::AGENT_INFO_MAP;
	ThrowNull(This->labelAgentInfoMapPointer);

	This->assetInfoMapPointer = new Asset::Info::MAP;
	ThrowNull(This->assetInfoMapPointer);

	#ifdef WINDOWS
	This->threadVectorPointer = new HANDLE_VECTOR;
	ThrowNull(This->threadVectorPointer);

	This->moduleHandleVectorPointer = new MODULE_HANDLE_VECTOR;
	ThrowNull(This->moduleHandleVectorPointer);
	#endif

	ThrowErr(GetProductVersionMajor_TStdXtra(This));
	ThrowErr(GetSymbols_TStdXtra(This));

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(void) MoaDestroy_TStdXtra(TStdXtra* This) {
	#ifdef WINDOWS
	HANDLE_VECTOR::iterator threadVectorIterator = {};
	MODULE_HANDLE_VECTOR::iterator moduleHandleVectorIterator = {};
	#endif

	moa_try

	ThrowNull(This);

	releaseInterface((PPMoaVoid)&This->cacheInterfacePointer);
	releaseInterface((PPMoaVoid)&This->drPlayerInterfacePointer);
	releaseInterface((PPMoaVoid)&This->mmValueInterfacePointer);
	releaseInterface((PPMoaVoid)&This->drValueInterfacePointer);
	releaseInterface((PPMoaVoid)&This->mmListInterfacePointer);
	releaseInterface((PPMoaVoid)&This->drUtilsInterfacePointer);
	releaseInterface((PPMoaVoid)&This->handleInterfacePointer);

	releaseInterface((PPMoaVoid)&This->mmImageInterfacePointer);
	releaseInterface((PPMoaVoid)&This->drMediaValueInterfacePointer);

	releaseInterface((PPMoaVoid)&This->formatServicesInterfacePointer);
	releaseInterface((PPMoaVoid)&This->dataObjectServicesInterfacePointer);
	releaseInterface((PPMoaVoid)&This->agentServicesInterfacePointer);

	if (This->registryEntryVariantPointer) {
		Registry::Entry::VARIANT &registryEntryVariant = *This->registryEntryVariantPointer;

		if (std::holds_alternative<PIMoaRegistryEntryDict>(registryEntryVariant)) {
			releaseInterface((PPMoaVoid)&std::get<PIMoaRegistryEntryDict>(registryEntryVariant));
		}

		delete This->registryEntryVariantPointer;
		This->registryEntryVariantPointer = 0;
	}

	if (This->exportFileValueConverterPointer) {
		delete This->exportFileValueConverterPointer;
		This->exportFileValueConverterPointer = 0;
	}

	if (This->labelsInfoPointer) {
		delete This->labelsInfoPointer;
		This->labelsInfoPointer = 0;
	}

	if (This->typeLabelsPointer) {
		delete This->typeLabelsPointer;
		This->typeLabelsPointer = 0;
	}

	if (This->assetsInfoPointer) {
		delete This->assetsInfoPointer;
		This->assetsInfoPointer = 0;
	}

	if (This->labelAgentInfoMapPointer) {
		delete This->labelAgentInfoMapPointer;
		This->labelAgentInfoMapPointer = 0;
	}

	if (This->assetInfoMapPointer) {
		delete This->assetInfoMapPointer;
		This->assetInfoMapPointer = 0;
	}

	#ifdef WINDOWS
	MoaError err = kMoaErr_NoErr;

	if (This->threadVectorPointer) {
		{
			HANDLE_VECTOR &threadVector = *This->threadVectorPointer;

			for (threadVectorIterator = threadVector.begin(); threadVectorIterator != threadVector.end(); threadVectorIterator++) {
				err = errOrDefaultErr(osErr(closeThread(*threadVectorIterator)), err);
			}
		}

		delete This->threadVectorPointer;
		This->threadVectorPointer = 0;
	}

	if (This->moduleHandleVectorPointer) {
		{
			MODULE_HANDLE_VECTOR &moduleHandleVector = *This->moduleHandleVectorPointer;

			for (moduleHandleVectorIterator = moduleHandleVector.begin(); moduleHandleVectorIterator != moduleHandleVector.end(); moduleHandleVectorIterator++) {
				err = errOrDefaultErr(osErr(freeLibrary(*moduleHandleVectorIterator)), err);
			}
		}

		delete This->moduleHandleVectorPointer;
		This->moduleHandleVectorPointer = 0;
	}

	ThrowErr(err);
	#endif

	moa_catch
	moa_catch_end
	moa_try_end_void
}

STD_INTERFACE_CREATE_DESTROY(TStdXtra, IMoaRegister)

BEGIN_DEFINE_CLASS_INTERFACE(TStdXtra, IMoaRegister)
END_DEFINE_CLASS_INTERFACE

STDMETHODIMP TStdXtra_IMoaRegister::Register(PIMoaCache cacheInterfacePointer, PIMoaXtraEntryDict xtraEntryDictInterfacePointer) {
	PMoaVoid memoryStringPointer = NULL;

	moa_try

	ThrowNull(cacheInterfacePointer);
	ThrowNull(xtraEntryDictInterfacePointer);

	{
		// this interface should NOT be released
		PIMoaRegistryEntryDict registryEntryDictInterfacePointer = NULL;
		ThrowErr(cacheInterfacePointer->AddRegistryEntry(xtraEntryDictInterfacePointer, &CLSID_TStdXtra, &IID_IMoaMmXScript, &registryEntryDictInterfacePointer));
		ThrowNull(registryEntryDictInterfacePointer);

		const char* VER_MAJORVERSION_STRING = "0";
		const char* VER_MINORVERSION_STRING = "4";
		const char* VER_BUGFIXVERSION_STRING = "9";

		const size_t VERSION_STRING_SIZE = min(256, kMoaMmMaxXtraMessageTable);
		char versionString[VERSION_STRING_SIZE] = "";

		if (sprintf_s(versionString, VERSION_STRING_SIZE, versionInfo, VER_MAJORVERSION_STRING, VER_MINORVERSION_STRING, VER_BUGFIXVERSION_STRING) == -1) {
			Throw(kMoaErr_OutOfMem);
		}

		MoaUlong versionStringSize = stringSizeMax(versionString, VERSION_STRING_SIZE);
		MoaUlong memoryStringSize = versionStringSize + strnlen_s(msgTable, kMoaMmMaxXtraMessageTable - versionStringSize);

		memoryStringPointer = pObj->pCalloc->NRAlloc(memoryStringSize);
		ThrowNull(memoryStringPointer);

		if (strncpy_s((char*)memoryStringPointer, memoryStringSize, versionString, versionStringSize)) {
			Throw(kMoaErr_OutOfMem);
		}

		if (strcat_s((char*)memoryStringPointer, memoryStringSize, msgTable)) {
			Throw(kMoaErr_OutOfMem);
		}

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDrDictType_MessageTable, memoryStringPointer, memoryStringSize, kMoaDrDictKey_MessageTable));

		Registry::Entry::VARIANT &registryEntryVariant = *pObj->registryEntryVariantPointer;
		registryEntryVariant = (PIMoaRegistryEntryDict)NULL;
		setInterface((PPMoaVoid)&std::get<PIMoaRegistryEntryDict>(registryEntryVariant), registryEntryDictInterfacePointer);
	}

	moa_catch
	moa_catch_end

	// always do this, whether there is an error or not
	freeMemory(memoryStringPointer, pObj->pCalloc);

	moa_try_end
}

STD_INTERFACE_CREATE_DESTROY(TStdXtra, IMoaMmXScript)

BEGIN_DEFINE_CLASS_INTERFACE(TStdXtra, IMoaMmXScript)
END_DEFINE_CLASS_INTERFACE

#define LABEL_INFO_NOT_FOUND (pObj->labelsInfoPointer->get().cend())

STDMETHODIMP TStdXtra_IMoaMmXScript::Call(PMoaDrCallInfo callPtr) {
	moa_try

	ThrowNull(callPtr);

	const MoaLong ARGS_BASE_STANDARD_SYNTAX = 0;
	const MoaLong ARGS_BASE_ALTERNATE_SYNTAX_EXPORT_FILE = 1;
	const MoaLong ARGS_BASE_ALTERNATE_SYNTAX_CALL = 2;

	MoaError err = kMoaErr_NoErr;

	pObj->argsBase = ARGS_BASE_STANDARD_SYNTAX;

	switch (callPtr->methodSelector) {
		case m_exportFile_Status:
		pObj->argsBase = ARGS_BASE_ALTERNATE_SYNTAX_EXPORT_FILE;
		[[fallthrough]];
		case m_exportFileStatus:
		Throw(ExportFileStatus(callPtr));
		break;
		case m_exportFile_Out:
		pObj->argsBase = ARGS_BASE_ALTERNATE_SYNTAX_EXPORT_FILE;
		[[fallthrough]];
		case m_exportFileOut:
		err = ExportFileOut(callPtr);
		break;
		case m_tellExportFileMixerSaved:
		err = TellExportFileMixerSaved(callPtr);
		break;
		case m_getExportFileLabelList:
		err = GetExportFileLabelList(callPtr);
		break;
		case m_getExportFileAgentPropList:
		err = GetExportFileAgentPropList(callPtr);
		break;
		case m_getExportFileDefaultPath:
		err = GetExportFileDefaultPath(callPtr);
		break;
		case m_getExportFileDefaultLabel:
		err = GetExportFileDefaultLabel(callPtr);
		break;
		case m_getExportFileDefaultAgent:
		err = GetExportFileDefaultAgent(callPtr);
		break;
		case m_getExportFileDefaultOptions:
		err = GetExportFileDefaultOptions(callPtr, false);
		break;
		case m_getExportFileDefaultOptionsX:
		err = GetExportFileDefaultOptions(callPtr, true);
		break;
		case m_getExportFileDisplayName:
		err = GetExportFileDisplayName(callPtr);
		break;
		case m_getExportFileTypePropList:
		err = GetExportFileTypePropList(callPtr);
		break;
		case m_getExportFileIconPropList:
		err = GetExportFileIconPropList(callPtr);
		break;
		case m_callTellExportFile:
		pObj->argsBase = ARGS_BASE_ALTERNATE_SYNTAX_CALL;
		err = CallTell(callPtr);
		break;
		case m_callGetExportFile:
		pObj->argsBase = ARGS_BASE_ALTERNATE_SYNTAX_CALL;
		err = CallGet(callPtr);
	}

	// for Mac: these macros seem to live in CFPlugInCOM.h I think?
	if (IS_ERROR(err)) {
		// always return void for errors
		// (use exportFileStatus handler to get the last error)
		releaseValue(callPtr->resultValue, pObj->mmValueInterfacePointer);
	}

	// in a low memory scenario, we want the last error to survive an Xtra unload/reload
	// so save it in the cache
	{
		// do NOT release this interface (it's owned by the Xtra and will be released by it)
		PIMoaRegistryEntryDict registryEntryDictInterfacePointer = NULL;
		ThrowErr(GetRegistryEntryDict(&registryEntryDictInterfacePointer));

		MoaLong lastErrCode = HRESULT_CODE(err);
		ThrowErr(Registry::Entry::setValueLong(kExportFileRegKey_LastErrCode, lastErrCode, registryEntryDictInterfacePointer));
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::ExportFileStatus(PMoaDrCallInfo callPtr) {
	moa_try

	ThrowNull(callPtr);

	{
		// do NOT release this interface (it's owned by the Xtra and will be released by it)
		PIMoaRegistryEntryDict registryEntryDictInterfacePointer = NULL;
		ThrowErr(GetRegistryEntryDict(&registryEntryDictInterfacePointer));

		MoaLong lastErrCode = 0;
		ThrowErr(Registry::Entry::getValueLong(kExportFileRegKey_LastErrCode, lastErrCode, registryEntryDictInterfacePointer));

		ThrowErr(pObj->mmValueInterfacePointer->IntegerToValue(lastErrCode, &callPtr->resultValue));
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::ExportFileOut(PMoaDrCallInfo callPtr) {
	PIMoaFile fileInterfacePointer = NULL;

	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);
	std::string path = "";

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgs(callPtr, &args, &directorMedia));

	// this file interface will always point to the file we actually write
	// the path of this file might later be changed to an incremented or temp file path
	// which we write to instead
	ThrowErr(pObj->pCallback->MoaCreateInstance(&CLSID_CMoaFile, &IID_IMoaFile, (PPMoaVoid)&fileInterfacePointer));
	ThrowNull(fileInterfacePointer);

	ThrowErr(CreateContentFormat(&args, &directorMedia));

	if (!directorMedia.contentPointer) {
		Throw(kMoaErr_InternalError);
	}

	Media::DirectorMedia::Content &content = *directorMedia.contentPointer;

	if (!content.formatPointer) {
		Throw(kMoaErr_InternalError);
	}

	if (!args.pathInfoOptional.has_value()) {
		Throw(kMoaErr_InternalError);
	}

	// we do not want to resolve this path
	// (that is, check the searchPath and whatnot for an existing file with this name)
	// the PATH_RELATIVE constant is because some formats (mixers) can't use relative paths
	if (!args.pathInfoOptional.value().getPath(path, content.formatPointer->PATH_RELATIVE)) {
		Throw(kMoaErr_BadParam);
	}

	if (path.empty()) {
		Throw(kMoaErr_InternalError);
	}

	ThrowErr(fileInterfacePointer->SetPathnameSpec(path.c_str(), FALSE));

	// sometimes CreateFile and Delete return GetLastError codes
	// NOT as an HRESULT, even though all its other returns are HRESULTs
	// this is a programming sin but nonetheless we need to deal with it, so fileErr fixes this
	// (what happens on Mac??)
	MoaError err = fileInterfacePointer->CreateFile();

	ThrowErr(fileErr(HandleCreateFileError(&args, &directorMedia, err, fileInterfacePointer)));

	err = WriteFile(&args, &directorMedia, fileInterfacePointer);

	ThrowErr(fileErr(HandleWriteFileError(&args, &directorMedia, err, fileInterfacePointer)));

	ThrowErr(pObj->exportFileValueConverterPointer->toValue(args, callPtr->resultValue));

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&fileInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::TellExportFileMixerSaved(PMoaDrCallInfo callPtr) {
	moa_try

	ThrowNull(callPtr);

	MoaLong mixerSavedCallHandler = FALSE;
	ThrowErr(GetArgMixerSavedCallHandler(callPtr, &mixerSavedCallHandler));

	{
		// do NOT release this interface
		PIMoaRegistryEntryDict registryEntryDictInterfacePointer = NULL;
		ThrowErr(GetRegistryEntryDict(&registryEntryDictInterfacePointer));
		ThrowErr(Registry::Entry::setValueLong(kExportFileRegKey_MixerSavedCallHandler, mixerSavedCallHandler, registryEntryDictInterfacePointer));
	}

	ThrowErr(pObj->mmValueInterfacePointer->IntegerToValue(mixerSavedCallHandler != FALSE, &callPtr->resultValue));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileLabelList(PMoaDrCallInfo callPtr) {
	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgMember(callPtr, &args));
	ThrowErr(GetArgOptions(callPtr, &args, &directorMedia, 2));

	ThrowErr(GetLabelMappedVector(&args, &directorMedia));

	if (!directorMedia.labelMappedVectorOptional.has_value()) {
		Throw(kMoaErr_InternalError);
	}

	ThrowErr(pObj->exportFileValueConverterPointer->toValue(directorMedia.labelMappedVectorOptional.value(), callPtr->resultValue));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileAgentPropList(PMoaDrCallInfo callPtr) {
	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgMember(callPtr, &args));
	ThrowErr(GetArgPath(callPtr, &args, &directorMedia));
	ThrowErr(GetArgLabel(callPtr, &args, &directorMedia));
	ThrowErr(GetArgOptions(callPtr, &args, &directorMedia, 4));

	ThrowErr(GetLabelAgentInfoMap(&args, &directorMedia));

	if (!directorMedia.agentInfoMapOptional.has_value()) {
		Throw(kMoaErr_InternalError);
	}

	ThrowErr(pObj->exportFileValueConverterPointer->toValue(directorMedia.agentInfoMapOptional.value(), callPtr->resultValue));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileDefaultPath(PMoaDrCallInfo callPtr) {
	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);
	std::string path = "";

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgMember(callPtr, &args));
	ThrowErr(GetArgInfo(callPtr, &args));
	ThrowErr(GetArgLabel(callPtr,  &args, &directorMedia));
	ThrowErr(GetArgAgent(callPtr, &args, &directorMedia));
	ThrowErr(GetArgOptions(callPtr, &args, &directorMedia, 5));

	// should be always set by now
	if (!args.pathInfoOptional.has_value()) {
		Throw(kMoaErr_InternalError);
	}

	if (args.infoOptional.has_value()
	&& args.infoOptional.value()) {
		ThrowErr(pObj->exportFileValueConverterPointer->toValue(args.pathInfoOptional.value(), callPtr->resultValue));
	} else {
		if (!args.pathInfoOptional.value().getPath(path)) {
			Throw(kMoaErr_BadParam);
		}

		if (path.empty()) {
			Throw(kMoaErr_InternalError);
		}

		ThrowErr(pObj->mmValueInterfacePointer->StringToValue(path.c_str(), &callPtr->resultValue));
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileDefaultLabel(PMoaDrCallInfo callPtr) {
	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgMember(callPtr, &args));
	ThrowErr(GetArgPath(callPtr, &args, &directorMedia));
	ThrowErr(GetArgLabelDefault(&args, &directorMedia));
	ThrowErr(GetArgOptions(callPtr, &args, &directorMedia, 3));

	if (!std::holds_alternative<MoaMmSymbol>(args.labelSymbolVariant)) {
		Throw(kMoaErr_InternalError);
	}

	ThrowErr(pObj->mmValueInterfacePointer->SymbolToValue(std::get<MoaMmSymbol>(args.labelSymbolVariant), &callPtr->resultValue));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileDefaultAgent(PMoaDrCallInfo callPtr) {
	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgMember(callPtr, &args));
	ThrowErr(GetArgPath(callPtr, &args, &directorMedia));
	ThrowErr(GetArgLabel(callPtr, &args, &directorMedia));
	ThrowErr(GetArgAgentDefault(&args, &directorMedia));
	ThrowErr(GetArgOptions(callPtr, &args, &directorMedia, 4));
	
	ThrowErr(pObj->mmValueInterfacePointer->StringToValue(args.agentStringOptional.value_or("").c_str(), &callPtr->resultValue));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileDefaultOptions(PMoaDrCallInfo callPtr, bool x) {
	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgMember(callPtr, &args));
	ThrowErr(GetArgPath(callPtr, &args, &directorMedia));
	ThrowErr(GetArgLabel(callPtr, &args, &directorMedia));
	ThrowErr(GetArgAgent(callPtr, &args, &directorMedia));
	ThrowErr(GetArgOptionsDefault(&args, &directorMedia, x));

	if (!args.optionsOptional.has_value()) {
		args.optionsOptional.emplace(pObj->mmValueInterfacePointer);
	}

	ThrowErr(pObj->exportFileValueConverterPointer->toValue(args.optionsOptional.value(), callPtr->resultValue));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileDisplayName(PMoaDrCallInfo callPtr) {
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);
	MoaMmValue typeValue = kVoidMoaMmValueInitializer;
	std::string displayName = "";

	moa_try

	ThrowNull(callPtr);

	ThrowErr(GetArgs(callPtr, &args, &directorMedia));

	// lock in the agent
	if (!args.agentStringOptional.has_value()) {
		args.agentStringOptional = "";
	}

	if (args.agentStringOptional.value().empty()) {
		ThrowErr(FindLabelInfo(&args, &directorMedia));

		if (directorMedia.labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
			Throw(kMoaErr_InternalError);
		}

		const Label::Info &LABEL_INFO = directorMedia.labelInfoMapIterator->second;

		// this being empty signals to use Type Display Name instead
		if (LABEL_INFO.name.empty()) {
			drCastMemInterfacePointer = args.getDrCastMemInterfacePointer();
			
			if (!drCastMemInterfacePointer) {
				Throw(kMoaErr_InternalError);
			}

			ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.Type, &typeValue));

			MoaMmSymbol typeSymbol = 0;
			ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&typeValue, &typeSymbol));

			ThrowErr(GetTypeDisplayName(typeSymbol, &displayName));
		} else {
			displayName = LABEL_INFO.name;
		}
	} else {
		ThrowErr(FindAgentInfo(&args, &directorMedia));

		if (!directorMedia.agentInfoOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		displayName = directorMedia.agentInfoOptional.value().name;
	}

	ThrowErr(pObj->mmValueInterfacePointer->StringToValue(displayName.c_str(), &callPtr->resultValue));

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);

	releaseValue(typeValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileTypePropList(PMoaDrCallInfo callPtr) {
	Registry::SYMBOL_SET excludedTypeSymbolSet = {};
	std::string typeDisplayName = "";

	moa_try

	ThrowErr(pObj->mmListInterfacePointer->NewPropListValue(&callPtr->resultValue));

	{
		Registry::ExcludedTypeSymbolSet registryEntriesExcludedTypeSymbolSet = {};

		registryEntriesExcludedTypeSymbolSet.mmValueInterfacePointer = pObj->mmValueInterfacePointer;
		registryEntriesExcludedTypeSymbolSet.excludedTypeSymbolSetPointer = &excludedTypeSymbolSet;

		ThrowFailed(
			pObj->cacheInterfacePointer->EnumerateRegistryEntries(
				MoaCacheRegistryEntryEnumProc_ExcludedTypeSymbolSet_TStdXtra,
				(PMoaVoid)&registryEntriesExcludedTypeSymbolSet
			)
		);
	}

	MoaLong typeCount = 0;
	ThrowErr(pObj->drPlayerInterfacePointer->GetCastMemTypeCount(&typeCount));
	
	MoaMmSymbol typeSymbol = 0;
	
	for (MoaLong i = 1; i <= typeCount; i++) {
		ThrowErr(pObj->drPlayerInterfacePointer->GetNthCastMemTypeSymbol(i, &typeSymbol));

		// ExportFile only concerns assets, not bitmap filters etc.
		if (excludedTypeSymbolSet.find(typeSymbol) != excludedTypeSymbolSet.end()) {
			continue;
		}

		ThrowErr(GetTypeDisplayName(typeSymbol, &typeDisplayName));
		ThrowErr(pObj->exportFileValueConverterPointer->appendToPropList(typeSymbol, typeDisplayName.c_str(), callPtr->resultValue));
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetExportFileIconPropList(PMoaDrCallInfo callPtr) {
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	Args args;
	Media::DirectorMedia directorMedia(LABEL_INFO_NOT_FOUND);
	MoaMmValue typeValue = kVoidMoaMmValueInitializer;
	MoaMmValue subTypeValue = kVoidMoaMmValueInitializer;
	IconValues::MAP::const_iterator foundIconValues = {};

	moa_try

	ThrowNull(callPtr);

	if (!pObj->mmImageInterfacePointer) {
		// pre-Director 8
		Throw(kMoaErr_BadClass);
	}

	ThrowErr(GetArgMember(callPtr, &args));

	drCastMemInterfacePointer = args.getDrCastMemInterfacePointer();
	
	if (!drCastMemInterfacePointer) {
		Throw(kMoaErr_InternalError);
	}

	ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.Type, &typeValue));

	MoaMmSymbol typeSymbol = 0;
	ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&typeValue, &typeSymbol));

	// a previous run might've already created Assets Info
	// (it requires an Active Movie so we do it on the first call to getExportFileIconPropList instead of MoaCreate)
	if (!pObj->assetsInfoPointer) {
		// the Bitmap Importer is only created if we still need to create the Assets Info
		// it is expensive to create, so it should only be created if absolutely necessary
		//if (!directorMedia.bitmapImporterPointer) {
		directorMedia.bitmapImporterPointer = std::make_shared<BitmapImporter>(
			*pObj->labelsInfoPointer,
			pObj->drPlayerInterfacePointer,
			pObj->mmValueInterfacePointer,
			pObj->drUtilsInterfacePointer,
			pObj->mmImageInterfacePointer
		);

		ThrowNull(directorMedia.bitmapImporterPointer);
		//}

		pObj->assetsInfoPointer = new Asset::Assets::Info(
			*directorMedia.bitmapImporterPointer,
			*pObj->labelsInfoPointer,
			pObj->productVersionMajor,
			pObj->mmValueInterfacePointer,
			pObj->pCallback
		);

		ThrowNull(pObj->assetsInfoPointer);
	}

	// is this an Asset Xtra type
	bool assetXtra = false;

	directorMedia.assetInfoOptional = pObj->assetsInfoPointer->find(typeSymbol);
	
	if (!directorMedia.assetInfoOptional.has_value()) {
		// not a built-in type, so try finding the Asset Xtra
		MoaError err = FindXtraAssetInfo(&args, &directorMedia);
		
		if (err == kMoaMmErr_XAssetTypeUnknown) {
			// the Asset Xtra isn't loaded: use the default Xtra Media icon
			directorMedia.assetInfoOptional = pObj->assetsInfoPointer->find("");
		} else {
			ThrowErr(err);

			assetXtra = true;
		}

		if (!directorMedia.assetInfoOptional.has_value()) {
			// failed to get any Asset Info
			Throw(kMoaErr_InternalError);
		}
	}

	// scope because assetInfoOptional's value may change after this
	{
		const Asset::Info &ASSET_INFO = directorMedia.assetInfoOptional.value();

		if (std::holds_alternative<MoaMmSymbol>(ASSET_INFO.subType)) {
			// for behaviour/movie/parent scripts, different icons for same type
			ThrowErr(drCastMemInterfacePointer->GetProp(std::get<MoaMmSymbol>(ASSET_INFO.subType), &subTypeValue));

			MoaMmSymbol subTypeSymbol = 0;
			ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&subTypeValue, &subTypeSymbol));

			foundIconValues = ASSET_INFO.iconValuesMap.find(subTypeSymbol);

			if (foundIconValues == ASSET_INFO.iconValuesMap.end()) {
				// default icon if not one of the known subtypes
				foundIconValues = ASSET_INFO.iconValuesMap.find("");
			}
		} else {
			// no subtypes for this type
			foundIconValues = ASSET_INFO.iconValuesMap.find("");
		}
	}

	// we use cend here because we aren't getting this off the const ASSET_INFO variable
	if (foundIconValues == directorMedia.assetInfoOptional.value().iconValuesMap.cend()) {
		if (!assetXtra) {
			Throw(kMoaErr_InternalError);
		}

		// the Asset Xtra doesn't have an icon: use the default Xtra Media icon
		directorMedia.assetInfoOptional = pObj->assetsInfoPointer->find("");

		if (!directorMedia.assetInfoOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		const Asset::Info &ASSET_INFO = directorMedia.assetInfoOptional.value();

		foundIconValues = ASSET_INFO.iconValuesMap.find("");

		if (foundIconValues == ASSET_INFO.iconValuesMap.end()) {
			// failed to get any icons
			Throw(kMoaErr_InternalError);
		}
	}

	if (std::holds_alternative<IconValues::POINTER>(foundIconValues->second)) {
		// intentional copy of icon values for Lingo so scripts can't modify a direct reference to our cached icons occurs HERE
		// (so if you call getExportFileIconPropList once, modify the image, and call it again, you won't get the modified image)
		IconValues iconValues = *std::get<IconValues::POINTER>(foundIconValues->second); // removing this line causes the same image objects to be returned every time
		ThrowErr(pObj->exportFileValueConverterPointer->toValue(iconValues.toIconValueMap(), callPtr->resultValue, drCastMemInterfacePointer));
	} else {
		// Mix Services missing
		Throw(kMoaErr_BadClass);
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);

	releaseValue(typeValue, pObj->mmValueInterfacePointer);
	releaseValue(subTypeValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::CallTell(PMoaDrCallInfo callPtr) {
	moa_try

	ThrowNull(callPtr);

	MoaMmSymbol propertyNameSymbol = 0;
	ThrowErr(GetArgPropertyName(callPtr, &propertyNameSymbol));

	if (propertyNameSymbol == pObj->symbols.MixerSaved) {
		Throw(TellExportFileMixerSaved(callPtr));
	}

	Throw(kMoaDrErr_HandlerNotDefined);

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::CallGet(PMoaDrCallInfo callPtr) {
	moa_try

	ThrowNull(callPtr);

	MoaMmSymbol propertyNameSymbol = 0;
	ThrowErr(GetArgPropertyName(callPtr, &propertyNameSymbol));

	if (propertyNameSymbol == pObj->symbols.LabelList) {
		Throw(GetExportFileLabelList(callPtr));
	}
	
	if (propertyNameSymbol == pObj->symbols.AgentPropList) {
		Throw(GetExportFileAgentPropList(callPtr));
	}
	
	if (propertyNameSymbol == pObj->symbols.DefaultPath) {
		Throw(GetExportFileDefaultPath(callPtr));
	}
	
	if (propertyNameSymbol == pObj->symbols.DefaultLabel) {
		Throw(GetExportFileDefaultLabel(callPtr));
	}
	
	if (propertyNameSymbol == pObj->symbols.DefaultAgent) {
		Throw(GetExportFileDefaultAgent(callPtr));
	}
	
	if (propertyNameSymbol == pObj->symbols.DefaultOptions) {
		Throw(GetExportFileDefaultOptions(callPtr, false));
	}

	if (propertyNameSymbol == pObj->symbols.DisplayName) {
		Throw(GetExportFileDisplayName(callPtr));
	}
	
	if (propertyNameSymbol == pObj->symbols.TypePropList) {
		Throw(GetExportFileTypePropList(callPtr));
	}
	
	if (propertyNameSymbol == pObj->symbols.IconPropList) {
		Throw(GetExportFileIconPropList(callPtr));
	}

	Throw(kMoaDrErr_HandlerNotDefined);

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::LoadMember(PIMoaDrCastMem drCastMemInterfacePointer) {
	MoaMmValue loadedValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(drCastMemInterfacePointer);

	// I have verified that, unlike preloadMember, idle loading does not
	// have any impact on this, so we don't need to temporarily turn it off
	ThrowErr(pObj->mmValueInterfacePointer->IntegerToValue(TRUE, &loadedValue));
	ThrowErr(drCastMemInterfacePointer->SetProp(pObj->symbols.Loaded, &loadedValue));

	// ensure it actually loaded
	MoaLong loaded = FALSE;
	ThrowErr(GetLoaded(&loaded, drCastMemInterfacePointer));

	if (!loaded) {
		Throw(kMoaDrErr_MediaNotReady);
	}

	moa_catch
	moa_catch_end

	releaseValue(loadedValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::FindLabelInfo(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (directorMediaPointer->labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
		if (!std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
			Throw(kMoaErr_InternalError);
		}

		const Label::Info::MAP &LABEL_INFO_MAP = pObj->labelsInfoPointer->get();

		directorMediaPointer->labelInfoMapIterator = LABEL_INFO_MAP.find(std::get<MoaMmSymbol>(argsPointer->labelSymbolVariant));

		if (directorMediaPointer->labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
			Throw(kMoaDrErr_LabelNotFound);
		}

		if (!directorMediaPointer->labelInfoMapIterator->second.labelType) {
			Throw(kMoaDrErr_MediaLabelNotSupported);
		}
	}

	moa_catch

	if (directorMediaPointer) {
		directorMediaPointer->labelInfoMapIterator = LABEL_INFO_NOT_FOUND;
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::FindAgentInfo(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	Agent::Info::MAP::const_iterator foundAgentMapInfo = {};

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->agentInfoOptional.has_value()) {
		ThrowErr(GetLabelAgentInfoMap(argsPointer, directorMediaPointer));

		if (!directorMediaPointer->agentInfoMapOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		const Agent::Info::MAP &AGENT_INFO_MAP = directorMediaPointer->agentInfoMapOptional.value();

		if (!argsPointer->agentStringOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		foundAgentMapInfo = AGENT_INFO_MAP.find(argsPointer->agentStringOptional.value());

		if (foundAgentMapInfo == AGENT_INFO_MAP.end()) {
			Throw(kMoaMixErr_NoSuchAgent);
		}

		// unlike the labelInfoIterator, which we keep on directorMedia as an iterator
		// this is moved (if possible, but realistically, copied) to an optional instead
		// this is because the Agent Info Map is found in this method, as opposed to
		// stored permanently on the Xtra, and I don't want to have to deal with
		// the nasty iterator invalidation issues that's going to cause, so
		// this is worth moving, I think
		directorMediaPointer->agentInfoOptional = foundAgentMapInfo->second;
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::FindXtraAssetInfo(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	MoaMmValue typeValue = kVoidMoaMmValueInitializer;
	Asset::Info::MAP::iterator foundAssetInfo = {};

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->assetInfoOptional.has_value()) {
		// calling this twice is worth it
		// getting the assetInfoMap is by far slower (~2 ms vs. ~10 ms)
		// and I don't expect this hash to change often
		size_t assetMoaIDsHash = 0;

		ThrowFailed(
			pObj->cacheInterfacePointer->EnumerateRegistryEntries(
				MoaCacheRegistryEntryEnumProc_AssetMoaIDsHash_TStdXtra,
				(PMoaVoid)&assetMoaIDsHash
			)
		);

		Asset::Info::MAP &assetInfoMap = *pObj->assetInfoMapPointer;

		if (assetMoaIDsHash != pObj->assetMoaIDsHash) {
			Registry::AssetInfoMap registryEntriesAssetInfoMap = {};

			registryEntriesAssetInfoMap.mmValueInterfacePointer = pObj->mmValueInterfacePointer;
			registryEntriesAssetInfoMap.callocInterfacePointer = pObj->pCalloc;

			registryEntriesAssetInfoMap.mmImageInterfacePointer = pObj->mmImageInterfacePointer;

			// creating BitmapImporter is expensive, so for the sake of performance
			// we create it here outside the loop, if applicable
			// (it is only required for getting assetInfoMap)
			// this is a unique_ptr so it'll last the entire scope of this function
			// but we only create it here
			// also this is specifically a pointer and not an optional so it doesn't
			// get unnecessarily copy constructed
			if (!directorMediaPointer->bitmapImporterPointer) {
				directorMediaPointer->bitmapImporterPointer = std::make_shared<BitmapImporter>(
					*pObj->labelsInfoPointer,
					pObj->drPlayerInterfacePointer,
					pObj->mmValueInterfacePointer,
					pObj->drUtilsInterfacePointer,
					pObj->mmImageInterfacePointer
				);

				ThrowNull(directorMediaPointer->bitmapImporterPointer);
			}

			registryEntriesAssetInfoMap.bitmapImporterPointer = directorMediaPointer->bitmapImporterPointer.get();
			registryEntriesAssetInfoMap.assetInfoMapPointer = &assetInfoMap;

			ThrowFailed(
				pObj->cacheInterfacePointer->EnumerateRegistryEntries(
					MoaCacheRegistryEntryEnumProc_AssetInfoMap_TStdXtra,
					(PMoaVoid)&registryEntriesAssetInfoMap
				)
			);

			pObj->assetMoaIDsHash = assetMoaIDsHash;
		}

		drCastMemInterfacePointer = argsPointer->getDrCastMemInterfacePointer();

		if (!drCastMemInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.Type, &typeValue));

		MoaMmSymbol typeSymbol = 0;
		ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&typeValue, &typeSymbol));

		foundAssetInfo = assetInfoMap.find(typeSymbol);

		if (foundAssetInfo == assetInfoMap.end()) {
			Throw(kMoaMmErr_XAssetTypeUnknown);
		}

		directorMediaPointer->assetInfoOptional = foundAssetInfo->second;
	}

	moa_catch

	if (directorMediaPointer) {
		directorMediaPointer->assetInfoOptional = std::nullopt;
	}

	moa_catch_end

	if (directorMediaPointer) {
		directorMediaPointer->bitmapImporterPointer = 0;
	}

	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);

	releaseValue(typeValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::CreateMixerThread(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	bool createdThread = false;
	Media::MixerMedia* mixerMediaPointer = 0;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	// can't use any fancy dancy RAII here, this is to be passed to another thread
	mixerMediaPointer = new Media::MixerMedia(
		*pObj->exportFileValueConverterPointer,
		pObj->drPlayerInterfacePointer,
		pObj->mmValueInterfacePointer
	);

	ThrowNull(mixerMediaPointer);

	drCastMemInterfacePointer = argsPointer->getDrCastMemInterfacePointer();

	if (!drCastMemInterfacePointer) {
		Throw(kMoaErr_InternalError);
	}

	mixerMediaPointer->setDrCastMemInterfacePointer(drCastMemInterfacePointer);

	{
		// do NOT release this interface
		PIMoaRegistryEntryDict registryEntryDictInterfacePointer = NULL;
		ThrowErr(GetRegistryEntryDict(&registryEntryDictInterfacePointer));

		if (!registryEntryDictInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		mixerMediaPointer->setRegistryEntryDictInterfacePointer(registryEntryDictInterfacePointer);
	}

	ThrowErr(CreateContentFormat(argsPointer, directorMediaPointer));

	if (!directorMediaPointer->contentPointer) {
		Throw(kMoaErr_InternalError);
}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	if (!content.formatPointer) {
		Throw(kMoaErr_InternalError);
	}

	mixerMediaPointer->formatPointer = content.formatPointer;

	#ifdef MACINTOSH
	// create a thread here
	#endif
	#ifdef WINDOWS
	// this is a Windows trick to keep the DLL loaded for at least the duration this thread is running
	// no idea the way to do this on Mac
	// http://devblogs.microsoft.com/oldnewthing/20131105-00/?p=2733
	ThrowErr(
		osErr(
			GetModuleHandleEx(
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
				(LPCSTR)&Mixer::thread,
				&mixerMediaPointer->moduleHandle
			)
		)
	);

	ThrowErr(osErr(mixerMediaPointer->moduleHandle));

	// attempt to create the thread
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, Mixer::thread, mixerMediaPointer, 0, NULL);
	createdThread = thread && thread != INVALID_HANDLE_VALUE;

	// ensure we close the thread, erroring if we failed to create it
	MoaError err = osErr(createdThread, true);

	// if closing the thread fails, delay the error until the Xtra is destroyed
	// this way we at least get the file out and the thread runs, but
	// we still free resources at a later time
	MoaError err2 = osErr(closeThread(thread));

	if (err2 != kMoaErr_NoErr) {
		pObj->threadVectorPointer->push_back(thread);
	}

	// same deal for freeing the library
	if (!createdThread) {
		err2 = osErr(freeLibrary(mixerMediaPointer->moduleHandle));

		if (err2 != kMoaErr_NoErr) {
			pObj->moduleHandleVectorPointer->push_back(mixerMediaPointer->moduleHandle);
		}
	}

	ThrowErr(err);
	#endif

	moa_catch

	// clean up MixerMedia if the thread can't do it for us
	if (!createdThread && mixerMediaPointer) {
		delete mixerMediaPointer;
	}

	moa_catch_end

	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::CreateContentFormat(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;
	PIMoaMmXAsset mmXAssetInterfacePointer = NULL;

	MoaDrMediaInfo mediaInfo = {};
	MoaMmValue sampleSizeValue = kVoidMoaMmValueInitializer;
	MoaMmValue typeValue = kVoidMoaMmValueInitializer;
	MoaMmValue memberPropertyValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->contentPointer) {
		directorMediaPointer->contentPointer = std::make_shared<Media::DirectorMedia::Content>();
		ThrowNull(directorMediaPointer->contentPointer);
	}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	// attempt to use the Format Factory
	// (no, not the knockoff Chinese bootleg video converter of the same name, the ExportFile Format Factory)
	if (!content.formatPointer) {
		ThrowErr(FindLabelInfo(argsPointer, directorMediaPointer));

		if (directorMediaPointer->labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
			Throw(kMoaErr_InternalError);
		}

		const Label::Info &LABEL_INFO = directorMediaPointer->labelInfoMapIterator->second;

		drCastMemInterfacePointer = argsPointer->getDrCastMemInterfacePointer();

		if (!drCastMemInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		if (LABEL_INFO.labelType & Label::TYPE_XTRA_MEDIA) {
			// must be loaded for StreamOutMedia to work
			ThrowErr(LoadMember(drCastMemInterfacePointer));

			ThrowErr(pObj->exportFileValueConverterPointer->toAsset(drCastMemInterfacePointer, mmXAssetInterfacePointer));
			ThrowNull(mmXAssetInterfacePointer);

			if (LABEL_INFO.labelType & Label::TYPE_XTRA_MEDIA_MIXER_ASYNC) {
				content.formatPointer = Formats::FormatFactory::createXtraMediaFormat(
					LABEL_INFO.labelType,
					mmXAssetInterfacePointer,
					pObj->productVersionMajor,
					drCastMemInterfacePointer,
					pObj->mmValueInterfacePointer,
					pObj->pCallback,
					pObj->pCalloc
				);
			} else {
				content.formatPointer = Formats::FormatFactory::createXtraMediaFormat(
					LABEL_INFO.labelType,
					mmXAssetInterfacePointer,
					pObj->productVersionMajor,
					pObj->pCallback,
					pObj->pCalloc
				);
			}
		} else {
			if (!std::holds_alternative<MoaMmSymbol>(LABEL_INFO.labelFormat)) {
				Throw(kMoaDrErr_MediaFormatNotSupported);
			}

			// label symbols should directly map to these
			const MoaMmSymbol &FORMAT_SYMBOL = std::get<MoaMmSymbol>(LABEL_INFO.labelFormat);

			MoaError err = kMoaErr_NoErr;

			if (LABEL_INFO.labelType & Label::TYPE_MEDIA_INFO) {
				if (!std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
					Throw(kMoaErr_InternalError);
				}

				const MoaMmSymbol &LABEL_SYMBOL = std::get<MoaMmSymbol>(argsPointer->labelSymbolVariant);

				if (LABEL_SYMBOL == pObj->symbols.Sound) {
					// test if its bit depth is 0 bits
					// this prevents a crash in IML when converting sounds created with _movie.newMember(#sound)
					// and replicates the Director 6.5 (before the crash was introduced) behaviour
					ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.SampleSize, &sampleSizeValue));

					MoaLong sampleSize = 0;
					ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&sampleSizeValue, &sampleSize));

					if (!sampleSize) {
						Throw(kMoaDrErr_MediaDataNonexistant);
					}
				}

				// note: we only need to call releaseMedia in the event of an error because
				// the format takes ownership (and knows the native call to release this data)
				ThrowErr(
					pObj->drUtilsInterfacePointer->NewMediaInfo(
						LABEL_SYMBOL,
						FORMAT_SYMBOL,
						NULL,
						kMoaDrMediaOpts_None,
						NULL,
						&mediaInfo
					)
				);

				bool gotMedia = false;

				if (LABEL_SYMBOL == pObj->symbols.Image) {
					// no point importing a bitmap to another bitmap
					ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.Type, &typeValue));

					MoaMmSymbol typeSymbol = 0;
					ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&typeValue, &typeSymbol));

					if (typeSymbol != pObj->symbols.Bitmap) {
						// must be loaded otherwise the image may turn out blank
						ThrowErr(LoadMember(drCastMemInterfacePointer));

						// this is for members that have an #image property that cannot be obtained via GetMedia
						// (such as Shockwave 3D)
						// Text members also end up here
						// (old style Director 5 Rich Text works fine with GetMedia, but not new Text since it's technically Xtra Media)
						{
							BitmapImporter bitmapImporter(
								*pObj->labelsInfoPointer,
								pObj->drPlayerInterfacePointer,
								pObj->mmValueInterfacePointer,
								pObj->drUtilsInterfacePointer,
								pObj->mmImageInterfacePointer
							);

							err = bitmapImporter.getMedia(mediaInfo, drCastMemInterfacePointer);

							gotMedia = true;
						}
					}
				}
				
				// this call MUST NOT be moved before creation of the Bitmap Importer
				// (GetMedia causes a Director bug that breaks the Picture property for this member temporarily)
				if (!gotMedia) {
					err = drCastMemInterfacePointer->GetMedia(&mediaInfo);
				}

				// if an error occured, toss out the media data before continuing
				// (don't move this past moa_catch! It throws its own error on failure!)
				if (err != kMoaErr_NoErr) {
					ThrowErr(releaseMedia(mediaInfo, pObj->drUtilsInterfacePointer));
					Throw(err);
				}

				// this is always NULL for bitmaps created with _movie.newMember(#bitmap)
				// regardless of the label/format chosen
				// so we error here for those
				if (!mediaInfo.mediaData) {
					Throw(kMoaDrErr_MediaDataNonexistant);
				}

				content.formatPointer = Formats::FormatFactory::createMediaInfoFormat(
					LABEL_INFO.labelType,
					mediaInfo.mediaData,
					pObj->productVersionMajor,
					pObj->handleInterfacePointer,
					pObj->pCalloc
				);

				// if no format to own the media? must release it ourselves
				if (!content.formatPointer) {
					ThrowErr(releaseMedia(mediaInfo, pObj->drUtilsInterfacePointer));
				}
			} else if (LABEL_INFO.labelType & Label::TYPE_MEMBER_PROPERTY) {
				// needs to be GetPropA for the benefit of Text Agents, which don't expect wide strings
				// (this is fine, it'll return out UTF-8 in Director 11 where Unicode is supported)
				err = drCastMemInterfacePointer->GetPropA(FORMAT_SYMBOL, &memberPropertyValue);

				if (FORMAT_SYMBOL == pObj->symbols.Picture
					&& err != kMoaErr_NoErr) {
					ThrowErr(LoadMember(drCastMemInterfacePointer));

					{
						BitmapImporter bitmapImporter(
							*pObj->labelsInfoPointer,
							pObj->drPlayerInterfacePointer,
							pObj->mmValueInterfacePointer,
							pObj->drUtilsInterfacePointer,
							pObj->mmImageInterfacePointer
						);

						err = bitmapImporter.getProp(memberPropertyValue, drCastMemInterfacePointer);
					}
				}

				ThrowErr(err);

				if (LABEL_INFO.labelType & Label::TYPE_MEMBER_PROPERTY_PICTURE) {
					if (!pObj->drMediaValueInterfacePointer) {
						// pre-Director 7
						Throw(kMoaErr_BadClass);
					}

					content.formatPointer = Formats::FormatFactory::createMemberPropertyFormat(
						LABEL_INFO.labelType,
						&memberPropertyValue,
						pObj->productVersionMajor,
						pObj->handleInterfacePointer,
						pObj->drMediaValueInterfacePointer,
						pObj->mmValueInterfacePointer,
						pObj->pCalloc
					);
				} else {
					content.formatPointer = Formats::FormatFactory::createMemberPropertyFormat(
						LABEL_INFO.labelType,
						&memberPropertyValue,
						pObj->productVersionMajor,
						pObj->mmValueInterfacePointer,
						pObj->pCalloc
					);
				}
			}
		}

		ThrowNull(content.formatPointer);
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
	releaseInterface((PPMoaVoid)&mmXAssetInterfacePointer);

	releaseValue(sampleSizeValue, pObj->mmValueInterfacePointer);
	releaseValue(typeValue, pObj->mmValueInterfacePointer);
	releaseValue(memberPropertyValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::CreateContentReader(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaReader readerInterfacePointer = NULL;
	PIMoaRegistryEntryDict readerRegistryEntryDictInterfacePointer = NULL;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->contentPointer) {
		directorMediaPointer->contentPointer = std::make_shared<Media::DirectorMedia::Content>();
		ThrowNull(directorMediaPointer->contentPointer);
	}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	readerInterfacePointer = content.getReaderInterfacePointer();

	if (!readerInterfacePointer) {
		MoaError err = GetContentReaderRegistryEntryDict(argsPointer, directorMediaPointer);

		if (err != kMoaErr_NoErr) {
			Throw(kMoaMixErr_NoSuchReader);
		}

		ThrowErr(err);

		readerRegistryEntryDictInterfacePointer = content.getReaderRegistryEntryDictInterfacePointer();

		if (!readerRegistryEntryDictInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		// headers for MIX urge not to use MoaCreateInstance to create Readers directly
		// because it "bypasses internal logic in MIX"
		// but enumerating them with GetReaderInfo does not allow access to hidden readers
		// it's also stated that CreateReader uses CreateNewInstanceFromRegistry under the hood
		// so this method should be allowed
		err = pObj->cacheInterfacePointer->CreateNewInstanceFromRegistry(readerRegistryEntryDictInterfacePointer, &IID_IMoaReader, (PPMoaVoid)&readerInterfacePointer);

		if (err != kMoaErr_NoErr
			|| !readerInterfacePointer) {
			Throw(kMoaMixErr_NoSuchReader);
		}

		content.setReaderInterfacePointer(readerInterfacePointer);
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&readerInterfacePointer);
	releaseInterface((PPMoaVoid)&readerRegistryEntryDictInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::CreateContentDataObject(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaDataObject dataObjectInterfacePointer = NULL;
	PIMoaStream streamInterfacePointer = NULL;

	MoaFormatEtc formatEtc = {};

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->contentPointer) {
		directorMediaPointer->contentPointer = std::make_shared<Media::DirectorMedia::Content>();
		ThrowNull(directorMediaPointer->contentPointer);
	}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	dataObjectInterfacePointer = content.getDataObjectInterfacePointer();

	if (!dataObjectInterfacePointer) {
		ThrowErr(FindLabelInfo(argsPointer, directorMediaPointer));

		if (directorMediaPointer->labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
			Throw(kMoaErr_InternalError);
		}

		const Label::Info &LABEL_INFO = directorMediaPointer->labelInfoMapIterator->second;

		if (LABEL_INFO.agentFormatName.empty()) {
			Throw(kMoaMixErr_BadDataSourceType);
		}

		if (!pObj->dataObjectServicesInterfacePointer) {
			ThrowErr(GetIMoaDataObjectServices(pObj->pCallback, &pObj->dataObjectServicesInterfacePointer));
			ThrowNull(pObj->dataObjectServicesInterfacePointer);
		}

		// data object
		ThrowErr(pObj->dataObjectServicesInterfacePointer->CreateEmptyDataObject(&dataObjectInterfacePointer));
		ThrowNull(dataObjectInterfacePointer);

		if (!pObj->formatServicesInterfacePointer) {
			ThrowErr(GetIMoaFormatServices(pObj->pCallback, &pObj->formatServicesInterfacePointer));
			ThrowNull(pObj->formatServicesInterfacePointer);
		}

		// format etc.
		MixFormat mixFormat = 0;
		ThrowErr(pObj->formatServicesInterfacePointer->RegisterFormat(LABEL_INFO.agentFormatName.c_str(), &mixFormat));

		SetMoaFormatEtc(&formatEtc, mixFormat, kMoaStorageMediumType_HGlobal);

		ThrowErr(CreateContentFormat(argsPointer, directorMediaPointer));
		
		if (!content.formatPointer) {
			Throw(kMoaErr_InternalError);
		}

		// stream
		// (this lives on directorMedia because the reader interface needs it kept alive)
		content.streamPointer = std::make_unique<Stream>(pObj->pCallback);
		ThrowNull(content.streamPointer);

		Stream &stream = *content.streamPointer;
		ThrowErr(stream.open());

		streamInterfacePointer = stream.getStreamInterfacePointer();
		
		if (!streamInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		MoaUlong size = 0;
		ThrowErr(content.formatPointer->get(size, streamInterfacePointer));

		//if (!size) {
			//Throw(kMoaDrErr_MediaDataInvalid);
		//}

		// this copies the data so the data object owns it
		// so the format (and it's associated global handle)
		// don't need to be kept alive after this
		// THIS MUST BE PASSED AS AN OPEN STREAM!
		// AddPtrToDataObject is evil: the result stream from DataObjectToMoaStream starts closed instead of open
		// which the built in Readers/Writers don't expect
		// it also errors on empty data
		// we use AddMoaStreamToDataObject (which also AddRef's the stream) instead
		ThrowErr(stream.resetPosition());
		ThrowErr(pObj->dataObjectServicesInterfacePointer->AddMoaStreamToDataObject(dataObjectInterfacePointer, &formatEtc, streamInterfacePointer));

		content.setDataObjectInterfacePointer(dataObjectInterfacePointer);
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&dataObjectInterfacePointer);
	releaseInterface((PPMoaVoid)&streamInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::EnumWriter(Agent::Info::MAP* agentInfoMapPointer, Agent::HIDDEN_READER_SET* agentHiddenReaderSetPointer, PIMoaEnumMixAgentInfo enumMixWriterInfoInterfacePointer) {
	PIMoaMixAgentInfo mixWriterInfoInterfacePointer = NULL;
	PIMoaMixFormatInfo mixFormatInfoInterfacePointer = NULL;
	PIMoaWriter writerInterfacePointer = NULL;

	std::string formatName = "";
	Agent::Info::MAP::iterator foundAgentInfo = {};
	Agent::Info agentInfo;
	Agent::Info::Writer writer;

	moa_try

	ThrowNull(agentInfoMapPointer);
	ThrowNull(agentHiddenReaderSetPointer);
	ThrowNull(enumMixWriterInfoInterfacePointer);

	ThrowErr(enumMixWriterInfoInterfacePointer->Next(1, &mixWriterInfoInterfacePointer, NULL));
	ThrowNull(mixWriterInfoInterfacePointer);

	// format
	MixFormat mixFormat = 0;
	ThrowErr(mixWriterInfoInterfacePointer->GetAgentFormatInfo(&mixFormat, &mixFormatInfoInterfacePointer));
	ThrowNull(mixFormatInfoInterfacePointer);

	ThrowErr(GetFormatName(mixFormat, &formatName));

	// ignore any Xtra that's breaking the rules with an empty format name
	if (!formatName.empty()) {
		// create the writer for later use, but don't set its reader yet
		// for projectors: ignore the writer if it is missing
		MoaError err = mixWriterInfoInterfacePointer->CreateWriter(NULL, &writerInterfacePointer);

		if (err == kMoaErr_NoErr
		&& writerInterfacePointer) {
			// test if the agent for this format already exists in the agent info map
			Agent::Info::MAP &agentInfoMap = *agentInfoMapPointer;

			foundAgentInfo = agentInfoMap.find(formatName);

			// idea here is that first agent enumerated should be preferred
			// unless it is hidden, and another with same format isn't
			// then prefer the first visible one
			bool createdNew = foundAgentInfo == agentInfoMap.end();

			if (!createdNew) {
				agentInfo = foundAgentInfo->second;
			}

			ThrowErr(mixWriterInfoInterfacePointer->GetAgentRegInfo(NULL, &writer.classID));

			// hidden
			if (agentInfo.hidden || createdNew) {
				// if the associated reader is hidden
				// then don't show the writer either
				ThrowErr(GetAgentInfoHidden(agentHiddenReaderSetPointer, &writer.classID, &agentInfo.hidden, mixWriterInfoInterfacePointer));

				if (!agentInfo.hidden || createdNew) {
					// name
					ThrowErr(GetAgentInfoName(&agentInfo.name, mixWriterInfoInterfacePointer));
				}
			}

			// extensions
			ThrowErr(AddAgentInfoExtensions(agentInfo.name.c_str(), &agentInfo.pathExtensions, mixFormatInfoInterfacePointer));

			// do these things last so we don't leak the writer interface pointer
			// or save invalid agent info into the map
			// in the event of another error occuring somewhere above
			writer.setWriterInterfacePointer(writerInterfacePointer);
			agentInfo.writerVector.push_back(writer);

			agentInfoMap[formatName] = agentInfo;
		}
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&mixWriterInfoInterfacePointer);
	releaseInterface((PPMoaVoid)&mixFormatInfoInterfacePointer);
	releaseInterface((PPMoaVoid)&writerInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::HandleCreateFileError(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, MoaError err, PIMoaFile fileInterfacePointer) {
	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);
	ThrowNull(fileInterfacePointer);

	// the idea here is to handle any error that occurs after any other error
	// for example, handling the default case, we might switch from using the
	// member name to number, only to discover there's already a file with
	// that name (so now there's a duplicate spec error)
	// then, mid-handling the duplicate spec error, there's a tiny chance that
	// the folder the file is to be exported to gets deleted
	// (so now there's a file not found error)
	// we only do this to a maximum of twenty retries, in case there's some scenario
	// where two errors alternate back and forth, to not get stuck in an infinite loop
	const int DEFAULT_RETRIES = 10;
	const int MAX_RETRIES = DEFAULT_RETRIES + DEFAULT_RETRIES;

	MoaError err2 = kMoaErr_NoErr;

	bool defaultError = false;

	for (int i = 0; i < MAX_RETRIES; i++) {
		// outside of the switch so that this break means something
		if (err == kMoaErr_NoErr) {
			break;
		}

		if (i == DEFAULT_RETRIES) {
			if (defaultError) {
				// if we've already tried the default fix, give up
				Throw(kMoaFileErr_IoError);
			} else {
				// ensure at the halfway point that this runs at least once
				// this is in case a member has a name that causes alternating errors
				// so we never try substituting the number instead (which we do now)
				// this could restart the cycle of duplicate spec/not found errors, so that's
				// why we don't do this at the end
				err2 = HandleDefaultCreateFileError(argsPointer, directorMediaPointer, err, fileInterfacePointer);

				// if it didn't fix it, give up now
				if (err == err2) {
					Throw(kMoaFileErr_IoError);
				}

				defaultError = true;
			}
		} else {
			switch (err) {
				case kMoaFileErr_IoError:
				// "nothing else left I can do" error
				Throw(kMoaFileErr_IoError);
				break;
				case kMoaErr_FileNotFound:
				err2 = HandleFileNotFound(argsPointer, directorMediaPointer, fileInterfacePointer);

				defaultError = false;
				break;
				case kMoaFileErr_DuplicateSpec:
				err2 = HandleDuplicateSpec(argsPointer, directorMediaPointer, fileInterfacePointer);

				defaultError = false;
				break;
				default:
				// don't run this error handler twice in a row, even if it returned a different error to the previous one
				err2 = defaultError
					? err
					: HandleDefaultCreateFileError(argsPointer, directorMediaPointer, err, fileInterfacePointer);

				defaultError = true;
			}

			// didn't fix it, no point trying the same thing again
			if (err == err2) {
				Throw(err);
			}
		}

		err = err2;
	}

	// hit the max retries
	if (err != kMoaErr_NoErr) {
		Throw(kMoaFileErr_IoError);
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::HandleFileNotFound(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, PIMoaFile fileInterfacePointer) {
	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);
	ThrowNull(fileInterfacePointer);

	if (!argsPointer->optionsOptional.has_value()) {
		Throw(kMoaErr_InternalError);
	}

	if (argsPointer->optionsOptional.value().newFolder) {
		if (!argsPointer->pathInfoOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		ThrowErr(argsPointer->pathInfoOptional.value().newFolder());
	}

	// HandleDefaultCreateFileError is called here to revert the temporary file
	// if one was made earlier by HandleDuplicateSpec
	// (we pass kMoaErr_NoErr here to just get the path info as is)
	// we always call this even if #newFolder is off for if incrementFilename throws
	Throw(HandleDefaultCreateFileError(argsPointer, directorMediaPointer, kMoaErr_NoErr, fileInterfacePointer));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::HandleDuplicateSpec(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, PIMoaFile fileInterfacePointer) {
	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);
	ThrowNull(fileInterfacePointer);

	if (!argsPointer->optionsOptional.has_value()) {
		Throw(kMoaErr_InternalError);
	}

	const Options &OPTIONS = argsPointer->optionsOptional.value();

	MoaError err = kMoaErr_NoErr;

	if (OPTIONS.incrementFilename) {
		// some time passes between incrementing the filename and
		// creating the file, so to prevent a filesystem race issue we
		// retry more than once
		// also on Mac the filename is only incremented by one number per loop
		// even if the file exists already, so this must be at least 1000
		const int MAX_RETRIES = 1000;

		if (!argsPointer->pathInfoOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		Path::Info &pathInfo = argsPointer->pathInfoOptional.value();

		for (int i = 0; i < MAX_RETRIES; i++) {
			err = pathInfo.incrementFilename();

			if (err != kMoaFileErr_BadFileSpec) {
				ThrowErr(err);
			}

			// try creating the new file, and also
			// handle for path being too long after incrementing the filename
			// (we do this here instead of returning out to the switch to get more retries)
			err = HandleDefaultCreateFileError(argsPointer, directorMediaPointer, err, fileInterfacePointer);

			if (err != kMoaFileErr_DuplicateSpec) {
				break;
			}
		}

		if (err == kMoaFileErr_DuplicateSpec) {
			// hit the max retries (this is highly unusual so deserves a different error code)
			Throw(kMoaFileErr_IoError);
		}

		Throw(err);
	}

	if (OPTIONS.replaceExistingFile) {
		ThrowErr(CreateContentFormat(argsPointer, directorMediaPointer));

		if (!directorMediaPointer->contentPointer) {
			Throw(kMoaErr_InternalError);
		}

		Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

		if (!content.formatPointer) {
			Throw(kMoaErr_InternalError);
		}

		Throw(content.formatPointer->replaceExistingFile(fileInterfacePointer));
	}

	Throw(kMoaFileErr_DuplicateSpec);

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::HandleDefaultCreateFileError(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, MoaError err, PIMoaFile fileInterfacePointer) {
	std::string path = "";

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);
	ThrowNull(fileInterfacePointer);

	// file couldn't be created for some other reason
	// if we're using the member's name as the basename
	// try using it's number instead
	// this is necessary for when the member's name is a reserved filename e.g. "con"
	ThrowErr(GetArgPathDefaultBasename(argsPointer, err));

	ThrowErr(CreateContentFormat(argsPointer, directorMediaPointer));

	if (!directorMediaPointer->contentPointer) {
		Throw(kMoaErr_InternalError);
	}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	if (!content.formatPointer) {
		Throw(kMoaErr_InternalError);
	}

	if (!argsPointer->pathInfoOptional.has_value()) {
		Throw(kMoaErr_InternalError);
	}

	if (!argsPointer->pathInfoOptional.value().getPath(path, content.formatPointer->PATH_RELATIVE)) {
		Throw(kMoaErr_BadParam);
	}

	if (path.empty()) {
		Throw(kMoaErr_InternalError);
	}

	// this is potentially now a duplicate spec
	// in which case, we perform the outer for loop again
	ThrowErr(fileInterfacePointer->SetPathnameSpec(path.c_str(), FALSE));
	Throw(fileInterfacePointer->CreateFile());

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::HandleWriteFileError(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, MoaError err, PIMoaFile fileInterfacePointer) {
	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);
	ThrowNull(fileInterfacePointer);

	ThrowErr(CreateContentFormat(argsPointer, directorMediaPointer));

	if (!directorMediaPointer->contentPointer) {
		Throw(kMoaErr_InternalError);
	}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	if (!content.formatPointer) {
		Throw(kMoaErr_InternalError);
	}

	Formats::Format &format = *content.formatPointer;

	if (err != kMoaErr_NoErr) {
		// failure: cancel the file if we failed to write it
		ThrowErr(format.cancelFile());
		Throw(err);
	}

	// success: swap the file
	// after this it becomes our responsibility to delete fileInterfacePointer if necessary
	err = format.swapFile(true);

	// if false, file was not swapped, create the mixer thread instead
	if (err == kMoaStatus_False) {
		err = CreateMixerThread(argsPointer, directorMediaPointer);

		if (err != kMoaErr_NoErr) {
			// failure: cancel the file if we failed to create the mixer thread
			ThrowErr(format.cancelFile());
		}
	}

	Throw(err);

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::WriteFile(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, PIMoaFile fileInterfacePointer) {
	PIMoaReader readerInterfacePointer = NULL;
	PIMoaDataObject dataObjectInterfacePointer = NULL;
	
	Agent::Info::WRITER_VECTOR::const_iterator writerVectorIterator = {};
	MoaMmValue agentOptionsValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);
	ThrowNull(fileInterfacePointer);

	ThrowErr(CreateContentFormat(argsPointer, directorMediaPointer));

	if (!directorMediaPointer->contentPointer) {
		Throw(kMoaErr_InternalError);
	}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	if (!content.formatPointer) {
		Throw(kMoaErr_InternalError);
	}

	// we are going to use the agent here, so lock in the value
	if (!argsPointer->agentStringOptional.has_value()) {
		argsPointer->agentStringOptional = "";
	}

	bool agent = !argsPointer->agentStringOptional.value().empty();

	// this needs to know we're going to write a file
	// but the agent will write the file itself if it's to be used
	ThrowErr(content.formatPointer->writeFile(agent, fileInterfacePointer));

	if (agent) {
		ThrowErr(FindAgentInfo(argsPointer, directorMediaPointer));

		if (!directorMediaPointer->agentInfoOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		const Agent::Info::WRITER_VECTOR &WRITER_VECTOR = directorMediaPointer->agentInfoOptional.value().writerVector;

		if (!argsPointer->optionsOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		Options &options = argsPointer->optionsOptional.value();
		options.getAgentOptionsValue(agentOptionsValue);

		ThrowErr(CreateContentReader(argsPointer, directorMediaPointer));

		if (!directorMediaPointer->contentPointer) {
			Throw(kMoaErr_InternalError);
		}

		Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

		readerInterfacePointer = content.getReaderInterfacePointer();
	
		if (!readerInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		ThrowErr(CreateContentDataObject(argsPointer, directorMediaPointer));

		dataObjectInterfacePointer = content.getDataObjectInterfacePointer();

		if (!dataObjectInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		ThrowErr(readerInterfacePointer->SetReaderDataSource(dataObjectInterfacePointer));

		MoaError writeFileAgentErr = kMoaErr_NoErr;
		MoaError err = kMoaErr_NoErr;

		MoaLong size = 0;

		for (writerVectorIterator = WRITER_VECTOR.begin(); writerVectorIterator != WRITER_VECTOR.end(); writerVectorIterator++) {
			if (options.writerClassID != IID_NULL
			&& options.writerClassID != writerVectorIterator->classID) {
				continue;
			}
		
			writeFileAgentErr = WriteFileAgent(&*writerVectorIterator, &agentOptionsValue, fileInterfacePointer, readerInterfacePointer);

			// this is done so that if we never export a non-empty file, then
			// we return any error that occured, instead of silencing them
			err = errOrDefaultErr(writeFileAgentErr, err);

			// some writers don't return an error, but don't write anything
			// only break if no error occured AND the size is non-zero
			// however, a zero size does not necessarily indicate an error
			// (the exported file might simply be empty)
			if (writeFileAgentErr == kMoaErr_NoErr) {
				ThrowErr(fileInterfacePointer->GetSize(&size));

				if (size) {
					err = kMoaErr_NoErr;
					break;
				}
			}
		}

		ThrowErr(err);

		if (writerVectorIterator != WRITER_VECTOR.end()) {
			options.writerClassID = writerVectorIterator->classID;
		}

		/*
		if (!size) {
			Throw(kMoaDrErr_MediaDataInvalid);
		}
		*/
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&readerInterfacePointer);
	releaseInterface((PPMoaVoid)&dataObjectInterfacePointer);

	releaseValue(agentOptionsValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::WriteFileAgent(const Agent::Info::Writer* writerPointer, PMoaMmValue agentOptionsValuePointer, PIMoaFile fileInterfacePointer, PIMoaReader readerInterfacePointer) {
	PIMoaWriter writerInterfacePointer = NULL;
	PIMoaDict dictInterfacePointer = NULL;

	moa_try

	ThrowNull(writerPointer);
	ThrowNull(agentOptionsValuePointer);
	ThrowNull(fileInterfacePointer);
	ThrowNull(readerInterfacePointer);

	writerInterfacePointer = writerPointer->getWriterInterfacePointer();
	
	if (!writerInterfacePointer) {
		Throw(kMoaErr_InternalError);
	}

	ThrowErr(writerInterfacePointer->SetWriterDataSource(readerInterfacePointer));

	MoaError err = writerInterfacePointer->GetDefaultWriterOptions(&dictInterfacePointer);

	if (err == kMoaErr_NoErr
		&& dictInterfacePointer) {
		bool empty = true;
		ThrowErr(pObj->exportFileValueConverterPointer->concatToDictInterfacePointer(*agentOptionsValuePointer, empty, dictInterfacePointer));

		// only set the writer options if they are not the defaults
		// so that this doesn't ever error in the default case and prevent use of the writer altogether
		if (!empty) {
			ThrowErr(writerInterfacePointer->SetWriterOptions(dictInterfacePointer));
		}
	}

	//err = writerInterfacePointer->WriteToMoaStream(streamInterfacePointer);

	// if writing to the stream failed, try writing to the file directly
	//if (err != kMoaErr_NoErr) {
	if (!pObj->agentServicesInterfacePointer) {
		ThrowErr(GetIMoaAgentServices(pObj->pCallback, &pObj->agentServicesInterfacePointer));
		ThrowNull(pObj->agentServicesInterfacePointer);
	}

	// truncate the file in case a previous writer wrote something then errored
	// docs say the writer should overwrite the file entirely
	// but I don't entirely trust them to not append to it instead
	ThrowErr(fileInterfacePointer->SetSize(0));

	MoaChar pathnameSpec[MOA_MAX_PATHNAME] = "";
	ThrowErr(fileInterfacePointer->GetPathnameSpec(pathnameSpec, MOA_MAX_PATHNAME));

	ThrowErr(pObj->agentServicesInterfacePointer->WriteToFileSpec(writerInterfacePointer, pathnameSpec));
	//}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&writerInterfacePointer);
	releaseInterface((PPMoaVoid)&dictInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::AddAgentInfoExtensions(ConstPMoaChar agentInfoNameStringPointer, Path::EXTENSION_MAPPED_VECTOR* agentInfoPathExtensionsPointer, PIMoaMixFormatInfo mixFormatInfoInterfacePointer) {
	PMoaVoid fileExtListPointer = NULL;

	Path::EXTENSION_MAPPED_VECTOR pathExtensions = {};

	moa_try

	ThrowNull(agentInfoNameStringPointer);
	ThrowNull(agentInfoPathExtensionsPointer);
	ThrowNull(mixFormatInfoInterfacePointer);

	const MoaUlong FILTER_PATTERN_SIZE = 7;

	MoaUlong fileExtListSize = stringSize(agentInfoNameStringPointer) + 1 + FILTER_PATTERN_SIZE + 1;

	MoaError err = kMoaErr_NoErr;

	do {
		if (fileExtListPointer) {
			fileExtListSize += fileExtListSize;
		}

		freeMemory(fileExtListPointer, pObj->pCalloc);

		fileExtListPointer = pObj->pCalloc->NRAlloc(fileExtListSize);
		ThrowNull(fileExtListPointer);

		err = mixFormatInfoInterfacePointer->GetFileExtList(fileExtListSize, (PMoaChar)fileExtListPointer);
	} while (err == kMoaMixErr_BufferTooSmall
		/*|| (err == kMoaErr_NoErr
		&& stringTruncated(fileExtList.get(), fileExtListSize))*/);

	ThrowErr(err);

	if (!Path::filterExtensions((PMoaChar)fileExtListPointer, pathExtensions)) {
		Throw(kMoaErr_OutOfMem);
	}

	*agentInfoPathExtensionsPointer += pathExtensions;

	moa_catch
	moa_catch_end

	freeMemory(fileExtListPointer, pObj->pCalloc);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetRegistryEntryDict(PIMoaRegistryEntryDict* registryEntryDictInterfacePointerPointer) {
	moa_try

	ThrowNull(registryEntryDictInterfacePointerPointer);

	PIMoaRegistryEntryDict &registryEntryDictInterfacePointer = *registryEntryDictInterfacePointerPointer;
	registryEntryDictInterfacePointer = NULL;

	Registry::Entry::VARIANT &registryEntryVariant = *pObj->registryEntryVariantPointer;

	if (!std::holds_alternative<PIMoaRegistryEntryDict>(registryEntryVariant)) {
		ThrowFailed(
			pObj->cacheInterfacePointer->EnumerateRegistryEntries(
				MoaCacheRegistryEntryEnumProc_RegistryEntryDict_TStdXtra,
				(PMoaVoid)&registryEntryVariant
			)
		);

		if (!std::holds_alternative<PIMoaRegistryEntryDict>(registryEntryVariant)) {
			Throw(kMoaErr_BadClass);
		}
	}

	{
		// do NOT release this interface (it's owned by the Xtra and will be released by it)
		registryEntryDictInterfacePointer = std::get<PIMoaRegistryEntryDict>(registryEntryVariant);
		ThrowNull(registryEntryDictInterfacePointer);
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgs(PMoaDrCallInfo callPtr, Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	moa_try

	ThrowNull(callPtr);
	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	ThrowErr(GetArgMember(callPtr, argsPointer));
	ThrowErr(GetArgPath(callPtr, argsPointer, directorMediaPointer));
	ThrowErr(GetArgLabel(callPtr, argsPointer, directorMediaPointer));
	ThrowErr(GetArgAgent(callPtr, argsPointer, directorMediaPointer));
	ThrowErr(GetArgOptions(callPtr, argsPointer, directorMediaPointer, 5));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgLong(PMoaDrCallInfo callPtr, std::optional<MoaLong>* argOptionalPointer, MoaLong argIndex) {
	moa_try

	ThrowNull(callPtr);
	ThrowNull(argOptionalPointer);

	std::optional<MoaLong> &argOptional = *argOptionalPointer;

	//if (!argOptional.has_value()) {
	{
		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;

		argIndex += pObj->argsBase;

		bool voidP = callPtr->nargs < argIndex;

		if (!voidP) {
			AccessArgByIndex(argIndex, &argumentValue);

			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(argumentValue, voidP));
		}

		argOptional.emplace(0);

		if (!voidP) {
			ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&argumentValue, &argOptional.value()));
		}
	}

	moa_catch

	if (argOptionalPointer) {
		*argOptionalPointer = std::nullopt;
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgPropertyName(PMoaDrCallInfo callPtr, PMoaMmSymbol propertyNameSymbolPointer) {
	moa_try

	ThrowNull(callPtr);
	ThrowNull(propertyNameSymbolPointer);

	{
		const MoaLong PROPERTY_NAME_ARG_INDEX = 2;

		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;
		AccessArgByIndex(PROPERTY_NAME_ARG_INDEX, &argumentValue);

		ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&argumentValue, propertyNameSymbolPointer));
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgMixerSavedCallHandler(PMoaDrCallInfo callPtr, PMoaLong mixerSavedCallHandlerPointer) {
	moa_try

	ThrowNull(callPtr);
	ThrowNull(mixerSavedCallHandlerPointer);

	{
		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;
		AccessArgByIndex(pObj->argsBase + pObj->ARG_INDEX_MIXER_SAVED_CALL_HANDLER, &argumentValue);

		ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&argumentValue, mixerSavedCallHandlerPointer));
	}

	moa_catch

	if (mixerSavedCallHandlerPointer) {
		*mixerSavedCallHandlerPointer = FALSE;
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgMember(PMoaDrCallInfo callPtr, Args* argsPointer) {
	PIMoaDrMovie drMovieInterfacePointer = NULL;
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	MoaDrCMRef cmRef = {};

	moa_try

	ThrowNull(callPtr);
	ThrowNull(argsPointer);

	/*
	drCastMemInterfacePointer = argsPointer->getDrCastMemInterfacePointer();

	if (!drCastMemInterfacePointer) {
	*/
	{
		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;
		AccessArgByIndex(pObj->argsBase + pObj->ARG_INDEX_MEMBER, &argumentValue);

		ThrowErr(pObj->drValueInterfacePointer->ValueToCMRef(&argumentValue, &cmRef));

		ThrowErr(pObj->drPlayerInterfacePointer->GetActiveMovie(&drMovieInterfacePointer));
		ThrowNull(drMovieInterfacePointer);

		ThrowErr(drMovieInterfacePointer->GetCastMemFromCMRef(&cmRef, &drCastMemInterfacePointer));
		ThrowNull(drCastMemInterfacePointer);

		argsPointer->setDrCastMemInterfacePointer(drCastMemInterfacePointer);
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&drMovieInterfacePointer);
	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgPath(PMoaDrCallInfo callPtr, Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	MoaMmValue dirnameValue = kVoidMoaMmValueInitializer;
	MoaMmValue basenameValue = kVoidMoaMmValueInitializer;
	MoaMmValue extensionValue = kVoidMoaMmValueInitializer;
	MoaMmValue filenameValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(callPtr);
	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	//if (!argsPointer->pathInfoOptional.has_value()) {
	{
		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;

		MoaLong argIndex = pObj->argsBase + pObj->ARG_INDEX_PATH;

		bool voidP = callPtr->nargs < argIndex;
		MoaMmValueType valueType = kMoaMmValueType_Void;

		if (!voidP) {
			AccessArgByIndex(argIndex, &argumentValue);

			ThrowErr(pObj->exportFileValueConverterPointer->getValueType(argumentValue, voidP, valueType));
		}

		if (voidP) {
			// we can't call GetArgPathDefault here because the label and agent haven't been found yet
			// instead, it is deferred to GetArgOptions
			argsPointer->pathInfoOptional.emplace(pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
		} else {
			MoaChar pathnameSpec[MOA_MAX_PATHNAME] = "";

			MoaError err = pObj->mmValueInterfacePointer->ValueToString(&argumentValue, pathnameSpec, MOA_MAX_PATHNAME);

			if (err == kMoaMmErr_StringExpected) {
				// must be a property list specifically, not just a list (getAProp doesn't distinguish)
				if (valueType != kMoaMmValueType_PropList) {
					Throw(kMoaMmErr_ValueTypeMismatch);
				}

				argsPointer->pathInfoOptional.emplace(pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
				Path::Info &pathInfo = argsPointer->pathInfoOptional.value();

				ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.Dirname, dirnameValue));
				ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(dirnameValue, voidP));

				if (!voidP) {
					ThrowErr(pObj->mmValueInterfacePointer->ValueToString(&dirnameValue, pathnameSpec, MOA_MAX_PATHNAME));
					pathInfo.setDirnameOptional(pathnameSpec);
				}

				ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.Basename, basenameValue));
				ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(basenameValue, voidP));

				if (!voidP) {
					ThrowErr(pObj->mmValueInterfacePointer->ValueToString(&basenameValue, pathnameSpec, MOA_MAX_PATHNAME));
					pathInfo.setBasenameOptional(pathnameSpec);
				}

				ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.Extension, extensionValue));
				ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(extensionValue, voidP));

				if (!voidP) {
					ThrowErr(pObj->mmValueInterfacePointer->ValueToString(&extensionValue, pathnameSpec, MOA_MAX_PATHNAME));
					pathInfo.setExtensionOptional(pathnameSpec);
				}

				ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.Filename, filenameValue));
				ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(filenameValue, voidP));

				if (!voidP) {
					ThrowErr(pObj->mmValueInterfacePointer->ValueToString(&filenameValue, pathnameSpec, MOA_MAX_PATHNAME));
					pathInfo.setFilenameOptional(pathnameSpec);
				}
			} else {
				ThrowErr(err);

				try {
					argsPointer->pathInfoOptional.emplace(pathnameSpec, pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
				} catch (Path::Info::Invalid) {
					Throw(kMoaErr_BadParam);
				}
			}
		}
	}

	moa_catch

	if (argsPointer) {
		argsPointer->pathInfoOptional = std::nullopt;
	}

	moa_catch_end

	releaseValue(dirnameValue, pObj->mmValueInterfacePointer);
	releaseValue(basenameValue, pObj->mmValueInterfacePointer);
	releaseValue(extensionValue, pObj->mmValueInterfacePointer);
	releaseValue(filenameValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgPathDefault(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	// the basename must be done last, since we control the length of it
	ThrowErr(GetArgPathDefaultDirname(argsPointer));
	ThrowErr(GetArgPathDefaultExtension(argsPointer, directorMediaPointer));
	ThrowErr(GetArgPathDefaultBasename(argsPointer, kMoaErr_NoErr));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgPathDefaultDirname(Args* argsPointer) {
	PIMoaPathName pathNameInterfacePointer = NULL;
	//PIMoaPathName workingDirectoryPathNameInterfacePointer = NULL;

	//PMoaVoid workingDirectoryPathStringPointer = NULL;

	std::optional<std::string> dirnameOptional = std::nullopt;

	moa_try

	ThrowNull(argsPointer);

	if (!argsPointer->pathInfoOptional.has_value()) {
		argsPointer->pathInfoOptional.emplace(pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
	}

	Path::Info &pathInfo = argsPointer->pathInfoOptional.value();

	// bad param happens here if bad path passed in
	if (!pathInfo.getDirnameOptional(dirnameOptional)) {
		Throw(kMoaErr_BadParam);
	}

	bool hasValue = dirnameOptional.has_value();

	// also turn it into an absolute path if it's relative
	if (hasValue) {
		ThrowErr(pObj->pCallback->MoaCreateInstance(&CLSID_CMoaPath, &IID_IMoaPathName, (PPMoaVoid)&pathNameInterfacePointer));
		ThrowNull(pathNameInterfacePointer);

		ThrowErr(pathNameInterfacePointer->InitFromString(dirnameOptional.value().c_str(), kMoaPathDialect_LOCAL, FALSE, FALSE));
		hasValue = pathNameInterfacePointer->IsAbsolute();
	}

	if (!hasValue) {
		if (!argsPointer->optionsOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		const Options &OPTIONS = argsPointer->optionsOptional.value();

		const MoaMmSymbol &LOCATION_SYMBOL = OPTIONS.locationSymbol;

		if (LOCATION_SYMBOL == pObj->symbols.Current) {
			pathInfo.setDirnameOptional(
				FILESYSTEM_DIRECTOR_STRING(
					(
						std::filesystem::current_path()

						/ FILESYSTEM_DIRECTOR_PATH(
							Path::Info::elementOrEmpty(
								dirnameOptional
							),

							pObj->productVersionMajor
						)
					),

					pObj->productVersionMajor
				)
			);
		} else {
			#ifdef MACINTOSH
			// I've never used a Mac, so
			// I have no idea if there are even equivalents for these
			// on MacOS. Hopefully this can eventually do something sensible?
			if (LOCATION_SYMBOL == pObj->symbols.Documents) {
				Throw(kMoaErr_NotImplemented);
			} else if (LOCATION_SYMBOL == pObj->symbols.Pictures) {
				Throw(kMoaErr_NotImplemented);
			} else if (LOCATION_SYMBOL == pObj->symbols.Music) {
				Throw(kMoaErr_NotImplemented);
			} else if (LOCATION_SYMBOL == pObj->symbols.Videos) {
				Throw(kMoaErr_NotImplemented);
			} else {
				Throw(kMoaErr_BadParam);
			}
			#endif
			#ifdef WINDOWS
			// we don't verify the folder, it'll get created as necessary if the #newFolder option was specified
			// verifying can also cause other problems
			// http://forums.ivanti.com/s/article/Environment-Manager-Engineering-Setting-CreateSpecialPaths?language=en_US
			int csidl = CSIDL_FLAG_DONT_VERIFY;

			if (LOCATION_SYMBOL == pObj->symbols.Documents) {
				csidl |= CSIDL_PERSONAL;
			} else if (LOCATION_SYMBOL == pObj->symbols.Pictures) {
				csidl |= CSIDL_MYPICTURES;
			} else if (LOCATION_SYMBOL == pObj->symbols.Music) {
				csidl |= CSIDL_MYMUSIC;
			} else if (LOCATION_SYMBOL == pObj->symbols.Videos) {
				csidl |= CSIDL_MYVIDEO;
			} else {
				Throw(kMoaErr_BadParam);
			}

			WCHAR pathWide[MAX_PATH] = L"";
			ThrowErr(SHGetFolderPathW(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, pathWide));

			pathInfo.setDirnameOptional(
				FILESYSTEM_DIRECTOR_STRING(
					(
						FILESYSTEM_DIRECTOR_PATH(
							(const char*)CW2A(
								pathWide,

								CP_DIRECTOR(
									pObj->productVersionMajor
								)
							),

							pObj->productVersionMajor
						)

						/ FILESYSTEM_DIRECTOR_PATH(
							Path::Info::elementOrEmpty(
								dirnameOptional
							),

							pObj->productVersionMajor
						)
					),

					pObj->productVersionMajor
				)
			);
			#endif
		}
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&pathNameInterfacePointer);
	//releaseInterface((PPMoaVoid)&workingDirectoryPathNameInterfacePointer);

	//freeMemory(workingDirectoryPathStringPointer, pObj->pCalloc);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgPathDefaultExtension(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	std::optional<std::string> extensionOptional = std::nullopt;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!argsPointer->pathInfoOptional.has_value()) {
		argsPointer->pathInfoOptional.emplace(pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
	}

	Path::Info &pathInfo = argsPointer->pathInfoOptional.value();

	// bad param happens here if bad path passed in
	if (!pathInfo.getExtensionOptional(extensionOptional, false)) {
		Throw(kMoaErr_BadParam);
	}

	if (!extensionOptional.has_value()) {
		if (!argsPointer->optionsOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		const Options &OPTIONS = argsPointer->optionsOptional.value();

		// the agent can change after this, based on the extension, if the agent options are set
		if (argsPointer->agentStringOptional.value_or("").empty()) {
			ThrowErr(FindLabelInfo(argsPointer, directorMediaPointer));

			if (directorMediaPointer->labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
				Throw(kMoaErr_InternalError);
			}

			const Label::Info &LABEL_INFO = directorMediaPointer->labelInfoMapIterator->second;

			// this being empty signals to use Xtra Asset Info instead
			if (LABEL_INFO.pathExtensions.empty()) {
				ThrowErr(FindXtraAssetInfo(argsPointer, directorMediaPointer));

				if (!directorMediaPointer->assetInfoOptional.has_value()) {
					Throw(kMoaErr_InternalError);
				}

				const Asset::Info &ASSET_INFO = directorMediaPointer->assetInfoOptional.value();

				if (ASSET_INFO.pathExtensions.empty()) {
					pathInfo.setExtensionOptional("");
				} else {
					if (OPTIONS.alternatePathExtension >= ASSET_INFO.pathExtensions.size()) {
						Throw(kMoaMmErr_ArgOutOfRange);
					}

					pathInfo.setExtensionOptional(ASSET_INFO.pathExtensions[OPTIONS.alternatePathExtension]);
				}
			} else {
				if (OPTIONS.alternatePathExtension >= LABEL_INFO.pathExtensions.size()) {
					Throw(kMoaMmErr_ArgOutOfRange);
				}

				pathInfo.setExtensionOptional(LABEL_INFO.pathExtensions[OPTIONS.alternatePathExtension]);
			}
		} else {
			ThrowErr(FindAgentInfo(argsPointer, directorMediaPointer));

			if (!directorMediaPointer->agentInfoOptional.has_value()) {
				Throw(kMoaErr_InternalError);
			}

			const Agent::Info &AGENT_INFO = directorMediaPointer->agentInfoOptional.value();

			if (AGENT_INFO.pathExtensions.empty()) {
				pathInfo.setExtensionOptional("");
			} else {
				if (OPTIONS.alternatePathExtension >= AGENT_INFO.pathExtensions.size()) {
					Throw(kMoaMmErr_ArgOutOfRange);
				}

				pathInfo.setExtensionOptional(AGENT_INFO.pathExtensions[OPTIONS.alternatePathExtension]);
			}
		}
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgPathDefaultBasename(Args* argsPointer, MoaError createFileErr) {
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	std::optional<std::string> basenameOptional = std::nullopt;
	MoaMmValue nameValue = kVoidMoaMmValueInitializer;
	std::string basename = "";
	std::optional<std::string> filenameOptional = std::nullopt;
	std::string path = "";

	moa_try

	ThrowNull(argsPointer);

	if (!argsPointer->pathInfoOptional.has_value()) {
		argsPointer->pathInfoOptional.emplace(pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
	}

	Path::Info &pathInfo = argsPointer->pathInfoOptional.value();

	bool valid = createFileErr == kMoaErr_NoErr;

	if (valid) {
		if (!pathInfo.getBasenameOptional(basenameOptional)) {
			Throw(kMoaErr_BadParam);
		}

		if (!basenameOptional.has_value()) {
			drCastMemInterfacePointer = argsPointer->getDrCastMemInterfacePointer();

			if (!drCastMemInterfacePointer) {
				Throw(kMoaErr_InternalError);
			}

			ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.Name, &nameValue));

			// always use the number if the name is empty
			bool nameEmpty = true;
			ThrowErr(pObj->exportFileValueConverterPointer->testStringValueEmpty(nameValue, nameEmpty));

			valid = !nameEmpty;

			if (valid) {
				MoaError err = pObj->exportFileValueConverterPointer->toString(nameValue, basename);

				valid = err == kMoaErr_NoErr;

				if (valid) {
					// reject any basename that results in an invalid filename
					pathInfo.setBasenameOptional(Path::getValidName(basename));

					valid = pathInfo.getFilenameOptional(filenameOptional);
				}
			}

			pathInfo.basenameMemberName = valid;
		}

		// this is written such that it may be called multiple times
		// for if the filename is incremented
		// so we care about this even if basenameOptional already has a value
		if (pathInfo.basenameMemberName) {
			valid = pathInfo.getPath(path);

			if (valid) {
				valid = !path.empty();

				if (valid) {
					// check for platform specific path size limitations here
					#ifdef MACINTOSH
					// does Mac even have a path size limit outside of MOA's?
					#endif
					#ifdef WINDOWS
					// subtle issue: MAX_PATH describes the length of a WinAPI string (ANSI or Unicode)
					// but this path string is (potentially) UTF-8
					// because characters in UTF-8 are variable length, there is a chance its size
					// will be larger than if it were represented as a WinAPI string
					// where characters are (almost) always a fixed size
					// so to properly compare, we need to convert it first
					valid = stringSizeWide(CA2W(path.c_str(), CP_DIRECTOR(pObj->productVersionMajor))) <= MAX_PATH;
					#endif
				}
			}
		}
	} else {
		// we already fell back to using the member index previously
		// (or the name is explicit and we can't change it)
		if (!pathInfo.basenameMemberName) {
			Throw(createFileErr);
		}
	}

	// if the path is too long or invalid, use the member index instead
	if (!valid) {
		if (!drCastMemInterfacePointer) {
			drCastMemInterfacePointer = argsPointer->getDrCastMemInterfacePointer();

			if (!drCastMemInterfacePointer) {
				Throw(kMoaErr_InternalError);
			}
		}

		MoaDrMemberIndex memberIndex = 0;
		ThrowErr(drCastMemInterfacePointer->GetMemberIndex(&memberIndex));

		pathInfo.setBasenameOptional(std::to_string(memberIndex));
		pathInfo.basenameMemberName = false;
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);

	releaseValue(nameValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgInfo(PMoaDrCallInfo callPtr, Args* argsPointer) {
	moa_try

	ThrowNull(callPtr);
	ThrowNull(argsPointer);

	ThrowErr(GetArgLong(callPtr, &argsPointer->infoOptional, pObj->ARG_INDEX_INFO));

	moa_catch

	if (argsPointer) {
		argsPointer->infoOptional = std::nullopt;
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgLabel(PMoaDrCallInfo callPtr, Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	moa_try

	ThrowNull(callPtr);
	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	//if (!std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
	{
		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;

		MoaLong argIndex = pObj->argsBase + pObj->ARG_INDEX_LABEL;

		bool voidP = callPtr->nargs < argIndex;

		if (!voidP) {
			AccessArgByIndex(argIndex, &argumentValue);

			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(argumentValue, voidP));
		}

		if (voidP) {
			ThrowErr(GetArgLabelDefault(argsPointer, directorMediaPointer));
		} else {
			// uniquely to GetArgLabel, this might be called again from GetArgOptions
			// in this case, don't get the specified label again
			if (!std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
				MoaMmSymbol labelSymbol = 0;
				ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&argumentValue, &labelSymbol));
				argsPointer->labelSymbolVariant = labelSymbol;

				// we call this just to vet that the label can actually be used for this member
				ThrowErr(GetLabelMappedVector(argsPointer, directorMediaPointer));

				if (!directorMediaPointer->labelMappedVectorOptional.has_value()) {
					Throw(kMoaErr_InternalError);
				}

				Label::MAPPED_VECTOR &labelMappedVector = directorMediaPointer->labelMappedVectorOptional.value();

				if (!labelMappedVector.find(labelSymbol)) {
					Throw(kMoaDrErr_LabelNotFound);
				}
			}
		}
	}

	moa_catch

	if (argsPointer) {
		argsPointer->labelSymbolVariant = "";
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgLabelDefault(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	std::optional<std::string> extensionOptional = std::nullopt;
	Label::MAPPED_VECTOR::CONST_ITERATOR labelMappedVectorIterator = {};
	Label::SYMBOL_VARIANT labelSymbolVariant = "";
	MoaMmValue agentOptionsValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	// DON'T check if the labelSymbol is a MoaMmSymbol
	// (this may be called more than once in which case it should replace the value)
	// agentInfoMapOptional having a value means no more finding
	if (!directorMediaPointer->agentInfoMapOptional.has_value()) {
		// this should not be defined without a label
		if (argsPointer->agentStringOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		if (!argsPointer->pathInfoOptional.has_value()) {
			argsPointer->pathInfoOptional.emplace(pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
		}

		// bad param happens here if bad path passed in
		if (!argsPointer->pathInfoOptional.value().getExtensionOptional(extensionOptional, false)) {
			Throw(kMoaErr_BadParam);
		}

		ThrowErr(GetLabelMappedVector(argsPointer, directorMediaPointer));

		if (!directorMediaPointer->labelMappedVectorOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		Label::MAPPED_VECTOR &labelMappedVector = directorMediaPointer->labelMappedVectorOptional.value();

		if (!labelMappedVector.front(labelSymbolVariant)) {
			Throw(kMoaDrErr_LabelNotFound);
		}

		// shouldn't happen usually?
		if (!std::holds_alternative<MoaMmSymbol>(labelSymbolVariant)) {
			Throw(kMoaErr_InternalError);
		}

		if (extensionOptional.has_value()) {
			// if the extension is not empty, loop through the labels for this member type
			MoaError err = kMoaErr_NoErr;

			for (labelMappedVectorIterator = labelMappedVector.cbegin(); labelMappedVectorIterator != labelMappedVector.cend(); labelMappedVectorIterator++) {
				argsPointer->labelSymbolVariant = *labelMappedVectorIterator;
				directorMediaPointer->labelInfoMapIterator = LABEL_INFO_NOT_FOUND;

				err = FindLabelInfo(argsPointer, directorMediaPointer);

				if (err != kMoaErr_NoErr) {
					continue;
				}

				// check extension matches label - if it does, use that label
				if (directorMediaPointer->labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
					Throw(kMoaErr_InternalError);
				}

				const Label::Info &LABEL_INFO = directorMediaPointer->labelInfoMapIterator->second;
				const std::string &EXTENSION = extensionOptional.value();

				// consider Xtra Media extensions too
				if (LABEL_INFO.pathExtensions.empty()) {
					err = FindXtraAssetInfo(argsPointer, directorMediaPointer);

					if (err != kMoaErr_NoErr) {
						continue;
					}

					if (!directorMediaPointer->assetInfoOptional.has_value()) {
						Throw(kMoaErr_InternalError);
					}

					const Asset::Info &ASSET_INFO = directorMediaPointer->assetInfoOptional.value();

					if ((ASSET_INFO.pathExtensions.empty() && EXTENSION.empty())
						|| ASSET_INFO.pathExtensions.find(EXTENSION)) {
						break;
					}
				} else {
					if (LABEL_INFO.pathExtensions.find(EXTENSION)) {
						break;
					}
				}
			}

			// if we found a Label, but the Agent Options are not empty, try looking for an Agent too
			// (for example, prefer WAVE Agent over Sound Label if Agent Options are specified)
			bool empty = labelMappedVectorIterator != labelMappedVector.cend();

			if (empty
				&& argsPointer->optionsOptional.has_value()) {
				argsPointer->optionsOptional.value().getAgentOptionsValue(agentOptionsValue);
				ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(agentOptionsValue, empty, kMoaMmValueType_PropList));

				if (!empty) {
					ThrowErr(pObj->exportFileValueConverterPointer->testListValueEmpty(agentOptionsValue, empty));
				}
			}

			if (empty) {
				// don't redundantly look this up if GetArgAgentDefault is called later
				// (this might be undone later by GetArgAgent, which is intended)
				directorMediaPointer->agentInfoMapOptional.emplace();
			} else {
				/*
				if (!agentPositionPointer) {
					Throw(kMoaStatus_OK);
				}
				*/

				// this is seperated into a second loop
				// because finding the agents is quite slow
				for (labelMappedVectorIterator = labelMappedVector.cbegin(); labelMappedVectorIterator != labelMappedVector.cend(); labelMappedVectorIterator++) {
					argsPointer->labelSymbolVariant = *labelMappedVectorIterator;
					directorMediaPointer->labelInfoMapIterator = LABEL_INFO_NOT_FOUND;

					if (!std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
						Throw(kMoaErr_InternalError);
					}

					err = GetArgAgentDefault(argsPointer, directorMediaPointer);

					// if we can't get the agents for this label, just skip it
					if (err != kMoaErr_NoErr) {
						continue;
					}

					ThrowErr(err);

					if (argsPointer->agentStringOptional.has_value()) {
						break;
					}
				}

				if (labelMappedVectorIterator == labelMappedVector.cend()) {
					// agentStringOptional is set to an empty string because
					// this is the end of the road, so there is no benefit
					// to GetArgOptions calling this again
					// (it might get set to the user specified agent later, which is fine)
					argsPointer->labelSymbolVariant = labelSymbolVariant;
					argsPointer->agentStringOptional = "";

					directorMediaPointer->labelInfoMapIterator = LABEL_INFO_NOT_FOUND;
					directorMediaPointer->agentInfoMapOptional.emplace();
				}
			}
		} else {
			argsPointer->labelSymbolVariant = labelSymbolVariant;
			directorMediaPointer->labelInfoMapIterator = LABEL_INFO_NOT_FOUND;
		}
	}

	moa_catch

	if (argsPointer) {
		argsPointer->labelSymbolVariant = "";
	}

	if (directorMediaPointer) {
		directorMediaPointer->labelInfoMapIterator = LABEL_INFO_NOT_FOUND;
		directorMediaPointer->agentInfoMapOptional.emplace();
	}

	moa_catch_end

	releaseValue(agentOptionsValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgAgent(PMoaDrCallInfo callPtr, Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	std::string agentString = "";

	moa_try

	ThrowNull(callPtr);
	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	// this should override the agent string set by the path if any
	// (so this check was moved to GetArgAgentDefault)
	//if (!argsPointer->agentStringOptional.has_value()) {
	{
		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;

		MoaLong argIndex = pObj->argsBase + pObj->ARG_INDEX_AGENT;

		bool voidP = callPtr->nargs < argIndex;

		// get the agent as an integer or symbol
		// falling back to the previously discovered default if it is not specified
		if (!voidP) {
			AccessArgByIndex(argIndex, &argumentValue);

			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(argumentValue, voidP));
		}

		if (voidP) {
			ThrowErr(GetArgAgentDefault(argsPointer, directorMediaPointer));
		} else {
			// we do NOT check if the agent is already set like we do for labels
			// because it might have been set by GetArgLabelDefault already
			// (but this takes precedence)
			ThrowErr(pObj->exportFileValueConverterPointer->toString(argumentValue, agentString));
			argsPointer->agentStringOptional = agentString;

			// if this was set, unset it, it's no longer valid
			directorMediaPointer->agentInfoMapOptional = std::nullopt;

			// now ensure the agent is available
			// since we set the agent explicitly, we don't need to value_or here
			if (!argsPointer->agentStringOptional.value().empty()) {
				ThrowErr(FindAgentInfo(argsPointer, directorMediaPointer));
			}
		}
	}

	moa_catch

	if (argsPointer) {
		argsPointer->agentStringOptional = std::nullopt;
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgAgentDefault(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	std::optional<std::string> extensionOptional = std::nullopt;
	Agent::Info::MAP::const_iterator agentInfoMapIterator = {};

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	// this SHOULD check agentInfoMapOptional has a value and do nothing if so
	// even if the agent is empty, agentInfoMapOptional can have an empty map in it
	// which means: don't find the path agent yet
	if (!argsPointer->agentStringOptional.has_value()
		&& !directorMediaPointer->agentInfoMapOptional.has_value()) {
		if (!argsPointer->pathInfoOptional.has_value()) {
			argsPointer->pathInfoOptional.emplace(pObj->productVersionMajor, pObj->pCallback, pObj->pCalloc);
		}

		// bad param happens here if bad path passed in
		if (!argsPointer->pathInfoOptional.value().getExtensionOptional(extensionOptional, false)) {
			Throw(kMoaErr_BadParam);
		}

		argsPointer->agentStringOptional = std::nullopt;

		if (extensionOptional.has_value()) {
			ThrowErr(GetLabelAgentInfoMap(argsPointer, directorMediaPointer));

			if (!directorMediaPointer->agentInfoMapOptional.has_value()) {
				Throw(kMoaErr_InternalError);
			}

			const Agent::Info::MAP &AGENT_INFO_MAP = directorMediaPointer->agentInfoMapOptional.value();
			const std::string &EXTENSION = extensionOptional.value();

			for (agentInfoMapIterator = AGENT_INFO_MAP.begin(); agentInfoMapIterator != AGENT_INFO_MAP.end(); agentInfoMapIterator++) {
				const Path::EXTENSION_MAPPED_VECTOR &PATH_EXTENSIONS = agentInfoMapIterator->second.pathExtensions;

				if ((PATH_EXTENSIONS.empty() && EXTENSION.empty())
					|| PATH_EXTENSIONS.find(EXTENSION)) {
					argsPointer->agentStringOptional = agentInfoMapIterator->first;
					break;
				}
			}

			if (agentInfoMapIterator == AGENT_INFO_MAP.end()) {
				// NEVER set the agent string to an empty string here in GetArgAgentDefault
				// this method only cares about the current label
				// if the label changes later, setting the agent in advance will be wrong
				// important to clear contentPointer because we didn't find an agent
				// so we don't want an outdated reader, data object, etc. to be used under any circumstance
				directorMediaPointer->agentInfoMapOptional = std::nullopt;
				directorMediaPointer->contentPointer = 0;
			}
		} else {
			directorMediaPointer->agentInfoMapOptional = std::nullopt;
			directorMediaPointer->contentPointer = 0;
		}
	}

	moa_catch

	if (directorMediaPointer) {
		directorMediaPointer->agentInfoMapOptional = std::nullopt;
		directorMediaPointer->contentPointer = 0;
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgOptions(PMoaDrCallInfo callPtr, Args* argsPointer, Media::DirectorMedia* directorMediaPointer, MoaLong argIndex) {
	MoaMmValue incrementFilenameValue = kVoidMoaMmValueInitializer;
	MoaMmValue replaceExistingFileValue = kVoidMoaMmValueInitializer;
	MoaMmValue newFolderValue = kVoidMoaMmValueInitializer;
	MoaMmValue alternatePathExtensionValue = kVoidMoaMmValueInitializer;
	MoaMmValue locationValue = kVoidMoaMmValueInitializer;
	MoaMmValue writerClassIDValue = kVoidMoaMmValueInitializer;
	MoaMmValue agentOptionsValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(callPtr);
	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	//if (!argsPointer->optionsOptional.has_value()) {
	{
		MoaMmValue argumentValue = kVoidMoaMmValueInitializer;

		argIndex += pObj->argsBase;

		bool voidP = callPtr->nargs < argIndex;

		if (!voidP) {
			AccessArgByIndex(argIndex, &argumentValue);

			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(argumentValue, voidP, kMoaMmValueType_PropList));
		}

		argsPointer->optionsOptional.emplace(pObj->mmValueInterfacePointer);
		Options &options = argsPointer->optionsOptional.value();

		if (!voidP) {
			ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.IncrementFilename, incrementFilenameValue));
			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(incrementFilenameValue, voidP));

			if (!voidP) {
				ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&incrementFilenameValue, &options.incrementFilename));
			}

			ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.ReplaceExistingFile, replaceExistingFileValue));
			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(replaceExistingFileValue, voidP));

			if (!voidP) {
				ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&replaceExistingFileValue, &options.replaceExistingFile));
			}

			ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.NewFolder, newFolderValue));
			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(newFolderValue, voidP));

			if (!voidP) {
				ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&newFolderValue, &options.newFolder));
			}

			ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.AlternatePathExtension, alternatePathExtensionValue));
			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(alternatePathExtensionValue, voidP));

			if (!voidP) {
				ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&alternatePathExtensionValue, (PMoaLong)&options.alternatePathExtension));
			}

			ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.Location, locationValue));
			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(locationValue, voidP));

			if (!voidP) {
				ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&locationValue, &options.locationSymbol));
			}

			ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.WriterClassID, writerClassIDValue));
			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(writerClassIDValue, voidP));

			if (!voidP) {
				ThrowErr(pObj->exportFileValueConverterPointer->toID(writerClassIDValue, options.writerClassID));
			}

			ThrowErr(pObj->exportFileValueConverterPointer->getAProp(argumentValue, pObj->symbols.AgentOptions, agentOptionsValue));
			ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(agentOptionsValue, voidP));

			if (!voidP) {
				options.setAgentOptionsValue(agentOptionsValue);
			}
		}

		// this is where all the complicated stuff that has to happen for all arguments goes
		// if there is a label now (don't do this if GetArgOptions called but not GetArgLabel)
		// we call GetArgPathDefault here to populate the defaults
		// (regardless of if options were specified - this needs to happen last regardless)
		if (std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
			ThrowErr(GetArgPathDefault(argsPointer, directorMediaPointer));
		}

		// (this is still true if the agent options are set)
		if (!voidP) {
			// if the agent is still not specified (and NOT explicitly specified off)
			// and GetArgLabel was called previously (so the label symbol variant is a symbol)
			// we call GetArgLabel again here, since agent options were passed
			// strongly hinting that the user wants an agent to be used
			// this needs to happen after getting the default path info (above)
			// because GetArgLabel relies on the extension having been set
			// (the extension might even be the label's - but it doesn't matter!)
			// now that we set the agent options, GetArgLabel can know this
			// this will cause the WAVE Agent to be preferred over the Sound label
			// note that we do not call GetArgLabelDefault directly
			// since this shouldn't override the label passed in, if any
			// we don't reset the labelSymbolVariant so GetArgLabel does not do redundant work
			// if the label was specified by the user. It clears it by itself
			if (!argsPointer->agentStringOptional.has_value()
				&& std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
				// re-enable finding
				// this can have a value if a label was selected from the path
				// if so, we need to clear it again
				directorMediaPointer->agentInfoMapOptional = std::nullopt;

				// if the label was passed in, GetArgLabelDefault doesn't get called
				// in that case, we force the agent for the path to be looked up too by calling GetArgAgentDefault
				// it is fine to call directly because if the agent was user specified we wouldn't be here
				// and it'll do nothing if the agent WAS found by GetArgLabel
				ThrowErr(GetArgLabel(callPtr, argsPointer, directorMediaPointer));
				ThrowErr(GetArgAgentDefault(argsPointer, directorMediaPointer));
			}

			// agent options are void if no agent will be used
			if (!argsPointer->agentStringOptional.has_value()) {
				argsPointer->agentStringOptional = "";
			}

			if (argsPointer->agentStringOptional.value().empty()) {
				options.setAgentOptionsValue(kVoidMoaMmValueInitializer);
			}
		}

		// lastly, we get the default options (must happen after everything else)
		ThrowErr(GetArgOptionsDefault(argsPointer, directorMediaPointer, false));
	}

	moa_catch

	if (argsPointer) {
		argsPointer->optionsOptional = std::nullopt;
	}

	moa_catch_end

	releaseValue(incrementFilenameValue, pObj->mmValueInterfacePointer);
	releaseValue(replaceExistingFileValue, pObj->mmValueInterfacePointer);
	releaseValue(newFolderValue, pObj->mmValueInterfacePointer);
	releaseValue(alternatePathExtensionValue, pObj->mmValueInterfacePointer);
	releaseValue(locationValue, pObj->mmValueInterfacePointer);
	releaseValue(writerClassIDValue, pObj->mmValueInterfacePointer);
	releaseValue(agentOptionsValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgOptionsDefault(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, bool x) {
	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	// nothing else here yet, but eventually, maybe?
	ThrowErr(GetArgOptionsDefaultAgentOptions(argsPointer, directorMediaPointer, x));

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgOptionsDefaultAgentOptions(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, bool x) {
	PIMoaReader readerInterfacePointer = NULL;
	PIMoaDataObject dataObjectInterfacePointer = NULL;

	MoaMmValue agentOptionsValue = kVoidMoaMmValueInitializer;
	MoaMmValue propListValue = kVoidMoaMmValueInitializer;
	Agent::Info::WRITER_VECTOR::iterator writerVectorIterator = {};
	MoaMmValue emptyPropListValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	// this is a point of no return
	// we assign to the agent string here instead of just using value_or temporarily
	// because we don't want later code to see this doesn't have a value and
	// assign it something else (so these agent options become outdated)
	// the agent must be set in stone by this point
	if (!argsPointer->agentStringOptional.has_value()) {
		argsPointer->agentStringOptional = "";
	}

	if (!argsPointer->agentStringOptional.value().empty()) {
		if (!argsPointer->optionsOptional.has_value()) {
			argsPointer->optionsOptional.emplace(pObj->mmValueInterfacePointer);
		}

		Options &options = argsPointer->optionsOptional.value();

		options.getAgentOptionsValue(agentOptionsValue);

		bool _void = true;
		ThrowErr(pObj->exportFileValueConverterPointer->testValueVoid(agentOptionsValue, _void));

		if (_void) {
			ThrowErr(FindAgentInfo(argsPointer, directorMediaPointer));

			if (!directorMediaPointer->agentInfoOptional.has_value()) {
				Throw(kMoaErr_InternalError);
			}

			Agent::Info::WRITER_VECTOR &writerVector = directorMediaPointer->agentInfoOptional.value().writerVector;

			ThrowErr(CreateContentReader(argsPointer, directorMediaPointer));

			if (!directorMediaPointer->contentPointer) {
				Throw(kMoaErr_InternalError);
			}

			Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

			readerInterfacePointer = content.getReaderInterfacePointer();

			if (!readerInterfacePointer) {
				Throw(kMoaErr_InternalError);
			}

			// we must also create the data object because
			// the default agent options may change based on the data
			ThrowErr(CreateContentDataObject(argsPointer, directorMediaPointer));

			dataObjectInterfacePointer = content.getDataObjectInterfacePointer();

			if (!dataObjectInterfacePointer) {
				Throw(kMoaErr_InternalError);
			}

			ThrowErr(readerInterfacePointer->SetReaderDataSource(dataObjectInterfacePointer));

			ThrowErr(pObj->mmListInterfacePointer->NewPropListValue(&propListValue));

			// note: this is looped backwards so that the first agent's default options
			// override those of the last
			MoaError err = kMoaErr_NoErr;

			writerVectorIterator = writerVector.end();

			while (writerVectorIterator != writerVector.begin()) {
				err = GetArgOptionsDefaultAgentOptions(&propListValue, &*--writerVectorIterator, readerInterfacePointer, x);

				if (err != kMoaErr_NoErr) {
					break;
				}
			}

			// if there was any error getting the default options, then just have them be an empty property list
			// this way, at least the writer can still be used without options specified
			if (err == kMoaErr_NoErr) {
				options.setAgentOptionsValue(propListValue);
			} else {
				ThrowErr(pObj->mmListInterfacePointer->NewPropListValue(&emptyPropListValue));
				options.setAgentOptionsValue(emptyPropListValue);
			}
		}
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&readerInterfacePointer);
	releaseInterface((PPMoaVoid)&dataObjectInterfacePointer);

	releaseValue(agentOptionsValue, pObj->mmValueInterfacePointer);
	releaseValue(propListValue, pObj->mmValueInterfacePointer);
	releaseValue(emptyPropListValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetArgOptionsDefaultAgentOptions(PMoaMmValue propListValuePointer, Agent::Info::Writer* writerPointer, PIMoaReader readerInterfacePointer, bool x) {
	PIMoaWriter writerInterfacePointer = NULL;
	PIMoaDict dictInterfacePointer = NULL;

	moa_try

	ThrowNull(propListValuePointer);
	ThrowNull(writerPointer);
	ThrowNull(readerInterfacePointer);

	writerInterfacePointer = writerPointer->getWriterInterfacePointer();

	if (!writerInterfacePointer) {
		Throw(kMoaErr_InternalError);
	}

	// the data source can influence the writer's default options
	MoaError err = writerInterfacePointer->SetWriterDataSource(readerInterfacePointer);

	if (err == kMoaErr_NoErr) {
		err = writerInterfacePointer->GetDefaultWriterOptions(&dictInterfacePointer);

		if (err == kMoaErr_NoErr
			&& dictInterfacePointer) {
			// hack: return a default value if the default options can't be set on the writer
			// (this fixes a bug with TIFF, which considers its own defaults invalid)
			ThrowErr(writerInterfacePointer->SetWriterOptions(dictInterfacePointer));

			if (err == kMoaErr_NoErr) {
				ThrowErr(pObj->exportFileValueConverterPointer->concatToPropList(*propListValuePointer, x, dictInterfacePointer));
			}
		}
	}

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&writerInterfacePointer);
	releaseInterface((PPMoaVoid)&dictInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetLoaded(PMoaLong loadedPointer, PIMoaDrCastMem drCastMemInterfacePointer) {
	MoaMmValue loadedValue = kVoidMoaMmValueInitializer;

	moa_try

	ThrowNull(loadedPointer);
	ThrowNull(drCastMemInterfacePointer);

	ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.Loaded, &loadedValue));
	ThrowErr(pObj->mmValueInterfacePointer->ValueToInteger(&loadedValue, loadedPointer));

	moa_catch

	if (loadedPointer) {
		*loadedPointer = FALSE;
	}

	moa_catch_end

	releaseValue(loadedValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetTypeDisplayName(MoaMmSymbol typeSymbol, std::string* typeDisplayNamePointer) {
	PMoaVoid typeDisplayNameStringPointer = NULL;

	moa_try

	ThrowNull(typeDisplayNamePointer);

	// kMoaMmMaxXtraDisplayName includes the null byte, we only add one
	// so stringTruncated doesn't go off when it shouldn't
	MoaUlong typeDisplayNameStringSize = kMoaMmMaxXtraDisplayName + 1;
	typeDisplayNameStringPointer = pObj->pCalloc->NRAlloc(typeDisplayNameStringSize);
	ThrowNull(typeDisplayNameStringPointer);

	// this is a regular while loop instead of a do while like I normally use for this pattern
	// (because otherwise, displayNameStringSize will be incorrectly incremented for every outer loop)
	ThrowErr(pObj->drPlayerInterfacePointer->GetCastMemTypeDisplayName(typeSymbol, (PMoaChar)typeDisplayNameStringPointer, typeDisplayNameStringSize));

	while (/*err == kMoaMixErr_BufferTooSmall
		|| */stringTruncated((PMoaChar)typeDisplayNameStringPointer, typeDisplayNameStringSize)) {
		typeDisplayNameStringSize += typeDisplayNameStringSize;

		freeMemory(typeDisplayNameStringPointer, pObj->pCalloc);

		typeDisplayNameStringPointer = pObj->pCalloc->NRAlloc(typeDisplayNameStringSize);
		ThrowNull(typeDisplayNameStringPointer);

		ThrowErr(pObj->drPlayerInterfacePointer->GetCastMemTypeDisplayName(typeSymbol, (PMoaChar)typeDisplayNameStringPointer, typeDisplayNameStringSize));
	}

	std::string &typeDisplayName = *typeDisplayNamePointer;
	typeDisplayName = (PMoaChar)typeDisplayNameStringPointer;
	Asset::trimEllipsis(typeDisplayName);

	moa_catch

	if (typeDisplayNamePointer) {
		*typeDisplayNamePointer = "";
	}

	moa_catch_end

	freeMemory(typeDisplayNameStringPointer, pObj->pCalloc);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetLabelMappedVector(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;
	PIMoaMmXAsset mmXAssetInterfacePointer = NULL;

	MoaMmValue typeValue = kVoidMoaMmValueInitializer;
	MoaMmValue scriptTextValue = kVoidMoaMmValueInitializer;
	MoaMmValue scriptSyntaxValue = kVoidMoaMmValueInitializer;
	TypeLabel::MAP::const_iterator foundTypeLabel = {};
	Label::MAPPED_VECTOR::CONST_ITERATOR labelMappedVectorIterator = {};
	Args getLabelMappedVectorArgs;
	Media::DirectorMedia getLabelMappedVectorDirectorMedia(LABEL_INFO_NOT_FOUND);

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->labelMappedVectorOptional.has_value()) {
		directorMediaPointer->labelMappedVectorOptional.emplace();

		Label::MAPPED_VECTOR &labelMappedVector = directorMediaPointer->labelMappedVectorOptional.value();

		drCastMemInterfacePointer = argsPointer->getDrCastMemInterfacePointer();

		if (!drCastMemInterfacePointer) {
			Throw(kMoaErr_InternalError);
		}

		// get the member's type as a symbol
		ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.Type, &typeValue));

		MoaMmSymbol typeSymbol = 0;
		ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&typeValue, &typeSymbol));

		// very important
		if (typeSymbol != pObj->symbols.Empty) {
			// if true, the member's type is a script
			bool script = typeSymbol == pObj->symbols.Script;

			// if true, the member's type is not a script, but the scriptText is not empty
			bool memberScriptText = !script;

			if (memberScriptText) {
				ThrowErr(drCastMemInterfacePointer->GetProp(pObj->symbols.ScriptText, &scriptTextValue));

				bool scriptTextEmpty = true;
				ThrowErr(pObj->exportFileValueConverterPointer->testStringValueEmpty(scriptTextValue, scriptTextEmpty));

				memberScriptText = !scriptTextEmpty;
			}

			MoaLong linked = 0;
			ThrowErr(pObj->exportFileValueConverterPointer->testMemberLinked(linked, drCastMemInterfacePointer));

			MoaError err = kMoaErr_NoErr;

			// get the scriptSyntax of the member so we can return either #lingo or #javascript label correctly
			MoaMmSymbol scriptSyntaxSymbol = 0;

			// only get the property if we need it
			if (memberScriptText || (script && !linked)) {
				err = drCastMemInterfacePointer->GetProp(pObj->symbols.ScriptSyntax, &scriptSyntaxValue);

				if (err != kMoaMmErr_PropertyNotFound) {
					ThrowErr(err);
				}

				// manually check error here, default to #lingo (for pre-Director MX 2004)
				if (err == kMoaErr_NoErr) {
					ThrowErr(pObj->mmValueInterfacePointer->ValueToSymbol(&scriptSyntaxValue, &scriptSyntaxSymbol));
				} else {
					scriptSyntaxSymbol = pObj->symbols.Lingo;
				}
			}

			if (linked) {
				// for linked members, we don't add #composite or use type labels
				if (memberScriptText) {
					// if there's still script text on the member give the label for that
					// unless the member type is script (meaning it's a linked script, which we don't export)
					labelMappedVector = { scriptSyntaxSymbol };
				}
			} else {
				const TypeLabel::MAP &TYPE_LABEL_MAP = pObj->typeLabelsPointer->get();

				foundTypeLabel = TYPE_LABEL_MAP.find(typeSymbol);

				// otherwise get the label mapped vector for this type
				if (foundTypeLabel == TYPE_LABEL_MAP.end()) {
					labelMappedVector = {};
				} else {
					labelMappedVector = foundTypeLabel->second;
				}

				// interface property lookup for #xtraMedia
				err = pObj->exportFileValueConverterPointer->toAsset(drCastMemInterfacePointer, mmXAssetInterfacePointer);

				if (err == kMoaErr_NoErr
					&& mmXAssetInterfacePointer) {
					labelMappedVector.push(pObj->symbols.XtraMedia);
				}

				// if the member type is #script add the scriptSyntax as a label
				if (script) {
					labelMappedVector.push(scriptSyntaxSymbol);
				}

				// for members with media #composite is always added
				labelMappedVector.push(pObj->symbols.Composite);

				// if member type is not script #composite takes priority over script label
				if (memberScriptText) {
					labelMappedVector.push(scriptSyntaxSymbol);
				}
			}

			// now erase any irrelevant labels for this member
			if (!mmXAssetInterfacePointer) {
				for (labelMappedVectorIterator = labelMappedVector.cbegin(); labelMappedVectorIterator != labelMappedVector.cend(); labelMappedVectorIterator++) {
					getLabelMappedVectorArgs.labelSymbolVariant = *labelMappedVectorIterator;
					getLabelMappedVectorDirectorMedia.labelInfoMapIterator = LABEL_INFO_NOT_FOUND;

					ThrowErr(FindLabelInfo(&getLabelMappedVectorArgs, &getLabelMappedVectorDirectorMedia));

					if (getLabelMappedVectorDirectorMedia.labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
						Throw(kMoaErr_InternalError);
					}

					const Label::Info &LABEL_INFO = getLabelMappedVectorDirectorMedia.labelInfoMapIterator->second;

					if (LABEL_INFO.labelType & Label::TYPE_XTRA_MEDIA) {
						//&& !mmXAssetInterfacePointer) {
						labelMappedVector.eraseIterator(labelMappedVectorIterator);
					}
				}
			}
		}
	}

	moa_catch

	if (directorMediaPointer) {
		directorMediaPointer->labelMappedVectorOptional = std::nullopt;
	}

	moa_catch_end

	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
	releaseInterface((PPMoaVoid)&mmXAssetInterfacePointer);

	releaseValue(typeValue, pObj->mmValueInterfacePointer);
	releaseValue(scriptTextValue, pObj->mmValueInterfacePointer);
	releaseValue(scriptSyntaxValue, pObj->mmValueInterfacePointer);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetLabelAgentInfoMap(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	Label::AGENT_INFO_MAP::iterator labelAgentInfoMapIterator = {};

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->agentInfoMapOptional.has_value()) {
		if (!std::holds_alternative<MoaMmSymbol>(argsPointer->labelSymbolVariant)) {
			Throw(kMoaErr_InternalError);
		}

		const MoaMmSymbol &LABEL_SYMBOL = std::get<MoaMmSymbol>(argsPointer->labelSymbolVariant);

		// so that we have agentMoaIDsHash
		ThrowErr(GetAgentHiddenReaderSet(directorMediaPointer));

		if (directorMediaPointer->agentMoaIDsHash == pObj->agentMoaIDsHash) {
			labelAgentInfoMapIterator = pObj->labelAgentInfoMapPointer->find(LABEL_SYMBOL);
		} else {
			*pObj->labelAgentInfoMapPointer = {};
			pObj->agentMoaIDsHash = directorMediaPointer->agentMoaIDsHash;

			labelAgentInfoMapIterator = pObj->labelAgentInfoMapPointer->end();
		}

		if (labelAgentInfoMapIterator == pObj->labelAgentInfoMapPointer->end()) {
			MoaError err = GetAgentInfoMapSLOW(argsPointer, directorMediaPointer);

			if (err == kMoaErr_NoErr) {
				if (!directorMediaPointer->agentInfoMapOptional.has_value()) {
					// succeeded, but no map found, so create an empty one
					directorMediaPointer->agentInfoMapOptional.emplace();
				}
			} else {
				// error occured, map may be invalid, so empty it
				// we don't throw this error because most likely, we just failed to get a reader
				// precisely because this label can't be used with agents, so it should be an empty map
				directorMediaPointer->agentInfoMapOptional.emplace();
			}

			(*pObj->labelAgentInfoMapPointer)[LABEL_SYMBOL] = directorMediaPointer->agentInfoMapOptional.value();
		} else {
			directorMediaPointer->agentInfoMapOptional = labelAgentInfoMapIterator->second;
		}
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetContentReaderRegistryEntryDict(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaRegistryEntryDict readerRegistryEntryDictInterfacePointer = NULL;

	Registry::Entry::VARIANT registryEntryVariant = {};

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->contentPointer) {
		directorMediaPointer->contentPointer = std::make_shared<Media::DirectorMedia::Content>();
		ThrowNull(directorMediaPointer->contentPointer);
	}

	Media::DirectorMedia::Content &content = *directorMediaPointer->contentPointer;

	readerRegistryEntryDictInterfacePointer = content.getReaderRegistryEntryDictInterfacePointer();

	if (!readerRegistryEntryDictInterfacePointer) {
		ThrowErr(FindLabelInfo(argsPointer, directorMediaPointer));

		if (directorMediaPointer->labelInfoMapIterator == LABEL_INFO_NOT_FOUND) {
			Throw(kMoaErr_InternalError);
		}

		Registry::Entry &registryEntry = std::get<Registry::Entry>(registryEntryVariant);
		registryEntry.classIDPointer = (MoaClassID*)&directorMediaPointer->labelInfoMapIterator->second.agentReaderClassID;
		registryEntry.interfaceIDPointer = (MoaInterfaceID*)&IID_IMoaReader;

		ThrowFailed(
			pObj->cacheInterfacePointer->EnumerateRegistryEntries(
				MoaCacheRegistryEntryEnumProc_RegistryEntryDict_TStdXtra,
				(PMoaVoid)&registryEntryVariant
			)
		);

		if (!std::holds_alternative<PIMoaRegistryEntryDict>(registryEntryVariant)) {
			Throw(kMoaErr_BadClass);
		}

		readerRegistryEntryDictInterfacePointer = std::get<PIMoaRegistryEntryDict>(registryEntryVariant);
		ThrowNull(readerRegistryEntryDictInterfacePointer);

		content.setReaderRegistryEntryDictInterfacePointer(readerRegistryEntryDictInterfacePointer);
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetReceptorIDs(Args* argsPointer, Media::DirectorMedia* directorMediaPointer, PMoaLong receptorCountPointer, PPMoaVoid receptorIDsPointerPointer) {
	PIMoaReader readerInterfacePointer = NULL;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);
	ThrowNull(receptorCountPointer);
	ThrowNull(receptorIDsPointerPointer);

	ThrowErr(CreateContentReader(argsPointer, directorMediaPointer));

	if (!directorMediaPointer->contentPointer) {
		Throw(kMoaErr_InternalError);
	}

	readerInterfacePointer = directorMediaPointer->contentPointer->getReaderInterfacePointer();

	if (!readerInterfacePointer) {
		Throw(kMoaErr_InternalError);
	}

	MoaLong &receptorCount = *receptorCountPointer;
	ThrowErr(readerInterfacePointer->CountUnderstoodReceptorIDs(&receptorCount));

	const MoaUlong MOA_INTERFACE_ID_SIZE = sizeof(MoaInterfaceID);

	MoaUlong receptorIDsSize = receptorCount * MOA_INTERFACE_ID_SIZE;

	PMoaVoid &receptorIDsPointer = *receptorIDsPointerPointer;
	receptorIDsPointer = pObj->pCalloc->NRAlloc(receptorIDsSize);
	ThrowNull(receptorIDsPointer);

	MoaInterfaceID* receptorIDPointer = (MoaInterfaceID*)receptorIDsPointer;
	ThrowNull(receptorIDPointer);

	for (MoaLong i = 0; i < receptorCount; i++) {
		ThrowErr(readerInterfacePointer->GetNthUnderstoodReceptorID(i, receptorIDPointer));
		ThrowNull(++receptorIDPointer);
	}

	moa_catch

	if (receptorCountPointer) {
		*receptorCountPointer = 0;
	}

	if (receptorIDsPointerPointer) {
		freeMemory(*receptorIDsPointerPointer, pObj->pCalloc);
	}

	moa_catch_end

	releaseInterface((PPMoaVoid)&readerInterfacePointer);

	moa_try_end
}

/*
Please don't call me directly!
Go through GetLabelAgentInfoMap instead, which caches the results of GetAgentInfoMapSLOW
*/
MoaError TStdXtra_IMoaMmXScript::GetAgentInfoMapSLOW(Args* argsPointer, Media::DirectorMedia* directorMediaPointer) {
	PIMoaEnumMixAgentInfo enumMixWriterInfoInterfacePointer = NULL;

	PMoaVoid receptorIDsPointer = NULL;

	moa_try

	ThrowNull(argsPointer);
	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->agentInfoMapOptional.has_value()) {
		directorMediaPointer->agentInfoMapOptional.emplace();

		ThrowErr(GetAgentHiddenReaderSet(directorMediaPointer));

		if (!directorMediaPointer->agentHiddenReaderSetPointer) {
			Throw(kMoaErr_InternalError);
		}

		MoaLong receptorCount = 0;
		ThrowErr(GetReceptorIDs(argsPointer, directorMediaPointer, &receptorCount, &receptorIDsPointer));
		
		if (!receptorIDsPointer) {
			Throw(kMoaErr_InternalError);
		}

		if (!pObj->agentServicesInterfacePointer) {
			ThrowErr(GetIMoaAgentServices(pObj->pCallback, &pObj->agentServicesInterfacePointer));
			ThrowNull(pObj->agentServicesInterfacePointer);
		}

		// we don't consider no writers to be an error
		MoaError err = pObj->agentServicesInterfacePointer->GetWriterInfo(receptorCount, (MoaInterfaceID*)receptorIDsPointer, &enumMixWriterInfoInterfacePointer);

		if (err != kMoaMixErr_NoSuchWriter) {
			do {
				err = EnumWriter(&directorMediaPointer->agentInfoMapOptional.value(), directorMediaPointer->agentHiddenReaderSetPointer.get(), enumMixWriterInfoInterfacePointer);
				
				ThrowFailed(err);
			} while (err == kMoaStatus_OK);
		}
	}

	moa_catch

	if (directorMediaPointer) {
		directorMediaPointer->agentInfoMapOptional = std::nullopt;
	}

	moa_catch_end

	releaseInterface((PPMoaVoid)&enumMixWriterInfoInterfacePointer);

	freeMemory(receptorIDsPointer, pObj->pCalloc);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetAgentHiddenReaderSet(Media::DirectorMedia* directorMediaPointer) {
	moa_try

	ThrowNull(directorMediaPointer);

	if (!directorMediaPointer->agentHiddenReaderSetPointer) {
		Registry::AgentHiddenReaderSet registryEntriesAgentHiddenReaderSet = {};

		registryEntriesAgentHiddenReaderSet.agentMoaIDsHashPointer = &directorMediaPointer->agentMoaIDsHash;

		directorMediaPointer->agentHiddenReaderSetPointer = std::make_shared<Agent::HIDDEN_READER_SET>();
		ThrowNull(directorMediaPointer->agentHiddenReaderSetPointer);

		registryEntriesAgentHiddenReaderSet.agentHiddenReaderSetPointer = directorMediaPointer->agentHiddenReaderSetPointer.get();

		ThrowFailed(
			pObj->cacheInterfacePointer->EnumerateRegistryEntries(
				MoaCacheRegistryEntryEnumProc_AgentHiddenReaderSet_TStdXtra,
				(PMoaVoid)&registryEntriesAgentHiddenReaderSet
			)
		);
	}

	moa_catch

	if (directorMediaPointer) {
		directorMediaPointer->agentMoaIDsHash = 0;
		directorMediaPointer->agentHiddenReaderSetPointer = 0;
	}

	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetFormatName(MixFormat mixFormat, std::string* formatNamePointer) {
	PMoaVoid formatNameStringPointer = NULL;

	moa_try

	ThrowNull(formatNamePointer);

	if (!pObj->formatServicesInterfacePointer) {
		ThrowErr(GetIMoaFormatServices(pObj->pCallback, &pObj->formatServicesInterfacePointer));
		ThrowNull(pObj->formatServicesInterfacePointer);
	}

	MoaError err = kMoaErr_NoErr;

	MoaUlong formatNameStringSize = kMoaMmMaxPropName + 1;

	// this truncates the string instead of returning the error code (sometimes?)
	do {
		if (formatNameStringPointer) {
			formatNameStringSize += formatNameStringSize;
		}

		freeMemory(formatNameStringPointer, pObj->pCalloc);

		formatNameStringPointer = pObj->pCalloc->NRAlloc(formatNameStringSize);
		ThrowNull(formatNameStringPointer);

		err = pObj->formatServicesInterfacePointer->GetFormatName(mixFormat, formatNameStringSize, (PMoaChar)formatNameStringPointer);
	} while (err == kMoaMixErr_BufferTooSmall
		|| (err == kMoaErr_NoErr
		&& stringTruncated((PMoaChar)formatNameStringPointer, formatNameStringSize)));

	ThrowErr(err);

	*formatNamePointer = (PMoaChar)formatNameStringPointer;

	moa_catch
	moa_catch_end

	freeMemory(formatNameStringPointer, pObj->pCalloc);

	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetAgentInfoHidden(Agent::HIDDEN_READER_SET* agentHiddenReaderSetPointer, MoaClassID* classIDPointer, MoaBool* agentInfoHiddenPointer, PIMoaMixAgentInfo mixWriterInfoInterfacePointer) {
	moa_try

	ThrowNull(agentHiddenReaderSetPointer);
	ThrowNull(classIDPointer);
	ThrowNull(agentInfoHiddenPointer);
	ThrowNull(mixWriterInfoInterfacePointer);

	MoaLong flags = 0;
	ThrowErr(mixWriterInfoInterfacePointer->GetAgentFlags(&flags));

	MoaBool &agentInfoHidden = *agentInfoHiddenPointer;
	agentInfoHidden = flags & kMoaAgentInfoFlags_Hidden;

	if (!agentInfoHidden) {
		Agent::HIDDEN_READER_SET &agentHiddenReaderSet = *agentHiddenReaderSetPointer;
		agentInfoHidden = agentHiddenReaderSet.find(*classIDPointer) != agentHiddenReaderSet.end();
	}

	// this doesn't work because it gets the agent's dictionary (not the reader's)
	/*
	err = mixWriterInfoInterfacePointer->GetAgentRegInfo(&writerRegistryEntryDictInterfacePointer, NULL);

	if (err == kMoaErr_NoErr
		&& writerRegistryEntryDictInterfacePointer) {
		const MoaLong AGENT_REGISTRY_ENTRY_DICT_INTERFACE_POINTER_SIZE = sizeof(agentRegistryEntryDictInterfacePointer);

		err = writerRegistryEntryDictInterfacePointer->Get(kMoaDictType_Dict, &agentRegistryEntryDictInterfacePointer, AGENT_REGISTRY_ENTRY_DICT_INTERFACE_POINTER_SIZE, kWriterRegKey_AgentRegDict);

		if (err == kMoaErr_NoErr
			&& agentRegistryEntryDictInterfacePointer) {
			agentInfoPointer->hidden = FALSE;
			const MoaLong HIDDEN_SIZE = sizeof(agentInfoPointer->hidden);

			// do not error if this fails
			agentRegistryEntryDictInterfacePointer->Get(kMoaDictType_Bool, &agentInfoPointer->hidden, HIDDEN_SIZE, kReaderRegKey_Hidden);
		}
	}
	*/

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError TStdXtra_IMoaMmXScript::GetAgentInfoName(std::string* agentInfoNamePointer, PIMoaMixAgentInfo mixWriterInfoInterfacePointer) {
	PMoaVoid agentNameStringPointer = NULL;

	moa_try

	ThrowNull(agentInfoNamePointer);
	ThrowNull(mixWriterInfoInterfacePointer);

	MoaError err = kMoaErr_NoErr;

	MoaUlong agentNameStringSize = kMoaMmMaxXtraName + 1;

	do {
		if (agentNameStringPointer) {
			agentNameStringSize += agentNameStringSize;
		}

		freeMemory(agentNameStringPointer, pObj->pCalloc);

		agentNameStringPointer = pObj->pCalloc->NRAlloc(agentNameStringSize);
		ThrowNull(agentNameStringPointer);

		err = mixWriterInfoInterfacePointer->GetAgentName(agentNameStringSize, (PMoaChar)agentNameStringPointer);
	} while (err == kMoaMixErr_BufferTooSmall
		|| (err == kMoaErr_NoErr
		&& stringTruncated((PMoaChar)agentNameStringPointer, agentNameStringSize)));

	ThrowErr(err);

	*agentInfoNamePointer = (PMoaChar)agentNameStringPointer;

	moa_catch
	moa_catch_end

	freeMemory(agentNameStringPointer, pObj->pCalloc);

	moa_try_end
}

// "Each of you should use whatever gift you have received to serve others, as faithful stewards of God's grace in its various forms." - 1 Peter 4:10