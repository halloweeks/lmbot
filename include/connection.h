#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
  #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0601
  #endif
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  typedef unsigned int uint;
  #define close_socket closesocket
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <fcntl.h>
  #define close_socket close
#endif

#include <string.h>
#include <stdio.h>

#include "des.h"

typedef enum {
	EMS_Null,
	EMS_Begin,
	EMS_End,
	EMS_BeginAndEnd
} eMsgState;

typedef struct {
	uint16_t packet_size;
	uint16_t packet_type;
    uint8_t buffer[4096];
    size_t read_pos;
    size_t parse_pos;
} PacketStream;

typedef struct {
	uint8_t data[4096];
	uint16_t size;
	uint16_t offset;
} PacketBuffer;

/*
typedef struct {
	uint8_t data[4096];
	uint16_t size;
	uint16_t offset;
} Stream;*/


#define MAX_ITEM_COUNT 65536

typedef struct {
    uint32_t food;
    uint32_t rock;
    uint32_t wood;
    uint32_t ore;
    uint32_t gold;
} ResourceStock;

typedef struct {
    int64_t food;
    int64_t rock;
    int64_t wood;
    int64_t ore;
    int64_t gold;
} ResourceProduction;

typedef enum {
    RESOURCE_FOOD,
    RESOURCE_ROCK,
    RESOURCE_WOOD,
    RESOURCE_ORE,
    RESOURCE_GOLD
} ResourceType;

typedef struct {
	uint16_t quantity;
} Item;

typedef struct {
    uint16_t item_id;
    uint16_t item_count;
    uint8_t resource_kind;
    uint32_t resource_count;
    uint8_t rare;
} MarketItem;

typedef struct {
	bool speed_up;
    bool speed_up_research;
    bool speed_up_merging;
    bool speed_up_training;
    
    bool bright_talent_orb;
} BlackMarketBuy;

typedef struct {
	bool auto_trade;
	bool use_bag_rss;
    bool spend_food;
    bool spend_rock;
    bool spend_wood;
    bool spend_ore;
    bool spend_gold;
} MarketSettings;

typedef struct {
	bool loaded;
	bool buy_pending;
	uint8_t trade_locks;
	uint8_t trade_status;
	uint64_t refresh_time;
	MarketItem items[4];
	ResourceStock reserve;
	MarketSettings settings;
} BlackMarket;

typedef struct {
	char name[13];
	uint64_t power;
	uint64_t kills;
	uint32_t gems;
	uint32_t vip_point;
	uint16_t current_kingdom_id;
	uint16_t home_kingdom_id;
	uint16_t zone_id;
	uint8_t point_id;
	uint8_t max_marches;
	uint8_t current_marches;
} PlayerInfo;

// optional 
typedef struct {
    ResourceStock current;
    ResourceStock bag;

    bool loaded;
} ResourceInfo;

typedef struct {
    char addr[16];
    uint32_t port;
} ServerInfo;

typedef struct {
    uint8_t  version_major;
    uint8_t  version_minor;
    uint16_t version_patch;
    uint8_t  language_code;
} AppInfo;

typedef struct {
    int64_t igg_id;
    char device_uuid[50];
    uint16_t session_len;
    char session[512];
} AuthInfo;

typedef struct {
    uint32_t seq_id;
    uint32_t guest_seq_id;
} ProtocolState;

typedef struct {
    char player_name[13];
    char message[1024];
    bool pending;
} ChatState;

/*
typedef enum {
    TRANSFER_IDLE,
    TRANSFER_FIND_TARGET,
    TRANSFER_WAIT_TARGET,
    TRANSFER_SEND_MARCH,
    TRANSFER_WAIT_MARCH,
    TRANSFER_COMPLETE,
    TRANSFER_FAILED
} TransferState;


typedef struct {
	bool active;
	TransferState state;
	char request_name[13];
	char target_name[13];
	
	ResourceType resource;
	
	uint32_t total_amount;
	uint32_t remaining_amount;
	
	uint16_t zone_id;
	uint8_t point_id;
	
	uint32_t current_chunk;
} ResourceTransfer;
*/

