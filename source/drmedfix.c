/*
ADOBE SYSTEMS INCORPORATED
Copyright 1994 - 2007 Adobe Macromedia Software LLC
All Rights Reserved

NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the 
terms of the Adobe license agreement accompanying it.  If you have received this file from a 
source other than Adobe, then your use, modification, or distribution of it requires the prior 
written permission of Adobe.
*/

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

#include "drmedfix.h"

//#ifdef WINDOWS
#ifdef MOA_DEBUG
#include <stddef.h>		/* for offsetof() */
#endif

/* ------------------------------------------------------------------------ */
// Reverse the bytes in a long
long  mem_XLong(long x)
{
	MoaByte temp;
	union
	{
		long oneDWord;
		MoaByte fourBytes[4];
	} Moto;
	Moto.oneDWord = x;
	/* swap two end bytes */
	temp = Moto.fourBytes[3];
	Moto.fourBytes[3] = Moto.fourBytes[0];
	Moto.fourBytes[0] = temp;
	/* now swap the middle bytes */
	temp = Moto.fourBytes[2];
	Moto.fourBytes[2] = Moto.fourBytes[1];
	Moto.fourBytes[1] = temp;
	return (Moto.oneDWord);
}

/* ------------------------------------------------------------------------ */
// Swap the bytes of a short
short  mem_XShort(short x)
{
	MoaByte temp;
	union
	{
		short oneWord;
		MoaByte twoBytes[2];
	} Moto;
	Moto.oneWord = x;
	/* swap two end bytes */
	temp = Moto.twoBytes[1];
	Moto.twoBytes[1] = Moto.twoBytes[0];
	Moto.twoBytes[0] = temp;
	return (Moto.oneWord);
}
//#endif /* WINDOWS */

#ifdef WINDOWS
/*
 * Flip the media header.
 * But we may fix the Director bug that Director doesn't flip.
 * Either both Director and this code need to check if already
 * flipped, or this code has to write an indicator bit, or ???
 *
 */
void DrFixMediaHeader(const char * pMediaData, MoaUlong size)
{
	if (!pMediaData) {
		return;
	}

	MediaDataHdr *pMDHdr = (MediaDataHdr *)pMediaData;
	/* Should be 26 bytes between start of last (4-byte) field and start of first */

	MOA_ASSERT((offsetof(MediaDataHdrDirector5, propLength) == 26), "DrMedFix, sizeof(MediaDataHdrDirector5) seems wrong!\n");
	MOA_ASSERT((offsetof(MediaDataHdrDirector7, propLength) == 24), "DrMedFix, sizeof(MediaDataHdrDirector7) seems wrong!\n");

	const MoaUlong HDR_SIZE_OFFSET_SIZE = offsetof(MediaDataHdrDirector5, hdrSize) + sizeof(pMDHdr->director5.hdrSize);

	if (size < HDR_SIZE_OFFSET_SIZE) {
		return;
	}

	/* When you GetMedia, if the version has already been flipped, 
	 * the version field will be really big, so don't do work.
	 * 
	 * But on AttachMedia, there's no way to know if the media has been flipped or not.
	 * ??? What if we fix this in a later version??
	 */
	if (pMDHdr->director5.hdrSize == XHDR_SIZE_DIRECTOR5 || pMDHdr->director5.hdrSize == HDR_SIZE_DIRECTOR7) {
		return;
	}

	const MoaUlong DIRECTOR5_SIZE = sizeof(pMDHdr->director5);

	if (size < DIRECTOR5_SIZE) {
		return;
	}

	pMDHdr->director5.mediaHdlInfo1	= mem_XLong(pMDHdr->director5.mediaHdlInfo1);
	pMDHdr->director5.mediaHdlInfo2	= mem_XLong(pMDHdr->director5.mediaHdlInfo2);
	pMDHdr->director5.hdrSize			= mem_XShort(pMDHdr->director5.hdrSize);
	pMDHdr->director5.version			= mem_XLong(pMDHdr->director5.version);
	pMDHdr->director5.cmType			= mem_XLong(pMDHdr->director5.cmType);
	pMDHdr->director5.propLength		= mem_XLong(pMDHdr->director5.propLength);
}
#endif
