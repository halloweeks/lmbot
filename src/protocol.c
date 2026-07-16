#include "protocol.h"
#include "net_rw.h"
#include "connection.h"
#include "packet_enum.h"
#include "map_point.h"
#include "items.h"
#include "log.h"
#include <stdlib.h>

#include "MAP_UPDATE_KIND.h"

// Bootstrap login
void RequestGuestLogIn(Connection *c)
{
	// reserve space for packet length
    c->size = 2;
    
    // write packet type 
    write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOL);
    c->size += 2;
    
    // write igg id
    write_u64(c->data + c->size, c->auth.igg_id);
    c->size += 8;
    
    // write game minor version 
    write_u8(c->data + c->size, c->app.version_minor);
    c->size += 1;
    
    // write game major version
    write_u8(c->data + c->size, c->app.version_major);
    c->size += 1;
    
    // write game patch version
    write_u16(c->data + c->size, c->app.version_patch);
    c->size += 2;

    write_u8(c->data + c->size, 1);
    c->size += 1;
    
    // write language code
    write_u8(c->data + c->size, c->app.language_code);
    c->size += 1;
    
    // write Device Universally Unique Identifier.
    write_raw(c->data + c->size, c->auth.device_uuid, 50);
    c->size += 50;
    
    // write session length
    write_u16(c->data + c->size, c->auth.session_len);
    c->size += 2;
    
    // write session token
    write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;
    
    // rewrite total packet length 
    write_u16(c->data, c->size);
    
    // call send packet
    send_packet(c, false);
}


// Game login
void RequestLogIn(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOP);
	c->size += 2;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	// write 50 byte zero 
	for (int i = 0; i < 25; i++) {
		write_u16(c->data + c->size, 0);
		c->size += 2;
	}
	
	// write version 
	write_u32(c->data + c->size, 0);
	c->size += 4;
	
	// battle_is_oul
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// b_recv_kingdom
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// session len
	write_u16(c->data + c->size, c->auth.session_len);
	c->size += 2;
	
	// session 
	write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;

    write_u16(c->data, c->size);
    
    send_packet(c, false);
}

// 
void RequestClientInitOver(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_CLIENTINITOVER);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	write_u16(c->data, c->size);
    
    send_packet(c, true);
}

void RequestHeartBeat(Connection *c) {
	// reserve space for packet length
	c->size = 2;
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_ACTIVE);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// update packet length 
	write_u16(c->data, c->size);
    
    send_packet(c, true);
}

void RequestSimpleUseItem(Connection *c, uint32_t item_id, uint16_t quantity) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write item id
	write_u16(c->data + c->size, item_id);
	c->size += 2;
	
	// write item quantity
	write_u16(c->data + c->size, quantity);
	c->size += 2;
	
	// 10 byte zero
	write_zero(c->data + c->size, 10); c->size += 10;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true); // true is encryption flag
}

void RequestUseAdvancedRelocator(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write item id
	write_u16(c->data + c->size, ADVANCE_RELOCATOR);
	c->size += 2;
	
	// write item quantity
	write_u16(c->data + c->size, 0x0001);
	c->size += 2;
	
	// write kingdom id
	write_u16(c->data + c->size, kingdom_id);
	c->size += 2;
	
	// write zone id
	write_u16(c->data + c->size, zone_id);
	c->size += 2;
	
	// write point id
	write_u8 (c->data + c->size, point_id);
	c->size += 1;
	
	// nothing just write 5 byte zero 
	write_zero(c->data + c->size, 5); c->size += 5;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true);
}

void RequestMapData(Connection *c, uint8_t count, uint16_t zone[]) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_MAPDATA);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// count
	write_u8 (c->data + c->size, count);
	c->size += 1;
	
	// zone id
	write_u16(c->data + c->size, zone[0]);
	c->size += 2;
	
	write_u16(c->data + c->size, zone[1]);
	c->size += 2;
	
	write_u16(c->data + c->size, zone[2]);
	c->size += 2;
	
	write_u16(c->data + c->size, zone[3]);
	c->size += 2;
	
	write_zero(c->data + c->size, 32);
	c->size += 32;
	
	write_u16(c->data,  c->size);
	
	send_packet(c, true);
}


void GetBlackMarketData(Connection *c) {
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUSET_BLACKMARKET_DATA); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u8 (c->data + c->size, 1); c->size += 1;
	write_u16(c->data,  c->size);
	
	send_packet(c, true);
}

void SendBlackMarketBuy(Connection *c, uint8_t mIdx) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_BLACKMARKET_BUY);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write slot
	write_u8(c->data + c->size, mIdx);
	c->size += 1;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true);
}

void RequestSmartUseBlackMarketBuy(Connection *c, SmartUseList smart_use, uint8_t mIdx) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_SMARTUSE_FOR_BLACKMARKET);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write slot
	write_u8(c->data + c->size, mIdx);
	c->size += 1;
	
	// number of items required 
	write_u16(c->data + c->size, c->smart_use.count);
	c->size += 2;
	
	for (int i = 0; i < c->smart_use.count; i++) {
		// item id
		write_u16(c->data + c->size, c->smart_use.items[i].id); c->size += 2;
		// quantity 
		write_u16(c->data + c->size, c->smart_use.items[i].qty); c->size += 2;
	}
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true);
}

void RequestTroopTraining(Connection *c, uint8_t kind, uint8_t tier, uint32_t amount) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_TRAINING_); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);   c->size += 4;
	write_u8 (c->data + c->size, kind);                   c->size += 1;// write RD_Kind troop type (infantry, ranged, cavalry, siege)
	write_u8 (c->data + c->size, tier);                   c->size += 1;// write RD_Rank (server expects 0-based) (t1, t2, t3, t4, t5)
	write_u32(c->data + c->size, amount);                 c->size += 4;// write troop amount 
	write_u16(c->data, c->size); // update packet size
	
	send_packet(c, true);
}

void CancelTroopTraining(Connection *c)
{
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_CANCELTRAINING); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);        c->size += 4;
	write_u16(c->data, c->size); // update packet size
	
	send_packet(c, true);
}

typedef struct {
	uint32_t food;
	uint32_t rock;
	uint32_t wood;
	uint32_t ore;
	uint32_t gold;
} Resources;


bool SendResource(Connection *c, Resources resource, uint16_t zoneId, uint8_t pointId) {
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_SEND_RESHELP);   c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);                 c->size += 4;
	
	write_u16(c->data + c->size, zoneId);  c->size += 2;
	write_u8 (c->data + c->size, pointId); c->size += 1;
	
	write_u32(c->data + c->size, resource.food); c->size += 4;
	write_u32(c->data + c->size, resource.rock); c->size += 4;
	write_u32(c->data + c->size, resource.wood); c->size += 4;
	write_u32(c->data + c->size, resource.ore);  c->size += 4;
	write_u32(c->data + c->size, resource.gold); c->size += 4;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
	return 1;
}

void RequestMissionInfo(Connection *c, uint8_t missionType) {
	if (missionType != 0 && missionType != 1) return;
	
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_MISSIONINFO);   c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);       c->size += 4;
	write_u8 (c->data + c->size, (missionType + 1)); c->size += 1;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
	return;
}

void RequestAllyPoint(Connection *c, const char *name) 
{
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_ALLYPOINT);   c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);     c->size += 4;
	write_raw(c->data + c->size, name, 13);                 c->size += 13;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}

void RequestSendChat(Connection *c, uint8_t channel, const char *message) {
	uint16_t message_len = (uint16_t)strlen(message);
	
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_SENDCHAT);
	c->size += 2;
	
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	write_u8 (c->data + c->size, channel);
	c->size += 1;
	
	write_u8 (c->data + c->size, 0);
	c->size += 1;
	
	write_u8 (c->data + c->size, 5);
	c->size += 1;
	
	write_u16(c->data + c->size, message_len);
	c->size += 2;
	
	write_raw(c->data + c->size, message, message_len);
	c->size += message_len;
	
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}


void RequestViewChat(Connection *c, uint8_t channel, uint8_t prev, int8_t kind, int64_t DataID, int64_t DataTime) {
	c->size = 2;
	
	// packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_VIEWCHAT);
	c->size += 2;
	
	// sequence id
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// channel type 
	write_u8 (c->data + c->size, channel);
	c->size += 1;
	
	// previous 
	write_u8 (c->data + c->size, prev);
	c->size += 1;
	
	if (c->app.version_major != 0) {
		write_u8(c->data + c->size, (kind == -1) ? 0xFF : (uint8_t)kind);
		c->size += 1;
		/*
		if (kind == -1) {
			write_u8 (c->data + c->size, 0xFF);
			c->size += 1;
		} else {
			write_u8 (c->data + c->size, (uint8_t)kind);
			c->size += 1;
		}
		*/
	}
	
	if (channel != 0) {
		write_u64(c->data + c->size, DataID);
		c->size += 8;
		
		write_u64(c->data + c->size, DataTime);
		c->size += 8;
	}
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}


void RequestHelpAllianceMember(Connection *c, uint16_t record_sn_count, const uint32_t *record_sn)
{
	c->size = 2;
	
	write_u16(c->data + c->size, 0x0b27);
	c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	write_u16(c->data + c->size, record_sn_count);
	c->size += 2;
	
	for (uint16_t i = 0; i < record_sn_count; i++) {
		write_u32(c->data + c->size, record_sn[i]);
		c->size += 4;
	}
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}

void SendStartBuilding(Connection *c,
                       uint16_t position_id,
                       uint16_t build_id,
                       uint8_t operation_type)
{
    c->size = 2;

    write_u16(c->data + c->size, _MSG_REQUEST_BUILDBEGIN);
    c->size += 2;

    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    write_u16(c->data + c->size, position_id);
    c->size += 2;

    write_u16(c->data + c->size, build_id);
    c->size += 2;

    write_u8(c->data + c->size, operation_type);
    c->size += 1;

    write_u16(c->data, c->size);

    send_packet(c, true);
}

void ServerNewbieTeleport(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
    c->size += 2;
    
    // sequence id 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // item id
    write_u16(c->data + c->size, 0x03ed);
    c->size += 2;
    
    // item quantity 
    write_u16(c->data + c->size, 0x0001);
    c->size += 2;
    
    // Kingdom id
    write_u16(c->data + c->size, kingdom_id);
    c->size += 2;
    
    // zone id
    write_u16(c->data + c->size, zone_id);
    c->size += 2;
    
    // point id
    write_u8(c->data + c->size, point_id);
    c->size += 1;
    
    // zero
    write_zero(c->data + c->size, 5);
    c->size += 5;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}



void ServerRename(Connection *c, bool bought, uint16_t num, const char *name) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, 0x0450);
    c->size += 2;
    
    // sequence id
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // bought 
    write_u8(c->data + c->size, bought);
    c->size += 1;
    
    // num
    write_u16(c->data + c->size, num);
    c->size += 2;
    
    // item id
    write_u16(c->data + c->size, 0x03ee);
    c->size += 2;
    
    // name length 
    write_u8(c->data + c->size, strlen(name));
    c->size += 1;
    
    write_raw(c->data + c->size, name, 12); 
    c->size += 12;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
	
}

void RequestAllianceGiftInfo(Connection *c) {
	c->size = 2;

    write_u16(c->data + c->size, 0x0B2E);
    c->size += 2;

    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void RequestOpenAllianceGift(Connection *c, uint32_t SN) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, 0x0b30);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // SN
    write_u32(c->data + c->size, SN);
    c->size += 4;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void ServerMagicGateDoEvent(Connection *c, uint16_t n, uint8_t x) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, 0x2dd4);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // 
    write_u16(c->data + c->size, n);
    c->size += 2;
    
    write_u8(c->data + c->size, x);
    c->size += 1;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void RequestBuyItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_BUYITEM);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Type
    write_u8(c->data + c->size, Type);
    c->size += 1;
    
    // key
    write_u16(c->data + c->size, Key);
    c->size += 2;
    
    // item id
    write_u16(c->data + c->size, ItemID);
    c->size += 2;
    
    // quantity
    write_u16(c->data + c->size, Qty);
    c->size += 2;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
    // Debug printf("[INFO] RequestBuyItem(c, %u, %u, %u, %u)\n", Type, Key, ItemID, Qty);
}

void RequestBuyGiftItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty, const char Name[13]) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_GIFT);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Type
    write_u8(c->data + c->size, Type);
    c->size += 1;
    
    // key
    write_u16(c->data + c->size, Key);
    c->size += 2;
    
    // item id
    write_u16(c->data + c->size, ItemID);
    c->size += 2;
    
    // name
    write_raw(c->data + c->size, Name, 13);
    c->size += 13;
    
    // quantity
    write_u16(c->data + c->size, Qty);
    c->size += 2;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
    // Debug printf("[INFO] ServerBuyItem(c, %u, %u, %u, %s, %u)\n", Type, Key, ItemID, Name, Qty);
}

