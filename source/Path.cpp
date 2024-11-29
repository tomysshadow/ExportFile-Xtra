#include "Path.h"
#include <vector>
#include <regex>
#include <sstream>

#ifdef WINDOWS
#include <Shlobj.h>
#endif

bool Path::filterPatternExtensions(std::string filterPattern, EXTENSION_MAPPED_VECTOR &extensions) {
	// clear because we want to pass out a new mapped vector instead of appending
	extensions.clear();

	const std::regex FILTER_PATTERN_EXTENSIONS("^\\*\\.([^\\.;]+);*");

	std::smatch matches = {};

	// clear again on failure
	MAKE_SCOPE_EXIT(clearExtensionsScopeExit) {
		extensions.clear();
	};

	// note: although the documentation says there must not be a trailing semicolon
	// in practice, the MP4 Audio agent includes one anyway so we must handle it
	while (std::regex_search(filterPattern, matches, FILTER_PATTERN_EXTENSIONS)
		&& matches.length() > 1) {
		const std::string &MATCH = matches[1];

		// don't add invalid names
		// for example, don't add *.* if present in the filter pattern
		// (checked outside of the regex so we still continue to the next extension)
		if (testValidName(MATCH)) {
			extensions.push(MATCH);
		}

		filterPattern = matches.suffix();
	}

	// invalid
	if (!filterPattern.empty()) {
		return false;
	}

	clearExtensionsScopeExit.dismiss();
	return true;
}

bool Path::filterExtensions(ConstPMoaChar filterPointer, EXTENSION_MAPPED_VECTOR &extensions) {
	extensions.clear();

	RETURN_NULL_BOOL(filterPointer);

	EXTENSION_MAPPED_VECTOR patternExtensions = {};

	// immediately before the loop so we don't clear twice pointlessly when filterPointer is NULL
	MAKE_SCOPE_EXIT(clearExtensionsScopeExit) {
		extensions.clear();
	};

	while (*filterPointer) {
		filterPointer += stringSize(filterPointer);
		RETURN_NULL_BOOL(filterPointer);

		// if this fails it is not a serious error
		// we just clear out the extensions and return
		if (!filterPatternExtensions(filterPointer, patternExtensions)) {
			return true;
		}

		extensions += patternExtensions;

		filterPointer += stringSize(filterPointer);
		RETURN_NULL_BOOL(filterPointer);
	}

	clearExtensionsScopeExit.dismiss();
	return true;
}

// chars that are invalid in a name, taken from the output of Path.GetInvalidFileNameChars in C#
static const char INVALID_NAME_CHARS[] = "\\/:*?\"<>|\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";

bool Path::testValidName(const std::string &name) {
	return name.find_first_of(INVALID_NAME_CHARS) == std::string::npos;
}

// previous versions of this function replaced invalid characters with a period
// however, a file name with only periods is itself invalid
// so now it uses underscores, which is the next best thing
std::string Path::getValidName(const std::string &name) {
	// don't use testValidName here so we retain this index
	std::string::size_type foundFirstOfIndex = name.find_first_of(INVALID_NAME_CHARS);

	// immediately return if none found (most common, best case scenario)
	if (foundFirstOfIndex == std::string::npos) {
		return name;
	}

	// reserve the length of the name, since the valid name will be the same length or shorter
	std::string validName = name.substr(0, foundFirstOfIndex);
	validName.reserve(name.length());

	std::string::size_type foundFirstNotOfIndex = 0;

	do {
		// if multiple invalid characters in a row, collapse to a single underscore
		foundFirstNotOfIndex = name.find_first_not_of(INVALID_NAME_CHARS, foundFirstOfIndex);
		validName.append("_");

		if (foundFirstNotOfIndex == std::string::npos) {
			return validName;
		}

		foundFirstOfIndex = name.find_first_of(INVALID_NAME_CHARS, foundFirstNotOfIndex);
		validName.append(name, foundFirstNotOfIndex, foundFirstOfIndex - foundFirstNotOfIndex);
	} while (foundFirstOfIndex != std::string::npos);
	return validName;
}

void Path::Info::destroy() {
	releaseInterface((PPMoaVoid)&callbackInterfacePointer);
	releaseInterface((PPMoaVoid)&callocInterfacePointer);

	releaseInterface((PPMoaVoid)&pathNameInterfacePointer);

	#ifdef WINE_BUGFIX
	MoaError err = kMoaErr_NoErr;

	for (HANDLE_VECTOR::iterator findVectorIterator = findVector.begin(); findVectorIterator != findVector.end(); findVectorIterator++) {
		err = errOrDefaultErr(osErr(closeFind(*findVectorIterator)), err);
	}

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Close Find Vector");
	}
	#endif
}

