#pragma once
#include "shared.h"

class Stream {
	private:
	void destroy();
	//void duplicate(const Stream &stream);

	PIMoaStream streamInterfacePointer = NULL;
	PIMoaFile fileInterfacePointer = NULL;

	bool temp = false;

	public:
	class DuplicateSpec : public std::runtime_error {
		public:
		DuplicateSpec() noexcept : std::runtime_error("Stream duplicate spec") {
		}
	};

	Stream(PIMoaCallback callbackInterfacePointer);
	Stream(ConstPMoaChar pathStringPointer, bool replace, PIMoaCallback callbackInterfacePointer);
	~Stream();
	Stream(const Stream &stream) = delete;
	Stream &operator=(const Stream &stream) = delete;
	MoaError open();
	MoaError close();
	MoaError flush();
	MoaError copy(PIMoaStream writeStreamInterfacePointer, MoaUlong size = -1);
	MoaError readSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead);
	MoaError writeSafe(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite);
	MoaError readPartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToRead, MoaStreamCount &numberOfBytesRead);
	MoaError writePartial(PMoaVoid buffer, MoaStreamCount numberOfBytesToWrite, MoaStreamCount &numberOfBytesWritten);
	MoaError getPosition(MoaStreamPosition &position);
	MoaError setPosition(MoaStreamPosition position);
	MoaError resetPosition();
	MoaError getEnd(MoaStreamPosition &end);
	PIMoaStream getStreamInterfacePointer() const;
};