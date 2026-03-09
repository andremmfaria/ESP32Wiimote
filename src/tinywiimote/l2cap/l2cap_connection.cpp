// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "l2cap_connection.h"

void l2cap_clear_connections(L2capConnectionTable* table) {
  if (table == 0) {
    return;
  }
  table->size = 0;
}

int l2cap_find_connection(const L2capConnectionTable* table, uint16_t ch) {
  if (table == 0) {
    return -1;
  }

  for (int i = 0; i < table->size; i++) {
    if (table->list[i].ch == ch) {
      return i;
    }
  }
  return -1;
}

int l2cap_add_connection(L2capConnectionTable* table, l2cap_connection_t connection) {
  if (table == 0) {
    return -1;
  }

  if (table->size >= L2CAP_CONNECTION_LIST_SIZE) {
    return -1;
  }

  table->list[table->size++] = connection;
  return table->size;
}

int l2cap_get_remote_cid(const L2capConnectionTable* table, uint16_t ch, uint16_t* remoteCID) {
  const int idx = l2cap_find_connection(table, ch);
  if (idx < 0 || remoteCID == 0) {
    return -1;
  }

  *remoteCID = table->list[idx].remoteCID;
  return 0;
}
