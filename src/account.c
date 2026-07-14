#include "account.h"
#include "net_rw.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

AccountError LoadAccount(Connection *conn, const char *filename)
{
	int file = open(filename, O_RDONLY);
	
	if (file == -1) return ACC_ERR_FILE_OPEN;
	
	uint8_t data[1024];
	
	ssize_t r = read(file, data, sizeof(data));
	
	close(file);
	
	if (r <= 0) {
		return ACC_ERR_FILE_READ;
	}
	
	// minimum required size
	if (r < 580) {
		return ACC_ERR_TOO_SMALL;
	}
	
	uint16_t offset = 0;
	
	uint32_t magic   = read_u32(data + offset); offset += 4;
	uint32_t version = read_u32(data + offset); offset += 4;
	
	if (magic != ACCOUNT_MAGIC) {
		return ACC_ERR_MAGIC;
	}
	
	if (version != ACCOUNT_VERSION) {
		return ACC_ERR_VERSION;
	}
	
	conn->auth.igg_id = read_u64(data + offset);
    offset += 8;

    read_bytes(conn->auth.device_uuid, data + offset, 50);
    offset += 50;
    
    conn->auth.session_len = read_u16(data + offset);
    offset += 2;
    
    if (conn->auth.session_len == 0 || conn->auth.session_len > 512) return ACC_ERR_SESSION_INVALID;
    
    read_bytes(conn->auth.session, data + offset, 512);
    offset += 512;

    return ACC_OK;
}

const char *AccountErrorStr(AccountError err)
{
    switch (err) {
        case ACC_OK: return "OK";
        case ACC_ERR_FILE_OPEN: return "File open failed";
        case ACC_ERR_FILE_READ: return "File read failed";
        case ACC_ERR_TOO_SMALL: return "File too small";
        case ACC_ERR_MAGIC: return "Invalid magic";
        case ACC_ERR_VERSION: return "Invalid version";
        case ACC_ERR_SESSION_INVALID: return "Invalid session size";
        default: return "Unknown error";
    }
}