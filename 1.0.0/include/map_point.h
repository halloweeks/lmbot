#ifndef _MAP_POINT_
#define _MAP_POINT_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint16_t x;
	uint16_t y;
} map_pos_t;

typedef struct {
	uint16_t zoneID;
	uint8_t pointID;
} PointCode;

int TileMapPosToMapID(int in_posx, int in_posy);
int PointCodeToMapID(uint16_t zoneID, uint8_t pointID);
void MapIDToPointCode(int mapID, uint16_t *zoneID, uint8_t *pointID);
map_pos_t getTileMapPosbyMapID(int mapID);
bool CheckTileMapPos(int in_posx, int in_posy);
int PositionToMapId(int in_posx, int in_posy);
map_pos_t MapIdToPosition(int MapID);
void MapPosToPointCode(map_pos_t pos, uint16_t *zoneId, uint8_t *pointId);
map_pos_t getTileMapPosbyPointCode(uint16_t zoneID, uint8_t pointID);
PointCode getPointCodeByMapPos(uint16_t x, uint16_t y);

#endif