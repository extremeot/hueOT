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

#include "configmanager.h"
#include <iostream>
#include <algorithm>
#include <string>

ConfigManager::ConfigManager()
{
	L = NULL;

	m_isLoaded = false;

	m_confString[IP] = "";
	m_confInteger[GAME_PORT] = 0;
	m_confInteger[LOGIN_PORT] = 0;
	m_confInteger[STATUS_PORT] = 0;
}

ConfigManager::~ConfigManager()
{
	//
}

bool ConfigManager::loadFile(const std::string& _filename)
{
	std::string __filename = _filename;
	std::transform(__filename.begin(), __filename.end(), __filename.begin(), ::tolower);
	if(L)
		lua_close(L);

	L = lua_open();

	if(!L) return false;

	if(luaL_dofile(L, __filename.c_str()))
	{
		lua_close(L);
		L = NULL;
		return false;
	}

	// parse config
	if(!m_isLoaded) // info that must be loaded one time (unless we reset the modules involved)
	{
		m_confString[CONFIG_FILE] = __filename;

		// These settings might have been set from command line
		if(m_confString[IP] == "")
			m_confString[IP] = getGlobalString(L, "IP", "127.0.0.1");
		if(m_confInteger[GAME_PORT] == 0)
			m_confInteger[GAME_PORT] = getGlobalNumber(L, "GAME_PORT");
		if(m_confInteger[LOGIN_PORT] == 0)
			m_confInteger[LOGIN_PORT] = getGlobalNumber(L, "LOGIN_PORT");
		if(m_confInteger[STATUS_PORT] == 0)
			m_confInteger[STATUS_PORT] = getGlobalNumber(L, "STATUS_PORT");

		m_confString[DATA_DIRECTORY] = getGlobalString(L, "DATA_DIRECTORY");
		m_confString[MAP_FILE] = getGlobalString(L, "MAP_FILE");
		m_confString[HOUSE_RENT_PERIOD] = getGlobalString(L, "HOUSE_RENT_PERIOD", "monthly");
		m_confString[MAP_KIND] = getGlobalString(L, "MAP_KIND");
		if(getGlobalString(L, "md5passwords") != ""){
			std::cout << "Warning: [ConfigManager] md5passwords is deprecated. Use passwordtype instead." << std::endl;
		}
		m_confString[PASSWORD_TYPE_STR] = getGlobalString(L, "PASSWORD_TYPE_STR");
		m_confString[PASSWORD_SALT] = getGlobalString(L, "PASSWORD_SALT", "");
		m_confString[WORLD_TYPE] = getGlobalString(L, "WORLD_TYPE");
		m_confString[SQL_HOST] = getGlobalString(L, "SQL_HOST");
		m_confString[SQL_USER] = getGlobalString(L, "SQL_USER");
		m_confString[SQL_PASS] = getGlobalString(L, "SQL_PASS");
		m_confString[SQL_DB] = getGlobalString(L, "SQL_DB");
		m_confString[SQL_TYPE] = getGlobalString(L, "SQL_TYPE");
		m_confInteger[SQL_PORT] = getGlobalNumber(L, "SQL_PORT");
		m_confInteger[PASSWORD_TYPE] = PASSWORD_TYPE_PLAIN;
	}

	m_confString[LOGIN_MSG] = getGlobalString(L, "LOGIN_MSG", "Welcome.");
	m_confString[SERVER_NAME] = getGlobalString(L, "SERVER_NAME");
	m_confString[WORLD_NAME] = getGlobalString(L, "WORLD_NAME", "Gameworld");
	m_confString[OWNER_NAME] = getGlobalString(L, "ownername");
	m_confString[OWNER_EMAIL] = getGlobalString(L, "OWNER_EMAIL");
	m_confString[URL] = getGlobalString(L, "URL");
	m_confString[LOCATION] = getGlobalString(L, "LOCATION");
	m_confString[MAP_STORAGE_TYPE] = getGlobalString(L, "MAP_STORAGE_TYPE", "relational");
	m_confInteger[LOGIN_TRIES] = getGlobalNumber(L, "LOGIN_TRIES", 5);
	m_confInteger[RETRY_TIMEOUT] = getGlobalNumber(L, "RETRY_TIMEOUT", 30 * 1000);
	m_confInteger[LOGIN_TIMEOUT] = getGlobalNumber(L, "LOGIN_TIMEOUT", 5 * 1000);
	m_confString[MOTD] = getGlobalString(L, "MOTD");
	m_confInteger[MOTD_NUM] = getGlobalNumber(L, "MOTD_NUM");
	m_confInteger[MAX_PLAYERS] = getGlobalNumber(L, "MAX_PLAYERS");
	m_confInteger[STAIRHOP_EXHAUSTED] = getGlobalNumber(L, "STAIRHOP_EXHAUSTED", 2*1000);
	m_confInteger[IN_FIGHT_DURATION] = getGlobalNumber(L, "IN_FIGHT_DURATION", 60 * 1000);
	m_confInteger[HUNTING_KILL_DURATION] = getGlobalNumber(L, "hunting_kill_duration", 60 * 1000);
	m_confInteger[FIELD_OWNERSHIP_DURATION] = getGlobalNumber(L, "FIELD_OWNERSHIP_DURATION", 5 * 1000);
	m_confInteger[MIN_ACTIONTIME] = getGlobalNumber(L, "MIN_ACTIONTIME", 200);
	m_confInteger[MIN_ACTIONEXTIME] = getGlobalNumber(L, "MIN_ACTIONEXTIME", 1000);
	m_confInteger[DEFAULT_DESPAWNRADIUS] = getGlobalNumber(L, "DEFAULT_DESPAWNRADIUS", 50);
	m_confInteger[ALLOW_CLONES] = getGlobalBoolean(L, "ALLOW_CLONES", false);
	m_confInteger[PARTY_MEMBER_EXP_BONUS] = getGlobalNumber(L, "PARTY_MEMBER_EXP_BONUS", 5);
	m_confInteger[RATE_EXPERIENCE] = getGlobalNumber(L, "RATE_EXPERIENCE", 1);
	m_confInteger[RATE_SKILL] = getGlobalNumber(L, "RATE_SKILL", 1);
	m_confInteger[RATE_LOOT] = getGlobalNumber(L, "RATE_LOOT", 1);
	m_confInteger[RATE_MAGIC] = getGlobalNumber(L, "RATE_MAGIC", 1);
	m_confInteger[RATE_SPAWN] = getGlobalNumber(L, "RATE_SPAWN", 1);
	m_confInteger[MAX_MESSAGEBUFFER] = getGlobalNumber(L, "MAX_MESSAGEBUFFER", 4);
	m_confInteger[SAVE_CLIENT_DEBUG_ASSERTIONS] = getGlobalBoolean(L, "SAVE_CLIENT_DEBUG_ASSERTIONS", true);
	m_confInteger[CHECK_ACCOUNTS] = getGlobalBoolean(L, "CHECK_ACCOUNTS", true);
	m_confInteger[USE_BALANCE_HOUSE_PAYING] = getGlobalBoolean(L, "USE_BALANCE_HOUSE_PAYING", true);
	m_confInteger[PREMIUM_ONLY_BEDS] = getGlobalBoolean(L, "PREMIUM_ONLY_BEDS", true);
	m_confInteger[UNJUST_SKULL_DURATION] = getGlobalNumber(L, "UNJUST_SKULL_DURATION", 15*60*1000);
	m_confInteger[KILLS_PER_DAY_RED_SKULL] = getGlobalNumber(L, "KILLS_PER_DAY_RED_SKULL", 3);
	m_confInteger[KILLS_PER_WEEK_RED_SKULL] = getGlobalNumber(L, "KILLS_PER_WEEK_RED_SKULL", 5);
	m_confInteger[KILLS_PER_MONTH_RED_SKULL] = getGlobalNumber(L, "KILLS_PER_MONTH_RED_SKULL", 10);
	m_confInteger[RED_SKULL_DURATION] = getGlobalNumber(L, "RED_SKULL_DURATION", 30*24*60*60);
	m_confInteger[REMOVE_AMMUNITION] = getGlobalBoolean(L, "REMOVE_AMMUNITION", true);
	m_confInteger[REMOVE_RUNE_CHARGES] = getGlobalBoolean(L, "REMOVE_RUNE_CHARGES", true);
	m_confInteger[REMOVE_WEAPON_CHARGES] = getGlobalBoolean(L, "REMOVE_WEAPON_CHARGES", true);
	m_confInteger[LOGIN_ATTACK_DELAY] = getGlobalNumber(L, "LOGIN_ATTACK_DELAY", 10*1000);
	m_confInteger[IDLE_TIME] = getGlobalNumber(L, "IDLE_TIME", 16*60*1000);
	m_confInteger[IDLE_TIME_WARNING] = getGlobalNumber(L, "IDLE_TIME_WARNING", 15*60*1000);
	m_confInteger[HOUSE_ONLY_PREMIUM] = getGlobalBoolean(L, "HOUSE_ONLY_PREMIUM", true);
	m_confInteger[HOUSE_LEVEL] = getGlobalNumber(L, "HOUSE_LEVEL", 1);
	m_confInteger[SHOW_HOUSE_PRICES] = getGlobalBoolean(L, "SHOW_HOUSE_PRICES", false);
	m_confInteger[BROADCAST_BANISHMENTS] = getGlobalBoolean(L, "BROADCAST_BANISHMENTS", false);
	m_confInteger[NOTATIONS_TO_BAN] = getGlobalNumber(L, "NOTATIONS_TO_BAN", 3);
	m_confInteger[WARNINGS_TO_FINALBAN] = getGlobalNumber(L, "WARNINGS_TO_FINALBAN", 4);
	m_confInteger[WARNINGS_TO_DELETION] = getGlobalNumber(L, "WARNINGS_TO_DELETION", 5);
	m_confInteger[BAN_LENGTH] = getGlobalNumber(L, "BAN_LENGTH", 7 * 86400);
	m_confInteger[FINALBAN_LENGTH] = getGlobalNumber(L, "FINALBAN_LENGTH", 30 * 86400);
	m_confInteger[IPBANISHMENT_LENGTH] = getGlobalNumber(L, "IPBANISHMENT_LENGTH", 86400);
	m_confInteger[ALLOW_GAMEMASTER_MULTICLIENT] = getGlobalBoolean(L, "ALLOW_GAMEMASTER_MULTICLIENT", false);
	m_confInteger[DEATH_ASSIST_COUNT] = getGlobalNumber(L, "DEATH_ASSIST_COUNT", 1);
	m_confInteger[LAST_HIT_PZBLOCK_ONLY] = getGlobalBoolean(L, "LAST_HIT_PZBLOCK_ONLY", true);
	m_confInteger[DEFENSIVE_PZ_LOCK] = getGlobalBoolean(L, "DEFENSIVE_PZ_LOCK", false);
	m_confInteger[DISTANCE_WEAPON_INTERRUPT_SWING] = getGlobalBoolean(L, "DISTANCE_WEAPON_INTERRUPT_SWING", true);
	m_confInteger[RATES_FOR_PLAYER_KILLING] = getGlobalBoolean(L, "RATES_FOR_PLAYER_KILLING", false);
	m_confInteger[RATE_EXPERIENCE_PVP] = getGlobalNumber(L, "RATE_EXPERIENCE_PVP", 0);
	m_confInteger[FIST_STRENGTH] = getGlobalNumber(L, "FIST_STRENGTH", 7);
	m_confInteger[STATUSQUERY_TIMEOUT] = getGlobalNumber(L, "STATUSQUERY_TIMEOUT", 30 * 1000);
	m_confInteger[CAN_ROPE_CREATURES] = getGlobalBoolean(L, "CAN_ROPE_CREATURES", true);
	m_confString[DEATH_MSG] = getGlobalString(L, "DEATH_MSG", "You are dead.");
	m_confInteger[CAN_ATTACK_INVISIBLE] = getGlobalBoolean(L, "CAN_ATTACK_INVISIBLE", false);
	m_confInteger[MIN_PVP_LEVEL] = getGlobalNumber(L, "MIN_PVP_LEVEL", 0);
	#ifdef __MIN_PVP_LEVEL_APPLIES_TO_SUMMONS__
	m_confInteger[MIN_PVP_LEVEL_APPLIES_TO_SUMMONS] = getGlobalBoolean(L, "MIN_PVP_LEVEL_APPLIES_TO_SUMMONS", true);
	#endif
	m_confInteger[HEIGHT_MINIMUM_FOR_IDLE] = getGlobalNumber(L, "HEIGHT_MINIMUM_FOR_IDLE", 3);
	m_confInteger[EXPERIENCE_STAGES] = getGlobalBoolean(L, "EXPERIENCE_STAGES", false);
	m_confInteger[PVP_DAMAGE] = getGlobalNumber(L, "PVP_DAMAGE", 50);
	m_confInteger[WANDS_INTERRUPT_SWING] = getGlobalBoolean(L, "WANDS_INTERRUPT_SWING", true);
	m_confInteger[LUA_EXCEPTED_TYPE_ERRORS_ENABLED]	= getGlobalBoolean(L, "LUA_EXCEPTED_TYPE_ERRORS_ENABLED", false);
	m_confInteger[BIND_ONLY_GLOBAL_ADDRESS]	= getGlobalBoolean(L, "BIND_ONLY_GLOBAL_ADDRESS", false);
	m_confInteger[USE_RUNE_LEVEL_REQUIREMENTS] = getGlobalBoolean(L, "USE_RUNE_LEVEL_REQUIREMENTS", true);
	m_confInteger[CONTAINER_ITEMS_AUTO_STACK] = getGlobalBoolean(L, "CONTAINER_ITEMS_AUTO_STACK", false);
	m_confInteger[UPDATE_DATABASE_HOUSES] = getGlobalBoolean(L, "UPDATE_DATABASE_HOUSES", false);
	m_confInteger[DECREASE_SPAWN_REGEN] = getGlobalNumber(L, "DECREASE_SPAWN_REGEN", 0);
	m_confInteger[RESTORE_MAP_DATA] = getGlobalBoolean(L, "RESTORE_MAP_DATA", true);
	m_confInteger[KILLS_TO_BAN] = getGlobalNumber(L, "KILLS_TO_BAN", 10);
	m_confInteger[GOLD_DROP_CHANCE] = getGlobalNumber(L, "GOLD_DROP_CHANCE", 0);
	m_confInteger[ACCOUNT_LOCK_DURATION] = getGlobalNumber(L, "ACCOUNT_LOCK_DURATION", 5 * 60 * 1000);
	m_confInteger[IP_LOGIN_TRIES] = getGlobalNumber(L, "IP_LOGIN_TRIES", 10);
	m_confInteger[MAX_PACKETS_PER_SECOND] = getGlobalNumber(L, "MAX_PACKETS_PER_SECOND", 25);

	m_isLoaded = true;
	return true;
}

