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

#include "ban.h"
#include "tools.h"
#include "configmanager.h"
#include "ioplayer.h"
#include "database.h"

extern ConfigManager g_config;

uint32_t BanManager::getNotationsCount(uint32_t accountId) const {
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	int32_t t = 0;

	//Check for banishments
	query <<
		"SELECT "
		"COUNT(*) AS `count` "
		"FROM "
		"`account_notations` "
		"WHERE "
		"`account` = " << accountId;

	if ((result = db->storeQuery(query.str()))) {
		t += result->getDataInt("count");
		db->freeResult(result);
	}

	return t;
}

bool BanManager::isPlayerNameReported(std::string player) const {
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	int32_t t = 0;

	//Check for banishments
	query <<
		"SELECT "
		"COUNT(*) AS `count` "
		"FROM "
		"`name_reports` "
		"WHERE "
		"`player` = " << db->escapeString(player);

	if ((result = db->storeQuery(query.str()))) {
		t += result->getDataInt("count");
		db->freeResult(result);
	}

	return t > 0;
}
bool BanManager::isPlayerBanished(std::string player) const {
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	int32_t t = 0;

	//Check for banishments
	query <<
		"SELECT "
		"COUNT(*) AS `count` "
		"FROM "
		"`player_banishments` "
		"WHERE "
		"`player` = " << db->escapeString(player);

	if ((result = db->storeQuery(query.str()))) {
		t += result->getDataInt("count");
		db->freeResult(result);
	}

	return t > 0;
}

