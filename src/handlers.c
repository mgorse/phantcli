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

#include "phantcli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <bsd/md5.h>

static ServerDataHandler handlers[MAX_PACKET_COUNT];

static void
get_hash (int cookie, char *out)
{
  char buf[128];
	MD5_CTX context;
  unsigned char digest[16];
  int i;

  if (!cookie)
  {
    strcpy (out, "0");
    return;
  }

  sprintf (buf, "Impressive%d", cookie);

  MD5Init(&context);
  MD5Update(&context, (unsigned char *)buf, strlen (buf));
		MD5Final(digest, &context);

  for (i = 0; i < 16; i++)
    sprintf (out + i * 2, "%2x", digest[i]);
  digest[32] = '\0';
}

static pbool
handle_handshake (STATE *state, const char *buf)
{
  char hash[33];

#ifdef PHANT5
  respond (state, "BETA");
  respond (state, "010");
#endif
  respond (state, "1004");
  respond (state, "%d", state->cookie);
  get_hash (state->cookie, hash);
  respond (state, "%s", hash);
  respond (state, "%d", time (NULL));
  return FALSE;
}

static pbool
handle_close (STATE *state, const char *buf)
{
  return FALSE;
}

static pbool
handle_ping (STATE *state, const char *buf)
{
  send_string_f (state, "%d", C_PING_PACKET);
  ui_timeout (state);
  return FALSE;
}

static pbool
handle_add_player (STATE *state, const char *buf)
{
  PLAYER *player;
  PLAYER *p;

  if (!buf)
    return TRUE;

  if (state->line_count++ == 0)
  {
    player = (PLAYER *) calloc (sizeof (PLAYER), 1);
    player->name = strdup (buf);
    if (!state->players)
    {
      state->players = player;
      return TRUE;
    }
    p = state->players;
    while (p->next)
      p = p->next;
    p->next = player;
    return TRUE;
  }
  else
  {
    for (p = state->players; p->next; p = p->next);
    p->type = strdup (buf);
    return FALSE;
  }
}

static pbool
handle_remove_player (STATE *state, const char *buf)
{
  PLAYER *player = state->players;
  PLAYER *prev = NULL;;

  if (!buf)
    return TRUE;

  while (player)
  {
    if (!strcmp (player->name, buf))
    {
      if (prev)
        prev->next = player->next;
      else
        state->players = player->next;
      free (player->name);
      free (player);
      return FALSE;
    }
    prev = player;
    player = player->next;
  }

  return FALSE;
}

static pbool
handle_shutdown (STATE *state, const char *buf)
{
  ui_writeline (state, "The game is shutting down. Press any key to exit..");
  ui_get_key (state);
  return FALSE;
}

static pbool
handle_error (STATE *state, const char *buf)
{
  if (!buf)
    return TRUE;
  fprintf (stderr, "ERROR: %s\n", buf);
  return FALSE;
}

static pbool
handle_clear (STATE *state, const char *buf)
{
  ui_clear (state);
  return FALSE;
}

static pbool
handle_writeline (STATE *state, const char *buf)
{
  if (!buf)
    return TRUE;
  ui_writeline (state, buf);
  return FALSE;
}

static pbool
handle_buttons (STATE *state, const char *buf)
{
  if (!buf)
  {
    int i;
    for (i = 0; i < 8; i++)
    {
      free (state->buttons[i]);
      state->buttons[i] = NULL;
    }
    state->dialog_mode = state->cur_packet;
    return TRUE;
  }
  if (buf[0])
    state->buttons[state->line_count] = strdup (buf);
  state->line_count++;
  if (state->line_count < 8)
    return TRUE;
  ui_present_dialog (state);
  return FALSE;
}

static pbool
handle_string_dialog (STATE *state, const char *buf)
{
  if (!buf)
    return TRUE;

  state->dialog_mode = state->cur_packet;
  ui_present_string_dialog (state, buf);
  return FALSE;
}

static pbool
handle_scoreboard_dialog (STATE *state, const char *buf)
{
  if (!buf)
    return TRUE;

  switch (state->line_count++)
  {
  case 0:
    /* start */
    return TRUE;
  case 1:
    state->lines_expected = 0;
    sscanf (buf, "%d", &state->lines_expected);
    return TRUE;
  default:
    ui_post_special_text (state, buf);
    if (state->line_count >= state->lines_expected + 2)
    {
      ui_post_special_text (state, NULL);
      return FALSE;
    }
    return TRUE;
  }
}

static pbool
handle_chat (STATE *state, const char *buf)
{
  if (!buf)
    return TRUE;
  ui_chat_message (state, buf);
  return FALSE;
}

static pbool
handle_activate_chat (STATE *state, const char *buf)
{
  ui_chat_enable (state, 1);
  return FALSE;
}

static pbool
handle_deactivate_chat (STATE *state, const char *buf)
{
  ui_chat_enable (state, 0);
  return FALSE;
}

