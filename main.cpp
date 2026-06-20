#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
// nano ~/.bashrc
#include <math.h>

#include <errno.h>
#include "packet_enum.h"
#include "packet_map.h"
#include "des.h"
#include "packet.hpp"
#include "map_point.h"

#include "MAP_UPDATE_KIND.h"

#include "items.h"

#include "bank.h"

#define MAX_BANK_RANGE 100

void dump_data(const char *filename, const char *str, void *data, size_t size) {
    char name[1024] = {0};
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // nanosecond precision
    
    // Use seconds + nanoseconds in filename (avoid collisions)
    snprintf(name, sizeof(name), "/sdcard/lmlogin/log/%ld_%ld_%s_%s", ts.tv_sec, ts.tv_nsec, str, filename);

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


int connect_server(const char *ip, const unsigned int port) {
	// Create socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		perror("Socket creation error");
		return -1;
	}
	
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	
	// Convert IPv4 address from text to binary
	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		perror("Invalid address or Address not supported");
		return -1;
	}
	
	// Connect to server
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Connection failed");
		return -1;
	}
	
	return sock;
}

typedef struct {
	uint8_t logged;
	uint32_t port;
	uint64_t igg_id;
	char ip[16];
} log_t;

// 168.2.269

#define ServerVersionMajor 168

log_t bootstrap_login(const char *filename) {
	log_t res = {0};
	
	int file = open(filename, O_RDONLY);
	
	if (file == -1) {
		fprintf(stderr, "Unable to login file!\n");
		return res;
	}
	
	uint8_t data[4096];
	read(file, data, 582);
	close(file);
	
	int conn = connect_server("192.243.44.63", 5999);
	
	if (conn == -1) {
		fprintf(stderr, "Can't connect bootstrap server!\n");
		return res;
	}
	
	send(conn, data, 582, 0);
	
	
	
	int len = recv(conn, data, 4096, 0);
	
	printf("len: %u\n", len);
	
	close(conn);
	
	Packet packet;
	packet.clear();
	memcpy(packet.read_buffer, data, len);
	// packet.write_bytes(data, len);
	packet.rewind();
	
	uint16_t packet_size = packet.read_u16();
	uint16_t packet_type = packet.read_u16();
	
	printf("packet type: %s (%04x)\n", get_packet_name(packet_type), packet_type);
	
	if (packet_type == _MSG_RESP_LOGINVALIDATE) {
		res.port = packet.read_u32();
		res.igg_id = packet.read_u64();
		packet.read_bytes(res.ip, 16);
		res.logged = 1;
		return res;
	}
	
	return res;
}


void game_login(int conn, const char *filename) {
	int file = open(filename, O_RDONLY);
	
	if (file == -1) {
		fprintf(stderr, "Unable to login file!\n");
		return;
	}
	
	uint8_t data[1024];
	read(file, data, 582);
	close(file);
	
	send(conn, data, 582, 0);
}

const char* format_timestamp(time_t ts) {
    static char buf[20];  // YYYY-MM-DD HH:MM:SS + '\0'
    struct tm *tm_info = localtime(&ts);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

typedef enum {
	WORLD,
    GUILD
} MESSAGE_TYPE;

void send_message(MESSAGE_TYPE type, const char *message) {
	switch (type) {
		case GUILD:
			printf("[GUILD] %s\n", message);
			break;
		case WORLD:
			printf("[WORLD] %s\n", message);
			break;
		default:
			printf("[UNKNOWN] %s\n", message);
			break;
	}
}

typedef struct {
	uint32_t food;
	uint32_t rock;
	uint32_t wood;
	uint32_t ore;
	uint32_t gold;
} Resources;

typedef struct {
	char food[20];
	char rock[20];
	char wood[20];
	char  ore[20];
	char gold[20];
} ResourceStr;

static int current_kingdom = 0; // saved kingdom in memory

void Heartbeat(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_ACTIVE);
	packet->write_seq_id();
	packet->send(true);
}

void ServerSendChat(Packet *packet, uint8_t channel, const char *message) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_SENDCHAT);
	packet->write_seq_id();
	packet->write_u8(channel);
	packet->write_u8(0);
	packet->write_u8(5);
	packet->write_u16(strlen(message));
	packet->write_bytes(message, strlen(message));
	packet->send(true);
}

void SendEmoji(Packet *packet, uint16_t EmojiKey) {
	
}

void ServerSendChatFmt(Packet *packet, uint8_t channel, const char *fmt, ...) {
    char buf[512];  // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    ServerSendChat(packet, channel, buf);
}

void ServerViewChat(Packet *packet, uint8_t channel, uint8_t prev, int8_t kind, int64_t DataID, int64_t DataTime) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_VIEWCHAT);
	packet->write_seq_id();
	packet->write_u8(channel);
	packet->write_u8(prev);
	
	if (ServerVersionMajor != 0) {
		if (kind == -1) {
			packet->write_u8(0xFF);
		} else {
			packet->write_u8((uint8_t)kind);
		}
	}
	
	if (channel != 0) {
		packet->write_u64(DataID);
		packet->write_u64(DataTime);
	}
	
	packet->send(true);
}



bool SendAllyPoint(Packet *packet, const char *name) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_ALLYPOINT);
	packet->write_seq_id();
	packet->write_bytes(name, 13);
	return packet->send(true);
}


void RecvAllyPoint(Packet p, Packet *packet) {
	uint8_t b = p.read_u8();
	uint16_t zoneID = p.read_u16();
	uint8_t pointID = p.read_u8();
	
	if (b == 0) {
		map_pos_t map = getTileMapPosbyPointCode(zoneID, pointID);
		printf("Here: %u:%u\n", map.x, map.y);
	} else if (b == 1) {
		printf("another kingdom\n");
	} else {
		printf("error b: %u\n", b);
	}
	
	
	map_pos_t map = getTileMapPosbyPointCode(zoneID, pointID);
	printf("IDK Here: %u:%u\n", map.x, map.y);
		
	exit(1);
}


void SendClientInitOver(Packet *packet, uint64_t igg_id) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_CLIENTINITOVER);
	packet->write_seq_id();
	packet->write_u64(igg_id);
	packet->send(true);
}

bool SendResource(Packet *packet, Resources resource, uint16_t zoneId, uint8_t pointId) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_SEND_RESHELP);
	packet->write_seq_id();
	
	// printf("\n\nRequest SendResource\n");
	map_pos_t res = getTileMapPosbyPointCode(zoneId, pointId);
	// printf ("Location: X:%u Y:%u\n", res.x, res.y);
				
	// printf("food = %u\n", resource.food);
	// printf("rock = %u\n", resource.rock);
	// printf("wood = %u\n", resource.wood);
	// printf("ore  = %u\n", resource.ore);
	// printf("gold = %u\n", resource.gold);
					
	// location 
	packet->write_u16(zoneId);
	packet->write_u8(pointId);
	
	// resources 
	packet->write_u32(resource.food);
	packet->write_u32(resource.rock);
	packet->write_u32(resource.wood);
	packet->write_u32(resource.ore);
	packet->write_u32(resource.gold);
	
	return packet->send(true);
}

void RequestMapdata(Packet *packet, uint8_t count, uint16_t zone[]) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_MAPDATA);
	packet->write_seq_id();
	
	packet->write_u8(count);
	
	packet->write_u16(zone[0]);
	packet->write_u16(zone[1]);
	packet->write_u16(zone[2]);
	packet->write_u16(zone[3]);
	
	/*
	for (int i = 0; i < 4; i++) {
		printf("zone[%u]: %u\n", i, zone[i]);
	}
	*/
	
	long data = 0;
	
	for (uint8_t i = 0; i < 4; i++) {
		packet->write_u64(data);
	}
	
	packet->send(true);
}

/*
void RequsetMapdata(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_MAPDATA);
	packet->write_seq_id();
	
	packet->write_u8(1);
	
	packet->write_u16(0);
	packet->write_u16(0);
	packet->write_u16(0);
	packet->write_u16(0);
	
	long data = 0;
	
	for (uint8_t i = 0; i < 4; i++) {
		packet->write_u64(data);
	}
	
	// dump_data("req_map.bin", packet->data, packet->size);
	
	packet->send(true);
	
	printf("MSG_REQUEST_MAPDATA\n");
}*/


void RequestAdvanceMapdata(Packet *packet, uint16_t x, uint16_t y) {
	PointCode DesPoint = getPointCodeByMapPos(x, y);
		
	packet->clear();
	packet->write_u16(_MSG_REQUEST_MAP_ADVANCE);
	packet->write_seq_id();
	packet->write_u16(DesPoint.zoneID);
	packet->write_u8(DesPoint.pointID);
	packet->send(true);
	
	printf("\nMSG_REQUEST_MAP_ADVANCE\n");
	printf("ZoneId: %u\n", DesPoint.zoneID);
	printf("PointId: %u\n\n", DesPoint.pointID);
	
}

typedef enum {
    TROOP_INFANTRY = 0,
    TROOP_RANGED   = 1,
    TROOP_CAVALRY  = 2,
    TROOP_SIEGE    = 3
} TroopKind;

typedef enum {
    TIER_T1 = 0,
    TIER_T2 = 1,
    TIER_T3 = 2,
    TIER_T4 = 3,
    TIER_T5 = 4
} TroopTier;

void RequestTroopTraining(Packet *packet, uint8_t kind, uint8_t tier, uint32_t amount) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_TRAINING_);
	packet->write_seq_id();
	packet->write_u8(kind);       // RD_Kind troop type (infantry, ranged, cavalry, siege)
    packet->write_u8(tier);   // RD_Rank (server expects 0-based) (t1, t2, t3, t4, t5)
    packet->write_u32(amount);             // TrainingMax
	packet->send(true);
}

void CancelTroopTraining(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_CANCELTRAINING);
	packet->write_seq_id();
	packet->send(true);
}

void SendScout(Packet *packet, PointCode DesPoint) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_SENDSCOUT);
	packet->write_seq_id();
	packet->write_u16(DesPoint.zoneID);
	packet->write_u8(DesPoint.pointID);
	packet->send(true);
}

void SendResPointLv(Packet *packet, PointCode point) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_RESPOINT_OWNER_LV);
	packet->write_seq_id();
	packet->write_u16(point.zoneID);
	packet->write_u8(point.pointID);
	packet->send(true);
}

void Send_Mall_Combobox(Packet *packet, uint16_t SN) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_TREASURE_COMBOBOX);
	packet->write_seq_id();
	packet->write_u16(SN);
	packet->send(true);
}

void Send_Mall_TestBuy(Packet *packet, uint32_t TreasureID) {
	packet->clear();
	packet->write_u16(_MSG_TREASURE_DEBUGBUY);
	packet->write_seq_id();
	packet->write_u32(TreasureID);
	packet->send(true);
}

void SendTreasureInfo(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_TREASURE_INFO);
	packet->write_seq_id();
	packet->write_u8(0);
	packet->send(true);
}

void RequsetBlackMarketData(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUSET_BLACKMARKET_DATA);
	packet->write_seq_id();
	packet->write_u8(1);
	packet->send(true);
}


// #define RANDOM_RELOCATOR 0x03eb 

const uint16_t RandomTeleportItemID = 1003;
const uint16_t AdvanceTeleportItemID = 1004;
const uint16_t Shield24HourItemID = 1052;
const uint16_t Shield8HourItemID = 1051;
const uint16_t Shield4HourItemID = 1146;

const uint16_t Wood150KItemId = 1021;

// #define SHIELD_24H
// #define SHIELD_8H
// #define SHIELD_4H


void SendRelocate(Packet *packet, uint16_t kingdom_id, uint16_t x, uint16_t y) {
	PointCode DesPoint = getPointCodeByMapPos(x, y);
	packet->clear();
	packet->write_u16(_MSG_REQUEST_USEITEM);
	packet->write_seq_id();
	packet->write_u16(AdvanceTeleportItemID);
	packet->write_u16(1); // Quantity 
	packet->write_u16(kingdom_id);
	packet->write_u16(DesPoint.zoneID);
	packet->write_u8(DesPoint.pointID);
	// nothing 5 byte zero 
	packet->write_u32(0);
	packet->write_u8(0);
	packet->send(true);
}

void SendRandomRelocate(Packet *packet, uint16_t kingdom_id) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_USEITEM);
	packet->write_seq_id();
	packet->write_u16(RandomTeleportItemID);
	packet->write_u16(1); // Quantity 
	packet->write_u16(kingdom_id);
	// add 8 byte zero
	packet->write_u64(0);
	packet->send(true);
}

void RandomRelocateChaosArena(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_USEITEM);
	packet->write_seq_id();
	packet->write_u16(RandomTeleportItemID);
	packet->write_u16(1); // Quantity 
	packet->write_u16(86);
	
	packet->write_u8(255);
	packet->write_u8(255);
	packet->write_u8(255);
	
	// add 5 byte zero
	packet->write_u32(0);
	packet->write_u8(0);
	packet->send(true);
}


void RelocateChaosArena(Packet *packet, uint16_t kingdom_id, uint16_t x, uint16_t y) {
	PointCode DesPoint = getPointCodeByMapPos(x, y);
	packet->clear();
	packet->write_u16(_MSG_REQUEST_USEITEM);
	packet->write_seq_id();
	packet->write_u16(AdvanceTeleportItemID);
	packet->write_u16(1); // Quantity 
	packet->write_u16(86);
	packet->write_u16(DesPoint.zoneID);
	packet->write_u8(DesPoint.pointID);
	// nothing 5 byte zero 
	packet->write_u32(0);
	packet->write_u8(0);
	packet->send(true);
}


void UseItem(Packet *packet, uint32_t item_id, uint16_t quantity) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_USEITEM);
	packet->write_seq_id();
	packet->write_u16(item_id);
	packet->write_u16(quantity);
	
	// 10 byte zero
	for (uint8_t i = 0; i < 5; i++) {
		packet->write_u16(0);
	}
	
	packet->send(true);
}


void ShieldInfo(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_SHIELD_LOG_LIST);
	packet->write_seq_id();
	packet->send(true);
}


void SendMail(Packet *packet, const char *player_name, const char *subject, const char *message) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_SENDREGMAIL);
	packet->write_seq_id();
	packet->write_u32(0);
	packet->write_bytes(player_name, 13);
	packet->write_u8(1);
	
	packet->write_u8(strlen(subject));
	packet->write_u16(strlen(message));
	
	packet->write_u8(0);
	
	packet->write_bytes(subject, strlen(subject));
	
	packet->write_bytes(message, strlen(message));
	
	for (uint8_t i = 0; i < 10; i++) {
		packet->write_u8(0);
	}
	
	packet->send(true);
}

void SendMailFmt(Packet *packet, const char *player_name, const char *subject, const char *fmt, ...) {
    char buf[4096];  // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    SendMail(packet, player_name, subject, buf);
}

void SendAllMail(Packet *packet, const char *subject, const char *message) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_SENDALLIMAIL);
	packet->write_seq_id();
	packet->write_u8(1);
	packet->write_u8(strlen(subject));
	packet->write_u16(strlen(message));
	packet->write_u8(0);
	packet->write_bytes(subject, strlen(subject));
	packet->write_bytes(message, strlen(message));
	for (uint8_t i = 0; i < 10; i++) {
		packet->write_u8(0);
	}
	packet->send(true);
}

void SendAllMailFmt(Packet *packet, const char *subject, const char *fmt, ...) {
    char buf[4096];  // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    SendAllMail(packet, subject, buf);
}


void MonsterHunt(Packet *packet, uint8_t count, uint16_t zoneId, uint8_t pointId) {
	packet->reset();
	packet->write_u16(_MSG_REQUEST_SENDREGMAIL);
	packet->write_seq_id();
	// packet->write_u64(igg_id);
	
	packet->send(true);
}


void GambleStartGame(Packet *packet, uint8_t type) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_GAMBLE_STARTGAME);
	packet->write_seq_id();
	packet->write_u8(type);
	packet->send(true);
}


void SendGamblePrize(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_GAMBLE_PRIZE);
	packet->write_seq_id();
	packet->send(true);
}


void SendGambleInfo(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_GAMBLE_INFO);
	packet->write_seq_id();
	packet->send(true);
}

void sendStartBuilding(Packet *packet, uint16_t MonorID, uint16_t BuildID) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_BUILDBEGIN);
	packet->write_seq_id();
	packet->write_u16(0x0e02);
	packet->write_u16(4);
	packet->write_u8(2);
	// uint8_t data[5] = {0x02, 0x0e, 0x04, 0x00, 0x02};
	// packet->write_bytes(data, 5);
	/*
	packet->write_u16(0x0e02); // 
	packet->write_u16(0x04);
	packet->write_u8(0x02);*/
	/*
	packet->write_u16(MonorID);
	packet->write_u16(BuildID);*/
	packet->send(true);
}


/*
void SendUseItem(Packet *packet, uint16_t ItemID, uint16_t Quantity, uint16_t Target = 0, uint16_t Parameter1 = 0, uint16_t Parameter2 = 0, uint32_t Parameter3 = 0U, const char *Name) {
	packet->reset();
	packet->write_u16(0x0000);
	packet->write_u16(_MSG_REQUEST_USEITEM);
	packet->write_seq_id();
	
	
	
	packet->write_u8(1);
	packet->send(true);
	
}*/

/*

void ServerUseSimpleItem(uint16_t item, uint16_t amount) {
	packet.write_u16(0x057E)?;
                packet.write_seq_id(connection)?;
                packet.write_u16(*item)?;
                packet.write_u16(*amount)?;
                packet.write_all(&[0; 10])?;
            }
*/


uint64_t parse_number_u64(const char *str) {
    double value = 0.0;
    char suffix = '\0';

    // Read numeric part and optional suffix
    sscanf(str, "%lf%c", &value, &suffix);
    suffix = tolower(suffix); // handle both lowercase and uppercase

    // Apply multiplier
    switch (suffix) {
        case 'k': value *= 1000ULL; break;
        case 'm': value *= 1000000ULL; break;
        case 'b': value *= 1000000000ULL; break;
        default: break;//return 0;//break; // no suffix
    }

    if (value < 0) value = 0;

    return (uint64_t)value;
}


void replace_underscore_with_space(char *str) {
    while (*str) {
        if (*str == '_') {
            *str = ' ';
        }
        str++;
    }
}

const char* format_number(long num, int precision) {
    static char out[20];  // Static so it survives after return
    if (num >= 1000000000)
        snprintf(out, sizeof(out), "%.*fB", precision, num / 1000000000.0);
    else if (num >= 1000000)
        snprintf(out, sizeof(out), "%.*fM", precision, num / 1000000.0);
    else if (num >= 1000)
        snprintf(out, sizeof(out), "%.*fK", precision, num / 1000.0);
    else
        snprintf(out, sizeof(out), "%ld", num);
    return out;
}

typedef enum {
    RESOURCE_NONE = 0,
    RESOURCE_FOOD,
    RESOURCE_WOOD,
    RESOURCE_ROCK,
    RESOURCE_ORE,
    RESOURCE_GOLD
} ResourceType;

const char *resource_to_string(ResourceType res) {
    switch (res) {
        case RESOURCE_NONE: return "none";
        case RESOURCE_FOOD: return "food";
        case RESOURCE_WOOD: return "wood";
        case RESOURCE_ROCK: return "rock";
        case RESOURCE_ORE:  return "ore";
        case RESOURCE_GOLD: return "gold";
        default: return "unknown";
    }
}

void GetResourceType(ResourceType res, char *buf) {
	switch (res) {
		case RESOURCE_NONE:
			strcpy(buf, "none");
			break;
		case RESOURCE_FOOD:
			strcpy(buf, "food");
			break;
		case RESOURCE_WOOD: 
			strcpy(buf, "wood");
			break;
		case RESOURCE_ROCK: 
			strcpy(buf, "rock");
			break;
		case RESOURCE_ORE:
			strcpy(buf, "ore");
			break;
		case RESOURCE_GOLD: 
			strcpy(buf, "gold");
			break;
		default: 
			strcpy(buf, "unknown");
			break;
	}
}

// Convert troop type string → numeric ID
uint8_t TroopTypeFromString(const char *str) {
    if (strcasecmp(str, "infantry") == 0) return 0;
    if (strcasecmp(str, "ranged")   == 0) return 1;
    if (strcasecmp(str, "cavalry")  == 0) return 2;
    if (strcasecmp(str, "siege")    == 0) return 3;
    return 0xFF; // invalid
}

