/*
Command handling logic not implemented 
*/

static void join_rally() {
	
}

void command_handler(Connection *c, const char *player_name, const char *message) {
	if (c->bot.command_prefix == 0) return;
	
	if (message[0] != c->bot.command_prefix) return;
	
	message++;
	
	if (memcmp(message, "move", 4) == 0) {
		message += 4;
		process_relocate(c, player_name, message);
		return;
	}
	
	if (memcmp(message, "food", 4) == 0) {
		message += 4;
		
		
		return;
	}
	
	if (memcmp(message, "join", 4) == 0) {
		message += 4;
		
		
	}
	
	
}