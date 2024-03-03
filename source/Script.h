/*
ADOBE SYSTEMS INCORPORATED
Copyright 1994 - 2008 Adobe Macromedia Software LLC
All Rights Reserved

NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the 
terms of the Adobe license agreement accompanying it.  If you have received this file from a 
source other than Adobe, then your use, modification, or distribution of it requires the prior 
written permission of Adobe.
*/

#define _CRT_NO_VA_START_VALIDATION

#ifndef _H_Script
#define _H_Script

#include "shared.h"
#include "Registry.h"
#include "ExportFileValueConverter.h"
#include "Label.h"
#include "TypeLabels.h"
#include "Asset.h"
#include "Args.h"
#include "Media.h"
#include "Agent.h"
#include <vector>

extern "C" {
	#include "exportfile.h"
}

#include "mmixasst.h"
#include "driservc.h"
#include "drivalue.h"
#include "mixsvc.h"
#include "xclassver.h"

#ifdef MACINTOSH
#include "MacPICTAgent.h"
#endif
#ifdef WINDOWS
#include "WinBMPAgent.h"
#endif

#define kExportFileRegKey_LastErrCode MOADICT_RUNTIME_KEY_PREFIX "LastErrCode"

#define CLSID_TStdXtra IID_IExportFile

/*
IMPORTANT: you can't use RAII classes here!
This struct is malloc'd and free'd by the host application, which bypasses unwinding
Any object MUST be a raw pointer, which is new'd by MoaCreate, and
then deleted by MoaDestroy. That means no std::unique_ptr! Don't even think about it
It'll appear to be working, but the destructor will never be called. Don't.
It is fine to use RAII classes everywhere else, just not on this one class
*/
EXTERN_BEGIN_DEFINE_CLASS_INSTANCE_VARS(TStdXtra)
	PIMoaCache cacheInterfacePointer = NULL;
	PIMoaDrPlayer drPlayerInterfacePointer = NULL;
	PIMoaMmValue mmValueInterfacePointer = NULL;
	PIMoaDrValue drValueInterfacePointer = NULL;
	PIMoaMmList mmListInterfacePointer = NULL;
	PIMoaDrUtils drUtilsInterfacePointer = NULL;
	PIMoaHandle handleInterfacePointer = NULL;

	PIMoaMmImage mmImageInterfacePointer = NULL;
	PIMoaDrMediaValue drMediaValueInterfacePointer = NULL;

	PIMoaFormatServices formatServicesInterfacePointer = NULL;
	PIMoaDataObjectServices dataObjectServicesInterfacePointer = NULL;
	PIMoaAgentServices agentServicesInterfacePointer = NULL;

	Registry::Entry::VARIANT* registryEntryVariantPointer = NULL;

	ExportFileValueConverter* exportFileValueConverterPointer = 0;
	Label::Labels::Info* labelsInfoPointer = 0;
	TypeLabel::TypeLabels* typeLabelsPointer = 0;
	Asset::Assets::Info* assetsInfoPointer = 0;

	size_t agentMoaIDsHash = 0;
	Label::AGENT_INFO_MAP* labelAgentInfoMapPointer = 0;

	size_t assetMoaIDsHash = 0;
	Asset::Info::MAP* assetInfoMapPointer = 0;

	#ifdef WINDOWS
	HANDLE_VECTOR* threadVectorPointer = 0;
	MODULE_HANDLE_VECTOR* moduleHandleVectorPointer = 0;
	#endif

	unsigned long productVersionMajor = 0;

	struct Symbols {
		MoaMmSymbol AgentOptions = 0;
		MoaMmSymbol AgentPropList = 0;
		MoaMmSymbol AlternatePathExtension = 0;
		MoaMmSymbol Basename = 0;
		MoaMmSymbol Bitmap = 0;
		MoaMmSymbol Composite = 0;
		MoaMmSymbol Current = 0;
		MoaMmSymbol DefaultAgent = 0;
		MoaMmSymbol DefaultLabel = 0;
		MoaMmSymbol DefaultOptions = 0;
		MoaMmSymbol DefaultPath = 0;
		MoaMmSymbol Dirname = 0;
		MoaMmSymbol DisplayName = 0;
		MoaMmSymbol Documents = 0;
		MoaMmSymbol Empty = 0;
		MoaMmSymbol Extension = 0;
		MoaMmSymbol Filename = 0;
		MoaMmSymbol IconPropList = 0;
		MoaMmSymbol Image = 0;
		MoaMmSymbol IncrementFilename = 0;
		MoaMmSymbol LabelList = 0;
		MoaMmSymbol Lingo = 0;
		MoaMmSymbol Loaded = 0;
		MoaMmSymbol Location = 0;
		MoaMmSymbol MixerSaved = 0;
		MoaMmSymbol Music = 0;
		MoaMmSymbol Name = 0;
		MoaMmSymbol NewFolder = 0;
		MoaMmSymbol Picture = 0;
		MoaMmSymbol Pictures = 0;
		MoaMmSymbol ReplaceExistingFile = 0;
		MoaMmSymbol SampleSize = 0;
		MoaMmSymbol Script = 0;
		MoaMmSymbol ScriptSyntax = 0;
		MoaMmSymbol ScriptText = 0;
		MoaMmSymbol Sound = 0;
		MoaMmSymbol Type = 0;
		MoaMmSymbol TypePropList = 0;
		MoaMmSymbol Videos = 0;
		MoaMmSymbol WriterClassID = 0;
		MoaMmSymbol XtraMedia = 0;
	};

	Symbols symbols;

	static const MoaLong ARG_INDEX_MIXER_SAVED_CALL_HANDLER = 1;
	static const MoaLong ARG_INDEX_MEMBER = 1;
	static const MoaLong ARG_INDEX_PATH = 2;
	static const MoaLong ARG_INDEX_INFO = ARG_INDEX_PATH;
	static const MoaLong ARG_INDEX_LABEL = 3;
	static const MoaLong ARG_INDEX_AGENT = 4;

	MoaLong argsBase = 0;
