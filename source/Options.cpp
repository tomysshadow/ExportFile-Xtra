#include "Options.h"

void Options::destroy() {
	releaseValue(agentOptionsValue, mmValueInterfacePointer);

	releaseInterface((PPMoaVoid)&mmValueInterfacePointer);
}

void Options::duplicate(const Options &options) {
	setInterface((PPMoaVoid)&mmValueInterfacePointer, options.mmValueInterfacePointer);

	setValue(agentOptionsValue, options.agentOptionsValue, mmValueInterfacePointer);

	incrementFilename = options.incrementFilename;
	replaceExistingFile = options.replaceExistingFile;
	newFolder = options.newFolder;
	alternatePathExtension = options.alternatePathExtension;
	locationSymbol = options.locationSymbol;
	writerClassID = options.writerClassID;
}

Options::Options(PIMoaMmValue mmValueInterfacePointer)
	: mmValueInterfacePointer(mmValueInterfacePointer) {
	if (!mmValueInterfacePointer) {
		throw std::invalid_argument("mmValueInterfacePointer must not be NULL");
	}

	mmValueInterfacePointer->AddRef();

	// location defaults to #current
	MoaError err = mmValueInterfacePointer->StringToSymbol("Current", &locationSymbol);

	if (err != kMoaErr_NoErr) {
		throw std::runtime_error("Failed to Convert String To Symbol");
	}
}

Options::~Options() {
	destroy();
}

Options::Options(const Options &options) {
	duplicate(options);
}

Options &Options::operator=(const Options &options) {
	if (this == &options) {
		return *this;
	}

	duplicate(options);
	return *this;
}

void Options::getAgentOptionsValue(MoaMmValue &agentOptionsValue) const {
	getValue(agentOptionsValue, this->agentOptionsValue, mmValueInterfacePointer);
}

void Options::setAgentOptionsValue(const MoaMmValue &agentOptionsValue) {
	setValue(this->agentOptionsValue, agentOptionsValue, mmValueInterfacePointer);
}