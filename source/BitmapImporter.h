#pragma once
#include "shared.h"
#include "GlobalHandle.h"
#include "Label.h"
#include "IconValues.h"

#include "mmiimage.h"

class BitmapImporter {
	private:
	void destroy();
	void duplicate(const BitmapImporter &bitmapImporter);
	void move(BitmapImporter &bitmapImporter);

	MoaDrMemberIndex memberIndex = NULL;
	MoaDrCastIndex castIndex = NULL;

	PIMoaMmValue mmValueInterfacePointer = NULL;
	PIMoaDrUtils drUtilsInterfacePointer = NULL;
	PIMoaMmImage mmImageInterfacePointer = NULL;
	PIMoaDrMovie drMovieInterfacePointer = NULL;
	PIMoaDrCast drCastInterfacePointer = NULL;
	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	MoaMmSymbol formatSymbol = 0;

	struct Symbols {
		MoaMmSymbol Bitmap = 0;
		MoaMmSymbol Field = 0;
		MoaMmSymbol Image = 0;
		MoaMmSymbol Picture = 0;
		MoaMmSymbol Type = 0;
	};

	Symbols symbols;

	MoaError getSymbols();
	MoaError getFormatSymbol(const Label::Labels::Info &labelsInfo);
	MoaError createMemberInCast(MoaDrCastIndex movieCastIndex);
	MoaError createMember();
	MoaError getMemberImageValue(GlobalHandleLock<>::GlobalHandle mediaData, MoaMmValue &memberStageImageValue, bool color = true);

	public:
	BitmapImporter(const Label::Labels::Info &labelsInfo, PIMoaDrMovie drMovieInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaDrUtils drUtilsInterfacePointer, PIMoaMmImage mmImageInterfacePointer);
	BitmapImporter(const Label::Labels::Info &labelsInfo, PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer, PIMoaDrUtils drUtilsInterfacePointer, PIMoaMmImage mmImageInterfacePointer);
	~BitmapImporter();
	BitmapImporter(const BitmapImporter &bitmapImporter);
	BitmapImporter(BitmapImporter &&bitmapImporter) noexcept;
	BitmapImporter &operator=(const BitmapImporter &bitmapImporter);
	BitmapImporter &operator=(BitmapImporter &&bitmapImporter) noexcept;
	MoaError toImageValue(GlobalHandleLock<>::GlobalHandle mediaData, MoaMmValue &stageImageValue, bool color = true);
	MoaError insertIntoIconValues(GlobalHandleLock<>::GlobalHandle mediaData, IconValues &iconValues, RESOURCE_ID resourceID);
	MoaError getMedia(MoaDrMediaInfo &mediaInfo, PIMoaDrCastMem imageDrCastMemInterfacePointer);
	MoaError getProp(MoaMmValue &memberPropertyValue, PIMoaDrCastMem pictureDrCastMemInterfacePointer);
};