static const char *player_info_records[] =
{
  "TItle",
  "Location",
  "Account",
  "Network",
  "Channel",
  "Level",
  "Experience",
  "Next level",
  "Energy",
  "Max energy",
  "Shield",
  "Strength",
  "Max strength",
  "Sword",
  "Quickness",
  "Max quickness",
  "Quicksilver",
  "Brains",
  "Magic level",
  "Mana",
  "Gender",
  "Poison",
  "Sin",
  "Lives",
  "Gold",
  "Gems",
  "Holy water",
  "Amulets",
  "Charms",
  "Crowns",
  "Virgin",
  "Blessing",
  "Palantir",
  "Ring",
#ifdef PHANT5
  "Staff",
#endif
  "Cloaked",
  "Blind",
  "Age",
  "Degenerated",
  "Time played",
  "Date loaded",
  "Date created",
  NULL
};

static pbool
handle_player_info (STATE *state, const char *buf)
{
  char out[256];

  if (!buf)
    return TRUE;

  snprintf (out, sizeof (out), "%s: %s", player_info_records[state->line_count++], buf);
  out[sizeof(out) - 1] = '\0';
  ui_post_special_text (state, out);

  if (!player_info_records[state->line_count])
  {
    ui_post_special_text (state, NULL);
    return FALSE;
  }

  return TRUE;
}

static pbool
handle_examine (STATE *state, const char *buf)
{
  if (!buf)
    return TRUE;

  switch (state->line_count++)
  {
  case 0:
    state->lines_expected = 0;
    sscanf (buf, "%d", &state->lines_expected);
    return TRUE;
  default:
    ui_post_special_text (state, buf);
    if (state->line_count >= state->lines_expected + 1)
    {
      ui_post_special_text (state, NULL);
      return FALSE;
    }
    return TRUE;
  }
}

static pbool
handle_name (STATE *state, const char *buf)
{
  if (!buf)
  {
    free (state->player.name);
    state->player.name = NULL;
    return TRUE;
  }
  if (buf[0])
    state->player.name = strdup (buf);
  ui_update_stat (state, state->cur_packet);
  return FALSE;
}

static pbool
handle_location (STATE *state, const char *buf)
{
  if (!buf)
  {
    free (state->player.location);
    state->player.location = NULL;
    return TRUE;
  }
  switch (state->line_count++)
  {
  case 0:
    state->player.y = atoi (buf);
    return TRUE;
  case 1:
    state->player.x = atoi (buf);
    return TRUE;
  case 2:
    state->player.location = strdup (buf);
    ui_update_stat (state, state->cur_packet);
    return FALSE;
  default: /* shouldn't reach here */
    return FALSE;
  }
}

static pbool
handle_int_array (STATE *state, const char *buf, int *out, int count)
{
  if (!buf)
    return TRUE;

  out[state->line_count++] = atoi (buf);
  if (state->line_count >= count)
  {
    ui_update_stat (state, state->cur_packet);
    return FALSE;
  }

  return TRUE;
}

static pbool
handle_energy (STATE *state, const char *buf)
{
  return handle_int_array (state, buf, state->player.energy, 3);
}

static pbool
handle_strength (STATE *state, const char *buf)
{
  return handle_int_array (state, buf, state->player.strength, 2);
}

static pbool
handle_speed (STATE *state, const char *buf)
{
  return handle_int_array (state, buf, state->player.speed, 2);
}

static pbool
handle_int_val (STATE *state, const char *buf, int *out)
{
  if (!buf)
    return TRUE;
  *out = atoi (buf);
  ui_update_stat (state, state->cur_packet);
  return FALSE;
}

static pbool
handle_shield (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.shield);
}

static pbool
handle_sword (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.sword);
}

static pbool
handle_quicksilver (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.quicksilver);
  return FALSE;
}

static pbool
handle_mana (STATE *state, const char *buf)
{
#ifdef PHANT5
  return handle_int_array (state, buf, state->player.mana, 2);
#else
  return handle_int_array (state, buf, state->player.mana, 1);
#endif
}

static pbool
handle_level (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.level);
}

static pbool
handle_gold (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.gold);
}

static pbool
handle_gems (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.gems);
}

static pbool
handle_bool_val (STATE *state, const char *buf, int *out)
{
  if (!buf)
    return TRUE;
  if (!strcmp (buf, "Yes"))
    *out = 1;
  else if (!strcmp (buf, "No"))
    *out = 0;
  else
    dlog ("ERROR: %s: Unexpected value %s, packet %d\n", __func__, buf, state->cur_packet);
  ui_update_stat (state, state->cur_packet);
  return FALSE;
}

static pbool
handle_cloak (STATE *state, const char *buf)
{
  return handle_bool_val (state, buf, &state->player.cloak);
}

static pbool
handle_blessing (STATE *state, const char *buf)
{
  return handle_bool_val (state, buf, &state->player.blessing);
}

