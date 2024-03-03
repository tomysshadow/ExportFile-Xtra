#pragma once
#include "shared.h"
#include "Path.h"
#include "Label.h"
#include "Agent.h"
#include "Options.h"
#include <optional>

#include "driservc.h"

class Args {
	private:
	void destroy();
	void duplicate(const Args &args);

	PIMoaDrCastMem drCastMemInterfacePointer = NULL;

	public:
	Args();
	~Args();
	Args(const Args &args);
	Args &operator=(const Args &args);
	PIMoaDrCastMem getDrCastMemInterfacePointer() const;
	void setDrCastMemInterfacePointer(PIMoaDrCastMem drCastMemInterfacePointer);

	std::optional<Path::Info> pathInfoOptional = std::nullopt;
	std::optional<MoaLong> infoOptional = std::nullopt;

	Label::SYMBOL_VARIANT labelSymbolVariant = "";

	std::optional<Agent::STRING> agentStringOptional = std::nullopt;

	// strictly speaking this does not need to be an optional
	// but I prefer it to be
	// this way an error occurs if code uses it before it should
	std::optional<Options> optionsOptional = std::nullopt;
};