#pragma once
#include "shared.h"
#include <map>

#include "mmiimage.h"

class IconValues {
	public:
	typedef std::map<RESOURCE_ID, MoaMmValue> ICON_VALUE_MAP;

	typedef std::shared_ptr<IconValues> POINTER;
	typedef std::variant<MoaLong, POINTER> VARIANT;
	typedef std::map<SYMBOL_VARIANT, VARIANT> MAP;

	private:
	void destroy();
	void duplicate(const IconValues &iconValues);
	void move(IconValues &iconValues);

	PIMoaMmValue mmValueInterfacePointer = NULL;
	PIMoaMmImage mmImageInterfacePointer = NULL;

	/*
	These icons are cleverly imported into Visual Studio in smallest > largest order
	with the mask icons being the smallest, and color icons being the largest
	This is so that when the map is iterated, the same stream can be reused to read them
	since the icon stream can grow (but not be shrunk back down) so the whole contents
	can be overwritten with the next icon in the map
	*/
	ICON_VALUE_MAP iconValueMap = {
		{IDB_ASSET_INFO_MAP_ICON_MASK, kVoidMoaMmValueInitializer},
		{IDB_ASSET_INFO_MAP_ICON_MASK_LINKED, kVoidMoaMmValueInitializer},
		{IDB_ASSET_INFO_MAP_ICON_BW, kVoidMoaMmValueInitializer},
		{IDB_ASSET_INFO_MAP_ICON_BW_LINKED, kVoidMoaMmValueInitializer},
		{IDB_ASSET_INFO_MAP_ICON_COLOR, kVoidMoaMmValueInitializer},
		{IDB_ASSET_INFO_MAP_ICON_COLOR_LINKED, kVoidMoaMmValueInitializer}
	};

	public:
	IconValues(PIMoaMmValue mmValueInterfacePointer, PIMoaMmImage mmImageInterfacePointer);
	~IconValues();
	IconValues(const IconValues &iconValues);
	IconValues(IconValues &&iconValues) noexcept;
	IconValues &operator=(const IconValues &iconValues);
	IconValues &operator=(IconValues &&iconValues) noexcept;
	MoaError setValue(RESOURCE_ID resourceID, const MoaMmValue &value);
	const IconValues::ICON_VALUE_MAP& toIconValueMap() const;
};