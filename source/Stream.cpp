#include "Stream.h"

#include "mixsvc.h"

void Stream::destroy() {
	MoaError err = closeStream(streamInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Close Stream");
	}

	if (temp) {
		err = deleteFile(fileInterfacePointer);

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Delete File");
		}
	}
}

/*
void Stream::duplicate(const Stream &stream) {
	setInterface((PPMoaVoid)&streamInterfacePointer, stream.streamInterfacePointer);
	setInterface((PPMoaVoid)&fileInterfacePointer, stream.fileInterfacePointer);

	temp = stream.temp;
}
*/

Stream::Stream(PIMoaCallback callbackInterfacePointer)
	: temp(true) {
	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	// try getting the temp stream services
	// but earlier Director versions don't have it
	PIMoaTempStreamServices tempStreamServicesInterfacePointer = NULL;

	MoaError err = GetIMoaTempStreamServices(callbackInterfacePointer, &tempStreamServicesInterfacePointer);

	SCOPE_EXIT {
		releaseInterface((PPMoaVoid)&tempStreamServicesInterfacePointer);
	};

	if (err == kMoaErr_NoErr
		&& tempStreamServicesInterfacePointer) {
		// if we have the temp stream services we can create a memory backed stream
		err = tempStreamServicesInterfacePointer->CreateMemStream(NULL, TRUE, TRUE, TRUE, TRUE, &streamInterfacePointer);
	}

	if (err != kMoaErr_NoErr
		|| !streamInterfacePointer) {
		// if we failed to create a memory stream fallback to creating a stream from a temporary file
		err = callbackInterfacePointer->MoaCreateInstance(&CLSID_CMoaFile, &IID_IMoaFile, (PPMoaVoid)&fileInterfacePointer);

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Create Instance");
		}

		if (!fileInterfacePointer) {
			throw std::logic_error("fileInterfacePointer must not be NULL");
		}

		err = fileInterfacePointer->SetNewTempSpec(NULL);

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Set New Temp Spec");
		}

		err = fileInterfacePointer->CreateFile();

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Create File");
		}

		err = fileInterfacePointer->GetStream(0, &streamInterfacePointer);

		if (err != kMoaErr_NoErr) {
			throw std::runtime_error("Failed to Get Stream");
		}

		if (!streamInterfacePointer) {
			throw std::logic_error("streamInterfacePointer must not be NULL");
		}
	}
}

Stream::Stream(ConstPMoaChar pathStringPointer, bool replace, PIMoaCallback callbackInterfacePointer)
	: temp(false) {
	if (!pathStringPointer) {
		throw std::invalid_argument("pathStringPointer must not be NULL");
	}

	if (!callbackInterfacePointer) {
		throw std::invalid_argument("callbackInterfacePointer must not be NULL");
	}

	MoaError err = callbackInterfacePointer->MoaCreateInstance(&CLSID_CMoaFile, &IID_IMoaFile, (PPMoaVoid)&fileInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Create Instance");
	}

	if (!fileInterfacePointer) {
		throw std::logic_error("fileInterfacePointer must not be NULL");
	}

	err = fileInterfacePointer->SetPathnameSpec(pathStringPointer, FALSE);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Set Pathname Spec");
	}

	err = fileInterfacePointer->CreateFile();

	if (err == kMoaFileErr_DuplicateSpec) {
		if (replace) {
			err = kMoaErr_NoErr;
		} else {
			throw DuplicateSpec();
		}
	}

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Create File");
	}

	err = fileInterfacePointer->GetStream(0, &streamInterfacePointer);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Get Stream");
	}

	if (!streamInterfacePointer) {
		throw std::logic_error("streamInterfacePointer must not be NULL");
	}
}

Stream::~Stream() {
	destroy();
}

/*
Stream::Stream(const Stream &stream) {
	duplicate(stream);
}

Stream &Stream::operator=(const Stream &stream) {
	if (this == &stream) {
		return *this;
	}

	duplicate(stream);
	return *this;
}
*/