// Convert numeric ID → troop type string
const char *TroopTypeToString(uint8_t type) {
    switch (type) {
        case 0: return "Infantry";
        case 1: return "Ranged";
        case 2: return "Cavalry";
        case 3: return "Siege";
        default: return "Unknown";
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


ResHelp make_resource(ResourceType rtype, uint64_t amount) {
    ResHelp res;
    memset(&res, 0, sizeof(res));

    switch (rtype) {
        case RESOURCE_FOOD:  res.food  = (amount > UINT32_MAX) ? UINT32_MAX : (uint32_t)amount; break;
        case RESOURCE_ROCK:  res.rock  = (amount > UINT32_MAX) ? UINT32_MAX : (uint32_t)amount; break;
        case RESOURCE_WOOD:  res.wood  = (amount > UINT32_MAX) ? UINT32_MAX : (uint32_t)amount; break;
        case RESOURCE_ORE:   res.ore   = (amount > UINT32_MAX) ? UINT32_MAX : (uint32_t)amount; break;
        case RESOURCE_GOLD:  res.gold  = (amount > UINT32_MAX) ? UINT32_MAX : (uint32_t)amount; break;
        default: /* unknown type, all 0 */ break;
    }

    return res;
}


uint32_t tid = 0;

// Resources bank_resource = {0};

ResourceType get_resource_type(const char *command) {
    if (strcasecmp(command, "food") == 0) return RESOURCE_FOOD;
    if (strcasecmp(command, "wood") == 0) return RESOURCE_WOOD;
    if (strcasecmp(command, "stone") == 0) return RESOURCE_ROCK;
    if (strcasecmp(command, "ore")  == 0) return RESOURCE_ORE;
    if (strcasecmp(command, "gold") == 0) return RESOURCE_GOLD;
    return RESOURCE_NONE;
}

#define TAX_VALUE 12

// Calculate tax with rounding (12%)
static inline uint64_t calculate_tax(uint64_t amount) {
    // Multiply first, then add 50 for rounding (12% of amount)
    return (amount * TAX_VALUE + 50) / 100;
}

typedef struct {
    char player_name[13]; // player name who commanded
    char target_name[13]; // target player name optional
    uint64_t total_amount; // Player requested amount (net)
    uint64_t remains_amount; // Remaining amount yet to be sent
    uint32_t tax_amount; // Tax amount (calculated as % of player request amount)
    uint64_t gross_amount; // amount + tax (what bank will send) 
    ResourceType resource_type;
    uint16_t zoneId;
    uint8_t pointId;
	bool is_locked;
	bool send_self;
} ResourceRequest;


ResourceRequest send_resources(Packet *packet, const char *player_name, const char *message) {
	char command[16] = {0};
	char amount_str[32] = {0};
	char player[14] = {0};
	ResourceRequest req;
	memset(&req, 0, sizeof(req));
	
	
	if (sscanf(message, "%15s %31s %13[^\n]", command, amount_str, player) != 3) {
		if (sscanf(message, "%15s %31s", command, amount_str) != 2) {
			SendMailFmt(packet, player_name,"", "Invalid command format. Use '!%s <amount>' or '!%s <amount> <player_name>'.", command);
			return req;
		} else {
			req.send_self = true;
		}
	} else {
		strncpy(req.target_name, player, sizeof(player));
		req.send_self = false;
	}
	
	// player name who executed command 
	strcpy(req.player_name, player_name);
	
	printf("player: [%s]\n", req.player_name);
	
	req.total_amount = parse_number_u64(amount_str);
	
	req.tax_amount = (uint64_t)((double)req.total_amount * 0.08 + 0.5);
    // req2.tax_amount = (uint64_t)(req2.total_amount * TAX_RATE);
    
    
	req.resource_type = get_resource_type(command);
	
	if (req.resource_type == RESOURCE_NONE) {
        SendMailFmt(packet, player_name, "", "Error: unknown resource type.");
        memset(&req, 0,  sizeof(req));
        return req;
    }
    
    if (req.total_amount < 5) {
    	SendMailFmt(packet, player_name, "", "Error: invalid amount. Must be greater than 5 and formatted like 100, 1k, 10M, 1B.");
        memset(&req, 0,  sizeof(req));
        return req;
    }
    
    req.gross_amount = req.total_amount;// + req.tax_amount;
	
	req.remains_amount = req.total_amount;// + req.tax_amount;
	
    return req;
}

#define MAX_ITEM_COUNT 65536

typedef struct {
	uint16_t quantity;
} Item;

Item items[MAX_ITEM_COUNT];

void change_location(Packet *packet, const char *player_name, char *message) {
	if (current_kingdom == 0) {
        SendMailFmt(packet, player_name, "",
            "Error: current kingdom not set! Use !kd <kingdom_id> first."
        );
        return;
    }
    
    /*
    if (items[1004].quantity == 0) {
    	SendMailFmt(packet, player_name, "", "You don't have advance relocator!");
        return;
    }*/
    
	int k = 0, x = 0, y = 0;
	
	size_t len = strlen(message);
	
	for (size_t i = 0; i < len; i++) {
		message[i] = toupper((unsigned char)message[i]);
	}
	
	if (sscanf(message, "K:%d X:%d Y:%d", &k, &x, &y) != 3) {
		if (sscanf(message, "%d:%d:%d", &k, &x, &y) != 3) {
			if (sscanf(message, "X:%d Y:%d", &x, &y) != 2) {
				if (sscanf(message, "%d:%d", &x, &y) != 2) {
					SendMailFmt(packet, player_name, "", "Expected formats:\n"
                        "K:<kingdom> X:<x> Y:<y>\n"
                        "<kingdom>:<x>:<y>\n"
                        "X:<x> Y:<y>\n"
                        "<x>:<y>");
					return;
				}
			}
		}
	}
	
	if (k == 0) {
		k = current_kingdom;
	}
	
	SendRelocate(packet, k, x, y);
}

void change_location_rand(Packet *packet, const char *player_name) {
	if (current_kingdom == 0) {
        SendMailFmt(packet, player_name, "",
            "Error: current kingdom not set! Use !kd <kingdom_id> first."
        );
        return;
    }
    
    /*
    some bug not stored;
    if (items[1003].quantity == 0) {
    	SendMailFmt(packet, player_name, "", "You don't have random relocator!");
        return;
    }*/
    
    SendRandomRelocate(packet, current_kingdom);
}


void handle_kd_command(Packet *packet, const char *player_name, const char *message) {
    int k = 0;
    
    // Parse the command: "!kd 1745"
    if (sscanf(message, "%d", &k) != 1) {
        SendMailFmt(packet, player_name, "", "!kd <kingdom_id>");
        return;
    }

    current_kingdom = k; // save in memory
    SendMailFmt(packet, player_name, "", "Current kingdom set to %d", current_kingdom);
}


void use_resource(Packet *packet, const char *player_name, char *message) {
	char type[16] = {0}; // resource string (food/wood/ore/rock/gold)
	char size_str[16];  // pack size string (like 150k, 10k, 1m)
	uint32_t count;     // number of packs
	
	if (sscanf(message, "!use %15s %15s %u", type, size_str, &count) != 3) {
		SendMailFmt(packet, player_name, "", "Usage: !use <food|rock|wood|ore|gold> <pack_size> <count>");
		return;
	}
	
	// We continue here if sscan success 
	if (strcasecmp(type, "food") == 0) {
		
	} else if (strcasecmp(type, "stone") == 0) {
		
	} else if (strcasecmp(type, "wood") == 0) {
		
	} else if (strcasecmp(type, "ore") == 0) {
		
	} else if (strcasecmp(type, "gold") == 0) {
		
	}
	
	
}
/*
MessagePacket messagePacket = new MessagePacket(1024);
		messagePacket.Protocol = Protocol._MSG_REQUEST_GIFT;
		messagePacket.AddSeqId();
		messagePacket.Add(Type);
		messagePacket.Add(Key);
		messagePacket.Add(ItemID);
		messagePacket.Add(Name.ToString(), 13);
		messagePacket.Add(Qty);
		messagePacket.Send(false);
*/
void buy(Packet *packet) {
	char name[13] = {0};
	memcpy(name, "PORT COLD", 9);
	
	packet->clear();
	packet->write_u16(_MSG_REQUEST_GIFT);
	packet->write_seq_id();
	
	packet->write_u8(1); // Type  
	packet->write_u16(0x5d); // Key
	packet->write_u16(0x41C); // Item Id
	packet->write_bytes(name, 13);
	packet->write_u16(2); // Quantity 
	packet->send(true);
}


void SendSellItem(Packet *packet, uint16_t ItemID, uint16_t Quantity) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_SELLITEM);
	packet->write_seq_id();
	packet->write_u8(1); // Type  
	packet->write_u16(ItemID); // 
	packet->write_u16(Quantity); // 
	packet->send(true);
}

void RecvSellItem(Packet *p) {
	uint8_t b = p->read_u8();
	uint8_t b2 = b;
	
	if (b2 != 0) {
		if (b2 == 1) {
			printf("Error message!\n");
			return;
			// GUIManager.Instance.AddHUDMessage(this.mStringTable.GetStringByID(83U), 255, true);
		}
	} else {
		uint32_t gold = p->read_u32();
		uint8_t b3 = p->read_u8();
		printf("gold: %u\n", gold);
		printf("b3: %u\n", b3);
		
		for (int i = 0; i < (int)b3; i++) {
			uint16_t num = p->read_u16();
			uint16_t num2 = p->read_u16();
			
			printf("num: 0x%04X (%u)\n", num, num);
			printf("num2: 0x%04X (%u)\n", num2, num2);
			
			
		}
		// GUIManager.Instance.HideUILock(EUILock.SellItem);
	}
}



/*
6f 00 // Packet Size
6f 09 // Packet type 
0e 00 00 00 // Sequence id 
// max 5 hero in march
12 00 // hero id
01 00 // hero id
1d 00 // hero id
04 00 // hero id
11 00 // hero id

// troop infantry 
00 00 00 00 // t1
00 00 00 00 // t2
60 d8 03 00 // t3
00 00 00 00 // t4

// troop cavalry 
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00

// troop ranged
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00

// troop siege 
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00


07 03 // ZoneId 
9a // PointId

// i don't know about this data
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 

packet 2
6f 00
6f 09
12 00 00 00

06 00
05 00
03 00
0d 00
15 00 

00 00 00 00
00 00 00 00
01 a7 02 00
00 00 00 00

00 00 00 00
00 00 00 00
7f f8 00 00
00 00 00 00

00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00

00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00

07 03
9a 

00 00
00 00
00 00 
00 00
00 00 

00 00 00 00
00 00 00 00
00 00 00 00 
00 00 00 00 


*/
// Forest K:1745 X:247 Y:777


/*
_MSG_REQUEST_TROOPMARCH_NOTATK

6f 00 Packet size
d7 19 Packet Type 
13 00 00 00 Sequence id 

// Heros
00 00
00 00
00 00
00 00
00 00

// Troop Infantry 
01 00 00 00 t1
00 00 00 00
00 00 00 00 
00 00 00 00

00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00

00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00

00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00

07 03
9b

12 00
08 00
00 00 
00 00 
00 00 

00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00 
*/

typedef struct {
    // ---- Heroes ----
    uint16_t heroes[5];     // 5 heroes (2 bytes each)

    // ---- Troops T1–T4 ----
    // [Infantry, Cavalry, Ranged, Siege] × [T1..T5]
    uint32_t troops[4][5];  // 4 troop types × 4 tiers

    // ---- Target ----
    uint16_t zone_id;       // Zone ID
    uint8_t  point_id;      // Point ID

    // ---- Familiars ----
    uint16_t familiars[5];  // 5 familiars (2 bytes each)
} __attribute__((packed))  MarchData;

void RequestMarchTroop(Packet *packet, MarchData data) {
	packet->clear();
	
	//if (attack) {
		packet->write_u16(_MSG_REQUEST_TROOPMARCH);
	//} else {
	//	packet->write_u16(_MSG_REQUEST_TROOPMARCH_NOTATK);
	//}
	
	packet->write_seq_id();
	
	// Heros
	packet->write_u16(data.heroes[0]);
	packet->write_u16(data.heroes[1]);
	packet->write_u16(data.heroes[2]);
	packet->write_u16(data.heroes[3]);
	packet->write_u16(data.heroes[4]);
	
	// Troop Infantry 
	packet->write_u32(data.troops[0][0]); // T1 Infantry 
	packet->write_u32(data.troops[0][1]); // T2 Infantry 
	packet->write_u32(data.troops[0][2]); // T3 Infantry 
	packet->write_u32(data.troops[0][3]); // T4 Infantry 
	
	// Troop Ranged
	packet->write_u32(data.troops[2][0]); // T1 Range
	packet->write_u32(data.troops[2][1]); // T2 Range
	packet->write_u32(data.troops[2][2]); // T3 Range
	packet->write_u32(data.troops[2][3]); // T4 Range
	
	
	
	// Troop Cavalry
	packet->write_u32(data.troops[1][0]); // T1 Cavalry 
	packet->write_u32(data.troops[1][1]); // T2 Cavalry 
	packet->write_u32(data.troops[1][2]); // T3 Cavalry 
	packet->write_u32(data.troops[1][3]); // T4 Cavalry 
	
	/* Troop Ranged
	packet->write_u32(data.troops[2][0]); // T1 Range
	packet->write_u32(data.troops[2][1]); // T2 Range
	packet->write_u32(data.troops[2][2]); // T3 Range
	packet->write_u32(data.troops[2][3]); // T4 Range
	*/
	
	// Troop Siege
	packet->write_u32(data.troops[3][0]); // T1 Siege
	packet->write_u32(data.troops[3][1]); // T2 Siege
	packet->write_u32(data.troops[3][2]); // T3 Siege
	packet->write_u32(data.troops[3][3]); // T4 Siege
	
	// PointCode
	packet->write_u16(data.zone_id);
	packet->write_u8(data.point_id);
	
	// Familiar 
	packet->write_u16(data.familiars[0]);
	packet->write_u16(data.familiars[1]);
	packet->write_u16(data.familiars[2]);
	packet->write_u16(data.familiars[3]);
	packet->write_u16(data.familiars[4]);
	
	// T5 Troops 
	packet->write_u32(data.troops[0][4]); // T5 Infantry 
	packet->write_u32(data.troops[1][4]); // T5 Cavalry 
	packet->write_u32(data.troops[2][4]); // T5 Range
	packet->write_u32(data.troops[3][4]); // T5 Siege
	
	packet->send(true);
	
	//dump_data("atk", "", packet->write_buffer, packet->write_size);
	
	//exit(0);
}

void TroopeTakeBack(Packet *packet, uint32_t i) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_TROOPRETURN);
	packet->write_u32(i);
	packet->send(true);
}

// Forest K:1745 X:247 Y:777
void send_troop(Packet *packet, const char *player_name, char *message) {
	int k = 0, x = 0, y = 0;
	
	size_t len = strlen(message);
	
	for (size_t i = 0; i < len; i++) {
		message[i] = toupper((unsigned char)message[i]);
	}
	
	if (sscanf(message, "X:%d Y:%d", &x, &y) != 2) {
		if (sscanf(message, "%d:%d", &x, &y) != 2) {
			SendMailFmt(packet, player_name, "", "Expected formats:\nX:<num> Y:<num>\n<num>:<num>");
			return;
		}
	}
	
	PointCode DesPoint = getPointCodeByMapPos(x, y);
	
	printf("zoneId: %u\n", DesPoint.zoneID);
	printf("pointId: %u\n", DesPoint.pointID);
	
	MarchData data;
	memset(&data, 0, sizeof(data));
	
	data.troops[1][0] = 1; // T1 Cavalry 
	
	data.zone_id = DesPoint.zoneID;
    data.point_id = DesPoint.pointID;
    
	RequestMarchTroop(packet, data);
	return;
}

void send_ret_troop(Packet *packet, const char *player_name, char *message) {
	int army_num = 0;
	
	if (sscanf(message, "%d", &army_num) != 1) {
		SendMailFmt(packet, player_name, "",  "Require <army_num>");
		return;
	}
	
	if (army_num <= 0 || army_num >= 8) {
		SendMailFmt(packet, player_name, "",  "Invalid!");
		return;
	}
	
	TroopeTakeBack(packet, army_num);
}


#define ERR_NOT_AUTH0 "You are not authorized to use this command."
#define ERR_NOT_AUTH1 "Arre chutiy@! Ee command tere bas ka nahi hai"
#define ERR_NOT_AUTH2 "Oye sala! Iska try karna band kar"
#define ERR_NOT_AUTH3 "Bhai, ee command tere liye forbidden hai"
#define ERR_NOT_AUTH4 "Oye bakra! Tere liye ye kaam allowed nahi"
#define ERR_NOT_AUTH5 "Arey madarchod! Ye command sirf authorized log ke liye hai"

// At the top of your file
#define ERR_NOT_AUTH ERR_NOT_AUTH0


/*
typedef struct {
	char player_name[13];
	uint32_t food;
	uint32_t stone;
	uint32_t wood;
	uint32_t ore;
	uint32_t gold;
} PlayerBank;

PlayerBank banks[200] = {0};

void init_banks(void) {
    for(int i = 0; i < 200; i++) {
        banks[i].player_name[0] = '\0';
        banks[i].food = 0;
        banks[i].stone = 0;
        banks[i].wood = 0;
        banks[i].ore = 0;
        banks[i].gold = 0;
    }
}

void save_bank(const char *filename, PlayerBank *banks) {
	int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	
	if (file == -1) {
		fprintf("Can't save bank details!\n");
		return;
	}
	
	for (uint32_t i = 0; i < 200; i++) {
		write(file, banks[i].player_name, 13);
		write(file, &banks[i].food, 4);
		write(file, &banks[i].stone, 4);
		write(file, &banks[i].wood, 4);
		write(file, &banks[i].ore, 4);
		write(file, &banks[i].gold, 4);
	}
	
	close(file);
}

void load_bank();

void update_player_resource(const char *player_name, ResourceType type, int32_t amount) {
	for (uint32_t i = 0; i < 200; i++) {
		if (strcmp(banks[i].player_name, player_name) == 0) {
			
			
		}
	}
}
*/
/*
aa 02 0f 01 
170.2.271
*/
void login_v1(Packet *packet, uint64_t igg_id, const char *access_key) {
	uint8_t session[128] = {0};
	uint8_t session_len = strlen(access_key);
	memcpy(session, access_key, session_len);
	
	packet->clear();
	packet->write_u16(_MSG_LOGIN_LOGINTOL);
	packet->write_u64(igg_id);
	
	// 168.2.269
	packet->write_u8(170); // Version[0]
	
	
	packet->write_u8(2); // Version[1]
	
	packet->write_u16(271); // Version[2]
	
	packet->write_u8(session_len);
	packet->write_bytes(session, 128);
	packet->write_u8(1);
	packet->write_u8(0);
	packet->send(false);
	
	printf("session len: %u\n", session_len);
}


void login_v2(Packet *packet, uint64_t igg_id, const char *access_key) {
	packet->clear();
	packet->write_u16(_MSG_GUESTLOGIN_REQUESTIPTOP);
	packet->write_u64(igg_id);
	packet->write_bytes(access_key, strlen(access_key) + 1);
	packet->send(false);
	
}

/*
MessagePacket messagePacket = new MessagePacket(1024);
			messagePacket.Protocol = Protocol._MSG_GUESTLOGIN_REQUESTIPTOP;
			messagePacket.AddSeqId();
			messagePacket.Add(Id);
			messagePacket.Add(NetworkManager.UDID, NetworkManager.UDID.Length);
			*/

/*
MessagePacket messagePacket = new MessagePacket(1024);
			messagePacket.Protocol = Protocol._MSG_LOGIN_LOGINTOL;
			messagePacket.Add(NetworkManager.UserID);
			messagePacket.Add((byte)GameConstants.Version[1]);
			messagePacket.Add((byte)GameConstants.Version[0]);
			messagePacket.Add(GameConstants.Version[2]);
			messagePacket.Add((byte)NetworkManager.UDID.Length);
			messagePacket.Add(NetworkManager.UDID, NetworkManager.SessionKey.Length);
			messagePacket.Add(1);
			messagePacket.Add((byte)DataManager.Instance.UserLanguage);
			messagePacket.Send(true);

*/


#define MAX_BUFFER 4096

uint8_t recv_buffer[MAX_BUFFER];
size_t recv_len = 0;

void handle_packet(uint8_t *data, size_t size) {
	uint16_t packet_type = 0;
	
	printf("PACKET TYPE: %s\n", get_packet_name(packet_type));
	
	
}

const char* format_number2(long num, int precision) {
    static char out[20];  // Static so it survives after return
    if (num >= 1000000000)
        snprintf(out, sizeof(out), "%.*fB", precision, num / 1000000000.0);
    else if (num >= 1000000)
        snprintf(out, sizeof(out), "%.*fM", precision, num / 1000000.0);
    else if (num >= 1000)
        snprintf(out, sizeof(out), "%.*fK", precision, num / 1000.0);
    else
        snprintf(out, sizeof(out), "%ld", num);
    return out;
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

typedef enum {
	EMS_Null,
	EMS_Begin,
	EMS_End,
	EMS_BeginAndEnd
} eMsgState;



void SendSomeBody(Packet *packet) {
	packet->clear();
	packet->write_u16(0x0B24);
	packet->write_seq_id();
	packet->write_u8(1);
	packet->send(true);
}

void sendBuildingCancel(Packet *packet, uint8_t b) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_BUILDCANCEL);
	packet->write_seq_id();
	packet->write_u8(b);
	packet->send(true);
}