bool BanManager::addPlayerBanishment(std::string player, std::string gamemaster, int64_t expirationdate, std::string reason, std::string action, std::string comment, std::string statement) const {
	if (player.length() < 1) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `player_banishments` (`date`, `player`, `gamemaster`, `expirationdate`, `reason`, `action`, `comment`, `statement`) VALUES ");

	DBQuery query;
	query << std::time(NULL) << ", " << db->escapeString(player) << ", " << db->escapeString(gamemaster) << ", " << expirationdate << ", " << db->escapeString(reason)
		<< ", " << db->escapeString(action) << ", "
		<< db->escapeString(comment) << ", " << db->escapeString(statement);

	if (!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::addPlayerStatement(std::string player, std::string gamemaster, std::string statement, std::string comment, std::string reason) const
{
	if (player.length() < 1) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `statement_reports` (`date`, `gamemaster`, `player`, `statement`, `comment`, `reason`) VALUES ");

	DBQuery query;
	query << std::time(NULL) << ", " << db->escapeString(gamemaster) << ", " << db->escapeString(player) << ", " << db->escapeString(statement) 
		<< ", " << db->escapeString(comment) << ", "
		<< db->escapeString(reason);

	if (!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::addNameReport(std::string player, std::string gamemaster, std::string comment, std::string reason) const {
	if (player.length() < 1) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `name_reports` (`date`, `gamemaster`, `player`, `comment`, `reason`) VALUES ");

	DBQuery query;
	query << std::time(NULL) << ", " << db->escapeString(gamemaster) << ", " << db->escapeString(player) << ", " << db->escapeString(comment) << ", "
		<< db->escapeString(reason);

	if (!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::addAccountBanishment(uint32_t account, std::string gamemaster, std::string comment, std::string reason, int64_t expirationdate, int64_t deletiondate, std::string action) const {
	if (account < 1) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `account_banishments` (`date`, `account`, `gamemaster`, `comment`, `reason`, `action`, `expirationdate`, `deletiondate`) VALUES ");

	DBQuery query;
	query << std::time(NULL) << ", " << account << ", " << db->escapeString(gamemaster) << ", " << db->escapeString(comment) << ", "
		<< db->escapeString(reason) << ", " << db->escapeString(action) << ", " << expirationdate << ", " << deletiondate;

	if (!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::isAccountBanished(uint32_t accountId) const {
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	int32_t t = 0;

	//Check for banishments
	query <<
		"SELECT "
		"COUNT(*) AS `count` "
		"FROM "
		"`account_banishments` "
		"WHERE "
		"`account` = " << accountId;

	if ((result = db->storeQuery(query.str()))) {
		t += result->getDataInt("count");
		db->freeResult(result);
	}

	//Check if account is blocked
	query.str("");
	query <<
		"SELECT "
		"COUNT(*) AS `count` "
		"FROM "
		"`accounts` "
		"WHERE "
		"`id` = " << accountId << " AND "
		"`blocked` = 1";

	if ((result = db->storeQuery(query.str()))) {
		t += result->getDataInt("count");
		db->freeResult(result);
	}

	return t > 0;
}

bool BanManager::clearTemporaryBans() const
{
	Database* db = Database::instance();
	DBQuery query;

	query << "DELETE FROM `player_banishments` WHERE `expirationdate` < " << time(NULL);
	db->executeQuery(query.str());

	query.str("");

	query << "DELETE FROM `account_banishments` WHERE `expirationdate` > -1 AND `expirationdate` < " << time(NULL) << " OR `deletiondate` < " << time(NULL) << " AND `deletiondate` > -1";
	db->executeQuery(query.str());

	query.str("");

	query << "DELETE FROM `account_notations` WHERE `expirationdate` < " << time(NULL);
	db->executeQuery(query.str());

	query.str("");

	query << "DELETE FROM `ip_banishments` WHERE `time` < " << time(NULL);
	db->executeQuery(query.str());

	return true;
}

bool BanManager::acceptConnection(uint32_t clientip)
{
	if(clientip == 0) return false;
	banLock.lock();

	uint64_t currentTime = OTSYS_TIME();
	IpConnectMap::iterator it = ipConnectMap.find(clientip);
	if(it == ipConnectMap.end()){
		ConnectBlock cb;
		cb.startTime = currentTime;
		cb.blockTime = 0;
		cb.count = 1;

		ipConnectMap[clientip] = cb;
		banLock.unlock();
		return true;
	}

	it->second.count++;
	if(it->second.blockTime > currentTime){
		banLock.unlock();
		return false;
	}

	if(currentTime - it->second.startTime > 1000){
		uint32_t connectionPerSec = it->second.count;
		it->second.startTime = currentTime;
		it->second.count = 0;
		it->second.blockTime = 0;

		if(connectionPerSec > 10){
			it->second.blockTime = currentTime + 10000;
			banLock.unlock();
			return false;
		}
	}

	banLock.unlock();
	return true;
}

bool BanManager::isIpDisabled(uint32_t clientip)
{
	if (g_config.getNumber(ConfigManager::IP_LOGIN_TRIES) == 0 || clientip == 0) return false;
	banLock.lock();

	time_t currentTime = std::time(NULL);
	IpLoginMap::iterator it = ipLoginMap.find(clientip);
	if(it != ipLoginMap.end()){
		uint32_t loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
		if ((it->second.numberOfLogins >= (uint32_t)g_config.getNumber(ConfigManager::IP_LOGIN_TRIES)) &&
			((uint32_t)currentTime < (uint32_t)it->second.lastLoginTime + loginTimeout) )
		{
			banLock.unlock();
			return true;
		}
	}

	banLock.unlock();
	return false;
}

bool BanManager::isIpBanished(uint32_t clientip, uint32_t mask /*= 0xFFFFFFFF*/) const
{
	if(clientip == 0) return false;
	Database* db = Database::instance();

	DBQuery query;
	query <<
		"SELECT "
			"COUNT(*) AS `count` "
		"FROM "
			"`ip_banishments` "
		"WHERE "
			"`mask` = " << mask << " AND `ip` = " << clientip;

	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	int t = result->getDataInt("count");
	db->freeResult(result);
	return t > 0;
}

bool BanManager::isAccountLocked(uint32_t account) {
	if (g_config.getNumber(ConfigManager::LOGIN_TRIES) == 0 || account == 0) return false;
	banLock.lock();

	time_t currentTime = std::time(NULL);
	accountLoginMap::iterator it = accountloginMap.find(account);
	if (it != accountloginMap.end()) {
		uint32_t loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::ACCOUNT_LOCK_DURATION) / 1000;
		if ((it->second.numberOfLogins >= (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES)) &&
			((uint32_t)currentTime < (uint32_t)it->second.lastLoginTime + loginTimeout)) {
			banLock.unlock();
			return true;
		}
	}

	banLock.unlock();
	return false;
}

void BanManager::addAccountLoginAttempt(uint32_t account, bool isSuccess) {
	if (account == 0) return;
	banLock.lock();

	time_t currentTime = std::time(NULL);
	accountLoginMap::iterator it = accountloginMap.find(account);
	if (it == accountloginMap.end()) {
		LoginBlock lb;
		lb.lastLoginTime = 0;
		lb.numberOfLogins = 0;

		accountloginMap[account] = lb;
		it = ipLoginMap.find(account);
	}

	if (it->second.numberOfLogins >= (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES))
		it->second.numberOfLogins = 0;

	uint32_t retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
	if (!isSuccess || ((uint32_t)currentTime < (uint32_t)it->second.lastLoginTime + retryTimeout))
		++it->second.numberOfLogins;
	else
		it->second.numberOfLogins = 0;

	it->second.lastLoginTime = currentTime;
	banLock.unlock();
}

void BanManager::addLoginAttempt(uint32_t clientip, bool isSuccess)
{
	if(clientip == 0) return;
	banLock.lock();

	time_t currentTime = std::time(NULL);
	IpLoginMap::iterator it = ipLoginMap.find(clientip);
	if(it == ipLoginMap.end()){
		LoginBlock lb;
		lb.lastLoginTime = 0;
		lb.numberOfLogins = 0;

		ipLoginMap[clientip] = lb;
		it = ipLoginMap.find(clientip);
	}

	if (it->second.numberOfLogins >= (uint32_t)g_config.getNumber(ConfigManager::IP_LOGIN_TRIES))
		it->second.numberOfLogins = 0;

	uint32_t retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
	if(!isSuccess || ((uint32_t)currentTime < (uint32_t)it->second.lastLoginTime + retryTimeout))
		++it->second.numberOfLogins;
	else
		it->second.numberOfLogins = 0;

	it->second.lastLoginTime = currentTime;
	banLock.unlock();
}

bool BanManager::addIpBan(uint32_t ip, uint32_t mask, int32_t time,
	std::string gamemaster, std::string comment) const
{
	if(ip == 0 || mask == 0) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `ip_banishments` (`date`, `mask`, `time`, `gamemaster`, `comment`, `ip`) VALUES ");

	DBQuery query;
	query << std::time(NULL) << ", " << mask << ", " << time << ", " << db->escapeString(gamemaster) << ", ";
	query << db->escapeString(comment) << ", " << ip;

	if(!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::addAccountNotation(uint32_t account, std::string comment, std::string reason, int64_t expirationdate) const
{
	if(account == 0) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `account_notations` (`date`, `account`, `comment`, `reason`, `expirationdate`) VALUES ");

	DBQuery query;
	query << time(NULL) << ", " << account << ",  " << db->escapeString(comment) << ", " << db->escapeString(reason) << ", " << expirationdate;

	if(!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::removeIpBans(uint32_t ip, uint32_t mask) const
{
	if(!isIpBanished(ip, mask)) return false;
	Database* db = Database::instance();

	DBQuery query;
	query << "DELETE FROM `ip_banishments` WHERE mask = " << mask << " AND ip = " << ip;

	return db->executeQuery(query.str());
}

bool BanManager::removePlayerBans(const std::string& name) const
{
	Database* db = Database::instance();

	DBQuery query;
	query << "DELETE FROM player_banishments WHERE player = " << db->escapeString(name);
	return db->executeQuery(query.str());
}

bool BanManager::removeAccountBans(uint32_t accno) const
{
	if(!isAccountBanished(accno)) return false;
	Database* db = Database::instance();

	DBQuery query;
	query << "DELETE FROM account_banishments WHERE account = " << accno;
	return db->executeQuery(query.str());
}

bool BanManager::removeNotations(uint32_t accno) const
{
	Database* db = Database::instance();
	DBQuery query;

	query << "DELETE FROM `account_notations` WHERE `account` = " << accno;
	return db->executeQuery(query.str());
}