// It's for calculate how many migration scrolls require 
void RequsetWorldTeleportItemCount(Connection *c, uint64_t Power)
{
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_WORLD_TELEPORT_ITEM);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Power
    write_u64(c->data + c->size, Power);
    c->size += 8;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void RequestDeleteAllianceGiftBox(Connection *c, uint32_t sn) {
	c->size = 2; // reserved 2 byte for packet length 
	
	// packet type 
	write_u16(c->data + c->size, 0x0b32);
	c->size += 2;
	
	// Sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// Serial Number 
	write_u32(c->data + c->size, sn);
	c->size += 4;
	
	/*
	// unknown! no information about this field 
	write_u8(c->data + c->size, 0);
	c->size += 1;
	*/
	
	// Update packet size
	write_u16(c->data, c->size);
	
	send_packet(c, true);
}

/*
 * Returns troops that have already reached their destination
 * (e.g. gathering, camping, or reinforcing) back to the castle.
 */
void RequestTroopTakeBack(Connection *c, uint8_t Index) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_TROOPRETURN);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Index
    write_u32(c->data + c->size, Index);
    c->size += 4;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

/*
 * Recalls a march before it reaches its destination.
 * Requires a Withdraw Squad item.
 */
void RequestTroopRecall(Connection *c, uint8_t Index) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write item id
	write_u16(c->data + c->size, WITHDRAW_SQUAD);
	c->size += 2;
	
	// write item quantity
	write_u16(c->data + c->size, 1);
	c->size += 2;
	
	// write index
	write_u8(c->data + c->size, Index);
	c->size += 1;
	
	// 9 byte zero
	write_zero(c->data + c->size, 9); c->size += 9;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true); // true is encryption flag
}


void RequestJoinRally(Connection *c, const char *ally_name, const uint32_t troop_array[16])
{
    c->size = 2;

    // Packet type.
    write_u16(c->data + c->size, _MSG_REQUEST_JOIN_RALLY);
    c->size += 2;
	
    // Sequence ID.
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    // Rally leader name (13 bytes).
    write_raw(c->data + c->size, ally_name, 13);
    c->size += 13;

    // Build troop mask.
    uint32_t troop_mask = 0;

    for (int i = 0; i < 16; i++) {
        if (troop_array[i] != 0)
            troop_mask |= (1u << i);
    }

    // Write troop mask.
    write_u32(c->data + c->size, troop_mask);
    c->size += 4;

    // Write only selected troop counts.
    for (int i = 0; i < 16; i++) {
        if (troop_array[i] == 0)
            continue;

        write_u32(c->data + c->size, troop_array[i]);
        c->size += 4;
    }

    // Unknown flag (currently always 0).
    write_u8(c->data + c->size, 0);
    c->size += 1;

    // Packet length.
    write_u16(c->data, c->size);
    
    send_packet(c, true);
}

// Fetch all active rallies 
void RequestRallyList(Connection *c) {
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, _MSG_REQUEST_WARHALL_LIST);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
}

void RequestRallyDetail(Connection *c, uint8_t arg1, uint32_t arg2) {
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, _MSG_REQUEST_WARHALL_LIST_DETAIL);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// zero?
	write_u8(c->data + c->size, arg1);
	c->size += 1;
	
	// index
	write_u32(c->data + c->size, arg2);
	c->size += 4;
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
}

void Send_Mall_TestBuy(Connection *c, uint16_t type)
{
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, type);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	
	write_zero(c->data + c->size, 50);
	c->size += 50;
	
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
	
}

void RequestUnknown(Connection *c) {
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, 0x252d);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
	
	// 09 00 2d 25 0a 00 00 00 00 
}

void RecvBuyItem(Connection *c, const uint8_t *data, uint16_t size) {
	return; // exit 
	
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	printf("\nRecvBuyItem\n");
	printf("b: %u\n", b);
	printf("payload length: %u\n", size);
	
	// if (b != 0) return;
	
	uint8_t b2 = read_u8(data + offset); offset += 1;
	uint16_t type = read_u16(data + offset); offset+= 2;
	uint16_t item_id = read_u16(data + offset); offset+= 2;
	uint16_t quantity = read_u16(data + offset); offset+= 2;
	
	printf("b2: %u\n", b2);
	printf("type: %u\n", type);
	printf("item_id: %u\n", item_id);
	printf("quantity: %u\n", quantity);
	
	// if (b == 0) exit(0);
}

void HandleLoginValidate(Connection *c, const uint8_t *data, uint16_t size)
{
	uint16_t offset = 0;
	
	c->game_server.port = read_i32(data + offset); offset += 4;
	c->auth.igg_id      = read_i64(data + offset); offset += 8;
	read_bytes(c->game_server.addr, data + offset, 16); offset += 16;
	c->lobby_login = 1;
}


void RecvChatMessage(Connection *c, const uint8_t *data) {
	char player_name[13] = {0};
	char title_name[3] = {0};
	char str1[20] = {0};
	char str2[20] = {0};
	char message[4096] = {0};
	uint16_t offset = 0;
	
	memset(c->chat.player_name, 0,   13);
	memset(c->chat.message,     0, 4096);
	
	
	uint8_t b2 = read_u8(data + offset);
	offset += 1;
	
	if (c->app.version_major != 0) {
		uint8_t num3 = read_u8(data + offset);
		offset += 1;
	}
	
	if (b2 == 0 || b2 == 1) {
		uint16_t num4 = read_u16(data + offset);
		offset += 2;
		// printf("num4: %u\n", num4);
		
		for (uint16_t i = 0; i < num4; i++) {
			int64_t num5 = read_u64(data + offset);
			offset += 8;
			int64_t num6 = read_u64(data + offset);
			offset += 8;
			int64_t num7 = read_u64(data + offset);
			offset += 8;
			uint8_t alli_or_king = read_u8(data + offset);
			offset += 1;
			uint8_t num8 = read_u8(data + offset);
			offset += 1;
			uint16_t pic_id = read_u16(data + offset);
			offset+= 2;
			read_bytes(c->chat.player_name, data + offset, 13);
			offset += 13;
			uint8_t vip_rank = read_u8(data + offset);
			offset+= 1;
			read_bytes(title_name, data + offset, 3);
			offset += 3;
			uint8_t special_block_id = read_u8(data + offset);
			offset += 1;
			uint8_t title_id = read_u8(data + offset);
			offset += 1;
			uint8_t b_have_arabic = read_u8(data + offset);
			offset += 1;
			uint16_t num9 = read_u16(data + offset);
			offset += 2;
					
			// printf("playID: %lu\n", num6);
			// printf("player_name: %s\n", player_name);
			// printf("vip_rank: %u\n", vip_rank);
			
			if (num8 == 108) {
				int message_talk_kind = 3;
				uint16_t message_kingdom_id = read_u16(data + offset);
				offset += 2;
				
				read_bytes(str1, data + offset, 3);
				offset += 3;
				read_bytes(str2, data + offset, 13);
				offset += 13;
				// printf("[%s] %s\n", str1, str2);
			} else if (num8 == 109) {
				int message_talk_kind = 0;
				uint16_t message_emoji_key = read_u16(data + offset);
				offset += 2;
				uint16_t message_num10 = read_u16(data + offset);
				offset += 2;
						
				// printf("message_emoji_key: %u\n", message_emoji_key);
				// printf("message_num10: %u\n", message_num10);
			} else if (num8 == 0) {
				read_bytes(c->chat.message, data + offset, num9);
				offset += num9;
				c->chat.message[num9] = '\0';
				// memcpy(res.player_name, player_name, 13);
				//p.read_bytes(message, num9);
			}
		}
	}
	
	return;
}

typedef enum {
	EMS_Null,
	EMS_Begin,
	EMS_End,
	EMS_BeginAndEnd
} eMsgState;



void RecvItemInfo(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t off = 0;
	
	eMsgState RecvItemState = (eMsgState)read_u8(data + off); off += 1;
	
	/*
	switch (RecvItemState) {
		case EMS_Begin:
			printf("Begin item list\n");
			break;
		case EMS_End:
			printf("End item list\n");
			break;
		case EMS_BeginAndEnd:
			printf("Complete item list\n");
			break;
		default:
			break;
	}*/
	
	
	uint16_t counts = read_u16(data + off); off += 2;
	
	for (int i = 0; i < counts; i++) {
		uint16_t item_id       = read_u16(data + off); off += 2;
		uint16_t item_quantity = read_u16(data + off); off += 2;
		
		c->items[item_id].quantity = item_quantity;
	}
}

const char *FormatTime(uint32_t totalSecs) {
    static char buffer[64];
    uint32_t secs = totalSecs;

    uint32_t days    = secs / 86400; secs %= 86400;
    uint32_t hours   = secs / 3600;  secs %= 3600;
    uint32_t minutes = secs / 60;    secs %= 60;

    if (days > 0 && hours > 0)
        snprintf(buffer, sizeof(buffer), "%ud %uh", days, hours);
    else if (days > 0)
        snprintf(buffer, sizeof(buffer), "%ud", days);
    else if (hours > 0 && minutes > 0)
        snprintf(buffer, sizeof(buffer), "%uh %um", hours, minutes);
    else if (hours > 0)
        snprintf(buffer, sizeof(buffer), "%uh", hours);
    else if (minutes > 0 && secs > 0)
        snprintf(buffer, sizeof(buffer), "%um %us", minutes, secs);
    else if (minutes > 0)
        snprintf(buffer, sizeof(buffer), "%um", minutes);
    else
        snprintf(buffer, sizeof(buffer), "%us", secs);

    return buffer;
}


void RecvIBuffInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->shield_info.active = false;
	
	uint8_t b = read_u8(data + offset); offset +=1;
	
	uint64_t end_time = 0;
	
	uint64_t remaining = 0;
	
	for (int i = 0; i < b; i++) {
		// quantity 
		uint16_t num    = read_u16(data + offset); offset += 2;
		// item id
		uint16_t itemID = read_u16(data + offset);  offset += 2;
		// use time
		uint64_t num2 = read_u64(data + offset); offset += 8;
		// duration 
		uint32_t num3 = read_u32(data + offset); offset += 4;
		
		switch (itemID) {
			case SHIELD_4H:
			case SHIELD_8H:
			case SHIELD_12H:
			case SHIELD_1D:
			case SHIELD_3D:
			case SHIELD_7D:
			case SHIELD_14D:
				end_time  = num2 + num3;
				remaining = (end_time > c->server_time) ? (end_time - c->server_time) : 0;
				
				c->shield_info.active     = true;
				c->shield_info.item_id    = itemID;
				c->shield_info.begin_time = num2;
				c->shield_info.duration   = num3;
				
				printf("[INFO] Shield expires in: %s\n", FormatTime(remaining));
				break;
		}
	}
	
	c->shield_info.loaded = true;
}

void RecvMarchData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	c->player.max_marches     = read_u8(data + offset); offset += 1;
	c->player.current_marches = read_u8(data + offset); offset += 1;
	
	// There is more data include troops and location 
	/*
	printf("RecvMarchData\n");
	printf("max_marches: %u\n", c->player.max_marches);
	printf("current_matches: %u\n", c->player.current_marches);
	*/
	return;
}


uint16_t RoleAttrLevelUp(const uint8_t *data, int UpdateFlag) {
	uint16_t offset = 0;
	
	if ((UpdateFlag & 1) != 0) {
		read_u8(data + offset); offset += 1;
		
		// this.UpdateRoleAttrLevel(MP.ReadByte(-1));
	}
	
	if ((UpdateFlag & 2) != 0) {
		read_u32(data + offset); offset += 4;
		// this.UpdateRoleAttrExp(MP.ReadUInt(-1));
	}
	
	if ((UpdateFlag & 4) != 0) {
		read_u32(data + offset); offset += 4;
		// DataManager.Instance.Resource[4].Stock = MP.ReadUInt(-1);
	}
	
	if ((UpdateFlag & 8) != 0) {
		read_u16(data + offset); offset += 2;
		// this.UpdateRoleAttrMorale(MP.ReadUShort(-1));
	}
	
	if ((UpdateFlag & 16) != 0) {
		read_u64(data + offset); offset += 8;
		// DataManager.Instance.RoleAttr.LastMoraleRecoverTime = MP.ReadLong(-1);
	}
	
	if ((UpdateFlag & 32) != 0) {
		read_u16(data + offset); offset += 2;
		// this.UpdateRoleTalentPoint(MP.ReadUShort(-1));
	}
	
	return offset;
}

