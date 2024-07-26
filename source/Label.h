#pragma once
#include "shared.h"
#include "Path.h"
#include "Agent.h"
#include <map>
#include <variant>

namespace Label {
	typedef ::SYMBOL_VARIANT SYMBOL_VARIANT;
	typedef MappedVector<SYMBOL_VARIANT> MAPPED_VECTOR;

	typedef unsigned int TYPE;

	static const TYPE TYPE_MEDIA_INFO = 0x00000001;
	static const TYPE TYPE_MEMBER_PROPERTY = 0x00000002;
	static const TYPE TYPE_XTRA_MEDIA = 0x00000004;

	static const TYPE TYPE_MEDIA_INFO_GLOBAL_HANDLE = 0x00000100;
	static const TYPE TYPE_MEDIA_INFO_COMPOSITE = 0x00000200;
	#ifdef MACINTOSH
	// reserved in case the Mac labels need their own types
	// because I legitimately don't know if they can use the Global Handle Format or
	// will need their own Format classes
	static const TYPE TYPE_MEDIA_INFO_MAC_RESERVED = 0x00000400; // reserved in case Mac #image label needs its own type
	static const TYPE TYPE_MEDIA_INFO_MAC_RESERVED2 = 0x00000800; // reserved in case Mac #sound label needs its own type
	static const TYPE TYPE_MEDIA_INFO_MAC_RESERVED3 = 0x00001000; // reserved in case Mac #palette label needs its own type
	#endif
	#ifdef WINDOWS
	static const TYPE TYPE_MEDIA_INFO_WIN_DIB = 0x00002000;
	static const TYPE TYPE_MEDIA_INFO_WIN_PALETTE = 0x00004000;
	#endif

	static const TYPE TYPE_MEMBER_PROPERTY_PICTURE = 0x00000100;

	static const TYPE TYPE_XTRA_MEDIA_SWF = 0x00000100;
	static const TYPE TYPE_XTRA_MEDIA_W3D = 0x00000200;
	static const TYPE TYPE_XTRA_MEDIA_MIXER_ASYNC = 0x00000400;

	static const TYPE TYPE_XTRA_MEDIA_MIXER_WAV_ASYNC = 0x00010000;
	static const TYPE TYPE_XTRA_MEDIA_MIXER_MP4_ASYNC = 0x00020000;

	typedef std::map<MoaMmSymbol, Agent::Info::MAP> AGENT_INFO_MAP;

	struct Info {
		typedef std::map<SYMBOL_VARIANT, Info> MAP;

		std::string name = "";

		Path::EXTENSION_MAPPED_VECTOR pathExtensions = {};

		TYPE labelType = TYPE_MEDIA_INFO;
		::SYMBOL_VARIANT labelFormat = "";

		std::string agentFormatName = "";
		MoaClassID agentReaderClassID = {};
	};

