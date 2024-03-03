#pragma once
#include "shared.h"
#include "Path.h"
#include "IconValues.h"
#include "BitmapImporter.h"
#include <map>
#include <optional>

#include "mmiimage.h"

namespace Asset {
	std::string& trimEllipsis(std::string &displayName);
	//MoaError createIconAssetStream(std::optional<AssetStream>* streamOptionalPointer, PMoaVoid data, MoaUlong size, PIMoaCallback callbackInterfacePointer);

	struct Info {
		typedef std::map<SYMBOL_VARIANT, Info> MAP;

		//MoaChar type[kMoaMmMaxXtraDisplayName] = "";

		SYMBOL_VARIANT subType = "";
		Path::EXTENSION_MAPPED_VECTOR pathExtensions = {};
		IconValues::MAP iconValuesMap = {};
	};

	namespace Assets {
		class Info {
			public:
			void destroy();
			void duplicate(const Info &info);

			private:
			PIMoaMmValue mmValueInterfacePointer = NULL;
			PIMoaMmImage mmImageInterfacePointer = NULL;

			std::optional<IconValues> iconValuesOptional = std::nullopt;
			//std::optional<AssetStream> streamOptional = std::nullopt;

			// extensions are not defined here because these only apply to #composite
			Asset::Info::MAP assetInfoMap = {
				{"", {"", {}, {{"", 16}}}},
				{"Bitmap", {"", {}, {{"", 0}}}},
				{"FilmLoop", {"", {}, {{"", 5}}}},
				{"Field", {"", {}, {{"", 12}}}},
				{"Palette", {"", {}, {{"", 9}}}},
				{"Picture", {"", {}, {{"", 1}}}},
				{"Sound", {"", {}, {{"", 4}}}},
				{"Button", {"ButtonType", {}, {{"PushButton", 13}, {"CheckBox", 15}, {"RadioButton", 14}}}},
				{"Shape", {"", {}, {{"", 2}}}},
				{"Movie", {"", {}, {{"", 8}}}},
				{"DigitalVideo", {"", {}, {{"", 6}}}},
				{"Script", {"ScriptType", {}, {{"", 18}, {"Movie", 11}, {"Score", 19}, {"Parent", 10}}}},
				{"Text", {"", {}, {{"", 3}}}},
				{"RichText", {"", {}, {{"", 3}}}},
				{"OLE", {"", {}, {{"", 17}}}},
				{"Transition", {"", {}, {{"", 7}}}}
			};

			MoaError findIconValue(IconValues::VARIANT &iconValuesVariant);
			RESOURCE_ID getBaseResourceID(unsigned long productVersionMajor);
			MoaError getAssetInfoMapSymbols();
			MoaError getAssetInfoMapIcon(BitmapImporter &bitmapImporter, Label::Labels::Info &labelsInfo, IconValues &iconValues, RESOURCE_ID resourceID, RESOURCE_ID baseResourceID, XtraResourceCookie myCookie);
			MoaError getAssetInfoMapIcons(BitmapImporter &bitmapImporter, Label::Labels::Info &labelsInfo, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer);

			public:
			Info(BitmapImporter &bitmapImporter, Label::Labels::Info &labelsInfo, unsigned long productVersionMajor, PIMoaMmValue mmValueInterfacePointer, PIMoaCallback callbackInterfacePointer);
			~Info();
			Info(const Info &info);
			Info &operator=(const Info &info);
			std::optional<Asset::Info> find(SYMBOL_VARIANT typeSymbol);
			bool insert(SYMBOL_VARIANT typeSymbol, const Asset::Info &assetInfo);
		};
	};
};