// Cargo trade 
void GetBlackMarketData(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUSET_BLACKMARKET_DATA);
	packet->write_seq_id();
	packet->write_u8(1);
	packet->send(true);
}

void SendBlackMarketBuy(Packet *packet, uint8_t mIdx) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_BLACKMARKET_BUY);
	packet->write_seq_id();
	packet->write_u8(mIdx);
	packet->send(true);
}


typedef struct {
    uint16_t id;
    uint32_t pack_amount; // amount per pack
} ResourcePack;




#define RANDOM_RELOCATOR 1003
#define ADVANCE_RELOCATOR 1004
#define SHIELD_24H 1052
#define SHIELD_8H 1051
#define SHIELD_4H 1146

#define FAMILIAR_MAGMA_LORD  0x0012
#define WOOD_150K 1021


typedef struct {
	char player_name[13];
	char message[65535];
} ChatMessage;



ChatMessage RecvChatMessage(Packet *packet) {
	ChatMessage res;
	memset(&res, 0, sizeof(res));
	
	char player_name[13] = {0};
	char title_name[3] = {0};
	char str1[20] = {0};
	char str2[20] = {0};
	char message[65536] = {0};
	
	uint8_t b2 = packet->read_u8();
	
	if (ServerVersionMajor != 0) {
		uint8_t num3 = packet->read_u8();
	}
	
	if (b2 == 0 || b2 == 1) {
		uint16_t num4 = packet->read_u16();
		// printf("num4: %u\n", num4);
		
		for (uint16_t i = 0; i < num4; i++) {
			int64_t num5 = packet->read_u64();
			int64_t num6 = packet->read_u64();
			int64_t num7 = packet->read_u64();
			uint8_t alli_or_king = packet->read_u8();
			uint8_t num8 = packet->read_u8();
			uint16_t pic_id = packet->read_u16();
			packet->read_bytes(res.player_name, 13);
			uint8_t vip_rank = packet->read_u8();
			packet->read_bytes(title_name, 3);
			uint8_t special_block_id = packet->read_u8();
			uint8_t title_id = packet->read_u8();
			uint8_t b_have_arabic = packet->read_u8();
			uint16_t num9 = packet->read_u16();
					
			// printf("playID: %lu\n", num6);
			// printf("player_name: %s\n", player_name);
			// printf("vip_rank: %u\n", vip_rank);
			
			if (num8 == 108) {
				int message_talk_kind = 3;
				uint16_t message_kingdom_id = packet->read_u16();
				packet->read_bytes(str1, 3);
				packet->read_bytes(str2, 13);
				
				// printf("[%s] %s\n", str1, str2);
			} else if (num8 == 109) {
				int message_talk_kind = 0;
				uint16_t message_emoji_key = packet->read_u16();
				uint16_t message_num10 = packet->read_u16();
						
				// printf("message_emoji_key: %u\n", message_emoji_key);
				// printf("message_num10: %u\n", message_num10);
			} else if (num8 == 0) {
				packet->read_bytes(res.message, num9);
				// memcpy(res.player_name, player_name, 13);
				//p.read_bytes(message, num9);
			}
		}
	}
	
	return res;
}

ResourcePack food_packs[] = {
    {0x03F1, 30000},    // Food 30K
    {0x03F6, 150000},   // Food 150K
    {0x03FB, 500000},   // Food 500K
    {0x0400, 2000000},  // Food 2M
    {0x0445, 6000000},  // Food 6M
    {0x044A, 20000000}, // Food 20M
    {0x044F, 60000000}, // Food 60M
};

uint64_t GetBagFood(void) {
	return 
		((uint64_t)items[FOOD_5K].quantity * 5000) + // FOOD 5K
		((uint64_t)items[FOOD_30K].quantity * 30000) + // FOOD 30K
		((uint64_t)items[FOOD_150K].quantity * 150000) + // FOOD 150K
		((uint64_t)items[FOOD_500K].quantity * 500000) + // FOOD 500K
		((uint64_t)items[FOOD_2M].quantity * 2000000) + // FOOD 2M
		((uint64_t)items[FOOD_6M].quantity * 6000000) + // FOOD 6M
		((uint64_t)items[FOOD_20M].quantity * 20000000) + // FOOD 20M
		((uint64_t)items[FOOD_60M].quantity * 60000000); // FOOD 60M
}


uint64_t GetBagRock(void) {
	return 
		((uint64_t)items[STONE_3K].quantity   * 3000) + // STONE 3K
		((uint64_t)items[STONE_10K].quantity  * 10000) + // STONE 10K
		((uint64_t)items[STONE_50K].quantity  * 50000) + // STONE 50K
		((uint64_t)items[STONE_150K].quantity * 150000) + // STONE 150K
		((uint64_t)items[STONE_500K].quantity * 500000) + // STONE 500K
		((uint64_t)items[STONE_1_5M].quantity * 1500000) + // STONE 1.5M
		((uint64_t)items[STONE_5M].quantity   * 5000000) + // STONE 5M
		((uint64_t)items[STONE_15M].quantity  * 15000000); // STONE 15M
}


uint64_t GetBagWood(void) {
	return 
		((uint64_t)items[TIMBER_3K].quantity * 3000) + // WOOD 3K
		((uint64_t)items[TIMBER_10K].quantity * 10000) + // WOOD 10K
		((uint64_t)items[TIMBER_50K].quantity * 50000) + // WOOD 50K
		((uint64_t)items[TIMBER_150K].quantity * 150000) + // WOOD 150K
		((uint64_t)items[TIMBER_500K].quantity * 500000) + // WOOD 500K
		((uint64_t)items[TIMBER_1_5M].quantity * 1500000) + // WOOD 1.5M
		((uint64_t)items[TIMBER_5M].quantity * 5000000) + // WOOD 5M
		((uint64_t)items[TIMBER_15M].quantity * 15000000); // WOOD 15M
}

uint64_t GetBagOre(void) {
	return 
		((uint64_t)items[ORE_3K].quantity * 3000) + // ORE 3K
		((uint64_t)items[ORE_10K].quantity * 10000) + // ORE 10K
		((uint64_t)items[ORE_50K].quantity * 50000) + // ORE 50K
		((uint64_t)items[ORE_150K].quantity * 150000) + // ORE 150K
		((uint64_t)items[ORE_500K].quantity * 500000) + // ORE 500K
		((uint64_t)items[ORE_1_5M].quantity * 1500000) + // ORE 1.5M
		((uint64_t)items[ORE_5M].quantity * 5000000) + // ORE 5M
		((uint64_t)items[ORE_15M].quantity * 15000000); // ORE 15M
}

uint64_t GetBagGold(void) {
	return 
		((uint64_t)items[GOLD_3K].quantity   * 3000) + // FOOD 5K
		((uint64_t)items[GOLD_15K].quantity  * 15000) + // FOOD 30K
		((uint64_t)items[GOLD_50K].quantity  * 50000) + // FOOD 150K
		((uint64_t)items[GOLD_200K].quantity * 200000) + // FOOD 500K
		((uint64_t)items[GOLD_600K].quantity * 600000) + // FOOD 2M
		((uint64_t)items[GOLD_2M].quantity   * 2000000) + // FOOD 6M
		((uint64_t)items[GOLD_6M].quantity   * 6000000); // FOOD 20M
}

uint64_t GetBagResource(ResourceType type) {
	return 0;
}

// Bank Resource!
Resources bank_resource;
ResourceStr bank_resource_str;
ResourceStr bank_bag_resource_str;

ResourceStr bank_total_resource_str;


uint64_t igg_id = 0;
// SATAN 2
uint64_t bank_id = 2037459303;


static uint32_t help_spam_count = 0;
static uint32_t help_speed = 0;

ResourceRequest req = {0};
bool resource_sending_active = false;
bool troops_in_march = false;

static uint8_t MaxMarchEventNum = 0;
static uint8_t CurrentMarch = 0;


static uint64_t server_time = 0;


static char login_name[13];
static uint32_t gems = 0;

static uint16_t bank_zoneId = 0;
static uint8_t bank_pointId = 0;


static bool message_spam = false;

// 0 == world
// 1 == guild 
// 2 == mail
uint8_t message_type = 1;

void SendMessage(Packet *packet, const char *player_name, const char *subject, const char *fmt, ...) {
	char buf[4096];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	
	switch (message_type) {
		case 0: 
			ServerSendChat(packet, WORLD, buf);
			break;
		case 1: 
			ServerSendChat(packet, GUILD, buf);
			break;
		case 2:
			SendMail(packet, player_name, subject, buf);
		default:
			SendMail(packet, player_name, subject, buf);
			return;
	}
}



void ScoutHandle(Packet *packet, const char *player_name, const char *message) {
	uint32_t x = 0, y = 0;
	
	if (sscanf(message, "%u:%u", &x, &y) != 2) {
		if (sscanf(message, "X:%u Y:%u", &x, &y) != 2) {
			x = 0xFFFFFFFF;
			y = 0xFFFFFFFF;
		}
	}
	
	if (x == 0xFFFFFFFF || y == 0xFFFFFFFF) {
		printf("Invalid location\n");
		SendMailFmt(packet, player_name, "", "Invalid scout location!");
	} else {
		PointCode DesPoint = getPointCodeByMapPos(x, y);
		SendScout(packet, DesPoint);
	}
}

void ShowSwapRates(Packet *packet, const char *player_name) {
    SendMailFmt(packet, player_name,
        "Swap Rates",
        "All resource swaps are subject to a 30%% tax.\n"
        "You will always receive 70%% of the amount you swap.\n\n"
        "Examples:\n"
        " - 100M Wood → 70M Food\n"
        " - 200M Food → 140M Ore\n"
        " - 50M Stone → 35M Gold\n\n"
        "Formula: received = input × 0.7"
    );
}

bool enough_resource(ResourceType type, PlayerBank bank, uint64_t amount) {
	switch (type) {
		case RESOURCE_FOOD: return bank.food >= amount;
		case RESOURCE_ROCK: return bank.rock >= amount;
		case RESOURCE_WOOD: return bank.wood >= amount;
		case RESOURCE_ORE: return bank.ore >= amount;
		case RESOURCE_GOLD: return bank.gold >= amount;
		default: return false;  // RESOURCE_NONE
	}
}

uint32_t get_resource_amount(ResourceType type, PlayerBank bank) {
	switch (type) {
		case RESOURCE_FOOD: return bank.food;
		case RESOURCE_ROCK: return bank.rock;
		case RESOURCE_WOOD: return bank.wood;
		case RESOURCE_ORE: return bank.ore;
		case RESOURCE_GOLD: return bank.gold;
		default: return 0;  // RESOURCE_NONE
	}
}

void Swap_Handler(Packet *packet, const char *player_name, const char *message) {
	char input[20];
	char output[20];
	char amount[20];
	uint64_t quantity;
	char temp[20];
	
	// Parse input: e.g. "food wood 100M"
	if (sscanf(message, "%19s %19s %19s", input, output, amount) < 3) {
		SendMailFmt(packet, player_name, "Swap Error", "Usage: !swap <input_resource> <output_resource> <amount>");
		return;
	}
	
	quantity = parse_number_u64(amount);
	
	ResourceType input_type = get_resource_type(input);
	ResourceType output_type = get_resource_type(output);
	
	// Validate resources
	if (input_type == RESOURCE_NONE || output_type == RESOURCE_NONE) {
		SendMailFmt(packet, player_name, "Swap Error", "Unknown resource type.");
		return;
	}
	
	// Prevent swapping same-to-same
	if (input_type == output_type) {
		SendMailFmt(packet, player_name, "Swap Error", "You cannot swap %s to %s (same resource).", input, output);
		return;
	}
	
	// Check minimum amount
	if (quantity < 1000) {
		SendMailFmt(packet, player_name, "Swap Error", "Minimum swap amount is 1k.");
		return;
	}
	
	PlayerBank bnk = get_player_bank(player_name);
	
	format_number2(get_resource_amount(input_type, bnk), temp, 20);
	
	if (enough_resource(input_type, bnk, quantity) == false) {
		SendMailFmt(packet, player_name, "Swap Error",
			"Not enough %s! You have %s, but tried to exchange %s.",
			input, temp, amount
		);
		return;
	}
	
	// Apply 30% tax
	uint64_t received = (quantity * 70) / 100;
	
	char huh[20];
	format_number2(received, huh, 20);
	
	ResHelp decrease_rss = make_resource(input_type, quantity);
	ResHelp increase_rss = make_resource(output_type, received);
	
	update_balance(player_name, decrease_rss, BAL_DEBIT);
	update_balance(player_name, increase_rss, BAL_CREDIT);
	
	SendMailFmt(packet, player_name, "Swap Success", 
		"You swapped %s %s → %s %s (30%% tax applied).",
		amount, input, huh, output
	);
	return;
}


typedef enum {
	FIND_SPECIFIC,
	FIND_TAG_BY,
	FIND_ALL,
} FIND_TYPE;

bool one_time = false;
uint32_t find_kingdom = 0;
char find_player[14] = {0};
int find_player_len = 0;
bool is_finding = false;
bool is_found= false;

bool is_ready = false;

bool get_all_player_by_guild = false;

uint8_t find_type = 0;

uint16_t zone[4];

/*
uint64_t received = calculate_swap(input_amount);
SendMessage(packet, player_name, "Swap Complete",
    "%llu of %s swapped → %llu %s received",
    input_amount, from_resource, received, to_resource);
*/

// default command prefix
#define COMMAND_PREFIX '$'


typedef struct {
	uint16_t quantity;
	uint16_t item_id;
	uint64_t begin;
	uint32_t duration;
} ItemInfo;

ItemInfo itemInfo[255];

#include "command.h"


static bool kingdom_scanning = false;

// Global command
// Command cmd = { .cmd = INVALID, .sub = SUB_INVALID, .offset = 0, .name = "" };


void Deposit(Packet *packet, const char *player_name, const char *message) {
	char type[16];
	char amount[32];
	char name[14];
	
	if (sscanf(message, "%15s %31s %13[^\n]", type, amount, name) != 3) {
		if (sscanf(message, "%15s %31s", type, amount) != 2) {
			SendMailFmt(packet, player_name, "Invalid", "Invalid format. Use '%cdeposit type amount' or '%cdeposit type amount name'", COMMAND_PREFIX, COMMAND_PREFIX);
			return;
		} else {
			strcpy(name, player_name);
		}
	}
	
	uint64_t ramount = parse_number_u64(amount);
	ResourceType rtype = get_resource_type(type);
	
	if (rtype == RESOURCE_NONE) {
		SendMailFmt(packet, player_name, "", "Unknown resource type.");
		return;
	}
	
	if (ramount < 5) {
		SendMailFmt(packet, player_name, "", "invalid amount. Must be greater than 5.");
		return;
	}
	
	ResHelp r = make_resource(rtype, ramount);
	
	if (rtype != RESOURCE_NONE) {
		update_balance(name, r, BAL_CREDIT);
	}
	
	/*
	if (ramount > 100000000) {
		SendAllMailFmt(packet, "Bank Deposits", "Deposited %s %s into %s's account.", amount, type, name);
		return;
	}
	*/
	
	SendMailFmt(packet, player_name, "Success", "Deposited %s %s into %s's account.", amount, type, name);
	return;
}

#define MAX_CASTLE 65535

typedef struct {
	uint8_t kind;
	uint16_t zone;
	uint8_t point;
	char name[13];
	char tag[3];
	uint16_t kingdom;
	uint8_t level;
	uint8_t shield;
	time_t last_update;
} castle;

castle ct[MAX_CASTLE];

bool castle_exist(const char name[13]) {
	if (name[0] == '\0') return false;
	
	for (uint32_t index = 0; index < MAX_CASTLE; index++) {
		if (memcmp(name, ct[index].name, 13) == 0) {
			return true;
		}
	}
	
	return false;
}


void castle_add(uint16_t zone, uint8_t point, const char name[13], const char *tag, uint16_t kingdom, uint8_t level, uint8_t shield) {
    for (uint32_t i = 0; i < MAX_CASTLE; i++) {
        if (ct[i].name[0] == '\0') {
            memcpy(ct[i].name, name, 13);
            memcpy(ct[i].tag, tag, 3);
            ct[i].zone = zone;
            ct[i].point = point;
            ct[i].kingdom = kingdom;
            ct[i].level = level;
            ct[i].shield = shield;
            ct[i].last_update = time(NULL);
            printf("index: %u\n", i);
            return;
        }
    }
}

/*
void castle_add(uint16_t zone, uint8_t point, const char name[13], const char *tag, uint16_t kingdom, uint8_t level) {
	int index = -1;
	
	for (uint32_t i = 0; i < MAX_CASTLE; i++) {
		if (ct[i].name[0] == '\0') {
			index = i;
			break;
		}
	}
	
	if (index == -1) return;
	
	
	memcpy(ct[index].name, name, 13);
	ct[index].zone = zone;
	ct[index].point = point;
	memcpy(ct[index].tag, tag, 3);
	ct[index].kingdom = kingdom;
	ct[index].level = level;
	ct[index].last_update = time(NULL);
}*/

void castle_update(uint16_t zone, uint8_t point, const char name[], const char *tag, uint16_t kingdom, uint8_t level, uint8_t shield) {
	for (uint32_t index = 0; index < MAX_CASTLE; index++) {
		if (memcmp(name, ct[index].name, 13) == 0) {
			ct[index].zone = zone;
			ct[index].point = point;
			ct[index].shield = shield;
			ct[index].last_update = time(NULL);
			return;
		}
	}
}


void castle_clear() {
	/*
    time_t now = time(NULL);

    for (uint32_t i = 0; i < MAX_CASTLE; i++) {
        if (ct[i].last_update == 0)
            continue;

        if ((now - ct[i].last_update) >= 300) {
            memset(&ct[i], 0, sizeof(ct[i]));
        }
    }*/
}

uint32_t castle_count(void)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < MAX_CASTLE; i++) {
        if (ct[i].name[0] != '\0') {
            count++;
        }
    }

    return count;
}

castle find_castle_by_name(const char *name) {
	castle empty = {0};
	
    for (int i = 0; i < MAX_CASTLE; i++) {
    	if (strncasecmp(ct[i].name, name, strlen(name)) == 0) { 
            return ct[i];
        }
    }
    
    return empty;
}

