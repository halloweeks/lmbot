#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdint.h>

typedef enum {
	CMD_NONE,
	CMD_USER_BALANCE,
	CMD_BANK_BALANCE,
	CMD_RAND_RELOCATE,
	CMD_ADVANCED_RELOCATE,
	CMD_SHOW_RELOCATORS,
	CMD_PROCESS_STATUS,
	CMD_PROCESS_STOP,
	CMD_SEND_FOOD,
	CMD_SEND_STONE,
	CMD_SEND_WOOD,
	CMD_SEND_ORE,
	CMD_SEND_GOLD,
	CMD_ADD_ADMIN,
	CMD_DISMISS_ADMIN,
	CMD_LIST_ADMIN,
	CMD_RANK_ADMIN,
	CMD_WHOAMI,
	CMD_WHOIS,
	CMD_SWAP_RESOURCE,
	CMD_DEPOSIT_RESOURCE,
	CMD_TRANSFER_RESOURCE,
	CMD_KINGDOM,
	CMD_SEND_MAIL,
	CMD_SEND_MAIL_ALL,
	CMD_SHIELD,
	CMD_SUPERUSER,
	CMD_FIND_PLAYER,
	CMD_SQL,
	CMD_SCAN_KINGDOM 
} CommandType;

typedef struct {
	char name[13];
	CommandType cmd;
	uint8_t offset;
} Command;

