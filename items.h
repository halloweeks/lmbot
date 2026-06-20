#ifndef _ITEMS_H_
#define _ITEMS_H_
// __VIM__0123456789
#define RANDOM_RELOCATOR 1003
#define ADVANCE_RELOCATOR 1004
#define SHIELD_72H 1053
#define SHIELD_24H 1052
#define SHIELD_8H 1051
#define SHIELD_4H 1146

#define SHIELD_7D 1287

#define FAMILIAR_MAGMA_LORD  0x0012

#define FOOD_5K 0x0492 
#define FOOD_30K 0x03F1 
#define FOOD_150K 0x03F6 
#define FOOD_500K 0x03FB 
#define FOOD_2M 0x0400 
#define FOOD_6M 0x0445 
#define FOOD_20M 0x044A 
#define FOOD_60M 0x044F 

#define STONE_3K 0x0493 
#define STONE_10K 0x03F2 
#define STONE_50K 0x03F7 
#define STONE_150K 0x03FC 
#define STONE_500K 0x0401 
#define STONE_1_5M 0x0446 
#define STONE_5M 0x044B 
#define STONE_15M 0x0450 

#define TIMBER_3K 0x0494 
#define TIMBER_10K 0x03F3 
#define TIMBER_50K 0x03F8 
#define TIMBER_150K 0x03FD 
#define TIMBER_500K 0x0402 
#define TIMBER_1_5M 0x0447 
#define TIMBER_5M 0x044C 
#define TIMBER_15M 0x0451 

#define ORE_3K 0x0495 
#define ORE_10K 0x03F4 
#define ORE_50K 0x03F9 
#define ORE_150K 0x03FE 
#define ORE_500K 0x0403 
#define ORE_1_5M 0x0448 
#define ORE_5M 0x044D 
#define ORE_15M 0x0452 


#define GOLD_3K 0x03F5 
#define GOLD_15K 0x03FA 
#define GOLD_50K 0x03FF 
#define GOLD_200K 0x0404 
#define GOLD_600K 0x0449 
#define GOLD_2M 0x044E 
#define GOLD_6M 0x0453 

/*
gold: 994385595
b3: 1
num: 0x03FD (1021)
num2: 0x1CEE (7406)
*/

void get_name_by_item_id(uint16_t id, char *out) {
    switch (id) {
        case SHIELD_4H:
            strcpy(out, "4h");
            break;
        case SHIELD_8H:
            strcpy(out, "8h");
            break;
        case SHIELD_24H:
            strcpy(out, "24h");
            break;
        case SHIELD_72H:
            strcpy(out, "3 Day");
            break;
        case SHIELD_7D:
            strcpy(out, "7 Day");
            break;
        case RANDOM_RELOCATOR:
            strcpy(out, "random relocator");
            break;
        case ADVANCE_RELOCATOR:
            strcpy(out, "advance relocator");
            break;
        default:
            strcpy(out, "Unknown Item");
            break;
    }
}

#endif