// Command Handler 
void command_handler(Packet *packet, const char *player_name, Command cmd, char *message) {
	if (message[0] == COMMAND_PREFIX) message++;
	
	while (*message == ' ') message++;
	
	uint8_t previlage_level = get_rank(player_name);
	
	
	if (cmd.cmd == CMD_USER_BALANCE) {
		char food[20], rock[20], wood[20], ore[20], gold[20];
		
		if (message[cmd.offset] == ' ') {
			if (previlage_level < RANK_5) {
				SendMailFmt(packet, player_name, "Request denied", "You don't have permission to check other players balances!");
				return;
			}
			
			uint8_t rank = get_rank(message + 4);
			
			if (rank > previlage_level) {
				SendMailFmt(packet, player_name, "Request denied", "You cannot view the balance of a player with higher rank than yours!");
				return;
			}
			
			PlayerBank pb = get_player_bank(message + cmd.offset + 1);
			
			// Format BANK
			format_number2(pb.food, food, 20);
			format_number2(pb.rock, rock, 20);
			format_number2(pb.wood, wood, 20);
			format_number2(pb.ore,  ore,  20);
			format_number2(pb.gold, gold, 20);
			
			SendMailFmt(packet, player_name, "Balance", 
				"Hi, %s balance\n\n\nFood: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s\n",
				pb.player_name, food, rock, wood, ore, gold
			);
			
			return;
		}
		
		
		PlayerBank pb = get_player_bank(player_name);
		
		// Format BANK
		format_number2(pb.food, food, 20);
		format_number2(pb.rock, rock, 20);
		format_number2(pb.wood, wood, 20);
		format_number2(pb.ore,  ore,  20);
		format_number2(pb.gold, gold, 20);
		
		SendMailFmt(packet, player_name, "Balance", 
			"Hi, %s your balance\n\n\nFood: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s\n",
			pb.player_name, food, rock, wood, ore, gold
		);
		
		return;
	}
	
	
	if (cmd.cmd == CMD_SEND_FOOD) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage food", "Usage: '!food <amount>'");
			return;
		}
	} else if (cmd.cmd == CMD_SEND_STONE) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage stone", "Usage: '!stone <amount>'");
			return;
		}
	} else if (cmd.cmd == CMD_SEND_WOOD) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage wood", "Usage: '!wood <amount>'");
			return;
		}
	} else if (cmd.cmd == CMD_SEND_ORE) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage ore", "Usage: '!ore <amount>'");
			return;
		}
	} else if (cmd.cmd == CMD_SEND_GOLD) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage gold", "Usage: '!gold <amount>'");
			return;
		}
	}
	
	if (cmd.cmd == CMD_SEND_FOOD || 
		cmd.cmd == CMD_SEND_STONE ||
		cmd.cmd == CMD_SEND_WOOD ||
		cmd.cmd == CMD_SEND_ORE ||
		cmd.cmd == CMD_SEND_GOLD) {
		
		if (resource_sending_active) {
			SendMailFmt(packet, player_name, "In process",
				"A resource process is already in progress by %s. Please wait or use %cstop to cancel.", 
				req.player_name, 
				COMMAND_PREFIX
			);
			return;
		}
		
		if (CurrentMarch >= MaxMarchEventNum) {
			SendMailFmt(packet, player_name, 
				"March limit reached",
				"All march slots are currently in use. Please try again later."
			);
			return;
		}
		
		/*
		if (CurrentMarch >= MaxMarchEventNum) {
			// resource_sending_active = false;
			// memset(&req, 0, sizeof(req));
			SendMailFmt(packet, player_name, "Try again later", "No troops available right now. Please try again later.");
			return;
		}
		*/
		
		req = send_resources(packet, player_name, message);
		
		// only previlage level 8, 9, 10 allowed to send resources to other players 
		if (req.send_self == false) {
			if (previlage_level < RANK_8) {
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "You don't have permission to send resources to other players!");
				return;
			}
		}
		
		PlayerBank pb = get_player_bank(player_name);
		
		if (req.resource_type == RESOURCE_FOOD) {
			// check player bank has enough food!
			if (req.gross_amount > pb.food) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "You don't have enough food in your bank");
				return;
			}
			
			// check bank has enough food
			if (req.gross_amount > bank_resource.food) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "Sorry, the guild bank doesn’t have enough Food available right now.");
				return;
			}
		} else if (req.resource_type == RESOURCE_ROCK) {
			if (req.gross_amount > pb.rock) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "You don't have enough stone in your bank");
				return;
			}
			
			if (req.gross_amount > bank_resource.rock) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "Sorry, the guild bank doesn’t have enough Stone available right now.");
				return;
			}
		} else if (req.resource_type == RESOURCE_WOOD) {
			if (req.gross_amount > pb.wood) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "You don't have enough wood in your bank");
				return;
			}
			
			if (req.gross_amount > bank_resource.wood) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "Sorry, the guild bank doesn’t have enough wood available right now.");
				return;
			}
		} else if (req.resource_type == RESOURCE_ORE) {
			if (req.gross_amount > pb.ore) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "You don't have enough ore in your bank");
				return;
			}
			
			if (req.gross_amount > bank_resource.ore) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "Sorry, the guild bank doesn’t have enough ore available right now.");
				return;
			}
		} else if (req.resource_type == RESOURCE_GOLD) {
			if (req.gross_amount > pb.gold) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "You don't have enough gold in your bank");
				return;
			}
			
			if (req.gross_amount > bank_resource.gold) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Request denied", "Sorry, the guild bank doesn’t have enough gold available right now.");
				return;
			}
		}
		
		if (req.remains_amount > 0) {
			char name[13];
			
			if (req.send_self) {
				strcpy(name, req.player_name);
			}  else {
				strcpy(name, req.target_name);
			}
			
			SendAllyPoint(packet, name);
			printf("target player: '%s'\n", name);
			return;
		}
		
		return;
	}
	
	/*
	if (memcmp(message, "wc", 2) == 0) {
		if (message[2] == '\0') {
			SendMailFmt(packet, player_name, "", "Use '%cwc message'", COMMAND_PREFIX);
		} else {
			ServerSendChat(packet, WORLD, message + 3);
		}
		return;
	}
	
	if (memcmp(message, "gc", 2) == 0) {
		if (message[2] == '\0') {
			SendMailFmt(packet, player_name, "", "Use '%cgc message'", COMMAND_PREFIX);
		} else {
			ServerSendChat(packet, GUILD, message + 3);
		}
		return;
	}
	
	if (memcmp(message, "f\0", 2) == 0) {
		ServerSendChat(packet, WORLD, "Fuck you");//"Fūçk yôù");
		return;
	}
	
	if (memcmp(message, "s\0", 2) == 0) {
		ServerSendChat(packet, WORLD, "Surprise surprise møtherfūckers king satan is back!");
		return;
	}
	*/
	
	if (cmd.cmd == CMD_WHOAMI) {
		uint8_t lvl = get_rank(player_name);
		const char *nickname = rank_name(lvl);
		const char *title = rank_title(lvl);
		
		SendMailFmt(packet, player_name,
			"Permission", 
			"You are '%s' (Rank %d)",
			title, lvl
		);
		
		return;
	}
	/*
	if (memcmp(message, "whoami", 6) == 0) {
		uint8_t lvl = get_rank(player_name);
		const char *nickname = rank_name(lvl);
		SendMessage(packet, player_name, "Permission", "You are '%s' (level %d)", nickname, lvl);
		return;
	}*/
	
	if (cmd.cmd == CMD_WHOIS) {
		if (message[cmd.offset] == '\0') {
			// User typed just "!whois" with no name
			SendMailFmt(packet, player_name, "Usage", "!whois <admin_name>");
			return;
		}
		
		// while (*message == ' ') message++;
		
		// Check if target exists
		if (!is_authorized(message + cmd.offset + 1)) {
			SendMailFmt(packet, player_name, "Error", "Admin '%s' not found.", message + cmd.offset + 1);
			return;
		}
		
		uint8_t lvl = get_rank(message + cmd.offset + 1);
		const char *nickname = rank_name(lvl);
		const char *title = rank_title(lvl);
		
		SendMailFmt(packet, player_name, "Permission", "He is '%s' (rank %d)", title, lvl);
		return;
	}
	
	// Only authorized player!
	/*
	if (!is_authorized(player_name)) {
		SendMessage(packet, player_name, "Unauthorized Access", "You are not authorized to use this command.");
		return;
	}*/
	
	/*
	if (memcmp(message, "swap", 4) == 0) {
		if (message[4] == '\0') {
			
			return;
		}
		
		Resource_Swap(packet, player_name, message + 5);
		return;
	}
	*/
	
	if (cmd.cmd == CMD_DEPOSIT_RESOURCE) {
		if (previlage_level < RANK_9) {
			SendMailFmt(packet, player_name, "Request denied", "Only rank 9 admins and the Superuser can perform deposits!");
			// SendMessage(packet, player_name, "Request denied", "Sorry, you don’t have permission to use the deposit command.");
			return;
		}
		
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage", "%cdeposit <type> <amount> or %cdeposit <type> <amount> <player_name>", COMMAND_PREFIX, COMMAND_PREFIX);
			return;
		}
		
		Deposit(packet, player_name, message + cmd.offset + 1);
		return;
	}

	
	if (cmd.cmd == CMD_ADD_ADMIN) {
		AdminResult res = add_admin_default(player_name, message + cmd.offset + 1);
		
		if (res == ADMIN_OK) {
			SendMailFmt(packet, player_name, "Admin Added", "Admin '%s' added successfully.", message + cmd.offset + 1);
			return;
		}
		
		SendMailFmt(packet, player_name, "Action", "%s", admin_result_to_msg(res));
		return;
	}
	
	if (cmd.cmd == CMD_LIST_ADMIN) {
		char buff[512];
		get_admin_list(buff);
		SendMailFmt(packet, player_name, "admin list", "%s", buff);
		return;
	}
	
	if (cmd.cmd == CMD_DISMISS_ADMIN) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "dismiss", "!dismiss admin_name");
			return;
		}
		
		AdminResult res = remove_admin(player_name, message + cmd.offset + 1);
		
		if (res == ADMIN_OK) {
			SendMailFmt(packet, player_name, "Success", "Removed admin: %s", message + cmd.offset + 1);
		} else {
			SendMailFmt(packet, player_name, "Action", "%s", admin_result_to_msg(res));
		}
		
		return;
	}
	
	if (cmd.cmd == CMD_RANK_ADMIN) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "dismiss", "!rank <level> <name> ");
			return;
		}
		
		char target_name[13];
		int level = 0;
		
		if (sscanf(message + cmd.offset + 1, "%d %12[^\n]", &level, target_name) != 2) {
			SendMailFmt(packet, player_name, "Usage", "!rank <level> <name>");
			return;
		}
		
		// Validate level range
		if (level < 1 || level > 10) {
			SendMailFmt(packet, player_name, "Error", "Rank must be between 1 and 10.");
			return;
		}
		
		// 2. Check if target admin exists
		if (!is_authorized(target_name)) {
			SendMailFmt(packet, player_name, "Error", "Target admin '%s' not found.", target_name);
			return;
		}
		
		AdminResult res = set_admin_level(player_name, target_name, (AdminRank)level);
		
		if (res == ADMIN_OK) {
			const char *nickname = rank_name(level);
			SendMailFmt(packet, player_name, "Success", "Admin '%s' is now '%s' (level %d).", target_name, nickname, level);
			return;
		}
		
		SendMailFmt(packet, player_name, "Action", "%s", admin_result_to_msg(res));
		return;
	}
	
	/*
	if (memcmp(message, "scout", 5) == 0) {
		if (message[5] == '\0') {
			return;
		}
		
		ScoutHandle(packet, message + 6);
	}
	
	if (memcmp(message, "io", 2) == 0) {
		if (message[2] == '\0') {
			printf("Input Output undefined!\n");
			return;
		}
		
		if (memcmp(message + 3, "-g", 2) == 0) {
			message_type = 1;
			SendMessage(packet, player_name, "IO Info", "All action output will shown in guild chat!");
			printf("Input Output Guild!\n");
			return;
		} 
		
		if (memcmp(message + 3, "-m", 2) == 0) {
			message_type = 2;
			SendMessage(packet, player_name, "IO Info", "All action output will shown in mail!");
			printf("Input Output Mail!\n");
			return;
		}
	}
	
	if (memcmp(message, "spam", 4) == 0) {
		message_spam = true;
	}*/
	
	if (cmd.cmd == CMD_BANK_BALANCE) {
		if (message[cmd.offset] == ' ') {
			if (memcmp(message + cmd.offset + 1, "where", 5) == 0) {
				map_pos_t map = getTileMapPosbyPointCode(bank_zoneId, bank_pointId);
				ServerSendChatFmt(packet, GUILD, "Bank Here K:%u X:%u Y:%u", current_kingdom, map.x, map.y);
				// SendMailFmt(packet, player_name, "Bank Location", "Bank Here K:%u X:%u Y:%u", nowKingdomID, map.x, map.y);
			}
			return;
		}
		
		if (previlage_level < RANK_5) {
			SendMailFmt(packet, player_name, "Request denied", "You don't have permission to view the bank balance!");
			return;
		}
		
		// Format BANK
		format_number2(bank_resource.food, bank_resource_str.food, 20);
		format_number2(bank_resource.rock, bank_resource_str.rock, 20);
		format_number2(bank_resource.wood, bank_resource_str.wood, 20);
		format_number2(bank_resource.ore,  bank_resource_str.ore,  20);
		format_number2(bank_resource.gold, bank_resource_str.gold, 20);
		
		uint64_t BagFood = GetBagFood();
		uint64_t BagRock = GetBagRock();
		uint64_t BagWood = GetBagWood();
		uint64_t BagOre = GetBagOre();
		uint64_t BagGold= GetBagGold();
		
		// Format BAG
		format_number2(BagFood, bank_bag_resource_str.food, 20);
		format_number2(BagRock, bank_bag_resource_str.rock, 20);
		format_number2(BagWood, bank_bag_resource_str.wood, 20);
		format_number2(BagOre,  bank_bag_resource_str.ore,  20);
		format_number2(BagGold, bank_bag_resource_str.gold, 20);
		
		// Format TOTAL
		format_number2(bank_resource.food + BagFood,  bank_total_resource_str.food, 20);
		format_number2(bank_resource.rock + BagRock,  bank_total_resource_str.rock, 20);
		format_number2(bank_resource.wood + BagWood,  bank_total_resource_str.wood, 20);
		format_number2(bank_resource.ore  + BagOre,   bank_total_resource_str.ore,  20);
		format_number2(bank_resource.gold + BagGold,  bank_total_resource_str.gold, 20);
		
		SendMailFmt(packet, player_name, "Bank Balance", 
			"[BNK] Food: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s\n"
			"[BAG] Food: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s\n"
			"[SUM] Food: %s | Stone: %s | Wood: %s | Ore: %s | Gold: %s",
			bank_resource_str.food, bank_resource_str.rock, bank_resource_str.wood, bank_resource_str.ore, bank_resource_str.gold,
			bank_bag_resource_str.food, bank_bag_resource_str.rock, bank_bag_resource_str.wood, bank_bag_resource_str.ore, bank_bag_resource_str.gold,
			bank_total_resource_str.food, bank_total_resource_str.rock, bank_total_resource_str.wood, bank_total_resource_str.ore, bank_total_resource_str.gold
		);
		return;
	}
	
	// Show how many relocators available
	if (cmd.cmd == CMD_SHOW_RELOCATORS) {
		if (message[cmd.offset] != '\0') return;
		SendMailFmt(packet, player_name, "Relocators Info", "Advance Relocator: %u\nRandom Relocator: %u\nMigration scroll: 5", items[ADVANCE_RELOCATOR].quantity, items[RANDOM_RELOCATOR].quantity);
		return;
	}
	
	// kingdom 
	if (cmd.cmd == CMD_KINGDOM) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Kingdom Info", "Current Kingdom: %u", current_kingdom);
		} else { // command "!kd 1024" set kingdom 
			handle_kd_command(packet, player_name, message + cmd.offset + 1);
		}
		return;
	}
	
	if (cmd.cmd == CMD_ADVANCED_RELOCATE) {
		if (previlage_level < RANK_8) {
			SendMailFmt(packet, player_name, "Request denied", "Only admins with rank 8-10 can relocate the bank!");
			return;
		}
		
		if (message[cmd.offset] == ' ') {
			if (memcmp(message + cmd.offset + 1, "rand", 4) == 0) {
				change_location_rand(packet, player_name);
				return;
			}
			
			change_location(packet, player_name, message + cmd.offset + 1);
			return;
		} else {
			// command without parameters → show usage instructions
			SendMailFmt(packet, player_name, "Usage relocator", "Usage: %c%.*s X:<num> Y:<num> or <num>:<num>", COMMAND_PREFIX, cmd.offset, message);
		}
		
		return;
	}
	
	// relocate randomly 
	if (memcmp(message, "rand", 4) == 0) {
		if (previlage_level < RANK_8) {
			SendMailFmt(packet, player_name, "Request denied", "Only admins with rank 8-10 can relocate the bank!");
			return;
		}
		
		if (message[cmd.offset] == '\0') {
			change_location_rand(packet, player_name);
			return;
		}
		
		if (memcmp(message + cmd.offset + 1, "chaos", 5) == 0) {
			RandomRelocateChaosArena(packet);
			return;
		}
		return;
	}
	
	if (cmd.cmd == CMD_PROCESS_STOP) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage stop", 
				"Missing stop target. Usage: %cstop rss | %cstop help | %cstop all",
				COMMAND_PREFIX, COMMAND_PREFIX, COMMAND_PREFIX
			);
			return;
		}
		
		if (strcmp(player_name, req.player_name) != 0) {
			// Only high-rank admins can stop others
			if (previlage_level < RANK_8) {
				SendMailFmt(packet, player_name, "Request denied", "You cannot stop another player's process!");
				return;
			}
		}

		
		if (memcmp(message + cmd.offset + 1, "rss\0", 4) == 0) {
			if (resource_sending_active) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				SendMailFmt(packet, player_name, "Process Stopped", "Resource sending has been stopped by %s", player_name);
			} else {
				SendMailFmt(packet, player_name, "No process", "No resource sending is currently in progress.");
			}
			return;
		}
		
		if (memcmp(message + cmd.offset + 1, "help\0", 5) == 0) {
			if (help_spam_count > 0) {
				SendMailFmt(packet, player_name, "Process Stopped", "Alliance help spam has been stopped by %s", player_name);
				help_spam_count = 0;
				sendBuildingCancel(packet, 0);
			} else {
				SendMailFmt(packet, player_name, "No process", "No help spam is currently running.");
			}
			return;
		}
		
		if (memcmp(message + cmd.offset + 1, "scan\0", 5) == 0) {
			if (kingdom_scanning) {
				SendMailFmt(packet, player_name, "Scan stop", "Scanning stopped");
				kingdom_scanning = false;
			} else {
				SendMailFmt(packet, player_name, "No process", "No scanning is currently running!");
			}
			return;
		}
		
		if (memcmp(message + cmd.offset + 1, "all\0", 4) == 0) {
			bool stopped = false;
			char status_msg[4096];
			size_t len = 0;
			
			if (resource_sending_active) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				// Add newline only if help spam exists
				const char *newline = (help_spam_count > 0) ? "\n" : "";
				len += snprintf(status_msg + len, sizeof(status_msg) - len, "Resource sending has been stopped by %s%s", player_name, newline);
				stopped = true;
			}
			
			if (help_spam_count > 0) {
				help_spam_count = 0;
				sendBuildingCancel(packet, 0);
				len += snprintf(status_msg + len, sizeof(status_msg) - len, "Alliance help spam has been stopped by %s", player_name);
				stopped = true;
			} 
			
			if (stopped) {
				SendMailFmt(packet, player_name, "Process stopped", status_msg);
			} else {
				SendMailFmt(packet, player_name, "No process", "No active processes to stop.");
			}
			return;
		}
		
		if (memcmp(message + cmd.offset + 1, "spam", 4) == 0) {
			message_spam = false;
			return;
		}
		
		// ❌ Invalid subcommand
		SendMailFmt(packet, player_name, "Oops",
			"Unknown stop command. Valid options are: %cstop rss | %cstop help | %cstop all", COMMAND_PREFIX, COMMAND_PREFIX, COMMAND_PREFIX);
		return;
	}
	
	
	if (memcmp(message, "camp", 4) == 0) {
		if (message[4] == ' ') {
			send_troop(packet, player_name, message + 5);
		} else {
			SendMailFmt(packet, player_name, "Usage camp", "Usage: !camp X:<num> Y:<num> or <num>:<num>");
		}
		return;
	}
	
	if (memcmp(message, "ret", 3) == 0) {
		if (message[4] == ' ') {
			send_ret_troop(packet, player_name, message + 4);
		} else {
			SendMailFmt(packet, player_name,"usage ret", "Usage: !ret <army_num>");
		}
		return;
	}
	
	if (memcmp(message, "shield", 6) == 0) {
		if (message[6] == '\0') {
			SendMailFmt(packet, player_name, "Shield Usages", 
				"Shield commands:\n"
				"%cshield list  – show shield items\n"
				"%cshield info  – show shield status\n"
				"%cshield use 72h / 24h / 8h / 4h  – activate shield",
            COMMAND_PREFIX, COMMAND_PREFIX, COMMAND_PREFIX);
			return;
		}
		
		if (memcmp(message + 7, "list\0", 5) == 0) {
			SendMailFmt(packet, player_name, "Shield Lists",
				"Shield Inventory:\n"
				"4h: %u, 8h: %u, 24h: %u, 3 Day: %u, 7 Day: %u",
				items[SHIELD_4H].quantity,
				items[SHIELD_8H].quantity,
				items[SHIELD_24H].quantity,
				items[SHIELD_72H].quantity,
				items[SHIELD_7D].quantity
			);
			return;
		}
		
		if (memcmp(message + 7, "info", 4) == 0 || memcmp(message + 7, "status", 6) == 0) {
			uint32_t i = 0;
			bool shield_active = false;
			char name[50];
			
			for (i = 0; i < 255; i++) {
				uint32_t itemID = itemInfo[i].item_id;
				
				if (itemID == SHIELD_4H || itemID == SHIELD_8H || itemID == SHIELD_24H || itemID == SHIELD_72H || itemID == SHIELD_7D) {
					shield_active = true;
					uint32_t remaining = 0;
					uint64_t endTime = itemInfo[i].begin + itemInfo[i].duration;
					
					if (endTime > server_time) {
						remaining = (uint32_t)(endTime - server_time);
					}
					
					get_name_by_item_id(itemID, name);
					
					
					SendMailFmt(packet, player_name, "Shield Info", "Shield Duration: %s\nShield Type: %s", FormatTime(remaining), name);
				}
				
			}
			
			if (!shield_active) {
				SendMailFmt(packet, player_name, "No shield", "No shield active!");
				return;
			}
			/*
			itemInfo[i].quantity = num;
					itemInfo[i].item_id = itemID;
					itemInfo[i].begin = num2;
					itemInfo[i].duration = num3;*/
			return;
		}
		
		if (memcmp(message + 7, "use", 3) == 0) {
			if (previlage_level < RANK_8) {
				SendMailFmt(packet, player_name,
					"Request denied", 
					"You don't have permission to activate a shield!"
				);
				return;
			}
			
			if (message[10] == '\0') {
				SendMailFmt(packet, player_name,
                    "Usage",
                    "Usage: !shield use [24h|72h|7d]");
                    return;
			}
			
			if (message[10] == ' ') {
				if (memcmp(message + 11, "4h\0", 3) == 0) {
					UseItem(packet, SHIELD_4H, 1);
					return;
				}
				
				if (memcmp(message + 11, "8h\0", 3) == 0) {
					UseItem(packet, SHIELD_8H, 1);
					return;
				}
				
				if (memcmp(message + 11, "24h\0", 4) == 0 || memcmp(message + 11, "1day\0", 5) == 0) {
					UseItem(packet, SHIELD_24H, 1);
					return;
				}
				
				if (memcmp(message + 11, "72h\0", 4) == 0 || memcmp(message + 11, "3day\0", 5) == 0) {
					UseItem(packet, SHIELD_72H, 1);
					return;
				}
			} 
		}
		
		// fallback for unknown subcommand
		SendMailFmt(packet, player_name, "Err",
			"Invalid shield command. Try:\n"
			"%cshield list\n"
			"%cshield info\n"
			"%cshield use 3day / 24h / 8h / 4h",
			COMMAND_PREFIX, COMMAND_PREFIX, COMMAND_PREFIX);
		return;
	}
	
	if (memcmp(message, "help", 4) == 0) {
		if (message[4] == '\0') {
			SendMailFmt(packet, player_name, "Usage help", "%chelp <count> <speed>", COMMAND_PREFIX);
			return;
		}
		
		if (help_spam_count != 0) {
			SendMailFmt(packet, player_name, "in process", "Helping process already in background!");
			return;
		}
		
		if (sscanf(message + 5, "%d %d", &help_spam_count, &help_speed) != 2) {
			SendMailFmt(packet, player_name, "idk", "Usage: %chelp <count> <speed>", COMMAND_PREFIX);
			return;
		}
		
		if (help_spam_count <= 0 || help_spam_count > 100) {
			SendMailFmt(packet, player_name, "", "Help count must be 1-100");
			return;
		}
		
		if (help_speed <= 0 || help_speed >= 10) {
			SendMailFmt(packet, player_name, "", "Help speed must be 1-10");
			return;
		}
		
		sendStartBuilding(packet, 0, 0);
		SendMailFmt(packet, player_name, "", "Started");
		return;
	}
	
	if (memcmp(message, "status", 6) == 0 || memcmp(message, "proc", 4) == 0) {
		bool any = false;
		char status_msg[4096];
		size_t len = 0;
		char mm[20];
		
		if (resource_sending_active) {
			format_number2(req.remains_amount, mm, 20);
			
			map_pos_t res = getTileMapPosbyPointCode(req.zoneId, req.pointId);
			
			// Add newline only if help spam exists
			const char *newline = (help_spam_count > 0) ? "\n" : "";
			
			len += snprintf(status_msg + len, sizeof(status_msg) - len, 
				"Resource sending → Target: %s | Type: %s | Remaining: %s | Location: %u:%u%s",
				req.player_name,
				resource_to_string(req.resource_type),
				mm,
				res.x,
				res.y,
				newline
			);
			any = true;
		}
		
		if (help_spam_count > 0) {
			len += snprintf(status_msg + len, sizeof(status_msg) - len,
				"Alliance help spam → Remaining: %u",
				help_spam_count
			);
			any = true;
		}
		
		if (any) {
			SendMailFmt(packet, player_name, "Running Process", status_msg);
		} else {
			SendMailFmt(packet, player_name, "No process", "No active processes are running.");
		}
		
		return;
	}
	
	if (memcmp(message, "mailall", 7) == 0) {
		if (message[7] == '\0') {
			SendMailFmt(packet, player_name, "Usage of mailall", "%cmailall \"subject\" \"message\"");
			return;
		}
		
		char subject[255];
		char msg[1024];
		
		// Read both quoted strings
		if (sscanf(message + 8, "\"%254[^\"]\" \"%1023[^\"]\"", subject, msg) != 2) {
			SendMailFmt(packet, player_name, "", "Invalid parameter");
			return;
		}
		
		SendAllMail(packet, subject, msg);
	}
	
	/*
	if (memcmp(message, "food", 4) == 0) {
		if (message[4] == '\0') {
			ServerSendChat(packet, GUILD, "Usage: '!food <amount>' or '!food <amount> <player_name>'.");
			return;
		}
	} else if (memcmp(message, "stone", 5) == 0) {
		if (message[5] == '\0') {
			ServerSendChat(packet, GUILD, "Usage: '!stone <amount>' or '!stone <amount> <player_name>'.");
			return;
		}
	} else if (memcmp(message, "wood", 4) == 0) {
		if (message[4] == '\0') {
			ServerSendChat(packet, GUILD, "Usage: '!wood <amount>' or '!wood <amount> <player_name>'.");
			return;
		}
	} else if (memcmp(message, "ore", 3) == 0) {
		if (message[3] == '\0') {
			ServerSendChat(packet, GUILD, "Usage: '!ore <amount>' or '!ore <amount> <player_name>'.");
			return;
		}
	} else if (memcmp(message, "gold", 4) == 0) {
		if (message[4] == '\0') {
			ServerSendChat(packet, GUILD, "Usage: '!gold <amount>' or '!gold <amount> <player_name>'.");
			return;
		}
	}
	
	
	if (memcmp(message, "food", 4) == 0 || 
		memcmp(message, "stone", 5) == 0 || 
		memcmp(message, "wood", 4) == 0 ||
		memcmp(message, "ore", 3) == 0 || 
		memcmp(message, "gold", 4) == 0) {
		
		if (CurrentMarch == MaxMarchEventNum) {
			resource_sending_active = false;
			memset(&req, 0, sizeof(req));
			ServerSendChat(packet, GUILD, "Troops marching. Wait until they return.");
			return;
		}
		
		if (resource_sending_active) {
			resource_sending_active = false;
			memset(&req, 0, sizeof(req));
			ServerSendChat(packet, GUILD, "Previous process cancelled!");
			// ServerSendChat(packet, GUILD, "Previous resources sending process in progress. Use !stop to cancel.");
			return;
		}
		
		
		
		req = send_resources(packet, message, player_name);
		
		if (req.resource_type == RESOURCE_FOOD) {
			if (req.gross_amount > bank_resource.food) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				ServerSendChat(packet, GUILD, "Not enough food!");
				return;
			}
		} else if (req.resource_type == RESOURCE_ROCK) {
			if (req.gross_amount > bank_resource.rock) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				ServerSendChat(packet, GUILD, "Not enough rock!");
				return;
			}
		} else if (req.resource_type == RESOURCE_WOOD) {
			if (req.gross_amount > bank_resource.wood) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				ServerSendChat(packet, GUILD, "Not enough wood!");
				return;
			}
		} else if (req.resource_type == RESOURCE_ORE) {
			if (req.gross_amount > bank_resource.ore) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				ServerSendChat(packet, GUILD, "Not enough ore!");
				return;
			}
		} else if (req.resource_type == RESOURCE_GOLD) {
			if (req.gross_amount > bank_resource.gold) {
				resource_sending_active = false;
				memset(&req, 0, sizeof(req));
				ServerSendChat(packet, GUILD, "Not enough gold!");
				return;
			}
		}
		
		if (req.remains_amount > 0) {
			SendAllyPoint(packet, req.player_name);
			printf("target player: '%s'\n", req.player_name);
			return;
		}
	}
	*/
	
	// !transfer food 10M TNTBay 
	if (cmd.cmd == CMD_TRANSFER_RESOURCE) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage", "Usage: %ctransfer <rss_type> <amount> <target_name>");
			return;
		}
		
		
		return;
	}
	
	// !swap rate
	// !swap food wood 100M
	if (cmd.cmd == CMD_SWAP_RESOURCE) {
		// command without parameters 
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usage", "Usage: %cswap <input_resource> <output_resource> <amount>");
			return;
		}
		
		if (strncasecmp(message + cmd.offset + 1, "rate", 4) == 0) {
			ShowSwapRates(packet, player_name);
			return;
		}
		
		Swap_Handler(packet, player_name, message + cmd.offset + 1);
		return;
	}
	
	if (cmd.cmd == CMD_SUPERUSER) {
		if (message[cmd.offset] != '\0') {
			SendMailFmt(packet, player_name, "Superuser Request Denied",
				"Unauthorized attempt detected. Superuser privileges cannot be granted."
			);
			return;
		}
		
		SendMailFmt(packet, player_name, "Superuser Info", "The current superuser is: %s", SUPERUSER);
		return;
	}
	
	if (cmd.cmd == CMD_SQL) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "", "%csql query require", COMMAND_PREFIX);
			// SendMailFmt(packet, player_name, "Usages", "%cfind player_name", COMMAND_PREFIX);
			return;
		}
		
		SendMailFmt(packet, player_name, "", "DONE");
		return;
	}
	
	if (cmd.cmd == CMD_SCAN_KINGDOM) {
		if (kingdom_scanning) {
			SendMailFmt(packet, player_name, 
				"Kingdom Scan",
				"A kingdom scan is already in progress."
			);
			return;
		}
		
		kingdom_scanning = true;
		zone[0] = 0;
		
		SendMailFmt(packet, player_name,
			"Kingdom Scan Started",
			"Map scanning has started.\n"
			"Player data will be updated as the scan progresses."
		);
	}
	
	if (cmd.cmd == CMD_FIND_PLAYER) {
		if (message[cmd.offset] == '\0') {
			SendMailFmt(packet, player_name, "Usages of find", "%cfind player_name", COMMAND_PREFIX);
			// SendMailFmt(packet, player_name, "Usages", "%cfind player_name", COMMAND_PREFIX);
			return;
		}
		
		char name[13] = {0};
		memcpy(name, message + cmd.offset + 1, strlen(message + cmd.offset + 1));
		
		castle ct = find_castle_by_name(name);
		
		if (ct.name[0] == '\0') {
			SendMailFmt(packet, player_name,
            "Player Not Found",
            "The map is continuously scanning.\n"
            "• Make sure the name is correct\n"
            "• Try the full player name\n"
            "• Or use a few characters from the name\n"
            "If not found, wait a moment and try again.");
			// ServerSendChat(packet, GUILD, "Not found (+_•)");
		} else {
			map_pos_t map = getTileMapPosbyPointCode(ct.zone, ct.point);
			
			if (strcmp(player_name, SUPERUSER) == 0) {
				if (ct.tag[0] == '\0') {
					SendMailFmt(packet, player_name, "Here", "%s K:%u X:%u Y:%u shield: %s", ct.name, ct.kingdom, map.x, map.y, ct.shield ? "ON" : "OFF");
				} else {
					SendMailFmt(packet, player_name, "Here", "[%s]%s K:%u X:%u Y:%u shield: %s", ct.tag, ct.name, ct.kingdom, map.x, map.y, ct.shield ? "ON" : "OFF");
				}
				return;
			}
			
			if (ct.tag[0] == '\0') {
				ServerSendChatFmt(packet, GUILD, "%s K:%u X:%u Y:%u shield: %s", ct.name, ct.kingdom != 1745 ? 1745 : ct.kingdom, map.x, map.y, ct.shield ? "ON" : "OFF");
			} else {
				ServerSendChatFmt(packet, GUILD, "[%s]%s K:%u X:%u Y:%u shield: %s", ct.tag, ct.name, ct.kingdom != 1745 ? 1745 : ct.kingdom, map.x, map.y, ct.shield ? "ON" : "OFF");
			}
			
			return;
		}
	
		
		
		
		return;
		if (is_finding) {
			ServerSendChatFmt(packet, GUILD, "Error", "Already finding process in background!");
			return;
		}
		
		if (sscanf(message + cmd.offset + 1, "%u %13[^\n]", &find_kingdom, find_player) != 2) {
			if (sscanf(message + cmd.offset + 1, "%13[^\n]", find_player) != 1) {
				ServerSendChatFmt(packet, GUILD, "!find playerName");
				return;
			}
		}
		
		zone[0] = 0;
		is_ready = true; 
		
		find_player_len = strlen(find_player);
		
		is_finding = true;
		ServerSendChatFmt(packet, GUILD, "Finding, Please wait!");
		return;
	}
	
	
	
}		


