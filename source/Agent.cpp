#include "Agent.h"

namespace Agent {
	void Info::Writer::destroy() {
		releaseInterface((PPMoaVoid)&writerInterfacePointer);
	}

	void Info::Writer::duplicate(const Writer &writer) {
		setInterface((PPMoaVoid)&writerInterfacePointer, writer.writerInterfacePointer);

		classID = writer.classID;
	}

	Info::Writer::~Writer() {
		destroy();
	}

	Info::Writer::Writer(const Writer &writer) {
		duplicate(writer);
	}

	Info::Writer &Info::Writer::operator=(const Writer &writer) {
		if (this == &writer) {
			return *this;
		}

		duplicate(writer);
		return *this;
	}

	PIMoaWriter Info::Writer::getWriterInterfacePointer() const {
		return (PIMoaWriter)getInterface(writerInterfacePointer);
	}

	void Info::Writer::setWriterInterfacePointer(PIMoaWriter writerInterfacePointer) {
		setInterface((PPMoaVoid)&this->writerInterfacePointer, writerInterfacePointer);
	}
}