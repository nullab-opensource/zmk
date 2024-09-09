/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <zmk/endpoints_types.h>

void zmk_hid_rpc_process_report(uint8_t *data, size_t len, enum zmk_transport transport);
