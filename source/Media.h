#pragma once
#include "utils.h"
#include "Mixer.h"
#include "BitmapImporter.h"
#include "ExportFileValueConverter.h"
#include "Agent.h"
#include "Label.h"
#include "Asset.h"
#include "Formats.h"
#include "Stream.h"
#include <optional>

#include "driservc.h"
#include "mixsvc.h"
#include "mixpix.h"

#define ERR_CANNOT_READ (receptorPixelsInterfacePointer\
	? kMoaErr_BadParam : kMoaStatus_False)

//#define READ_RPCS_INDEXED_RGB
//#define READ_RPCS_RGB16

namespace Media {
	class DirectorMedia {
		public:
		DirectorMedia(Label::Info::Map::const_iterator labelInfoNotFound);

		size_t agentMoaIDsHash = 0;
		std::shared_ptr<Agent::HiddenReaderSet> agentHiddenReaderSetPointer = nullptr;
		std::optional<Label::MappedVector> labelMappedVectorOptional = std::nullopt;
		std::optional<Agent::Info::Map> agentInfoMapOptional = std::nullopt;
		Label::Info::Map::const_iterator labelInfoMapIterator = {};
		std::optional<Agent::Info> agentInfoOptional = std::nullopt;
		std::shared_ptr<BitmapImporter> bitmapImporterPointer = nullptr;
		std::optional<Asset::Info> assetInfoOptional = std::nullopt;

		class Content : NonCopyable {
			private:
			void destroy();
			//void duplicate(const Content &content);

			PIMoaReader readerInterfacePointer = NULL;
			PIMoaRegistryEntryDict readerRegistryEntryDictInterfacePointer = NULL;
			PIMoaDataObject dataObjectInterfacePointer = NULL;

			public:
			Content() = default;
			~Content();
			PIMoaReader getReaderInterfacePointer() const;
			PIMoaRegistryEntryDict getReaderRegistryEntryDictInterfacePointer() const;
			PIMoaDataObject getDataObjectInterfacePointer() const;
			void setReaderInterfacePointer(PIMoaReader readerInterfacePointer);

			void setReaderRegistryEntryDictInterfacePointer(
				PIMoaRegistryEntryDict readerRegistryEntryDictInterfacePointer
			);

			void setDataObjectInterfacePointer(
				PIMoaDataObject dataObjectInterfacePointer
			);

			Formats::Format::Pointer formatPointer = nullptr;
			std::unique_ptr<Stream> streamPointer = nullptr;
		};

		std::shared_ptr<Content> contentPointer = nullptr;
	};

	class MixerMedia {
		private:
		void destroy();
		void duplicate(const MixerMedia &mixerMedia);

		PIMoaDrMovieContext drMovieContextInterfacePointer = NULL;
		PIMoaDrCastMem drCastMemInterfacePointer = NULL;
		PIMoaRegistryEntryDict registryEntryDictInterfacePointer = NULL;
		PIMoaMmValue mmValueInterfacePointer = NULL;

		MoaMmSymbol isSavingSymbol = 0;

		public:
		MixerMedia(
			const ExportFileValueConverter &exportFileValueConverter,
			PIMoaDrMovie drMovieInterfacePointer,
			PIMoaMmValue mmValueInterfacePointer
		);

		MixerMedia(
			const ExportFileValueConverter &exportFileValueConverter,
			PIMoaDrPlayer drPlayerInterfacePointer,
			PIMoaMmValue mmValueInterfacePointer
		);

		~MixerMedia();
		MixerMedia(const MixerMedia &mixerMedia);
		MixerMedia &operator=(const MixerMedia &mixerMedia);
		PIMoaDrMovieContext getDrMovieContextInterfacePointer() const;
		PIMoaDrCastMem getDrCastMemInterfacePointer() const;
		PIMoaRegistryEntryDict getRegistryEntryDictInterfacePointer() const;
		PIMoaMmValue getMmValueInterfacePointer() const;
		MoaMmSymbol getIsSavingSymbol() const;

		void setDrCastMemInterfacePointer(
			PIMoaDrCastMem drCastMemInterfacePointer
		);

		void setRegistryEntryDictInterfacePointer(
			PIMoaRegistryEntryDict registryEntryDictInterfacePointer
		);

