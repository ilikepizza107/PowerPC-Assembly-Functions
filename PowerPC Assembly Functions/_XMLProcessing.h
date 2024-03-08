#ifndef XML_PROCESSING_H_V1
#define XML_PROCESSING_H_V1

#include "stdafx.h"
#include "Code Menu.h"
#include "pugi/pugixml.hpp"
#include "_lavaBytes.h"
#include <conio.h>
#include <filesystem>

class Page;
class Line;

namespace xml
{
	// ==================== Menu Config Parsing and Constants =====================

	// Parses the specified Configuration XML and applies any changes it requests to the menu.
	bool parseAndApplyConfigXML(std::string configFilePath, lava::outputSplitter& logOutput);

	// ============================================================================



	// ==================== Menu Options Parsing and Constants ====================

	// Incoming Configuration XML Variables (See "Code Menu.cpp" for defaults, and "_AdditionalCode.cpp" for relevant Config Parsing code!)
	extern std::vector<std::string> CONFIG_INCOMING_COMMENTS;
	extern bool CONFIG_DELETE_CONTROLS_COMMENTS;
	extern bool CONFIG_PSCC_ENABLED;
	extern bool CONFIG_DASH_ATTACK_ITEM_GRAB_ENABLED;
	extern bool CONFIG_JUMPSQUAT_OVERRIDE_ENABLED;

	// Enumeration of the xml-definable line fields.
	enum lineFields
	{
		lf_ValDefault = 0,
		lf_ValMin,
		lf_ValMax,
		lf_Speed,
		lf_Options,
		lc__COUNT
	};
	typedef std::array<bool, lineFields::lc__COUNT> fieldChangeArr;

	// A bundle which holds the details for an externally defined line!
	struct externalLineBundle
	{
		std::string lineName = "";
		int INDEX = INT_MAX;
		unsigned long INDEX_EXPORT_ADDRESS = ULONG_MAX;
		std::shared_ptr<Line> linePtr = nullptr;
		fieldChangeArr populated{};

	private:
		void buildIntegerLine(const pugi::xml_node& sourceNode);
		void buildFloatLine(const pugi::xml_node& sourceNode);
		void buildSelectionLine(const pugi::xml_node& sourceNode);

	public:
		externalLineBundle(const pugi::xml_node& sourceNode);
	};


	void applyLineSettingsFromMenuOptionsTree(Page& mainPageIn, const pugi::xml_document& xmlDocumentIn, lava::outputSplitter& logOutput);
	bool applyLineSettingsFromMenuOptionsTree(Page& mainPageIn, std::string xmlPathIn, lava::outputSplitter& logOutput);
	bool buildMenuOptionsTreeFromMenu(Page& mainPageIn, std::string xmlPathOut);
}

#endif
	