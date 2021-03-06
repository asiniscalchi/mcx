/*
 * MallinCam Control
 * Copyright (C) 2012-2018 Andrew Galasso
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
# include "wx/wx.h"
#endif

#include "mcx.h"
#include "mcxcomm.h"
#include "crc16.h"

#include <stdio.h>

enum {
  CMD_SET = 0x21,
  CMD_GET = 0x31,

  RSP_OK = 0xA0,
  RSP_NE1 = 0xA2,
  RSP_NE2 = 0xA3,
  RSP_FAIL = 0xA4,
  RSP_OTHER = 0xA5,
};

void
mcxcmd_dump(char *buf, size_t len, const msg *msg)
{
  snprintf(buf, len,
	   "stx=%02x cmd=%02x ctrl=%02x data=(%02x %02x %02x %02x %02x %02x %02x %02x %02x "
	   "%02x %02x %02x %02x) etx=%02x crc=%02x%02x",
	   msg->stx, msg->cmdrsp, msg->ctrl,
	   msg->data[0], msg->data[1], msg->data[2],
	   msg->data[3], msg->data[4], msg->data[5],
	   msg->data[6], msg->data[7], msg->data[8],
	   msg->data[9], msg->data[10], msg->data[11], msg->data[12],
	   msg->etx, msg->crc_hi, msg->crc_lo);
}

static void
__init_cmd(msg *msg, u8 cmdrsp, u8 item, const u8 *data, size_t len)
{
  msg->stx = STX;
  msg->cmdrsp = cmdrsp;
  msg->ctrl = item;

  if (len > sizeof(msg->data))
    len = sizeof(msg->data);
  unsigned int i;
  for (i = 0; i < len; i++)
    msg->data[i] = data[i];
  for (; i < sizeof(msg->data); i++)
    msg->data[i] = 0;

  msg->etx = ETX;

  unsigned short crc = crc16(&msg->cmdrsp, (unsigned long) &msg->crc_hi - (unsigned long) &msg->cmdrsp);
  msg->crc_hi = (u8) (crc >> 8);
  msg->crc_lo = (u8) (crc & 0xff);
}

void
mcxcmd_set(msg *msg, u8 item, const u8 *data, size_t len)
{
  __init_cmd(msg, CMD_SET, item, data, len);
}

void
mcxcmd_get(msg *msg, u8 item, u8 subitem, u8 subsubitem)
{
    u8 const data[] = { subitem, subsubitem };
    __init_cmd(msg, CMD_GET, item, &data[0], sizeof(data));
}

static bool
_cmdrsp_ok(const msg *msg)
{
    return msg->cmdrsp == RSP_OK ||
        msg->cmdrsp == RSP_NE2 ||
        msg->cmdrsp == 0xa9;       // zoom responds with this sometimes?
}

int
mcxcmd_validate(const msg *msg)
{
    int ret = 0;

    if (msg->stx != STX) {
        WARN("bad stx");
        ret = -1;
    }

    if (!_cmdrsp_ok(msg))
    {
        WARN("bad cmdrsp 0x%x", msg->cmdrsp);
        ret = -2;
    }

    if (msg->etx != ETX) {
        WARN("bad etx");
        ret = -4;
    }

    unsigned short crc = crc16(&msg->cmdrsp, (unsigned long) &msg->crc_hi - (unsigned long) &msg->cmdrsp);
    unsigned short msgcrc = (msg->crc_hi << 8) | msg->crc_lo;
    if (msgcrc != crc) {
        WARN("bad crc");
        ret = -5;
    }

    return ret;
}
