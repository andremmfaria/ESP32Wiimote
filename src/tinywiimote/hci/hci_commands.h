// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __TINYWIIMOTE_HCI_COMMANDS_H__
#define __TINYWIIMOTE_HCI_COMMANDS_H__

#include "../utils/hci_utils.h"

#include <stdint.h>

uint16_t make_cmd_reset(uint8_t* buf);
uint16_t make_cmd_read_bd_addr(uint8_t* buf);
uint16_t make_cmd_write_local_name(uint8_t* buf, const uint8_t* name, uint8_t len);
uint16_t make_cmd_write_class_of_device(uint8_t* buf, const uint8_t* cod);
uint16_t make_cmd_write_scan_enable(uint8_t* buf, uint8_t mode);
uint16_t make_cmd_inquiry(uint8_t* buf, uint32_t lap, uint8_t len, uint8_t num);
uint16_t make_cmd_inquiry_cancel(uint8_t* buf);
uint16_t make_cmd_remote_name_request(uint8_t* buf,
                                      struct bd_addr_t bdAddr,
                                      uint8_t psrm,
                                      uint16_t clkofs);
uint16_t make_cmd_create_connection(uint8_t* buf,
                                    struct bd_addr_t bdAddr,
                                    uint16_t pt,
                                    uint8_t psrm,
                                    uint16_t clkofs,
                                    uint8_t ars);

#endif  // __TINYWIIMOTE_HCI_COMMANDS_H__
