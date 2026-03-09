// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "l2cap_connection.h"

static l2cap_connection_t g_l2capConnectionList[L2CAP_CONNECTION_LIST_SIZE];
static int g_l2capConnectionSize = 0;

void l2cap_clear_connections(void) {
  g_l2capConnectionSize = 0;
}

int l2cap_find_connection(uint16_t ch) {
  for (int i = 0; i < g_l2capConnectionSize; i++) {
    if (g_l2capConnectionList[i].ch == ch) {
      return i;
    }
  }
  return -1;
}

int l2cap_add_connection(l2cap_connection_t connection) {
  if (g_l2capConnectionSize >= L2CAP_CONNECTION_LIST_SIZE) {
    return -1;
  }

  g_l2capConnectionList[g_l2capConnectionSize++] = connection;
  return g_l2capConnectionSize;
}

int l2cap_get_remote_cid(uint16_t ch, uint16_t* remoteCID) {
  const int idx = l2cap_find_connection(ch);
  if (idx < 0 || remoteCID == 0) {
    return -1;
  }

  *remoteCID = g_l2capConnectionList[idx].remoteCID;
  return 0;
}
