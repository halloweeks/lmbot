
#include "connection.h"

// Bootstrap login
void RequestGuestLogIn(Connection *c)
{
	// reserve space for packet length
    c->size = 2;
    
    // write packet type 
    write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOL);
    c->size += 2;
    
    // write igg id
    write_u64(c->data + c->size, c->auth.igg_id);
    c->size += 8;
    
    // write game minor version 
    write_u8(c->data + c->size, c->app.version_minor);
    c->size += 1;
    
    // write game major version
    write_u8(c->data + c->size, c->app.version_major);
    c->size += 1;
    
    // write game patch version
    write_u16(c->data + c->size, c->app.version_patch);
    c->size += 2;

    write_u8(c->data + c->size, 1);
    c->size += 1;
    
    // write language code
    write_u8(c->data + c->size, c->app.language_code);
    c->size += 1;
    
    // write Device Universally Unique Identifier.
    write_raw(c->data + c->size, c->auth.device_uuid, 50);
    c->size += 50;
    
    // write session length
    write_u16(c->data + c->size, c->auth.session_len);
    c->size += 2;
    
    // write session token
    write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;
    
    // rewrite total packet length 
    write_u16(c->data, c->size);
    
    // call send packet
    send_packet(c, false);
}


// Game login
void RequestLogIn(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOP);
	c->size += 2;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	// write 50 byte zero 
	for (int i = 0; i < 25; i++) {
		write_u16(c->data + c->size, 0);
		c->size += 2;
	}
	
	// write version 
	write_u32(c->data + c->size, 0);
	c->size += 4;
	
	// battle_is_oul
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// b_recv_kingdom
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// session len
	write_u16(c->data + c->size, c->auth.session_len);
	c->size += 2;
	
	// session 
	write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;

    write_u16(c->data, c->size);
    
    send_packet(c, false);
}

// 
void RequestClientInitOver(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_CLIENTINITOVER);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	write_u16(c->data, c->size);
    
    send_packet(c, true);
}