/*
if (inventory[SHIELD_24H] > 0) {
    UseItem(packet, SHIELD_24H, 1);
} else {
    ServerSendChat(packet, GUILD, "❌ No 24h shield available in inventory.");
}
*/

void SendResourcesHandler(Packet *packet) {
	if (CurrentMarch == MaxMarchEventNum) {
		return;
	}
	
	if (req.is_locked) return;
	
	Resources resource2;
	
	memset(&resource2, 0, sizeof(resource2));
	uint32_t chunk = 0;
	
	if (req.remains_amount > 0) {
		chunk = (req.remains_amount >= 3260870) ? 3260870 : req.remains_amount;
		
		if (req.resource_type == RESOURCE_FOOD) {
			resource2.food = chunk;
		} else if (req.resource_type == RESOURCE_ROCK) {
			resource2.rock = chunk;
		} else if (req.resource_type == RESOURCE_WOOD) {
			resource2.wood = chunk;
		} else if (req.resource_type == RESOURCE_ORE) {
			resource2.ore  = chunk;
		} else if (req.resource_type == RESOURCE_GOLD) {
			resource2.gold = chunk;
		}
		
		// map_pos_t res = getTileMapPosbyPointCode(req.zoneId, req.pointId);
		//printf ("Location: X:%u Y:%u\n", res.x, res.y);
		
		if (SendResource(packet, resource2, req.zoneId, req.pointId)) {
			req.is_locked = true;
		}
	}
}

char name[13] = "PORT COLD";

void RecvUseItem(Packet p, Packet *packet) {
	uint8_t b = p.read_u8();
	uint16_t item_id = p.read_u16();
	uint16_t item_quantity = p.read_u16();
	uint16_t num3 = p.read_u16();
	
	if (b != 0) {
		printf("RecvUseItem b: %u\n", b);
		return;
	}
	
	items[item_id].quantity = item_quantity;
	
	if (item_id == ADVANCE_RELOCATOR || item_id == RANDOM_RELOCATOR) {
		uint16_t zoneId = p.read_u16();
		uint8_t pointId = p.read_u8();
		uint16_t kingdom_id = p.read_u16();
		
		bank_zoneId = zoneId;
		bank_pointId = pointId;
		
		
		map_pos_t map = getTileMapPosbyPointCode(zoneId, pointId);
		
		if (kingdom_id == 86) {
			SendMailFmt(packet, SUPERUSER, "", "Relocated: PK:1 X:%u Y:%u", map.x, map.y);
			return;
		}
		
		SendMailFmt(packet, SUPERUSER,"", "Relocated: %u:%u:%u", kingdom_id, map.x, map.y);
		return;
	} else if (item_id == SHIELD_4H || item_id == SHIELD_8H || item_id == SHIELD_24H || item_id == SHIELD_72H) {
		memset(&itemInfo, 0, sizeof(itemInfo));
		
		uint16_t num = p.read_u16(); // how many shield
		uint16_t itemID = p.read_u16(); // id
		
		uint64_t num2 = p.read_u64(); // Unix timestamp 
		uint32_t num3 = p.read_u32(); // Duration
		
		itemInfo[0].quantity = num;
		itemInfo[0].item_id = itemID;
		itemInfo[0].begin = num2;
		itemInfo[0].duration = num3;
					
		char name[50];
		get_name_by_item_id(item_id, name);
		
		SendMailFmt(packet, SUPERUSER, "done", "%s shield activated!", name);
	}
	
	printf("item_id: %u\n", item_id);
	printf("item_quantity: %u\n", item_quantity);
	printf("num2: %u\n", num3);
	
	return;
}

void RecvAllianceHelp(Packet p) {
	uint8_t b = p.read_u8();
	
	if (b != 0) {
		return;
	}
	
	int num = (int)p.read_u8();
	uint16_t EventID = p.read_u16();
	uint8_t EventDataLv = p.read_u8();
	uint8_t HelpMax = p.read_u8();
	
	printf("\n\n\n");
	printf("num: %u\n", num);
	printf("EventID: %u\n", EventID);
	printf("EventDataLv: %u\n", EventDataLv);
	printf("HelpMax: %u\n", HelpMax);
	
}

/*

PACKET TYPE: _MSG_RESP_DELETEMAIL
PACKET SIZE: 9
PACKET TYPE: _MSG_RESP_MAILINFO
PACKET SIZE: 52
*/

void RoleAttrLevelUp(Packet *p, int UpdateFlag = 59) {
	if ((UpdateFlag & 1) != 0) {
		p->read_u8();
		// this.UpdateRoleAttrLevel(MP.ReadByte(-1));
	}
	
	if ((UpdateFlag & 2) != 0) {
		p->read_u32();
		// this.UpdateRoleAttrExp(MP.ReadUInt(-1));
	}
	
	if ((UpdateFlag & 4) != 0) {
		p->read_u32();
		// DataManager.Instance.Resource[4].Stock = MP.ReadUInt(-1);
	}
	
	if ((UpdateFlag & 8) != 0) {
		p->read_u16();
		// this.UpdateRoleAttrMorale(MP.ReadUShort(-1));
	}
	
	if ((UpdateFlag & 16) != 0) {
		p->read_u64();
		// DataManager.Instance.RoleAttr.LastMoraleRecoverTime = MP.ReadLong(-1);
	}
	
	if ((UpdateFlag & 32) != 0) {
		p->read_u16();
		// this.UpdateRoleTalentPoint(MP.ReadUShort(-1));
	}
}

static char Title[256];
static char Content[65535];
static char SenderTag[4];
static char SenderName[13];

void RecvMailInfo(Packet *p, Packet *packet) {
	uint32_t SerialID = p->read_u32();
	uint8_t b = p->read_u8();
	uint64_t stime = p->read_u64();
	uint8_t MailType = p->read_u8();
	uint32_t ReplyID = p->read_u32();
	uint16_t SenderHead = p->read_u16();
	uint16_t SenderKindom = p->read_u16();
	
	p->read_bytes(SenderTag, 3);
	p->read_bytes(SenderName, 13);
	SenderTag[3] = '\0';
	
	uint8_t ExtraFlag = p->read_u8();
	uint8_t TitleLen = p->read_u8();
	uint16_t ContentLen = p->read_u16();
	uint8_t AttachNum = p->read_u8();
	
	printf("\ntag: %s\n", SenderTag);
	printf("name: %s\n", SenderName);
	printf("SenderKindom: %u\n", SenderKindom);
	printf("ExtraFlag: %u\n", ExtraFlag);
	printf("TitleLen: %u\n", TitleLen);
	printf("ContentLen: %u\n", ContentLen);
	
	printf("AttachNum: %u\n", AttachNum);
	
	for (int i = 0; i < AttachNum; i++) {
		uint16_t KingdomID = p->read_u16();
		uint16_t ZoneID = p->read_u16();
		uint8_t PointID = p->read_u8();
			
		printf("KingdomID: %u\n", KingdomID);
		printf("ZoneID: %u\n", ZoneID);
		printf("PointID: %u\n", PointID);
	}
				
	// char Title[256];
	// char Content[65535];
	memset(Title, 0, sizeof(Title));
	memset(Content, 0, sizeof(Content));
	p->read_bytes(Title, TitleLen);
	p->read_bytes(Content, ContentLen);
	
	printf("Title: %s\n", Title);
	printf("Content: %s\n", Content);
	
	Command cmd = is_command(Content);
	
	if (cmd.cmd != CMD_NONE) {
		command_handler(packet, SenderName, cmd, Content);
	} 
	
	return;
}


time_t last_call = 0;

void DelayHandler(Packet *packet, int delay) {
    time_t now = time(NULL);

    // if not enough time passed → exit early
    if (now - last_call < delay) {
        return;
    }
    
    //ServerSendChat(packet, GUILD, "Advance Relocator: %u\nRandom Relocator: %u\nMi

    // update timestamp
    last_call = now;
}


uint32_t TreasureIDTransToNew(uint32_t TreasureID) {
	return TreasureID;
}