void Path::Info::duplicate(const Path::Info &info) {
	setInterface((PPMoaVoid)&callbackInterfacePointer, info.callbackInterfacePointer);
	setInterface((PPMoaVoid)&callocInterfacePointer, info.callocInterfacePointer);
	
	MoaError err = clonePathName(pathNameInterfacePointer, info.pathNameInterfacePointer);

	// we can just recreate this anyway
	if (err != kMoaErr_NoErr) {
		pathNameInterfacePointer = NULL;
	}

	path = info.path;
	dirnameOptional = info.dirnameOptional;
	basenameOptional = info.basenameOptional;
	extensionOptional = info.extensionOptional;
	filenameOptional = info.filenameOptional;

	#ifdef WINE_BUGFIX
	findVector = info.findVector;
	#endif

	productVersionMajor = info.productVersionMajor;

	basenameMemberName = info.basenameMemberName;
}

// avoid calling me directly!
// go through makePathNameInterfacePointer instead if possible
// (the constructor is exempt from this)
// note that this method only validates the optionals on the object and that they form a valid state together
// it does not validate the element optionals returned by getDirnameOptional, getFilenameOptional, etc.
// (they perform that validation on their own)
void Path::Info::validate() {
	MoaError err = makePathNameInterfacePointer();

	if (err != kMoaErr_NoErr
		|| !pathNameInterfacePointer) {
		throw Invalid();
	}

	// URLs are not supported
	if (pathNameInterfacePointer->IsRemote()) {
		throw Invalid();
	}

	// check for invalid file name characters
	bool basenameOptionalHasValue = basenameOptional.has_value();

	if (basenameOptionalHasValue) {
		if (!testValidName(basenameOptional.value())) {
			throw Invalid();
		}
	}

	bool extensionOptionalHasValue = extensionOptional.has_value();

	if (extensionOptionalHasValue) {
		if (!testValidName(extensionOptional.value())) {
			throw Invalid();
		}
	}

	if (!filenameOptional.has_value()) {
		// if there is no filename
		// then there is no potential for a mismatch
		// validation ends here
		return;
	}

	const std::string &FILENAME = filenameOptional.value();

	if (!testValidName(FILENAME)) {
		throw Invalid();
	}

	bool empty = elementOrEmpty(extensionOptional).empty();

	// there is a filename
	// if the basename has a value, then
	// check for a mismatch
	if (basenameOptionalHasValue) {
		const std::string &BASENAME = basenameOptional.value();

		if (empty) {
			// if the extension is empty but the basename has a period in it
			// then after the period should be interpreted as the extension
			if (!basenameEquals(toBasename(BASENAME), FILENAME)) {
				// potentially: extension empty, basename does not have extension, but filename does
				// in this case, we should not call toBasename or toExtension on the basename and 
				// should treat the basename and extension like normal
				// so test equality again in the if statement below
				empty = false;
			}
		}

		if (!empty) {
			if (!basenameEquals(BASENAME, FILENAME)) {
				// no dice, mismatch
				throw Invalid();
			}
		}
	}

	// there is a filename
	// if the extension has a value, then
	// check for a mismatch
	if (extensionOptionalHasValue) {
		if (
			!extensionEquals(
				(
					empty
					&& basenameOptionalHasValue
				)

				? toExtension(
					basenameOptional.value()
				)

				: extensionOptional.value(),
				FILENAME
				)
			) {
			throw Invalid();
		}
	}
}

MoaError Path::Info::makePathNameInterfacePointer() {
	if (pathNameInterfacePointer) {
		return kMoaErr_NoErr;
	}

	path = "";

	MAKE_SCOPE_EXIT(releasePathNameInterfacePointerScopeExit) {
		releaseInterface((PPMoaVoid)&pathNameInterfacePointer);
	};

	RETURN_ERR(callbackInterfacePointer->MoaCreateInstance(&CLSID_CMoaPath, &IID_IMoaPathName, (PPMoaVoid)&pathNameInterfacePointer));
	RETURN_NULL(pathNameInterfacePointer);

	std::optional<std::string> elementOptional = std::nullopt;

	if (!getDirnameOptional(elementOptional)) {
		return kMoaErr_BadParam;
	}

	// initialize the path, don't resolve to an absolute path if it's relative
	// (so we can tell whether the dirname is there)
	RETURN_ERR(pathNameInterfacePointer->InitFromString(elementOrEmpty(elementOptional).c_str(), kMoaPathDialect_LOCAL, FALSE, FALSE));

	if (!getFilenameOptional(elementOptional)) {
		return kMoaErr_BadParam;
	}

	if (elementOptional.has_value()) {
		RETURN_ERR(pathNameInterfacePointer->AddFinal(elementOptional.value().c_str()));
	}

	try {
		validate();
	} catch (Invalid) {
		return kMoaErr_BadParam;
	}

	releasePathNameInterfacePointerScopeExit.dismiss();
	return kMoaErr_NoErr;
}