typedef struct {
    uint32_t serial_id;

    uint64_t send_time;

    uint8_t mail_type;
    uint32_t reply_id;

    uint16_t sender_head;
    uint16_t sender_kingdom;

    char sender_tag[4];
    char sender_name[14];

    uint8_t extra_flag;

    char title[256];
    char content[4096];

    uint8_t attachment_count;

} MailInfo;

typedef enum {
    Research = 0,
    Building,
    Max,
} HelpKind;

typedef struct {
    uint32_t record_sn;
    uint16_t head;
    uint8_t rank;
    char player_name[14];

    HelpKind help_kind;

    uint16_t event_id;
    uint8_t event_data_lv;

    uint8_t already_helped;
    uint8_t help_max;
    
    uint32_t record_sn_arr[255];
} AllianceHelp;

typedef struct {
	uint16_t position_id;
    uint16_t build_id;
    uint8_t level;
} BuildingInfo;

/*
typedef struct {
    uint32_t serial_id;

    uint8_t status;
    uint64_t receive_time;

    uint16_t box_item_id;
    uint16_t item_id;
    uint16_t quantity;

    uint8_t item_rank;

    char sender_name[13];
} AllianceGift;
*/


typedef struct {
    uint32_t sn;
    uint8_t status;
    int64_t rcv_time;
    uint16_t box_item_id;
    uint16_t item_id;
    uint16_t num;
    uint8_t item_rank;
    // Option
    uint32_t diamond;
    uint32_t money;
    char player[13];
} AllianceGift;


#define GIFT_TABLE_SIZE 8192

typedef enum {
    GIFT_EMPTY   = 0,
    GIFT_USED    = 1,
    GIFT_DELETED = 2
} GiftSlotState;

typedef struct {
	bool loaded;
    uint16_t count;
    uint16_t index;
    bool opening;
    AllianceGift gifts[GIFT_TABLE_SIZE];
} AllianceGiftList;


typedef enum {
    GIFT_STATE_IDLE,      // No gift data loaded yet.
    GIFT_STATE_LOADING,   // Waiting for RequestAllianceGiftInfo() response.
    GIFT_STATE_READY,     // Ready to process gifts.
    GIFT_STATE_OPENING,   // Waiting for open gift response.
    GIFT_STATE_DELETING   // Waiting for delete gift response.
} GiftState;

typedef struct {
    bool auto_help;
    bool auto_open_gifts;
    uint16_t gift_count;
    uint16_t gift_offset;
    
    uint16_t unopened_gift_count;
    
    GiftState gift_state;
    uint16_t recv_index;
    AllianceGift gifts[300];
} AllianceSettings;


#define MAX_SMART_USE_ITEMS 50

typedef struct {
    uint16_t id;
    uint16_t qty;
} SmartUseItem;

typedef struct {
    uint16_t count;
    SmartUseItem items[MAX_SMART_USE_ITEMS];
} SmartUseList;

typedef enum
{
	// Token: 0x04000893 RID: 2195
	NONE,
	// Token: 0x04000894 RID: 2196
	RANK1,
	// Token: 0x04000895 RID: 2197
	RANK2,
	// Token: 0x04000896 RID: 2198
	RANK3,
	// Token: 0x04000897 RID: 2199
	RANK4,
	// Token: 0x04000898 RID: 2200
	RANK5,
	// Token: 0x04000899 RID: 2201
	RANKMAX = 5
} AllianceRank;


typedef struct {
	uint32_t Channel;
    AllianceRank Rank;
    uint8_t Apply;
    uint32_t Money;
} AllianceInfo;

typedef enum {
    HELP_SPAM_IDLE,
    HELP_SPAM_START_BUILD,
    HELP_SPAM_WAIT_BUILD,
    HELP_SPAM_WAIT_HELP,
    HELP_SPAM_CANCEL_BUILD
} HelpSpamState;