void RecvMall_Info(Packet *MP) {
	uint8_t b = MP->read_u8();
	uint8_t b2 = MP->read_u8();
	
	printf("b2: %u\n", b2);
	
	for (int i = 0; i < (int)b2; i++) {
		uint16_t sn = MP->read_u16();
		
		uint32_t TreasureID = TreasureIDTransToNew(MP->read_u32());
		printf("SN: %u\n", sn);
		printf("TreasureID: %u\n", TreasureID);
		return;
	}
	
	return;
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

typedef enum {
	// Token: 0x04000CC1 RID: 3265
	CCR_BATTLE,
	// Token: 0x04000CC2 RID: 3266
	CCR_RESOURCE,
	// Token: 0x04000CC3 RID: 3267
	CCR_COLLECT,
	// Token: 0x04000CC4 RID: 3268
	CCR_SCOUT,
	// Token: 0x04000CC5 RID: 3269
	CCR_RECON,
	// Token: 0x04000CC6 RID: 3270
	CCR_MONSTER,
	// Token: 0x04000CC7 RID: 3271
	CCR_NPCSCOUT,
	// Token: 0x04000CC8 RID: 3272
	CCR_NPCCOMBAT,
	// Token: 0x04000CC9 RID: 3273
	CCR_PETREPORT,
	// Token: 0x04000CCA RID: 3274
	CCR_MAX
} CombatCollectReport;


const char* point_kind_to_str(int pk) {
    switch (pk) {
        case PK_NONE: return "PK_NONE";
        case PK_FOOD: return "PK_FOOD";
        case PK_STONE: return "PK_STONE";
        case PK_IRON: return "PK_IRON";
        case PK_WOOD: return "PK_WOOD";
        case PK_GOLD: return "PK_GOLD";
        case PK_CRYSTAL: return "PK_CRYSTAL";
        case PK_SP_MINE: return "PK_SP_MINE";
        case PK_CITY: return "PK_CITY";
        case PK_CAMP: return "PK_CAMP";
        case PK_NPC: return "PK_NPC";
        case PK_YOLK: return "PK_YOLK";
        case PK_DYNAMIC_OBSTACLE: return "PK_DYNAMIC_OBSTACLE";
        case PK_UNDEFINED: return "PK_UNDEFINED";
        case PK_MAX: return "PK_MAX";
        default: return "UNKNOWN";
    }
}

// Calculate total number of troop might 
uint64_t CalcTroopMight(uint32_t TroopsInfo[19]) {
    return
        (TroopsInfo[0]  *  2)  +  // INF T1
        (TroopsInfo[1]  *  8)  +  // INF T2
        (TroopsInfo[2]  * 24)  +  // INF T3
        (TroopsInfo[3]  * 36)  +  // INF T4
        
        (TroopsInfo[4]  *  2)  +  // RNG T1
        (TroopsInfo[5]  *  8)  +  // RNG T2
        (TroopsInfo[6]  * 24)  +  // RNG T3
        (TroopsInfo[7]  * 36)  +  // RNG T4
        
        (TroopsInfo[8]  *  2)  +  // CAV T1
        (TroopsInfo[9]  *  8)  +  // CAV T2
        (TroopsInfo[10] * 24)  +  // CAV T3
        (TroopsInfo[11] * 36)  +  // CAV T4
        
        (TroopsInfo[12] *  2)  +  // SIE T1
        (TroopsInfo[13] *  8)  +  // SIE T2
        (TroopsInfo[14] * 24)  +  // SIE T3
        (TroopsInfo[15] * 36);    // SIE T4
}

typedef struct {
	uint16_t HeroID;
	uint8_t Rank;
	uint8_t Star;
} Defense_Hero;

// Handles player targets
void HandleScoutReportInfo(Packet *packet, Packet* p) {
	uint32_t Id = p->read_u32();
	uint8_t flag = p->read_u8();
	uint64_t Time = p->read_u64();
	
	uint16_t KingdomID = p->read_u16();
	uint16_t CombatlZone = p->read_u16();
	uint8_t CombatPoint = p->read_u8();
	
	uint8_t CombatPointKind = (POINT_KIND)p->read_u8();
	uint16_t ObjKingdomID = p->read_u16();
	
	char ObjAllianceTag[4] = {0};
	p->read_bytes(ObjAllianceTag, 3);
	
	char ObjName[13] = {0};
	p->read_bytes(ObjName, 13);
	
	uint8_t ScoutResult = p->read_u8();
	uint8_t ScoutLevel = p->read_u8();
	uint16_t ScoutContentLen = p->read_u16();
	uint8_t ScoutContent[65535];
	// p->read_bytes(ScoutContent, ScoutContentLen);
			
	map_pos_t dst = getTileMapPosbyPointCode(CombatlZone, CombatPoint);
	
	printf("Id: %u\n", Id);
	printf("flag: %u\n", flag);
	printf("Time: %lu\n", Time);
	printf("KingdomID: %u\n", KingdomID);
	printf("CombatlZone: %u\n", CombatlZone);
	printf("CombatPoint: %u\n", CombatPoint);
	printf("CombatPointKind = %s (%u)\n", point_kind_to_str(CombatPointKind), CombatPointKind);
	printf("ObjKingdomID: %u\n", ObjKingdomID);
	printf("ObjAllianceTag: %s\n", ObjAllianceTag);
	printf("ObjName: %s\n", ObjName);
	printf("ScoutResult: %u\n", ScoutResult);
	printf("ScoutLevel: %u\n", ScoutLevel);
	printf("ScoutContentLen: %u\n", ScoutContentLen);
	
	// dump_data("scout.bin", ObjName, p->data + p->offset, ScoutContentLen);
	
	if (ScoutResult == 1) {
		SendMailFmt(packet, SUPERUSER, "", "[Scout] Target '%s' at (%u:%u) is protected by shield or anti-scout.", ObjName, dst.x, dst.y);
		// dump_data("scout.bin", ObjName, p->data + p->offset, ScoutContentLen);
		return;
	}
	
	if (CombatPointKind != PK_CITY) {
		SendMailFmt(packet, SUPERUSER, "", "[Scout] Only castles are valid targets.");
		return;
	}
	
	uint8_t bKind = 3;// CombatPointKind;
	
	if (CombatPointKind == PK_CITY) {
		bKind = CCR_SCOUT;
	} else {
		bKind = CCR_MAX;
	}
	
	uint32_t food = 0;
	uint32_t rock = 0;
	uint32_t wood = 0;
	uint32_t ore = 0;
	uint32_t gold = 0;
	
	uint32_t TrapsInfo[12] = {0};
	uint32_t TroopsInfo[16] = {0};
	uint32_t ReinforceInfo[16] = {0};
	
	uint32_t DefenseNum = 0;
	uint16_t TroopsFlag = 0;
	
	uint32_t TrapsNum = 0;
	
	uint32_t ReinforceNum = 0;
	char player_name[20][13] = {0};
	
	Defense_Hero DefenseHero[5] = {0};
	
	
	uint32_t MusterNum = 0;
	uint16_t MusterFlag = 0;
	uint32_t MusterInfo[16] = {0};
	
	uint8_t ReinforcePlayerCount = 0;
	
	uint16_t MainHeroID = 0;
	uint8_t MainRank = 0;
	uint8_t MainStar = 0;
	
	uint8_t DefenseHeroCount = 0;
	
	
	if (ScoutLevel >= 1) {
		if (bKind == 0 || bKind == 3) {
			food = p->read_u32();
			rock = p->read_u32();
			wood = p->read_u32();
			ore =  p->read_u32();
			
			printf("food: %u\n", food);
			printf("rock: %u\n", rock);
			printf("wood: %u\n", wood);
			printf("ore: %u\n", ore);
		}
		DefenseNum = p->read_u32();
		printf("DefenseNum: %u\n", DefenseNum);
	}
	
	if (ScoutLevel >= 2) {
		if (bKind == 0 || bKind == 3) {
			gold = p->read_u32();
			TrapsNum = p->read_u32();
				
			printf("gold: %u\n", gold);
			printf("TrapsNum: %u\n", TrapsNum);
		}
		
		TroopsFlag = p->read_u16();
		printf("TroopsFlag: %u\n", TroopsFlag);
	}
	
	if (ScoutLevel >= 3 && bKind != 1) {
		ReinforceNum = p->read_u32();
		printf("ReinforceNum: %u\n", ReinforceNum);
	}
	
	uint16_t ReinforceFlag = 0;
	
	if (ScoutLevel >= 4) {
		if (bKind == 0 || bKind == 3) {
			uint16_t TrapsFlag = p->read_u16();
			ReinforceFlag = p->read_u16();
			
			for (int j = 0; j < 12; j++) {
				if ((TrapsFlag >> j & 1) == 1) {
					TrapsInfo[j] = p->read_u32();
					printf("TrapsInfo[%u]: %u\n", j, TrapsInfo[j]);
				}
			}
		}
		
		if (bKind == 2) {
			uint16_t ReinforceFlag = p->read_u16();
		}
	}
	
	if (ScoutLevel >= 5) {
		uint16_t MainHero = p->read_u16();
		uint8_t MainHeroHome = p->read_u8();
		
		for (int k = 0; k < 16; k++) {
			if ((TroopsFlag >> k & 1) == 1) {
				TroopsInfo[k] = p->read_u32();;
				printf("TroopsInfo[%u]: %u\n", k, TroopsInfo[k]);
			}
		}
	}
	
	if (ScoutLevel >= 6) {
		if (bKind == 0 || bKind == 3) {
			uint8_t WallStatus = p->read_u8();
		}
		uint32_t ReinforceInfo[16] = {0};
		
		if (bKind != 1) {
			for (int l = 0; l < 16; l++) {
				if ((ReinforceFlag >> l & 1) == 1) {
					ReinforceInfo[l] = p->read_u32();
					printf("ReinforceInfo[%u]: %u\n", l, ReinforceInfo[l]);
				}
			}
		}
	}
	
	if (ScoutLevel >= 7) {
		if (bKind == 0 || bKind == 3) {
			MusterNum = p->read_u32();
			MusterFlag = p->read_u16();
			
			for (int m = 0; m < 16; m++) {
				if ((MusterFlag >> m & 1) == 1) {
					MusterInfo[m] = p->read_u32();
				}
			}
		}
		
		if (bKind != 1) {
			ReinforcePlayerCount = p->read_u8();
			printf("Total Reinforce: %u\n", ReinforcePlayerCount);
			
			for (uint32_t i = 0; i < ReinforcePlayerCount; i++) {
				p->read_bytes(player_name[i], 13);
				printf("name: %s\n", player_name[i]);
			}
		}
	}
	
	if (ScoutLevel >= 8) {
		if (bKind == 0 || bKind == 3) {
			MainHeroID = p->read_u16();
			MainRank = p->read_u8();
			MainStar = p->read_u8();
			
			printf("Main HeroID: %u\n", MainHeroID);
			printf("Main Rank: %u\n", MainRank);
			printf("Main Star: %u\n", MainStar);
			
		}
		
		DefenseHeroCount = p->read_u8();
		
		printf("DefenseHeroCount: %u\n", DefenseHeroCount);
		
		for (uint32_t i = 0; i < DefenseHeroCount; i++) {
			DefenseHero[i].HeroID = p->read_u16();
			DefenseHero[i].Rank = p->read_u8();
			DefenseHero[i].Star = p->read_u8();
		}
	}
	
	
	
	
	char food_str[20], rock_str[20], wood_str[20], ore_str[20], gold_str[20];
	char troop_str[20] = {0};
	char troop_might[20] = {0};
	
	snprintf(food_str, sizeof(food_str), "%s", format_number(food, 0));
	snprintf(rock_str, sizeof(rock_str), "%s", format_number(rock, 0));
	snprintf(wood_str, sizeof(wood_str), "%s", format_number(wood, 0));
	snprintf(ore_str,  sizeof(ore_str),  "%s", format_number(ore, 0));
	snprintf(gold_str, sizeof(gold_str), "%s", format_number(gold, 0));
			
	snprintf(troop_str, sizeof(troop_str), "%s", format_number(DefenseNum, 0));
	snprintf(troop_might, sizeof(troop_might), "%s", format_number(CalcTroopMight(TroopsInfo), 0));
	
	// Now send formatted message
	SendMailFmt(packet, SUPERUSER, "Scout Report",
		"[Scout] Target: %s (%u:%u)\n"
		"Total troops: %s (might: %s)\n"
		"INF:  T1=%u T2=%u, T3=%u  T4=%u\n"
		"RNG:  T1=%u T2=%u, T3=%u  T4=%u\n"
		"CAV:  T1=%u T2=%u, T3=%u  T4=%u\n"
		"SIE:  T1=%u T2=%u, T3=%u  T4=%u\n"
		"Resources: Food: %s Stone: %s Wood: %s Ore: %s Gold: %s",
		ObjName, dst.x, dst.y,
		troop_str, troop_might,
		TroopsInfo[0], TroopsInfo[1], TroopsInfo[2], TroopsInfo[3],
		TroopsInfo[4], TroopsInfo[5], TroopsInfo[6], TroopsInfo[7],
		TroopsInfo[8], TroopsInfo[9], TroopsInfo[10], TroopsInfo[11],
		TroopsInfo[12], TroopsInfo[13], TroopsInfo[14], TroopsInfo[15],
		food_str, rock_str, wood_str, ore_str, gold_str
	);
}


#include <stdio.h>
#include <stdint.h>
#include <math.h>

// Food example
double CurrentStock;
uint32_t Capacity;
long Speed;          // per hour
double SpeedInSec;   // per second
double UpdateTime;

void SetResource(uint32_t val, long speed, uint32_t capacity) {
    CurrentStock = val;
    Speed = speed;
    SpeedInSec = (double)speed / 3600.0;  // convert to per second
    Capacity = capacity;
    UpdateTime = 1.0;
}

int Update(float delta) {
    if (Speed == 0 ||
        (Speed > 0 && CurrentStock >= Capacity) ||
        (Speed < 0 && CurrentStock <= 0.0)) {
        return 0;
    }

    UpdateTime -= delta;
    if (UpdateTime <= 0.0) {
        int before = (int)CurrentStock;

        if (SpeedInSec > 0.0) {
            if (CurrentStock + SpeedInSec > Capacity)
                CurrentStock = Capacity;
            else
                CurrentStock += SpeedInSec;
        } else {
            if (CurrentStock + SpeedInSec < 0.0)
                CurrentStock = 0.0;
            else
                CurrentStock += SpeedInSec;
        }

        UpdateTime += 1.0;

        if ((int)CurrentStock != before)
            return 1;  // stock changed
    }
    return 0;
}

void ResourceHelpReportInfo(Packet *p) {
	uint32_t aa = p->read_u32();
	uint8_t bb = p->read_u8();
	uint64_t cc = p->read_u64();
	uint8_t dd = p->read_u8(); // it's results 1 means received from player! and 0 means bank sending the resource
				
	char player_name[13] = {0};
	p->read_bytes(player_name, 13);
	
	ResHelp res;
	
	// how much resources received target player
	res.food = p->read_u32();
	res.rock = p->read_u32();
	res.wood = p->read_u32();
	res.ore  = p->read_u32();
	res.gold = p->read_u32();
	
	if (dd == 1) {
		update_balance(player_name, res, dd);
	}
	
	printf("Result: %u\n", dd);
	printf("player: %s\n", player_name);
				
	printf("food: %u\n", res.food);
	printf("rock: %u\n", res.rock);
	printf("wood: %u\n", res.wood);
	printf("ore: %u\n", res.ore);
	printf("gold: %u\n\n", res.gold);
}

void RequsetYolkswitch(Packet *packet) {
	packet->clear();
	packet->write_u16(_MSG_REQUEST_WONDER_SWITCH);
	packet->write_seq_id();
	packet->send(true);
}


/*
// Euclidean distance between two points
double distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    return sqrt((double)(dx * dx + dy * dy));
}*/

int distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    return (int)(sqrt((double)(dx * dx + dy * dy)) + 0.5); // rounds instead of truncating
}

// [JVN]PORT COLD K:1745 X:87 Y:701
// Forest K:1745 X:247 Y:777
void send_attack_troop(Packet *packet, bool attack_flag, uint16_t x, uint16_t y) {
	// if (attack_flag == false) return;
	
	PointCode DesPoint = getPointCodeByMapPos(x, y);
	
	printf("zoneId: %u\n", DesPoint.zoneID);
	printf("pointId: %u\n", DesPoint.pointID);
	
	MarchData data;
	memset(&data, 0, sizeof(data));
	
	// data.troops[0][2] = 202000; // T3 Infantry 
	data.troops[1][0] = 1; // T1 Cavalry 
	
	data.zone_id = DesPoint.zoneID;
    data.point_id = DesPoint.pointID;
    
	RequestMarchTroop(packet, data);
	
	
	/*
	RequestMarchTroop(packet, data, true);
	
	data.troops[0][2] = 0; // T3 Infantry 
	
	data.troops[1][2] = 202000; // T3 Cavalry 
	
	RequestMarchTroop(packet, data, true);
	RequestMarchTroop(packet, data, true);
	
	data.troops[1][2] = 0;
	
	data.troops[2][2] = 202000; // T3 Range
	
	RequestMarchTroop(packet, data, true);
	*/
	return;
}

/*
char name_list[100] = {0};

void _add_list(const char *name) {
	for (int i = 0; i < 100; i++) {
		if (name_list[i][0] == '\0') {
			strcpy(name_list, name);
		}
	}
}

void add_list(const char *fmt, ...) {
    char buf[4096];  // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    _add_list(buf);
}

void clear_list() {
	for (int i = 0; i < 100; i++) {
		if (name_list[i][0] != '\0') {
			name_list[i][0] = '\0';
		}
	}
}*/



int fd = -1;

void log(int fd, const char *fmt, ...) {
	if (fd == -1) return;
	
    char buf[4096];  // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    write(fd, buf, strlen(buf));
}


void RecvMapData(Packet *packet, uint8_t *data, uint16_t size) {
	// dump for debugging or analyzing
	// dump_data("mapdata.bin", "", data, size);
	
	uint16_t zoneId;
	uint8_t  pointId;
	char     name[13];
	char     tag[3];
	uint16_t kingdomID;
	uint8_t  level;
	uint8_t capitalFlag;
	uint8_t shield;
	
	for (uint32_t i = 0; i < size; i++) {
		if (data[i] != PK_CITY) {
			continue;
		}
		// printf("\n\n");
		
		memcpy(name,          data + i + 1,  13);
		memcpy(tag,           data + i + 14, 3);
		memcpy(&kingdomID,    data + i + 17, 2);
		memcpy(&level,        data + i + 19, 1);
		memcpy(&capitalFlag,  data + i + 20, 1);
		
		if (name[0] == '\0') {
			continue;
		}
		
		if (level < 1 || level > 55) {
			continue;
		}
		
		// printf("level: %u\n", level);
		
		if (kingdomID == 0 || kingdomID > 3000) {
			continue;
		}
		
		// printf("kingdom: %u\n", kingdomID);
		
		if (name[12] != '\0') {
			continue;
		}
		
		if (strcmp(name, "Dark.nest") == 0) {
			continue;
		}
		
		// printf("name: %s\n", name);
		
		memcpy(&zoneId,  data + i - 3, 2);
		memcpy(&pointId, data + i - 1, 1);
		
		//printf("zoneId: %u\n", zoneId);
		// printf("pointId: %u\n", pointId);
		
		if (zoneId > 1023) {
			continue;
		}
		
		if (capitalFlag & (1 << 2)) {
			shield = true;
		} else {
			shield = false;
		}
		
		
		// printf("zoneId: %u\n", zoneId);
		
		/* ✅ confirmed city */
		// map_pos_t map = getTileMapPosbyPointCode(zoneId, pointId);
		//name[12] = '\0';
		//tag[2]   = '\0';
		
		
		if (castle_exist(name)) {
			castle_update(zoneId, pointId, name, tag, kingdomID, level, shield);
			printf("Updated '%s' zone = %u\n", name, zoneId);
		} else {
			castle_add(zoneId, pointId, name, tag, kingdomID, level, shield);
			printf("Added '%s' zone = %u\n", name, zoneId);
		}
		
		/*
		if (tag[0] == '\0') {
			//sprintf(buf + strlen(buf), "%s K:%u X:%u Y:%u, ", name, kingdomID, map.x, map.y);
			printf("%s K:%u X:%u Y:%u L:%u\n", name, kingdomID, map.x, map.y, level);
			
			log(fd, "%s K:%u X:%u Y:%u L:%u\n", name, kingdomID, map.x, map.y, level);
		} else {
			//sprintf(buf + strlen(buf), "[%s]%s K:%u X:%u Y:%u, ", tag, name, kingdomID, map.x, map.y);
			printf("[%s]%s K:%u X:%u Y:%u L:%u\n", tag, name, kingdomID, map.x, map.y, level);
			
			log(fd, "[%s]%s K:%u X:%u Y:%u L:%u\n", tag, name, kingdomID, map.x, map.y, level);
		}*/
		
		/* skip ahead */
		// i += 20;
	}
	
	
	// ServerSendChatFmt(packet, GUILD, "FOUND: %s", buf);
	// printf("b: %s\n", buf);
    //exit(0);
}

/*
void RecvMapData(uint8_t *data, uint16_t size) {
	// dump for debugging or analyzing 
	// dump_data("mapdata.bin", "", data, size);
	
	uint16_t kingdomID;
	uint16_t zoneId;
	uint8_t pointId;
	uint8_t kind;
	char name[13];
	char tag[3];
	printf("packet size: %u\n", size);
	
	for (uint32_t i = 0; i < size; i++) {
		if (data[i] != PK_CITY) continue;
		
		
		memcpy(&zoneId,  data + i, 2);
		i += 2;
		memcpy(&pointId, data + i, 1);
		i += 1;
		memcpy(&kind, data + i, 1);
		i += 1;
		memcpy(name, data + i, 13);
		i += 13;
		memcpy(tag,  data + i, 3);
		i += 3;
		memcpy(&kingdomID, data + i, 2);
		i += 2;
		
		if (kind == PK_CITY) {
			map_pos_t map = getTileMapPosbyPointCode(zoneId, pointId);
			
			if (tag[0] == '\0') {
				printf("%s K:%u X:%u Y:%u\n", name, kingdomID, map.x, map.y);
			} else {
				printf("[%s]%s K:%u X:%u Y:%u\n", tag, name, kingdomID, map.x, map.y);
			}
		}
	}
	
	return;
}
*/