void RecvLoginRoleInfo(Connection *c, const uint8_t *data, uint16_t size) 
{
	uint16_t offset = 0;
	
	// printf("\n\n\n");
	
	uint32_t ReadPackNum = read_u32(data + offset); offset += 4;
	uint64_t UserId      = read_u64(data + offset); offset += 8;
	read_raw(c->player.name, data + offset, 13); offset += 13;
	uint16_t Head = read_u16(data + offset); offset += 2;
	
	offset += RoleAttrLevelUp(data + offset, 27);
	// p.read_bytes(d, 27);
		
	uint64_t ServerTime = read_u64(data + offset); offset += 8;
	uint64_t LogoutTime = read_u64(data + offset); offset += 8;
	uint64_t Guide = (unsigned long)read_u32(data + offset); offset += 4;
	uint32_t Diamond = read_u32(data + offset); offset += 4;
	
	c->player.gems = Diamond;
	
	uint8_t HeroSkillPoint = read_u8(data + offset); offset += 1;
	uint64_t LastHeroSPRecoverTime = read_u64(data + offset); offset += 8;
	uint16_t EnhanceEventHeroID = read_u16(data + offset); offset += 2;
	uint64_t HeroEnhanceEventTime_BeginTime = read_u64(data + offset); offset += 8;
	uint32_t HeroEnhanceEventTime_RequireTime = read_u32(data + offset); offset += 4;
	
	uint16_t StarUpEventHeroID = read_u16(data + offset); offset += 2;
	uint64_t HeroStarUpEventTime_BeginTime = read_u64(data + offset); offset += 8;
	uint32_t HeroStarUpEventTime_RequireTime = read_u32(data + offset); offset += 4;
	
	uint8_t temp[100];
	read_raw(temp, data + offset, 12); offset += 12;
	read_raw(temp, data + offset, 48); offset += 48;
	
	uint16_t abb = read_u16(data + offset); offset += 2;
	read_u16(data + offset); offset += 2;
	// printf("abb: %u\n", abb);
	
	// printf("Head: %u\n", Head);
	uint64_t BattleID = read_u64(data + offset); offset += 8;
	
	c->player.zone_id  = read_u16(data + offset); offset += 2;
	c->player.point_id = read_u8 (data + offset); offset += 1;
	/*
	uint16_t newZoneID = read_u16(data + offset); offset += 2;
	uint8_t newPointID = read_u8(data + offset); offset += 1;
	*/
	// bank_zoneId = newZoneID;
	// bank_pointId = newPointID;
	
	// map_pos_t pos = getTileMapPosbyPointCode(newZoneID, newPointID);
	// printf("Here: X:%u Y:%u\n", pos.x, pos.y);
	
	/*
	printf("name: %s\n", c->name);
	printf("userid: %lu\n", UserId);
	printf("ServerTime: %lu\n", ServerTime);
	
	printf("Diamond: %u\n", Diamond);
	
	printf("\n");
	*/
	// strcpy(login_name, name);
	// gems = Diamond;
	
	uint64_t LastChatterTime = read_u64(data + offset); offset += 8;
	uint32_t AllianceChatID  = read_u32(data + offset); offset += 4;
	c->player.power = read_u64(data + offset); offset += 8;
	c->player.kills = read_u64(data + offset); offset += 8;
	c->player.vip_point = read_u32(data + offset); offset += 4;
	uint64_t FirstTimer = read_u64(data + offset); offset += 8;
	uint32_t PrizeFlag = read_u32(data + offset); offset += 4;
	
	char PowerStr[30];
	char KillsStr[30];
				
	// format_number2(Power, PowerStr, 30);
	// format_number2(Kills, KillsStr, 30);
	
	/*
	printf("LastChatterTime: %lu\n", LastChatterTime);
	printf("AllianceChatID: %u\n", AllianceChatID);
	
	printf("Power: %s (%lu)\n", PowerStr, Power);
	printf("Kills: %s (%lu)\n", KillsStr, Kills);
	
	printf("VipPoint: %u\n", VipPoint);
	printf("FirstTimer: %lu\n", FirstTimer);
	printf("PrizeFlag: %u\n", PrizeFlag);
	*/
	
	uint64_t BookmarkTime = read_u64(data + offset); offset += 8;
	uint16_t BookmarkLimit = read_u16(data + offset); offset += 2;
	uint16_t BookmarkNum = read_u16(data + offset); offset += 2;
	
	/*
	printf("BookmarkTime: %ld\n", (int64_t)BookmarkTime);
	printf("BookmarkLimit: %u\n", BookmarkLimit);
	printf("BookmarkNum: %u\n", BookmarkNum);
	*/
	
	// read UpdateCorpsStageInfo data
	read_u8(data + offset); offset += 1;
	for (int i = 0; i < 10; i++) {
		/*
		NowCombatStageInfo[i].SoldierTableID = read_u8(data + offset); offset += 1;
		NowCombatStageInfo[i].Amount = read_u32(data + offset); offset += 4;
		
		printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, NowCombatStageInfo[i].SoldierTableID);
		printf("NowCombatStageInfo[%d].Amount: %u\n", i, NowCombatStageInfo[i].Amount);
		*/
		
		uint8_t SoldierTableID = read_u8(data + offset); offset += 1;
		uint32_t Amount = read_u32(data + offset); offset += 4;
		
		/*
		printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, SoldierTableID);
		printf("NowCombatStageInfo[%d].Amount: %u\n", i, Amount);
		*/
	}
	
	uint32_t CorpsStageWallDefence = read_u32(data + offset); offset += 4;
	// printf("CorpsStageWallDefence: %u\n", CorpsStageWallDefence);
	
	uint16_t SuccessiveLoginDays = read_u16(data + offset); offset += 2;
	// printf("SuccessiveLoginDays: %u\n", SuccessiveLoginDays);
	
	uint8_t TodayUseMoraleItemTimes = read_u8(data + offset); offset += 1;
	// printf("TodayUseMoraleItemTimes: %u\n", TodayUseMoraleItemTimes);
	
	uint8_t LordEquipBagSize = read_u8(data + offset); offset += 1;
	// printf("LordEquipBagSize: %u\n", LordEquipBagSize);
	
	uint64_t NextOnlineGiftOpenTime = read_u64(data + offset); offset += 8;
	// printf("NextOnlineGiftOpenTime: %ld\n", (int64_t)NextOnlineGiftOpenTime);
	
	uint8_t OnlineGiftOpenTimes = read_u8(data + offset); offset += 1;
	// printf("OnlineGiftOpenTimes: %u\n", OnlineGiftOpenTimes);
	
	uint16_t OnlineGiftItemID_ItemID = read_u16(data + offset); offset += 2;
	// printf("OnlineGiftItemID_ItemID: %u\n", OnlineGiftItemID_ItemID);
	
	uint16_t OnlineGiftItemID_Quantity = read_u16(data + offset); offset += 2;
	// printf("OnlineGiftItemID_Quantity: %u\n", OnlineGiftItemID_Quantity);
	
	int64_t LastLordEquipUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	int64_t LastItemMatUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	int64_t LastItemGemUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	uint16_t LordEquipEventData_ItemID = read_u16(data + offset); offset += 2;
	int8_t LordEquipEventData_Color = (int8_t)read_u8(data + offset); offset += 1;
	
	/*
	printf("LastLordEquipUpdateTime: %ld\n", LastLordEquipUpdateTime);
	printf("LastItemMatUpdateTime: %ld\n", LastItemMatUpdateTime);
	printf("LastItemGemUpdateTime: %ld\n", LastItemGemUpdateTime);
	printf("LordEquipEventData_ItemID: %u\n", LordEquipEventData_ItemID);
	printf("LordEquipEventData_Color: %d\n", LordEquipEventData_Color);
	*/
	
	int8_t LordEquipEventData_GemColor;
	
	for (int i = 0; i < 4; i++)
	{
		LordEquipEventData_GemColor = (int8_t)read_u8(data + offset); offset += 1;
		// printf("LordEquipEventData.GemColor[%d]: %d\n", i, LordEquipEventData_GemColor);
	}
	
	uint16_t LordEquipEventData_Gem;
	
	for (int j = 0; j < 4; j++) {
		LordEquipEventData_Gem = read_u16(data + offset); offset += 2;
		// printf("LordEquipEventData.Gem[%d]: %u\n", j, LordEquipEventData_Gem);
	}
	
	uint32_t LordEquipEventData_SerialNO = read_u32(data + offset); offset += 4;
	int64_t LordEquipEventTime_BeginTime = (int64_t)read_u64(data + offset); offset += 8;
	uint32_t LordEquipEventTime_RequireTime = read_u32(data + offset); offset += 4;
	int8_t VipLevelUp = (int8_t)read_u8(data + offset); offset += 1;
	
	/*
	printf("LordEquipEventData_SerialNO: %u\n", LordEquipEventData_SerialNO);
	printf("LordEquipEventTime_BeginTime: %ld\n", LordEquipEventTime_BeginTime);
	printf("LordEquipEventTime_RequireTime: %u\n", LordEquipEventTime_RequireTime);
	printf("VipLevelUp: %u\n", VipLevelUp);
	*/
	
	uint16_t nowKingdomID  = read_u16(data + offset); offset += 2;
	uint16_t homeKingdomID = read_u16(data + offset); offset += 2;
	
	c->player.current_kingdom_id = nowKingdomID;
	c->player.home_kingdom_id = homeKingdomID;
	// current_kingdom = nowKingdomID;
	
	// printf("nowKingdomID: %u\n", nowKingdomID);
	// printf("homeKingdomID: %u\n", homeKingdomID);
	
	/*
	DataManager.MapDataController.updateMyKingdom(MP.ReadUShort(-1), MP.ReadUShort(-1));
	DataManager.MapDataController.updateCapitalPoint(newZoneID, newPointID, DataManager.MapDataController.OtherKingdomData.kingdomID, false);
	*/
	
	// exit(1);
}

void format_number2(uint64_t num, char *out, size_t size) {
    if (num >= 1000000000ULL) {
    	snprintf(out, size, "%.*fB", 2, num / 1000000000.0);
        // snprintf(out, size, "%lluB", num / 1000000000ULL);
    } else if (num >= 1000000ULL) {
    	snprintf(out, size, "%.*fM", 2, num / 1000000.0);
        // snprintf(out, size, "%lluM", num / 1000000ULL);
    } else if (num >= 1000ULL) {
    	snprintf(out, size, "%.*fK", 2, num / 1000.0);
        // snprintf(out, size, "%lluK", num / 1000ULL);
    } else {
        snprintf(out, size, "%lu", num);
    }
}




#include <time.h>


void format_duration(time_t current_time, time_t expiry_time) {
    long diff = (long)(expiry_time - current_time);

    if (diff <= 0) {
        printf("Expired\n");
        return;
    }

    long days = diff / 86400;
    diff %= 86400;

    long hours = diff / 3600;
    diff %= 3600;

    long minutes = diff / 60;
    long seconds = diff % 60;
    
    if (days > 0) {
    printf("%ldd %ldh %ldm %lds\n",
           days, hours, minutes, seconds);
} else if (hours > 0) {
    printf("%ldh %ldm %lds\n",
           hours, minutes, seconds);
} else if (minutes > 0) {
    printf("%ldm %lds\n",
           minutes, seconds);
} else {
    printf("%lds\n", seconds);
}

}



uint64_t GetResourceAmount(
    const Connection *c,
    ResourceType type)
{
    switch (type) {
        case RESOURCE_FOOD:
            return c->resources.food;

        case RESOURCE_ROCK:
            return c->resources.rock;

        case RESOURCE_WOOD:
            return c->resources.wood;

        case RESOURCE_ORE:
            return c->resources.ore;

        case RESOURCE_GOLD:
            return c->resources.gold;

        default:
            return 0;
    }
}

const char *GetResourceName(ResourceType type)
{
    switch (type) {
        case RESOURCE_FOOD:
            return "food";

        case RESOURCE_ROCK:
            return "rock";

        case RESOURCE_WOOD:
            return "wood";

        case RESOURCE_ORE:
            return "ore";

        case RESOURCE_GOLD:
            return "gold";

        default:
            return "unknown";
    }
}


void BlackMarketTick(Connection *c) 
{
	if (!c->market.loaded)
		return;
	
	if (c->server_time >= c->market.refresh_time + 5) {
		printf("[MARKET] Refresh expired, requesting new data\n");
		RequestMissionInfo(c, 0);
		c->market.loaded = false;
	}
}

void HeartbeatTick(Connection *c)
{
    time_t now = time(NULL);

    if (now - c->last_heartbeat >= 15) {
        c->last_heartbeat = now;
        RequestHeartBeat(c);
    }
}

void LogMarketDecision(
    int slot,
    const char *action,
    ResourceType type,
    uint32_t need,
    uint64_t have)
{
    char need_str[32];
    char have_str[32];

    format_number2(need, need_str, sizeof(need_str));
    format_number2(have, have_str, sizeof(have_str));

    printf(
        "[MARKET] Slot %d -> %s (need %s %s, have %s)\n",
        slot,
        action,
        need_str,
        GetResourceName(type),
        have_str
    );
}

/*
bool ShouldBuyItem(Connection *c, const MarketItem *item)
{
	switch (item->item_id) {
		case BRIGHT_TALENT_ORB:
			return true;
		case SPEED_UP_30_MINUTE:
			return true;
		case SPEED_UP_60_MINUTE:
			return true;
		case SPEED_UP_3_HOUR:
			return true;
		case ANIMA:
			return true;
		case SPEED_UP_MERGING_3_HOUR: 
			return true;
		case SPEED_UP_RESEARCH_3_HOUR:
			return true;
		default:
			return false;
	}
}
*/

bool ShouldBuyItem(const MarketItem *item)
{
	// Return true; Buy everything 
	return true;
	
	switch (item->item_id) {
		case BRIGHT_TALENT_ORB:
		case SPEED_UP_30_MINUTE:
		case SPEED_UP_60_MINUTE:
		case SPEED_UP_3_HOUR:
		case SPEED_UP_MERGING_3_HOUR: 
		case SPEED_UP_RESEARCH_3_HOUR:
		case ANIMA:
			return true;
	}
	
	return false;
}

