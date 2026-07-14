#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdint.h>
#include <stdbool.h>
#include "connection.h"

#define ACCOUNT_MAGIC   0x44524f4c
#define ACCOUNT_VERSION 1

typedef enum {
    ACC_OK = 0,

    ACC_ERR_FILE_OPEN,
    ACC_ERR_FILE_READ,
    ACC_ERR_TOO_SMALL,

    ACC_ERR_MAGIC,
    ACC_ERR_VERSION,

    ACC_ERR_SESSION_INVALID,
    ACC_ERR_PARSE
} AccountError;

AccountError LoadAccount(Connection *conn, const char *filename);
const char *AccountErrorStr(AccountError err);

#endif