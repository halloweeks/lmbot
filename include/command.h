#ifndef COMMAND_PARSER
#define COMMAND_PARSER

#include <stdint.h>
#include "connection.h"

void command_handler(Connection*, const char*, const char*);

static void ResourceCommandHandler(
    Connection *c,
    const char *player_name,
    const char *message,
    ResourceType type,
    const char *name
);

#endif