MoaError Stream::open() {
	// close it if it was open before
	// (so we can get read/write permission)
	RETURN_ERR(close());
	return openStream(kMoaStreamOpenAccess_ReadWrite, true, streamInterfacePointer);
}

MoaError Stream::close() {
	// don't care if stream is not open already
	MoaError err = streamInterfacePointer->Close();

	if (err != kMoaStreamErr_StreamNotOpen) {
		RETURN_ERR(err);
	}
	return kMoaErr_NoErr;
}

MoaError Stream::flush() {
	MoaError err = streamInterfacePointer->Flush();

	if (err == kMoaStreamErr_StreamNotOpen) {
		open();

		err = streamInterfacePointer->Flush();
	}
	return err;
}

MoaError Stream::copy(PIMoaStream writeStreamInterfacePointer, MoaUlong size) {
	RETURN_NULL(writeStreamInterfacePointer);

	if (!size) {
		return kMoaErr_NoErr;
	}

	const size_t BUFFER_SIZE = 0x8000;
	char buffer[BUFFER_SIZE] = {};

	MoaStreamCount numberOfBytesToRead = BUFFER_SIZE;
	MoaStreamCount numberOfBytesToWrite = 0;
	MoaStreamCount numberOfBytesCopied = 0;

	do {
		numberOfBytesToRead = min(size, numberOfBytesToRead);
		RETURN_ERR(readPartial((PMoaVoid)buffer, numberOfBytesToRead, numberOfBytesCopied));

		if (!numberOfBytesCopied) {	
			break;
		}

		numberOfBytesToWrite = numberOfBytesCopied;
		RETURN_ERR(writeStreamInterfacePointer->Write((PMoaVoid)buffer, numberOfBytesToWrite, &numberOfBytesCopied));

		if (numberOfBytesToWrite != numberOfBytesCopied) {
			return kMoaStreamErr_WrotePastEnd;
		}

		if (size != -1) {
			size -= numberOfBytesCopied;

			if (!size) {
				break;
			}
		}
	} while (numberOfBytesToRead == numberOfBytesCopied);

	if (size != -1) {
		if (size) {
			return kMoaStreamErr_ReadPastEnd;
		}
	}
	return kMoaErr_NoErr;
}

MoaError Stream::readSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead) {
	RETURN_NULL(buffer);

	MoaStreamCount numberOfBytesRead = 0;

	MoaError err = streamInterfacePointer->Read(buffer, numberOfBytesToRead, &numberOfBytesRead);

	if (err != kMoaStreamErr_StreamNotOpen
		&& err != kMoaStreamErr_BadAccessMode) {
		RETURN_ERR(err);
	}

	if (err == kMoaStreamErr_StreamNotOpen
		|| err == kMoaStreamErr_BadAccessMode) {
		MoaStreamPosition position = 0;
		err = streamInterfacePointer->GetPosition(&position);

		if (err != kMoaStreamErr_StreamNotOpen) {
			RETURN_ERR(err);
		}

		open();
		RETURN_ERR(streamInterfacePointer->SetPosition(position));
		RETURN_ERR(streamInterfacePointer->Read(buffer, numberOfBytesToRead, &numberOfBytesRead));
	}

	if (numberOfBytesToRead != numberOfBytesRead) {
		return kMoaStreamErr_ReadPastEnd;
	}
	return kMoaErr_NoErr;
}