EXTERN_END_DEFINE_CLASS_INSTANCE_VARS

EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(TStdXtra, IMoaRegister)
	EXTERN_DEFINE_METHOD(MoaError, Register, (PIMoaCache, PIMoaXtraEntryDict))
EXTERN_END_DEFINE_CLASS_INTERFACE

EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(TStdXtra, IMoaMmXScript)
	EXTERN_DEFINE_METHOD(MoaError, Call, (PMoaDrCallInfo))
	private:
	/* Methods */
	EXTERN_DEFINE_METHOD(MoaError, ExportFileStatus, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, ExportFileOut, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, TellExportFileMixerSaved, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileLabelList, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileAgentPropList, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileDefaultPath, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileDefaultLabel, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileDefaultAgent, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileDefaultOptions, (PMoaDrCallInfo, bool))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileDisplayName, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileTypePropList, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetExportFileIconPropList, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, CallTell, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, CallGet, (PMoaDrCallInfo))

	/* Load */
	// Member
	EXTERN_DEFINE_METHOD(MoaError, LoadMember, (PIMoaDrCastMem))

	/* Find */
	// Label
	EXTERN_DEFINE_METHOD(MoaError, FindLabelInfo, (Args*, Media::DirectorMedia*))

	// Agent
	EXTERN_DEFINE_METHOD(MoaError, FindAgentInfo, (Args*, Media::DirectorMedia*))

	// Asset
	EXTERN_DEFINE_METHOD(MoaError, FindXtraAssetInfo, (Args*, Media::DirectorMedia*))

	/* Create */
	// Mixer
	EXTERN_DEFINE_METHOD(MoaError, CreateMixerThread, (Args*, Media::DirectorMedia*))

	// Label
	EXTERN_DEFINE_METHOD(MoaError, CreateContentFormat, (Args*, Media::DirectorMedia*))

	// Agent
	EXTERN_DEFINE_METHOD(MoaError, CreateContentReader, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, CreateContentDataObject, (Args*, Media::DirectorMedia*))

	/* Enum */
	// Agent
	EXTERN_DEFINE_METHOD(MoaError, EnumWriter, (Agent::Info::MAP*, Agent::HIDDEN_READER_SET*, PIMoaEnumMixAgentInfo))

	/* Handle */
	// File
	EXTERN_DEFINE_METHOD(MoaError, HandleCreateFileError, (Args*, Media::DirectorMedia*, MoaError, PIMoaFile))
	EXTERN_DEFINE_METHOD(MoaError, HandleFileNotFound, (Args*, Media::DirectorMedia*, PIMoaFile))
	EXTERN_DEFINE_METHOD(MoaError, HandleDuplicateSpec, (Args*, Media::DirectorMedia*, PIMoaFile))
	EXTERN_DEFINE_METHOD(MoaError, HandleDefaultCreateFileError, (Args*, Media::DirectorMedia*, MoaError, PIMoaFile))
	EXTERN_DEFINE_METHOD(MoaError, HandleWriteFileError, (Args*, Media::DirectorMedia*, MoaError, PIMoaFile))

	/* Write */
	// File
	EXTERN_DEFINE_METHOD(MoaError, WriteFile, (Args*, Media::DirectorMedia*, PIMoaFile))
	EXTERN_DEFINE_METHOD(MoaError, WriteFileAgent, (const Agent::Info::Writer*, PMoaMmValue, PIMoaFile, PIMoaReader))

	/* Add */
	// Agent
	EXTERN_DEFINE_METHOD(MoaError, AddAgentInfoExtensions, (ConstPMoaChar, Path::EXTENSION_MAPPED_VECTOR*, PIMoaMixFormatInfo))

	/* Get */
	// Registry
	EXTERN_DEFINE_METHOD(MoaError, GetRegistryEntryDict, (PIMoaRegistryEntryDict*))

	// Args
	EXTERN_DEFINE_METHOD(MoaError, GetArgs, (PMoaDrCallInfo, Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgLong, (PMoaDrCallInfo, std::optional<MoaLong>*, MoaLong))
	EXTERN_DEFINE_METHOD(MoaError, GetArgPropertyName, (PMoaDrCallInfo, PMoaMmSymbol))
	EXTERN_DEFINE_METHOD(MoaError, GetArgMixerSavedCallHandler, (PMoaDrCallInfo, PMoaLong))
	EXTERN_DEFINE_METHOD(MoaError, GetArgMember, (PMoaDrCallInfo, Args*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgPath, (PMoaDrCallInfo, Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgPathDefault, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgPathDefaultDirname, (Args*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgPathDefaultExtension, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgPathDefaultBasename, (Args*, MoaError))
	EXTERN_DEFINE_METHOD(MoaError, GetArgInfo, (PMoaDrCallInfo, Args*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgLabel, (PMoaDrCallInfo, Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgLabelDefault, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgAgent, (PMoaDrCallInfo, Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgAgentDefault, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetArgOptions, (PMoaDrCallInfo, Args*, Media::DirectorMedia*, MoaLong))
	EXTERN_DEFINE_METHOD(MoaError, GetArgOptionsDefault, (Args*, Media::DirectorMedia*, bool))
	EXTERN_DEFINE_METHOD(MoaError, GetArgOptionsDefaultAgentOptions, (Args*, Media::DirectorMedia*, bool))
	EXTERN_DEFINE_METHOD(MoaError, GetArgOptionsDefaultAgentOptions, (PMoaMmValue, Agent::Info::Writer*, PIMoaReader, bool))

	// Member
	EXTERN_DEFINE_METHOD(MoaError, GetLoaded, (PMoaLong, PIMoaDrCastMem))
	EXTERN_DEFINE_METHOD(MoaError, GetTypeDisplayName, (MoaMmSymbol, std::string*))

	// Label
	EXTERN_DEFINE_METHOD(MoaError, GetLabelMappedVector, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetLabelAgentInfoMap, (Args*, Media::DirectorMedia*))

	// Agent
	EXTERN_DEFINE_METHOD(MoaError, GetContentReaderRegistryEntryDict, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetReceptorIDs, (Args*, Media::DirectorMedia*, PMoaLong, PPMoaVoid))
	EXTERN_DEFINE_METHOD(MoaError, GetAgentInfoMapSLOW, (Args*, Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetAgentHiddenReaderSet, (Media::DirectorMedia*))
	EXTERN_DEFINE_METHOD(MoaError, GetFormatName, (MixFormat, std::string*))
	EXTERN_DEFINE_METHOD(MoaError, GetAgentInfoHidden, (Agent::HIDDEN_READER_SET*, MoaClassID*, MoaBool*, PIMoaMixAgentInfo))
	EXTERN_DEFINE_METHOD(MoaError, GetAgentInfoName, (std::string*, PIMoaMixAgentInfo))
EXTERN_END_DEFINE_CLASS_INTERFACE

#define XTRA_VERSION_NUMBER XTRA_CLASS_VERSION

BEGIN_XTRA
BEGIN_XTRA_DEFINES_CLASS(TStdXtra, XTRA_CLASS_VERSION)
CLASS_DEFINES_INTERFACE(TStdXtra, IMoaRegister, XTRA_VERSION_NUMBER)
CLASS_DEFINES_INTERFACE(TStdXtra, IMoaMmXScript, XTRA_VERSION_NUMBER)
END_XTRA_DEFINES_CLASS
#ifdef MACINTOSH
BEGIN_XTRA_DEFINES_CLASS(CMacPICTAgent, XTRA_CLASS_VERSION)
CLASS_DEFINES_INTERFACE(CMacPICTAgent, IMoaRegister, XTRA_VERSION_NUMBER)
CLASS_DEFINES_INTERFACE(CMacPICTAgent, IMoaReader, XTRA_VERSION_NUMBER)
END_XTRA_DEFINES_CLASS
#endif
#ifdef WINDOWS
BEGIN_XTRA_DEFINES_CLASS(CWinBMPAgent, XTRA_CLASS_VERSION)
CLASS_DEFINES_INTERFACE(CWinBMPAgent, IMoaRegister, XTRA_VERSION_NUMBER)
CLASS_DEFINES_INTERFACE(CWinBMPAgent, IMoaReader, XTRA_VERSION_NUMBER)
END_XTRA_DEFINES_CLASS
#endif
END_XTRA

#endif