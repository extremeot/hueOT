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

#include "monsters.h"
#include "monster.h"
#include "container.h"
#include "tools.h"
#include "spells.h"
#include "combat.h"
#include "luascript.h"
#include "weapons.h"
#include "configmanager.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern Spells* g_spells;
extern Monsters g_monsters;
extern ConfigManager g_config;

MonsterType::MonsterType()
{
	reset();
}

void MonsterType::reset()
{
	experience = 0;

	defense = 0;
	armor = 0;

	canPushItems = false;
	canPushCreatures = false;
	staticAttackChance = 95;
	maxSummons = 0;
	targetDistance = 1;
	runAwayHealth = 0;
	pushable = true;
	base_speed = 200;
	health = 100;
	health_max = 100;

	outfit.lookHead   = 0;
	outfit.lookBody   = 0;
	outfit.lookLegs   = 0;
	outfit.lookFeet   = 0;
	outfit.lookType   = 0;
	outfit.lookTypeEx = 0;

	lookCorpse = 0;

	conditionImmunities = 0;
	damageImmunities = 0;
	race = RACE_BLOOD;
	isSummonable = false;
	isIllusionable = false;
	isConvinceable = false;
	isAttackable = true;
	isHostile = true;
	isLureable = false;

	lightLevel = 0;
	lightColor = 0;

	manaCost = 0;
	summonList.clear();
	lootItems.clear();
	elementMap.clear();
	m_parameters.clear();
	m_spellParameters.clear();

	for(SpellList::iterator it = spellAttackList.begin(); it != spellAttackList.end(); ++it){
		if(it->combatSpell){
			delete it->spell;
			it->spell = NULL;
		}
	}

	spellAttackList.clear();

	for(SpellList::iterator it = spellDefenseList.begin(); it != spellDefenseList.end(); ++it){
		if(it->combatSpell){
			delete it->spell;
			it->spell = NULL;
		}
	}

	spellDefenseList.clear();

	yellSpeedTicks = 0;
	yellChance = 0;
	voiceVector.clear();

	changeTargetSpeed = 0;
	changeTargetChance = 0;

	scriptList.clear();
}

MonsterType::~MonsterType()
{
	reset();
}

uint32_t Monsters::getLootRandom()
{
	return uniform_random(0, MAX_LOOTCHANCE) / g_config.getNumber(ConfigManager::RATE_LOOT);
}

void Monsters::pushSpellParameters(const std::string name, LuaScriptInterface* env)
{
	lua_State* L = env->getLuaState();

	MonsterType* mType = getMonsterType(name);
	if(!mType || mType->m_spellParameters.empty()){
		lua_pushnil(L);
		return;
	}

	lua_newtable(L);
	ParametersMap::const_iterator it = mType->m_spellParameters.begin();
	while(it != mType->m_spellParameters.end()){
		env->setField(L, it->first.c_str(), it->second);
		++it;
	}
}

void MonsterType::createLoot(Container* corpse)
{
	Container* bagContainer = Item::CreateItem(1987, 1)->getContainer();
	corpse->__internalAddThing(bagContainer);

	for(LootItems::const_iterator it = lootItems.begin(); it != lootItems.end() && (corpse->capacity() - corpse->size() > 0); ++it){
		std::list<Item*> itemList = createLootItem(*it);
		if(!itemList.empty()){
			for(std::list<Item*>::iterator iit = itemList.begin(); iit != itemList.end(); ++iit){
				Item* tmpItem = *iit;

				//check containers
				if(Container* container = tmpItem->getContainer()){
					createLootContainer(container, *it);
					if(container->size() == 0){
						delete container;
					}
					else{
						corpse->__internalAddThing(tmpItem);
					}
				}
				else{
					if (tmpItem->isWeapon())
						bagContainer->getContainer()->__internalAddThing(tmpItem);
					else
						corpse->__internalAddThing(tmpItem);
				}
			}
		}
	}

	if (bagContainer->getContainer()->getItemHoldingCount() < 1)
		corpse->__removeThing(bagContainer, 1);

	corpse->__startDecaying();
}

std::list<Item*> MonsterType::createLootItem(const LootBlock& lootBlock)
{
	Item* tmpItem = NULL;
	int32_t itemCount = 0;

	uint32_t randvalue = Monsters::getLootRandom();
	if(randvalue < lootBlock.chance){
		if(Item::items[lootBlock.id].stackable){
			itemCount = randvalue % lootBlock.countmax + 1;
		}
		else{
			itemCount = 1;
		}
	}

	std::list<Item*> itemList;

	while(itemCount > 0){
		uint16_t n = (uint16_t)std::min(itemCount, (int32_t)100);
		itemCount -= n;
		
		if((tmpItem = Item::CreateItem(lootBlock.id, n))){
			if(lootBlock.subType != -1){
				tmpItem->setSubType(lootBlock.subType);
			}

			if(lootBlock.actionId != -1){
				tmpItem->setActionId(lootBlock.actionId);
			}

			if(lootBlock.text != ""){
				tmpItem->setText(lootBlock.text);
			}

			itemList.push_back(tmpItem);
		}
	}

	return itemList;
}

