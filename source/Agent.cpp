#include "Agent.h"

void Agent::Info::Writer::destroy() {
	releaseInterface((PPMoaVoid)&writerInterfacePointer);
}

void Agent::Info::Writer::duplicate(const Writer &writer) {
	setInterface((PPMoaVoid)&writerInterfacePointer, writer.writerInterfacePointer);

	classID = writer.classID;
}

Agent::Info::Writer::Writer() {
}

Agent::Info::Writer::~Writer() {
	destroy();
}

Agent::Info::Writer::Writer(const Writer &writer) {
	duplicate(writer);
}

Agent::Info::Writer &Agent::Info::Writer::operator=(const Writer &writer) {
	if (this == &writer) {
		return *this;
	}

	duplicate(writer);
	return *this;
}

PIMoaWriter Agent::Info::Writer::getWriterInterfacePointer() const {
	return (PIMoaWriter)getInterface(writerInterfacePointer);
}

void Agent::Info::Writer::setWriterInterfacePointer(PIMoaWriter writerInterfacePointer) {
	setInterface((PPMoaVoid)&this->writerInterfacePointer, writerInterfacePointer);
}