#define HORIZONTAL 16
#define TOTAL_ZONES 1024   // or 1024 depending on server

static int row = 0;
static int col = 0;
static unsigned int scan_count = 0;


typedef struct {
	uint8_t SoldierTableID;
	uint32_t Amount;
} NowCombatStageInfo_t;

NowCombatStageInfo_t NowCombatStageInfo[10];


#include "net_rw.h"

void Login_roleinfo_handler(Packet p) {
	printf("\n\n\n");
	char name[13];
	
	uint32_t ReadPackNum = p.read_u32();
	uint64_t UserId      = p.read_u64();
	p.read_bytes(name, 13);
	uint16_t Head = p.read_u16();
	
	RoleAttrLevelUp(&p, 27);
	// p.read_bytes(d, 27);
		
	uint64_t ServerTime = p.read_u64();
	uint64_t LogoutTime = p.read_u64();
	uint64_t Guide = (unsigned long)p.read_u32();
	uint32_t Diamond = p.read_u32();
	
	uint8_t HeroSkillPoint = p.read_u8();
	uint64_t LastHeroSPRecoverTime = p.read_u64();
	uint16_t EnhanceEventHeroID = p.read_u16();
	uint64_t HeroEnhanceEventTime_BeginTime = p.read_u64();
	uint32_t HeroEnhanceEventTime_RequireTime = p.read_u32();
	
	uint16_t StarUpEventHeroID = p.read_u16();
	uint64_t HeroStarUpEventTime_BeginTime = p.read_u64();
	uint32_t HeroStarUpEventTime_RequireTime = p.read_u32();
	
	uint8_t temp[100];
	p.read_bytes(temp, 12);
	p.read_bytes(temp, 48);
	
	uint16_t abb = p.read_u16();
	p.read_u16();
	printf("abb: %u\n", abb);
	
	printf("Head: %u\n", Head);
	uint64_t BattleID = p.read_u64();
	uint16_t newZoneID = p.read_u16();
	uint8_t newPointID = p.read_u8();
	
	bank_zoneId = newZoneID;
	bank_pointId = newPointID;
	
	map_pos_t pos = getTileMapPosbyPointCode(newZoneID, newPointID);
	printf("Here: X:%u Y:%u\n", pos.x, pos.y);
		
	printf("name: %s\n", name);
	printf("userid: %lu\n", UserId);
	printf("ServerTime: %lu\n", ServerTime);
	
	printf("Diamond: %u\n", Diamond);
	
	printf("\n");
	
	strcpy(login_name, name);
	gems = Diamond;
	
	uint64_t LastChatterTime = p.read_u64();
	uint32_t AllianceChatID  = p.read_u32();
	uint64_t Power = p.read_u64();
	uint64_t Kills = p.read_u64();
	uint32_t VipPoint = p.read_u32();
	uint64_t FirstTimer = p.read_u64();
	uint32_t PrizeFlag = p.read_u32();
	
	char PowerStr[30];
	char KillsStr[30];
				
	format_number2(Power, PowerStr, 30);
	format_number2(Kills, KillsStr, 30);
	
	printf("LastChatterTime: %lu\n", LastChatterTime);
	printf("AllianceChatID: %u\n", AllianceChatID);
	
	printf("Power: %s (%lu)\n", PowerStr, Power);
	printf("Kills: %s (%lu)\n", KillsStr, Kills);
	
	printf("VipPoint: %u\n", VipPoint);
	printf("FirstTimer: %lu\n", FirstTimer);
	printf("PrizeFlag: %u\n", PrizeFlag);
	
	uint64_t BookmarkTime = p.read_u64();
	uint16_t BookmarkLimit = p.read_u16();
	uint16_t BookmarkNum = p.read_u16();
		
	printf("BookmarkTime: %ld\n", (int64_t)BookmarkTime);
	printf("BookmarkLimit: %u\n", BookmarkLimit);
	printf("BookmarkNum: %u\n", BookmarkNum);
	
	// read UpdateCorpsStageInfo data
	p.read_u8();
	for (int i = 0; i < 10; i++) {
		NowCombatStageInfo[i].SoldierTableID = p.read_u8();
		NowCombatStageInfo[i].Amount = p.read_u32();
			
		printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, NowCombatStageInfo[i].SoldierTableID);
		printf("NowCombatStageInfo[%d].Amount: %u\n", i, NowCombatStageInfo[i].Amount);
	}
	
	uint32_t CorpsStageWallDefence = p.read_u32();
	printf("CorpsStageWallDefence: %u\n", CorpsStageWallDefence);
	
	uint16_t SuccessiveLoginDays = p.read_u16();
	printf("SuccessiveLoginDays: %u\n", SuccessiveLoginDays);
	
	uint8_t TodayUseMoraleItemTimes = p.read_u8();
	printf("TodayUseMoraleItemTimes: %u\n", TodayUseMoraleItemTimes);
	
	uint8_t LordEquipBagSize = p.read_u8();
	printf("LordEquipBagSize: %u\n", LordEquipBagSize);
	
	uint64_t NextOnlineGiftOpenTime = p.read_u64();
	printf("NextOnlineGiftOpenTime: %ld\n", (int64_t)NextOnlineGiftOpenTime);
	
	uint8_t OnlineGiftOpenTimes = p.read_u8();
	printf("OnlineGiftOpenTimes: %u\n", OnlineGiftOpenTimes);
	
	uint16_t OnlineGiftItemID_ItemID = p.read_u16();
	printf("OnlineGiftItemID_ItemID: %u\n", OnlineGiftItemID_ItemID);
	
	uint16_t OnlineGiftItemID_Quantity = p.read_u16();
	printf("OnlineGiftItemID_Quantity: %u\n", OnlineGiftItemID_Quantity);
	
	int64_t LastLordEquipUpdateTime = (int64_t)p.read_u64();
	int64_t LastItemMatUpdateTime = (int64_t)p.read_u64();
	int64_t LastItemGemUpdateTime = (int64_t)p.read_u64();
	uint16_t LordEquipEventData_ItemID = p.read_u16();
	int8_t LordEquipEventData_Color = (int8_t)p.read_u8();
	
	printf("LastLordEquipUpdateTime: %ld\n", LastLordEquipUpdateTime);
	printf("LastItemMatUpdateTime: %ld\n", LastItemMatUpdateTime);
	printf("LastItemGemUpdateTime: %ld\n", LastItemGemUpdateTime);
	printf("LordEquipEventData_ItemID: %u\n", LordEquipEventData_ItemID);
	printf("LordEquipEventData_Color: %d\n", LordEquipEventData_Color);
	
	int8_t LordEquipEventData_GemColor;
	
	for (int i = 0; i < 4; i++)
	{
		LordEquipEventData_GemColor = (int8_t)p.read_u8();
		printf("LordEquipEventData.GemColor[%d]: %d\n", i, LordEquipEventData_GemColor);
	}
	
	uint16_t LordEquipEventData_Gem;
	
	for (int j = 0; j < 4; j++) {
		LordEquipEventData_Gem = p.read_u16();
		printf("LordEquipEventData.Gem[%d]: %u\n", j, LordEquipEventData_Gem);
	}
	
	uint32_t LordEquipEventData_SerialNO = p.read_u32();
	int64_t LordEquipEventTime_BeginTime = (int64_t)p.read_u64();
	uint32_t LordEquipEventTime_RequireTime = p.read_u32();
	int8_t VipLevelUp = (int8_t)p.read_u8();
	
	printf("LordEquipEventData_SerialNO: %u\n", LordEquipEventData_SerialNO);
	printf("LordEquipEventTime_BeginTime: %ld\n", LordEquipEventTime_BeginTime);
	printf("LordEquipEventTime_RequireTime: %u\n", LordEquipEventTime_RequireTime);
	printf("VipLevelUp: %u\n", VipLevelUp);
	
	uint16_t nowKingdomID  = p.read_u16();
	uint16_t homeKingdomID = p.read_u16();
	
	current_kingdom = nowKingdomID;
	
	printf("nowKingdomID: %u\n", nowKingdomID);
	printf("homeKingdomID: %u\n", homeKingdomID);
	
	/*
	DataManager.MapDataController.updateMyKingdom(MP.ReadUShort(-1), MP.ReadUShort(-1));
	DataManager.MapDataController.updateCapitalPoint(newZoneID, newPointID, DataManager.MapDataController.OtherKingdomData.kingdomID, false);
	*/
	
	// exit(1);
}


