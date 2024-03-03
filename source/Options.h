#pragma once
#include "shared.h"
#include "Path.h"

class Options {
	private:
	void destroy();
	void duplicate(const Options &options);

	PIMoaMmValue mmValueInterfacePointer = NULL;

	MoaMmValue agentOptionsValue = kVoidMoaMmValueInitializer;

	public:
	Options(PIMoaMmValue mmValueInterfacePointer);
	~Options();
	Options(const Options &options);
	Options &operator=(const Options &options);
	void getAgentOptionsValue(MoaMmValue &agentOptionsValue) const;
	void setAgentOptionsValue(const MoaMmValue &agentOptionsValue);

	MoaLong incrementFilename = FALSE;
	MoaLong replaceExistingFile = TRUE;
	MoaLong newFolder = FALSE;
	Path::EXTENSION_MAPPED_VECTOR::SIZE_TYPE alternatePathExtension = 0;
	MoaMmSymbol locationSymbol = 0;
	MoaClassID writerClassID = IID_NULL;
};