bool CanAffordItem(Connection *c, const MarketItem *item)
{
    switch (item->resource_kind) {
        case RESOURCE_FOOD:
            return c->resources.food >= item->resource_count;

        case RESOURCE_ROCK:
            return c->resources.rock >= item->resource_count;

        case RESOURCE_WOOD:
            return c->resources.wood >= item->resource_count;

        case RESOURCE_ORE:
            return c->resources.ore >= item->resource_count;

        case RESOURCE_GOLD:
            return c->resources.gold >= item->resource_count;
    }

    return false;
}

bool CanAffordMarketItem(Connection *c, const MarketItem *item)
{
    switch (item->resource_kind) {

        case RESOURCE_FOOD:
            return c->resources.food >=
                   c->market.reserve.food +
                   item->resource_count;

        case RESOURCE_ROCK:
            return c->resources.rock >=
                   c->market.reserve.rock +
                   item->resource_count;

        case RESOURCE_WOOD:
            return c->resources.wood >=
                   c->market.reserve.wood +
                   item->resource_count;

        case RESOURCE_ORE:
            return c->resources.ore >=
                   c->market.reserve.ore +
                   item->resource_count;

        case RESOURCE_GOLD:
            return c->resources.gold >=
                   c->market.reserve.gold +
                   item->resource_count;
    }

    return false;
}

bool CanSpendResource(Connection *c, ResourceType type)
{
	switch (type) {
		case RESOURCE_FOOD: return c->market.settings.spend_food;
		case RESOURCE_ROCK: return c->market.settings.spend_rock;
		case RESOURCE_WOOD: return c->market.settings.spend_wood;
		case RESOURCE_ORE:  return c->market.settings.spend_ore;
		case RESOURCE_GOLD: return c->market.settings.spend_gold;
	}
	
	return false;
}

void EvaluateBlackMarket(Connection *c)
{
	if (!c->market.settings.auto_trade)
		return;
	
	if (!c->market.loaded)
		return;
	
	if (c->market.buy_pending)
		return;
		
	for (int i = 0; i < 4; i++) 
	{
		// already purchased
		if (c->market.trade_status & (1 << i))
			continue;
		
		MarketItem *item = &c->market.items[i];
		
		if (!ShouldBuyItem(item)) {
			printf("[MARKET] Slot %d -> SKIP (unwanted item %u)\n", i, item->item_id);
			continue;
		}
		
		if (!CanSpendResource(c, item->resource_kind)) {
			printf("[MARKET] Slot %d -> SKIP (%s trading disabled)\n",
				i,
				GetResourceName(item->resource_kind)
			);
			
			continue;
		}
		
		if (!CanAffordMarketItem(c, item)) {
			uint64_t have = GetResourceAmount(c, (ResourceType)item->resource_kind);
			
			LogMarketDecision(
				i,
				"SKIP",
				item->resource_kind,
				item->resource_count,
				have
			);
			
			continue;
		}
		
		/*
		// skip unwanted items
		if (!ShouldBuyItem(c, item)) 
			continue;
		
		// not enough resources
		if (!CanAffordItem(c, item))
			continue;
		*/
		
		SendBlackMarketBuy(c, i);
		
		c->market.buy_pending = true;
		
		break;
    }
}

void TryEvaluateBlackMarket(Connection *c)
{
	if (!c->market.settings.auto_trade)
		return;
	
	if (!c->resource_loaded) 
		return;
		
	if (!c->market.loaded)
		return;
	
	EvaluateBlackMarket(c);
}

void RecvBlackMarket_Buy(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t result = read_u8(data + offset); offset++;
	
	if (result != 0) {
		c->market.buy_pending = false;
		printf("BlackMarket buy failed: %u\n", result);
		return;
	}
	
	uint8_t new_trade_status = read_u8(data + offset); offset++;
	uint8_t changed = new_trade_status ^ c->market.trade_status;
	
	for (int i = 0; i < 4; i++) 
	{
		if (((changed >> i) & 1) == 1) 
		{
			printf("Purchased slot %d\n", i);
			
			read_u16(data + offset); offset += 2;
			read_u16(data + offset); offset += 2;
			
			uint8_t resource_kind = read_u8(data + offset);
			offset += 1;
			
			uint32_t stock = read_u32(data + offset);
			offset += 4;
			
			printf("resource=%u stock=%u\n", resource_kind, stock);
		}
	}
	
	c->market.trade_status = new_trade_status;
	c->market.buy_pending = false;
	
	EvaluateBlackMarket(c);
}

void BlackMarketDataLog(Connection *c) {
	for (int i = 0; i < 4; i++) {
		printf("item_id:        %u\n", c->market.items[i].item_id);
		printf("item_count:     %u\n", c->market.items[i].item_count);
		printf("resource_kind:  %u\n", c->market.items[i].resource_kind);
		printf("resource_count: %u\n", c->market.items[i].resource_count);
		printf("rare:           %u\n", c->market.items[i].rare);
		printf("\n\n");
	}
}


void RecvBlackMarket_Data(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->market.refresh_time = read_u64(data + offset); offset += 8;
	c->market.trade_locks  = read_i8( data + offset); offset += 1;
	c->market.trade_status = read_u8( data + offset); offset += 1;
	
	for (int i = 0; i < 4; i++) {
		c->market.items[i].item_id        = read_u16(data + offset);  offset += 2;
		c->market.items[i].item_count     = read_u16(data + offset);  offset += 2;
		c->market.items[i].resource_kind  = read_u8( data + offset);  offset += 1;
		c->market.items[i].resource_count = read_u32(data + offset);  offset += 4;
		c->market.items[i].rare           = read_u8( data + offset);  offset += 1;
		
		/*
		printf("\n\n");
		printf("item_id: %u\n", c->market.items[i].item_id);
		printf("item_count: %u\n", c->market.items[i].item_count);
		printf("resource_kind: %u\n", c->market.items[i].resource_kind);
		printf("resource_count: %u\n", c->market.items[i].resource_count);
		printf("rare: %u\n", c->market.items[i].rare);
		*/
	}
	
	c->market.loaded = true;
	
	int8_t   extra_data = read_u8(data + offset); offset += 1;
	uint32_t extra_treasure_id = read_u32(data + offset); offset += 4;
	
	// printf("extra_data: %u\n", extra_data);
	if (extra_data != 1)
	{
		
	}
	
	printf("[MARKET RESET] ");
	format_duration(c->server_time, c->market.refresh_time);
	
	TryEvaluateBlackMarket(c);
}

void ResourcesLog(Connection *c) {
	char food_str[20];
	char rock_str[20];
	char wood_str[20];
	char  ore_str[20];
	char gold_str[20];
	
	format_number2(c->resources.food, food_str, 20);
	format_number2(c->resources.rock, rock_str, 20);
	format_number2(c->resources.wood, wood_str, 20);
	format_number2(c->resources.ore,   ore_str, 20);
	format_number2(c->resources.gold, gold_str, 20);
	
	printf("[RESOURCE] FOOD = %s\n", food_str);
	printf("[RESOURCE] ROCK = %s\n", rock_str);
	printf("[RESOURCE] WOOD = %s\n", wood_str);
	printf("[RESOURCE] ORE  = %s\n", ore_str);
	printf("[RESOURCE] GOLD = %s\n", gold_str);
	return;
}

void RecvResources(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->resources.food  = read_u32(data + offset); offset += 4;
	c->production.food = read_i64(data + offset); offset += 8;
	
	c->resources.rock  = read_u32(data + offset); offset += 4;
	c->production.rock = read_i64(data + offset); offset += 8;
	
	c->resources.wood  = read_u32(data + offset); offset += 4;
	c->production.wood = read_i64(data + offset); offset += 8;
	
	c->resources.ore  = read_u32(data + offset); offset += 4;
	c->production.ore = read_i64(data + offset); offset += 8;
	
	c->resources.gold  = read_u32(data + offset); offset += 4;
	c->production.gold = read_i64(data + offset); offset += 8;
	
	// Server time when the resource values were last synchronized.
    c->resources_last_update = c->server_time;
    
	ResourcesLog(c);
	
	c->resource_loaded = true;
	
	if (c->market.loaded) {
        EvaluateBlackMarket(c);
    }
	return;
}


void RecvRefreshResources(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->resources.food  = read_u32(data + offset); offset += 4;
	c->resources.rock  = read_u32(data + offset); offset += 4;
	c->resources.wood  = read_u32(data + offset); offset += 4;
	c->resources.ore   = read_u32(data + offset); offset += 4;
	c->resources.gold  = read_u32(data + offset); offset += 4;
	
	return;
}

bool IsBuilding(uint16_t build_id)
{
    switch (build_id)
    {
        case 1:   // Timber
        case 2:   // Stone
        case 3:   // Ore
        case 4:   // Food
        case 5:   // Manor
        case 6:   // Barracks
        case 7:   // Infirmary
        case 8:   // Castle
        case 9:   // Vault
        case 10:  // Academy
        case 12:  // Wall
        case 13:  // Watchtower
        case 14:  // Embassy
        case 15:  // Workshop
        case 17:  // Trading Post
            return true;

        default:
            return false;
    }
}

const char *GetBuildingName(uint16_t build_id)
{
    switch (build_id)
    {
        case 1:  return "Timber";
        case 2:  return "Stone";
        case 3:  return "Ore";
        case 4:  return "Food";
        case 5:  return "Manor";
        case 6:  return "Barracks";
        case 7:  return "Infirmary";
        case 8:  return "Castle";
        case 9:  return "Vault";
        case 10: return "Academy";
        case 12: return "Wall";
        case 13: return "Watchtower";
        case 14: return "Embassy";
        case 15: return "Workshop";
        case 17: return "Trading Post";
        default: return "Unknown";
    }
}

static const uint32_t trading_post_supply_capacity[] = {
	0,      // Level 0 (unused)
	5000,   // Level 1
	15000,  // Level 2
	30000,  // Level 3
	50000,  // Level 4
	75000,  // Level 5
	105000,  // Level 6
	140000,  // Level 7
	180000,  // Level 8
	225000,  // Level 9
	275000,  // Level 10
	330000,  // Level 11
	400000,  // Level 12
	490000,  // Level 13
	600000,  // Level 14
	730000,  // Level 15
	880000,  // Level 16
	1050000,  // Level 17
	1250000,  // Level 18
	1450000,  // Level 19
	1650000,  // Level 20
	1850000,  // Level 21
	2050000,  // Level 22
	2250000,  // Level 23
	2500000,  // Level 24
	3000000,  // Level 25
};

uint32_t GetTradingPostSupplyCapacity(uint8_t level) {
	if (level > 25) return 0;
	
	return trading_post_supply_capacity[level];
}

void RecvAllBuildData(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	c->building_count = read_u8(data + offset); offset += 1;
	
	// printf("building:\n");
	uint8_t trading_post_lv = 0;
	
	for (int i = 0; i < c->building_count; i++) {
		c->building[i].position_id = read_u16(data + offset); offset += 2;
		c->building[i].build_id    = read_u16(data + offset); offset += 2;
		c->building[i].level       = read_u8(data + offset);  offset += 1;
		
		if (c->building[i].build_id == 17) {
			trading_post_lv = c->building[i].level;
		}
		
		// display only building 
		if (IsBuilding(c->building[i].build_id) == false) continue;
		
		// filter 
		// if (c->building[i].build_id != BUILD_MANOR) continue;
		
		/*
		printf("Building Name: %s\n", GetBuildingName(c->building[i].build_id));
		printf("Building Pos: %u\n", c->building[i].position_id);
		printf("Building Level: %u\n", c->building[i].level);
		
		printf("\n");
		*/
		
	}
	
	c->supply_capacity += GetTradingPostSupplyCapacity(trading_post_lv);
	
}

void RecvMailInfo(Connection *c, const uint8_t *data)
{
    uint16_t offset = 0;

    MailInfo *mail = &c->mail;

    memset(mail, 0, sizeof(*mail));

    mail->serial_id = read_u32(data + offset);
    offset += 4;

    read_u8(data + offset); // b
    offset += 1;

    mail->send_time = read_u64(data + offset);
    offset += 8;

    mail->mail_type = read_u8(data + offset);
    offset += 1;

    mail->reply_id = read_u32(data + offset);
    offset += 4;

    mail->sender_head = read_u16(data + offset);
    offset += 2;

    mail->sender_kingdom = read_u16(data + offset);
    offset += 2;

    memcpy(mail->sender_tag, data + offset, 3);
    mail->sender_tag[3] = '\0';
    offset += 3;

    memcpy(mail->sender_name, data + offset, 13);
    mail->sender_name[13] = '\0';
    offset += 13;

    mail->extra_flag = read_u8(data + offset);
    offset += 1;

    uint8_t title_len = read_u8(data + offset);
    offset += 1;

    uint16_t content_len = read_u16(data + offset);
    offset += 2;

    mail->attachment_count = read_u8(data + offset);
    offset += 1;

    for (int i = 0; i < mail->attachment_count; i++) {
        offset += 5; // KingdomID + ZoneID + PointID
    }

    memcpy(mail->title, data + offset, title_len);
    offset += title_len;

    memcpy(mail->content, data + offset, content_len);

    printf("\n");
    printf("Sender: [%s] %s\n",
           mail->sender_tag,
           mail->sender_name);

    printf("Kingdom: %u\n",
           mail->sender_kingdom);

    printf("Title: %s\n",
           mail->title);

    printf("Content: %s\n",
           mail->content);
}


