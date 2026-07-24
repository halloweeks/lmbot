/*
 * Configuration parser.
 *
 * Each configuration key is matched using a simple if/return chain.
 *
 * Although a lookup table or hash map could also be used, this function is
 * only executed once during application startup when the configuration file
 * is loaded. It is never called from the main game loop, network packet
 * processing, or any performance-critical code.
 *
 * Because the parser runs only once, the cost of multiple strcmp() calls is
 * negligible compared to the overall initialization time. This approach keeps
 * the code straightforward, easy to read, and simple to extend—adding a new
 * configuration option only requires adding another comparison.
 *
 * Prefer clarity and maintainability here over unnecessary optimization.
 */

#include "config.h"
#include <stdlib.h>

#include <ctype.h>

static CommandChannel ParseCommandChannel(const char *value)
{
    if (strcmp(value, "WORLD") == 0)
        return COMMAND_CHANNEL_WORLD;

    if (strcmp(value, "GUILD") == 0)
        return COMMAND_CHANNEL_GUILD;

    if (strcmp(value, "MAIL") == 0)
        return COMMAND_CHANNEL_MAIL;

    return COMMAND_CHANNEL_GUILD; // default
}


static uint16_t ParseShield(const char *value)
{
    if (strcmp(value, "SHIELD_4H") == 0)
        return SHIELD_4H;

    if (strcmp(value, "SHIELD_8H") == 0)
        return SHIELD_8H;

    if (strcmp(value, "SHIELD_12H") == 0)
        return SHIELD_12H;

    if (strcmp(value, "SHIELD_1D") == 0)
        return SHIELD_1D;

    if (strcmp(value, "SHIELD_3D") == 0)
        return SHIELD_3D;

    if (strcmp(value, "SHIELD_7D") == 0)
        return SHIELD_7D;

    if (strcmp(value, "SHIELD_14D") == 0)
        return SHIELD_14D;

    return 0;
}

static bool ParseShieldPriority(Connection *c, const char *value)
{
    char buffer[512];

    strncpy(buffer, value, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    int count = 0;

    char *token = strtok(buffer, ",");
    
    while (token && count < 8) {
        while (*token == ' ')
            token++;

        uint16_t shield = ParseShield(token);

        if (shield == 0) {
            printf("Invalid shield priority value: %s\n", token);
            return false;
        }

        c->protection.shield_priority[count++] = shield;

        token = strtok(NULL, ",");
    }

    c->protection.shield_priority_count = count;
    return true;
}

/*
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
*/

uint64_t parse_number_u64(const char *str);

static bool ParserConfig(Connection *c, const char *key, const char *value) {
	// gateway server 
	if (strcmp(key, "server.addr") == 0) {
		strncpy(c->gateway_server.addr, value, 16);
		return true;
	}
	
	if (strcmp(key, "server.port") == 0) {
		c->gateway_server.port = (uint16_t)strtoul(value, NULL, 10);
		return true;
	}
	
	// client version 
	if (strcmp(key, "client.version_major") == 0) {
		c->app.version_major = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "client.version_minor") == 0) {
		c->app.version_minor = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "client.version_patch") == 0) {
		c->app.version_patch = (uint16_t)strtoul(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "client.language_code") == 0) {
		c->app.language_code = (uint8_t)strtoul(value, NULL, 10);
		return true;
	}
	
	// login 
	if (strcmp(key, "account.igg_id") == 0) {
		c->auth.igg_id = (uint64_t)strtoull(value, NULL, 10);
		return true;
	}
	
	if (strcmp(key, "account.device_uuid") == 0) {
		strncpy(c->auth.device_uuid, value, sizeof(c->auth.device_uuid));
		return true;
	}
	
	if (strcmp(key, "account.access_key") == 0) {
		strcpy(c->auth.session, value);
		c->auth.session_len = (uint16_t)strlen(c->auth.session);
		return true;
	}
	
	// command 
	if (strcmp(key, "command.input") == 0) {
		c->bot.command_input = ParseCommandChannel(value);
		return true;
	}
	
	if (strcmp(key, "command.output") == 0) {
		c->bot.command_output = ParseCommandChannel(value);
		return true;
	}
	
	if (strcmp(key, "command.prefix") == 0) {
		if (value[0] != '\0') {
			c->bot.command_prefix = value[0];
		}
		return true;
	}
	
	
	if (strcmp(key, "alliance.auto_help") == 0) {
		c->alliance.auto_help = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "alliance.auto_open_gifts") == 0) {
		c->alliance.auto_open_gifts = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.enabled") == 0) {
		c->protection.enabled = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_always_on") == 0) {
		c->protection.enabled = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_always_on") == 0) {
		c->protection.enabled = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_on_incoming_attack") == 0) {
		c->protection.shield_on_incoming_attack = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_on_incoming_scout") == 0) {
		c->protection.shield_on_incoming_scout = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "protection.shield_priority") == 0) {
		return ParseShieldPriority(c, value);
	}
	
	if (strcmp(key, "admin.name") == 0) {
		strncpy(c->bot.admin_name, value, sizeof(c->bot.admin_name) - 1);
		c->bot.admin_name[sizeof(c->bot.admin_name) - 1] = '\0';
		return true;
	}
	
	// Cargo ship setting 
	if (strcmp(key, "cargo_ship.auto_trade") == 0) {
		if (strcmp(value, "true") != 0 && strcmp(value, "false") != 0) {
			return false;
		}
		
		c->market.settings.auto_trade = (strcmp(value, "true") == 0);
		return true;
	}
	
	// Spend resources 
	if (strcmp(key, "cargo_ship.spend_food") == 0) {
		c->market.settings.spend_food = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_rock") == 0) {
		c->market.settings.spend_rock = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_wood") == 0) {
		c->market.settings.spend_wood = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_ore") == 0) {
		c->market.settings.spend_ore  = (strcmp(value, "true") == 0);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.spend_gold") == 0) {
		c->market.settings.spend_gold  = (strcmp(value, "true") == 0);
		return true;
	}
	
	// Use resources from bag
	if (strcmp(key, "cargo_ship.use_bag_rss") == 0) {
		c->market.settings.use_bag_rss  = (strcmp(value, "true") == 0);
		return true;
	}
	
	
	// Reserved resources 
	if (strcmp(key, "cargo_ship.reserve_food") == 0) {
		c->market.reserve.food  = (uint32_t)parse_number_u64(value);
		// printf("cargo_ship.reserve_food: %lu\n", c->market.reserve.food);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_rock") == 0) {
		c->market.reserve.rock  = (uint32_t)parse_number_u64(value);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_wood") == 0) {
		c->market.reserve.wood  = (uint32_t)parse_number_u64(value);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_ore") == 0) {
		c->market.reserve.ore  = (uint32_t)parse_number_u64(value);
		return true;
	}
	
	if (strcmp(key, "cargo_ship.reserve_gold") == 0) {
		c->market.reserve.gold  = (uint32_t)parse_number_u64(value);
		return true;
	}
	
	return true;
}

