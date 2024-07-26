#pragma once
#include "shared.h"
#include <optional>

#include "moapath.h"

#ifdef WINDOWS
#define WINE_BUGFIX
#endif

namespace Path {
	typedef MappedVector<std::string, IgnoreCaseComparer> EXTENSION_MAPPED_VECTOR;

	bool filterPatternExtensions(std::string filterPattern, EXTENSION_MAPPED_VECTOR &extensions);
	bool filterExtensions(ConstPMoaChar filterPointer, EXTENSION_MAPPED_VECTOR &extensions);
	bool testValidName(const std::string &name);
	std::string getValidName(const std::string &name);

	class Info {
		private:
		void destroy();
		void duplicate(const Info &info);

		static const char PERIOD = '.';

		PIMoaCallback callbackInterfacePointer = NULL;
		PIMoaCalloc callocInterfacePointer = NULL;

		PIMoaPathName pathNameInterfacePointer = NULL;

		// path is a string instead of an optional, since empty is an invalid state for it anyway
		std::string path = "";
		std::optional<std::string> dirnameOptional = std::nullopt;
		std::optional<std::string> basenameOptional = std::nullopt;
		std::optional<std::string> extensionOptional = std::nullopt;
		std::optional<std::string> filenameOptional = std::nullopt;

		#ifdef WINE_BUGFIX
		HANDLE_VECTOR findVector = {};
		#endif

		unsigned long productVersionMajor = 0;

		void validate();
		MoaError makePathNameInterfacePointer();
		MoaError makePath();

		static std::string toRelativePath(const std::string &path, unsigned long productVersionMajor);
		static std::string toBasename(const std::string &filename);
		static std::string toExtension(const std::string &filename);
		static std::string toFilename(const std::string &basename, const std::string &extension);
		static std::string& trimExtension(std::string &extension);
		static bool elementPeriodsOrWhitespace(const std::string &element);
		static bool basenameEquals(const std::string &basename, const std::string &filename);
		static bool extensionEquals(const std::string &extension, const std::string &filename);
		public:

		static std::string elementOrEmpty(const std::optional<std::string> &elementOptional);

		class Invalid : public std::invalid_argument {
			public:
			Invalid() noexcept : std::invalid_argument("Path Info invalid") {
			}
		};

		Info(unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
		Info(const std::string &path, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer);
		~Info();
		Info(const Info &info);
		Info &operator=(const Info &info);
		MoaError newFolder();
		MoaError incrementFilename();
		bool getPath(std::string &path, bool relative = false);
		bool getElementsOrEmpty(std::string &dirname, std::string &basename, std::string &extension, std::string &filename, std::string &path, bool relative = false);
		bool getElementsOrEmpty(std::string &dirname, std::string &basename, std::string &extension, std::string &filename);
		bool getDirnameOrEmpty(std::string &dirname);
		bool getBasenameOrEmpty(std::string &basename);
		bool getExtensionOrEmpty(std::string &extension, bool fromBasename = true);
		bool getFilenameOrEmpty(std::string &filename);
		bool getElementOptionals(std::optional<std::string> &dirnameOptional, std::optional<std::string> &basenameOptional, std::optional<std::string> &extensionOptional, std::optional<std::string> &filenameOptional, std::string &path, bool relative = false);
		bool getElementOptionals(std::optional<std::string> &dirnameOptional, std::optional<std::string> &basenameOptional, std::optional<std::string> &extensionOptional, std::optional<std::string> &filenameOptional);
		bool getDirnameOptional(std::optional<std::string> &dirnameOptional);
		bool getBasenameOptional(std::optional<std::string> &basenameOptional);
		bool getExtensionOptional(std::optional<std::string> &extensionOptional, bool fromBasename = true);
		bool getFilenameOptional(std::optional<std::string> &filenameOptional);
		MoaError setPath(const std::string &path);
		void setDirnameOptional(const std::optional<std::string> &dirnameOptional);
		void setBasenameOptional(const std::optional<std::string> &basenameOptional);
		void setExtensionOptional(const std::optional<std::string> &extensionOptional);
		void setFilenameOptional(const std::optional<std::string> &filenameOptional);

		bool basenameMemberName = false;
	};
};