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

#ifndef __OTSERV_BAN_H__
#define __OTSERV_BAN_H__

#include "definitions.h"
#include "enums.h"
#include <boost/thread.hpp>
#include <list>
#include <vector>

enum BanType_t {
	BAN_IPADDRESS = 1,
	BAN_PLAYER = 2,
	BAN_ACCOUNT = 3,
	BAN_NOTATION = 4,
	BAN_STATEMENT = 5,
	BAN_NAMELOCK = 6
};

struct Ban {
	BanType_t type;
	uint32_t id;
	uint32_t added;
	int32_t expires;
	uint32_t adminId;
	uint32_t reason;
	violationAction_t action;
	std::string comment;
	std::string statement;
	std::string value;
	std::string param;
};

struct LoginBlock {
	time_t lastLoginTime;
	uint32_t numberOfLogins;
};

struct ConnectBlock {
	uint64_t startTime;
	uint64_t blockTime;
	uint32_t count;
};

typedef std::map<uint32_t, LoginBlock > IpLoginMap;
typedef std::map<uint32_t, LoginBlock > accountLoginMap;
typedef std::map<uint32_t, ConnectBlock > IpConnectMap;

class BanManager {
public:
	BanManager() {}
	~BanManager() {}

	bool clearTemporaryBans() const;
	bool acceptConnection(uint32_t clientip);

	bool isIpDisabled(uint32_t clientip);
	bool isAccountLocked(uint32_t account);
	bool isIpBanished(uint32_t clientip, uint32_t mask = 0xFFFFFFFF) const;

	// Renewed functions
	bool addPlayerStatement(std::string player, std::string gamemaster, std::string statement, std::string comment, std::string reason) const;
	bool addNameReport(std::string player, std::string gamemaster, std::string comment, std::string reason) const;
	bool addAccountBanishment(uint32_t account, std::string gamemaster, std::string comment, std::string reason, int64_t expirationdate, int64_t deletiondate, std::string action) const;
	bool addPlayerBanishment(std::string player, std::string gamemaster, int64_t expirationdate, std::string reason, std::string action, std::string comment, std::string statement) const;
	uint32_t getNotationsCount(uint32_t account) const;
	bool addAccountNotation(uint32_t account, std::string comment, std::string reason, int64_t expirationdate) const;
	bool addIpBan(uint32_t ip, uint32_t mask, int32_t time, std::string gamemaster, std::string comment) const;

	bool isAccountBanished(uint32_t accountId) const;
	bool isPlayerBanished(std::string player) const;
	bool isPlayerNameReported(std::string player) const;

	bool removeNotations(uint32_t accno) const;
	bool removeAccountBans(uint32_t accno) const;
	bool removePlayerBans(const std::string& name) const;
	bool removeIpBans(uint32_t ip, uint32_t mask = 0xFFFFFFFF) const;

	void addLoginAttempt(uint32_t clientip, bool isSuccess);
	void addAccountLoginAttempt(uint32_t account, bool isSuccess);

protected:
	mutable boost::recursive_mutex banLock;

	IpLoginMap ipLoginMap;
	accountLoginMap accountloginMap;
	IpConnectMap ipConnectMap;
};

#endif
