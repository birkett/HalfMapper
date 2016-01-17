/*
 * halfmapper, a renderer for GoldSrc maps and chapters.
 *
 * Copyright(C) 2014  Gonzalo Ávila "gzalo" Alterach
 * Copyright(C) 2015  Anthony "birkett" Birkett
 *
 * This file is part of halfmapper.
 *
 * This program is free software; you can redistribute it and / or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 */
#include "util/MemoryDebugging.h"
#include "ConfigXML.h"
#include "Logger.h"

/**
 * Constructor to set some default values.
 *
 * These values are used as fallbacks when the user config.xml can't be loaded.
 * WriteDefaultProgramConfig() will use these.
 */
ConfigXML::ConfigXML()
{
	this->m_iWidth         = 800;
	this->m_iHeight        = 600;
	this->m_fFov           = 60.0f;
	this->m_bIsometric     = false;
	this->m_bFullscreen    = false;
	this->m_bMultisampling = false;
	this->m_bVsync         = true;

	this->m_szGamePaths.push_back(HALFLIFE_DEFAULT_GAMEPATH);
	this->m_szGamePaths.push_back(CSTRIKE_DEFAULT_GAMEPATH);

}//end ConfigXML::ConfigXML()


/**
 * Destructor to clean up the internal data storage.
 */
ConfigXML::~ConfigXML()
{
	this->m_xmlProgramConfig.Clear();
	this->m_xmlMapConfig.Clear();
	this->m_vWads.clear();
	this->m_vChapterEntries.clear();
	this->m_szGamePaths.clear();

}//end ConfigXML::~ConfigXML()


/**
 * Load the user (program) configuraion.
 */
XMLError ConfigXML::LoadProgramConfig()
{
	XMLError eRetCode = this->m_xmlProgramConfig.LoadFile("config.xml");

	if (eRetCode != XML_SUCCESS) {
		if (eRetCode == XML_ERROR_FILE_NOT_FOUND) {
			this->WriteDefaultProgramConfig();
		}
		else {
			Logger::GetInstance()->AddMessage(E_ERROR, "Error loading config.xml, code:", eRetCode);
			return eRetCode;
		}
	}

	XMLNode *rootNode = this->m_xmlProgramConfig.FirstChild();

	if (rootNode == nullptr) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XML. No root element.");
		return XML_ERROR_FILE_READ_ERROR;
	}

	XMLElement *window = rootNode->FirstChildElement("window");

	if (window == nullptr) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XML. No window  element.");
		return XML_ERROR_FILE_READ_ERROR;
	}

	window->QueryUnsignedAttribute("width",         &this->m_iWidth        );
	window->QueryUnsignedAttribute("height",        &this->m_iHeight       );
	window->QueryFloatAttribute   ("fov",           &this->m_fFov          );
	window->QueryBoolAttribute    ("isometric",     &this->m_bIsometric    );
	window->QueryBoolAttribute    ("fullscreen",    &this->m_bFullscreen   );
	window->QueryBoolAttribute    ("multisampling", &this->m_bMultisampling);
	window->QueryBoolAttribute    ("vsync",         &this->m_bVsync        );


	XMLElement *gamepaths = rootNode->FirstChildElement("gamepaths");

	if (gamepaths != nullptr) {
		XMLElement *gamepath = gamepaths->FirstChildElement("gamepath");

		while (gamepath != nullptr) {
			std::string path = gamepath->GetText();

			// Add a trailing slash if none.
			if (!path.empty() && *path.rbegin() != PATH_DELIM)
				path += PATH_DELIM;

			this->m_szGamePaths.push_back(path);
			gamepath = gamepath->NextSiblingElement("gamepath");
		}
	}

	return XML_SUCCESS;

}//end ConfigXML::LoadProgramConfig()


/**
 * Load the map configuration from a given file name.
 * \param szFilename File to load from.
 */
