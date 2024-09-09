/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/sys/ring_buffer.h>

#include <zmk/hid.h>
#include <zmk/hid_rpc.h>
#include <zmk/endpoints.h>
#include <zmk/studio/rpc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

static bool handling_hid_rx = false;

static int hid_start_rx(void) {
    handling_hid_rx = true;
    return 0;
}

static int hid_stop_rx(void) {
    handling_hid_rx = false;
    return 0;
}

static void *hid_tx_user_data(void) { return NULL; }

static void hid_tx_notify(struct ring_buf *buf, size_t added, bool msg_done, void *user_data) {
    uint8_t *tx_buf = zmk_hid_get_rpc_report()->data;
    uint32_t tx_len = 0;
    uint8_t *claim_buf;
    uint32_t claim_len;
    int ret;

    memset(tx_buf, 0, ZMK_HID_RPC_REPORT_COUNT);

    if (msg_done || ring_buf_size_get(buf) >= ZMK_HID_RPC_REPORT_COUNT) {
        while (tx_len < ZMK_HID_RPC_REPORT_COUNT && ring_buf_size_get(buf) > 0) {
            claim_len = ring_buf_get_claim(buf, &claim_buf, ZMK_HID_RPC_REPORT_COUNT - tx_len);
            memcpy(tx_buf + tx_len, claim_buf, claim_len);
            tx_len += claim_len;
            ring_buf_get_finish(buf, claim_len);
        }

        ret = zmk_endpoints_send_rpc_report();
        if (ret) {
            LOG_ERR("Failed to write to HID transport: %d", ret);
        }
    }
}

ZMK_RPC_TRANSPORT(hid, ZMK_TRANSPORT_USB, hid_start_rx, hid_stop_rx, hid_tx_user_data,
                  hid_tx_notify);

void zmk_hid_rpc_process_report(uint8_t *data, size_t len, enum zmk_transport transport) {
    struct ring_buf *buf = zmk_rpc_get_rx_buf();
    uint32_t copied = 0;
    uint8_t *claim_buf;
    uint32_t claim_len;

    if (transport == ZMK_TRANSPORT_USB && !handling_hid_rx) {
        return;
    }

    while (copied < len) {
        claim_len = ring_buf_put_claim(buf, &claim_buf, len - copied);

        if (claim_len > 0) {
            memcpy(claim_buf, data + copied, claim_len);
            copied += claim_len;
        }

        ring_buf_put_finish(buf, claim_len);
    }

    zmk_rpc_rx_notify();
}
