#pragma once
#include "shared.h"
#include "Label.h"
#include <map>

namespace TypeLabel {
	typedef std::map<SYMBOL_VARIANT, Label::MAPPED_VECTOR> MAP;

	class TypeLabels {
		private:
		MAP typeLabelMap = {
			{"Text", {"RTF", "HTML", "Text", "Image", "PICT"}},
			{"RichText", {"RTF", "HTML", "Text", "Image", "PICT"}},
			{"Field", {"Text", "TextStyles", "PICT", "Image"}},
			{"Button", {"Text", "TextStyles"}},
			{"Picture", {"PICT", "Image"}},
			{"Bitmap", {"Image", "PICT"}},
			{"OLE", {"Image", "PICT"}},
			{"Sound", {"Sound"}},
			{"Palette", {"Palette"}},
			{"FilmLoop", {"Score"}},
			//{"Movie", {"Score"}},
			{"Flash", {"SWF"}},
			{"FlashComponent", {"SWF"}},
			{"VectorShape", {"SWF"}},
			{"Shockwave3D", {"W3D", "Image", "PICT"}},
			{"AnimGif", {"GIF"}},
			{"Font", {"PFR"}},
			{"Mixer", {"WAVAsync", "MP4Async"}}
		};

		MoaError getTypeLabelMapSymbols(PIMoaMmValue mmValueInterfacePointer);

		public:
		TypeLabels(PIMoaMmValue mmValueInterfacePointer);
		const MAP &get() const;
	};
};