		ExportFileValueConverter exportFileValueConverter;

		Formats::Format::Pointer formatPointer = nullptr;

		class Lingo {
			void destroy();
			void duplicate(const Lingo &lingo);

			PIMoaDrMovie drMovieInterfacePointer = NULL;
			PIMoaMmValue mmValueInterfacePointer = NULL;

			MoaMmValue defaultResultValue = kVoidMoaMmValueInitializer;
			MoaMmValue defaultErrCodeValue = kVoidMoaMmValueInitializer;

			struct Symbols {
				MoaMmSymbol MixerSaved = 0;
			};

			Symbols symbols;

			MoaError getSymbols();
			MoaError getDefaultValues();
			public:
			Lingo(
				PIMoaDrMovie drMovieInterfacePointer, PIMoaMmValue mmValueInterfacePointer
			);

			Lingo(
				PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer
			);

			~Lingo();
			Lingo(const Lingo &lingo);
			Lingo &operator=(const Lingo &lingo);

			void callHandler(
				MoaMmValue &memberValue,
				MoaError mixerSavedCallHandlerThreadErr
			);

			PIMoaDrMovie getDrMovieInterfacePointer() const;
		};

		Lingo lingo;

		#ifdef WINDOWS
		HMODULE moduleHandle = NULL;
		Mixer::Window window;
		#endif
	};

	#ifdef MACINTOSH
	class MacPICTMedia {
		// TODO: Mac (see MacPICTReader.h for what this is meant to pair with)
	}
	#endif

	#ifdef WINDOWS
	class WinBMPMedia : NonCopyable {
		private:
		void destroy();

		HANDLE fileMapping = NULL;

		std::shared_ptr<char[]> bitmapInfoPointer = nullptr;
		size_t bitmapInfoSize = 0;

		DWORD imageSize = 0;

		std::optional<BITMAPFILEHEADER> sourceBitmapFileHeaderOptional = std::nullopt;

		std::shared_ptr<char[]> sourceBitmapInfoPointer = nullptr;
		size_t sourceBitmapInfoSize = 0;

		DWORD sourceBitmapInfoColorsSize = 0;

		MoaError allocateSourceBitmap(
			PIMoaReceptorPixels receptorPixelsInterfacePointer,
			PIMoaStream readStreamInterfacePointer
		);

		static bool rgbX(RGBTRIPLE* rgbTriplePointer, DWORD stride, DWORD imageSize);
		static bool rgbaX(RGBQUAD* rgbQuadPointer, DWORD stride, DWORD imageSize);
		static DWORD getStride(LONG absWidth, WORD bitCount);

		static bool getImageSize(
			MoaLong colorSpace,
			LONG absWidth, LONG absHeight,
			MoaLong &rowBytes, DWORD &imageSize
		);

		static bool getSamplesPerPixel(
			MoaLong colorSpace,
			MoaShort &samplesPerPixel
		);

		static int getCoordinate(int dimension);

		static bool getColorTableIndexedRGB(
			BITMAPINFO &bitmapInfo, MoaPixelFormat &pixelFormat
		);

		static bool getBitmapInfoColorsUsedRGB(
			const BITMAPINFOHEADER &bitmapInfoHeader, bool allocation, DWORD &colorsUsed
		);

		public:
		static constexpr WORD Type = 0x4D42;

		static bool validateBitmapFileHeader(
			const BITMAPFILEHEADER &bitmapFileHeader, DWORD end
		);

		static bool validateBitmapInfoHeader(
			const BITMAPINFOHEADER &bitmapInfoHeader
		);

		static bool getBitmapInfoColorsSize(
			const BITMAPINFOHEADER &bitmapInfoHeader,
			bool allocation,
			DWORD &colorsSize
		);

		WinBMPMedia() = default;
		~WinBMPMedia();
		MoaError getPixelFormat(
			PIMoaReceptorPixels receptorPixelsInterfacePointer
		);

		MoaError getMappedView(
			PIMoaReceptorPixels receptorPixelsInterfacePointer,
			PIMoaStream readStreamInterfacePointer
		);

		LPVOID mappedView = NULL;

		std::optional<MoaPixelFormat> pixelFormatOptional = std::nullopt;
		MoaLong direction = BOTTOM_UP;
	};
	#endif
};