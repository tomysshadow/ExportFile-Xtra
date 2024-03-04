/*
ADOBE SYSTEMS INCORPORATED
Copyright 1994 - 2008 Adobe Macromedia Software LLC
All Rights Reserved

NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the
terms of the Adobe license agreement accompanying it.  If you have received this file from a
source other than Adobe, then your use, modification, or distribution of it requires the prior
written permission of Adobe.
*/

#include "WinBMPAgent.h"
#include <stdlib.h>
#include <math.h>

#include "mixsvc.h"
#include "moatry.h"

#ifdef WINDOWS
STDMETHODIMP_(MoaError) MoaCreate_CWinBMPAgent(CWinBMPAgent* This) {
	moa_try

	ThrowNull(This);

	MoaError err = GetIMoaFormatServices(This->pCallback, &This->formatServicesInterfacePointer);

	if (err != kMoaErr_NoErr) {
		This->formatServicesInterfacePointer = NULL;
	}

	err = GetIMoaDataObjectServices(This->pCallback, &This->dataObjectServicesInterfacePointer);

	if (err != kMoaErr_NoErr) {
		This->dataObjectServicesInterfacePointer = NULL;
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STDMETHODIMP_(void) MoaDestroy_CWinBMPAgent(CWinBMPAgent* This) {
	moa_try

	ThrowNull(This);

	releaseInterface((PPMoaVoid)&This->formatServicesInterfacePointer);
	releaseInterface((PPMoaVoid)&This->dataObjectServicesInterfacePointer);

	releaseInterface((PPMoaVoid)&This->sourceDataObjectInterfacePointer);

	moa_catch
	moa_catch_end
	moa_try_end_void
}

STD_INTERFACE_CREATE_DESTROY(CWinBMPAgent, IMoaRegister)

BEGIN_DEFINE_CLASS_INTERFACE(CWinBMPAgent, IMoaRegister)
END_DEFINE_CLASS_INTERFACE

STDMETHODIMP CWinBMPAgent_IMoaRegister::Register(PIMoaCache cacheInterfacePointer, PIMoaXtraEntryDict xtraEntryDictInterfacePointer) {
	moa_try

	ThrowNull(cacheInterfacePointer);
	ThrowNull(xtraEntryDictInterfacePointer);

	{
		// this interface should NOT be released
		PIMoaRegistryEntryDict registryEntryDictInterfacePointer = NULL;
		ThrowErr(cacheInterfacePointer->AddRegistryEntry(xtraEntryDictInterfacePointer, &CLSID_CWinBMPAgent, &IID_IMoaAgent, &registryEntryDictInterfacePointer));
		ThrowNull(registryEntryDictInterfacePointer);

		const MoaLong NAME_SIZE = 7;
		MoaChar name[NAME_SIZE] = "WinBMP";

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_CString, name, NAME_SIZE, kAgentRegKey_Name));

		// IMPORTANT: this should NOT be kMoaCfFormatName_BMP
		// (it interferes with Director's default BMP reading behaviour otherwise)
		const MoaLong FORMAT_SIZE = sizeof(kMoaCfFormatName_WinBMP);

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_CString, kMoaCfFormatName_WinBMP, FORMAT_SIZE, kAgentRegKey_Format));

		const MoaLong FILE_CREATOR_SIZE = 5;
		MoaChar fileCreator[FILE_CREATOR_SIZE] = "????";

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_CString, fileCreator, FILE_CREATOR_SIZE, kAgentRegKey_FileCreator));

		const MoaLong TYPES_SIZE = 5;
		MoaChar types[TYPES_SIZE] = "WBMP";

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_CString, types, TYPES_SIZE, kAgentRegKey_FileTypes));
		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_CString, types, TYPES_SIZE, kAgentRegKey_MacScrapTypes));

		// importFileInto in projectors ignores the Hidden key, so
		// this must be assigned some other extension to never be used
		const MoaLong FILE_EXTS_SIZE = 9;
		MoaChar fileExts[FILE_EXTS_SIZE] = "*.winbmp";

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_CString, fileExts, FILE_EXTS_SIZE, kAgentRegKey_FileExts));

		/*
		const MoaLong MIME_TYPE_SIZE = 10;
		MoaChar mimeType[MIME_TYPE_SIZE] = "image/bmp";

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_CString, mimeType, MIME_TYPE_SIZE, kAgentRegKey_MimeType));
		*/

		registryEntryDictInterfacePointer = NULL;
		ThrowErr(cacheInterfacePointer->AddRegistryEntry(xtraEntryDictInterfacePointer, &CLSID_CWinBMPAgent, &IID_IMoaReader, &registryEntryDictInterfacePointer));
		ThrowNull(registryEntryDictInterfacePointer);

		MoaBool hasUI = FALSE;
		const MoaLong HAS_UI_SIZE = sizeof(hasUI);

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_Bool, &hasUI, HAS_UI_SIZE, kReaderRegKey_HasUI));

		// hidden so we don't compete with Director's default BMP reading behaviour
		MoaBool hidden = TRUE;
		const MoaLong HIDDEN_SIZE = sizeof(hidden);

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_Bool, &hidden, HIDDEN_SIZE, kReaderRegKey_Hidden));

		const MoaLong IID_I_MOA_RECEPTOR_PIXELS_SIZE = sizeof(IID_IMoaReceptorPixels);

		ThrowErr(registryEntryDictInterfacePointer->Put(kMoaDictType_Bytes, &IID_IMoaReceptorPixels, IID_I_MOA_RECEPTOR_PIXELS_SIZE, kReaderRegKey_UnderstoodReceptors));
	}

	moa_catch
	moa_catch_end
	moa_try_end
}