typedef enum {
    BUILD_TIMBER       = 1,
    BUILD_STONE        = 2,
    BUILD_ORE          = 3,
    BUILD_FOOD         = 4,
    BUILD_MANOR        = 5,
    BUILD_BARRACKS     = 6,
    BUILD_INFIRMARY    = 7,
    BUILD_CASTLE       = 8,
    BUILD_VAULT        = 9,
    BUILD_ACADEMY      = 10,
    BUILD_WALL         = 12,
    BUILD_WATCHTOWER   = 13,
    BUILD_EMBASSY      = 14,
    BUILD_WORKSHOP     = 15,
    BUILD_TRADING_POST = 17
} BUILDING_ID;


typedef struct {
    bool active;

    HelpSpamState state;

    uint16_t remaining_count;
    uint8_t speed;

    time_t last_action;
    
    uint16_t building_id;
    uint16_t building_pos;
    uint8_t building_level;
} HelpSpam;

typedef struct {
	bool enabled;
	
	bool send_food;
	bool send_rock;
	bool send_wood;
	bool send_ore;
	bool send_gold;
	
	ResourceStock reserve;
	uint32_t max_delivery_distance;
	
	bool use_bag_rss;
	bool use_bag_food;
	bool use_bag_rock;
	bool use_bag_wood;
	bool use_bag_ore;
	bool use_bag_gold;
} BankSettings;

typedef enum {
    COMMAND_CHANNEL_WORLD,
    COMMAND_CHANNEL_GUILD,
    COMMAND_CHANNEL_MAIL
} CommandChannel;

typedef struct {
    char admin_name[13];
    char command_prefix;
    bool admin_only;
    char data_path[256];
    
    CommandChannel command_input;
    CommandChannel command_output;
} BotSettings;

typedef struct {
	char player_name[13];
	ResourceStock resource;
} PlayerBank;

/*
typedef struct {
	bool enabled;
	bool always_on;
	bool on_attack;
	bool on_scout;
	
	uint16_t priority[8];
	uint8_t priority_count;
} ShieldSettings;
*/

/*
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

typedef struct {
    uint32_t line_id;
    uint8_t line_type;
    uint64_t begin_time;
    uint32_t require_time;
    EWATCHTOWER_LINE_TARGET target;
} WatchTowerEvent;
*/

typedef struct {
    bool active;
    bool loaded; // Have we received the buff list yet?
    bool pending;
    uint16_t item_id;
    uint16_t quantity;
    uint64_t begin_time;
    uint32_t duration;
} ShieldInfo;

typedef enum {
    HYPER_STATE_IDLE = 0,          // Nothing to do.
    HYPER_STATE_FIND_TARGET,       // Waiting for ally location.
    HYPER_STATE_READY,             // Target found, ready to transfer.
    HYPER_STATE_SENDING,           // Sending one or more marches.
    HYPER_STATE_WAIT_RETURN,       // Waiting for marches to return.
} HyperState;

typedef struct {
	bool enabled;
	char target_name[13];
	uint16_t max_transfer_distance;
	
	HyperState state;
    bool pending;              // Waiting for server response.

	struct {
		uint32_t send_at;
		uint32_t stop_at;
	} food, rock, wood, ore, gold;
	
	uint16_t zone_id;
	uint8_t point_id;
	
} HyperSettings;

typedef struct {
	bool loaded;
	uint32_t total;
	uint32_t infantry[4];
	uint32_t cavalry[4];
	uint32_t ranged[4];
	uint32_t siege[4];
	uint32_t t5_data[4];
} TroopData;

typedef struct {
	bool loaded;
	TroopData troop;
	TroopData healing;
	long num;
	uint total_time;
} WoundedTroopData;



typedef struct {
    bool pending;
    int64_t execute_time;
    char leader[13];
    uint8_t level;
} PendingRally;


typedef struct {
    // Master switch for all protection features.
    bool enabled;

    /* Shield */

    // Keep a shield active continuously (24/7).
    bool shield_always_on;

    // Use a shield when an incoming attack is detected.
    bool shield_on_incoming_attack;

    // Use a shield when an incoming scout is detected.
    bool shield_on_incoming_scout;

    // Shield item preference.
    uint8_t shield_priority_count;
    uint16_t shield_priority[8];

    /* Troop recall */

    // Recall troops when an incoming attack is detected.
    bool recall_on_incoming_attack;

    // Recall troops when an incoming scout is detected.
    bool recall_on_incoming_scout;
    
    bool recall_on_incoming_conflict;
} ProtectionSettings;