bool ConfigManager::reload()
{
	if(!m_isLoaded)
		return false;

	return loadFile(m_confString[CONFIG_FILE]);
}

const std::string& ConfigManager::getString(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_STRING_CONFIG)
		return m_confString[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getString] " << _what << std::endl;
		return m_confString[DUMMY_STR];
	}
}

int64_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_INTEGER_CONFIG)
		return m_confInteger[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getNumber] " << _what << std::endl;
		return 0;
	}
}

bool ConfigManager::setNumber(uint32_t _what, int64_t _value)
{
	if(_what < LAST_INTEGER_CONFIG)
	{
		m_confInteger[_what] = _value;
		return true;
	}
	else
	{
		std::cout << "Warning: [ConfigManager::setNumber] " << _what << std::endl;
		return false;
	}
}

bool ConfigManager::setString(uint32_t _what, const std::string& _value)
{
	if(_what < LAST_STRING_CONFIG)
	{
		m_confString[_what] = _value;
		return true;
	}
	else
	{
		std::cout << "Warning: [ConfigManager::setString] " << _what << std::endl;
		return false;
	}
}

std::string ConfigManager::getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isstring(_L, -1)){
		lua_pop(_L, 1);
		return _default;
	}

	int len = (int)lua_strlen(_L, -1);
	std::string ret(lua_tostring(_L, -1), len);
	lua_pop(_L,1);

	return ret;
}