MoaError Path::Info::makePath() {
	if (!path.empty()) {
		return kMoaErr_NoErr;
	}

	RETURN_ERR_BOOL(makePathNameInterfacePointer());
	
	if (!pathNameInterfacePointer) {
		return kMoaErr_InternalError;
	}

	MoaLong pathStringSize = 0;
	RETURN_ERR_BOOL(pathNameInterfacePointer->GetPathSize(&pathStringSize));

	PMoaVoid pathStringPointer = callocInterfacePointer->NRAlloc(pathStringSize);

	SCOPE_EXIT {
		freeMemory(pathStringPointer, callocInterfacePointer);
	};

	RETURN_NULL_BOOL(pathStringPointer);

	RETURN_ERR_BOOL(pathNameInterfacePointer->GetPath((PMoaChar)pathStringPointer, pathStringSize));

	path = (PMoaChar)pathStringPointer;
	return kMoaErr_NoErr;
}

std::string Path::Info::toRelativePath(const std::string &path, unsigned long productVersionMajor) {
	if (path.empty()) {
		return path;
	}

	// if a relative path was passed, try and ensure it can get written
	// even if its corresponding absolute path would be longer than MAX_PATH
	// note: we can't use GetRelativePath, Director doesn't implement it
	// so we can use the filesystem functionality here instead
	std::string relativePath = FILESYSTEM_DIRECTOR_STRING(
		FILESYSTEM_DIRECTOR_PATH(
			path,
			productVersionMajor
		).lexically_relative(
			std::filesystem::current_path()
		),

		productVersionMajor
	);

	// only use it if it's not periods or whitespace (the path can be expressed relatively)
	// and the length is shorter (so it's beneficial to use)
	if (!elementPeriodsOrWhitespace(relativePath) && relativePath.length() < path.length()) {
		return relativePath;
	}

	// old method, doesn't get the directory that file interface uses
	/*
	PIMoaPathName workingDirectoryPathNameInterfacePointer = NULL;

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&workingDirectoryPathNameInterfacePointer);
	};

	RETURN_ERR(pathNameInterfacePointer->GetWorkingDirectory(&workingDirectoryPathNameInterfacePointer));

	MoaLong workingDirectoryPathStringSize = 0;
	RETURN_ERR(workingDirectoryPathNameInterfacePointer->GetPathSize(&workingDirectoryPathStringSize));

	PMoaVoid workingDirectoryPathStringPointer = callocInterfacePointer->NRAlloc(workingDirectoryPathStringSize);

	SCOPE_EXIT {
		freeMemory(workingDirectoryPathStringPointer, callocInterfacePointer);
	};

	RETURN_NULL(workingDirectoryPathStringPointer);

	RETURN_ERR(workingDirectoryPathNameInterfacePointer->GetPath((PMoaChar)workingDirectoryPathStringPointer, workingDirectoryPathStringSize));
	*/
	return path;
}

std::string Path::Info::toBasename(const std::string &filename) {
	// here we don't use std::filesystem::path
	// because we want the "dumb" logic of, if there's a period
	// then there is an extension
	// trimFilename is called in case filename ends with a period(s)
	std::string::size_type periodIndex = filename.rfind(PERIOD);

	return periodIndex == std::string::npos
	? filename

	: filename.substr(
		0,
		periodIndex
	);
}

std::string Path::Info::toExtension(const std::string &filename) {
	const std::string::size_type PERIOD_SIZE = sizeof(PERIOD);

	std::string::size_type periodIndex = filename.rfind(PERIOD);

	return periodIndex == std::string::npos
	? ""

	: filename.substr(
		periodIndex + PERIOD_SIZE,
		std::string::npos
	);
}

std::string Path::Info::toFilename(const std::string &basename, const std::string &extension) {
	if (extension.empty()) {
		return basename;
	}
	return basename + PERIOD + extension;
}

std::string& Path::Info::trimExtension(std::string &extension) {
	return extension.erase(0, extension.find_first_not_of(PERIOD));
}

bool Path::Info::elementPeriodsOrWhitespace(const std::string &element) {
	for (std::string::const_iterator elementIterator = element.begin(); elementIterator != element.end(); elementIterator++) {
		const unsigned char &periodOrSpace = *elementIterator;

		if (periodOrSpace != PERIOD && !isspace(periodOrSpace)) {
			return false;
		}
	}
	return true;
}

bool Path::Info::basenameEquals(const std::string &basename, const std::string &filename) {
	// we can't use the actual filesystem path compare method here because it's not case-insensitive
	// we shouldn't need to make these lexically normal
	// if the basename isn't, they won't match anyway
	return stringEqualsCaseInsensitive(basename.c_str(), toBasename(filename).c_str());
}

