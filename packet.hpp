#ifndef PACKET_H
#define PACKET_H

#include "des.h"

class Packet {
	private:
		uint8_t ENCRYPTION_KEY[9] = "L*#)@!&8";
		uint32_t seq_id;
		// int sock;
	public: 
		uint16_t zoneID;
		uint8_t pointID;
		
		int sock;
		uint8_t read_buffer[65535];
		uint16_t read_size;
		uint16_t read_offset;
		
		uint8_t write_buffer[65535];
		uint16_t write_size;
		uint16_t write_offset;
		
		Packet();
		
		void set_socket(int);
		
		void clear(void); // reset write data
		
		void write_u8(uint8_t); 
		void write_u16(uint16_t);
		void write_u32(uint32_t);
		void write_u64(uint64_t);
		void write_bytes(const void *, size_t);
		void write_seq_id(void);
		void reset_seq(void);
		
		
		void rewind(void); // reset reader offset to zero
		void reset(void); // reset reader offset and size
		uint8_t read_u8();
		uint16_t read_u16();
		uint32_t read_u32();
		uint64_t read_u64();
		
		uint16_t read_u16_be();
		uint32_t read_u32_be();
		uint64_t read_u64_be();
		
		int8_t read_i8();
		int16_t read_i16();
		int32_t read_i32();
		int64_t read_i64();
		
		int16_t read_i16_be();
		int32_t read_i32_be();
		int64_t read_i64_be();
		
		
		void read_bytes(void *, size_t);
		
		bool send(bool);
		
};

Packet::Packet() {
	seq_id = 0;
	
	read_size = 0;
	read_offset = 0;
	
	write_size = 0;
	write_offset = 0;
}

void Packet::set_socket(int fd) {
	sock = fd;
}

// clear writing data
void Packet::clear() {
	write_offset = 2;
	write_size = 2;
}

void Packet::write_u8(uint8_t value) {
	memcpy(write_buffer + write_offset, &value, sizeof(value));
	write_offset += sizeof(value);
	write_size += sizeof(value);
}

void Packet::write_u16(uint16_t value) {
	memcpy(write_buffer + write_offset, &value, sizeof(value));
	write_offset += sizeof(value);
	write_size += sizeof(value);
}

void Packet::write_u32(uint32_t value) {
	memcpy(write_buffer + write_offset, &value, sizeof(value));
	write_offset += sizeof(value);
	write_size += sizeof(value);
}

void Packet::write_u64(uint64_t value) {
	memcpy(write_buffer + write_offset, &value, sizeof(value));
	write_offset += sizeof(value);
	write_size += sizeof(value);
	
}

void Packet::write_bytes(const void *value, size_t len) {
	memcpy(write_buffer + write_offset, value, len);
	write_offset += len;
	write_size += len;
}


void Packet::write_seq_id() {
	seq_id += 1;
	write_u32(seq_id);
}


void Packet::reset() {
	read_offset = 0;
	read_size = 0;
}

uint8_t Packet::read_u8() {
	uint8_t value = 0;
	memcpy(&value, read_buffer + read_offset, sizeof(value));
	read_offset += sizeof(value);
	return value;
}

uint16_t Packet::read_u16() {
	uint16_t value = 0;
	memcpy(&value, read_buffer + read_offset, sizeof(value));
	read_offset += sizeof(value);
	return value;
}

uint32_t Packet::read_u32() {
	uint32_t value = 0;
	memcpy(&value, read_buffer + read_offset, sizeof(value));
	read_offset += sizeof(value);
	return value;
}

uint64_t Packet::read_u64() {
	uint64_t value = 0;
	memcpy(&value, read_buffer + read_offset, sizeof(value));
	read_offset += sizeof(value);
	return value;
}

void Packet::read_bytes(void *value, size_t len) {
	memcpy(value, read_buffer + read_offset, len);
	read_offset += len;
}

int16_t Packet::read_i16(void) {
	int16_t value = 0;
	memcpy(&value, read_buffer + read_offset, sizeof(value));
	read_offset += sizeof(value);
	return value;
}


bool Packet::send(bool encrypt) {
	memcpy(write_buffer, &write_size, 2);
	
	if (encrypt) {
		EncryptData(write_buffer + 4, write_size - 4, write_buffer + 4, ENCRYPTION_KEY);
	}
	
	return::send(sock, write_buffer, write_size, MSG_DONTWAIT) == write_size;
}



void Packet::reset_seq() {
    seq_id = 0;
}

void Packet::rewind() {
	read_offset = 0;
}

#endif // PACKET_H