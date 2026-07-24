#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
  #include <unistd.h>
  #include <fcntl.h>
#endif

#include "connection.h"
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
