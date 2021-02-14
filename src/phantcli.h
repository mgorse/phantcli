/*
 * Copyright (C) 2021 by Mike Gorse.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "packet.h"

#include <stdarg.h>

/* Let's define our own bool, in order to avoid conflicts with bool from
 * anywhere else (ie, ncurses) */
typedef int pbool;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* Used for array sizes. Actually 57, as of v1004 */
#define MAX_PACKET_COUNT 64

typedef struct player PLAYER;

typedef struct state STATE;

typedef pbool (*ServerDataHandler) (STATE *, const char *buf);

struct player
{
  char *name;
  char *type;
  struct player *next;
};

struct state
{
  int fd;
  int cookie;
  char buf[1024]; /* for socket data */
  int bufpos;
  ServerDataHandler sdh;
  int cur_packet;
  int line_count; /* used when reading certain packets */
  int lines_expected; /* used when reading scoreboard */
  int dialog_mode;
  char *buttons[8];
  struct UI *ui;
  struct
  {
    char *name;
    char *location;
    int x;
    int y;
    int energy[3];
    int strength[2];
    int speed[2];
    int shield;
    int sword;
    int quicksilver;
    int mana[2];
    int level;
    int gold;
    int gems;
    pbool cloak;
    pbool blessing;
    pbool crown;
    pbool palantir;
    pbool ring;
    pbool virgin;
    int amulets;
    int charms;
    int tokens;
    pbool staff;
  } player;
  PLAYER *players;
};

void dlog (const char *fmt, ...);
void respond (STATE *state, const char *fmt, ...);
void respondv (STATE *state, const char *fmt, va_list args);
void send_string (STATE *state, const char *buf);
void send_string_f (STATE *state, const char *fmt, ...);
void send_string_fv (STATE *state, const char *fmt, va_list args);

pbool handle_packet (STATE *state, const char *buf);
void init_handlers ();

void ui_init (STATE *state);
void ui_teardown (STATE *state);
void ui_writeline (STATE *state, const char *buf);
void ui_present_dialog (STATE *state);
void ui_present_string_dialog (STATE *state, const char *buf);
void ui_get_key (STATE *state);
void ui_clear (STATE *state);
void ui_update_stat (STATE *state, int packet);
void ui_chat_enable (STATE *state, pbool enable);
void ui_chat_message (STATE *state, const char *message);
void ui_timeout (STATE *state);
void ui_post_special_text (STATE *state, const char *buf);
