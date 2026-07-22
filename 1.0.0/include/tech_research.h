#ifndef TECH_RESEARCH_H
#define TECH_RESEARCH_H

#include <stdint.h>

typedef enum {
	TECH_CONSTRUCTION_SPEED    = 6,
	TECH_GEM_HARVESTING_I      = 8,
	
	TECH_FIRE_TREBUCHET        = 54, // T3 Siege Engine 

	
	
	TECH_ENERGY_RECOVERY_I     = 74,
	
	TECH_MORE_GATHERERS        = 123,
	TECH_BIGGER_BAGS_I         = 125,
	TECH_GOLD_STORAGE_I        = 143,
	
	TECH_WONDER_MARCH_I        = 228,
	TECH_GEM_HARVESTING_II     = 229,
	
	TECH_BIGGER_BAGS_II        = 234,
	
	
    
    TECH_BARRACK_EXPANSION_II  = 299,
    TECH_RATION_RUN_IV         = 301,
    TECH_FORCED_MARCH_III      = 302,
    TECH_BIGGER_BAGS_III       = 303,
    TECH_QUICK_MANEUVERS_III   = 305,
} TechnologyId;

typedef struct {
    const char *name;
    const char *effect;
    float value[11];
} TechInfo;

/*
static const char *ResearchName[400] = {
	[8]     = "Economy: Gem Harvesting I",
	[74]    = "Monster hunt: Energy Recovery I",
	[123]   = "Army Leadership: More gatherers",
	[125]   = "Army Leadership: Bigger Bags I",
	[143]   = "Army Leadership: Gold Storage I",
	[299]   = "Gear: Barrack Expansion II",
	[301]   = "Gear: Ration Run IV",
	[302]   = "Gear: Forced March III",
	[303]   = "Gear: Bigger Bags III",
	
	[305]   = "Gear: Quick Maneuvers III",
	
};*/

static const TechInfo techs[400] = {
	[TECH_CONSTRUCTION_SPEED] = {
		.name = "Economy: Construction Speed",
		.effect = "Construction Speed",
		.value = {
			0.0f,
			1.0f,
			3.0f,
			6.0f,
			10.0f,
			15.0f,
			21.0f,
			28.0f,
			37.0f,
			48.0f,
			70.0f
		}
	},
	
	[TECH_GEM_HARVESTING_I] = {
		.name = "Economy: Gem Harvesting I",
		.effect = "Gem Gathering Speed",
		.value = { // Value in percentage 
			0,
			1,
			2,
			3,
			4,
			5,
			6,
			7,
			8,
			9,
			10
		}
	},
	
	[TECH_GEM_HARVESTING_II] = {
		.name = "Wonder Battles: Gem Harvesting II",
		.effect = "Gem Gathering Speed",
		.value = {
			0.0f,
			0.2f,
			0.4f,
			0.6f,
			0.8f,
			1.1f,
			1.5f,
			2.0f,
			3.0f,
			5.0f,
			10.0f
		}
		
	},
	
	[TECH_BIGGER_BAGS_I] = {
		.name = "Army Leadership: Bigger Bags I",
		.effect = "Supply Capacity",
		.value = { // Value in numbers 
			0,
			50000,
			110000,
			180000,
			260000,
			350000,
			470000,
			710000,
			1110000,
			1750000,
			3000000
		}
	},
	
	[TECH_BIGGER_BAGS_II] = {
		.name = "Wonder Battles: Bigger Bags II",
		.effect = "Supply Capacity",
		.value = { // Value in numbers 
			0,
			10000,
			30000,
			60000,
			100000,
			150000,
			210000,
			290000,
			400000,
			560000,
			1000000
		}
	}, 
	
	[TECH_WONDER_MARCH_I] = {
		.name = "Wonder Battles: Wonder March I",
		.effect = "Wonder Travel Speed",
		.value = {
			0.0f,
			0.5f,
			1.0f,
			1.5f,
			2.5f,
			3.5f,
			4.5f,
			6.0f,
			8.0f,
			11.0f,
			20.0f
		}
	},
	
	[TECH_BIGGER_BAGS_III] = {
		.name = "Gear: Bigger Bags III",
		.effect = "Supply Capacity",
		.value = {
			0,
			10000,
			30000,
			60000,
			100000,
			150000,
			210000,
			290000,
			400000,
			560000,
			1000000
		}
	},
	
	[TECH_FIRE_TREBUCHET] = {
		.name = "Military: Fire Trebuchet",
		.effect = "Tier T3 Siege Engine",
		.value = {0}
	}
	
};


static inline const TechInfo *GetTechInfo(TechnologyId id)
{
	if (id <= 0 || id >= 400)
		return NULL;
		
	if (techs[id].name == NULL)
		return NULL;
	
	return &techs[id];
}


static inline uint8_t GetTechLevel(const uint8_t *tech_data, uint16_t tech_id)
{
	if (tech_id == 0)
		return 0;
	
	size_t index = (tech_id - 1) >> 1;
	
	uint8_t b = tech_data[index];
	
	if (tech_id & 1)
		return b & 0x0F;          // Odd TechID
	else
		return (b >> 4) & 0x0F;   // Even TechID
}



#endif