// Check is valid command 
Command is_command(const char *message) {
	Command cmd;
	memset(&cmd, 0, sizeof(cmd));
	
	if (message[0] != COMMAND_PREFIX) return cmd;
	
	// skip the prefix 
	message++;
	
	// skip white space 
	while (*message == ' ') message++;
	
	if (strncasecmp(message, "sql", 3) == 0) {
		cmd.cmd = CMD_SQL;
		cmd.offset = 3;
		return cmd;
	}
	
	if (strncasecmp(message, "scan", 4) == 0) {
		cmd.cmd = CMD_SCAN_KINGDOM;
		cmd.offset = 4;
		return cmd;
	}
	
	
	// bank commands
	if (strncasecmp(message, "bal", 3) == 0) {
		cmd.cmd = CMD_USER_BALANCE;
		cmd.offset = 3;
		return cmd;
	}
	
	if (strncasecmp(message, "bank", 4) == 0) {
		cmd.cmd = CMD_BANK_BALANCE;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "food", 4) == 0) {
		cmd.cmd = CMD_SEND_FOOD;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "stone", 5) == 0) {
		cmd.cmd = CMD_SEND_STONE;
		cmd.offset = 5;
		return cmd;
	}
	
	if (strncasecmp(message, "wood", 4) == 0) {
		cmd.cmd = CMD_SEND_WOOD;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "ore", 3) == 0) {
		cmd.cmd = CMD_SEND_ORE;
		cmd.offset = 3;
		return cmd;
	}
	
	if (strncasecmp(message, "gold", 4) == 0) {
		cmd.cmd = CMD_SEND_GOLD;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "stop", 4) == 0) {
		cmd.cmd = CMD_PROCESS_STOP;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "status", 6) == 0) {
		cmd.cmd = CMD_PROCESS_STATUS;
		cmd.offset = 6;
		return cmd;
	}
	
	if (strncasecmp(message, "proc", 4) == 0) {
		cmd.cmd = CMD_PROCESS_STATUS;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "swap", 4) == 0) {
		cmd.cmd = CMD_SWAP_RESOURCE;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "deposit",  7) == 0) {
		cmd.cmd = CMD_DEPOSIT_RESOURCE;
		cmd.offset = 7;
		return cmd;
	}
	
	if (strncasecmp(message, "transfer", 8) == 0) {
		cmd.cmd = CMD_TRANSFER_RESOURCE;
		cmd.offset = 8;
		return cmd;
	}
	
	// admin commands 
	if (strncasecmp(message, "admin", 5) == 0) {
		cmd.cmd = CMD_ADD_ADMIN;
		cmd.offset = 5;
		return cmd;
	}
	
	if (strncasecmp(message, "dismiss",  7) == 0) {
		cmd.cmd = CMD_DISMISS_ADMIN;
		cmd.offset = 7;
		return cmd;
	}
	
	if (strncasecmp(message, "remove", 6) == 0) {
		cmd.cmd = CMD_DISMISS_ADMIN;
		cmd.offset = 6;
		return cmd;
	}
	
	if (strncasecmp(message, "list_admin", 10) == 0) {
		cmd.cmd = CMD_LIST_ADMIN;
		cmd.offset = 10;
		return cmd;
	}
	
	if (strncasecmp(message, "rank", 4) == 0) {
		cmd.cmd = CMD_RANK_ADMIN;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "whoami", 6) == 0) {
		cmd.cmd = CMD_WHOAMI;
		cmd.offset = 6;
		return cmd;
	}
	
	if (strncasecmp(message, "whois",  5) == 0) {
		cmd.cmd = CMD_WHOIS;
		cmd.offset = 5;
		return cmd;
	}
	
	// relocation commands
	if (strncasecmp(message, "rand", 4) == 0) {
		cmd.cmd = CMD_RAND_RELOCATE;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "move", 4) == 0) {
		cmd.cmd = CMD_ADVANCED_RELOCATE;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "teleport", 8) == 0) {
		cmd.cmd = CMD_ADVANCED_RELOCATE;
		cmd.offset = 8;
		return cmd;
	}
	
	if (strncasecmp(message, "relocate", 8) == 0) {
		cmd.cmd = CMD_ADVANCED_RELOCATE;
		cmd.offset = 8;
		return cmd;
	}
	
	// show relocators
	if (strncasecmp(message, "relocator",   9) == 0) {
		cmd.cmd = CMD_SHOW_RELOCATORS;
		cmd.offset = 9;
		return cmd;
	}
	
	if (strncasecmp(message, "relocators", 10) == 0) {
		cmd.cmd = CMD_SHOW_RELOCATORS;
		cmd.offset = 10;
		return cmd;
	}
	
	// kingdom commands
	if (strncasecmp(message, "kd", 2) == 0) {
		cmd.cmd = CMD_KINGDOM;
		cmd.offset = 2;
		return cmd;
	}
	
	// mail commands
	if (strncasecmp(message, "mail", 4) == 0) {
		cmd.cmd = CMD_SEND_MAIL;
		cmd.offset = 4;
		return cmd;
	}
	
	if (strncasecmp(message, "mailall", 7) == 0) {
		cmd.cmd = CMD_SEND_MAIL_ALL;
		cmd.offset = 7;
		return cmd;
	}
	
	//shield commands
	if (strncasecmp(message, "shield", 6) == 0) {
		cmd.cmd = CMD_SHIELD;
		cmd.offset = 6;
		return cmd;
	}
	
	if (strncasecmp(message, "su", 2) == 0) {
		cmd.cmd = CMD_SUPERUSER;
		cmd.offset = 2;
		return cmd;
	}
	
	if (strncasecmp(message, "find", 4) == 0) {
		cmd.cmd = CMD_FIND_PLAYER;
		cmd.offset = 4;
		return cmd;
	}
	
	
    // if (memcmp(message, "use", 3) == 0 && (message[3] == ' ' || message[3] == '\0')) return true;
    // if (memcmp(message, "scout", 5) == 0 && (message[5] == ' ' || message[5] == '\0')) return true;
    // if (memcmp(message, "camp", 4) == 0 && (message[4] == ' ' || message[4] == '\0')) return true;
    // if (memcmp(message, "ret", 3) == 0 && (message[3] == ' ' || message[3] == '\0')) return true;
    // if (memcmp(message, "hunt", 4) == 0 && (message[4] == ' ' || message[4] == '\0')) return true;
    // if (memcmp(message, "buy", 3) == 0 && (message[3] == ' ' || message[3] == '\0')) return true;
    
    // help spam
    // if (memcmp(message, "help", 4) == 0 && (message[4] == ' ' || message[4] == '\0')) return true;
    
    // send chat, public commands 
    // if (memcmp(message, "wc", 2) == 0 && (message[2] == ' ' || message[2] == '\0')) return true;
    // if (memcmp(message, "gc", 2) == 0 && (message[2] == ' ' || message[2] == '\0')) return true;
    // if (memcmp(message, "f", 1) == 0 && message[1] == '\0') return true;
    // if (memcmp(message, "s", 1) == 0 && message[1] == '\0') return true;
    // if (memcmp(message, "b", 1) == 0 && message[1] == '\0') return true;
    
    // if (memcmp(message, "test", 4) == 0 && (message[4] == ' ' || message[4] == '\0')) return true;
    // if (memcmp(message, "spam", 4) == 0 && (message[4] == ' ' || message[4] == '\0')) return true;
    
    // if (memcmp(message, "io", 2) == 0 && (message[2] == ' ' || message[2] == '\0')) return true;
    // if (memcmp(message, "scout", 5) == 0 && (message[5] == ' ' || message[5] == '\0')) return true;
    return cmd;
}

#endif
