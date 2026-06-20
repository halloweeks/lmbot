#ifndef __BANK_H__
#define __BANK_H__

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// default superuser 
#define SUPERUSER "halloweeks"

#define BANK_DATA_FILE "/sdcard/lmbot/bank.dat"
#define BANK_TEMP_FILE "/sdcard/lmbot/bank.tmp"

#define BAL_CREDIT 1
#define BAL_DEBIT  0

typedef enum {
	RANK_0,
    RANK_1,  // minimal privileges
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,
    RANK_9,
    RANK_10      // highest privileges (superuser)
} AdminRank;

typedef struct {
	char player_name[13];
	uint8_t rank;
	uint32_t food;
	uint32_t rock;
	uint32_t wood;
	uint32_t ore;
	uint32_t gold;
} __attribute__((packed)) PlayerBank;

typedef struct {
	uint32_t food;
	uint32_t rock;
	uint32_t wood;
	uint32_t ore;
	uint32_t gold;
} __attribute__((packed)) ResHelp;


static PlayerBank player_bank[1000];
static uint16_t player_count = 0;

void load_bank(const char *filename) {
	memset(&player_bank, 0, sizeof(player_bank));
	
	int file = open(BANK_DATA_FILE, O_RDONLY);
	
	if (file == -1) {
		return;
	}
	
	read(file, &player_count, 2);
	
	if (player_count == 0) {
		close(file);
		return;
	}
	
	for (int i = 0; i < player_count; i++) {
		read(file, player_bank[i].player_name, 13);
		read(file, &player_bank[i].rank, 1);
		read(file, &player_bank[i].food, 4);
		read(file, &player_bank[i].rock, 4);
		read(file, &player_bank[i].wood, 4);
		read(file, &player_bank[i].ore, 4);
		read(file, &player_bank[i].gold, 4);
	}
	
	close(file);
}