/*
typedef enum {
    T1 = 0,
    T2,
    T3,
    T4,
    T5
} TroopTier;
*/

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

typedef enum {
    DARKNEST_FORMATION_FIXED,
    DARKNEST_FORMATION_LEADER
} DarknestFormationMode;

typedef struct {
    // Enable automatic Darknest rally joining.
    bool enabled;
    bool auto_join;
    
    // Join only Darknest levels within this range.
    uint8_t min_level;
    uint8_t max_level;
    uint8_t max_march;
    
    uint16_t max_distance;
    bool auto_transmute;
    uint8_t essence_level;

    // Total troops to send.
    uint32_t troop_count;
    uint32_t min_join_troops;
    
    // Formation (e.g. 8480 = 80% Infantry, 40% Ranged, 80% Cavalry).
    uint16_t formation;
    // Formation selection mode.
    DarknestFormationMode formation_mode;
    
    uint8_t tier_priority_count;
    // Tier consumption order.
    TroopTier tier_order[5];

    // Random join delay (seconds).
    uint16_t min_join_delay;
    uint16_t max_join_delay;
    
    // Reserve marches for other activities.
    uint8_t max_marches_to_use;

    // Don't join if troop count falls below this percentage.
    uint8_t min_troop_percent;
} DarknestSettings;

typedef struct {
    uint32_t active_rally_count;
    uint32_t being_rally_count;
} RallyState;

typedef struct {
    uint16_t research_tech;
    uint8_t  unk;
    int64_t  finish_time;
    uint32_t total_time;
    uint8_t  tech_data[200];
} TechnologyInfo;


typedef struct {
	int64_t data_index;
	char leader[13];
	uint8_t level;
} DarknestRally;

typedef struct {
	double current;              // Current resource amount.
	uint32_t capacity;           // Maximum storage capacity.

	int64_t production_hour;     // Production per hour (negative = consumption).
	double production_second;    // Production per second.

	double update_timer;         // 1-second update accumulator.
} ResourceTracker;


typedef struct {
    char     name[13];
    uint8_t  vip;
    uint8_t  rank;
    int64_t  begin_time;
    uint32_t require_time;
    uint32_t troop_total;
    uint32_t troops[20];
} RallyMember;

typedef struct {
    uint32_t index;             // Rally index/id.
    uint8_t  kind;              // Rally type.

    int64_t  begin_time;        // Rally start time.
    uint32_t require_time;      // Time until march starts.

    /* Rally leader */
    uint16_t ally_zone_id;
    uint8_t  ally_point_id;
    uint16_t ally_head;
    char     ally_name[13];
    uint8_t  ally_vip;
    uint8_t  ally_rank;

    uint32_t ally_curr_troop;
    uint32_t ally_max_troop;

    uint16_t ally_home_kingdom;

    /* Target (Darknest) */
    uint16_t enemy_head;        // Always UINT16_MAX for NPC rallies.
    uint16_t enemy_zone_id;
    uint8_t  enemy_point_id;
    uint8_t  enemy_vip;
    uint16_t enemy_npc_id;

} NPCRally;

typedef struct {
	uint8_t  type;
	uint32_t index;
	uint8_t  kind;
	
	int64_t  begin_time;
	uint32_t require_time;
	
	/* Rally leader */
	uint16_t ally_zone_id;
	uint8_t  ally_point_id;
	uint16_t ally_head;
	char     ally_name[13];
	uint8_t  ally_vip;
	uint8_t  ally_rank;
	
	uint32_t ally_curr_troop;
	uint32_t ally_max_troop;
	
	/* Rally target */
	uint16_t enemy_zone_id;
	uint8_t  enemy_point_id;
	uint16_t enemy_head;
	char     enemy_name[13];
	uint8_t  enemy_vip;
	uint8_t  enemy_rank;
	
	char     enemy_alliance_tag[4]; // +1 for '\0'
	uint16_t enemy_home_kingdom;
} Rally;


