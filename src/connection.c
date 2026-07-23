#include "connection.h"

int set_nonblocking(Connection *conn)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(conn->sock, FIONBIO, &mode);
#else
    int flags = fcntl(conn->sock, F_GETFL, 0);
    if (flags < 0) return -1;

    return fcntl(conn->sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

int connect_server(const char *ip, unsigned short port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close_socket(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close_socket(sock);
        return -1;
    }

    return sock;
}

bool send_packet(Connection *conn, bool enc)
{
	if (enc) {
		EncryptData(conn->data + 4, conn->size - 4, conn->data + 4, ENCRYPTION_KEY);
	}
	
    return send(conn->sock, conn->data, conn->size, 0) == conn->size;
}

void disconnect(Connection *c)
{
    if (c->sock >= 0)
        close_socket(c->sock);
        c->sock = -1;
}


void reset_connection(Connection *c)
{
    c->sock = -1;
    
    memset(&c, 0, sizeof(c));
}