	namespace Labels {
		class Info {
			private:
			/*
			currently, the reader class IDs are hardcoded here
			in theory, I'd like ExportFile have have its own readers integrated
			for every type of writer it'll interact with
			in practice, this may not be practical. If someone writes a Text Agent Xtra
			they might be frustrated they can't use their own reader with ExportFile
			I feel like this should not be integrated into the Options, but rather
			if they are to be customized, there should be a seperate
			setExportFileReaderClassIDs handler (and corresponding alternate syntax, callSet)
			as well as a getExportFileReaderClassIDs, which access the reader class IDs globally
			the setExportFileReaderClassIDs handler would get the labelInfoMap and
			put the custom GUIDs passed to the handler into the map, then set the map back
			we'd also need to store this in the MoaCache as a runtime value
			so that it doesn't get unloaded
			because this is an ordered map, we could iterate it, saving the array of GUIDs
			to a Bytes value in the MoaCache dictionary, to later retrieve
			it MAY also be nice to set reader options, so maybe focusing on the class IDs is bad
			we could have a more generic setExportFileReader handler, taking the
			agent options and classID in as arguments or a property list argument
			but for now, this is a problem for future me - because as of writing, this
			is a non-issue. Nobody has made their own custom reader Xtras for ExportFile
			so it'll probably be better to design for this when the problem comes up
			and for the time being, YAGNI
			*/
			Label::Info::MAP labelInfoMap = {
				{"RTF", {"Rich Text Document", {"rtf"}, TYPE_MEMBER_PROPERTY, "RTF", "", IID_NULL}},
				{"HTML", {"HTML Document", {"htm", "html"}, TYPE_MEMBER_PROPERTY, "HTML", "", IID_NULL}},
				{"Text", {"Text Document", {"txt"}, TYPE_MEMBER_PROPERTY, "Text", kMoaCfFormatName_Text, IID_NULL}},
				{"TextStyles", {"Text Styles", {"stxt"}, TYPE_MEDIA_INFO, "MoaTEStyles", "", IID_NULL}},
				#ifdef MACINTOSH
				{"PICT", {"PICT", {"pic", "pct", "pict"}, TYPE_MEMBER_PROPERTY | TYPE_MEMBER_PROPERTY_PICTURE, "Picture", kMoaCfFormatName_PICT, IID_NULL}},
				{"Image", {"PICT", {"pic", "pct", "pict"}, TYPE_MEMBER_PROPERTY | TYPE_MEMBER_PROPERTY_PICTURE, "Picture", kMoaCfFormatName_PICT, IID_NULL}},
				{"Sound", {"snd Resource Sound", {"snd_"}, TYPE_MEDIA_INFO | TYPE_MEDIA_INFO_GLOBAL_HANDLE, "MacSnd", kMoaCfFormatName_snd, CLSID_CSndAgent}},
				{"Palette", {"Photoshop CLUT", {"act"}, TYPE_MEDIA_INFO | TYPE_MEDIA_INFO_GLOBAL_HANDLE, "MacColorTable", kMoaCfFormatName_CLUT, CLSID_CCLUTAgent}},
				#endif
				#ifdef WINDOWS
				{"PICT", {"PICT", {"pic", "pct", "pict"}, TYPE_MEMBER_PROPERTY | TYPE_MEMBER_PROPERTY_PICTURE, "Picture", kMoaCfFormatName_PICT, IID_NULL}},
				{"Image", {"BMP", {"bmp", "dib", "rle"}, TYPE_MEDIA_INFO | TYPE_MEDIA_INFO_WIN_DIB, "WinDIB", kMoaCfFormatName_WinBMP, CLSID_CWinBMPAgent}},
				{"Sound", {"WAVE Sound", {"wav", "wave"}, TYPE_MEDIA_INFO | TYPE_MEDIA_INFO_GLOBAL_HANDLE, "WinWAVE", kMoaCfFormatName_WAVE, CLSID_CWAVEAgent}},
				{"Palette", {"Microsoft Palette", {"pal"}, TYPE_MEDIA_INFO | TYPE_MEDIA_INFO_WIN_PALETTE, "WinPALETTE", kMoaCfFormatName_Palette, CLSID_CPaletteAgent}},
				#endif
				{"Score", {"VideoWorks Score", {"vwsc"}, TYPE_MEDIA_INFO, "MoaHandle", "", IID_NULL}},
				{"SWF", {"Flash", {"swf"}, TYPE_XTRA_MEDIA | TYPE_XTRA_MEDIA_SWF, "", kMoaCfFormatName_Flash, CLSID_CFlashAgent}},
				{"W3D", {"Shockwave 3D", {"w3d"}, TYPE_XTRA_MEDIA | TYPE_XTRA_MEDIA_W3D, "", kMoaCfFormatName_3D, IID_NULL}},
				{"GIF", {"Animated GIF", {"gif"}, TYPE_XTRA_MEDIA, "", kMoaCfFormatName_GIF, IID_NULL}},
				{"PFR", {"Font", {"pfr"}, TYPE_XTRA_MEDIA, "", "", IID_NULL}},
				{"XtraMedia", {"", {}, TYPE_XTRA_MEDIA, "", "", IID_NULL}},
				{"WAVAsync", {"WAV", {"wav", "wave"}, TYPE_XTRA_MEDIA | TYPE_XTRA_MEDIA_MIXER_ASYNC | TYPE_XTRA_MEDIA_MIXER_WAV_ASYNC, "", "", IID_NULL}},
				{"MP4Async", {"MP4", {"m4a", "mp4", "f4v"}, TYPE_XTRA_MEDIA | TYPE_XTRA_MEDIA_MIXER_ASYNC | TYPE_XTRA_MEDIA_MIXER_MP4_ASYNC, "", "", IID_NULL}},
				{"Lingo", {"Lingo", {"ls"}, TYPE_MEMBER_PROPERTY, "ScriptText", kMoaCfFormatName_Script, CLSID_CScriptAgent}},
				{"JavaScript", {"JavaScript", {"js"}, TYPE_MEMBER_PROPERTY, "ScriptText", kMoaCfFormatName_Script, CLSID_CScriptAgent}},
				{"Composite", {"Composite", {""}, TYPE_MEDIA_INFO | TYPE_MEDIA_INFO_COMPOSITE, "MoaHandle", "", IID_NULL}}
			};

			MoaError getLabelInfoMapSymbols(PIMoaMmValue mmValueInterfacePointer);

			public:
			Info(PIMoaMmValue mmValueInterfacePointer);
			const Label::Info::MAP &get() const;
		};
	}
};