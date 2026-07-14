#include "bank.h"

#include "connection.h"
#include "net_rw.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#define BANK_TEMP_FILE "bank.tmp"

static uint32_t SafeIncrement(uint32_t value, uint32_t inc)
{
    if (UINT32_MAX - value < inc)
        return UINT32_MAX;

    return value + inc;
}

static uint32_t SafeDecrement(uint32_t value, uint32_t dec)
{
    return (value > dec) ? value - dec : 0;
}

bool BankLoad(Connection *c, const char *filename)
{
	memset(&c->bank, 0, sizeof(c->bank));
	
	int file = open(filename, O_RDONLY);
	
	if (file == -1) {
		perror("[ERROR] Error opening file bank data");
		return false;
	}
	
	if (read(file, &bank
	
	close(file);
	return true;
}

bool BankSave(Connection *c)
{
	
	int fd = open(BANK_TEMP_FILE,
		O_WRONLY | O_CREAT | O_TRUNC,
		0644);
	
	if (fd == -1)
		return false;
	
	write(fd,
          &c->bank.player_count,
          sizeof(c->bank.player_count));

    write(fd,
          c->bank.players,
          c->bank.player_count * sizeof(PlayerBank));

    fsync(fd);
    close(fd);

    rename(BANK_TEMP_FILE, "bank.dat");

    return true;
}

PlayerBank *BankGetPlayer(Connection *c, const char *player_name)
{
    for (uint16_t i = 0; i < c->bank.player_count; i++)
    {
        if (strcmp(c->bank.players[i].player_name,
                   player_name) == 0)
        {
            return &c->bank.players[i];
        }
    }

    return NULL;
}

void BankUpdateBalance(Connection *c,
                       const char *player_name,
                       ResourceAmount res,
                       uint8_t operation)
{
    PlayerBank *player = BankGetPlayer(c, player_name);

    if (player == NULL)
    {
        if (c->bank.player_count >= MAX_BANK_PLAYERS)
            return;

        player = &c->bank.players[c->bank.player_count++];
        memset(player, 0, sizeof(*player));

        strcpy(player->player_name, player_name);
    }

    if (operation)
    {
        player->food = SafeIncrement(player->food, res.food);
        player->rock = SafeIncrement(player->rock, res.rock);
        player->wood = SafeIncrement(player->wood, res.wood);
        player->ore  = SafeIncrement(player->ore,  res.ore);
        player->gold = SafeIncrement(player->gold, res.gold);
    }
    else
    {
        player->food = SafeDecrement(player->food, res.food);
        player->rock = SafeDecrement(player->rock, res.rock);
        player->wood = SafeDecrement(player->wood, res.wood);
        player->ore  = SafeDecrement(player->ore,  res.ore);
        player->gold = SafeDecrement(player->gold, res.gold);
    }

    BankSave(c);
}