static pbool
handle_crown (STATE *state, const char *buf)
{
  return handle_bool_val (state, buf, &state->player.crown);
}

static pbool
handle_palantir (STATE *state, const char *buf)
{
  return handle_bool_val (state, buf, &state->player.palantir);
}

static pbool
handle_ring (STATE *state, const char *buf)
{
  return handle_bool_val (state, buf, &state->player.ring);
}

static pbool
handle_virgin (STATE *state, const char *buf)
{
  return handle_bool_val (state, buf, &state->player.virgin);
}

static pbool
handle_timed_ping (STATE *state, const char *buf)
{
  send_string_f (state, "%d", C_PONG_PACKET);
  return FALSE;
}

static pbool
handle_amulets (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.amulets);
}

static pbool
handle_charms (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.charms);
}

static pbool
handle_tokens (STATE *state, const char *buf)
{
  return handle_int_val (state, buf, &state->player.tokens);
}

static pbool
handle_staff (STATE *state, const char *buf)
{
  /* TODO: ui.c has no support for this */
  return handle_bool_val (state, buf, &state->player.staff);
}

void
init_handlers ()
{
  handlers[HANDSHAKE_PACKET] = handle_handshake;
  handlers[CLOSE_CONNECTION_PACKET] = handle_close;
  handlers[PING_PACKET] = handle_ping;
  handlers[ADD_PLAYER_PACKET] = handle_add_player;
  handlers[REMOVE_PLAYER_PACKET] = handle_remove_player;
  handlers[SHUTDOWN_PACKET] = handle_shutdown;
  handlers[ERROR_PACKET] = handle_error;
  handlers[CLEAR_PACKET] = handle_clear;
  handlers[WRITE_LINE_PACKET] = handle_writeline;
  handlers[BUTTONS_PACKET] = handle_buttons;
  handlers[FULL_BUTTONS_PACKET] = handle_buttons;
  handlers[STRING_DIALOG_PACKET] = handle_string_dialog;
  handlers[COORDINATES_DIALOG_PACKET] = handle_string_dialog;
  handlers[PLAYER_DIALOG_PACKET] = handle_string_dialog;
  handlers[PASSWORD_DIALOG_PACKET] = handle_string_dialog;
  handlers[SCOREBOARD_DIALOG_PACKET] = handle_scoreboard_dialog;
  handlers[CHAT_PACKET] = handle_chat;
  handlers[ACTIVATE_CHAT_PACKET] = handle_activate_chat;
  handlers[DEACTIVATE_CHAT_PACKET] = handle_deactivate_chat;
  handlers[PLAYER_INFO_PACKET] = handle_player_info;
  handlers[EXAMINE_PACKET] = handle_examine;
  handlers[NAME_PACKET] = handle_name;
  handlers[LOCATION_PACKET] = handle_location;
  handlers[ENERGY_PACKET] = handle_energy;
  handlers[STRENGTH_PACKET] = handle_strength;
  handlers[SPEED_PACKET] = handle_speed;
  handlers[SHIELD_PACKET] = handle_shield;
  handlers[SWORD_PACKET] = handle_sword;
  handlers[QUICKSILVER_PACKET] = handle_quicksilver;
  handlers[MANA_PACKET] = handle_mana;
  handlers[LEVEL_PACKET] = handle_level;
  handlers[GOLD_PACKET] = handle_gold;
  handlers[GEMS_PACKET] = handle_gems;
  handlers[CLOAK_PACKET] = handle_cloak;
  handlers[BLESSING_PACKET] = handle_blessing;
  handlers[CROWN_PACKET] = handle_crown;
  handlers[PALANTIR_PACKET] = handle_palantir;
  handlers[RING_PACKET] = handle_ring;
  handlers[VIRGIN_PACKET] = handle_virgin;
  handlers[TIMED_PING_PACKET] = handle_timed_ping;
  handlers[AMULETS_PACKET] = handle_amulets;
  handlers[CHARMS_PACKET] = handle_charms;
  handlers[TOKENS_PACKET] = handle_tokens;
  handlers[STAFF_PACKET] = handle_staff;
}

pbool
handle_packet (STATE *state, const char *buf)
{
  int type = 0;
  int ret;

  dlog ("%s: %s\n", __func__, buf);
  sscanf (buf, "%d", &type);
  if (type == 0)
  {
    fprintf (stderr, "%s: unexpected line %s\n", __func__, buf);
    return -1;
  }
  if (type < 0 || type >= MAX_PACKET_COUNT)
  {
    fprintf(stderr, "%s: bad packet number %d\n", __func__, type);
    return -1;
  }

  state->cur_packet = type;
  state->line_count = 0;
  if (!handlers[type])
  {
    fprintf (stderr, "%s: No handler for packet %d\n", __func__, type);
    return -1;
  }

  ret = handlers[type] (state, NULL);

  if (ret == 1)
    state->sdh = handlers[type];

  return ret;
}

