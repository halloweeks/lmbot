#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "connection.h"

void RequestGuestLogIn(Connection *conn);
void RequestLogIn(Connection *conn);
void RequestClientInitOver(Connection *conn);
void RequestHeartBeat(Connection *conn);

void RequestTroopTraining(Connection *c, uint8_t kind, uint8_t tier, uint32_t amount);

void RequestTroopRecall(Connection *c, uint8_t Index);
void RequestViewChat(Connection *c, uint8_t channel, uint8_t prev, int8_t kind, int64_t DataID, int64_t DataTime);

void RequestSendChat(Connection *c, uint8_t channel, const char *message);

void RequestRallyList(Connection *c);
void RequestRallyDetail(Connection *c, uint8_t arg1, uint32_t arg2);

void RequestJoinRally(Connection *c, const char *ally_name, const uint32_t troop_array[16]);


void Send_Mall_TestBuy(Connection *c, uint16_t type);

void RequestMapData(Connection *c, uint8_t count, uint16_t zone[]);

void RequestBlackMarketData(Connection *c);
void RequestBlackMarketBuy(Connection *c, uint8_t mIdx);
void RequestSmartUseBlackMarketBuy(Connection *c, SmartUseList smart_use, uint8_t mIdx);
void SendBlackMarketBuy(Connection *c, uint8_t mIdx);

void RequestMissionInfo(Connection *c, uint8_t missionType);
void RequestAllyPoint(Connection *c, const char *name);

void RequestWatchTowerLineDetail(Connection *c, uint32_t);
void RequestTroopTakeBack(Connection*, uint8_t);

void RequestSendHelp(Connection *c, uint16_t record_sn_count, const uint32_t *record_sn);
void SendStartBuilding(Connection *c, uint16_t position_id, uint16_t build_id, uint8_t operation_type);
void ServerNewbieTeleport(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id);
void ServerRelocate(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id);
void RequestAllianceGiftInfo(Connection*);
void RequestOpenAllianceGift(Connection*, uint32_t);

void RequestDeleteAllianceGiftBox(Connection*, uint32_t);

void ServerRename(Connection *c, bool bought, uint16_t num, const char *name);
void RequestBuyItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty);
void RequestBuyGiftItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty, const char Name[13]);


void RequestSimpleUseItem(Connection *c, uint32_t item_id, uint16_t quantity);
void RecvUseItem(Connection *c, const uint8_t *data, uint16_t);

void ServerMagicGateDoEvent(Connection *c, uint16_t n, uint8_t x);

void RequsetWorldTeleportItemCount(Connection *c, uint64_t Power);


void RecvLoginError(Connection *c, const uint8_t *data);
void HandleLoginValidate(Connection *c, const uint8_t *data, uint16_t size);
void RecvChatMessage(Connection *c, const uint8_t *data);

void RecvAllBuildData(Connection *c, const uint8_t *data);

void RecvItemInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvIBuffInfo(Connection *c, const uint8_t *data);
void RecvMarchData(Connection*, const uint8_t*);
void RecvLoginRoleInfo(Connection *c, const uint8_t *data, uint16_t size);

void RecvResources(Connection *c, const uint8_t *data);

void RecvBlackMarket_Data(Connection *c, const uint8_t *data);
void RecvBlackMarket_Buy(Connection *c, const uint8_t *data);
void RecvMailInfo(Connection *c, const uint8_t *data);


void RecvAllyPoint(Connection *c, const uint8_t *data);

void RecvAllianceHelp(Connection *c, const uint8_t *data);
void RecvAllianceMemberNeedsHelp(Connection *c, const uint8_t *data);
void RecvPendingAllianceMembersNeedHelp(Connection *c, const uint8_t *data);
void RecvAllianceGiftInfo(Connection *c, const uint8_t *data);
void RecvRoleUpdateInfo(Connection *, const uint8_t*);

void RecvAllianceGiftOpen(Connection *c, const uint8_t *data);
void RecvDeleteAllianceGiftBox(Connection*, const uint8_t*);

void RecvBuyItem(Connection *c, const uint8_t *data, uint16_t size);
void RecvArmyGroupInfo(Connection *c, const uint8_t *data);
void RecvWoundedTroopData(Connection *c, const uint8_t *data);

void RecvRefreshResources(Connection *c, const uint8_t *data);

void HeartbeatTick(Connection *c);
void BlackMarketTick(Connection *c);
void ShieldTick(Connection *c);
void AllianceGiftTick(Connection*);
void RecvAllianceInfo(Connection*, const uint8_t*);

void RecvBuildingQueue(Connection*, const uint8_t*);

void RecvUpdateWatchTowerAddLineInfo(Connection*, const uint8_t*);
void RecvWatchTowerLineDetail(Connection *c, const uint8_t *data);
const char *FormatTime(uint32_t totalSecs);


void RecvDarknestBroadcast(Connection *c, const uint8_t *data);

void RecvRallyCountData(Connection*, const uint8_t*);

void DarknestRallyTick(Connection *);

void RecvWarBegin(Connection *c, const uint8_t *data);

void RecvNPCWallHallData(Connection *c, const uint8_t *data);
void RecvWallHallTroop(Connection*, const uint8_t*);
void RecvNPCWallHallDetail(Connection*, const uint8_t*);
void RecvWallHallDel(Connection *c, const uint8_t *data);
void RecvWallHallData(Connection *c, const uint8_t *data);
void RecvWallHallDetail(Connection *c, const uint8_t *data);
void RecvTechnologyInfo(Connection*, const uint8_t*, uint16_t);


void RecvAddConflictLine(Connection *c, const uint8_t *data);
void RecvMapInfoPlus(Connection *c, const uint8_t *data, uint16_t size);

void RecvMagicGateDoEvent(Connection *c, const uint8_t *data);

void RequestUnknown(Connection *c);
void RecvWallHallDetailClose(Connection *c, const uint8_t *data);


void RecvJoinedRallyData(Connection *c, const uint8_t *data);

void ResourceTransferTick(Connection *c);


void RequestSendMail(Connection *c, const char *player_name, const char *subject, const char *message);
void RequestSendMailFmt(Connection *c, const char *player_name, const char *subject, const char *fmt, ...);

void RecvSHelp(Connection *c, const uint8_t *data);
void RecvHelp_Home(Connection *c, const uint8_t *data);


void format_number2(uint64_t num, char *out, size_t size);

#endif