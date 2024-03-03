#include "Args.h"

void Args::destroy() {
	releaseInterface((PPMoaVoid)&drCastMemInterfacePointer);
}

void Args::duplicate(const Args &args) {
	setInterface((PPMoaVoid)&drCastMemInterfacePointer, args.drCastMemInterfacePointer);

	pathInfoOptional = args.pathInfoOptional;
	infoOptional = args.infoOptional;

	labelSymbolVariant = args.labelSymbolVariant;

	agentStringOptional = args.agentStringOptional;

	optionsOptional = args.optionsOptional;
}

Args::Args() {
}

Args::~Args() {
	destroy();
}

Args::Args(const Args &args) {
	duplicate(args);
}

Args &Args::operator=(const Args &args) {
	if (this == &args) {
		return *this;
	}

	duplicate(args);
	return *this;
}

PIMoaDrCastMem Args::getDrCastMemInterfacePointer() const {
	return (PIMoaDrCastMem)getInterface(drCastMemInterfacePointer);
}

void Args::setDrCastMemInterfacePointer(PIMoaDrCastMem drCastMemInterfacePointer) {
	setInterface((PPMoaVoid)&this->drCastMemInterfacePointer, drCastMemInterfacePointer);
}