void RecvAllyPoint(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	uint8_t  status   = read_u8(data + offset);  offset += 1;
	uint16_t zone_id  = read_u16(data + offset); offset += 2;
	uint8_t  point_id = read_u8(data + offset);  offset += 1;
	map_pos_t pos;
	
	switch (status) {
		case 0: 
			pos = getTileMapPosbyPointCode(zone_id, point_id);
			
			printf(
				"Player found: Zone=%u Point=%u (%u,%u)\n",
				zone_id,
				point_id,
				pos.x,
				pos.y
			);
			
			c->hyper.zone_id = zone_id;
			c->hyper.point_id = point_id;
			
			c->hyper.pending = false;
			c->hyper.state = HYPER_STATE_READY;
            break;
            
        case 1:
            printf("Target is in another kingdom\n");
            c->hyper.state = HYPER_STATE_IDLE;
            break;

        default:
            printf("AllyPoint failed: %u\n", status);
            c->hyper.state = HYPER_STATE_IDLE;
            break;
    }
}

void RecvAllianceHelp(Connection *c, const uint8_t *data) {
	printf("RecvAllianceHelp()\n");
}


void RecvAllianceMemberNeedsHelp(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->help.record_sn = read_u32(data + offset); offset += 4;
	c->help.head      = read_u16(data + offset); offset += 2;
	c->help.rank      = read_u8(data + offset); offset += 1;
	read_raw(c->help.player_name, data + offset, 13); offset += 13;
	c->help.help_kind = (HelpKind)read_u8(data + offset); offset += 1;
	
	c->help.event_id = read_u16(data + offset); offset += 2;
	c->help.event_data_lv = read_u8(data + offset); offset += 1;
	c->help.already_helped = read_u8(data + offset); offset += 1;
	c->help.help_max = read_u8(data + offset); offset += 1;
	
	if (c->alliance.auto_help) {
		RequestHelpAllianceMember(c, 1, &c->help.record_sn);
		printf("[GUILD] Sent help to %s\n", c->help.player_name);
	}
}

void RecvPendingAllianceMembersNeedHelp(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	bool do_not_update_ui = read_u8(data + offset); offset += 1;
	uint8_t count = read_u8(data + offset); offset += 1;
	
	for (int i = 0; i < count; i++) {
		c->help.record_sn = read_u32(data + offset); offset += 4;
		c->help.head      = read_u16(data + offset); offset += 2;
		c->help.rank      = read_u8(data + offset); offset += 1;
		read_raw(c->help.player_name, data + offset, 13); offset += 13;
		c->help.help_kind = (HelpKind)read_u8(data + offset); offset += 1;
		c->help.event_id = read_u16(data + offset); offset += 2;
		c->help.event_data_lv = read_u8(data + offset); offset += 1;
		c->help.already_helped = read_u8(data + offset); offset += 1;
		c->help.help_max = read_u8(data + offset); offset += 1;
		c->help.record_sn_arr[i] = c->help.record_sn;
	}
	
	if (c->alliance.auto_help) {
		RequestHelpAllianceMember(c, count, c->help.record_sn_arr);
		printf("[GUILD] Sent help to %u guild members\n", count);
	}
}

const char *GiftStatusToString(uint8_t status)
{
	switch (status) {
	case 0: return "NEW";
	case 1: return "OPEN";
	case 2: return "EXP";
	default: return "?";
	}
}

/*
Opened Gift 173
Reward: 1 × Speed Up (60 min)
*/

void RecvAllianceGiftInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	if (b == 2 || b == 3)
	{
		// GUIManager.Instance.UpdateUI(EGUIWindow.UI_Alliance_Gift, 1, 0);
		// GUIManager.Instance.HideUILock(EUILock.Alliance_Gift);
	}
	
	
	uint8_t gift_count = read_u8(data + offset); offset += 1;
	
	// if (gift_count == 0) return;
	
	AllianceGift2 gift;
	
	for (int i = 0; i < gift_count; i++) {
		gift.sn          =    read_u32(data + offset); offset += 4;
		gift.status      =    read_u8(data + offset); offset += 1;
		gift.rcv_time    =    read_u64(data + offset); offset += 8;
		gift.box_item_id =    read_u16(data + offset); offset += 2;
		gift.item_id     =    read_u16(data + offset); offset += 2;
		gift.num         =    read_u16(data + offset); offset += 2;
		gift.item_rank   =    read_u8(data + offset); offset += 1;
		read_raw(gift.player, data + offset, 13); offset += 13;
		
		/*
		printf("Opened gift\n");
		printf("item id: %u\n", gift.item_id);
		printf("num: %u\n", gift.num);
		printf("name: %s\n", gift.player);
		*/
		
		if (gift.status != 0) continue; // ignore already opened or expired gift;
		
		printf("[GIFT] ID %u FROM %s\n",
			gift.sn,
			gift.player
		);
		
		AllianceGift *g = &c->alliance.gifts[c->alliance.gift_count];
		g->sn = gift.sn;
		g->status = gift.status;
		c->alliance.gift_count++;
		
		/*
		c->alliance.gifts[i].sn      = gift.sn;
		c->alliance.gifts[i].status  = gift.status;
		*/
	}
	
	c->alliance.gift_offset = 0;
	
	c->alliance.gift_state = GIFT_STATE_READY;
}

void RecvAllianceGiftOpen(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	if (b != 0 && b != 2)
		return;
	
	/*
	AllianceGift gift;
	
	gift.sn           = read_u32(data + offset); offset += 4;
	gift.status       = read_u8(data + offset);  offset += 1;
	gift.rcv_time     = read_u64(data + offset); offset += 8;
	gift.box_item_id  = read_u16(data + offset); offset += 2;
	gift.item_id      = read_u16(data + offset); offset += 2;
	gift.num          = read_u16(data + offset); offset += 2;
	gift.item_rank    = read_u8(data + offset);  offset += 1;
	*/
	
	uint32_t sn = read_u32(data + offset); offset += 4;
	
	printf("[OPENED] Gift ID %u\n", sn);
	
	for (int i = 0; i < c->alliance.gift_count; i++) {
		AllianceGift *gift = &c->alliance.gifts[i];
		
		if (gift->sn != sn)
            continue;
            
        gift->status = 0xFF;

        RequestDeleteAllianceGiftBox(c, 0xFFFFFFFF);
        c->alliance.gift_state = GIFT_STATE_DELETING;
        break;
	}
	
}

void RecvDeleteAllianceGiftBox(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	if (b == 0) {
		uint16_t unknown = read_u16(data + offset); offset += 2;
		uint16_t num = read_u16(data + offset); offset += 2;
		
		// printf("[Gift] Delete %u gift(s)\n", num);
		
		for (int i = 0; i < (int)num; i++) {
			uint32_t mGift_UpdateSN = read_u32(data + offset); offset += 4;
			
			c->alliance.gift_state = GIFT_STATE_READY;
			
			printf("[DELETE] GIFT ID %u\n", mGift_UpdateSN);
			
			
			for (int i = 0; i < c->alliance.gift_count; i++) {
				AllianceGift *gift = &c->alliance.gifts[i];
				
				if (gift->sn != mGift_UpdateSN) 
					continue;
			
				gift->status = 0xFF;
				
				break;
			}
			
		}
	} else {
		printf("Delete error: b: %u\n", b);
	}
}



void RecvLoginError(Connection *c, const uint8_t *data) {
	uint8_t kind = read_u8(data);
	
	if (kind == 110) {
		LOGE("Bootstrap Login failed update client version");
	} else if (kind == 9) {
		LOGE("Bootstrap Login error reason: logging from another device errorCode: %u", kind);
	} else {
		LOGE("Bootstrap Login failed: %u", kind);
	}
}


void RecvUseItem(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t offset = 0;
	
	uint8_t status = read_u8(data + offset); offset += 1;
	
	if (status == 0) {
		uint16_t item_id       = read_u16(data + offset); offset += 2;
		uint16_t item_quantity = read_u16(data + offset); offset += 2;
		uint16_t num3          = read_u16(data + offset); offset += 2;
		
		// Server returns the updated inventory quantity after using the item.
		c->items[item_id].quantity = item_quantity;
		
		if (item_id == ADVANCE_RELOCATOR || item_id == RANDOM_RELOCATOR) {
			c->player.zone_id            = read_u16(data + offset); offset += 2;
			c->player.point_id           = read_u8(data + offset);  offset += 1;
			c->player.current_kingdom_id = read_u16(data + offset); offset += 2;
			return;
		} else if (item_id == SHIELD_4H || 
				item_id == SHIELD_8H || 
				item_id == SHIELD_12H || 
				item_id == SHIELD_1D || 
				item_id == SHIELD_3D || 
				item_id == SHIELD_7D) {
			
			c->shield_info.active     =   true;
			c->shield_info.pending    =   false;
			c->shield_info.quantity   =   read_u16(data + offset); offset += 2; // quantity 
			c->shield_info.item_id    =   read_u16(data + offset); offset += 2; // item id
			c->shield_info.begin_time =   read_u64(data + offset); offset += 8; // begin time
			c->shield_info.duration   =   read_u32(data + offset); offset += 4; // duration
			return;
		} else if (item_id == WITHDRAW_SQUAD) {
			// uint8_t march_index = read_u8(data + offset); offset += 1;
			// printf("[INFO] Withdraw Squad used. Recalled march #%u.\n", march_index + 1);
			printf("[INFO] Withdraw Squad used. Recalled march.\n");
			return;
		} else if (item_id == 0x03ed) {
			printf("Novice Relocator\n");
			
			return;
		}
		
		return;
	}
	
	return;
}


void RecvAllianceInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
    
    c->RoleAlliance.Channel = read_u32(data + offset); offset += 4;
	c->RoleAlliance.Rank = (AllianceRank)read_u8(data + offset); offset += 1;
	c->RoleAlliance.Apply = read_u8(data + offset); offset += 1;
	c->RoleAlliance.Money = read_u32(data + offset); offset += 4;
	
	return;
}

void RecvBuildingQueue(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	
	uint8_t queue_build_type = read_u8(data + offset); offset += 1;
	uint16_t position = read_u16(data + offset); offset += 2;
	uint16_t build_id = read_u16(data + offset); offset += 2;
	uint8_t level = read_u8(data + offset); offset += 1;
	uint64_t start_time = read_u64(data + offset); offset += 8;
	uint32_t total_time = read_u32(data + offset); offset += 4;
	
	
	return;
	
	printf("queue_build_type: %u\n", queue_build_type);
	printf("position: %u\n", position);
	printf("build_id: %u\n", build_id);
	printf("level: %u\n", level);
	printf("start_time: %lu\n", start_time);
	printf("total_time: %u\n", total_time);
	
}

typedef enum
{
	EWATCHTOWER_LINE_TARGET_CAPITAL,
	EWATCHTOWER_LINE_TARGET_CAMP,
	EWATCHTOWER_LINE_TARGET_AMBUSH,
	EWATCHTOWER_ADDLINE_WONDER1,
	EWATCHTOWER_ADDLINE_WONDER2,
	EWATCHTOWER_ADDLINE_WONDER3,
	EWATCHTOWER_ADDLINE_WONDER4,
	EWATCHTOWER_ADDLINE_WONDER5,
	EWATCHTOWER_ADDLINE_WONDER6,
	EWATCHTOWER_ADDLINE_WONDER7
} EWATCHTOWER_LINE_TARGET;


typedef enum {
	GatherAttack,
	Attack,
	Wonder_GatherAttack,
	Wonder_Attack,
	None_Attack,
	PetAttack,
	Conflict,
	Detect,
	Wonder_Detect,
	None_Detect,
	Gather,
	Cantonment,
	Reinforce,
	Wonder_Reinforce,
	Supplies,
	_Max
} EAttackKind;


const char *GetShieldName(uint16_t item_id)
{
	switch (item_id) {
		case SHIELD_4H:  return "4 hour shield";
		case SHIELD_8H:  return "8 hour shield";
		case SHIELD_12H: return "12 hour shield";
		case SHIELD_1D:  return "1 day shield";
		case SHIELD_3D:  return "3 day shield";
		case SHIELD_7D:  return "7 day shield";
		case SHIELD_14D: return "14 day shield";
		default:         return "unknown shield";
	}
}

void UsePriorityShield(Connection *c)
{
	// A shield activation request has already been sent.
	if (c->shield_info.pending)
		return;
	
	for (uint8_t i = 0; i < c->protection.shield_priority_count; i++) {
		uint16_t item_id = c->protection.shield_priority[i];
		
		switch (item_id) {
			case SHIELD_4H:
			case SHIELD_8H:
			case SHIELD_12H:
			case SHIELD_1D:
			case SHIELD_3D:
			case SHIELD_7D:
			case SHIELD_14D:
				break;
			default:
				continue;
		}
		
		if (c->items[item_id].quantity > 0) {
			RequestSimpleUseItem(c, item_id, 1);
			c->shield_info.pending = true;
			printf("[INFO] Using %s\n", GetShieldName(item_id));
			return;
		}
	}
	
	printf("[INFO] No shield items available.\n");
	return; // No shields available
}

/*
 * Automatically activate a shield when an enemy scout approaches the
 * turf, if the feature is enabled and no shield is currently active.
 */
