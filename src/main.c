#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
  #include <unistd.h>
  #include <fcntl.h>
#endif

#include "connection.h"
#include "account.h"
#include "log.h"
#include "protocol.h"
#include "net_rw.h"
#include "packet_enum.h"

#include "command.h"

// included mapping for debugging purpose 
#include "packet_map.h"


#include "map_point.h"
#include "items.h"

#include "config.h"

#include "version.h"

#define SERVER_ADDR "192.243.44.63"
#define SERVER_PORT 5999
#define BUFFER_SIZE 4096

#define GAME_MAJOR_VERSION 2
#define GAME_MINOR_VERSION 195
#define GAME_PATCH_VERSION 304
#define RUSSIAN_LANGUAGE_CODE 6

// debugging purpose 
void dump_data(const char *filename, const char *str, void *data, size_t size) {
	(void)str;
	char name[1024] = {0};

	snprintf(name, sizeof(name), "%s.bin", filename);

	FILE *file = fopen(name, "wb");
	if (file == NULL) {
		perror("open failed");
		return;
	}
	if (fwrite(data, 1, size, file) != size) {
		perror("write failed");
	}
	fclose(file);

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
	
	ResourceTransferTick(c);
	
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


void logger(Connection *c) {
	
	for (int i = 0; i < c->alliance_member.count; i++) {
		AllianceMember *m = &c->alliance_member.member[i];
		
		printf("id: %ld, ", m->user_id);
		// printf("head: %u\n",     m->head);
		printf("name: %s\n",     m->name);
		// printf("rank: %u\n",     m->rank);
		// printf("power: %lu\n",   m->power);
		// printf("troop_kill_num: %lu\n", m->troop_kill_num);
		// printf("logout_time: %ld\n", m->logout_time);
		// printf("white_list_flag: %u\n", m->white_list_flag);
		// printf("\n");
	}
	
}

void ProcessConnection(Connection *c)
{
	PacketStream *s = &c->stream;
	
	map_pos_t pos;
	
	time_t last_update = time(NULL);
	
	while (1) {
		// Tick 
		BotTick(c);
		
		/*
		time_t now = time(NULL);
		
		if (now - last_update >= 10) {
			last_update = now;
			
			logger(c);
		}
		*/
		
		int n = recv(c->sock, s->buffer + s->read_pos, BUFFER_SIZE - s->read_pos, 0);
		
		if (n > 0) {
			s->read_pos += n;
		}
		else if (n == 0) {
			LOGI("Disconnected\n");
			break;
		}
		else {
			// non-blocking case
#ifdef _WIN32
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				Sleep(1);
				continue; // no more data right now
			}
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(1000);
				continue; // no more data right now
			}
#endif

			LOGE("Recv error\n");
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
			
			c->sin.offset = 0;
			c->sin.size = s->packet_size - 4;
			memcpy(c->sin.data, s->buffer + s->parse_pos + 4, s->packet_size - 4);
			
			switch(s->packet_type) {
				case _MSG_RESP_LOGINVALIDATE:
					HandleLoginValidate(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// HandleClientGuestLogin(c, s->buffer + s->parse_pos + 4, packet_size - 4);
					LOGI("Gateway Login success!\n");
					LOGI("Gateway server disconnected\n");
					disconnect(c);
					return;
				case _MSG_GAMESERVER_LOGINLOG: 
					// kind = read_u16(s->buffer + s->parse_pos + 4);
					LOGI("Game login successful\n");
					// ServerInitOver(c);
					break;
				case _MSG_LOGIN_LOGINERRORRESP: 
					RecvLoginError(c, s->buffer + s->parse_pos + 4);
					disconnect(c);
					// printf("Login error\n");
					return;
				case _MSG_CLIENT_LOGINTOLRESP: 
					// kind = read_i32(s->buffer + s->parse_pos + 4);
					
					LOGE("Bootstrap Login failed session expired: %d\n", 0/*kind*/);
					disconnect(c);
					return;
				case _MSG_RESP_ACTIVE: 
					c->server_time = read_u64(s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_CHATMESSAGE: 
					RecvChatMessage(c, s->buffer + s->parse_pos + 4);
					
					command_handler(c, c->chat.player_name, c->chat.message);
					
					if (c->chat.message[0] != c->bot.command_prefix) {
						printf("[MSG] [%s]: %s\n", c->chat.player_name, c->chat.message);
					}
					break;
				case _MSG_RESP_SOCIAL_DATA: 
					RequestAllianceGiftInfo(c);
					
					RequestRallyList(c);
					
					RequestAllianceMemberInfo(c);
					
					/*
					RequestViewChat(
						c, // Connection *c
						0, // Channel 0 for world, 1 for guild 
						0, // Previous message 
						3, // kind
						0, // Data id
						c->server_time + 5
					);
					*/
					
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
					
					LOGI("Character: %s\n", c->player.name);
					LOGI("Location: K:%u X:%u Y:%u\n", c->player.current_kingdom_id, pos.x, pos.y);
					LOGI("VIP: %u\n", GetVIPLevel(c->player.vip_point ));
					LOGI("Might: %lu\n", c->player.power);
					LOGI("Kills: %lu\n", c->player.kills);
					LOGI("Gems: %u\n", c->player.gems);
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
					// printf("_MSG_RESP_LOGIN_VERIFY_SESSION\n");
					break;
				case _MSG_RESP_BUILDINGINFO: 
					RecvAllBuildData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_MAILINFO: 
					RecvMailInfo(c, s->buffer + s->parse_pos + 4);
					
					command_handler(c, c->mail.sender_name, c->mail.content);
					
					if (c->chat.message[0] != c->bot.command_prefix) {
						printf("[MAIL] [%s]: %s\n", c->mail.sender_name, c->mail.content);
					}
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
					printf("RecvUpdateWatchTowerAddLineInfo()\n");
					// dump_data("_MSG_RESP_UPDATEWATCHTOWER_ADDLINE", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case 0x0B2B: 
					RecvRoleUpdateInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BROCAST_NPC_WAR_BEGIN: 
					RecvDarknestBroadcast(c, s->buffer + s->parse_pos + 4);
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
					// dump_data("_MSG_RESP_NPC_WARHALL_UPDATE_LISTELE", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
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
					break;
				case _MSG_RESP_WARHALL_DELETE_LISTELE:
					RecvWallHallDel(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WARHALL_END_LISTDETAIL: 
					RecvWallHallDetailClose(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_JOINED_RALLYDATA: 
					RecvJoinedRallyData(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_JOINED_RALLYDATA", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
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
					break;
				case _MSG_RESP_UPDATE_MAPINFO_PLUS: 
					RecvMapInfoPlus(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_SEND_RESHELP: 
					RecvSHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_RESHELP_HOME: 
					RecvHelp_Home(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_ALLIANCE_MEMBERINFO: 
					RecvAllianceMemberInfo(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_ALLIANCE_MEMBERINFO", "", s->buffer + s->parse_pos + 4, s->packet_size - 4);
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
					
					
					/*
					dump_data(get_packet_name(s->packet_type), "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					*/
					
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
	
	/*
	client->lobby_server_addr = 0;
	client->lobby_server_port = 0;
	
	client->game_server_addr = 0;
	client->game_server_port = 0;
	*/
	
	// Game Version And Language
	client->app.version_major = 2;
	client->app.version_minor = 197;
	client->app.version_patch = 307;
	client->app.language_code = 1;   // g_config.language_code;
	
	
	
	
	
	// client->cargo_ship.settings.auto_trade = true;
	
	
	// Automatically purchase desired Black Market (Cargo ship) items.
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
	client->alliance.auto_open_gifts = true; // Guild Gift will not open auto if set false 
	
	
	// Master switch for the protection system.
	client->protection.enabled = true;
	
	/* Shield */
	client->protection.shield_always_on = false; // shield 24/7
	client->protection.shield_on_incoming_attack = true; // shield when army invading 
	client->protection.shield_on_incoming_scout = true; // shield when scout approach
	
	/* Shield priority order.
	 * The bot tries shields from highest priority to lowest priority.
	 * If the first shield is unavailable, it falls back to the next available shield.
	 */
	client->protection.shield_priority_count = 4;
	client->protection.shield_priority[0] = SHIELD_4H;  // Priority 1
	client->protection.shield_priority[1] = SHIELD_8H;  // Priority 2
	client->protection.shield_priority[2] = SHIELD_12H; // Priority 3
	client->protection.shield_priority[3] = SHIELD_1D;  // Priority 4
	
	// Additional shields available for fallback.
	// Currently not included in the priority list above.
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
	
	// client->protection.shelter_always = false;
	// client->protection.shelter_leader = true;
	// client->protection.shelter_troops = false;
	// client->protection.shelter_on_incoming_attack = true;
	// client->protection.shelter_on_incoming_scout = true;
	
	/*
	 * Darknest Configuration 
	 * Automatic Darknest settings
	 * Currently core logic not implemented yet
	 */ 
	client->darknest.auto_join = true;
	client->darknest.min_level = 4;
	client->darknest.max_level = 6;
	
	// Maximum number of marches the bot can use for Darknest rallies at the same time (if available)
	client->darknest.max_march = 2;
	
	// Automatically set Darknest essence in Transmutation Lab
	client->darknest.auto_transmute = true;
	
	// Desired essence level
	client->darknest.essence_level = 18;
	
	// Do not join if the rally host is more than 200 miles away.
	client->darknest.max_distance = 200; 
	
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
	
	
	// currently no banking system implemented 
	client->bank.enabled   = true;
	client->bank.send_food = true;
	client->bank.send_rock = true;
	client->bank.send_wood = true;
	client->bank.send_ore  = true;
	client->bank.send_gold = true;
	
	client->bank.reserve.food = 0;
	client->bank.reserve.rock = 0;
	client->bank.reserve.wood = 0;
	client->bank.reserve.ore  = 0;
	client->bank.reserve.gold = 0;
	
	client->bank.max_delivery_distance = 100;
	
	client->bank.use_bag_rss  = false;
	client->bank.use_bag_food = false;
	client->bank.use_bag_rock = false;
	client->bank.use_bag_wood = false;
	client->bank.use_bag_ore  = false;
	client->bank.use_bag_gold = false;
	
}

void PrintUsage(void)
{
	printf(
		"Lords Mobile Bot\n"
		"\n"
		"Usage:\n"
		"  client <config_file>         Load and start the bot using the specified configuration file.\n"
		"  client --create-config, -c   Create a default configuration file.\n"
		"  client --help, -h            Display this help message.\n"
		"  client --version, -v         Display version information.\n"
		"\n"
		"Project:\n"
		"  https://github.com/halloweeks/lords-mobile-bot\n"
	);
}

void PrintVersion(void)
{
    printf(
        "Lords Mobile Bot v%s\n"
        "Build: %s %s\n"
        "Project: https://github.com/halloweeks/lords-mobile-bot\n",
        VERSION,
        __DATE__,
        __TIME__);
}

bool CreateDefaultConfig(const char *filename)
{
	FILE *fp = fopen(filename, "w");
	
	if (!fp)
		return false;
	
	fprintf(fp,
		"# Lords Mobile Bot Configuration\n"
		"# Generated automatically. Edit this file to configure the bot.\n"
		"# Project: https://github.com/halloweeks/lords-mobile-bot\n"
		"# Docs: https://github.com/halloweeks/lords-mobile-bot/blob/main/docs/configuration.md\n\n"
		
		"# Gateway server\n"
		"server.addr = 192.243.44.63\n"
		"server.port = 5999\n\n"
		
		"# Client version\n"
		"client.version_major = 2\n"
		"client.version_minor = 197\n"
		"client.version_patch = 308\n"
		"client.language_code = 1\n\n"
		
		"# Directory used to store bot data (logs, databases, cache, etc.).\n"
		"data.path = /sdcard/lmbot/\n\n"
		
		"# Privileged player.\n"
		"# This player can execute administrator commands and bypass normal restrictions.\n"
		"admin.name = halloweeks\n\n"
		
		"# Replace the example values below with your own account information.\n"
		"account.igg_id = 1234567890\n"
		"account.device_uuid = 12345678-1234-1234-1234-123456789abc\n"
		"account.access_key = YOUR_ACCESS_KEY_HERE\n\n"
		
		"# Prefix used to identify bot commands.\n"
		"command.prefix = $\n\n"
		
		"# Command channels: WORLD, GUILD, MAIL\n"
		"command.input = GUILD\n"
		"command.output = MAIL\n\n"
		
		"# Bank\n"
		"# Master switch for the banking system.\n"
		"# When enabled, the bot accepts and processes banking commands.\n"
		"# When disabled, all banking commands are ignored.\n"
		"bank.enabled = false\n\n"
		
		"# Resource types allowed for delivery.\n"
		"bank.send_food = false\n"
		"bank.send_rock = false\n"
		"bank.send_wood = false\n"
		"bank.send_ore  = false\n"
		"bank.send_gold = false\n\n"
		
		"# Resource reserve.\n"
		"# These values are reserved for the bot's own use. The bot will not send\n"
		"# resources that would reduce the balance below these amounts.\n"
		"bank.reserve_food = 20M\n"
		"bank.reserve_rock = 50M\n"
		"bank.reserve_wood = 50M\n"
		"bank.reserve_ore  = 30M\n"
		"bank.reserve_gold = 0\n\n"
		
		"# Maximum map distance (tiles) for resource delivery.\n"
		"bank.max_delivery_distance = 100\n\n"
		
		"# Automatically use resource items from the bag if the available\n"
		"# resources are insufficient to fulfill a banking command.\n"
		"bank.use_bag_rss  = false\n"
		"bank.use_bag_food = false\n"
		"bank.use_bag_rock = false\n"
		"bank.use_bag_wood = false\n"
		"bank.use_bag_ore  = false\n"
		"bank.use_bag_gold = false\n\n"
		
		/*
		"# Alliance\n"
		"alliance.auto_help = false\n"
		"alliance.auto_open_gifts = false\n\n"
		*/
		
		
		"# Enable or disable all automatic protection features.\n"
		"protection.enabled = false\n\n"
		
		"# Keep a shield active at all times.\n"
		"protection.shield_always_on = false\n\n"
		
		"# Automatically use a shield when an incoming attack is detected.\n"
		"protection.shield_on_incoming_attack = false\n\n"
		
		"# Automatically use a shield when an incoming scout is detected.\n"
		"protection.shield_on_incoming_scout = false\n\n"
		
		"# Shield priority list.\n"
		"# The bot will use the first available shield in this order.\n"
		"# Available shields:\n"
		"# SHIELD_4H, SHIELD_8H, SHIELD_12H, SHIELD_1D, SHIELD_3D, SHIELD_7D, SHIELD_14D\n"
		"protection.shield_priority = SHIELD_4H, SHIELD_8H, SHIELD_12H, SHIELD_1D\n"
		
		"\n\n# Automatically recall marches when an incoming attack is detected.\n"
		"protection.recall_on_incoming_attack = false\n\n"
		
		"# Automatically recall marches when an incoming scout is detected.\n"
		"protection.recall_on_incoming_scout = false\n\n"
		
		"# Automatically recalls a gathering or camp march before it reaches\n"
		"# its destination when an incoming conflict is detected.\n"
		"# Requires Withdraw Squad items; otherwise no action is taken.\n"
		"protection.recall_on_incoming_conflict = false\n\n"
		
		"# Currently this feature not available\n"
		"# protection.shelter_always = false\n"
		"# protection.shelter_leader = true\n"
		"# protection.shelter_troops = false\n"
		"# protection.shelter_on_incoming_attack = true\n"
		"# protection.shelter_on_incoming_scout = true\n\n"
		
		
		"# Cargo Ship Trading\n"
		"# Automatically completes Cargo Ship trades.\n"
		"# The options below specify which resources the bot is allowed to spend.\n"
		"cargo_ship.auto_trade  = false\n"
		"cargo_ship.spend_food  = false\n"
		"cargo_ship.spend_rock  = false\n"
		"cargo_ship.spend_wood  = false\n"
		"cargo_ship.spend_ore   = false\n"
		"cargo_ship.spend_gold  = false\n\n"
		
		"# Use resource items from the bag when required to complete a trade.\n"
		"# If disabled, the bot will never consume bag resource items.\n"
		"cargo_ship.use_bag_rss = false\n\n"
		
		"# Resource reserve limits.\n"
		"# The bot always keeps at least this amount and only spends the excess.\n"
		"# Set to 0 to disable the reserve.\n"
		"cargo_ship.reserve_food = 10M\n"
		"cargo_ship.reserve_rock = 10M\n"
		"cargo_ship.reserve_wood = 10M\n"
		"cargo_ship.reserve_ore  = 10M\n"
		"cargo_ship.reserve_gold = 10M\n\n"

	);

    fclose(fp);
    return true;
}

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		PrintUsage();
		return 0;
	}
	
	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		PrintUsage();
		return 0;
	}
	
	if (strcmp(argv[1], "--create-config") == 0 || strcmp(argv[1], "-c") == 0) {
		if (CreateDefaultConfig("config.cfg")) {
			printf("[INFO ] Default configuration generated: config.cfg\n");
			return 0;
		}
		
		printf("[ERROR] Failed to create configuration file: config.cfg\n");
		return 1;
	}
	
	if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		PrintVersion();
		return 0;
	}

#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		LOGE("WSAStartup failed");
		return 1;
	}
#endif
	
	if (argc != 2) {
        printf("Usage: %s <config.cfg>\n", argv[0]);
        return EXIT_FAILURE;
    }
	
	Connection client = {0};
	
	// Configuration(&client);
	
	if (!LoadConfig(&client, argv[1])) {
		LOGE("Failed to load config\n");
		return EXIT_FAILURE;
	}
	
	LOGI("Configuration loaded\n");
	
	// Establish TCP connection to server and store socket descriptor in client
	client.sock = connect_server(client.gateway_server.addr, client.gateway_server.port);
	
	// Validate socket creation; -1 indicates connection failure
	if (client.sock == -1) {
		LOGE("Failed to connect to %s:%d\n", client.gateway_server.addr, client.gateway_server.port);
		return EXIT_FAILURE;
	}
	
	LOGI("Connected gateway server: %s:%u\n", client.gateway_server.addr, client.gateway_server.port);
	LOGI("Game client: v%u.%u.%u\n", client.app.version_major, client.app.version_minor, client.app.version_patch);
	
	RequestGuestLogIn(&client);
		
	ProcessConnection(&client);
	
	if (!client.lobby_login) {
		LOGE("Login failed!\n");
		return EXIT_FAILURE;
	}
	
	// Establish TCP connection to game server 
	client.sock = connect_server(client.game_server.addr, client.game_server.port);
	
	if (client.sock == -1) {
		// LOGE("Failed to connect game server!");
		LOGE("Failed to connect game server %s:%d\n", client.game_server.addr, client.game_server.port);
		return EXIT_FAILURE;
	}
	
	if (set_nonblocking(&client) != 0) {
		LOGE("set_nonblocking\n");
	}
	
	LOGI("IGG ID: %lu\n", client.auth.igg_id);
	
	LOGI("Connected game server: %s:%u\n", client.game_server.addr, client.game_server.port);
	
	// clear old buffer 
	memset(&client.stream, 0, sizeof(client.stream));
	
	// Game login
	LOGI("Logging game server\n");
	RequestLogIn(&client);
	
	RequestClientInitOver(&client);
	
	// Handle 
	ProcessConnection(&client);
	
	disconnect(&client);
	
	return 0;
}
