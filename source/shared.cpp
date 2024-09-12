#include "shared.h"

MoaError getSymbol(SYMBOL_VARIANT &symbolVariant, PIMoaMmValue mmValueInterfacePointer) {
	RETURN_NULL(mmValueInterfacePointer);

	if (std::holds_alternative<std::string>(symbolVariant)) {
		const std::string &STR = std::get<std::string>(symbolVariant);

		// IMPORTANT: if empty string, leave it! Don't set it to zero!
		if (STR.empty()) {
			return kMoaErr_NoErr;
		}

		MoaMmSymbol symbol = 0;
		RETURN_ERR(mmValueInterfacePointer->StringToSymbol(STR.c_str(), &symbol));
		symbolVariant = symbol;
		return kMoaErr_NoErr;
	}

	if (std::holds_alternative<MoaMmSymbol>(symbolVariant)) {
		return kMoaErr_NoErr;
	}
	return kMoaErr_InternalError;
}

MoaError getSymbol(const SYMBOL_VARIANT &symbolVariant, MoaMmSymbol &symbol, PIMoaMmValue mmValueInterfacePointer) {
	RETURN_NULL(mmValueInterfacePointer);

	SYMBOL_VARIANT gotSymbolVariant = symbolVariant;
	RETURN_ERR(getSymbol(gotSymbolVariant, mmValueInterfacePointer));

	if (!std::holds_alternative<MoaMmSymbol>(gotSymbolVariant)) {
		return kMoaErr_InternalError;
	}

	symbol = std::get<MoaMmSymbol>(gotSymbolVariant);
	return kMoaErr_NoErr;
}

MoaError readStreamSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead, PIMoaStream readStreamInterfacePointer) {
	RETURN_NULL(buffer);
	RETURN_NULL(readStreamInterfacePointer);

	MoaStreamCount numberOfBytesRead = 0;
	RETURN_ERR(readStreamInterfacePointer->Read(buffer, numberOfBytesToRead, &numberOfBytesRead));

	if (numberOfBytesToRead != numberOfBytesRead) {
		return kMoaStreamErr_ReadPastEnd;
	}
	return kMoaErr_NoErr;
}

MoaError writeStreamSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite, PIMoaStream writeStreamInterfacePointer) {
	RETURN_NULL(buffer);
	RETURN_NULL(writeStreamInterfacePointer);

	MoaStreamCount numberOfBytesWritten = 0;
	RETURN_ERR(writeStreamInterfacePointer->Write(buffer, numberOfBytesToWrite, &numberOfBytesWritten));

	if (numberOfBytesToWrite != numberOfBytesWritten) {
		return kMoaStreamErr_WrotePastEnd;
	}
	return kMoaErr_NoErr;
}

MoaError readStreamPartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead, MoaStreamCount &numberOfBytesRead, PIMoaStream readStreamInterfacePointer) {
	RETURN_NULL(buffer);
	RETURN_NULL(readStreamInterfacePointer);

	numberOfBytesRead = 0;

	MoaError err = readStreamInterfacePointer->Read(buffer, numberOfBytesToRead, &numberOfBytesRead);

	if (err == kMoaStreamErr_ReadPastEnd) {
		err = kMoaErr_NoErr;
	}
	return err;
}

MoaError writeStreamPartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite, MoaStreamCount &numberOfBytesWritten, PIMoaStream writeStreamInterfacePointer) {
	RETURN_NULL(buffer);
	RETURN_NULL(writeStreamInterfacePointer);

	numberOfBytesWritten = 0;

	MoaError err = writeStreamInterfacePointer->Write(buffer, numberOfBytesToWrite, &numberOfBytesWritten);

	if (err == kMoaStreamErr_WrotePastEnd) {
		err = kMoaErr_NoErr;
	}
	return err;
}

#ifdef WINDOWS
MoaError setFileAttributeHiddenWide(bool hidden, LPCWSTR pathWideStringPointer) {
	RETURN_NULL(pathWideStringPointer);

	DWORD fileAttributes = GetFileAttributesW(pathWideStringPointer);
	RETURN_ERR(osErr(fileAttributes != INVALID_FILE_ATTRIBUTES));

	fileAttributes = hidden
		? fileAttributes | FILE_ATTRIBUTE_HIDDEN
		: fileAttributes & ~FILE_ATTRIBUTE_HIDDEN;
	return osErr(SetFileAttributesW(pathWideStringPointer, fileAttributes));
}
#endif