bool Path::Info::extensionEquals(const std::string &extension, const std::string &filename) {
	return stringEqualsCaseInsensitive(extension.c_str(), toExtension(filename).c_str());
}

std::string Path::Info::elementOrEmpty(const std::optional<std::string> &elementOptional) {
	return elementOptional.value_or("");
}

Path::Info::Info(unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: productVersionMajor(productVersionMajor),
	callbackInterfacePointer(callbackInterfacePointer),
	callocInterfacePointer(callocInterfacePointer) {
	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	if (!callocInterfacePointer) {
		throw std::invalid_argument("callocInterfacePointer must not be NULL");
	}

	callbackInterfacePointer->AddRef();
	callocInterfacePointer->AddRef();
}

Path::Info::Info(const std::string &path, unsigned long productVersionMajor, PIMoaCallback callbackInterfacePointer, PIMoaCalloc callocInterfacePointer)
	: productVersionMajor(productVersionMajor),
	callbackInterfacePointer(callbackInterfacePointer),
	callocInterfacePointer(callocInterfacePointer) {
	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	if (!callocInterfacePointer) {
		throw std::invalid_argument("callocInterfacePointer must not be NULL");
	}

	callbackInterfacePointer->AddRef();
	callocInterfacePointer->AddRef();

	MoaError err = setPath(path);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Set Path");
	}

	validate();
}

Path::Info::~Info() {
	destroy();
}

Path::Info::Info(const Info &info) {
	duplicate(info);
}

Path::Info &Path::Info::operator=(const Info &info) {
	if (this == &info) {
		return *this;
	}

	duplicate(info);
	return *this;
}

MoaError Path::Info::newFolder() {
	typedef std::vector<PIMoaFile> FILE_INTERFACE_POINTER_VECTOR;

	FILE_INTERFACE_POINTER_VECTOR fileInterfacePointerVector = { NULL };
	FILE_INTERFACE_POINTER_VECTOR::iterator fileInterfacePointerVectorIterator = fileInterfacePointerVector.begin();

	SCOPE_EXIT {
		for (fileInterfacePointerVectorIterator = fileInterfacePointerVector.begin(); fileInterfacePointerVectorIterator != fileInterfacePointerVector.end(); fileInterfacePointerVectorIterator++) {
			releaseInterface((PPMoaVoid)&*fileInterfacePointerVectorIterator);
		}
	};

	RETURN_ERR(callbackInterfacePointer->MoaCreateInstance(&CLSID_CMoaFile, &IID_IMoaFile, (PPMoaVoid)&*fileInterfacePointerVectorIterator));
	
	PIMoaFile fileInterfacePointer = *fileInterfacePointerVectorIterator;
	RETURN_NULL(fileInterfacePointer);

	std::string dirname = "";
	
	if (!getDirnameOrEmpty(dirname)) {
		return kMoaErr_BadParam;
	}

	if (dirname.empty()) {
		return kMoaErr_BadParam;
	}

	RETURN_ERR(fileInterfacePointer->SetPathnameSpec(dirname.c_str(), FALSE));

	MoaError err = fileInterfacePointer->CreateDirectory();

	while (err == kMoaErr_FileNotFound) {
		RETURN_ERR(fileInterfacePointer->GetDirectory(&fileInterfacePointer));
		fileInterfacePointerVector.push_back(fileInterfacePointer);
		RETURN_NULL(fileInterfacePointer);

		err = fileInterfacePointer->CreateDirectory();
	}

	// tried to create directory and it already exists? Proceed as normal
	// this may also happen if there's a file with the name of the directory we want to create
	// but in that case, it is CreateFile's responsibility to fail, not this function's
	// for any other error, give up here
	if (err != kMoaErr_NoErr && err != kMoaFileErr_DuplicateSpec) {
		return kMoaErr_FileNotFound;
	}

	fileInterfacePointerVectorIterator = fileInterfacePointerVector.end();

	if (fileInterfacePointerVectorIterator != fileInterfacePointerVector.begin()) {
		// skip the directory we just created
		// (don't remove it from the vector, we still need it to get released)
		fileInterfacePointerVectorIterator--;

		while (fileInterfacePointerVectorIterator != fileInterfacePointerVector.begin()) {
			fileInterfacePointer = *--fileInterfacePointerVectorIterator;
			RETURN_NULL(fileInterfacePointer);

			err = fileInterfacePointer->CreateDirectory();

			if (err != kMoaErr_NoErr && err != kMoaFileErr_DuplicateSpec) {
				return kMoaErr_FileNotFound;
			}
		}
	}
	return kMoaErr_NoErr;
}

