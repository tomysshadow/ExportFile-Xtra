#pragma once
#include "shared.h"
#include "Path.h"
#include <vector>
#include <map>
#include <unordered_set>

#include "mixsvc.h"

#define kMoaCfFormatName_CLUT "kMoaCfFormat_CLUT"
#define kMoaCfFormatName_WinBMP "kMoaCfFormat_WinBMP"
#define kMoaCfFormatName_Palette "kMoaCfFormat_Palette"
#define kMoaCfFormatName_3D "kMoaCfFormat_3D"
//#define kMoaCfFormatName_MPEG4 "kMoaCfFormat_MPEG4"

// {029E9D30-849C-11D0-B7FB-00A0C9086956}
//DEFINE_GUID(CLSID_CPICTAgent,
	//0x029E9D30, 0x849C, 0x11D0, 0xB7, 0xFB, 0x00, 0xA0, 0xC9, 0x08, 0x69, 0x56);

// {54414A60-4F99-11D0-80EA-00A0C9086956}
DEFINE_GUID(CLSID_CSndAgent,
	0x54414A60, 0x4F99, 0x11D0, 0x80, 0xEA, 0x00, 0xA0, 0xC9, 0x08, 0x69, 0x56);

// {F3C64E50-F6DD-11D0-A5BA-00059A80C487}
DEFINE_GUID(CLSID_CCLUTAgent,
	0xF3C64E50, 0xF6DD, 0x11D0, 0xA5, 0xBA, 0x00, 0x05, 0x9A, 0x80, 0xC4, 0x87);

// {46F1135A-AC43-11D2-ABA7-06063F000000}
//DEFINE_GUID(CLSID_CBMPAgent,
	//0x46F1135A, 0xAC43, 0x11D2, 0xAB, 0xA7, 0x06, 0x06, 0x3F, 0x00, 0x00, 0x00);

// {5187C88E-8189-4F64-AD6F-1AACF19E8486}
DEFINE_GUID(CLSID_CWinBMPAgent,
	0x5187c88e, 0x8189, 0x4f64, 0xad, 0x6f, 0x1a, 0xac, 0xf1, 0x9e, 0x84, 0x86);

// {6A5293A2-0810-11D0-B28A-000502E85810}
DEFINE_GUID(CLSID_CWAVEAgent,
	0x6A5293A2, 0x0810, 0x11D0, 0xB2, 0x8A, 0x00, 0x05, 0x02, 0xE8, 0x58, 0x10);

// {BBD58900-3CC2-11D0-A183-444553540000}
DEFINE_GUID(CLSID_CPaletteAgent,
	0xBBD58900, 0x3CC2, 0x11D0, 0xA1, 0x83, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);

// {75A88B0C-8902-11D6-B758-00039354AA6A}
DEFINE_GUID(CLSID_CFlashAgent,
	0x75A88B0C, 0x8902, 0x11D6, 0xB7, 0x58, 0x00, 0x03, 0x93, 0x54, 0xAA, 0x6A);

// {13CA95DC-9086-11D3-8889-0090277204FA}
//DEFINE_GUID(CLSID_CGIFAgent,
	//0x13CA95DC, 0x9086, 0x11D3, 0x88, 0x89, 0x00, 0x90, 0x27, 0x72, 0x04, 0xFA);

// {04F8CB95-168D-4D84-8212-E846299B46FF}
//DEFINE_GUID(CLSID_CMPEG4Agent,
	//0x04F8CB95, 0x168D, 0x4D84, 0x82, 0x12, 0xE8, 0x46, 0x29, 0x9B, 0x46, 0xFF);

// {9A60E1E1-5CA5-11D3-AE51-009027250B86}
DEFINE_GUID(CLSID_CScriptAgent,
	0x9A60E1E1, 0x5CA5, 0x11D3, 0xAE, 0x51, 0x00, 0x90, 0x27, 0x25, 0x0B, 0x86);

namespace Agent {
	typedef std::string STRING;

	typedef std::unordered_set<MoaClassID> HIDDEN_READER_SET;

	class Info {
		public:
		typedef std::map<std::string, Info, IgnoreCaseComparer> MAP;

		class Writer {
			private:
			void destroy();
			void duplicate(const Writer &writer);

			PIMoaWriter writerInterfacePointer = NULL;

			public:
			Writer();
			~Writer();
			Writer(const Writer &writer);
			Writer &operator=(const Writer &writer);
			PIMoaWriter getWriterInterfacePointer() const;
			void setWriterInterfacePointer(PIMoaWriter writerInterfacePointer);

			MoaClassID classID = IID_NULL;
		};

		typedef std::vector<Writer> WRITER_VECTOR;

		std::string name = "";
		Path::EXTENSION_MAPPED_VECTOR pathExtensions = {};
		MoaBool hidden = FALSE;
		WRITER_VECTOR writerVector = {};
	};
};