MoaError Stream::writeSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite) {
	RETURN_NULL(buffer);

	MoaStreamCount numberOfBytesWritten = 0;

	MoaError err = streamInterfacePointer->Write(buffer, numberOfBytesToWrite, &numberOfBytesWritten);

	if (err != kMoaStreamErr_StreamNotOpen
		&& err != kMoaStreamErr_BadAccessMode) {
		RETURN_ERR(err);
	}

	if (err == kMoaStreamErr_StreamNotOpen
		|| err == kMoaStreamErr_BadAccessMode) {
		MoaStreamPosition position = 0;
		err = streamInterfacePointer->GetPosition(&position);

		if (err != kMoaStreamErr_StreamNotOpen) {
			RETURN_ERR(err);
		}

		open();
		RETURN_ERR(streamInterfacePointer->SetPosition(position));
		RETURN_ERR(streamInterfacePointer->Write(buffer, numberOfBytesToWrite, &numberOfBytesWritten));
	}

	if (numberOfBytesToWrite != numberOfBytesWritten) {
		return kMoaStreamErr_WrotePastEnd;
	}
	return kMoaErr_NoErr;
}

MoaError Stream::readPartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead, MoaStreamCount &numberOfBytesRead) {
	RETURN_NULL(buffer);

	MoaError err = streamInterfacePointer->Read(buffer, numberOfBytesToRead, &numberOfBytesRead);

	if (err != kMoaStreamErr_StreamNotOpen
		&& err != kMoaStreamErr_BadAccessMode
		&& err != kMoaStreamErr_ReadPastEnd) {
		RETURN_ERR(err);
	}

	if (err == kMoaStreamErr_StreamNotOpen
		|| err == kMoaStreamErr_BadAccessMode) {
		MoaStreamPosition position = 0;
		err = streamInterfacePointer->GetPosition(&position);

		if (err != kMoaStreamErr_StreamNotOpen) {
			RETURN_ERR(err);
		}

		open();
		RETURN_ERR(streamInterfacePointer->SetPosition(position));
		RETURN_ERR(streamInterfacePointer->Read(buffer, numberOfBytesToRead, &numberOfBytesRead));
	}
	return kMoaErr_NoErr;
}

MoaError Stream::writePartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite, MoaStreamCount &numberOfBytesWritten) {
	RETURN_NULL(buffer);

	MoaError err = streamInterfacePointer->Write(buffer, numberOfBytesToWrite, &numberOfBytesWritten);

	if (err != kMoaStreamErr_StreamNotOpen
		&& err != kMoaStreamErr_BadAccessMode
		&& err != kMoaStreamErr_WrotePastEnd) {
		RETURN_ERR(err);
	}

	if (err == kMoaStreamErr_StreamNotOpen
		|| err == kMoaStreamErr_BadAccessMode) {
		MoaStreamPosition position = 0;
		err = streamInterfacePointer->GetPosition(&position);

		if (err != kMoaStreamErr_StreamNotOpen) {
			RETURN_ERR(err);
		}

		open();
		RETURN_ERR(streamInterfacePointer->SetPosition(position));
		RETURN_ERR(streamInterfacePointer->Write(buffer, numberOfBytesToWrite, &numberOfBytesWritten));
	}
	return kMoaErr_NoErr;
}

MoaError Stream::getPosition(MoaStreamPosition &position) {
	MoaError err = streamInterfacePointer->GetPosition(&position);

	if (err == kMoaStreamErr_StreamNotOpen) {
		open();

		err = streamInterfacePointer->GetPosition(&position);
	}
	return err;
}

MoaError Stream::setPosition(MoaStreamPosition position) {
	MoaError err = streamInterfacePointer->SetPosition(position);

	if (err == kMoaStreamErr_StreamNotOpen
		|| err == kMoaStreamErr_BadSetPositionMode) {
		open();

		err = streamInterfacePointer->SetPosition(position);
	}
	return err;
}

MoaError Stream::resetPosition() {
	return setPosition(0);
}

MoaError Stream::getEnd(MoaStreamPosition &end) {
	MoaError err = streamInterfacePointer->GetEnd(&end);

	if (err == kMoaStreamErr_StreamNotOpen) {
		open();

		err = streamInterfacePointer->GetEnd(&end);
	}
	return err;
}

PIMoaStream Stream::getStreamInterfacePointer() const {
	return (PIMoaStream)getInterface(streamInterfacePointer);
}