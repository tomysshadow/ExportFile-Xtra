#pragma once
#include "shared.h"
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

#define ERR_CANNOT_READ (receptorPixelsInterfacePointer ? kMoaErr_BadParam : kMoaStatus_False)
//#define READ_RPCS_INDEXED_RGB
//#define READ_RPCS_RGB16

namespace Media {
	class DirectorMedia {
		public:
		DirectorMedia(Label::Info::MAP::const_iterator labelInfoNotFound);

		size_t agentMoaIDsHash = 0;
		std::shared_ptr<Agent::HIDDEN_READER_SET> agentHiddenReaderSetPointer = 0;
		std::optional<Label::MAPPED_VECTOR> labelMappedVectorOptional = std::nullopt;
		std::optional<Agent::Info::MAP> agentInfoMapOptional = std::nullopt;
		Label::Info::MAP::const_iterator labelInfoMapIterator = {};
		std::optional<Agent::Info> agentInfoOptional = std::nullopt;
		std::shared_ptr<BitmapImporter> bitmapImporterPointer = 0;
		std::optional<Asset::Info> assetInfoOptional = std::nullopt;

		class Content {
			private:
			void destroy();
			//void duplicate(const Content &content);

			PIMoaReader readerInterfacePointer = NULL;
			PIMoaRegistryEntryDict readerRegistryEntryDictInterfacePointer = NULL;
			PIMoaDataObject dataObjectInterfacePointer = NULL;

			public:
			Content();
			~Content();
			Content(const Content &content) = delete;
			Content &operator=(const Content &content) = delete;
			PIMoaReader getReaderInterfacePointer() const;
			PIMoaRegistryEntryDict getReaderRegistryEntryDictInterfacePointer() const;
			PIMoaDataObject getDataObjectInterfacePointer() const;
			void setReaderInterfacePointer(PIMoaReader readerInterfacePointer);
			void setReaderRegistryEntryDictInterfacePointer(PIMoaRegistryEntryDict readerRegistryEntryDictInterfacePointer);
			void setDataObjectInterfacePointer(PIMoaDataObject dataObjectInterfacePointer);

			Formats::Format::POINTER formatPointer = 0;
			std::unique_ptr<Stream> streamPointer = 0;
		};

		std::shared_ptr<Content> contentPointer = 0;
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
		MixerMedia(const ExportFileValueConverter &exportFileValueConverter, PIMoaDrMovie drMovieInterfacePointer, PIMoaMmValue mmValueInterfacePointer);
		MixerMedia(const ExportFileValueConverter &exportFileValueConverter, PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer);
		~MixerMedia();
		MixerMedia(const MixerMedia &mixerMedia);
		MixerMedia &operator=(const MixerMedia &mixerMedia);
		PIMoaDrMovieContext getDrMovieContextInterfacePointer() const;
		PIMoaDrCastMem getDrCastMemInterfacePointer() const;
		PIMoaRegistryEntryDict getRegistryEntryDictInterfacePointer() const;
		PIMoaMmValue getMmValueInterfacePointer() const;
		MoaMmSymbol getIsSavingSymbol() const;
		void setDrCastMemInterfacePointer(PIMoaDrCastMem drCastMemInterfacePointer);
		void setRegistryEntryDictInterfacePointer(PIMoaRegistryEntryDict registryEntryDictInterfacePointer);

		ExportFileValueConverter exportFileValueConverter;

		Formats::Format::POINTER formatPointer = 0;

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
			Lingo(PIMoaDrMovie drMovieInterfacePointer, PIMoaMmValue mmValueInterfacePointer);
			Lingo(PIMoaDrPlayer drPlayerInterfacePointer, PIMoaMmValue mmValueInterfacePointer);
			~Lingo();
			Lingo(const Lingo &lingo);
			Lingo &operator=(const Lingo &lingo);
			void callHandler(MoaMmValue &memberValue, MoaError mixerSavedCallHandlerThreadErr);
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
	class WinBMPMedia {
		private:
		void destroy();

		HANDLE fileMapping = NULL;

		std::shared_ptr<char[]> bitmapInfoPointer = 0;
		size_t bitmapInfoSize = 0;

		DWORD imageSize = 0;

		std::optional<BITMAPFILEHEADER> sourceBitmapFileHeaderOptional = std::nullopt;

		std::shared_ptr<char[]> sourceBitmapInfoPointer = 0;
		size_t sourceBitmapInfoSize = 0;

		DWORD sourceBitmapInfoColorsSize = 0;

		MoaError allocateSourceBitmap(PIMoaReceptorPixels receptorPixelsInterfacePointer, PIMoaStream readStreamInterfacePointer);

		static bool rgbX(RGBTRIPLE* rgbTriplePointer, DWORD stride, DWORD imageSize);
		static bool rgbaX(RGBQUAD* rgbQuadPointer, DWORD stride, DWORD imageSize);
		static DWORD getStride(ULONG absWidth, WORD bitCount);
		static bool getImageSize(MoaLong colorSpace, ULONG absWidth, ULONG absHeight, MoaLong &rowBytes, DWORD &imageSize);
		static bool getSamplesPerPixel(MoaLong colorSpace, MoaShort &samplesPerPixel);
		static int getCoordinate(int dimension);
		static bool getColorTableIndexedRGB(BITMAPINFO &bitmapInfo, MoaPixelFormat &pixelFormat);
		static bool getBitmapInfoColorsUsedRGB(const BITMAPINFOHEADER &bitmapInfoHeader, bool allocation, DWORD &colorsUsed);

		public:
		static const DWORD BITMAPINFO_SIZE = sizeof(BITMAPINFO);
		static const DWORD BITMAPFILEHEADER_SIZE = sizeof(BITMAPFILEHEADER);
		static const DWORD BITMAPINFOHEADER_SIZE = sizeof(BITMAPINFOHEADER);
		static const WORD TYPE = 0x4D42;

		static bool validateBitmapFileHeader(const BITMAPFILEHEADER &bitmapFileHeader, DWORD end);
		static bool validateBitmapInfoHeader(const BITMAPINFOHEADER &bitmapInfoHeader);
		static bool getBitmapInfoColorsSize(const BITMAPINFOHEADER &bitmapInfoHeader, bool allocation, DWORD &colorsSize);

		WinBMPMedia();
		~WinBMPMedia();
		WinBMPMedia(const WinBMPMedia &winBMPMedia) = delete;
		WinBMPMedia &operator=(const WinBMPMedia &winBMPMedia) = delete;
		MoaError getPixelFormat(PIMoaReceptorPixels receptorPixelsInterfacePointer);
		MoaError getMappedView(PIMoaReceptorPixels receptorPixelsInterfacePointer, PIMoaStream readStreamInterfacePointer);

		LPVOID mappedView = NULL;

		std::optional<MoaPixelFormat> pixelFormatOptional = std::nullopt;
		MoaLong direction = BOTTOM_UP;
	};
	#endif
};