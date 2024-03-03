/*
ADOBE SYSTEMS INCORPORATED
Copyright 1994 - 2008 Adobe Macromedia Software LLC
All Rights Reserved

NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the
terms of the Adobe license agreement accompanying it.  If you have received this file from a
source other than Adobe, then your use, modification, or distribution of it requires the prior
written permission of Adobe.
*/

#ifndef _H_WinBMPAgent
#define _H_WinBMPAgent

#include "shared.h"
#include "Agent.h"
#include "Media.h"

#include "mixsvc.h"
#include "mixpix.h"

#ifdef WINDOWS
/*
Although Director includes a BMP Agent Xtra, it turns out to
be nothing but a placeholder, which always returns kMoaErr_BadParam.
This Agent reads the BMP format, in a Windows-specific fashion, by using GDI
(but it doesn't need to be crossplatform: GetMedia on Mac uses PICT instead)
Having this allows us access to the Writers for the Image label.
This Agent is marked hidden so it won't compete
with Director's built-in BMP reading behaviour.
*/

EXTERN_BEGIN_DEFINE_CLASS_INSTANCE_VARS(CWinBMPAgent)
	PIMoaFormatServices formatServicesInterfacePointer = NULL;
	PIMoaDataObjectServices dataObjectServicesInterfacePointer = NULL;

	PIMoaDataObject sourceDataObjectInterfacePointer = NULL;

	static const MoaLong RECEPTORS_SIZE = 1;
	static const MoaInterfaceID RECEPTORS[RECEPTORS_SIZE];

	static const MoaLong RECEPTOR_IID_I_MOA_RECEPTOR_PIXELS_INDEX = 0;
EXTERN_END_DEFINE_CLASS_INSTANCE_VARS

EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(CWinBMPAgent, IMoaRegister)
	EXTERN_DEFINE_METHOD(MoaError, Register, (PIMoaCache, PIMoaXtraEntryDict))
EXTERN_END_DEFINE_CLASS_INTERFACE

EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(CWinBMPAgent, IMoaAgent)
EXTERN_END_DEFINE_CLASS_INTERFACE

EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(CWinBMPAgent, IMoaReader)
	EXTERN_DEFINE_METHOD(MoaError, CountUnderstoodReceptorIDs, (MoaLong*))
	EXTERN_DEFINE_METHOD(MoaError, GetNthUnderstoodReceptorID, (MoaLong, MoaInterfaceID*))
	EXTERN_DEFINE_METHOD(MoaError, SetReaderDataSource, (PIMoaDataObject))
	EXTERN_DEFINE_METHOD(MoaError, SetReaderOptions, (PIMoaDict))
	EXTERN_DEFINE_METHOD(MoaError, GetReaderOptions, (PIMoaDict*))
	EXTERN_DEFINE_METHOD(MoaError, GetDefaultReaderOptions, (PIMoaDict*))
	EXTERN_DEFINE_METHOD(MoaError, GetReaderOptionsFromUser, (SysWindow, PIMoaDict*))
	EXTERN_DEFINE_METHOD(MoaError, CanRead, (ConstPMoaStorageMedium, MoaFileType))
	EXTERN_DEFINE_METHOD(MoaError, CountUserSelectors, (MoaLong*))
	EXTERN_DEFINE_METHOD(MoaError, GetNthUserSelector, (MoaLong, PMoaChar, PIMoaDict*))
	EXTERN_DEFINE_METHOD(MoaError, SetCurrentSelector, (PIMoaDict))
	EXTERN_DEFINE_METHOD(MoaError, Read, (PIMoaReceptor, ConstPMoaInterfaceID))
	private:
	/* Read */
	// Pixels
	EXTERN_DEFINE_METHOD(MoaError, ReadPixels, (PIMoaReceptorPixels))
	EXTERN_DEFINE_METHOD(MoaError, ReadPixels, (PIMoaReceptorPixels, PIMoaStream))
EXTERN_END_DEFINE_CLASS_INTERFACE
#endif

#endif