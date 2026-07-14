#include "map_point.h"


int TileMapPosToMapID(int in_posx, int in_posy) {
    return (in_posy << 8) + (((++in_posx - (in_posy & 1)) >> 1));
}

/*
int PointCodeToMapID(uint16_t zoneID, uint16_t pointID) {
    // zoneID = Y, pointID = X
    int lower = ((pointID - (zoneID & 1)) >> 1) & 0xFF;
    int upper = zoneID & 0xFF;  // upper 8 bits
    return (zoneID << 8) | lower;
}*/

int PointCodeToMapID(uint16_t zoneID, uint8_t pointID) {
	return ((int)(zoneID & 1023 & 15) << 4) + (int)(pointID & 15) + ((((zoneID & 1023) >> 4 << 4) + (pointID >> 4)) << 8);
}

void MapIDToPointCode(int mapID, uint16_t *zoneID, uint8_t *pointID) {
	int num = mapID & 255;
	int num2 = mapID >> 8;
	*zoneID = (uint16_t)((num2 >> 4 << 4) + (num >> 4));
	*pointID = (uint8_t)(((num2 & 15) << 4) + (num & 15));
}

map_pos_t getTileMapPosbyMapID(int mapID) {
	map_pos_t pos = {0};
	
	if (mapID > -1 && mapID < 262144) {
		int num = mapID >> 8;
		int num2 = ((mapID & 255) << 1) + (num & 1);
		
		pos.x = num2;
		pos.y = num;
		return pos;
	}
	
	return pos;
}

bool CheckTileMapPos(int in_posx, int in_posy) {
	return in_posx > -1 && in_posx < 512 && in_posy > -1 && in_posy < 1024;
}


int PositionToMapId(int in_posx, int in_posy) {
    return (in_posy << 8) + (((++in_posx - (in_posy & 1)) >> 1));
}

map_pos_t MapIdToPosition(int MapID) {
	return getTileMapPosbyMapID(MapID);
}

/*
_MSG_RESP_SEND_RESHELP
b: 1
*/
void MapPosToPointCode(map_pos_t pos, uint16_t *zoneId, uint8_t *pointId) {
	uint16_t num = (uint16_t)pos.x;
	uint16_t num2 = (uint16_t)pos.y;
	uint16_t num3 = (uint16_t)pos.x;
	uint16_t num4 = (uint16_t)pos.y;
	
	num = (uint16_t)(num >> 5);
	num2 = (uint16_t)(num2 >> 4);
	uint16_t num5 = num;
	num5 += (uint16_t)(num2 << 4);
	num3 = (uint16_t)(num3 >> 1);
	num3 &= 15;
	num4 &= 15;
	uint8_t b = (uint8_t)num3;
	b += (uint8_t)(num4 << 4);
	
	*zoneId = num5;
	*pointId = b;
}

map_pos_t getTileMapPosbyPointCode(uint16_t zoneID, uint8_t pointID) {
	return getTileMapPosbyMapID(PointCodeToMapID(zoneID, pointID));
}


PointCode getPointCodeByMapPos(uint16_t x, uint16_t y) {
	PointCode DesPoint = {0};
	MapIDToPointCode(PositionToMapId(x, y), &DesPoint.zoneID, &DesPoint.pointID);
	return DesPoint;
}