void MonsterType::createLootContainer(Container* parent, const LootBlock& lootblock)
{
	if(parent->size() < parent->capacity()){
		LootItems::const_iterator it;
		for(it = lootblock.childLoot.begin(); it != lootblock.childLoot.end(); ++it){
			std::list<Item*> itemList = createLootItem(*it);
			if(!itemList.empty()){
				for(std::list<Item*>::iterator iit = itemList.begin(); iit != itemList.end(); ++iit){
					Item* tmpItem = *iit;
					if(Container* container = tmpItem->getContainer()){
						createLootContainer(container, *it);
						if(container->size() == 0 && !it->dropEmpty){
							delete container;
						}
						else{
							parent->__internalAddThing(container);
						}
					}
					else{
						parent->__internalAddThing(tmpItem);
					}
				}
			}
		}
	}
}

bool MonsterType::getParameter(const std::string key, std::string& value)
{
	ParametersMap::const_iterator it = m_parameters.find(key);
	if(it != m_parameters.end()){
		value = it->second;
		return true;
	}

	return false;
}

Monsters::Monsters()
{
	loaded = false;
}

bool Monsters::loadFromXml(const std::string& _datadir, bool reloading /*= false*/)
{
	loaded = false;
	datadir = _datadir;

	std::string filename = datadir + "monster/monsters.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"monsters") != 0){
			xmlFreeDoc(doc);
			loaded = false;
			return false;
		}

		p = root->children;
		while(p){
			if(p->type != XML_ELEMENT_NODE){
				p = p->next;
				continue;
			}

			if(xmlStrcmp(p->name, (const xmlChar*)"monster") == 0){
				std::string file;
				std::string name;

				if(readXMLString(p, "file", file) && readXMLString(p, "name", name)){
					file = datadir + "monster/" + file;
					loadMonster(file, name, reloading);
				}
			}
			else{
				std::cout << "Warning: [Monsters::loadFromXml]. Unknown node name. " << p->name << std::endl;
			}
			p = p->next;
		}

		xmlFreeDoc(doc);
	}

	return loaded;
}

bool Monsters::reload()
{
	return loadFromXml(datadir, true);
}

ConditionDamage* Monsters::getDamageCondition(ConditionType_t conditionType,
	int32_t maxDamage, int32_t minDamage, int32_t startDamage, uint32_t tickInterval)
{
	ConditionDamage* condition = dynamic_cast<ConditionDamage*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, 0, 0));
	condition->setParam(CONDITIONPARAM_TICKINTERVAL, tickInterval);
	condition->setParam(CONDITIONPARAM_MINVALUE, minDamage);
	condition->setParam(CONDITIONPARAM_MAXVALUE, maxDamage);
	condition->setParam(CONDITIONPARAM_STARTVALUE, startDamage);
	condition->setParam(CONDITIONPARAM_DELAYED, 1);

	return condition;
}