void save_bank(void) {
	int file = open(BANK_TEMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	
	if (file == -1) {
		return;
	}
	
	// exit don't save anything 
	if (player_count <= 0 || player_count >= 10000) {
		return;
	}
	
	write(file, &player_count, 2);
	
	for (int i = 0; i < player_count; i++) {
		write(file, player_bank[i].player_name, 13);
		write(file, &player_bank[i].rank, 1);
		write(file, &player_bank[i].food, 4);
		write(file, &player_bank[i].rock, 4);
		write(file, &player_bank[i].wood, 4);
		write(file, &player_bank[i].ore, 4);
		write(file, &player_bank[i].gold, 4);
	}
	
	fsync(file);   // ensure flush to disk
    close(file);

    rename(BANK_TEMP_FILE, BANK_DATA_FILE);  // atomic replace
}

PlayerBank get_player_bank(const char *player_name) {
	for (int i = 0; i < player_count; i++) {
		if (strcmp(player_bank[i].player_name, player_name) == 0) {
			return player_bank[i]; // return copy
		}
	}
	PlayerBank empty = {0}; // zero-initialized if not found
	return empty;
}



// Return a copy with zero resources if not found
PlayerBank get_player_bank_safe(const char *player_name) {
	PlayerBank bank;
	memset(&bank, 0, sizeof(bank));
	
	for (int index = 0; index < player_count; index++) {
		if (strcmp(player_bank[index].player_name, player_name) == 0) {
			memcpy(&bank, &player_bank[index], sizeof(bank));
			return bank;
		}
	}
	
	return bank;
}

static inline uint32_t safe_increment(uint32_t value, uint32_t inc) {
    if (UINT32_MAX - value < inc) return UINT32_MAX;
    return value + inc;
}

static inline uint32_t safe_decrement(uint32_t value, uint32_t dec) {
    return (value > dec) ? value - dec : 0;
}

/*
void update_balance(const char *player_name, uint32_t food, uint32_t rock, uint32_t wood, uint32_t ore, uint32_t gold, uint8_t flag) {
	int index = -1;
	
	for (int i = 0; i < player_count; i++) {
		if (strcmp(player_name, player_bank[i].player_name) == 0) {
			index = i;
			break;
		}
	}
	
	if (index == -1) {
		index = player_count++;
	}
	
	strcpy(player_bank[index].player_name, player_name);
	
	if (flag == 1) {
		player_bank[index].food = safe_increment(player_bank[index].food, food);
		player_bank[index].rock = safe_increment(player_bank[index].rock, rock);
		player_bank[index].wood = safe_increment(player_bank[index].wood, wood);
		player_bank[index].ore  = safe_increment(player_bank[index].ore,  ore);
		player_bank[index].gold = safe_increment(player_bank[index].gold, gold);
	} else {
		player_bank[index].food = safe_decrement(player_bank[index].food, food);
		player_bank[index].rock = safe_decrement(player_bank[index].rock, rock);
		player_bank[index].wood = safe_decrement(player_bank[index].wood, wood);
		player_bank[index].ore  = safe_decrement(player_bank[index].ore,  ore);
		player_bank[index].gold = safe_decrement(player_bank[index].gold, res.gold);
	}
	
	save_bank();   // atomic save using tmp file
	return;
}*/


void update_balance(const char *player_name, ResHelp res, uint8_t flag) {
	int index = -1;
	
	for (int i = 0; i < player_count; i++) {
		if (strcmp(player_name, player_bank[i].player_name) == 0) {
			index = i;
			break;
		}
	}
	
	if (index == -1) {
		index = player_count++;
	}
	
	strcpy(player_bank[index].player_name, player_name);
	
	if (flag == 1) {
		player_bank[index].food = safe_increment(player_bank[index].food, res.food);
		player_bank[index].rock = safe_increment(player_bank[index].rock, res.rock);
		player_bank[index].wood = safe_increment(player_bank[index].wood, res.wood);
		player_bank[index].ore  = safe_increment(player_bank[index].ore,  res.ore);
		player_bank[index].gold = safe_increment(player_bank[index].gold, res.gold);
	} else {
		player_bank[index].food = safe_decrement(player_bank[index].food, res.food);
		player_bank[index].rock = safe_decrement(player_bank[index].rock, res.rock);
		player_bank[index].wood = safe_decrement(player_bank[index].wood, res.wood);
		player_bank[index].ore  = safe_decrement(player_bank[index].ore,  res.ore);
		player_bank[index].gold = safe_decrement(player_bank[index].gold, res.gold);
	}
	
	save_bank();   // atomic save using tmp file
	return;
}





// Short rank name (for logs / console)
const char* rank_name(uint8_t rank) {
    switch(rank) {
        case RANK_0:  return "Member";          // guild member
        case RANK_1:  return "Trusted";         // trusted member
        case RANK_2:  return "Senior";          // senior member
        case RANK_3:  return "Leader";          // team leader
        case RANK_4:  return "Officer";         // responsible user
        case RANK_5:  return "Supervisor";      // junior admin
        case RANK_6:  return "Manager";         // admin
        case RANK_7:  return "Administrator";   // high-level admin
        case RANK_8:  return "Senior Admin";    // high-level admin
        case RANK_9:  return "Executive Admin"; // almost full control
        case RANK_10: return "Superuser";       // full system control
        default:      return "Unknown";
    }
}

// Formal role title (for messages / UI)
const char* rank_title(uint8_t rank) {
    switch(rank) {
        case RANK_0:  return "Guild Member";
        case RANK_1:  return "Trusted Member";
        case RANK_2:  return "Senior Member";
        case RANK_3:  return "Team Leader";
        case RANK_4:  return "Officer";
        case RANK_5:  return "Supervisor";
        case RANK_6:  return "Manager";
        case RANK_7:  return "Administrator";
        case RANK_8:  return "Senior Admin";
        case RANK_9:  return "Executive Admin";
        case RANK_10: return "Superuser";
        default:      return "Unknown";
    }
}

typedef enum {
    ADMIN_OK = 0,                     // Success
    ADMIN_ALREADY_EXISTS,              // Target admin already exists
    ADMIN_NAME_TOO_LONG,               // Name > 12 characters
    ADMIN_UNAUTHORIZED,                // Requester not authorized
    ADMIN_RANK_TOO_LOW,               // Requester's level insufficient
    ADMIN_CANNOT_SET_HIGHER_LEVEL,     // Can't assign level >= own
    ADMIN_CANNOT_MODIFY_EQUAL_OR_HIGHER,// Can't modify admin of equal/higher level
    ADMIN_CANNOT_REMOVE_SELF,          // Requester tried to remove self
    ADMIN_SUPERUSER_PROTECTED,         // Superuser cannot be modified/removed
    ADMIN_NOT_FOUND,                   // Target admin not found
    ADMIN_ARRAY_FULL                   // Admin list full
} AdminResult;

const char *admin_result_to_msg(AdminResult res) {
    switch (res) {
        case ADMIN_OK:                     return "Operation succeeded.";
        case ADMIN_ALREADY_EXISTS:          return "Admin already exists.";
        case ADMIN_NAME_TOO_LONG:           return "Admin name is too long.";
        case ADMIN_ARRAY_FULL:              return "Admin list is full.";
        case ADMIN_CANNOT_REMOVE_SELF:      return "Cannot remove yourself.";
        case ADMIN_RANK_TOO_LOW:           return "Your privilege rank is too low for this action.";
        case ADMIN_CANNOT_MODIFY_EQUAL_OR_HIGHER: return "Cannot modify an admin with equal or higher rank.";
        case ADMIN_CANNOT_SET_HIGHER_LEVEL: return "Cannot assign a rank equal to or higher than your own.";
        case ADMIN_SUPERUSER_PROTECTED:     return "Cannot modify the superuser.";
        case ADMIN_NOT_FOUND:               return "Target admin not found.";
        case ADMIN_UNAUTHORIZED:            return "You are not authorized to perform this action.";
        default:                            return "Unknown error occurred.";
    }
}

bool is_superuser(const char *player_name) {
    return strcmp(player_name, SUPERUSER) == 0;
}

bool is_admin(const char *player_name) {
	// Superuser always authorized
	if (strcmp(SUPERUSER, player_name) == 0) return true;
	
	// check admin array
	for (int index = 0; index < player_count; index++) {
		if (strcmp(player_bank[index].player_name, player_name) == 0) {
			if (player_bank[index].rank != 0) {
				return true;
			}
		}
	}
	
	return false;
}

bool is_authorized(const char *name) {
	// Superuser always authorized
	if (strcmp(SUPERUSER, name) == 0) return true;
	
	// check admin array
    for (int index = 0; index < player_count; index++) {
        if (strcmp(player_bank[index].player_name, name) == 0 && player_bank[index].rank != 0) {
            return true;
        }
    }
    
    return false; // not found 
}

uint8_t get_rank(const char *name) {
	if (strcmp(SUPERUSER, name) == 0) return RANK_10;
	
	for (int index = 0; index < player_count; index++) {
		if (strcmp(player_bank[index].player_name, name) == 0) {
			return player_bank[index].rank;
		}
	}
	
	return RANK_0;
}

AdminResult add_admin(const char *admin_name, const char *target_name, uint8_t r) {
	// Check privilege level (only high-level admins can add)
	uint8_t rank = get_rank(admin_name);
	
	if (rank < RANK_8) return ADMIN_RANK_TOO_LOW; // e.g., level 8+ can add admins
	
	if (strlen(target_name) > 12) return ADMIN_NAME_TOO_LONG; // too long
	if (is_authorized(target_name)) return ADMIN_ALREADY_EXISTS;
	
	if (strcmp(target_name, SUPERUSER) == 0) return ADMIN_SUPERUSER_PROTECTED;
	
	for (int index = 0; index < 1000; index++) {
		if (player_bank[index].player_name[0] == '\0') {
			memcpy(player_bank[index].player_name, target_name, strlen(target_name));
			player_bank[index].rank = r;
			player_count++;
			save_bank();
			return ADMIN_OK; // added
		}
	}
	
	return ADMIN_ARRAY_FULL; // array full
}

AdminResult add_admin_default(const char *requester, const char *new_admin_name) {
    return add_admin(requester, new_admin_name, 1); // default level 1
}

AdminResult remove_admin(const char *admin_name, const char *target_name) {
	if (strcmp(admin_name, target_name) == 0) return ADMIN_CANNOT_REMOVE_SELF;
	
	// Check privilege level
	if (get_rank(admin_name) < RANK_8) return ADMIN_RANK_TOO_LOW;
	
	// Prevent removing superuser
	if (strcmp(target_name, SUPERUSER) == 0) return ADMIN_SUPERUSER_PROTECTED;
	
	for (int index = 0; index < player_count; index++) {
		if (strcmp(player_bank[index].player_name, target_name) == 0) {
			player_bank[index].rank = 0;
			save_bank();
			return ADMIN_OK; // removed
		}
	}
	
	return ADMIN_NOT_FOUND; // not found
}


AdminResult set_admin_level(const char *requester_name, const char *target_name, AdminRank new_rank) {
	uint8_t admin_rank = get_rank(requester_name);  // request player rank
	uint8_t target_rank = get_rank(target_name); // target player rank
	
	// Check privilege: only high-level admins can change levels
	if (admin_rank < RANK_5) return ADMIN_RANK_TOO_LOW;
	
	// Must be strictly higher than target
	if (admin_rank <= target_rank) return ADMIN_CANNOT_MODIFY_EQUAL_OR_HIGHER;
	
	// Cannot set a level equal or higher than own
	if (new_rank >= admin_rank) return ADMIN_CANNOT_SET_HIGHER_LEVEL;
		
	// Prevent modifying superuser
	if (strcmp(target_name, SUPERUSER) == 0) return ADMIN_SUPERUSER_PROTECTED;
	
	// Find target player 
	for (int i = 0; i < player_count; i++) {
		if (strcmp(player_bank[i].player_name, target_name) == 0) {
			player_bank[i].rank = new_rank;
			save_bank();
			return ADMIN_OK;
		}
	}
	
	return ADMIN_NOT_FOUND;
}

void get_admin_list(char *buffer) {
	strcpy(buffer, "admins: ");
	
	if (player_count == 0) {
		strcat(buffer, "(empty)");
		return;
	}
	
	for (int index = 0; index < player_count; index++) {
		strcat(buffer, player_bank[index].player_name);
		strcat(buffer, ", ");
	}
	
	buffer[strlen(buffer) - 2] = '\0';
}

#endif