static void WhenEnemyScoutApproachingTurf(Connection *c)
{
	// Protection system is disabled.
	if (!c->protection.enabled)
		return;
	
	// Shielding against incoming scouts is disabled.
	if (!c->protection.shield_on_incoming_scout)
		return;
	
	// A shield is already active; nothing to do.
	if (c->shield_info.active)
		return;
	
	// Activate the highest priority shield available.
	UsePriorityShield(c);
}

/*
 * Called when an enemy army is detected approaching the turf.
 * If protection is enabled for incoming attacks, activate the selected 
 * priority list shield available.
 */
static void WhenEnemyArmyApproachingTurf(Connection *c)
{
	// Protection system is disabled.
	if (!c->protection.enabled)
		return;
	
	// Shielding against incoming attacks is disabled.
	if (!c->protection.shield_on_incoming_attack) 
		return;
	
	// A shield is already active; nothing to do.
	if (c->shield_info.active)
		return;
	
	// Activate the highest priority shield available.
    UsePriorityShield(c);
}




static void WhenEnemyArmyApproachingCamp(Connection *c, uint8_t Index) {
	if (!c->protection.enabled) 
		return;
	
	if (!c->protection.recall_on_incoming_attack)
		return;
	
	RequestTroopTakeBack(c, Index);
}

static void WhenEnemyScoutApproachingCamp(Connection *c, uint8_t Index) {
	if (!c->protection.enabled) 
		return;
	
	if (!c->protection.recall_on_incoming_scout)
		return;
	
	RequestTroopTakeBack(c, Index);
}

// _MSG_RESP_WARHALL_INITLIST (0x9AD), size=12
// _MSG_RESP_WARHALL_INITLIST (0x9AD), size=12
// _MSG_RESP_UPDATEWATCHTOWER_UPDATELINE (0x98A), size=20
// [INFO ] PACKET TYPE: _MSG_RESP_TROOPHOME (0x973), size=105



// Prototype 
void RecvUpdateWatchTowerAddLineInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint32_t LineID   = read_u32(data + offset); offset += 4;
	uint8_t  LineType = read_u8(data + offset);  offset += 1;
	uint8_t  Index    = read_u8(data + offset);  offset += 1;
	
	uint64_t MarchTimeData_BeginTime   = read_u64(data + offset); offset += 8;
	uint32_t MarchTimeData_RequireTime = read_u32(data + offset); offset += 4;
	
	EWATCHTOWER_LINE_TARGET ewatchtower_LINE_TARGET = (EWATCHTOWER_LINE_TARGET)read_u8(data + offset);  offset += 1;
	
	/*
	printf("\nRecvUpdateWatchTowerAddLineInfo\n");
	
	printf("LineID: %u\n", LineID);
	printf("LineType: %u\n", LineType);
	printf("Index: %u\n", Index);
	
	printf("MarchTimeData_BeginTime: %lu\n", MarchTimeData_BeginTime);
	printf("MarchTimeData_RequireTime: %u\n", MarchTimeData_RequireTime);
	
	printf("ewatchtower_LINE_TARGET: %u\n", ewatchtower_LINE_TARGET);
	*/
	
	switch (LineType) {
		case 5: 
		case 7:
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					WhenEnemyArmyApproachingTurf(c);
					printf("[WARNING] Enemy army is invading turf\n");
					break;
				case EWATCHTOWER_LINE_TARGET_CAMP:
					printf("[WARNING] Enemy army is invading camp\n");
					WhenEnemyArmyApproachingCamp(c, Index);
					break;
				case EWATCHTOWER_LINE_TARGET_AMBUSH:
					printf("Enemy army has set an ambush\n");
					// EAttackKind.None_Attack
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					printf("[WARNING] Enemy invading wonder\n");
					// EAttackKind.Wonder_Attack
					break;
			}
			break;
		case 6:
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					printf("[INFO] Garrison troops are approaching your Turf\n");
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					// EAttackKind.Cantonment
					printf("[INFO] Relieve troop are approaching to wonder\n");
					break;
				default:
					break;
			}
			break;
		case 8: 
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					WhenEnemyScoutApproachingTurf(c);
					printf("[WARNING] Enemy scout approaching to turf\n");
					break;
				case EWATCHTOWER_LINE_TARGET_CAMP:
					printf("[WARNING] Enemy scout approaching to camp\n");
					WhenEnemyScoutApproachingCamp(c, Index);
					break;
				case EWATCHTOWER_LINE_TARGET_AMBUSH:
					printf("Enemy target ambush\n");
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					printf("[WARNING] Enemy scout approaching to wonder\n");
					break;
			}
			break;
		case 10: 
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					printf("[INFO] Reinforcement troops are approaching your Turf\n");
					// EAttackKind.Reinforce
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					printf("[INFO] Reinforcement troops are approaching wonder\n");
					break;
				default:
					break;
			}
			break;
		case 11:
			printf("EAttackKind.Gather\n");
			break;
		case 12: 
			printf("Someone approaching EAttackKind.Wonder_GatherAttack attacking\n");
			break;
		case 13: 
			printf("[INFO] Guild member is sending supplies\n");
			break;
		case 22: 
			printf("[WARNING] Familiar attack is approaching your Turf\n");
			break;
		default:
			break;
		
	}
	
	return;
}


void ShieldTick(Connection *c)
{
	if (!c->protection.enabled)
		return;
	
	if (!c->protection.shield_always_on)
		return;
		
	// Don't make any shield decisions until the server
	// has told us which buffs are currently active.
	if (!c->shield_info.loaded)
		return;
	
	// Already protected?
	if (c->shield_info.active) {
		// Still plenty of time remaining.
		uint64_t end_time = c->shield_info.begin_time + c->shield_info.duration;
		
		uint32_t remaining_time = (end_time > c->server_time) ? (uint32_t)(end_time - c->server_time) : 0;
		
		if (remaining_time > 300) // 5 minutes
			return;
	}
	
	// printf("[SHIELD] Shield expiring soon, renewing...\n");
	
	UsePriorityShield(c);
}

void AllianceGiftTick(Connection *c) {
	if (!c->alliance.auto_open_gifts) 
		return;
	
	if (c->alliance.gift_count == 0) 
		return;
	
	if (c->alliance.gift_state != GIFT_STATE_READY)
		return;
	
	for (int i = c->alliance.gift_offset; i < c->alliance.gift_count; i++) {
		AllianceGift *gift = &c->alliance.gifts[i];
		
		// Unopened gift → open it.
		if (gift->status == 0) {
			RequestOpenAllianceGift(c, gift->sn);
			c->alliance.gift_state = GIFT_STATE_OPENING;
			return;
		}
	}
	return;
}



// Not fully understand core mechanism yet
void RecvRoleUpdateInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	// return ;
	
	printf("RecvRoleUpdateInfo\n");
	
	printf("b: %u\n", b);
	
	// 21 => PackPoint(packet.read_t()?),
	if (b == 21) {
		uint32_t v0 = read_u32(data + offset); offset += 4;
		printf("PackPoint\n");
		// printf("v0: %u\n", v0);
		return;
	}
	
	// 41 => AllianceWarRegister(packet.read_t()?),
	if (b == 41) {
		uint8_t v0 = read_u8(data + offset); offset += 1;
		printf("AllianceWarRegister\n");
		printf("v0: %u\n", v0);
	}
	
	// 18 => GiftCount {
	if (b == 18) {
		uint16_t v0 = read_u16(data + offset); offset += 2;
		uint16_t v1 = read_u16(data + offset); offset += 2;
		
		printf("GiftCount\n");
		printf("v0: %u\n", v0);
		printf("v1: %u\n", v1);
		return;
	}
	
	// 19 => AllianceBox1 {
	// This will trigger when someone hunt monster in map and bot online 
	if (b == 19) {
		AllianceGift2 gift;
		
		gift.sn           = read_u32(data + offset); offset += 4;
		gift.status       = read_u8(data + offset);  offset += 1;
		gift.rcv_time     = read_u64(data + offset); offset += 8;
		gift.box_item_id  = read_u16(data + offset); offset += 2;
		gift.item_id      = read_u16(data + offset); offset += 2;
		gift.num          = read_u16(data + offset); offset += 2;
		gift.item_rank    = read_u8(data + offset);  offset += 1;
		read_raw(gift.player, data + offset, 13); offset += 13;
		
		uint32_t gift_update_sn = read_u32(data + offset); offset += 4;
		
		RequestOpenAllianceGift(c, gift.sn);
		return;
	}
}


void RecvArmyGroupInfoLog(Connection *c) {
	printf("Total Troop: %u\n", c->troop.total);
	
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Infantry: %u\n", i + 1, c->troop.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Ranged: %u\n", i + 1, c->troop.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Cavalry: %u\n", i + 1, c->troop.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Siege: %u\n", i + 1, c->troop.siege[i]);
	}
	
}

void RecvArmyGroupInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	for (int index = 0; index < 4; index++) {
		c->troop.infantry[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.infantry[index];
	}
	
	for (int index = 0; index < 4; index++) {
		c->troop.ranged[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.ranged[index];
	}
	
	for (int index = 0; index < 4; index++) {
		c->troop.cavalry[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.cavalry[index];
	}
	
	for (int index = 0; index < 4; index++) {
		c->troop.siege[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.siege[index];
	}
	
	c->troop.loaded = true;
	
	// RecvArmyGroupInfoLog(c);
	return;
}

void WoundedTroopDataLog(Connection *c) {
	
	printf("Total Wounded: %u\n", c->wounded.troop.total);
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Infantry: %u\n", i + 1, c->wounded.troop.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Ranged: %u\n", i + 1, c->wounded.troop.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Cavalry: %u\n", i + 1, c->wounded.troop.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Siege: %u\n", i + 1, c->wounded.troop.siege[i]);
	}
	
	
	printf("Healing Total: %u\n", c->wounded.healing.total);
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Infantry: %u\n", i + 1, c->wounded.healing.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Ranged: %u\n", i + 1, c->wounded.healing.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Cavalry: %u\n", i + 1, c->wounded.healing.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Siege: %u\n", i + 1, c->wounded.healing.siege[i]);
	}
	
	printf("num: %lu\n", c->wounded.num);
	printf("total time: %u\n", c->wounded.total_time);
}

void RecvWoundedTroopData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->wounded.troop.total = 0;
	c->wounded.healing.total = 0;
	
	// Infantry 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.infantry[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.infantry[index];
	}
	
	// Ranged 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.ranged[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.ranged[index];
	}
	
	// Cavalry 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.cavalry[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.cavalry[index];
	}
	
	// Siege
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.siege[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.siege[index];
	}
	
	
	// Infantry 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.infantry[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.infantry[index];
	}
	
	// Ranged 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.ranged[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.ranged[index];
	}
	
	// Cavalry 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.cavalry[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.cavalry[index];
	}
	
	// Siege
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.siege[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.siege[index];
	}
	
	c->wounded.num = read_u64(data + offset); offset += 8;
	c->wounded.total_time  = read_u32(data + offset); offset += 4;
	
	c->wounded.loaded = true;
	
	// WoundedTroopDataLog(c);
}

void RecvDarknestBroadcast(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	char player_name[13];
	
	int64_t data_index = read_i64(data + offset); offset += 8;
	uint8_t level      = read_u8 (data + offset); offset += 1;
	read_raw(player_name, data + offset, 13); offset += 13;
	
	
	strcpy(c->rally.leader, player_name);
	c->rally.pending = true;
	c->rally.level = level;
	
	// Join after 5 seconds.
	// c->rally.execute_time = c->server_time + 5;
	
	/*
	uint32_t troop_array[16] = {0};
	
	// Send all available T1 cavalry.
	troop_array[8] = 10;// c->troop.cavalry[0];
	
	RequestJoinRally(c, player_name, troop_array);
	*/
	printf("[INFO] Lv.%u Darknest rally opened by %s\n", level, player_name);
	printf("data_index: %ld\n", data_index);
	
	
}


typedef enum {
    T1_INFANTRY = 0,
    T1_RANGED   = 1,
    T1_CAVALRY  = 2,
    T1_SIEGE    = 3,
    
    T2_INFANTRY = 4,
    T2_RANGED   = 5,
    T2_CAVALRY  = 6,
    T2_SIEGE    = 7,
    
    T3_INFANTRY = 8,
    T3_RANGED   = 9,
    T3_CAVALRY  = 10,
    T3_SIEGE    = 11,
    
    T4_INFANTRY = 12,
    T4_RANGED   = 13,
    T4_CAVALRY  = 14,
    T4_SIEGE    = 15
} TroopSlot;

void DarknestRallyTick(Connection *c)
{
	if (!c->darknest.auto_join) 
		return;
	
    if (!c->rally.pending)
        return;

    if (c->server_time < c->rally.execute_time)
        return;
    printf("[DEBUG] RallyTick(Connection *c)\n");
    
    // RequestWarHallListDetail(c, 0, 0);
    
    /*
    uint32_t troop_array[16] = {0};
    troop_array[T1_CAVALRY] = 100;

    RequestJoinRally(c, c->rally.leader, troop_array);
    */
    c->rally.pending = false;
}