int main(int argc, const char *argv[]) {
	/*
	while (1) {
	int zone[4];

    int base = row * HORIZONTAL + col;

    zone[0] = base;              // top-left
    zone[1] = base + 1;          // top-right
    zone[2] = base + HORIZONTAL; // bottom-left
    zone[3] = base + HORIZONTAL + 1; // bottom-right

    // RequestMapdata(&packet, 4, zone);  // fetch 4 zones at once
    
    printf("zone[0]: %u\n", zone[0]);
    printf("zone[1]: %u\n", zone[1]);
    printf("zone[2]: %u\n", zone[2]);
    printf("zone[3]: %u\n", zone[3]);
    
    
    // Move to next column
    // col++;
    col += 2;
    if (col >= HORIZONTAL - 1) {  // stop 1 before last column
        col = 0;
        row++;                     // next row
        if (row >= TOTAL_ZONES / HORIZONTAL - 1) {
            row = 0;               // wrap to first row
            scan_count++;
            
            printf("\n\n\nScan total: %u\n\n\n", scan_count);
            break;
        }
    }
    
    }
    
    return 0;
	*/
	
	
	memset(items, 0, sizeof(items));
	memset(&bank_resource, 0, sizeof(bank_resource));
	memset(&req, 0, sizeof(req));
	memset(&itemInfo, 0, sizeof(itemInfo));
	
	memset(&ct, 0, sizeof(ct));
	
	load_bank(BANK_DATA_FILE);
	
	fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	
	
	const char *login1_file = "/sdcard/lmbot/login/MSG_NEWLOGIN_LOGINTOL.bin";
	const char *login2_file = "/sdcard/lmbot/login/MSG_NEWLOGIN_LOGINTOP.bin";
	
	log_t res = bootstrap_login(login1_file);
	
	if (!res.logged) {
		fprintf(stderr, "Can't login bootstrap server!\n");
		perror("error: ");
		return 1;
	}
	
	printf ("port: %u\n", res.port);
	printf("igg id: %lu\n", res.igg_id);
	printf("ip: %s\n", res.ip);
	
	int conn = connect_server(res.ip, res.port);
	
	if (conn == -1) {
		fprintf(stderr, "Can't connect game server!\n");
		perror("error: ");
		return 1;
	}
	
	game_login(conn, login2_file);

	
	Packet packet;
	
	packet.set_socket(conn);
	
	
	const int HEARTBEAT_INTERVAL = 15; // seconds
	time_t last_ping = time(NULL);
	
	igg_id = res.igg_id;
	SendClientInitOver(&packet, res.igg_id);
	
	printf("\n\nIGG Bank ID: %lu\n\n", igg_id);
	
	uint8_t buffer[65535];
	// int recv_len = 0;
	
	uint16_t packet_size = 0;
	uint16_t packet_type = 0;
	
	char name[13] = {0};
	
	char player_name[13] = {0};
	char title_name[3] = {0};
	char str1[20] = {0};
	char str2[20] = {0};
	char message[65536] = {0};
	
	bool GetAllyPoint = false;
    
	int len = -1;
	Packet p;
	// p.set_socket(conn);
	
	// add_allowed("PORT COLD");
	bool find_ally = false;
	
	// bool send_
	uint64_t amount = 0;
	PointCode DesPoint;
	DesPoint.zoneID = 0xFFFF;
	DesPoint.pointID = 0xFF;
	// Resources resource = {0};
	ResourceType current_resource = RESOURCE_NONE;
	
	uint64_t chunk = 0;//(amount >= 2250000) ? 2250000 : amount;
	// uint64_t chunk_to_send = (uint64_t)((double)chunk / (1.0 - 0.12) + 0.5);
	// resource.food = chunk_to_send;
	
	bool is_locked = false;
	bool ally_found = false;
	
	
	
	bool is_attacking = false;
	
	bool is_for_ally_resource = false;
	bool is_for_ally_location = false;
	bool is_for_bank_location = false;
	
	int x = -1, y = -1;
	
	uint32_t buffer_offset = 0;
	uint32_t buffer_size = 0;
	uint8_t buffer_data[65535];
	uint8_t data[65535];
	//bool lock = false;
	
	uint8_t recv_buffer[MAX_BUFFER];
	size_t recv_len = 0;
	
	
	Resources resource2;
	memset(&resource2, 0, sizeof(resource2));
	
	char ally_name[13];
	char target_name[13];
	
	// SendGamblePrize(&packet);
	// SendGambleInfo(&packet);
	static time_t last_help_time = time(NULL);
	
	bool one_time = true;
	
	time_t last_update = time(NULL);
	
	RequsetYolkswitch(&packet);
	
	PointCode _DesPoint;
	_DesPoint.zoneID = 0;
    _DesPoint.pointID = 0;
    
    MarchData mdata;
	memset(&mdata, 0, sizeof(mdata));
	
	
	memset(zone, 0, sizeof(zone));
	
	
	bool again = true;
	// kingdom_scanning = true;
	uint32_t scan_count = 0;
	
	
	while (1) {
		// Send heartbeat if 15 seconds have passed
		time_t now = time(NULL);
		
		if (now - last_ping >= HEARTBEAT_INTERVAL) {
			last_ping = now;
			Heartbeat(&packet);
		}
		
		if (again && kingdom_scanning) {
			int base = row * HORIZONTAL + col;
			
			zone[0] = base;              // top-left
			zone[1] = base + 1;          // top-right
			zone[2] = base + HORIZONTAL; // bottom-left
			zone[3] = base + HORIZONTAL + 1; // bottom-right
			
			RequestMapdata(&packet, 4, zone);  // fetch 4 zones at once
			
			// printf("zone[0]: %u\n", zone[0]);
			// printf("zone[1]: %u\n", zone[1]);
			// printf("zone[2]: %u\n", zone[2]);
			// printf("zone[3]: %u\n", zone[3]);
			
			// Move to next column
			// col++;
			col += 2;
			
			if (col >= HORIZONTAL - 1) {  // stop 1 before last column
				col = 0;
				row++;                     // next row
				
				if (row >= TOTAL_ZONES / HORIZONTAL - 1) {
					row = 0;               // wrap to first row
					scan_count++;
					
					printf("\n\n\nScan total: %u\n\n\n", scan_count);
					// break;
				}
			}
			
			again = false;
		}
		
		/*
		if (again && kingdom_scanning) {
			RequestMapdata(&packet, 1, zone);
			zone[0]++;
			
			if (zone[0] >= 1024) {
				scan_count++;
				zone[0] = 0;
				printf("\n\n\nScan total: %u\n\n\n", scan_count);
			}
			
			again = false;
		}
		*/
		
		/*
		if (is_finding) {
			if (is_ready) {
				RequestMapdata(&packet, 1, zone);
				zone[0] ++;
				
				if (zone[0] >= 1023) {
					get_all_player_by_guild = false;
					is_finding = false;
					printf("done!\n");
					
					if (!is_found) {
						ServerSendChatFmt(&packet, GUILD, "Player '%s' not found", find_player);
					}
					
					zone[0] = 0;
					is_ready = false;
				}
				
				is_ready = false;
			}
		}		
		*/
		/*
		if (is_ready) {
			if (zone[0] >= 1023) {
				is_ready = false;
				zone[0] = 0;
			} else {
				RequestMapdata(&packet, 1, zone);
				zone[0]++;
				is_ready = false;
			}
		}
		*/
		
		// execution every second 
		
		if (now - last_update >= 5) {
			castle_clear();
			/*
			if (_DesPoint.zoneID != 0) {
				mdata.troops[1][0] = 1; // T1 Cavalry 
				mdata.zone_id = _DesPoint.zoneID;
				mdata.point_id = _DesPoint.pointID;
				
				RequestMarchTroop(&packet, mdata);
				printf("attacking\n");
			}
			*/
			last_update = now;
			
			// Update(1.0f);
			// printf("Stone: %.2f\n", CurrentStock);
		}
		
		if (one_time) {
			_DesPoint = getPointCodeByMapPos(268, 456);
			printf("fetching location:\n");
			one_time = false;
		}
		
		// ---- Help spam handler ----
		if (help_spam_count > 0 && now - last_help_time >= help_speed) {
			sendBuildingCancel(&packet, 0);
			// sendStartBuilding(&packet);
			// help_spam_count--;
			last_help_time = now;
		}
		
		
		if (message_spam) {
			ServerSendChat(&packet, WORLD, "Now say victory to king satan!");//"Ab bol king satan ki jay ho!");
		}
		
		// ServerSendChat(&packet, GUILD, "Ab bol king cold ki jay ho!");
		
		// DelayHandler(&packet, 1);
		
		
		if (resource_sending_active) {
			// DelayHandler(&packet, 1);
			SendResourcesHandler(&packet);
		}
		
		len = recv(conn, buffer + recv_len, sizeof(recv_buffer) - recv_len, MSG_DONTWAIT);
		
		if (len == 0) {
			printf("Connection closed!\n");
			break;
		}
		
		if (len < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(10000); // no data yet, avoid busy loop
				continue;
			} else {
				perror("recv");
				break;
			}
		}
		
		recv_len += len;
		//printf("data received: %d (buffer now %zu)\n", len, recv_len);
		
		size_t offset = 0;
		
		while (recv_len - offset >= 2) {
			uint16_t packet_size;
			memcpy(&packet_size, buffer + offset, 2);
			
			if (recv_len - offset < packet_size) {
				// not a full packet yet
				break;
			}
		
			uint16_t packet_type;
			memcpy(&packet_type, buffer + offset + 2, 2);
			
			#ifdef DEBUG
				printf("PACKET TYPE: %s\n", get_packet_name(packet_type));
				// printf("PACKET SIZE: %u\n", packet_size);
			#endif
			
			p.reset();
			memcpy(p.read_buffer, buffer + offset + 4, packet_size - 4);
			// p.write_bytes(buffer + offset + 4, packet_size - 4);
			p.rewind();
			
			if (packet_type == _MSG_RESP_ACTIVE) {
				server_time = p.read_u64();
				// printf("SERVER TIME: %s\n", format_timestamp(server_time));
				
				
				// sendStartBuilding(&packet, 0, 0);
				// SendSomeBody(&packet);
			} else if (packet_type == _MSG_RESP_CHATMESSAGE) {
				// Get chat message 
				ChatMessage res = RecvChatMessage(&p);
				
				Command cmd = is_command(res.message);
				
				if (cmd.cmd != CMD_NONE) {
					command_handler(&packet, res.player_name, cmd, res.message);
				} else {
					// normal message 
					printf("player name: %s\n", res.player_name);
					printf("message: %s\n", res.message);
				}
			} else if (packet_type == _MSG_RESP_USEITEM) {
				RecvUseItem(p, &packet);
				// dump_data("MSG_RESP_USEITEM.bin", "", buffer + offset + 4, packet_size - 4);
			} else if (packet_type == _MSG_RESP_ITEMINFO) {
				memset(&items, 0, sizeof(items));
				
				eMsgState RecvItemState = (eMsgState)p.read_u8();
				
				uint16_t counts = p.read_u16();
				
				for (int i = 0; i < counts; i++) {
					uint16_t item_id = p.read_u16();
					uint16_t item_quantity = p.read_u16();
					
					items[item_id].quantity = item_quantity;
					// printf("item id: %u\n", item_id);
					// printf("quantity: %u\n", item_quantity);
					
				}
				
				uint64_t bagFood = 0;//GetBagFood();
				char bfood[25];
				format_number2(bagFood, bfood, 25);
				printf("Bag Food: %s\n", bfood);
				
				// printf("150K FOOD: %u\n", items[0x03F6].quantity);
			} else if (packet_type == _MSG_RESP_RESOURCEINFO) {
				bank_resource.food = p.read_u32();
				uint64_t a2 = p.read_u64();
				
				bank_resource.rock = p.read_u32();
				uint64_t b2 = p.read_u64();
				
				bank_resource.wood = p.read_u32();
				uint64_t c2 = p.read_u64();
				
				bank_resource.ore = p.read_u32();
				uint64_t d2 = p.read_u64();
				
				bank_resource.gold = p.read_u32();
				uint64_t e2 = p.read_u64();
				
				// Current stock, Speed, Max Capacity 
				SetResource(bank_resource.rock, b2, 2000000); // start with 1000, speed 652492/h, cap 4B
				
				printf("food = %u\n", bank_resource.food);
				printf("rock = %u\n", bank_resource.rock);
				printf("wood = %u\n", bank_resource.wood);
				printf("ore  = %u\n", bank_resource.ore);
				printf("gold = %u\n", bank_resource.gold);
					
				
				printf("Food Production: %lu\n", a2);
				printf("Stone Production: %lu\n", b2);
				printf("Wood Production: %lu\n", c2);
				printf("Ore Production: %lu\n", d2);
				printf("Gold Production: %lu\n", e2);
				
			//	exit (0);
			} else if (packet_type == _MSG_RESP_ALLYPOINT) {
				uint8_t b = p.read_u8();
				uint16_t zoneID = p.read_u16();
				uint8_t pointID = p.read_u8();
				
				if (b == 0) {
					map_pos_t map1 = getTileMapPosbyPointCode(bank_zoneId, bank_pointId);
					map_pos_t map2 = getTileMapPosbyPointCode(zoneID, pointID);
					
					int dist = distance(map1.x, map1.y, map2.x, map2.y);
					
					if (dist > MAX_BANK_RANGE) {
						SendMailFmt(&packet, req.player_name, "", "%s is %dm away (max %d). Can't deliver.", req.target_name, dist, MAX_BANK_RANGE);
						/*
						ServerSendChatFmt(&packet, GUILD, 
							"%s is %dm away (max %d). Can't deliver.", 
							req.target_name, dist, MAX_BANK_RANGE
						);*/
					} else {
						req.zoneId = zoneID;
						req.pointId = pointID;
						resource_sending_active = true;
						printf("Player found!\n");
						printf("\n\n\n\ndistance: %d\n\n\n\n", dist);
					}
				} else {
					// SendMailFmt(packet, req.player_name, "", "Can't find ally location!");
					memset(&req, 0, sizeof(req));
					resource_sending_active = false;
					printf("Player not found!\n");
				}
			} else if (packet_type == _MSG_RESP_SEND_RESHELP) {
				uint8_t b = p.read_u8();
				
				if (b == 0) {
					uint8_t b2 = p.read_u8();
					
					if (b2 >= 8) {
						printf("b2 exit!\n");
						return 0;
					}
					
					uint16_t zoneId = p.read_u16();
					uint8_t pointId = p.read_u8();
					
					uint64_t BeginTime =  p.read_u64();
					uint32_t RequireTime = p.read_u32();
					
					uint32_t food = p.read_u32();
					uint32_t rock = p.read_u32();
					uint32_t wood = p.read_u32();
					uint32_t ore = p.read_u32();
					uint32_t gold = p.read_u32();
					
					char _food_str[20] = {0};
					char _rock_str[20] = {0};
					char _wood_str[20] = {0};
					char  _ore_str[20] = {0};
					char _gold_str[20] = {0};
					
					
					// bank_resource.food = food;
					// bank_resource.rock = rock;
					// bank_resource.wood = wood;
					// bank_resource.ore = ore;
					// bank_resource.gold = gold;
					
					
					format_number2(food, _food_str, 20);
					format_number2(rock, _rock_str, 20);
					format_number2(wood, _wood_str, 20);
					format_number2(ore, _ore_str, 20);
					format_number2(gold, _gold_str, 20);
						
					
					printf("zoneId = %u\n", zoneId);
					printf("pointId = %u\n", pointId);
					
					printf("BeginTime = %lu\n", BeginTime);
					printf("RequireTime = %u\n", RequireTime);
					
					printf("food = %s\n", _food_str);
					printf("rock = %s\n", _rock_str);
					printf("wood = %s\n", _wood_str);
					printf("ore  = %s\n", _ore_str);
					printf("gold = %s\n", _gold_str);
					
					printf("\n\n");
					
					// printf("amount: %lu\n", amount);
					//if (req.remains_amount > 0) {
						chunk = (req.remains_amount >= 3260870) ? 3260870 : req.remains_amount;
						req.remains_amount -= chunk;
						
						
						
						
						if (req.resource_type == RESOURCE_FOOD) {
							bank_resource.food -= chunk;
							//update_balance(req.player_name, chunk, 0, 0, 0, 0, BAL_DEBIT); // decrease 
						} else if (req.resource_type == RESOURCE_ROCK) {
							bank_resource.rock -= chunk;
							//update_balance(req.player_name, 0, chunk, 0, 0, 0, BAL_DEBIT); // decrease 
						} else if (req.resource_type == RESOURCE_WOOD) {
							bank_resource.wood -= chunk;
							//update_balance(req.player_name, 0, 0, chunk, 0, 0, BAL_DEBIT); // decrease 
						} else if (req.resource_type == RESOURCE_ORE) {
							bank_resource.ore -= chunk;
							//update_balance(req.player_name, 0, 0, 0, chunk, 0, BAL_DEBIT); // decrease 
						} else if (req.resource_type == RESOURCE_GOLD) {
							//update_balance(req.player_name, 0, 0, 0, 0, chunk, BAL_DEBIT); // decrease 
							bank_resource.gold -= chunk;
						}
						
						ResHelp r = make_resource(req.resource_type, chunk);
						
						if (req.resource_type != RESOURCE_NONE) {
							update_balance(req.player_name, r, BAL_DEBIT);
						}
						
						
						
				//	}
					
					printf("chunk2: %lu\n", chunk);
					printf("remaining amount: %lu\n", req.remains_amount);
					
					if (req.remains_amount == 0) {
						ResourceRequest req2 = req;
						memset(&req, 0, sizeof(req));
						resource_sending_active = false;
						
						// SendMailFmt(&packet, req.player_name, "Complete", "Sending %s done!", resource_to_string(req.resource_type));
						
						// ServerSendChat(&packet, GUILD, "Process completed!");
						
						chunk = 0;
						printf("\n\nSending complete\n\n");
						
						
						map_pos_t res = getTileMapPosbyPointCode(req2.zoneId, req2.pointId);
						
						
						char sent_amount[20];
						char rss_type[30];
						
						format_number2(req2.total_amount, sent_amount, 20);
						GetResourceType(req2.resource_type, rss_type);
						
						printf("\n\n\nTotal Amount: %lu\n", req2.total_amount);
						printf("Tax amount: %u\n\n\n", req2.tax_amount);
						
						const char *name = (req2.target_name[0] == '\0') ? req2.player_name : req2.target_name;
						
						SendMailFmt(&packet, req2.player_name, "Complete", 
							"Completed → Target: %s | Type: %s | Sent: %s",
							name,
							rss_type,
							sent_amount
						);
					}
					
					req.is_locked = false;
					
					
					CurrentMarch++;
				} else if (b == 1) {
					// Army limit reached 
					troops_in_march = true;
					printf("troops_in_march\n");
				} else {
					memset(&req, 0, sizeof(req));
					resource_sending_active = false;
					//ServerSendChatFmt(&packet, GUILD, "Can't send resource to %s", req.player_name);
				}
			} else if (packet_type == _MSG_RESP_UPDATE_RESOURCEAMOUNT) {
				bank_resource.food = p.read_u32();
				bank_resource.rock = p.read_u32();
				bank_resource.wood = p.read_u32();
				bank_resource.ore = p.read_u32();
				bank_resource.gold = p.read_u32();
			} else if (packet_type == _MSG_RESP_RESHELPREPORTINFO) {
				ResourceHelpReportInfo(&p); // Received or Send rss to player
			} else if (packet_type == _MSG_RESP_RESHELP_HOME) {
				uint8_t b = p.read_u8();
				
				if (b >= 8) {
					// return 0;
					// return;
				}
				
				
				
				if (b < 8) {
					bank_resource.food = p.read_u32();
					bank_resource.rock = p.read_u32();
					bank_resource.wood = p.read_u32();
					bank_resource.ore  = p.read_u32();
					bank_resource.gold = p.read_u32();
					
					req.is_locked = false;
					CurrentMarch--;
				}
				
				
			} else if (packet_type == _MSG_ROLE_UPDATEINFO) {
				uint8_t b = p.read_u8();
				
				if (b == 3) {
					uint16_t num = p.read_u16();
					uint16_t itemID = p.read_u16();
					long num2 = p.read_u64();
					uint32_t num3 = p.read_u32();
					
					printf("num: %u\n", num);
					printf("itemID: %u\n", itemID);
					printf("num2: %lu\n", num2);
					printf("num3: %u\n", num3);
				} else {
					printf("\n\nb: %u\n\n", b);
				}
				
				// if (b == 4) {
				
			} else if (packet_type == _MSG_MARCH_MARCHEVENTDATA) {
				MaxMarchEventNum = p.read_u8();
				CurrentMarch = p.read_u8();
				/*
				uint8_t MaxMarchEventNum = p.read_u8();
				uint8_t b = p.read_u8();
				
				printf("\n\n\n");
				printf("MaxMarchEventNum: %u\n", MaxMarchEventNum);
				printf("b: %u\n", b);
				printf("\n\n\n");
				*/
			} else if (packet_type == _MSG_RESP_BUFFINFO) {
				uint8_t b = p.read_u8();
				
				printf("buff info b: %u\n", b);
				for (int i = 0; i < (int)b; i++) {
					uint16_t num = p.read_u16(); // how many shield
					uint16_t itemID = p.read_u16(); // id
					
					uint64_t num2 = p.read_u64(); // Unix timestamp 
					uint32_t num3 = p.read_u32(); // Duration
					
					
					itemInfo[i].quantity = num;
					itemInfo[i].item_id = itemID;
					itemInfo[i].begin = num2;
					itemInfo[i].duration = num3;
					
					/*
					printf("num: %u\n", num);
					printf("itemID: %u\n", itemID);
					printf("num2: %lu\n", num2);
					printf("num3: %u\n", num3);
					
					printf("stime: %lu\n", server_time);
					*/
					/*
					uint64_t a = server_time - num2;
					
					printf("shield: %s (%lu)\n", FormatTime(a), a);
					*/
					uint64_t end_time  = num2 + num3;
					uint64_t remaining = (end_time > server_time) ? (end_time - server_time) : 0;
					printf("shield: %s (%lu)\n", FormatTime(remaining), remaining);
					printf("itemID: %u\n", itemID);
					printf("num: %u\n", num);
					
				}
				
				
				printf("\n\n\n\n");
			} else if (packet_type == _MSG_RESP_BUILDBEGIN) {
				if (help_spam_count == 0) {
					//ServerSendChat(&packet, GUILD, "Help spam finished.");
					sendBuildingCancel(&packet, 0);
				} else {
					SendSomeBody(&packet);
				}
			} else if (packet_type == _MSG_RESP_BUILDINGERROR) {
				//ServerSendChat(&packet, GUILD, "Building error!");
			} else if (packet_type == 0x0b25) {
				RecvAllianceHelp(p);
				
				if (help_spam_count == 0) {
					//ServerSendChat(&packet, GUILD, "Help spam finished..");
					sendBuildingCancel(&packet, 0);
				} else {
					help_spam_count--;
				}
			} else if (packet_type == _MSG_RESP_BUILDCANCEL) {
				if (help_spam_count == 0) {
					//ServerSendChat(&packet, GUILD, "Help spam finished...");
					sendBuildingCancel(&packet, 0);
				} else {
					sendStartBuilding(&packet, 0,0);
				}
			} else if (packet_type == _MSG_LOGIN_ROLEINFO) {
				Login_roleinfo_handler(p);
				
				/*
				printf("\n\n\n");
				char name[13];
				uint32_t ReadPackNum = p.read_u32();
				uint64_t UserId = p.read_u64();
				p.read_bytes(name, 13);
				uint16_t Head = p.read_u16();
				
				RoleAttrLevelUp(&p, 27);
				// p.read_bytes(d, 27);
				
				uint64_t ServerTime = p.read_u64();
				uint64_t LogoutTime = p.read_u64();
				uint64_t Guide = (unsigned long)p.read_u32();
				uint32_t Diamond = p.read_u32();
				
				uint8_t HeroSkillPoint = p.read_u8();
				uint64_t LastHeroSPRecoverTime = p.read_u64();
				uint16_t EnhanceEventHeroID = p.read_u16();
				uint64_t HeroEnhanceEventTime_BeginTime = p.read_u64();
				uint32_t HeroEnhanceEventTime_RequireTime = p.read_u32();
				
				uint16_t StarUpEventHeroID = p.read_u16();
				uint64_t HeroStarUpEventTime_BeginTime = p.read_u64();
				uint32_t HeroStarUpEventTime_RequireTime = p.read_u32();
				
				uint8_t temp[100];
				p.read_bytes(temp, 12);
				p.read_bytes(temp, 48);
				
				uint16_t abb = p.read_u16();
				p.read_u16();
				printf("abb: %u\n", abb);
				
				printf("Head: %u\n", Head);
				uint64_t BattleID = p.read_u64();
				uint16_t newZoneID = p.read_u16();
				uint8_t newPointID = p.read_u8();
				
				bank_zoneId = newZoneID;
				bank_pointId = newPointID;
				
				map_pos_t pos = getTileMapPosbyPointCode(newZoneID, newPointID);
				printf("Here: X:%u Y:%u\n", pos.x, pos.y);
				
				
				printf("name: %s\n", name);
				printf("userid: %lu\n", UserId);
				printf("ServerTime: %lu\n", ServerTime);
				
				printf("Diamond: %u\n", Diamond);
				
				printf("\n");
				
				strcpy(login_name, name);
				gems = Diamond;
				
				uint64_t LastChatterTime = p.read_u64();
				uint32_t AllianceChatID  = p.read_u32();
				uint64_t Power = p.read_u64();
				uint64_t Kills = p.read_u64();
				uint32_t VipPoint = p.read_u32();
				uint64_t FirstTimer = p.read_u64();
				uint32_t PrizeFlag = p.read_u32();
				
				
				char PowerStr[30];
				char KillsStr[30];
				
						
				format_number2(Power, PowerStr, 30);
				format_number2(Kills, KillsStr, 30);
				
				printf("LastChatterTime: %lu\n", LastChatterTime);
				printf("AllianceChatID: %u\n", AllianceChatID);
				
				printf("Power: %s (%lu)\n", PowerStr, Power);
				printf("Kills: %s (%lu)\n", KillsStr, Kills);
				
				printf("VipPoint: %u\n", VipPoint);
				printf("FirstTimer: %lu\n", FirstTimer);
				printf("PrizeFlag: %u\n", PrizeFlag);
				
				
				uint64_t BookmarkTime = p.read_u64();
				uint16_t BookmarkLimit = p.read_u16();
				uint16_t BookmarkNum = p.read_u16();
				
				printf("BookmarkTime: %ld\n", (int64_t)BookmarkTime);
				printf("BookmarkLimit: %u\n", BookmarkLimit);
				printf("BookmarkNum: %u\n", BookmarkNum);
				
				// read UpdateCorpsStageInfo data
				p.read_u8();
				for (int i = 0; i < 10; i++) {
					NowCombatStageInfo[i].SoldierTableID = p.read_u8();
					NowCombatStageInfo[i].Amount = p.read_u32();
					
					printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, NowCombatStageInfo[i].SoldierTableID);
					printf("NowCombatStageInfo[%d].Amount: %u\n", i, NowCombatStageInfo[i].Amount);
				}
				
				uint32_t CorpsStageWallDefence = p.read_u32();
				printf("CorpsStageWallDefence: %u\n", CorpsStageWallDefence);
				
				uint16_t SuccessiveLoginDays = p.read_u16();
				printf("SuccessiveLoginDays: %u\n", SuccessiveLoginDays);
				
				uint8_t TodayUseMoraleItemTimes = p.read_u8();
				printf("TodayUseMoraleItemTimes: %u\n", TodayUseMoraleItemTimes);
				
				uint8_t LordEquipBagSize = p.read_u8();
				printf("LordEquipBagSize: %u\n", LordEquipBagSize);
				
				uint64_t NextOnlineGiftOpenTime = p.read_u64();
				printf("NextOnlineGiftOpenTime: %ld\n", (int64_t)NextOnlineGiftOpenTime);
				
				uint8_t OnlineGiftOpenTimes = p.read_u8();
				printf("OnlineGiftOpenTimes: %u\n", OnlineGiftOpenTimes);
				
				uint16_t OnlineGiftItemID_ItemID = p.read_u16();
				printf("OnlineGiftItemID_ItemID: %u\n", OnlineGiftItemID_ItemID);
				
				uint16_t OnlineGiftItemID_Quantity = p.read_u16();
				printf("OnlineGiftItemID_Quantity: %u\n", OnlineGiftItemID_Quantity);
				
				
				return 0;
				*/
			} else if (packet_type == _MSG_RESP_MAILINFO) {
				RecvMailInfo(&p, &packet);
			} else if (packet_type == _MSG_RESP_SELLITEM) {
				RecvSellItem(&p);
			} else if (packet_type == _MSG_RESP_TREASURE_INFO) {
				RecvMall_Info(&p);
				close(conn);
				break;
			} else if (packet_type == _MSG_RESP_SENDSCOUT) {
				// ServerSendChat(&packet, GUILD, "Scouting...");
				printf("Scouting...\n");
			} else if (packet_type == _MSG_RESP_SCOUTREPORTINFO) {
				HandleScoutReportInfo(&packet, &p);
			} else if (packet_type == _MSG_RESP_ANTISCOUTREPORTINFO) {
				printf("_MSG_RESP_ANTISCOUTREPORTINFO\n");
			} else if (packet_type == _MSG_RESP_WONDER_SWITCH) {
				printf("_MSG_RESP_WONDER_SWITCH\n");
				
				// dump_data("MSG_RESP_WONDER_SWITCH", "", buffer + offset + 4, packet_size - 4);
				
				//uint8_t b2 = p.read_u8();
				//printf("b2: %u\n", b2);
				
				
				
				// exit(1);
			} else if (packet_type == _MSG_RESP_TROOPMARCH) {
				printf("_MSG_RESP_TROOPMARCH: ");
				uint8_t b = p.read_u8();
				
				printf("%u", b);
				
				
				printf("\n");
			} else if (packet_type == _MSG_RESP_UPDATE_MAPINFO_PLUS) {
				// dump_data("mapdata.bin", "", buffer + offset + 4, packet_size - 4);
				
				
				uint8_t *data = buffer + offset + 4;
				uint32_t size = packet_size - 4;
				
				RecvMapData(&packet, data, size);
				
				again = true;
				
				
				
				
				// dump_data("mapdata.bin", "", buffer + offset + 4, packet_size - 4);
				//exit(0);
				/*
				dump_data("mapdata.bin", "", buffer + offset + 4, packet_size - 4);
				
				RecvMapData(buffer + offset + 4, packet_size - 4);
				
				
				printf("\n\n\n\n");
				
				// RecvMapInfoPlus(&p, &packet);
				// printf("MSG_RESP_UPDATE_MAPINFO_PLUS: %u\n", packet_size);
				
				//exit(0);
				*/
				
				
				
				/*
				is_ready = true;
				
				uint16_t kingdomID;
				uint16_t zoneId;
				uint8_t pointId;
				
				uint8_t kind;
				char name[13];
				char tag[3];
				
				for (uint32_t i = 0; i < size; i++) {
					if (memcmp(data + i, find_player, find_player_len) == 0) {
						is_found = true;
						is_finding = false;
						is_ready = false;
						zone[0] = 0;
						
						memcpy(name, data + i, 13);
						memcpy(tag,  data + i + 13, 3);
						memcpy(&kingdomID, data + i + 16, 2);
						
						i -= 4;
						
						memcpy(&zoneId,  data + i, 2);
						i += 2;
						memcpy(&pointId, data + i, 1);
						i += 1;
						memcpy(&kind, data + i, 1);
						i += 1;
						
						if (kind == PK_CITY) {
							map_pos_t map = getTileMapPosbyPointCode(zoneId, pointId);
							
							// [OD1]PORT COLD K:1745 X:320 Y:514
							if (tag[0] == '\0') {
								ServerSendChatFmt(&packet, GUILD, "%s K:%u X:%u Y:%u", name, kingdomID, map.x, map.y);
							} else {
								ServerSendChatFmt(&packet, GUILD, "[%s]%s K:%u X:%u Y:%u", tag, name, kingdomID, map.x, map.y);
							}
							
							break;
						}
						
						
					}
					
					if (zone[0] >= 1023) {
						is_ready = false;
					}
				} 
				
				*/
				
				
			} else if (packet_type == _MSG_MAP_MY_KINGDOMINFO) {
			//	dump_data("_MSG_MAP_MY_KINGDOMINFO.bin", "", buffer + offset + 2, packet_size - 2);
			}
			
			
            // process payload here...
            // dump_data("log.bin", "", buffer + offset + 2, packet_size - 2);

            offset += packet_size;
        }

        if (offset > 0) {
            memmove(buffer, buffer + offset, recv_len - offset);
            recv_len -= offset;
        }
        
	}

	close(conn);
	close(fd);
	
	return EXIT_SUCCESS;
}
