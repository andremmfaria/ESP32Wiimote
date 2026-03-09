// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __L2CAP_CONNECTION_H__
#define __L2CAP_CONNECTION_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint16_t ch;
        uint16_t remoteCID;
    } l2cap_connection_t;

#define L2CAP_CONNECTION_LIST_SIZE 8

    typedef struct
    {
        l2cap_connection_t list[L2CAP_CONNECTION_LIST_SIZE];
        int size;
    } L2capConnectionTable;

    void l2cap_clear_connections(L2capConnectionTable *table);
    int l2cap_find_connection(const L2capConnectionTable *table, uint16_t ch);
    int l2cap_add_connection(L2capConnectionTable *table, l2cap_connection_t connection);
    int l2cap_get_remote_cid(const L2capConnectionTable *table, uint16_t ch, uint16_t *remoteCID);

#ifdef __cplusplus
}
#endif

#endif // __L2CAP_CONNECTION_H__