STD_INTERFACE_CREATE_DESTROY(CWinBMPAgent, IMoaReader)

BEGIN_DEFINE_CLASS_INTERFACE(CWinBMPAgent, IMoaReader)
END_DEFINE_CLASS_INTERFACE

const MoaInterfaceID CWinBMPAgent::RECEPTORS[RECEPTORS_SIZE] = { IID_IMoaReceptorPixels };

MoaError CWinBMPAgent_IMoaReader::CountUnderstoodReceptorIDs(MoaLong* pNumReceptors) {
	moa_try

	ThrowNull(pNumReceptors);

	*pNumReceptors = pObj->RECEPTORS_SIZE;

	moa_catch

	if (pNumReceptors) {
		*pNumReceptors = 0;
	}

	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::GetNthUnderstoodReceptorID(MoaLong index, MoaInterfaceID* ppReceptorIID) {
	moa_try

	ThrowNull(ppReceptorIID);

	if (index == pObj->RECEPTOR_IID_I_MOA_RECEPTOR_PIXELS_INDEX) {
		*ppReceptorIID = IID_IMoaReceptorPixels;
	} else {
		Throw(kMoaErr_BadParam);
	}

	moa_catch

	if (ppReceptorIID) {
		*ppReceptorIID = IID_NULL;
	}

	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::SetReaderDataSource(PIMoaDataObject pSourceData) {
	moa_try

	// may be NULL
	//ThrowNull(pSourceData);

	if (!pSourceData) {
		Throw(kMoaMixErr_BadDataSourceType);
	}

	setInterface((PPMoaVoid)&pObj->sourceDataObjectInterfacePointer, pSourceData);

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::SetReaderOptions(PIMoaDict pOptionsDict) {
	moa_try

	// may be null
	//ThrowNull(pOptionsDict);

	// nothing

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::GetReaderOptions(PIMoaDict* ppOptionsDict) {
	moa_try

	ThrowNull(ppOptionsDict);

	Throw(GetDefaultReaderOptions(ppOptionsDict));

	moa_catch

	if (ppOptionsDict) {
		*ppOptionsDict = NULL;
	}

	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::GetDefaultReaderOptions(PIMoaDict* ppOptionsDict) {
	moa_try

	ThrowNull(ppOptionsDict);

	*ppOptionsDict = NULL;

	moa_catch

	if (ppOptionsDict) {
		*ppOptionsDict = NULL;
	}

	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::GetReaderOptionsFromUser(SysWindow pParentWindow, PIMoaDict* ppOptionsDict) {
	moa_try

	ThrowNull(pParentWindow);
	ThrowNull(ppOptionsDict);

	// Reader has no UI
	Throw(kMoaErr_BadParam);

	moa_catch

	if (ppOptionsDict) {
		*ppOptionsDict = NULL;
	}

	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::CanRead(ConstPMoaStorageMedium pStorage, MoaFileType fileType) {
	PIMoaStream streamInterfacePointer = NULL;

	moa_try

	ThrowNull(pStorage);

	if (!pObj->dataObjectServicesInterfacePointer) {
		Throw(kMoaErr_BadClass);
	}

	ThrowErr(pObj->dataObjectServicesInterfacePointer->StorageMediumToMoaStream(pStorage, &streamInterfacePointer));
	ThrowNull(streamInterfacePointer);

	MoaError err = openStream(kMoaStreamOpenAccess_ReadOnly, true, streamInterfacePointer);

	bool streamAlreadyOpen = false;

	if (err == kMoaStreamErr_StreamAlreadyOpen) {
		streamAlreadyOpen = true;

		err = kMoaErr_NoErr;
	}

	ThrowErr(err);

	err = ReadPixels(NULL, streamInterfacePointer);

	ThrowErr(streamInterfacePointer->SetPosition(0));

	if (!streamAlreadyOpen) {
		// if the stream was closed before, re-close it (explicitly)
		ThrowErr(streamInterfacePointer->Close());
	}

	Throw(err);

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&streamInterfacePointer);
	
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::CountUserSelectors(MoaLong* pNumSelectors) {
	moa_try

	ThrowNull(pNumSelectors);

	Throw(kMoaErr_BadParam);

	moa_catch

	if (pNumSelectors) {
		*pNumSelectors = 0;
	}

	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::GetNthUserSelector(MoaLong index, PMoaChar pSelectorName, PIMoaDict* ppSelectorData) {
	moa_try

	ThrowNull(pSelectorName);
	ThrowNull(ppSelectorData);

	Throw(kMoaErr_BadParam);

	moa_catch

	if (ppSelectorData) {
		*ppSelectorData = NULL;
	}

	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::SetCurrentSelector(PIMoaDict pSelectorData) {
	moa_try

	// may be null
	//ThrowNull(pSelectorData);

	Throw(kMoaErr_BadParam);

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::Read(PIMoaReceptor pReceptor, ConstPMoaInterfaceID pReceptorID) {
	moa_try

	ThrowNull(pReceptor);
	ThrowNull(pReceptorID);

	bool equalID = MoaEqualID(pReceptorID, &pObj->RECEPTORS[pObj->RECEPTOR_IID_I_MOA_RECEPTOR_PIXELS_INDEX]);

	if (equalID) {
		// this interface should NOT be released
		PIMoaReceptorPixels receptorPixelsInterfacePointer = (PIMoaReceptorPixels)pReceptor;
		ThrowNull(receptorPixelsInterfacePointer);

		Throw(ReadPixels(receptorPixelsInterfacePointer));
	}

	Throw(kMoaMixErr_NotMyType);

	moa_catch
	moa_catch_end
	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::ReadPixels(PIMoaReceptorPixels receptorPixelsInterfacePointer) {
	PIMoaStream streamInterfacePointer = NULL;

	MoaFormatEtc formatEtc = {};

	moa_try

	ThrowNull(receptorPixelsInterfacePointer);

	if (!pObj->sourceDataObjectInterfacePointer) {
		Throw(kMoaMixErr_BadDataSourceType);
	}

	if (!pObj->formatServicesInterfacePointer) {
		Throw(kMoaErr_BadClass);
	}

	// format etc.
	MixFormat mixFormat = 0;
	ThrowErr(pObj->formatServicesInterfacePointer->RegisterFormat(kMoaCfFormatName_WinBMP, &mixFormat));

	SetMoaFormatEtc(&formatEtc, mixFormat, kMoaStorageMediumType_HGlobal);

	if (!pObj->dataObjectServicesInterfacePointer) {
		Throw(kMoaErr_BadClass);
	}

	ThrowErr(pObj->dataObjectServicesInterfacePointer->DataObjectToMoaStream(pObj->sourceDataObjectInterfacePointer, &formatEtc, &streamInterfacePointer));
	ThrowNull(streamInterfacePointer);

	MoaError err = openStream(kMoaStreamOpenAccess_ReadOnly, true, streamInterfacePointer);

	// IMPORTANT UNDOCUMENTED BEHAVIOUR
	// if the stream was already open, writers expect the reader to leave it open
	// otherwise, the reader is expected to close it when it's done
	bool streamAlreadyOpen = false;

	if (err == kMoaStreamErr_StreamAlreadyOpen) {
		streamAlreadyOpen = true;

		err = kMoaErr_NoErr;
	}

	ThrowErr(err);

	err = ReadPixels(receptorPixelsInterfacePointer, streamInterfacePointer);

	// politely reset the stream for the next guy
	// (docs say readers shouldn't assume the position is zero, but eh)
	ThrowErr(streamInterfacePointer->SetPosition(0));

	if (!streamAlreadyOpen) {
		// if the stream was closed before, re-close it (explicitly)
		ThrowErr(streamInterfacePointer->Close());
	}

	ThrowErr(err);

	moa_catch
	moa_catch_end

	releaseInterface((PPMoaVoid)&streamInterfacePointer);

	moa_try_end
}

MoaError CWinBMPAgent_IMoaReader::ReadPixels(PIMoaReceptorPixels receptorPixelsInterfacePointer, PIMoaStream readStreamInterfacePointer) {
	Media::WinBMPMedia winBMPMedia;

	moa_try

	// NULL is valid
	//ThrowNull(receptorPixelsInterfacePointer);
	ThrowNull(readStreamInterfacePointer);

	// this also handles CanRead
	ThrowErr(winBMPMedia.getMappedView(receptorPixelsInterfacePointer, readStreamInterfacePointer));

	if (receptorPixelsInterfacePointer) {
		// get the pixel format if we don't already have it
		// (we should theoretically already have it, but I don't want to directly couple this to stretchSourceBitmap's internal logic)
		ThrowErr(winBMPMedia.getPixelFormat(receptorPixelsInterfacePointer));

		if (!winBMPMedia.pixelFormatOptional.has_value()) {
			Throw(kMoaErr_InternalError);
		}

		MoaPixelFormat &pixelFormat = winBMPMedia.pixelFormatOptional.value();

		// you like this format?
		ThrowErr(receptorPixelsInterfacePointer->BeginPixels(&pixelFormat));

		if (!winBMPMedia.mappedView) {
			Throw(kMoaErr_InternalError);
		}

		LPBYTE bytesPointer = (LPBYTE)winBMPMedia.mappedView;
		ThrowNull(bytesPointer);

		// a loop that can go either direction
		// be super careful here: access violations abound if you're off by one with the top/bottom row
		bool bottomUp = winBMPMedia.direction == BOTTOM_UP;

		MoaLong endRow = pixelFormat.dim.pixels.y;
		MoaLong nextRow = bottomUp ? -1 : 1;
		MoaLong topRow = bottomUp * endRow - bottomUp;
		MoaLong bottomRow = !bottomUp * endRow - bottomUp;

		for (MoaLong i = topRow; i != bottomRow; i += nextRow) {
			ThrowErr(receptorPixelsInterfacePointer->SetPixels(i, bytesPointer, pixelFormat.dim.rowBytes));

			bytesPointer += (SIZE_T)pixelFormat.dim.rowBytes;
			ThrowNull(bytesPointer);
		}

		ThrowErr(receptorPixelsInterfacePointer->EndPixels());
	}

	moa_catch
	moa_catch_end
	moa_try_end
}
#endif