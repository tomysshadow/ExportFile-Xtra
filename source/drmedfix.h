/*
ADOBE SYSTEMS INCORPORATED
Copyright 1994 - 2007 Adobe Macromedia Software LLC
All Rights Reserved

NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the 
terms of the Adobe license agreement accompanying it.  If you have received this file from a 
source other than Adobe, then your use, modification, or distribution of it requires the prior 
written permission of Adobe.
*/

#ifndef _H_drmedfix
#define _H_drmedfix

#include <stdint.h>

#ifndef _H_mmtypes
#include "mmtypes.h"
#endif

#ifndef _H_moaxtra
#include "moaxtra.h"
#endif

const short HDR_SIZE_DIRECTOR5 = 0x001E;
const short XHDR_SIZE_DIRECTOR5 = 0x1E00;
const short HDR_SIZE_DIRECTOR7 = 0x0000;

/****************************************************************************
 *
 * If you write the mediaData handle for #moaComposite passed back by 
 * IMoaDrCastMem::GetMedia() to a file, you will find at the start of the 
 * byte stream in Director 5:
 *  - 8 bytes of internal info about the media handle that appears to be irrelevant
 *  - two 32-bit numbers about the media handle that are _not_ byte swapped; 
 *    these are related to the handle size.
 *  - a 16-bit header size that is _not_ byte swapped
 *  - a 32-bit version number that is _not_ byte-swapped
 *  - a 32-bit internal cast member type number that is _not_ byte-swapped
 *  - a 32-bit properties length number that is _not_ byte-swapped 
 * This comes to 30 bytes (0x001e), which is the header size value.
 ****************************************************************************/ 
/*
 * Set up packing to force the 30-byte alignment.
 * Would be better to define this as bytes, but the mem_XLong() and mem_XShort()
 * routines from Goodies\CodeFrag take long and short as parameters, not bytes.
 */
#pragma pack(push, 2)
typedef struct MediaDataHdrDirector5 {
	unsigned char	dontCare[8];
	long			mediaHdlInfo1;
	long			mediaHdlInfo2;
	short			hdrSize;
	long			version;
	long			cmType;
	long			propLength;
} MediaDataHdrDirector5;

typedef struct MediaDataHdrDirector7 {
	uint8_t			zero[20];
	int32_t			cmType;
	uint32_t		propLength;
} MediaDataHdrDirector7;

typedef union MediaDataHdr {
	MediaDataHdrDirector5 director5;
	MediaDataHdrDirector7 director7;
} MediaDataHdr;
#pragma pack(pop)

long  mem_XLong(long x);
short  mem_XShort(short x);

#ifdef WINDOWS
void DrFixMediaHeader(const char * pMediaData, MoaUlong size);
#endif

#endif