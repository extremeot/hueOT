//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "scriptmanager.h"
#include "luascript.h"
#include "configmanager.h"
#include "actions.h"
#include "talkaction.h"
#include "spells.h"
#include "movement.h"
#include "weapons.h"
#include "creatureevent.h"
#include "globalevent.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

Actions* g_actions = NULL;
TalkActions* g_talkactions = NULL;
Spells* g_spells = NULL;
MoveEvents* g_moveEvents = NULL;
Weapons* g_weapons = NULL;
CreatureEvents* g_creatureEvents = NULL;
GlobalEvents* g_globalEvents = NULL;

extern ConfigManager g_config;
extern void ErrorMessage(const char* message) ;

ScriptingManager::ScriptingManager()
{
	g_weapons = new Weapons();
	g_spells = new Spells();
	g_actions = new Actions();
	g_talkactions = new TalkActions();
	g_moveEvents = new MoveEvents();
	g_creatureEvents = new CreatureEvents();
	g_globalEvents = new GlobalEvents();
}

ScriptingManager::~ScriptingManager()
{
	//
}

bool ScriptingManager::loadScriptSystems()
{
	std::cout << "Loading script systems..." << std::endl;

	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);

	//load weapons data
	//std::cout << "\tLoaded Weapons Data\n";
	if(!g_weapons->loadFromXml(datadir)){
		ErrorMessage("Unable to load Weapons!");
		return false;
	}

	g_weapons->loadDefaults();

	//load spells data
	//std::cout << "\tLoaded Spells Data\n";
	if(!g_spells->loadFromXml(datadir)){
		ErrorMessage("Unable to load Spells!");
		return false;
	}

	//load actions data
	//std::cout << "\tLoaded Actions Data\n";
	if(!g_actions->loadFromXml(datadir)){
		ErrorMessage("Unable to load Actions!");
		return false;
	}

	//load talkactions data
	//std::cout << "\tLoaded Talkactions Data\n";
	if(!g_talkactions->loadFromXml(datadir)){
		ErrorMessage("Unable to load Talkactions!");
		return false;
	}

	//load moveEvents
	//std::cout << "\tLoaded Moveevents Data\n";
	if(!g_moveEvents->loadFromXml(datadir)){
		ErrorMessage("Unable to load MoveEvents!");
		return false;
	}

	//load creature events
	//std::cout << "\tLoaded Creature Events Data\n";
	if(!g_creatureEvents->loadFromXml(datadir)){
		ErrorMessage("Unable to load CreatureEvents!");
		return false;
	}

	#ifdef __GLOBALEVENTS__
	//load global events
	//std::cout << "\tLoaded Global Events Data\n";
	if(!g_globalEvents->loadFromXml(datadir)){
		ErrorMessage("Unable to load GlobalEvents!");
		return false;
	}
	#endif
	
	return true;
}
