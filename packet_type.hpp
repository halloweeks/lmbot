
void ConnectServer();

void ServerGuestLogIn(const uint64_t igg_id, const char *ass_key) {
	packet->write_reset();
	packet->write_u16(_MSG_NEWLOGIN_LOGINTOL);
	packet->write_u64(igg_id);
	
	packet->write_u8(channel);
	packet->write_u8(channel);
	packet->write_u16(channel);
	
	
	packet->write_u8(1);
	
	packet->write_u8(1); // Language code
	packet->write_string("a658b2df-1f99-4f71-8cbd-b355185a9788", 50); // DEVICE_UUID
    packet.write_u16(strlen(ass_key));
    packet.write_string(ass_key, 512);
}

void ServerLogIn() {
	
}

void Heartbeat(Packet *packet) {
	packet->reset();
	packet->write_u16(_MSG_REQUEST_ACTIVE);
	packet->write_seq_id();
	packet->send(true);
}

void ClientInitOver(Packet *packet, uint64_t igg_id) {
	packet->reset();
	packet->write_u16(_MSG_REQUEST_CLIENTINITOVER);
	packet->write_seq_id();
	packet->write_u64(igg_id);
	packet->send(true);
}

void SendChatMessage(Packet *packet, uint8_t channel, const char *message) {
	packet->reset();
	packet->write_u16(_MSG_REQUEST_SENDCHAT);
	packet->write_seq_id();
	packet->write_u8(channel);
	packet->write_u8(0);
	packet->write_u8(5);
	packet->write_u16(strlen(message));
	packet->write_byte(message, strlen(message));
	packet->send(true);
}








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
	p->read_byte(ObjAllianceTag, 3);
	
	char ObjName[13] = {0};
	p->read_byte(ObjName, 13);
	
	uint8_t ScoutResult = p->read_u8();
	uint8_t ScoutLevel = p->read_u8();
	uint16_t ScoutContentLen = p->read_u16();
	uint8_t ScoutContent[65535];
	// p->read_byte(ScoutContent, ScoutContentLen);
			
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
	
	dump_data("scout.bin", ObjName, p->data + p->offset, ScoutContentLen);
	
	if (ScoutResult == 1) {
		ServerSendChatFmt(packet, GUILD, "[Scout] Target '%s' at (%u:%u) is protected by shield or anti-scout.", ObjName, dst.x, dst.y);
		// dump_data("scout.bin", ObjName, p->data + p->offset, ScoutContentLen);
		return;
	}
	
	if (CombatPointKind != PK_CITY) {
		ServerSendChat(packet, GUILD, "[Scout] Only castles are valid targets.");
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
				p->read_byte(player_name[i], 13);
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
	
	
	
	
	char food_str[20] = {0};
	char rock_str[20] = {0};
	char wood_str[20], ore_str[20], gold_str[20];
	char troop_str[20] = {0};
	char troop_might[20] = {0};
	
	
	format_number2(food_str, sizeof(food_str), food);
	
	
	snprintf(food_str, sizeof(food_str), "%s", format_number(food, 0));
	snprintf(rock_str, sizeof(rock_str), "%s", format_number(rock, 0));
	snprintf(wood_str, sizeof(wood_str), "%s", format_number(wood, 0));
	snprintf(ore_str,  sizeof(ore_str),  "%s", format_number(ore, 0));
	snprintf(gold_str, sizeof(gold_str), "%s", format_number(gold, 0));
			
	snprintf(troop_str, sizeof(troop_str), "%s", format_number(DefenseNum, 0));
	snprintf(troop_might, sizeof(troop_might), "%s", format_number(CalcTroopMight(TroopsInfo), 0));
	
	// Now send formatted message
	ServerSendChatFmt(packet, GUILD, 
		"[Scout] Target: %s (%u:%u)\n"
		"Total troops: %s (might: %s)\n"
		"INF:  T1: %u     T2: %u      T3: %u      T4: %u\n"
		"RNG:  T1: %u     T2: %u      T3: %u      T4: %u\n"
		"CAV:  T1: %u     T2: %u      T3: %u      T4: %u\n"
		"SIE:  T1: %u     T2: %u      T3: %u      T4: %u\n"
		"Rss: Food (%s) Stone (%s) Wood (%s) Ore: (%s) Gold: (%s)",
		ObjName, dst.x, dst.y,
		troop_str, troop_might,
		TroopsInfo[0], TroopsInfo[1], TroopsInfo[2], TroopsInfo[3],
		TroopsInfo[4], TroopsInfo[5], TroopsInfo[6], TroopsInfo[7],
		TroopsInfo[8], TroopsInfo[9], TroopsInfo[10], TroopsInfo[11],
		TroopsInfo[12], TroopsInfo[13], TroopsInfo[14], TroopsInfo[15],
		food_str, rock_str, wood_str, ore_str, gold_str
	);
}