MoaError Path::Info::incrementFilename() {
	// this should always exist at this stage
	std::optional<std::string> filenameOptional = std::nullopt;

	if (!getFilenameOptional(filenameOptional)) {
		return kMoaErr_BadParam;
	}

	if (!filenameOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	#ifdef MACINTOSH
	// for now this is a reimplementation of how Windows does it
	// (maybe there is a better API for this on Mac?)
	std::string basename = "";

	if (!getBasenameOrEmpty(basename)) {
		return kMoaErr_BadParam;
	}

	std::string extension = "";

	if (!getExtensionOrEmpty(extension)) {
		return kMoaErr_BadParam;
	}

	// default to 2 on the first copy
	unsigned long number = 2;

	{
		const std::regex INCREMENT_FILENAME("^([^\\(]*)\\((\\d{0,3})\\)(.*)$");

		std::smatch matches = {};

		// search for the first pair of (brackets)
		// not required to have spaces before or after them
		// can be anywhere in the filename (including as part of the extension)
		// can have any number inside, or empty (which counts as zero)
		// which we add 1 to, in order to increment the filename
		// if no (brackets) found, stick them on the end of the basename, after a space
		if (std::regex_search(filenameOptional.value(), matches, INCREMENT_FILENAME)
		&& matches.length() > 3) {
			const unsigned long MIN_NUMBER = 1;
			const unsigned long MAX_NUMBER = 999;

			basename = matches[1];

			number = strtoul(matches[2].str().c_str(), 0, 10) + 1;

			if (number < MIN_NUMBER || number > MAX_NUMBER) {
				return kMoaFileErr_IoError;
			}

			extension = matches[3];
		} else {
			basename += " ";
			extension = PERIOD + extension;
		}
	}

	std::ostringstream outputStringStream;
	outputStringStream.exceptions(std::ostringstream::badbit);
	outputStringStream << basename << "(" << number << ")" << extension;

	setFilenameOptional(outputStringStream.str());
	setBasenameOptional(std::nullopt);
	setExtensionOptional(std::nullopt);
	#endif
	#ifdef WINDOWS
	WCHAR uniqueName[MAX_PATH] = L"";

	RETURN_ERR(makePath());

	if (path.empty()) {
		return kMoaErr_InternalError;
	}

	UINT codePage = CP_DIRECTOR(productVersionMajor);

	#ifdef WINE_BUGFIX
	std::optional<std::string> dirnameOptional = std::nullopt;

	if (!getDirnameOptional(dirnameOptional)) {
		return kMoaErr_BadParam;
	}

	if (!dirnameOptional.has_value()) {
		return kMoaErr_InternalError;
	}

	// On Windows, the short/filespec arguments to PathYetAnotherMakeUniqueName are optional
	// they can be NULL, in which case the path argument should be the full destination path
	// just as the documentation states you can do
	// on Wine, the fourth argument is non-optional
	// (it returns the literal string "(null)" otherwise)
	// also, the path must be the folder path instead of the full destination path
	// and since Windows requires the short argument if the path is a folder path
	// we must get every argument ourselves to satisfy both
	// this path should exist (we wouldn't be incrementing the name otherwise)
	// so FindFirstFileW should not error here
	std::string relativePath = toRelativePath(path, productVersionMajor);

	// this shouldn't be true, even for the current directory
	// but we check it to avoid undefined behaviour with FindFirstFile
	if (relativePath.empty()) {
		return kMoaErr_InternalError;
	}

	// FindFirstFile is used instead of GetShortPathName since we
	// only care about the filename, not the entire path
	// (and this also avoids problems if we lack permissions to the parent directories)
	WIN32_FIND_DATAW win32FindDataWide = {};
	HANDLE find = FindFirstFileW(CA2W(relativePath.c_str(), codePage), &win32FindDataWide);

	SCOPE_EXIT {
		if (!closeFind(find)) {
			// off to the handle graveyard with you
			findVector.push_back(find);
		}
	};

	// file deleted in intervening time?
	if (!find || find == INVALID_HANDLE_VALUE) {
		return kMoaErr_FileNotFound;
	}

	// the alternate file name here may be empty if there is none, which is fine. This is not a bug
	if (
			!PathYetAnotherMakeUniqueName(
				uniqueName,

				CA2W(
					dirnameOptional.value().c_str(),
					codePage
				),

				win32FindDataWide.cAlternateFileName,
				win32FindDataWide.cFileName
			)
		) {
		return kMoaFileErr_BadFileSpec;
	}
	#else
	// the old beautiful version that works on Windows but not Wine, so I can't use it
	if (!PathYetAnotherMakeUniqueName(uniqueName, CA2W(path.c_str(), codePage), NULL, NULL)) {
		return kMoaFileErr_BadFileSpec;
	}
	#endif

	RETURN_ERR(setPath((LPCSTR)CW2A(uniqueName, codePage)));
	#endif
	return kMoaErr_NoErr;
}

bool Path::Info::getPath(std::string &path, bool relative) {
	path = "";

	RETURN_ERR_BOOL(makePath());
	
	if (this->path.empty()) {
		return false;
	}

	path = relative ? toRelativePath(this->path, productVersionMajor) : this->path;
	return true;
}

bool Path::Info::getElementsOrEmpty(std::string &dirname, std::string &basename, std::string &extension, std::string &filename, std::string &path, bool relative) {
	MAKE_SCOPE_EXIT(elementsScopeExit) {
		dirname = "";
		basename = "";
		extension = "";
		filename = "";
	};

	std::optional<std::string> elementOptional = std::nullopt;

	if (!getDirnameOptional(elementOptional)) {
		return false;
	}

	dirname = elementOrEmpty(elementOptional);

	if (!getBasenameOptional(elementOptional)) {
		return false;
	}

	basename = elementOrEmpty(elementOptional);

	if (!getExtensionOptional(elementOptional, true)) {
		return false;
	}

	extension = elementOrEmpty(elementOptional);

	if (!getFilenameOptional(elementOptional)) {
		return false;
	}

	filename = elementOrEmpty(elementOptional);

	if (!getPath(path, relative)) {
		return false;
	}

	elementsScopeExit.dismiss();
	return true;
}

bool Path::Info::getElementsOrEmpty(std::string &dirname, std::string &basename, std::string &extension, std::string &filename) {
	std::string path = "";
	return getElementsOrEmpty(dirname, basename, extension, filename, path);
}

bool Path::Info::getDirnameOrEmpty(std::string &dirname) {
	std::optional<std::string> dirnameOptional = std::nullopt;

	if (!getDirnameOptional(dirnameOptional)) {
		return false;
	}

	dirname = elementOrEmpty(dirnameOptional);
	return true;
}

bool Path::Info::getBasenameOrEmpty(std::string &basename) {
	std::optional<std::string> basenameOptional = std::nullopt;

	if (!getBasenameOptional(basenameOptional)) {
		return false;
	}

	basename = elementOrEmpty(basenameOptional);
	return true;
}

bool Path::Info::getExtensionOrEmpty(std::string &extension, bool fromBasename) {
	std::optional<std::string> extensionOptional = std::nullopt;

	if (!getExtensionOptional(extensionOptional, fromBasename)) {
		return false;
	}

	extension = elementOrEmpty(extensionOptional);
	return true;
}

bool Path::Info::getFilenameOrEmpty(std::string &filename) {
	std::optional<std::string> filenameOptional = std::nullopt;

	if (!getFilenameOptional(filenameOptional)) {
		return false;
	}

	filename = elementOrEmpty(filenameOptional);
	return true;
}

bool Path::Info::getElementOptionals(std::optional<std::string> &dirnameOptional, std::optional<std::string> &basenameOptional, std::optional<std::string> &extensionOptional, std::optional<std::string> &filenameOptional, std::string &path, bool relative) {
	MAKE_SCOPE_EXIT(elementOptionalsScopeExit) {
		dirnameOptional = std::nullopt;
		basenameOptional = std::nullopt;
		extensionOptional = std::nullopt;
		filenameOptional = std::nullopt;
	};

	if (!getDirnameOptional(dirnameOptional)) {
		return false;
	}

	if (!getBasenameOptional(basenameOptional)) {
		return false;
	}

	if (!getExtensionOptional(extensionOptional, true)) {
		return false;
	}

	if (!getFilenameOptional(filenameOptional)) {
		return false;
	}

	if (!getPath(path, relative)) {
		return false;
	}

	elementOptionalsScopeExit.dismiss();
	return true;
}

bool Path::Info::getElementOptionals(std::optional<std::string> &dirnameOptional, std::optional<std::string> &basenameOptional, std::optional<std::string> &extensionOptional, std::optional<std::string> &filenameOptional) {
	std::string path = "";
	return getElementOptionals(dirnameOptional, basenameOptional, extensionOptional, filenameOptional, path);
}

bool Path::Info::getDirnameOptional(std::optional<std::string> &dirnameOptional) {
	MoaError err = makePathNameInterfacePointer();

	if (err != kMoaErr_NoErr) {
		dirnameOptional = std::nullopt;
		return false;
	}

	dirnameOptional = this->dirnameOptional;

	if (dirnameOptional.has_value()) {
		// if the dirname has . or .. in it then get rid of those here
		std::string dirname = FILESYSTEM_DIRECTOR_STRING(
			FILESYSTEM_DIRECTOR_PATH(
				dirnameOptional.value(),
				productVersionMajor
			).lexically_normal(),
			
			productVersionMajor
		);

		// a whitespace dirname is not valid
		// unspecified is valid, "." is valid, but not whitespace
		if (stringWhitespace(dirname.c_str())) {
			dirnameOptional = std::nullopt;
			return false;
		}

		// add a trailing slash
		// (this only works correctly if the path has the preferred seperator! lexically normal should've fixed this for us)
		if (dirname.back() != std::filesystem::path::preferred_separator) {
			dirname += std::filesystem::path::preferred_separator;
		}

		dirnameOptional = dirname;
	}
	return true;
}

bool Path::Info::getBasenameOptional(std::optional<std::string> &basenameOptional) {
	MoaError err = makePathNameInterfacePointer();

	if (err != kMoaErr_NoErr) {
		basenameOptional = std::nullopt;
		return false;
	}

	bool empty = elementOrEmpty(this->extensionOptional).empty();

	bool fromFilename = this->filenameOptional.has_value()

	&& (
		empty
		|| !this->basenameOptional.has_value()
	);

	basenameOptional = fromFilename
	
	? toBasename(
		this->filenameOptional.value()
	)

	: this->basenameOptional;

	if (basenameOptional.has_value()) {
		// toBasename should never be called twice
		if (empty && !fromFilename) {
			// if there is no extension but the basename has a period in it
			// then after the period should be interpreted as the extension
			// this is also responsible for fixing basename ending in ... with no extension
			basenameOptional = toBasename(basenameOptional.value());
		}

		// should be ensured by validate (when it calls getValidFileName on this)
		/*
		if (FILESYSTEM_DIRECTOR_PATH(basenameOptional.value(), productVersionMajor).has_parent_path()) {
			return false;
		}
		*/
	}
	return true;
}

bool Path::Info::getExtensionOptional(std::optional<std::string> &extensionOptional, bool fromBasename) {
	MoaError err = makePathNameInterfacePointer();

	if (err != kMoaErr_NoErr) {
		extensionOptional = std::nullopt;
		return false;
	}

	bool filenameOptionalHasValue = this->filenameOptional.has_value();
	bool basenameOptionalHasValue = this->basenameOptional.has_value();

	// if first.second.txt is both the filename and the basename, and the extension is unspecified
	// if the #text label is used, then the
	// #text label appends its own txt extension, so the filename should be interpreted
	// as first.second.txt.txt (invalid, filename mismatch)
	// if on the other hand the #composite label is used, then the
	// so the filename should be reinterpreted as first.second.txt (valid)
	// so this should return the unspecified extension so the user of this class sets the extension
	if (
		filenameOptionalHasValue
		&& basenameOptionalHasValue
		&& !this->extensionOptional.has_value()
		&& stringEqualsCaseInsensitive(this->filenameOptional.value().c_str(), this->basenameOptional.value().c_str())
	) {
		extensionOptional = this->extensionOptional;
		return true;
	}

	if (!elementOrEmpty(this->extensionOptional).empty()) {
		// if the extension is not empty, always use it
		extensionOptional = this->extensionOptional;
	} else if (filenameOptionalHasValue) {
		// if the extension is empty (so it can be implicit)
		// and the filename has a value
		// then this takes precedence over all else for consistency
		extensionOptional = toExtension(this->filenameOptional.value());
	} else if (basenameOptionalHasValue
	&& fromBasename) {
		// and the extension is empty (so it can be implicit)
		// and the basename has a value
		// and the caller wants the extension from the basename
		// then try that next
		extensionOptional = toExtension(this->basenameOptional.value());

		// never allow an empty extension to materialize from toExtension here
		// empty extensions are valid but MUST be specified explicitly
		// (the extension from the filename counts as specifying the extension explicitly, but not the basename)
		if (extensionOptional.value().empty()) {
			extensionOptional = this->extensionOptional;
		}
	} else {
		extensionOptional = this->extensionOptional;
	}

	if (extensionOptional.has_value()) {
		// should be ensured by validate
		/*
		if (FILESYSTEM_DIRECTOR_PATH(EXTENSION, productVersionMajor).has_parent_path()) {
			return false;
		}
		*/

		// extension should not include the period (but may be whitespace)
		if (extensionOptional.value().find(PERIOD) != std::string::npos) {
			return false;
		}
	}
	return true;
}

bool Path::Info::getFilenameOptional(std::optional<std::string> &filenameOptional) {
	MoaError err = makePathNameInterfacePointer();

	if (err != kMoaErr_NoErr) {
		filenameOptional = std::nullopt;
		return false;
	}

	if (this->filenameOptional.has_value()) {
		filenameOptional = this->filenameOptional;
	} else {
		if (!this->basenameOptional.has_value() || !this->extensionOptional.has_value()) {
			filenameOptional = this->filenameOptional;
			return true;
		}

		filenameOptional = toFilename(
			elementOrEmpty(
				this->basenameOptional
			),

			elementOrEmpty(
				this->extensionOptional
			)
		);
	}
	return !elementPeriodsOrWhitespace(filenameOptional.value());
}

MoaError Path::Info::setPath(const std::string &path) {
	PIMoaPathName setPathNameInterfacePointer = NULL;

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&setPathNameInterfacePointer);
	};

	RETURN_ERR(callbackInterfacePointer->MoaCreateInstance(&CLSID_CMoaPath, &IID_IMoaPathName, (PPMoaVoid)&setPathNameInterfacePointer));
	RETURN_NULL(setPathNameInterfacePointer);

	// initialize the path, don't resolve to an absolute path if it's relative
	// (so we can tell whether the dirname is there)
	// this must be normalized for it to be interpreted correctly without resolving
	// (lexically normal so Add/RemoveFinal isn't tripped up by .., and because forward slashes/colons not recognized here)
	// the filename is trimmed here so if it ends in . the filename and extension don't mismatch
	RETURN_ERR(
		setPathNameInterfacePointer->InitFromString(
			FILESYSTEM_DIRECTOR_STRING(
				FILESYSTEM_DIRECTOR_PATH(
					path,
					productVersionMajor
				).lexically_normal(),

				productVersionMajor
			).c_str(),
			
			kMoaPathDialect_LOCAL,
			FALSE,
			FALSE
		)
	);

	// get the size of the path for a string
	// (seems to include the null terminator, plus one for any non-empty path?)
	MoaLong elementStringSize = 0;
	RETURN_ERR(setPathNameInterfacePointer->GetPathSize(&elementStringSize));

	PMoaVoid elementStringPointer = callocInterfacePointer->NRAlloc(elementStringSize);

	SCOPE_EXIT {
		freeMemory(elementStringPointer, callocInterfacePointer);
	};

	RETURN_NULL(elementStringPointer);

	// at this stage we now replace the any path that was previously here
	releaseInterface((PPMoaVoid)&pathNameInterfacePointer);

	this->path = "";

	MoaError err = setPathNameInterfacePointer->GetDisplayFileName((PMoaChar)elementStringPointer, elementStringSize, TRUE);

	// filename
	filenameOptional = err == kMoaErr_NoErr
	? (PMoaChar)elementStringPointer
	: "";

	err = setPathNameInterfacePointer->GetExtension((PMoaChar)elementStringPointer, elementStringSize);

	// extension
	PMoaChar extensionStringPointer = err == kMoaErr_NoErr
		? (PMoaChar)elementStringPointer
		: NULL;

	// trim off the period if it's there
	if (extensionStringPointer) {
		std::string extensionString = extensionStringPointer;
		extensionOptional = trimExtension(extensionString);
	} else {
		extensionOptional = "";
	}

	// basename
	// this sometimes gets the name with the extension so can't be used
	/*
	err = setPathNameInterfacePointer->GetDisplayFileName((PMoaChar)elementStringPointer, elementStringSize, FALSE);

	basenameOptional = err == kMoaErr_NoErr
		? (PMoaChar)elementStringPointer
		: "";
	*/

	basenameOptional = toBasename(filenameOptional.value());

	// dirname
	if (stringWhitespace(path.c_str())) {
		// whitespace paths should be treated as invalid, because the dirname is whitespace
		dirnameOptional = path;
	} else {
		err = setPathNameInterfacePointer->RemoveFinal();

		if (err == kMoaErr_NoErr) {
			err = setPathNameInterfacePointer->GetPath((PMoaChar)elementStringPointer, elementStringSize);
		}

		if (err == kMoaErr_NoErr) {
			dirnameOptional = (PMoaChar)elementStringPointer;

			// in the case of a relative path, empty is corrected to not specified
			if (dirnameOptional.value().empty()) {
				dirnameOptional = std::nullopt;
			}
		} else {
			dirnameOptional = std::nullopt;
		}
	}
	return kMoaErr_NoErr;
}

void Path::Info::setDirnameOptional(const std::optional<std::string> &dirnameOptional) {
	releaseInterface((PPMoaVoid)&pathNameInterfacePointer);

	path = "";

	this->dirnameOptional = dirnameOptional;
}

void Path::Info::setBasenameOptional(const std::optional<std::string> &basenameOptional) {
	releaseInterface((PPMoaVoid)&pathNameInterfacePointer);

	path = "";

	this->basenameOptional = basenameOptional;
}

void Path::Info::setExtensionOptional(const std::optional<std::string> &extensionOptional) {
	releaseInterface((PPMoaVoid)&pathNameInterfacePointer);

	path = "";

	this->extensionOptional = extensionOptional;
}

void Path::Info::setFilenameOptional(const std::optional<std::string> &filenameOptional) {
	releaseInterface((PPMoaVoid)&pathNameInterfacePointer);

	path = "";

	this->filenameOptional = filenameOptional;
}