void RecvRallyCountData(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	// ally_rally_count: number of rallies opened by our alliance (e.g., darknest, fort, wonder, turf attacks).
	c->rally_status.active_rally_count = read_u32(data + offset);
	offset += 4;
	
	// enemy_rally_count: number of rallies opened against our alliance by enemy (targeting fort, wonder, turf, etc.).
	c->rally_status.being_rally_count = read_u32(data + offset);
	offset += 4;
	
	printf("ActiveRally: %u\n", c->rally_status.active_rally_count);
	printf("BeingRally: %u\n",  c->rally_status.being_rally_count);
	
	// No active or incoming rallies.
	if (c->rally_status.active_rally_count == 0 && c->rally_status.being_rally_count == 0) {
		return;
	}
	
	// RequestRallyDetail(c, 0, 0);
	// printf("RequestWarHallListDetail\n");
	
	// Fetch All rallies 
	// RequestRallyList(c);
	return;
}


void RecvWarBegin(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	char tmpS[13];
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	// War rally initiated by ally
	if (b == 0) {
		printf("[NOTICE] Rally Initiated!\n");
		return;
	}
	
	// war rally initiated by enemy
	if (b == 1) {
		read_raw(tmpS, data + offset, 13);
		offset += 13;
		
		printf("[WARNING] %s declared on your ally!\n", tmpS);
		return;
	}
}

/*
map_pos_t pos = getTileMapPosbyPointCode(enemy_zone_id, enemy_point_id);

printf(
    "[%s] %s -> %s | Target: K:%u X:%u Y:%u | Troops %u/%u | Time %us\n",
    (type == 0) ? "ALLY RALLY" : "ENEMY RALLY",
    ally_name,
    enemy_name,
    enemy_zone_id,
    pos.x,
    pos.y,
    ally_curr_troop,
    ally_max_troop,
    require_time
);*/

const char *GetNPCName(uint16_t npc_id)
{
	switch (npc_id) {
		case 1:  return "Darknest";
		// case 2: return "...";
		// case 3: return "...";
		default: return "Unknown NPC";
	}
}

void NPCRallyLog(const NPCRally *r)
{
	printf(
		"[NPC RALLY] %s -> %s Lv.%u (%u/%u)\n",
		r->ally_name,
		GetNPCName(r->enemy_npc_id),
		r->enemy_vip,
		r->ally_curr_troop,
		r->ally_max_troop
	);
}

void RecvNPCWallHallData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint32_t index        = read_u32(data + offset); offset += 4;
	
	if (index >= 30) return;
	
	c->npc_rallies[index].index             = index;
	c->npc_rallies[index].kind              = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].begin_time        = read_i64(data + offset); offset += 8;
	c->npc_rallies[index].require_time      = read_u32(data + offset); offset += 4;
	c->npc_rallies[index].ally_zone_id      = read_u16(data + offset); offset += 2;
	c->npc_rallies[index].ally_point_id     = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].ally_head         = read_u16(data + offset); offset += 2;
	read_raw(c->npc_rallies[index].ally_name, data + offset,  13);     offset += 13;
	c->npc_rallies[index].ally_vip          = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].ally_rank         = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].ally_curr_troop   = read_u32(data + offset); offset += 4;
	c->npc_rallies[index].ally_max_troop    = read_u32(data + offset); offset += 4;
	c->npc_rallies[index].ally_home_kingdom = read_u16(data + offset); offset += 2;
	c->npc_rallies[index].enemy_head        = 255;
	c->npc_rallies[index].enemy_zone_id     = read_u16(data + offset); offset += 2;
	c->npc_rallies[index].enemy_point_id    = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].enemy_vip         = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].enemy_npc_id      = read_u16(data + offset); offset += 2;
	
	NPCRallyLog(&c->npc_rallies[index]);
	
	// if darknest auto join not enabled then leave 
	if (!c->darknest.auto_join) {
		return;
	}
	
	// must active rally
	if (c->npc_rallies[index].kind != 0) {
		return;
	}
	
	if (c->npc_rallies[index].enemy_vip < c->darknest.min_level || 
		c->npc_rallies[index].enemy_vip > c->darknest.max_level) {
		return;
	}
	
	if (c->darknest.formation_mode == DARKNEST_FORMATION_LEADER) {
		RequestRallyDetail(c, 0, index);
		return;
	}
	
	
	
	// RequestRallyDetail(c, 0, index);
	return;
	
	map_pos_t pos = getTileMapPosbyPointCode(c->npc_rallies[index].ally_zone_id, c->npc_rallies[index].ally_point_id);
	
	printf("=== NPC Wall Hall Data ===\n");

	printf("index: %u\n", index);
	printf("kind: %u\n", c->npc_rallies[index].kind);
	printf("begin_time: %lld\n", (long long)c->npc_rallies[index].begin_time);
	printf("require_time: %u\n", c->npc_rallies[index].require_time);

	printf("ally_zone_id: %u\n", c->npc_rallies[index].ally_zone_id);
	printf("ally_point_id: %u\n", c->npc_rallies[index].ally_point_id);
	printf("ally_head: %u\n", c->npc_rallies[index].ally_head);
	printf("ally_name: %s\n", c->npc_rallies[index].ally_name);

	printf("ally_vip: %u\n", c->npc_rallies[index].ally_vip);
	printf("ally_rank: %u\n", c->npc_rallies[index].ally_rank);

	printf("ally_curr_troop: %u\n", c->npc_rallies[index].ally_curr_troop);
	printf("ally_max_troop: %u\n", c->npc_rallies[index].ally_max_troop);
	printf("ally_home_kingdom: %u\n", c->npc_rallies[index].ally_home_kingdom);

	printf("enemy_zone_id: %u\n", c->npc_rallies[index].enemy_zone_id);
	printf("enemy_point_id: %u\n", c->npc_rallies[index].enemy_point_id);
	printf("enemy_vip: %u\n", c->npc_rallies[index].enemy_vip);
	printf("enemy_npc_id: %u\n", c->npc_rallies[index].enemy_npc_id);

	printf("%s K:%u X:%u Y:%u\n", c->npc_rallies[index].ally_name, c->npc_rallies[index].ally_home_kingdom, pos.x, pos.y);
	
	printf("=== End NPC Wall Hall Data ===\n");
}

static const char *TroopNameByIndex(int index)
{
    static const char *names[20] = {
        "T1 Infantry",
        "T1 Ranged",
        "T1 Cavalry",
        "T1 Siege",

        "T2 Infantry",
        "T2 Ranged",
        "T2 Cavalry",
        "T2 Siege",

        "T3 Infantry",
        "T3 Ranged",
        "T3 Cavalry",
        "T3 Siege",

        "T4 Infantry",
        "T4 Ranged",
        "T4 Cavalry",
        "T4 Siege",
        
        "T5 Infantry",
        "T5 Ranged",
        "T5 Cavalry",
        "T5 Siege"
    };

    if (index < 0 || index >= 20)
        return "Unknown";

    return names[index];
}

void RecvNPCWallHallDetail(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	printf("_MSG_RESP_NPC_WARHALL_INIT_LISTDETAIL\n");
	
	char ally_name[13];
	
	uint8_t  kind         = read_u8 (data + offset); offset += 1;
	int64_t  begin_time   = read_i64(data + offset); offset += 8;
	uint32_t require_time = read_u32(data + offset); offset += 4;
	uint16_t ally_zone_id = read_u16(data + offset); offset += 2;
	uint8_t ally_point_id = read_u8(data + offset);  offset += 1;
	uint16_t ally_head    = read_u16(data + offset); offset += 2;
	read_raw(ally_name, data + offset, 13); offset += 13;
	uint8_t ally_vip      = read_u8 (data + offset); offset += 1;
	uint8_t ally_rank     = read_u8 (data + offset); offset += 1;
	
	
	uint32_t ally_curr_troop = read_u32(data + offset); offset += 4;
	uint32_t ally_max_troop = read_u32(data + offset); offset += 4;
	
	uint16_t ally_home_kingdom = read_u16(data + offset); offset += 2;
	
	
	uint16_t enemy_head     = 255;
	uint16_t enemy_zone_id  = read_u16(data + offset); offset += 2;
	uint8_t  enemy_point_id = read_u8 (data + offset); offset += 1;
	uint8_t  enemy_vip      = read_u8 (data + offset); offset += 1;
	uint16_t enemy_npc_id   = read_u16(data + offset); offset += 2;
	
	map_pos_t pos = getTileMapPosbyPointCode(ally_zone_id, ally_point_id);
	
	
	printf("=== NPC Wall Hall Detail ===\n");

	printf("kind: %u\n", kind);
	printf("begin_time: %lld\n", (long long)begin_time);
	printf("require_time: %u\n", require_time);

	printf("ally_zone_id: %u\n", ally_zone_id);
	printf("ally_point_id: %u\n", ally_point_id);
	printf("ally_head: %u\n", ally_head);
	printf("ally_name: %s\n", ally_name);

	printf("ally_vip: %u\n", ally_vip);
	printf("ally_rank: %u\n", ally_rank);

	printf("ally_curr_troop: %u\n", ally_curr_troop);
	printf("ally_max_troop: %u\n", ally_max_troop);
	printf("ally_home_kingdom: %u\n", ally_home_kingdom);

	printf("enemy_zone_id: %u\n", enemy_zone_id);
	printf("enemy_point_id: %u\n", enemy_point_id);
	printf("enemy_vip: %u\n", enemy_vip);
	printf("enemy_npc_id: %u\n", enemy_npc_id);

	printf("%s K:%u X:%u Y:%u\n", ally_name, ally_home_kingdom, pos.x, pos.y);
	
	printf("=== End NPC Wall Hall Detail ===\n");
	
}

void RecvWallHallDel(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t  type  = read_u8(data + offset);  offset += 1;
	uint32_t index = read_u32(data + offset); offset += 4;
	
	if (type > 1) {
		return;
	}
	
	if (index >= 30) {
		return;
	}
	
	if (type == 0) {
		memset(&c->ally_rallies[index], 0, sizeof(Rally));
	} else {
		memset(&c->enemy_rallies[index], 0, sizeof(Rally));
	}
}


void RecvWallHallDetailClose(Connection *c, const uint8_t *data) {
	
}

void RecvWallHallDetail(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	printf("RecvWallHallDetail\n");
}

void RecvWallHallTroop(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint32_t index = read_u32(data + offset); offset += 4;
	
	if (index >= 30) return;
	
	read_raw(c->rally_members[index].name, data + offset, 13); offset += 13;
	
	c->rally_members[index].vip      = read_u8 (data + offset); offset += 1;
	c->rally_members[index].rank     = read_u8 (data + offset); offset += 1;
	
	
	c->rally_members[index].begin_time   = read_i64(data + offset); offset += 8;
	c->rally_members[index].require_time = read_u32(data + offset); offset += 4;
	
	// unknown 
	// i don't know about this maybe mana troop or sigils flags
	uint8_t unk[6];
	read_raw(unk, data + offset, 6);
	offset += 6;
	
	printf("unknown 6 byte: ");
	for (int i = 0; i < 6; i++) {
		printf("%02x ", unk[i]);
	}
	printf("\n");
	
	uint32_t troop_flag   = read_u32(data + offset); offset += 4;
	
	c->rally_members[index].troop_total = 0;
	
	for (int i = 0; i < 20; i++) {
		if ((troop_flag >> i) & 1) {
			c->rally_members[index].troops[i] = read_u32(data + offset); offset += 4;
			c->rally_members[index].troop_total += c->rally_members[index].troops[i];
		} else {
			c->rally_members[index].troops[i] = 0;
		}
	}
	
	printf("[RALLY] Index  : %u\n",  index);
	printf("[RALLY] Name   : %s\n",  c->rally_members[index].name);
	printf("[RALLY] VIP    : %u\n",  c->rally_members[index].vip);
	printf("[RALLY] Rank   : %u\n",  c->rally_members[index].rank);
	printf("[RALLY] Troop  : %u\n",  c->rally_members[index].troop_total);
	printf("[RALLY] Btime  : %ld\n", c->rally_members[index].begin_time);
	printf("[RALLY] RTime  : %us\n", c->rally_members[index].require_time);
	
	for (int tier = 0; tier < 20; tier++) {
		if (c->rally_members[index].troops[tier] == 0)
			continue;
		
		printf("[RALLY] %-11s : %u\n",
			TroopNameByIndex(tier),
			c->rally_members[index].troops[tier]);
	}
	
	printf("\n");
}


/*
public void Init(MessagePacket MP)
	{
		this.Kind = MP.ReadByte(-1);
		this.EventTime.BeginTime = MP.ReadLong(-1);
		this.EventTime.RequireTime = MP.ReadUInt(-1);
		this.AllyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.AllyCapitalPoint.pointID = MP.ReadByte(-1);
		this.AllyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.AllyName, -1);
		this.AllyNameID = this.AllyName.GetHashCode(false);
		this.AllyVIP = MP.ReadByte(-1);
		this.AllyRank = MP.ReadByte(-1);
		if (this.PositionInfo != 1)
		{
			this.AllyCurrTroop = MP.ReadUInt(-1);
		}
		this.AllyMAXTroop = MP.ReadUInt(-1);
		this.EnemyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.EnemyCapitalPoint.pointID = MP.ReadByte(-1);
		this.EnemyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.EnemyName, -1);
		this.EnemyVIP = MP.ReadByte(-1);
		this.EnemyRank = MP.ReadByte(-1);
		MP.ReadStringPlus(3, this.EnemyAllianceTag, -1);
		this.EnemyHomeKingdom = MP.ReadUShort(-1);
		this.WonderID = byte.MaxValue;
		this.UIWonderID = byte.MaxValue;
	}
	
	*/