typedef enum {
    TRANSFER_IDLE,
    TRANSFER_FIND_TARGET,
    TRANSFER_WAIT_TARGET,
    TRANSFER_SEND_MARCH,
    TRANSFER_WAIT_MARCH,
    TRANSFER_COMPLETE,
    TRANSFER_FAILED
} TransferState;


typedef struct {
	char issued_name[13]; // Who initiated resource command?
    char target_name[13]; // Who will receive resource?

    ResourceType resource_type;
    ResourceStock resource;
    
    time_t timeout;
    
    uint8_t max_marches;
    uint8_t cur_marches;
    
    uint32_t amount;
    uint32_t remaining;

    uint16_t zone_id;
    uint8_t point_id;

    TransferState state;
} ResourceTransfer;



#define MAX_ALLIANCE_MEMBER 100

typedef struct {
    int64_t user_id;
    uint16_t head;
    char name[14];
    uint8_t rank;
    uint64_t power;
    uint64_t troop_kill_num;
    int64_t logout_time;
    uint8_t white_list_flag;
} AllianceMember;

typedef struct {
    AllianceMember member[MAX_ALLIANCE_MEMBER];
    uint16_t recv_index;
    uint8_t data_finished;
    uint8_t count;
} AllianceMemberList;

typedef struct {
	// network
	int sock;
	PacketStream stream;
	// authentication
	AuthInfo auth;
	// game items
	Item items[MAX_ITEM_COUNT];
	bool items_loaded;
	// protocol state
	ProtocolState protocol;
	// outgoing packet buffer 
	// PacketBuffer reader;
	
	/*
	Stream sin;
	Stream sout;
	*/
	PacketBuffer sin;
	PacketBuffer sout;
	
	uint8_t data[4096];
	uint16_t size;
	uint16_t offset;
	// server state
	uint64_t server_time;
	time_t last_heartbeat;
	// login state
	bool lobby_login;
	// game server 
	ServerInfo game_server;
	ServerInfo gateway_server;
	
	// client configuration
	AppInfo app;
	// resources 
	ResourceStock resources;      // current resources
	ResourceStock bag_resources;  // consumable resource items
	ResourceProduction production; // Per-hour production
	uint64_t resources_last_update;
	
	ChatState chat;
	
	bool resource_loaded;
	// player information 
	PlayerInfo player;
	// game system 
	BlackMarket market;
	BlackMarketBuy buy;
	
	// resources sending
	// ResourceTransfer transfer;
	
	MailInfo mail;
	
	AllianceHelp help;
	
	uint8_t building_count;
	BuildingInfo building[256];
	
	AllianceGiftList alliance_gifts;
	AllianceSettings alliance;
	SmartUseList smart_use;
	
	HelpSpam help_spam;
	
	BankSettings bank;
	
	BotSettings bot;
	
	AllianceInfo RoleAlliance;
	
	PlayerBank player_bank[1000];
	
	// ShieldSettings shield;
	
	ShieldInfo shield_info;
	
	HyperSettings hyper;
	
	TroopData troop;
	
	WoundedTroopData wounded;
	
	ProtectionSettings protection;
	
	PendingRally rally;
	
	DarknestSettings darknest;
	
	RallyState rally_status;
	
	// Research information 
	TechnologyInfo technology;
	
	uint32_t supply_capacity;
	
	ResourceTracker tracker;
	
	NPCRally npc_rallies[30]; // maximum 30 npc rallies hold
	
	Rally ally_rallies[30];     // Rallies opened by our alliance.
	Rally enemy_rallies[30];    // Enemy rallies targeting us.
	
	RallyMember rally_members[30];
	
	ResourceTransfer transfer;
	
	AllianceMemberList alliance_member;
} Connection;

/* API */
int  connect_server(const char *ip, unsigned short port);
void disconnect(Connection *conn);
bool send_packet(Connection *conn, bool enc);
int set_nonblocking(Connection *conn);
void reset_connection(Connection *c);

#endif