XMLError ConfigXML::LoadMapConfig(const char *szFilename)
{
	XMLError eRetCode = this->m_xmlMapConfig.LoadFile(szFilename);

	if (eRetCode != XML_SUCCESS) {
		Logger::GetInstance()->AddMessage(E_ERROR, "%s %i", "Error loading map config, code:", eRetCode);
		return eRetCode;
	}

	XMLNode *rootNode = this->m_xmlMapConfig.FirstChild();

	if (rootNode == nullptr) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XML. No root element.");
		return XML_ERROR_FILE_READ_ERROR;
	}

	XMLElement *wadsElement = rootNode->FirstChildElement("wads");

	if (wadsElement == nullptr) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XML. No wad container element.");
		return XML_ERROR_FILE_READ_ERROR;
	}

	XMLElement *wad = wadsElement->FirstChildElement("wad");

	if (wad == nullptr) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XML. No wads element.");
		return XML_ERROR_FILE_READ_ERROR;
	}

	while (wad != nullptr) {
		this->m_vWads.push_back(wad->GetText());
		wad = wad->NextSiblingElement("wad");
	}


	XMLElement *chapter = rootNode->FirstChildElement("chapter");

	if (chapter == nullptr) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XML. No chapters found.");
		return XML_ERROR_FILE_READ_ERROR;
	}

	while (chapter != nullptr) {
		ChapterEntry sChapterEntry;
		sChapterEntry.m_szName  = chapter->Attribute("name");
		sChapterEntry.m_bRender = chapter->BoolAttribute("render");

		if (sChapterEntry.m_szName == "") {
			Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XMl. Chapter found without name attribute.");
			return XML_ERROR_FILE_READ_ERROR;
		}

		XMLElement *chapteroffset = chapter->FirstChildElement("offset");

		if (chapteroffset != nullptr) {
			chapteroffset->QueryFloatAttribute("x", &sChapterEntry.m_fOffsetX);
			chapteroffset->QueryFloatAttribute("y", &sChapterEntry.m_fOffsetY);
			chapteroffset->QueryFloatAttribute("z", &sChapterEntry.m_fOffsetZ);
		}

		XMLElement *map = chapter->FirstChildElement("map");

		while (map != nullptr) {
			MapEntry sMapEntry;
			sMapEntry.m_szName  = map->Attribute("name");
			sMapEntry.m_bRender = map->BoolAttribute("render");

			if (sMapEntry.m_szName == "") {
				Logger::GetInstance()->AddMessage(E_ERROR, "Malformed XML. Map found without name attribute.");
				return XML_ERROR_FILE_READ_ERROR;
			}

			XMLElement *offset = map->FirstChildElement("offset");

			if (offset != nullptr) {
				sMapEntry.m_szOffsetTargetName = offset->Attribute("targetname");
				offset->QueryFloatAttribute("x", &sMapEntry.m_fOffsetX);
				offset->QueryFloatAttribute("y", &sMapEntry.m_fOffsetY);
				offset->QueryFloatAttribute("z", &sMapEntry.m_fOffsetZ);
			}

			sChapterEntry.m_vMapEntries.push_back(sMapEntry);

			map = map->NextSiblingElement("map");
		}

		chapter = chapter->NextSiblingElement("chapter");
		this->m_vChapterEntries.push_back(sChapterEntry);
	}

	return XML_SUCCESS;

}//end LoadMapConfig()


/**
 * Write the default program config when not found.
 */
XMLError ConfigXML::WriteDefaultProgramConfig()
{
	// Document root node.
	XMLNode *rootNode = this->m_xmlProgramConfig.NewElement("config");

	// Window settings.
	XMLElement *window = this->m_xmlProgramConfig.NewElement("window");
	window->SetAttribute("width",         this->m_iWidth        );
	window->SetAttribute("height",        this->m_iHeight       );
	window->SetAttribute("fov",           this->m_fFov          );
	window->SetAttribute("isometric",     this->m_bIsometric    );
	window->SetAttribute("fullscreen",    this->m_bFullscreen   );
	window->SetAttribute("multisampling", this->m_bMultisampling);
	window->SetAttribute("vsync",         this->m_bVsync        );

	// Collection of game paths.
	XMLElement *gamepaths = this->m_xmlProgramConfig.NewElement("gamepaths");

	// Half Life game path.
	XMLElement *hlgamepath = this->m_xmlProgramConfig.NewElement("gamepath");
	hlgamepath->SetAttribute("name", "halflife");
	hlgamepath->SetText(HALFLIFE_DEFAULT_GAMEPATH);
	// Counter Strike game path.
	XMLElement *csgamepath = this->m_xmlProgramConfig.NewElement("gamepath");
	csgamepath->SetAttribute("name", "cstrike");
	csgamepath->SetText(CSTRIKE_DEFAULT_GAMEPATH);

	// Add elements to the document.
	this->m_xmlProgramConfig.InsertFirstChild(rootNode);
		rootNode->InsertFirstChild(window);
		rootNode->InsertEndChild(gamepaths);
			gamepaths->InsertFirstChild(hlgamepath);
			gamepaths->InsertEndChild(csgamepath);

	// Save.
	XMLError eRetCode = this->m_xmlProgramConfig.SaveFile("config.xml");

	if (eRetCode != XML_SUCCESS) {
		Logger::GetInstance()->AddMessage(E_ERROR, "Error writing config.xml, code:", eRetCode);
		return XML_ERROR_FILE_READ_ERROR;
	}

	return XML_SUCCESS;

}//end ConfigXML::WriteDefaultProgramConfig()
