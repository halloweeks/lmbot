#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "connection.h"
#include "account.h"
#include "log.h"
#include "protocol.h"
#include "net_rw.h"
#include "packet_enum.h"

// included mapping for debugging purpose 
#include "packet_map.h"


#include "map_point.h"
#include "items.h"


#define SERVER_ADDR "192.243.44.63"
#define SERVER_PORT 5999
#define BUFFER_SIZE 4096

#define GAME_MAJOR_VERSION 2
#define GAME_MINOR_VERSION 195
#define GAME_PATCH_VERSION 304
#define RUSSIAN_LANGUAGE_CODE 6

// debugging purpose 
void dump_data(const char *filename, const char *str, void *data, size_t size) {
	char name[1024] = {0};
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts); // nanosecond precision
	
	// Use seconds + nanoseconds in filename (avoid collisions)
	// snprintf(name, sizeof(name), "/sdcard/lmbot/logs/%ld_%ld_%s_%s", ts.tv_sec, ts.tv_nsec, str, filename);
	
	snprintf(name, sizeof(name), "/sdcard/lmbot/logs/%s.bin", filename);
	
	
	int file = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (file < 0) {
		perror("open failed");
		return;
	}
	if (write(file, data, size) != (ssize_t)size) {
		perror("write failed");
	}
	close(file);

	printf("file: %s saved\n", name);
}

void BotTick(Connection *c)
{
	if (c->server_time == 0) return;
	
	// Connection maintenance
	HeartbeatTick(c);
	
	// Automatic game features
	BlackMarketTick(c);
	
	// 
	ShieldTick(c);
	
	AllianceGiftTick(c);
	
	// 
	// DarknestRallyTick(c);
	
}

uint8_t GetVIPLevel(uint32_t vipPoints)
{
	if (vipPoints >= 1500000) return 15;
	if (vipPoints >= 730000)  return 14;
	if (vipPoints >= 350000)  return 13;
	if (vipPoints >= 175000)  return 12;
	if (vipPoints >= 90000)   return 11;
	if (vipPoints >= 50000)   return 10;
	if (vipPoints >= 20000)   return 9;
	if (vipPoints >= 8000)	return 8;
	if (vipPoints >= 4000)	return 7;
	if (vipPoints >= 1600)	return 6;
	if (vipPoints >= 800)	 return 5;
	if (vipPoints >= 400)	 return 4;
	if (vipPoints >= 300)	 return 3;
	if (vipPoints >= 100)	 return 2;
	
	return 1;
}