bool Monsters::deserializeSpell(xmlNodePtr node, spellBlock_t& sb, MonsterType* mType, const std::string& description /*= ""*/)
{
	sb.chance = 100;
	sb.speed = uniform_random(1000, 3000);
	sb.range = 7;
	sb.minCombatValue = 0;
	sb.maxCombatValue = 0;
	sb.combatSpell = false;
	sb.isMelee = false;

	std::string name = "";
	std::string scriptName = "";
	bool isScripted = false;

	if (readXMLString(node, "script", scriptName)){
		isScripted = true;
	}
	else if (!readXMLString(node, "Name", name)){
		return false;
	}

	int intValue;
	std::string strValue;

	if (name == "Melee")
	{
		sb.speed = 2000;
		sb.chance = 100;
		sb.range = 1;
	}

	if (readXMLInteger(node, "Chance", intValue)){
		if (intValue < 0 || intValue > 100){
			intValue = 80;
		}

		sb.chance = intValue;
	}

	if (readXMLInteger(node, "Range", intValue)){
		if (intValue < 0){
			intValue = 0;
		}

		if (intValue > Map::maxViewportX * 2){
			intValue = Map::maxViewportX * 2;
		}

		sb.range = intValue;
	}

	if (readXMLInteger(node, "Min", intValue)){
		if (intValue > 0 && asLowerCaseString(name) != "healing")
		{
			sb.minCombatValue = intValue*-1;
		}
		else {
			sb.minCombatValue = intValue;
		}
	}

	if (readXMLInteger(node, "Max", intValue)){
		if (intValue > 0 && asLowerCaseString(name) != "healing")
		{
			sb.maxCombatValue = intValue*-1;
		}
		else {
			sb.maxCombatValue = intValue;
		}

		//normalize values
		if (std::abs(sb.minCombatValue) > std::abs(sb.maxCombatValue)){
			int32_t value = sb.maxCombatValue;
			sb.maxCombatValue = sb.minCombatValue;
			sb.minCombatValue = value;
		}
	}

	if ((sb.spell = g_spells->getSpellByName(name))){
		deserializeParameters(node->children, mType, true);
		return true;
	}

	CombatSpell* combatSpell = NULL;
	bool needTarget = true;
	bool needDirection = false;

	if (isScripted){
		if (readXMLInteger(node, "Direction", intValue)){
			needDirection = (intValue == 1);
		}

		if (readXMLInteger(node, "Target", intValue)){
			needTarget = (intValue != 0);
		}

		combatSpell = new CombatSpell(NULL, needTarget, needDirection);

		std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
		if (!combatSpell->loadScript(datadir + g_spells->getScriptBaseName() + "/scripts/" + scriptName)){
			return false;
		}

		if (!combatSpell->loadScriptCombat()){
			return false;
		}

		deserializeParameters(node->children, mType, true);
		combatSpell->getCombat()->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
	}
	else{
		Combat* combat = new Combat;
		sb.combatSpell = true;

		if (readXMLInteger(node, "Length", intValue)){
			int32_t length = intValue + 1;

			if (length > 0){
				int32_t spread = 3;

				//need direction spell
				if (readXMLInteger(node, "Spread", intValue)){
					if (intValue == 30)
					{
						intValue = 4;
					}
					spread = std::max(0, intValue);
				}

				AreaCombat* area = new AreaCombat();
				area->setupArea(length, spread);
				combat->setArea(area);

				needTarget = false;
				needDirection = true;
			}
		}

		if (readXMLInteger(node, "Radius", intValue)){
			int32_t radius = intValue + 1;

			//target spell
			if (readXMLInteger(node, "Target", intValue)){
				needTarget = (intValue != 0);
			}

			AreaCombat* area = new AreaCombat();
			area->setupArea(radius);
			combat->setArea(area);
		}

		if (asLowerCaseString(name) == "melee"){
			int attack = 0;
			int skill = 0;
			sb.isMelee = true;
			if (readXMLInteger(node, "Attack", attack)){
				if (readXMLInteger(node, "Skill", skill)){
					sb.minCombatValue = 0;
					sb.maxCombatValue = -Weapons::getMaxMeleeDamage(skill, attack);
				}
			}

			ConditionType_t conditionType = CONDITION_NONE;
			int32_t minDamage = 0;
			int32_t maxDamage = 0;
			int32_t startDamage = 0;
			uint32_t tickInterval = 2000;

			if (readXMLInteger(node, "Fire", intValue)){
				conditionType = CONDITION_FIRE;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 2000;
			}
			else if (readXMLInteger(node, "Poison", intValue)){
				if (intValue > 0)
				{
					conditionType = CONDITION_POISON;

					minDamage = intValue;
					maxDamage = intValue;
					tickInterval = 2000;
				}
			}
			else if (readXMLInteger(node, "Energy", intValue)){
				conditionType = CONDITION_ENERGY;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 5000;
			}

			if (readXMLInteger(node, "tick", intValue) && intValue > 0){
				tickInterval = intValue;
			}

			if (conditionType != CONDITION_NONE){
				Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage, tickInterval);
				combat->setCondition(condition);
			}

			sb.range = 1;
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
			combat->setParam(COMBATPARAM_BLOCKEDBYSHIELD, 1);
		}
		else if (asLowerCaseString(name) == "physical"){
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
		}
		else if (asLowerCaseString(name) == "poison" || asLowerCaseString(name) == "earth"){
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_EARTHDAMAGE);
		}
		else if (asLowerCaseString(name) == "fire"){
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_FIREDAMAGE);
		}
		else if (asLowerCaseString(name) == "energy"){
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ENERGYDAMAGE);
		}
		else if (asLowerCaseString(name) == "lifedrain"){
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_LIFEDRAIN);
		}
		else if (asLowerCaseString(name) == "manadrain"){
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_MANADRAIN);
		}
		else if (asLowerCaseString(name) == "healing"){
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HEALING);
			combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
		}
		else if (asLowerCaseString(name) == "speed"){
			int32_t speedChange = 0;
			int32_t duration = 10000;

			if (readXMLInteger(node, "Duration", intValue)){
				duration = intValue * 1000;
			}

			if (readXMLInteger(node, "Strength", intValue)){
				if (intValue > 0)
					speedChange = 500 + intValue;
				else
					speedChange = -500 - intValue;

				if (speedChange < -1000){
					//cant be slower than 100%
					speedChange = -1000;
				}
				if (speedChange > 1000){
					speedChange = 1000;
				}
			}

			ConditionType_t conditionType;
			if (speedChange > 0){
				conditionType = CONDITION_HASTE;
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
			}
			else{
				conditionType = CONDITION_PARALYZE;
			}

			ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, duration, 0));
			condition->setFormulaVars(speedChange / 1000.0, 0, speedChange / 1000.0, 0);
			combat->setCondition(condition);
		}
		else if (asLowerCaseString(name) == "outfit"){
			int32_t duration = 10000;

			if (readXMLInteger(node, "Duration", intValue)){
				if (intValue < 1000)
					duration = intValue * 1000;
				else
					duration = intValue;
			}

			if (readXMLString(node, "Monster", strValue)){
				MonsterType* mType = g_monsters.getMonsterType(strValue);
				if (mType){
					ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
					condition->addOutfit(mType->outfit);
					combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
					combat->setCondition(condition);
				}
			}
			else if (readXMLInteger(node, "Type", intValue)){
				Outfit_t outfit;
				outfit.lookType = intValue;;

				ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
				condition->addOutfit(outfit);
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
				combat->setCondition(condition);
			}
			else if (readXMLInteger(node, "Item", intValue)){
				Outfit_t outfit;
				outfit.lookTypeEx = Item::items.getItemIdByClientId(intValue).id;

				ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
				condition->addOutfit(outfit);
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
				combat->setCondition(condition);
			}
		}
		else if (asLowerCaseString(name) == "invisible"){
			int32_t duration = 10000;

			if (readXMLInteger(node, "Duration", intValue)){
				duration = intValue * 1000;
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_INVISIBLE, duration, 0);
			combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
			combat->setCondition(condition);
		}
		else if (asLowerCaseString(name) == "drunk"){
			int32_t duration = 10000;

			if (readXMLInteger(node, "Duration", intValue)){
				duration = intValue * 1000;
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_DRUNK, duration, 0);
			combat->setCondition(condition);
		}
		else if (asLowerCaseString(name) == "skills" || asLowerCaseString(name) == "attributes"){
			uint32_t duration = 10000, subId = 0;
			if (readXMLInteger(node, "duration", intValue)){
				duration = intValue;
			}

			if (readXMLInteger(node, "subid", intValue)){
				subId = intValue;
			}

			intValue = 0;
			ConditionParam_t param = CONDITIONPARAM_BUFF_SPELL; //to know was it loaded
			if (readXMLInteger(node, "melee", intValue)){
				param = CONDITIONPARAM_SKILL_MELEE;
			}
			else if (readXMLInteger(node, "fist", intValue)){
				param = CONDITIONPARAM_SKILL_FIST;
			}
			else if (readXMLInteger(node, "club", intValue)){
				param = CONDITIONPARAM_SKILL_CLUB;
			}
			else if (readXMLInteger(node, "axe", intValue)){
				param = CONDITIONPARAM_SKILL_AXE;
			}
			else if (readXMLInteger(node, "sword", intValue)){
				param = CONDITIONPARAM_SKILL_SWORD;
			}
			else if (readXMLInteger(node, "distance", intValue) || readXMLInteger(node, "dist", intValue)){
				param = CONDITIONPARAM_SKILL_DISTANCE;
			}
			else if (readXMLInteger(node, "shielding", intValue) || readXMLInteger(node, "shield", intValue)){
				param = CONDITIONPARAM_SKILL_SHIELD;
			}
			else if (readXMLInteger(node, "fishing", intValue) || readXMLInteger(node, "fish", intValue)){
				param = CONDITIONPARAM_SKILL_FISHING;
			}
			else if (readXMLInteger(node, "meleePercent", intValue)){
				param = CONDITIONPARAM_SKILL_MELEEPERCENT;
			}
			else if (readXMLInteger(node, "fistPercent", intValue)){
				param = CONDITIONPARAM_SKILL_FISTPERCENT;
			}
			else if (readXMLInteger(node, "clubPercent", intValue)){
				param = CONDITIONPARAM_SKILL_CLUBPERCENT;
			}
			else if (readXMLInteger(node, "axePercent", intValue)){
				param = CONDITIONPARAM_SKILL_AXEPERCENT;
			}
			else if (readXMLInteger(node, "swordPercent", intValue)){
				param = CONDITIONPARAM_SKILL_SWORDPERCENT;
			}
			else if (readXMLInteger(node, "distancePercent", intValue) || readXMLInteger(node, "distPercent", intValue)){
				param = CONDITIONPARAM_SKILL_DISTANCEPERCENT;
			}
			else if (readXMLInteger(node, "shieldingPercent", intValue) || readXMLInteger(node, "shieldPercent", intValue)){
				param = CONDITIONPARAM_SKILL_SHIELDPERCENT;
			}
			else if (readXMLInteger(node, "fishingPercent", intValue) || readXMLInteger(node, "fishPercent", intValue)){
				param = CONDITIONPARAM_SKILL_FISHINGPERCENT;
			}
			else if (readXMLInteger(node, "maxhealth", intValue)){
				param = CONDITIONPARAM_STAT_MAXHITPOINTS;
			}
			else if (readXMLInteger(node, "maxmana", intValue)){
				param = CONDITIONPARAM_STAT_MAXMANAPOINTS;
			}
			else if (readXMLInteger(node, "soul", intValue)){
				param = CONDITIONPARAM_STAT_SOULPOINTS;
			}
			else if (readXMLInteger(node, "magiclevel", intValue) || readXMLInteger(node, "maglevel", intValue)){
				param = CONDITIONPARAM_STAT_MAGICPOINTS;
			}
			else if (readXMLInteger(node, "maxhealthPercent", intValue)){
				param = CONDITIONPARAM_STAT_MAXHITPOINTSPERCENT;
			}
			else if (readXMLInteger(node, "maxmanaPercent", intValue)){
				param = CONDITIONPARAM_STAT_MAXMANAPOINTSPERCENT;
			}
			else if (readXMLInteger(node, "soulPercent", intValue)){
				param = CONDITIONPARAM_STAT_SOULPOINTSPERCENT;
			}
			else if (readXMLInteger(node, "magiclevelPercent", intValue) || readXMLInteger(node, "maglevelPercent", intValue)){
				param = CONDITIONPARAM_STAT_MAGICPOINTSPERCENT;
			}
			if (param != CONDITIONPARAM_BUFF_SPELL){
				if (ConditionAttributes* condition = dynamic_cast<ConditionAttributes*>(Condition::createCondition(
					CONDITIONID_COMBAT, CONDITION_ATTRIBUTES, duration, subId))){
					condition->setParam(param, intValue);
					combat->setCondition(condition);
				}
			}
		}
		else if (asLowerCaseString(name) == "firefield"){
			combat->setParam(COMBATPARAM_CREATEITEM, ITEM_FIREFIELD);
		}
		else if (asLowerCaseString(name) == "poisonfield"){
			combat->setParam(COMBATPARAM_CREATEITEM, ITEM_POISONFIELD);
		}
		else if (asLowerCaseString(name) == "energyfield"){
			combat->setParam(COMBATPARAM_CREATEITEM, ITEM_ENERGYFIELD);
		}
		else if (asLowerCaseString(name) == "burning" ||
			asLowerCaseString(name) == "poisoning" ||
			asLowerCaseString(name) == "electricity"){
			ConditionType_t conditionType = CONDITION_NONE;
			uint32_t tickInterval = 2000;

			if (name == "Burning"){
				conditionType = CONDITION_FIRE;
				tickInterval = 2000;
			}
			else if (name == "Poisoning"){
				conditionType = CONDITION_POISON;
				tickInterval = 2000;
			}
			else if (name == "Electricity"){
				conditionType = CONDITION_ENERGY;
				tickInterval = 5000;
			}

			if (readXMLInteger(node, "Tick", intValue) && intValue > 0){
				tickInterval = intValue;
			}

			int32_t minDamage = std::abs(sb.minCombatValue);
			int32_t maxDamage = std::abs(sb.maxCombatValue);
			int32_t startDamage = 0;

			if (readXMLInteger(node, "Start", intValue)){
				intValue = std::abs(intValue);

				if (intValue <= minDamage){
					startDamage = intValue;
				}
			}

			int32_t totalcount = 3;

			if (readXMLInteger(node, "Total", intValue)){
				totalcount = intValue;
			}

			ConditionDamage* conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, conditionType);
			conditionDamage->addDamage(totalcount, tickInterval, minDamage);
			combat->setCondition(conditionDamage);
		}

		combat->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
		combatSpell = new CombatSpell(combat, needTarget, needDirection);

		if (readXMLInteger(node, "ShootEffect", intValue)){
			if (intValue >= 1)
			{
				intValue--;
				ShootType_t shoot = (ShootType_t)intValue--;
				if (shoot != NM_SHOOT_UNK){
					combat->setParam(COMBATPARAM_DISTANCEEFFECT, shoot);
				}
				else{
					std::cout << "Warning: [Monsters::deserializeSpell] - " << description << " - Unknown shootEffect: " << strValue << "," << intValue << std::endl;
				}
			}
		}
		if (readXMLInteger(node, "AreaEffect", intValue)){
			if (intValue >= 1)
			{
				intValue--;
				MagicEffectClasses effect = (MagicEffectClasses)intValue--;
				if (effect != NM_ME_UNK){
					combat->setParam(COMBATPARAM_EFFECT, effect);
				}
				else{
					std::cout << "Warning: [Monsters::deserializeSpell] - " << description << " - Unknown areaEffect: " << strValue << std::endl;
				}
			}
		}

		xmlNodePtr attributeNode = node->children;

		while (attributeNode){
			if (xmlStrcmp(attributeNode->name, (const xmlChar*)"attribute") == 0){
				if (readXMLString(attributeNode, "key", strValue)){
					if (asLowerCaseString(strValue) == "shooteffect"){
						if (readXMLString(attributeNode, "value", strValue)){
							ShootType_t shoot = getShootType(strValue);
							if (shoot != NM_SHOOT_UNK){
								combat->setParam(COMBATPARAM_DISTANCEEFFECT, shoot);
							}
							else{
								std::cout << "Warning: [Monsters::deserializeSpell] - " << description << " - Unknown shootEffect: " << strValue << std::endl;
							}
						}
					}
					else if (asLowerCaseString(strValue) == "areaeffect"){
						if (readXMLString(attributeNode, "value", strValue)){
							MagicEffectClasses effect = getMagicEffect(strValue);
							if (effect != NM_ME_UNK){
								combat->setParam(COMBATPARAM_EFFECT, effect);
							}
							else{
								std::cout << "Warning: [Monsters::deserializeSpell] - " << description << " - Unknown areaEffect: " << strValue << std::endl;
							}
						}
					}
				}
			}

			attributeNode = attributeNode->next;
		}
	}

	sb.spell = combatSpell;
	return true;
}