int64_t ConfigManager::getGlobalNumber(lua_State* _L, const std::string& _identifier, int64_t _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1)){
		lua_pop(_L, 1);
		return _default;
	}

	int64_t val = (int64_t)lua_tonumber(_L, -1);
	lua_pop(_L,1);

	return val;
}

double ConfigManager::getGlobalFloat(lua_State* _L, const std::string& _identifier, double _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1)){
		lua_pop(_L, 1);
		return _default;
	}

	double val = lua_tonumber(_L, -1);
	lua_pop(_L,1);

	return val;
}

bool ConfigManager::getGlobalBoolean(lua_State* _L, const std::string& _identifier, bool _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(lua_isnumber(_L, -1)){
		int val = (int)lua_tonumber(_L, -1);
		lua_pop(_L, 1);
		return val != 0;
	} else if(lua_isstring(_L, -1)){
		std::string val = lua_tostring(_L, -1);
		lua_pop(_L, 1);
		return val == "yes";
	} else if(lua_isboolean(_L, -1)){
		bool v = lua_toboolean(_L, -1) != 0;
		lua_pop(_L, 1);
		return v;
	}

	return _default;
}

void ConfigManager::getConfigValue(const std::string& key, lua_State* toL)
{
	lua_getglobal(L, key.c_str());
	moveValue(L, toL);
}

void ConfigManager::moveValue(lua_State* from, lua_State* to)
{
	switch(lua_type(from, -1)){
		case LUA_TNIL:
			lua_pushnil(to);
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean(to, lua_toboolean(from, -1));
			break;
		case LUA_TNUMBER:
			lua_pushnumber(to, lua_tonumber(from, -1));
			break;
		case LUA_TSTRING:
		{
			size_t len;
			const char* str = lua_tolstring(from, -1, &len);
			lua_pushlstring(to, str, len);
		}
			break;
		case LUA_TTABLE:
			lua_newtable(to);

			lua_pushnil(from); // First key
			while(lua_next(from, -2)){
				// Move value to the other state
				moveValue(from, to);
				// Value is popped, key is left

				// Move key to the other state
				lua_pushvalue(from, -1); // Make a copy of the key to use for the next iteration
				moveValue(from, to);
				// Key is in other state.
				// We still have the key in the 'from' state ontop of the stack

				lua_insert(to, -2); // Move key above value
				lua_settable(to, -3); // Set the key
			}
		default:
			break;
	}
	// Pop the value we just read
	lua_pop(from, 1);
}