void ProcessConnection(Connection *c)
{
	PacketStream *s = &c->stream;
	
	map_pos_t pos;
	
	while (1) {
		// Tick 
		BotTick(c);
		
		int n = recv(c->sock, s->buffer + s->read_pos, BUFFER_SIZE - s->read_pos, 0);
		
		if (n > 0) {
			s->read_pos += n;
		}
		else if (n == 0) {
			LOGI("Disconnected");
			break;
		}
		else {
			// non-blocking case
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(1000);
				continue; // no more data right now
			}

			LOGE("Recv error");
			break;
		}
		
		while (s->read_pos - s->parse_pos >= 4) {
			s->packet_size = read_u16(s->buffer + s->parse_pos);
			s->packet_type = read_u16(s->buffer + s->parse_pos + 2);
			
			if (s->packet_size < 4 || s->packet_size > BUFFER_SIZE) {
				s->parse_pos += 4;
				continue;
			}
			
			if (s->read_pos - s->parse_pos < s->packet_size) {
				break;
			}
			
			switch(s->packet_type) {
				case _MSG_RESP_LOGINVALIDATE:
					HandleLoginValidate(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// HandleClientGuestLogin(c, s->buffer + s->parse_pos + 4, packet_size - 4);
					LOGI("Bootstrap Login success!");
					LOGI("Bootstrap disconnected");
					disconnect(c);
					return;
				case _MSG_GAMESERVER_LOGINLOG: 
					// kind = read_u16(s->buffer + s->parse_pos + 4);
					// LOGI("Game Server login success");
					// ServerInitOver(c);
					break;
				case _MSG_LOGIN_LOGINERRORRESP: 
					RecvLoginError(c, s->buffer + s->parse_pos + 4);
					disconnect(c);
					return;
				case _MSG_CLIENT_LOGINTOLRESP: 
					// kind = read_i32(s->buffer + s->parse_pos + 4);
					
					LOGE("Bootstrap Login failed session expired: %d", 0/*kind*/);
					disconnect(c);
					return;
				case _MSG_RESP_ACTIVE: 
					c->server_time = read_u64(s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_CHATMESSAGE: 
					RecvChatMessage(c, s->buffer + s->parse_pos + 4);
					
					// command_handler(c, c->chat.player_name, c->chat.message);
					
					if (c->chat.message[0] != c->bot.command_prefix) {
						printf("[MSG] [%s]: %s\n", c->chat.player_name, c->chat.message);
					}
					break;
				case _MSG_RESP_SOCIAL_DATA: 
					RequestAllianceGiftInfo(c);
					
					RequestRallyList(c);
					// RequestDeleteAllianceGiftBox(c, 0xFFFFFFFF);
					break;
				case _MSG_RESP_ITEMINFO: 
					RecvItemInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_BUFFINFO: 
					RecvIBuffInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_MARCH_MARCHEVENTDATA:
					RecvMarchData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_LOGIN_ROLEINFO: 
					RecvLoginRoleInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					
					
					pos = getTileMapPosbyPointCode(c->player.zone_id, c->player.point_id);
					
					printf("%s K:%u X:%u Y:%u\n", c->player.name, c->player.current_kingdom_id, pos.x, pos.y);
					printf("VIP Lv: %u\n", GetVIPLevel(c->player.vip_point ));
					printf("might: %lu\n", c->player.power);
					printf("kills: %lu\n", c->player.kills);
					printf("gems: %u\n", c->player.gems);
					break;
				case _MSG_RESP_UPDATE_RESOURCEAMOUNT: 
					RecvRefreshResources(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_RESOURCEINFO: 
					RecvResources(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BLACKMARKET_DATA: 
					RecvBlackMarket_Data(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BLACKMARKET_BUY:
					RecvBlackMarket_Buy(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_LOGIN_VERIFY_SESSION: 
					// dump_data("verify_session", "", s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_BUILDINGINFO: 
					RecvAllBuildData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_MAILINFO: 
					RecvMailInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_ALLYPOINT: 
					RecvAllyPoint(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_ALLIANCE_HELP: 
					printf("_MSG_RESP_ALLIANCE_HELP\n");
					// RecvAllianceHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case 0xB26:
					RecvAllianceMemberNeedsHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case 0x0B23:
					RecvPendingAllianceMembersNeedHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case 0x0B2F: 
					RecvAllianceGiftInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case 0x0b33: 
					RecvDeleteAllianceGiftBox(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_USEITEM: 
					RecvUseItem(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// dump_data("_MSG_RESP_USEITEM", "", s->buffer + s->parse_pos, s->packet_size);
					break;
				case 0x0B31: 
					RecvAllianceGiftOpen(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BUYITEM: 
					RecvBuyItem(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// dump_data("_MSG_RESP_BUYITEM", "", s->buffer + s->parse_pos, s->packet_size);
					break;
				case _MSG_RESP_WORLD_TELEPORT_ITEM:
					// dump_data("_MSG_RESP_WORLD_TELEPORT_ITEM", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_REQUEST_ALLIANCE_INFO:
					RecvAllianceInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BUILDINGEVENT: 
					RecvBuildingQueue(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_UPDATEWATCHTOWER_ADDLINE:
					RecvUpdateWatchTowerAddLineInfo(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_UPDATEWATCHTOWER_ADDLINE", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case 0x0B2B: 
					RecvRoleUpdateInfo(c, s->buffer + s->parse_pos + 4);
					// dump_data("RecvRoleUpdateInfo", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					
					
					// printf("ClientAllianceData\n");
					break;
				case _MSG_RESP_BROCAST_NPC_WAR_BEGIN: 
					RecvDarknestBroadcast(c, s->buffer + s->parse_pos + 4);
					// printf("Darknest: %u\n",s->packet_size + 4);
					// dump_data("RecvDarknestBroadcast", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_ARMYGROUPINFO_: 
					RecvArmyGroupInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WATCHTOWER_LINEDETAIL: 
					// RecvWatchTowerLineDetail(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WARHALL_INITLIST: 
					RecvRallyCountData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_NPC_WARHALL_UPDATE_LISTELE: 
					RecvNPCWallHallData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_NPC_WARHALL_INIT_LISTDETAIL: 
					RecvNPCWallHallDetail(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_NPC_WARHALL_INIT_LISTDETAIL", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_BROCAST_WAR_BEGIN: 
					RecvWarBegin(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WARHALL_UPDATE_LISTDETAIL:
					RecvWallHallTroop(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_WARHALL_UPDATE_LISTDETAIL", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_WARHALL_UPDATE_LISTELE: 
					RecvWallHallData(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_WARHALL_UPDATE_LISTELE", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_WARHALL_INIT_LISTDETAIL:
					RecvWallHallDetail(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_WARHALL_INIT_LISTDETAIL", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_WARHALL_DELETE_LISTELE:
					RecvWallHallDel(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_RESEARCHINFO:
					RecvTechnologyInfo(c, s->buffer + s->parse_pos + 4, s->packet_size + 4);
					// dump_data("_MSG_RESP_RESEARCHINFO", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_ADDCONFLICT_LINE: 
					RecvAddConflictLine(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_HOSPITAL_HOSPITALINFO: 
					RecvWoundedTroopData(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_HOSPITAL_HOSPITALINFO", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					// RecvHospitalInfo
					break;
				case _MSG_RESP_UPDATE_MAPINFO_PLUS: 
					RecvMapInfoPlus(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				default:
					
					/*
					// Debugging purpose only 
					LOGI("PACKET TYPE: %s (0x%X), size=%u",
						get_packet_name(s->packet_type),
						s->packet_type,
						s->packet_size
					);
					*/
					
					//dump_data(get_packet_name(s->packet_type), "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					
					
					break;
			}
			
			s->parse_pos += s->packet_size;
		}
		
		if (s->parse_pos > 0) {
			memmove(s->buffer, s->buffer + s->parse_pos, s->read_pos - s->parse_pos);
			s->read_pos -= s->parse_pos;
			s->parse_pos = 0;
		}
	}
}
 

// Configuration settings 
void Configuration(Connection *client)
{
	
	// Default settings
	// Command prefix
	client->bot.command_prefix = '$';
	// Data folder 
	strcpy(client->bot.data_path, "./lmbot/");
	// default admin
	strcpy(client->bot.admin_name, "halloweeks");
	
	// Game Version And Language
	client->app.version_major = 2;
	client->app.version_minor = 197;
	client->app.version_patch = 307;
	client->app.language_code = 1;   // g_config.language_code;
	
	// Automatically purchase desired Black Market items.
	client->market.settings.auto_trade = true;
	
	// Minimum resources to keep after market purchases.
	client->market.reserve.food = 0;
	client->market.reserve.rock = 0;
	client->market.reserve.wood = 0;
	client->market.reserve.ore  = 0;
	client->market.reserve.gold = 0;
	
	// Allow these resources to be spent on Black Market trades.
	client->market.settings.spend_food = true;
	client->market.settings.spend_rock = true;
	client->market.settings.spend_wood = true;
	client->market.settings.spend_ore  = true;
	client->market.settings.spend_gold = true;
	
	// Guild Auto help
	client->alliance.auto_help = true;
	
	// Open alliance gifts
	client->alliance.auto_open_gifts = true; // Gift will not open auto
	
	
	
	
	
	// Master switch for the protection system.
	client->protection.enabled = true;
	
	/* Shield */
	client->protection.shield_always_on = false;
	client->protection.shield_on_incoming_attack = true;
	client->protection.shield_on_incoming_scout = true;
	
	// Shield priority (first available shield is used).
	client->protection.shield_priority_count = 4;
	client->protection.shield_priority[0] = SHIELD_4H; // 1
	client->protection.shield_priority[1] = SHIELD_8H; // 2
	client->protection.shield_priority[2] = SHIELD_12H;// 3
	client->protection.shield_priority[3] = SHIELD_1D; // 4
	// Unused 
	client->protection.shield_priority[4] = SHIELD_3D;
	client->protection.shield_priority[5] = SHIELD_7D;
	client->protection.shield_priority[6] = SHIELD_14D;
	
	/* Troop recall */
	// Recalls targeted camped or gathering troops.
	client->protection.recall_on_incoming_attack = true;
	client->protection.recall_on_incoming_scout = true;
	
	// Automatically recalls a gathering or camp march before it reaches
	// its destination when an incoming conflict is detected.
	// Requires Withdraw Squad items; otherwise no action is taken.
	client->protection.recall_on_incoming_conflict = true;
	
	
	//
	// client->protection.shelter_on_incoming_attack = true;
	// client->protection.shelter_on_incoming_scout = true;
	
	/*
	 * Darknest Configuration 
	 * Currently core logic not implemented yet
	 */ 
	client->darknest.auto_join = true;
	client->darknest.min_level = 4;
	client->darknest.max_level = 6;
	
	// Total troops to send when joining Darknest rally.
	client->darknest.troop_count = 200000;
	
	// Minimum troops required; if available troops cannot reach this, do not join.
	client->darknest.min_join_troops = 150000;
	
	client->darknest.formation_mode = DARKNEST_FORMATION_LEADER;
	
	// Used only when formation_mode == DARKNEST_FORMATION_FIXED
	// Troop ratio (8480 = 80% Inf, 40% Ranged, 80% Cavalry, 0% Siege).
	client->darknest.formation = 8480;
	
	// Join random delay between.
	client->darknest.min_join_delay = 3;
	client->darknest.max_join_delay = 180;
	
	// Darknest troop priority order: bot tries higher tier troops first (T5 → T1) when selecting troops for rally join.
	client->darknest.tier_priority_count = 3;
	client->darknest.tier_order[0] = TIER_T5; // T5
	client->darknest.tier_order[1] = TIER_T4;
	client->darknest.tier_order[2] = TIER_T3;
	client->darknest.tier_order[3] = TIER_T2;
	client->darknest.tier_order[4] = TIER_T1;
	
	
	return;
}

int main(int argc, const char *argv[]) {
	if (argc != 2) {
		printf("Usage: %s <account_file>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	Connection client = {0};
	memset(&client, 0, sizeof(client));
	
	// You can load these data from configuration file 
	Configuration(&client);
	
	// Load account credentials from file
	// AccountError err = LoadAccount(&client, "./lmbot/new_account.bin");
	
	AccountError err = LoadAccount(&client, argv[1]);
	
	if (err != ACC_OK) {
		LOGE("LoadAccount failed: %s", AccountErrorStr(err));
		return EXIT_FAILURE;
	}
	
	
	// If you don't use a serialized account file, you can initialize the
	// authentication information manually before connecting.
	//
	// Required fields:
	//   client.auth.igg_id        = Your IGG account ID (uint64_t)
	//   client.auth.device_uuid   = Device UUID (copy with memcpy)
	//   client.auth.session_len   = Session length in bytes
	//   client.auth.session       = Session token received after login (copy using memcpy)
	//
	// This allows you to authenticate without calling LoadAccount().
	// Example test account 
	// client.auth.igg_id = 2147586080;
	// memcpy(client.auth.device_uuid, "6f694273-67c2-46c5-a498-501dd571c97a", 36);
	// memcpy(client.auth.session, "eyJha2lkIjoyMDIxMDQyMiwicmtpZCI6MjAyMTA0MjMsImciOjEwNTEwMjk5MDIsInYiOjN9.rXr4A-8Muc8e5BWKhenJxRjwE-AubCJdOrf-BK5ZKBrpTSMhQSzj6zl58iXtRx9tqFuiUE-U3HybJzEADQXBcasgzgmmeWyyvv1KaH38Nkv8N_H9hb2vmldT5eqUmy4EhKHhosA4_uzb0FLqgAEgarae0x3IhnXR9Dig1O0iL3nel1G6THZjKyLW9YpVgAI7.Iz1RKOmxuZ6sop6dJT3DQpdSkl2Ps_OUzSTrv0E0EUFey21GTW0P5sGW7V7jJrR7PLbishKLzqROgHBck8xtG7lld4bGwASHMtj-_5oU8S0OsWfTs_nkPW6S_5M-NzX1fk-alygFptbnhScpIkGQ_quH5_g1NaI4T8dwcfAMY50", 437);
	// client.auth.session_len = (uint16_t)strlen(client.auth.session);
	
	
	// Establish TCP connection to server and store socket descriptor in client
	client.sock = connect_server(SERVER_ADDR, SERVER_PORT);
	
	// Validate socket creation; -1 indicates connection failure
	if (client.sock == -1) {
		LOGE("Failed to connect to %s:%d", SERVER_ADDR, SERVER_PORT);
		return EXIT_FAILURE;
	}
	
	LOGI("Connected bootstrap server: %s:%u", SERVER_ADDR, SERVER_PORT);
	LOGI("Game client: v%u.%u.%u", client.app.version_major, client.app.version_minor, client.app.version_patch);
	
	RequestGuestLogIn(&client);
		
	ProcessConnection(&client);
	
	if (!client.lobby_login) {
		LOGE("Login failed!");
		return EXIT_FAILURE;
	}
	
	// Establish TCP connection to game server 
	client.sock = connect_server(client.game_server.addr, client.game_server.port);
	
	if (client.sock == -1) {
		LOGE("Failed to connect game server!");
		return EXIT_FAILURE;
	}
	
	if (set_nonblocking(&client) != 0) {
		LOGE("set_nonblocking");
	}
	
	LOGI("IGG ID: %lu\n", client.auth.igg_id);
	
	LOGI("Connected game server: %s:%u", client.game_server.addr, client.game_server.port);
	
	// clear old buffer 
	memset(&client.stream, 0, sizeof(client.stream));
	
	// Game login
	RequestLogIn(&client);
	
	RequestClientInitOver(&client);
	
	// Handle 
	ProcessConnection(&client);
	
	disconnect(&client);
	
	return 0;
}