void WarRallyLog(Rally *r) {
	if (r->type == 0) {
		printf(
			"[ALLY RALLY] %s -> %s (%u/%u)\n",
			r->ally_name,
			r->enemy_name,
			r->ally_curr_troop,
			r->ally_max_troop
		);
	} else {
		printf(
			"[ENEMY RALLY] %s is rallying on %s reinforced (%u/%u)\n",
			r->enemy_name,
			r->ally_name,
			r->ally_curr_troop,
			r->ally_max_troop
		);
	}
}

// War rally information 
void RecvWallHallData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	Rally *rally;
	
	// type = 0 means our ally rallies; type = 1 is enemy rally
	uint8_t  type    =  read_u8 (data + offset); offset += 1;
	uint32_t index   =  read_u32(data + offset); offset += 4;
	
	if (type > 1) return;
	if (index >= 30) return;
	
	if (type == 0) {
		rally = &c->ally_rallies[index];
	} else if (type == 1) {
		rally = &c->enemy_rallies[index];
	} else {
		return;
	}
	
	rally->type         = type;
	rally->index        = index;
	
	rally->kind            = read_u8 (data + offset); offset += 1;
	rally->begin_time      = read_i64(data + offset); offset += 8;
	rally->require_time    = read_u32(data + offset); offset += 4;
	rally->ally_zone_id    = read_u16(data + offset); offset += 2;
	rally->ally_point_id   = read_u8(data + offset);  offset += 1;
	rally->ally_head       = read_u16(data + offset); offset += 2;
	read_raw(rally->ally_name, data + offset, 13); offset += 13;
	rally->ally_vip        = read_u8 (data + offset); offset += 1;
	rally->ally_rank       = read_u8 (data + offset); offset += 1;
	
	
	rally->ally_curr_troop = read_u32(data + offset); offset += 4;
	rally->ally_max_troop  = read_u32(data + offset); offset += 4;
	
	rally->enemy_zone_id   = read_u16(data + offset); offset += 2;
	rally->enemy_point_id  = read_u8 (data + offset); offset += 1;
	rally->enemy_head      = read_u16(data + offset); offset += 2;
	read_raw(rally->enemy_name, data + offset, 13); offset += 13;
	
	rally->enemy_vip       = read_u8(data + offset); offset += 1;
	rally->enemy_rank      = read_u8(data + offset); offset += 1;
	
	read_raw(rally->enemy_alliance_tag, data + offset, 3); offset += 3;
	
	rally->enemy_home_kingdom = read_u16(data + offset); offset += 2;
	
	WarRallyLog(rally);
	
	return;
	
	
	map_pos_t pos = getTileMapPosbyPointCode(rally->ally_zone_id, rally->ally_point_id);
	
	printf("\n\n\n");
	
	printf("=== Wall Hall Data ===\n");
	printf("type: %u\n", rally->type);
	printf("index: %u\n", rally->index);
	printf("kind: %u\n", rally->kind);
	printf("begin_time: %lld\n", (long long)rally->begin_time);
	printf("require_time: %u\n", rally->require_time);

	printf("ally_zone_id: %u\n", rally->ally_zone_id);
	printf("ally_point_id: %u\n", rally->ally_point_id);
	printf("ally_head: %u\n", rally->ally_head);
	printf("ally_name: %s\n", rally->ally_name);

	printf("ally_vip: %u\n", rally->ally_vip);
	printf("ally_rank: %u\n", rally->ally_rank);

	printf("ally_curr_troop: %u\n", rally->ally_curr_troop);
	printf("ally_max_troop: %u\n", rally->ally_max_troop);
	
	printf("enemy_zone_id: %u\n", rally->enemy_zone_id);
	printf("enemy_point_id: %u\n", rally->enemy_point_id);
	printf("enemy_head: %u\n", rally->enemy_head);
	printf("enemy_name: %s\n", rally->enemy_name);
	
	printf("enemy_vip:  %u\n", rally->enemy_vip);
	printf("enemy_rank: %u\n", rally->enemy_rank);
	
	printf("enemy_alliance_tag: %s\n", rally->enemy_alliance_tag);
	printf("enemy_home_kingdom: %u\n", rally->enemy_home_kingdom);
	
	printf("Ally : %s X:%u Y:%u\n", rally->ally_name, pos.x, pos.y);
	
	pos = getTileMapPosbyPointCode(rally->enemy_zone_id, rally->enemy_point_id);
	
	printf("Enemy: %s K:%u X:%u Y:%u\n", rally->enemy_name, rally->enemy_home_kingdom, pos.x, pos.y);
	// printf("Enemy: %s K:%u X:%u Y:%u\n", 
	
	printf("=== End Wall Hall Data ===\n");
	
	printf("\n\n\n");
	
}

/*
public void Init(MessagePacket MP)
	{
		this.Kind = MP.ReadByte(-1);
		this.EventTime.BeginTime = MP.ReadLong(-1);
		this.EventTime.RequireTime = MP.ReadUInt(-1);
		this.AllyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.AllyCapitalPoint.pointID = MP.ReadByte(-1);
		this.AllyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.AllyName, -1);
		this.AllyNameID = this.AllyName.GetHashCode(false);
		this.AllyVIP = MP.ReadByte(-1);
		this.AllyRank = MP.ReadByte(-1);
		if (this.PositionInfo != 1)
		{
			this.AllyCurrTroop = MP.ReadUInt(-1);
		}
		this.AllyMAXTroop = MP.ReadUInt(-1);
		this.EnemyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.EnemyCapitalPoint.pointID = MP.ReadByte(-1);
		this.EnemyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.EnemyName, -1);
		this.EnemyVIP = MP.ReadByte(-1);
		this.EnemyRank = MP.ReadByte(-1);
		MP.ReadStringPlus(3, this.EnemyAllianceTag, -1);
		this.EnemyHomeKingdom = MP.ReadUShort(-1);
		this.WonderID = byte.MaxValue;
		this.UIWonderID = byte.MaxValue;
	}
*/



/*
void RecvNPCWallHallData(Connection *c, const uint8_t *data)
{
		this.WarhallProtocol = 2476;
		byte b = 0;
		uint num = MP.ReadUInt(-1);
		if ((int)b >= this.WarHall.Length)
		{
			return;
		}
		WarlobbyData warlobbyData;
		bool warHallInstance = this.GetWarHallInstance(b, num, out warlobbyData);
		warlobbyData.PositionInfo = 0;
		warlobbyData.InitNpc(MP);
		if (warlobbyData.AllyNameID == this.RoleAttr.Name.GetHashCode(false))
		{
			this.Sponsor = (ushort)(num + 1U);
		}
		if (warHallInstance)
		{
			GUIManager.Instance.UpdateUI(EGUIWindow.UI_Alliance_Info, 4, 0);
		}
		else
		{
			GUIManager.Instance.UpdateUI(EGUIWindow.UI_Alliance_Info, 4, (int)num);
		}
		GUIManager.Instance.UpdateUI(EGUIWindow.UI_WarLobby, 0, 0);
		GameManager.OnRefresh(NetworkNews.Refresh_QBarTime, null);
	}*/

#include "tech_research.h"

void RecvTechnologyInfo(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t offset = 0;
	
	uint16_t researchTech = read_u16(data + offset); offset += 2;
	
	read_u8(data + offset); offset += 1;
	
	int64_t num = read_u64(data + offset); offset += 8;
	
	uint32_t totalTime = read_u32(data + offset); offset += 4;
	
	/*
	printf("\n\nRecvTechnologyInfo\n");
	
	printf("researchTech: %u\n", researchTech);
	
	printf("num: %ld\n", num);
	
	printf("totalTime: %u\n", totalTime);
	*/
	
	
	uint8_t AllTechData[1024];
	read_raw(AllTechData, data + offset, 200); offset += 150;
	
	// uint8_t level = GetTechLevel(c->technology.data, TECH_BIGGER_BAGS_I);
	
	
	const TechInfo *bag1 = GetTechInfo(TECH_BIGGER_BAGS_I);
	const TechInfo *bag2 = GetTechInfo(TECH_BIGGER_BAGS_II);
	const TechInfo *bag3 = GetTechInfo(TECH_BIGGER_BAGS_III);
	
	uint8_t lv1 = GetTechLevel(AllTechData, TECH_BIGGER_BAGS_I);
	uint8_t lv2 = GetTechLevel(AllTechData, TECH_BIGGER_BAGS_II);
	uint8_t lv3 = GetTechLevel(AllTechData, TECH_BIGGER_BAGS_III);
	
	/*
	if (bag1) {
		printf("%s Lv.%u: +%.1f%%\n", bag1->name, lv1, bag1->value[lv1]);
		c->supply_capacity += bag1->value[lv1];
	}
	
	if (bag2) {
		printf("%s Lv.%u: +%.1f%%\n", bag3->name, lv2, bag2->value[lv2]);
		c->supply_capacity += bag2->value[lv2];
	}
	
	if (bag3) {
		printf("%s Lv.%u: +%.1f%%\n", bag3->name, lv3, bag3->value[lv3]);
		c->supply_capacity += bag3->value[lv3];
	}
	
	
	printf("Supply Capacity: %u\n", c->supply_capacity);
	*/
	
	/*
	
	uint16_t tech_id = 1;
	
	for (size_t i = 0; i < 200; i++) {
		uint8_t b = AllTechData[i];
		
		uint8_t level1 = b & 0x0F;         // Odd TechID
		uint8_t level2 = (b >> 4) & 0x0F;  // Even TechID
		
		const TechInfo *bag1 = GetTechInfo(tech_id);
	
		
		printf("%s Lv.%u\n", 
			bag1->name,
			level1
		);
		
		tech_id++;
		
		const TechInfo *bag3 = GetTechInfo(tech_id);
		
		printf("%s Lv.%u\n",
			bag3->name,
			level2
		);
		
		tech_id++;
    
	}
	
	*/
	
	
	/*
	printf("%u\n", GetTechLevel(AllTechData, 1));
	printf("%u\n", GetTechLevel(AllTechData, 2));
	printf("%u\n", GetTechLevel(AllTechData, 280));
	*/
	//printf("\n\n");
}

/*
Debug log
Buy Item by gems
Withdraw Squad
seq_id: 24
Type: 1
Key: 2
ItemId: 1001
Qty: 1
*/

void RecvAddConflictLine(Connection *c, const uint8_t *data) {
	// Protection disabled.
	if (!c->protection.enabled) return;
	
	// Automatic conflict recall disabled.
	if (!c->protection.recall_on_incoming_conflict) return;
	
	uint8_t march_index = read_u8(data);
	
	// Army march index maximum 8 (0-7)
	// Valid march indices: 0-7.
	if (march_index >= 8) return;
	
	// Recall the affected march if a Withdraw Squad item is available.
	if (c->items[WITHDRAW_SQUAD].quantity == 0) return;
	
	RequestTroopRecall(c, march_index);
}

typedef enum {
    PK_NONE = 0,
    PK_FOOD,
    PK_STONE,
    PK_IRON,
    PK_WOOD,
    PK_GOLD,
    PK_CRYSTAL,
    PK_SP_MINE,
    PK_CITY,
    PK_CAMP,
    PK_NPC,
    PK_YOLK,
    PK_DYNAMIC_OBSTACLE,
    PK_UNDEFINED,
    PK_MAX
} POINT_KIND;

bool IsResources(uint layoutMapInfoID) {
	return layoutMapInfoID > PK_NONE && layoutMapInfoID < PK_CITY;
}

bool IsCityOrCamp(uint layoutMapInfoID) {
	return layoutMapInfoID == PK_CAMP || layoutMapInfoID == PK_CITY;
}

void point_kind_str(int pk, char *buf) {
    const char *str;

    switch (pk) {
        case PK_NONE: str = "PK_NONE"; break;
        case PK_FOOD: str = "food"; break;
        case PK_STONE: str = "stone"; break;
        case PK_IRON: str = "ore"; break;
        case PK_WOOD: str = "wood"; break;
        case PK_GOLD: str = "gold"; break;
        case PK_CRYSTAL: str = "gem"; break;
        case PK_SP_MINE: str = "PK_SP_MINE"; break;
        case PK_CITY: str = "PK_CITY"; break;
        case PK_CAMP: str = "PK_CAMP"; break;
        case PK_NPC: str = "PK_NPC"; break;
        case PK_YOLK: str = "PK_YOLK"; break;
        case PK_DYNAMIC_OBSTACLE: str = "PK_DYNAMIC_OBSTACLE"; break;
        case PK_UNDEFINED: str = "PK_UNDEFINED"; break;
        case PK_MAX: str = "PK_MAX"; break;
        default: str = "UNKNOWN"; break;
    }
    
    strcpy(buf, str);
}


// Not implemented core logic for parse map information
void RecvMapInfoPlus(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t offset = 0;
	
}
