#pragma once
#include "utils.h"
#include "Label.h"
#include <map>

namespace TypeLabel {
	using Map = std::map<SymbolVariant, Label::MappedVector>;

	class TypeLabels {
		private:
		Map typeLabelMap = {
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
		const Map &get() const;
	};
};