bool LoadConfig(Connection *c, const char *filename)
{
	FILE *fp = fopen(filename, "r");
	
	if (!fp) {
		return false;
	}
	
	char line[512];
	char key[64], value[512];
	uint32_t line_num = 0;
	
	while (fgets(line, sizeof(line), fp)) {
		line_num++;
		
		/* Skip blank lines */
		if (line[0] == '\n' || line[0] == '\r') 
			continue;
		
		/* Skip comments */
		if (line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/')) 
			continue;
		
		if (sscanf(line, " %63[^=]= %511[^\n]", key, value) != 2) {
			line[strcspn(line, "\r\n")] = '\0';
			printf("%s:%u: error: unexpected syntax: %s\n", filename, line_num, line);
			fclose(fp);
			return false;
		}
		
		key[strcspn(key, " \t")] = '\0';
		value[strcspn(value, "\r\n")] = '\0';	
		
		if (ParserConfig(c, key, value) != true) {
			printf("%s:%u: error: `%s`\n", filename, line_num, value);
			fclose(fp);
			return false;
		}
    }

    fclose(fp);
    return true;
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
		"account.igg_id      = YOUR_NUMERIC_ID_HERE\n"
		"account.device_uuid = YOUR_DEVICE_UUID_HERE\n"
		"account.access_key  = YOUR_ACCESS_KEY_HERE\n\n"
		
		"# Prefix used to identify bot commands.\n"
		"command.prefix = $\n\n"
		
		"# Command channels: WORLD, GUILD, MAIL\n"
		"command.input  = GUILD\n"
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
		
		"# Alliance\n"
		"# Automatically send Guild Help to guild members whenever available.\n"
		"# Helping guild members reduces their timers and earns Guild Coins.\n"
		"alliance.auto_help = false\n\n"
		
		"# Automatically open Alliance Gift chests and claim all available rewards.\n"
		"alliance.auto_open_gifts = false\n\n"
		
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
		"cargo_ship.auto_trade  = false\n"
		"# The options below specify which resources the bot is allowed to spend.\n"
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
		
		"# Allowed Items (The bot WILL buy these)\n"
		"cargo_ship.trade_for_archaic_tomes      = true\n"
		"cargo_ship.trade_for_bright_talent_orb  = true\n"
		"cargo_ship.trade_for_exp_elixir         = true\n"
		"cargo_ship.trade_for_speed_up           = true\n"
		"cargo_ship.trade_for_speed_up_research  = true\n\n"
		
		"# Blocked Items (The bot WILL ignore these)\n"
		"cargo_ship.trade_for_speed_up_training  = false\n"
		"cargo_ship.trade_for_speed_up_merging   = false\n"
		"cargo_ship.trade_for_anima = false\n\n"
		"# Trades resources\n"
		"cargo_ship.trade_for_food  = false\n"
		"cargo_ship.trade_for_rock  = false\n"
		"cargo_ship.trade_for_wood  = false\n"
		"cargo_ship.trade_for_ore   = false\n"
		"cargo_ship.trade_for_gold  = false\n"
		
	);

    fclose(fp);
    return true;
}