void Monsters::deserializeParameters(xmlNodePtr node, MonsterType* mType, bool fromSpell /*= false*/)
{
	if(mType){
		while(node){
			if(xmlStrcmp(node->name, (const xmlChar*)"parameter") == 0){
				std::string key;
				if(readXMLString(node, "key", key)){
					std::string value;
					if(readXMLString(node, "value", value)){
						if(fromSpell)
							mType->m_spellParameters[key] = value;
						else
							mType->m_parameters[key] = value;
					}
				}
			}

			node = node->next;
		}
	}
}

#define SHOW_XML_WARNING(desc) std::cout << "Warning: [Monsters::loadMonster]. " << desc << ". " << file << std::endl;
#define SHOW_XML_ERROR(desc) std::cout << "Error: [Monsters::loadMonster]. " << desc << ". " << file << std::endl;

bool Monsters::loadMonster(const std::string& file, const std::string& monster_name, bool reloading /*= false*/)
{
	bool monsterLoad;
	MonsterType* mType = NULL;
	bool new_mType = true;

	if (reloading){
		uint32_t id = getIdByName(monster_name);
		if (id != 0){
			mType = getMonsterType(id);
			if (mType != NULL){
				new_mType = false;
				mType->reset();
			}
		}
	}
	if (new_mType){
		mType = new MonsterType();
	}

	monsterLoad = true;
	xmlDocPtr doc = xmlParseFile(file.c_str());

	if (doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name, (const xmlChar*)"Monster") != 0){
			std::cerr << "Malformed XML: " << file << std::endl;
		}

		int intValue;
		std::string strValue;

		p = root->children;

		mType->staticAttackChance = 85;
		mType->isLureable = 0;
		mType->targetDistance = 1;

		while (p){
			if (p->type != XML_ELEMENT_NODE){
				p = p->next;
				continue;
			}

			if (xmlStrcmp(p->name, (const xmlChar*)"Name") == 0){
				if (readXMLString(p, "String", strValue))
				{
					mType->name = strValue;
				}
				else {
					SHOW_XML_ERROR("Missing Monster Name");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Blood") == 0){
				readXMLString(p, "String", strValue);

				if ((asLowerCaseString(strValue) == "slime") || (asLowerCaseString(strValue) == "poison")){
					mType->race = RACE_VENOM;
				}
				else if ((asLowerCaseString(strValue) == "blood")){
					mType->race = RACE_BLOOD;
				}
				else if ((asLowerCaseString(strValue) == "undead") || (asLowerCaseString(strValue) == "bones")){
					mType->race = RACE_UNDEAD;
				}
				else if ((asLowerCaseString(strValue) == "fire")){
					mType->race = RACE_FIRE;
				}
				else{
					SHOW_XML_WARNING("Unknown blood type " << strValue);
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Experience") == 0){
				if (readXMLInteger(p, "Value", intValue))
				{
					mType->experience = intValue;
				}
				else {
					SHOW_XML_ERROR("Missing Monster Experience");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"GoStrength") == 0){
				if (readXMLInteger(p, "Value", intValue))
				{
					if (intValue == 0)
					{
						mType->base_speed = 0;
					}
					else {
						mType->base_speed = 110 + intValue;
					}
				}
				else {
					SHOW_XML_ERROR("Missing Monster GoStrength");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"SummonCost") == 0){
				if (readXMLInteger(p, "Value", intValue))
				{
					mType->manaCost = intValue;
				}
				else {
					SHOW_XML_ERROR("Missing Monster SummonCost");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"FleeThreshold") == 0){
				if (readXMLInteger(p, "Value", intValue))
				{
					mType->runAwayHealth = intValue;
				}
				else {
					SHOW_XML_ERROR("Missing Monster FleeThreshold");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Article") == 0){
				if (readXMLString(p, "String", strValue))
				{
					if (strValue.length() > 0)
					{
						mType->nameDescription = strValue + " " + mType->name;
						toLowerCaseString(mType->nameDescription);
					}
					else {
						mType->nameDescription = mType->name;
					}
				}
				else {
					SHOW_XML_ERROR("Missing Monster Name");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Outfit") == 0){

				if (readXMLInteger(p, "Type", intValue)){
					mType->outfit.lookType = intValue;

					if (readXMLInteger(p, "Head", intValue)){
						mType->outfit.lookHead = intValue;
					}

					if (readXMLInteger(p, "Body", intValue)){
						mType->outfit.lookBody = intValue;
					}

					if (readXMLInteger(p, "Legs", intValue)){
						mType->outfit.lookLegs = intValue;
					}

					if (readXMLInteger(p, "Feet", intValue)){
						mType->outfit.lookFeet = intValue;
					}
				}
				else if (readXMLInteger(p, "Item", intValue)){
					if (intValue > 0)
						mType->outfit.lookTypeEx = Item::items.getItemIdByClientId(intValue).id;
				}
				else{
					SHOW_XML_WARNING("Missing look type/typeex");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Corpse") == 0){
				if (readXMLInteger(p, "Value", intValue))
				{
					mType->lookCorpse = Item::items.getItemIdByClientId(intValue).id;
				}
				else {
					SHOW_XML_ERROR("Missing Monster Corpse");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Hitpoints") == 0){

				if (readXMLInteger(p, "Value", intValue)){
					mType->health = intValue;
					mType->health_max = intValue;
				}
				else{
					SHOW_XML_ERROR("Missing Hitpoints Total");
					monsterLoad = false;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Defense") == 0){

				if (readXMLInteger(p, "Value", intValue)){
					mType->defense = intValue;
				}
				else{
					SHOW_XML_ERROR("Missing Defense");
					monsterLoad = false;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Armor") == 0){

				if (readXMLInteger(p, "Value", intValue)){
					mType->armor = intValue;
				}
				else{
					SHOW_XML_ERROR("Missing Armor");
					monsterLoad = false;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Flags") == 0){
				xmlNodePtr tmpNode = p->children;
				while (tmpNode){
					if (xmlStrcmp(tmpNode->name, (const xmlChar*)"Flag") == 0){

						if (readXMLInteger(tmpNode, "Summonable", intValue)){
							mType->isSummonable = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "Attackable", intValue)){
							mType->isAttackable = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "Hostile", intValue)){
							mType->isHostile = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "Illusionable", intValue)){
							mType->isIllusionable = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "Convinceable", intValue)){
							mType->isConvinceable = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "Pushable", intValue)){
							mType->pushable = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "CanPushItems", intValue)){
							mType->canPushItems = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "CanPushCreatures", intValue)){
							mType->canPushCreatures = (intValue != 0);
						}

						if (readXMLInteger(tmpNode, "DistanceFighting", intValue)){
							if (intValue == 1)
								mType->targetDistance = 4;
							else
								mType->targetDistance = 1;
						}
					}

					tmpNode = tmpNode->next;
				}
				//if a monster can push creatures,
				// it should not be pushable
				if (mType->canPushCreatures && mType->pushable){
					mType->pushable = false;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"LoseTarget") == 0){

				mType->changeTargetSpeed = 5000;

				if (readXMLInteger(p, "Value", intValue)){
					if (intValue == 50)
					{
						mType->changeTargetChance = 50;
					}
					else {
						mType->changeTargetChance = 100 - intValue;
					}

					if (mType->changeTargetChance < 0)
						mType->changeTargetChance = 0;
						
					if (intValue == 0)
						mType->changeTargetChance = 0;
				}
				else{
					SHOW_XML_WARNING("Missing LoseTarget.Chance");
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"AttackSkills") == 0){
				xmlNodePtr tmpNode = p->children;
				while (tmpNode){
					if (xmlStrcmp(tmpNode->name, (const xmlChar*)"Skill") == 0){

						spellBlock_t sb;
						if (deserializeSpell(tmpNode, sb, mType, monster_name)){
							mType->spellAttackList.push_back(sb);
						}
						else{
							SHOW_XML_WARNING("Cant load spell");
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"DefenseSkills") == 0){
				if (readXMLInteger(p, "Defense", intValue)){
					mType->defense = intValue;
				}

				if (readXMLInteger(p, "Armor", intValue)){
					mType->armor = intValue;
				}

				xmlNodePtr tmpNode = p->children;
				while (tmpNode){
					if (xmlStrcmp(tmpNode->name, (const xmlChar*)"Skill") == 0){

						spellBlock_t sb;
						if (deserializeSpell(tmpNode, sb, mType, monster_name)){
							mType->spellDefenseList.push_back(sb);
						}
						else{
							SHOW_XML_WARNING("Cant load spell");
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Immunities") == 0){
				xmlNodePtr tmpNode = p->children;
				while (tmpNode){
					if (xmlStrcmp(tmpNode->name, (const xmlChar*)"Immunity") == 0){

						if (readXMLInteger(tmpNode, "Physical", intValue)){
							if (intValue != 0){
								mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
							}
						}
						else if (readXMLInteger(tmpNode, "Energy", intValue)){
							if (intValue != 0){
								mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
								mType->conditionImmunities |= CONDITION_ENERGY;
							}
						}
						else if (readXMLInteger(tmpNode, "Fire", intValue)){
							if (intValue != 0){
								mType->damageImmunities |= COMBAT_FIREDAMAGE;
								mType->conditionImmunities |= CONDITION_FIRE;
							}
						}
						else if (readXMLInteger(tmpNode, "Poison", intValue)){
							if (intValue != 0){
								mType->damageImmunities |= COMBAT_EARTHDAMAGE;
								mType->conditionImmunities |= CONDITION_POISON;
							}
						}
						else if (readXMLInteger(tmpNode, "LifeDrain", intValue)){
							if (intValue != 0){
								mType->damageImmunities |= COMBAT_LIFEDRAIN;
								mType->conditionImmunities |= CONDITION_LIFEDRAIN;
							}
						}
						else if (readXMLInteger(tmpNode, "Paralyze", intValue)){
							if (intValue != 0){
								mType->conditionImmunities |= CONDITION_PARALYZE;
							}
						}
						else if (readXMLInteger(tmpNode, "Outfit", intValue)){
							if (intValue != 0){
								mType->conditionImmunities |= CONDITION_OUTFIT;
							}
						}
						else if (readXMLInteger(tmpNode, "Drunk", intValue)){
							if (intValue != 0){
								mType->conditionImmunities |= CONDITION_DRUNK;
							}
						}
						else if (readXMLInteger(tmpNode, "Invisible", intValue)){
							if (intValue != 0){
								mType->conditionImmunities |= CONDITION_INVISIBLE;
							}
						}
						else{
							SHOW_XML_WARNING("Unknown immunity " << strValue);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Voices") == 0){
				xmlNodePtr tmpNode = p->children;

				mType->yellChance = 7;
				mType->yellSpeedTicks = 3000;

				while (tmpNode){
					if (xmlStrcmp(tmpNode->name, (const xmlChar*)"Voice") == 0){

						voiceBlock_t vb;
						vb.text = "";
						vb.yellText = false;

						if (readXMLString(tmpNode, "String", strValue)){
							vb.text = strValue;
						}
						else{
							SHOW_XML_WARNING("Missing voice.sentence");
						}

						if (readXMLInteger(tmpNode, "Yell", intValue)){
							vb.yellText = (intValue != 0);
						}

						mType->voiceVector.push_back(vb);
					}

					tmpNode = tmpNode->next;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Loot") == 0){
				xmlNodePtr tmpNode = p->children;
				while (tmpNode){
					if (tmpNode->type != XML_ELEMENT_NODE){
						tmpNode = tmpNode->next;
						continue;
					}

					LootBlock lootBlock;
					if (loadLootItem(tmpNode, lootBlock)){
						mType->lootItems.push_back(lootBlock);
					}
					else{
						SHOW_XML_WARNING("Cant load loot");
					}

					tmpNode = tmpNode->next;
				}
			}
			else if (xmlStrcmp(p->name, (const xmlChar*)"Summons") == 0){

				if (readXMLInteger(p, "Max", intValue)){
					mType->maxSummons = intValue;
				}
				else{
					SHOW_XML_WARNING("Missing summons.maxSummons");
				}

				xmlNodePtr tmpNode = p->children;
				while (tmpNode){

					summonBlock_t sb;

					if (xmlStrcmp(tmpNode->name, (const xmlChar*)"Summon") == 0){
						int32_t chance = 100;
						int32_t speed = 1000;

						if (readXMLInteger(tmpNode, "Value", intValue)){
							chance = intValue;
						}

						if (readXMLString(tmpNode, "String", strValue)){
							sb.name = strValue;
							sb.speed = speed;
							sb.chance = chance;
						}
						else{
							SHOW_XML_WARNING("Missing summon.name");
						}

						if (readXMLInteger(tmpNode, "Max", intValue)){
							mType->maxSummons += intValue;
							sb.max = intValue;
						} else {
							sb.max = mType->maxSummons;
						}

						mType->summonList.push_back(sb);
					}

					tmpNode = tmpNode->next;
				}
			}
			else{
				SHOW_XML_WARNING("Unknown attribute type - " << p->name);
			}

			p = p->next;
		}

		xmlFreeDoc(doc);
	}
	else{
		monsterLoad = false;
	}

	if (monsterLoad){

		static uint32_t id = 0;
		if (new_mType){
			std::string lowername = monster_name;
			toLowerCaseString(lowername);

			id++;
			monsterNames[lowername] = id;
			monsters[id] = mType;
		}

		return true;
	}
	else{
		if (new_mType){
			delete mType;
		}
		return false;
	}
}

bool Monsters::loadLootItem(xmlNodePtr node, LootBlock& lootBlock)
{
	int intValue;
	std::string strValue;

	if (readXMLInteger(node, "Id", intValue)){
		lootBlock.id = Item::items.getItemIdByClientId(intValue).id;
	}

	if (lootBlock.id == 0){
		return false;
	}

	if (readXMLInteger(node, "Max", intValue)){
		lootBlock.countmax = intValue;

		if (lootBlock.countmax > 100){
			lootBlock.countmax = 100;
		}
	}
	else {
		lootBlock.countmax = 1;
	}

	if (readXMLInteger(node, "Chance", intValue) || readXMLInteger(node, "chance1", intValue)){
		lootBlock.chance = intValue;

		if (lootBlock.chance > MAX_LOOTCHANCE){
			lootBlock.chance = MAX_LOOTCHANCE;
		}

		if (lootBlock.id == ITEM_COINS_GOLD) {
			if (g_config.getNumber(ConfigManager::GOLD_DROP_CHANCE) > 0) {
				lootBlock.chance = g_config.getNumber(ConfigManager::GOLD_DROP_CHANCE);
			}
		}
	}
	else{
		lootBlock.chance = MAX_LOOTCHANCE;
	}

	if (Item::items[lootBlock.id].isContainer()){
		loadLootContainer(node, lootBlock);
	}

	//optional
	if (readXMLInteger(node, "subtype", intValue)){
		lootBlock.subType = intValue;
	}

	if (readXMLInteger(node, "actionId", intValue)){
		lootBlock.actionId = intValue;
	}

	if (readXMLString(node, "text", strValue)){
		lootBlock.text = strValue;
	}

	lootBlock.dropEmpty = true;
	if (readXMLString(node, "dropEmpty", strValue)){
		lootBlock.dropEmpty = (asLowerCaseString(strValue) == "true");
	}

	return true;
}

bool Monsters::loadLootContainer(xmlNodePtr node, LootBlock& lBlock)
{
	if(node == NULL){
		return false;
	}

	xmlNodePtr tmpNode = node->children;
	xmlNodePtr p;

	if(tmpNode == NULL){
		return false;
	}

	while(tmpNode){
		if(xmlStrcmp(tmpNode->name, (const xmlChar*)"inside") == 0){
			p = tmpNode->children;
			while(p){
				LootBlock lootBlock;
				if(loadLootItem(p, lootBlock)){
					lBlock.childLoot.push_back(lootBlock);
				}
				p = p->next;
			}
			return true;
		}//inside

		tmpNode = tmpNode->next;
	}

	return false;
}

MonsterType* Monsters::getMonsterType(const std::string& name)
{
	uint32_t mId = getIdByName(name);
	if(mId == 0){
		return NULL;
	}

	return getMonsterType(mId);
}

MonsterType* Monsters::getMonsterType(uint32_t mid)
{
	MonsterMap::iterator it = monsters.find(mid);
	if(it != monsters.end()){
		return it->second;
	}
	else{
		return NULL;
	}
}

uint32_t Monsters::getIdByName(const std::string& name)
{
	std::string lower_name = name;
	std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), tolower);
	MonsterNameMap::iterator it = monsterNames.find(lower_name);
	if(it != monsterNames.end()){
		return it->second;
	}
	else{
		return 0;
	}
}

Monsters::~Monsters()
{
	for(MonsterMap::iterator it = monsters.begin(); it != monsters.end(); ++it)
		delete it->second;
}
