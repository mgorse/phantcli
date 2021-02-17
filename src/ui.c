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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <termios.h>
#include <ctype.h>

typedef struct
{
  const char *label;
  int row;
  int label_col;
  int data_col;
} STAT;

#define MSGROWS 6

typedef struct UI UI;
struct UI
{
  int nrows;
  int ncols;
  WINDOW *msgwin;
  WINDOW *dlgwin;
  WINDOW *locwin;
  WINDOW *statwin;
  WINDOW *chatwin;
  WINDOW *chatrespwin;
  char kbuf[256];
  int kbufpos;
  char chatkbuf[256];
  int chatkbufpos;
  int inpline;
  char *msglin[MSGROWS];
  int msgpos;
  int chatmode;
  char **special_text;
  int special_text_size;
  int special_text_len;
  int special_text_pos;
  pbool is_class_dlg;
  int class;
};

static STAT stats[MAX_PACKET_COUNT];

static void
add_stat (STATE *state, int index, const char *label)
{
  static int row = 0, col = 0;

  if (row >= 8)
    return; /* no room for it */

  stats[index].label = label;
  stats[index].row = row;
  stats[index].label_col = col * 40;
  stats[index].data_col = stats[index].label_col + 15;

  col++;
  if (col >= state->ui->ncols / 40)
  {
    row++;
    col = 0;
  }
}

void
ui_init (STATE *state)
{
  struct termios tty;

  state->ui = (UI *) calloc (sizeof (UI), 1);
	initscr();		/* turn on curses */
	noecho();		/* do not echo input */
	cbreak();		/* do not process erase, kill */
#ifdef NCURSES_VERSION /* Ncurses needs some terminal mode fiddling */
	tcgetattr(0, &tty);
	tty.c_iflag |= ICRNL;
	tcsetattr(0, TCSANOW, &tty);
#endif
  state->ui->nrows = getmaxy (stdscr);
  state->ui->ncols = getmaxx (stdscr);

  add_stat (state, ENERGY_PACKET, "Energy");
  add_stat (state, STRENGTH_PACKET, "Strength");
  add_stat (state, SPEED_PACKET, "Speed");
  add_stat (state, SHIELD_PACKET, "Shield");
  add_stat (state, SWORD_PACKET, "Sword");
  add_stat (state, QUICKSILVER_PACKET, "Quicksilver");
  add_stat (state, MANA_PACKET, "Mana");
  add_stat (state, LEVEL_PACKET, "Level");
  add_stat (state, GOLD_PACKET, "Gold");
  add_stat (state, GEMS_PACKET, "Gems");
  add_stat (state, CLOAK_PACKET, "Cloak");
  add_stat (state, BLESSING_PACKET, "Blessing");
  add_stat (state, CROWN_PACKET, "Crowns");
  add_stat (state, PALANTIR_PACKET, "Palantir");
  add_stat (state, RING_PACKET, "Ring");
  add_stat (state, VIRGIN_PACKET, "Virgin");
#ifdef PHANT5
  add_stat (state, AMULETS_PACKET, "Amulets");
  add_stat (state, CHARMS_PACKET, "Charms");
  add_stat (state, TOKENS_PACKET, "Tokens");
#endif
  
	clear();
	refresh();
  state->ui->msgwin = newwin (MSGROWS, state->ui->ncols, 1, 0);
  scrollok (state->ui->msgwin, TRUE);
  idlok (state->ui->msgwin, TRUE);
  state->ui->dlgwin = newwin (2, state->ui->ncols, MSGROWS + 3, 0);
  state->ui->locwin = newwin (1, state->ui->ncols, 0, 0);
}

/* Moves the cursor to where it should be, if necessary. This is needed when
 * updating the screen while in chat mode, or when entering or exiting chat
 * mode. */
static void
fix_cursor (STATE *state)
{
  if (state->ui->chatmode)
  {
    wmove (state->ui->chatrespwin, 0, state->ui->chatkbufpos);
    wrefresh (state->ui->chatrespwin);
  }
  else if (state->dialog_mode == STRING_DIALOG_PACKET || state->dialog_mode == COORDINATES_DIALOG_PACKET || state->dialog_mode == PLAYER_DIALOG_PACKET || state->dialog_mode == PASSWORD_DIALOG_PACKET)
  {
    wmove (state->ui->msgwin, state->ui->inpline, state->ui->kbufpos);
    wrefresh (state->ui->msgwin);
  }
  else if (state->dialog_mode == BUTTONS_PACKET || state->dialog_mode == FULL_BUTTONS_PACKET)
  {
    wmove (state->ui->dlgwin, 0, 0);
    wrefresh (state->ui->dlgwin);
  }
}

void
ui_writeline (STATE *state, const char *buf)
{
  int curpos = 0, count;
  int i;

  while (buf[curpos])
  {
    count = strlen (buf + curpos);
    if (count > state->ui->ncols)
    {
      count = state->ui->ncols;
      while (count > 0 && buf[curpos + count] != ' ')
        count--;
      if (!count)
        count = state->ui->ncols;
    }
    if (!state->ui->special_text_pos)
    {
      waddnstr (state->ui->msgwin, buf + curpos, count);
      waddch (state->ui->msgwin, '\n');
    }
    if (state->ui->msgpos == MSGROWS)
    {
      free (state->ui->msglin[0]);
      for (i = 0; i < MSGROWS - 1; i++)
        state->ui->msglin[i] = state->ui->msglin[i + 1];
      state->ui->msgpos--;
    }
    state->ui->msglin[state->ui->msgpos] = (char *) malloc(count + 1);
    memcpy (state->ui->msglin[state->ui->msgpos], buf + curpos, count);
    state->ui->msglin[state->ui->msgpos][count] = '\0';
    curpos += count;
    state->ui->msgpos++;
  }
  wrefresh (state->ui->msgwin);
}

static int
is_more_prompt (STATE *state)
{
  int i;

  if (!state->buttons[0])
    return FALSE;
  for (i = 1; i < 8; i++)
    if (state->buttons[i])
      return FALSE;
  return TRUE;
}

static pbool
is_class_dlg (STATE *state)
{
  return (state->buttons[0] && !strcmp (state->buttons[0], "Magic-User"));
}

static pbool
do_reroll (STATE *state)
{
  if (!state->buttons[0] || strcmp (state->buttons[0], "Reroll") != 0)
    return FALSE;

  switch (state->ui->class)
  {
  case 2: /* fighter */
    return state->player.speed[0] < 37 || state->player.strength[0] < 42;
  default:
    return FALSE;
  }
}

void
ui_present_dialog (STATE *state)
{
  char buf[256];
  int i;
  int curcol = 0;
  int len;

  if (state->ui->special_text_pos > 0)
    return;

  werase (state->ui->dlgwin);

  state->ui->is_class_dlg = is_class_dlg (state);

  if (is_more_prompt (state))
  {
    sprintf (buf, "--%s--", state->buttons[0]);
    waddstr (state->ui->dlgwin, buf);
    wrefresh (state->ui->dlgwin);
    return;
  }

  if (do_reroll (state))
  {
    respond (state, "0");
    state->dialog_mode = 0;
    return;
  }

  for (i = 0; i < 8; i++)
  {
    if (state->buttons[i])
    {
      sprintf (buf, "%d=%s", i + 1, state->buttons[i]);
      len = strlen (buf);
      if (curcol + len + 1 >= state->ui->ncols)
      {
        waddch (state->ui->dlgwin, '\n');
        curcol = 0;
      }
      if (curcol > 0)
      {
        waddch (state->ui->dlgwin, ' ');
        curcol++;
      }
      waddstr (state->ui->dlgwin, buf);
      curcol += len;
    }
  }
  waddstr (state->ui->dlgwin, "> ");
  wrefresh (state->ui->dlgwin);
}

void
ui_present_string_dialog (STATE *state, const char *buf)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  int x;
#pragma GCC diagnostic pop

  ui_writeline (state, buf);
  state->ui->kbufpos = 0;
  memset (state->ui->kbuf, 0, sizeof (state->ui->kbuf));
  getyx (state->ui->msgwin, state->ui->inpline, x);
}

static void
end_dialog (STATE *state, const char *fmt, ...)
{
  va_list args;
  int x, y;

  dlog ("end_dialog: %s\n", fmt);
  va_start (args, fmt);
  if (state->dialog_mode == COORDINATES_DIALOG_PACKET)
  {
    char *buf;
    buf = va_arg (args, char *);
    if (sscanf (buf, "%d %d", &x, &y) != 2)
    {
      ui_writeline (state, "Must enter x y coordinates");
    }
    else
    {
      respond (state, "%d", x);
      respond (state, "%d", y);
    }
  }
  else
    respondv (state, fmt, args);
  va_end (args);
  if (state->dialog_mode == BUTTONS_PACKET)
  {
    werase (state->ui->dlgwin);
    wrefresh (state->ui->dlgwin);
  }
  state->dialog_mode = 0;
}

static int
getkey (const char *buf, int size)
{
  int key;
  const unsigned char *ubuf = (const unsigned char *)buf;

  key = (int) ubuf[0];
  if (size > 1)
    key = (key << 8) + ubuf[1];
  if (size > 2)
    key = (key << 8) + ubuf[2];
  if (size > 3)
    key = (key << 8) + ubuf[3];

  return key;
}

static void
add_key (STATE *state, WINDOW *win, int top, char *buf, int *bufpos, int bufsize, int password, int ch)
{
  int pos = *bufpos;
  int row, col;
  int row2, col2;
  int end;

  switch (ch)
  {
  case 0x7f:
    if (pos > 0)
    {
      memmove (buf + pos - 1, buf + pos, strlen (buf + pos) + 1);
      pos--;
      row = pos / state->ui->ncols;
      col = pos % state->ui->ncols;
      if (password)
        mvwaddstr (win, row + top, col, buf + pos);
      end = strlen (buf);
      row2 = end / state->ui->ncols;
      col2 = end % state->ui->ncols;
      mvwaddch (win, row2 + top, col2, ' ');
      wmove (win, row + top, col);
      wrefresh (win);
    }
    break;
  default:
    if (pos < bufsize - 1)
    {
      memmove (buf + pos + 1, buf + pos, strlen (buf + pos));
      buf[bufsize - 1] = '\0';
    }
    buf[pos] = ch;
    row = pos / state->ui->ncols;
    col = pos% state->ui->ncols;
    if (password)
      mvwaddch (win, row + top, col, '*');
    else
      mvwaddstr (win, row + top, col, buf + pos);
    pos++;
    wrefresh (win);
    break;
  }
  *bufpos = pos;
}

static void
handle_key_for_dialog (STATE *state, int ch)
{
  if (ch > 0x7f)
    return; /* not supported yet */

  if (ch == 27)
  {
    char tmp[4];
    sprintf (tmp, "%d", C_CANCEL_PACKET);
    send_string (state, tmp);
    return;
  }

  switch (state->dialog_mode)
  {
  case FULL_BUTTONS_PACKET:
    switch (ch)
    {
    case 'y':
      end_dialog (state, "8");
      break;
    case 'k':
      end_dialog (state, "9");
      break;
    case 'u':
      end_dialog (state, "10");
      break;
    case 'h':
      end_dialog (state, "11");
      break;
    case '.':
    case ' ':
      end_dialog (state, "12");
      break;
    case 'l':
      end_dialog (state, "13");
      break;
    case 'b':
      end_dialog (state, "14");
      break;
    case 'j':
      end_dialog (state, "15");
      break;
    case 'n':
      end_dialog (state, "16");
      break;
  }
  /* fall through to next case */
  case BUTTONS_PACKET:
    if (ch >= '1' && ch <= '8' && state->buttons[ch - '1'])
    {
      end_dialog (state, "%c", ch - 1);
      if (state->ui->is_class_dlg)
        state->ui->class = ch - '0';
    }
    if (ch == ' ' && is_more_prompt (state))
      end_dialog (state, "0");
    break;
  case COORDINATES_DIALOG_PACKET:
    if (ch != ' ' && ch != '-' && !isdigit (ch) && ch != 0x7f && ch != '\n')
      break;
  /* It's a valid character, so fall through */
  case STRING_DIALOG_PACKET:
  case PLAYER_DIALOG_PACKET:
  case PASSWORD_DIALOG_PACKET:
    switch (ch)
    {
    case '\n':
      waddch (state->ui->msgwin, ch);
      end_dialog (state, "%s", state->ui->kbuf);
      break;
    default:
      add_key (state, state->ui->msgwin, state->ui->inpline, state->ui->kbuf, &state->ui->kbufpos, sizeof (state->ui->kbuf), state->dialog_mode == PASSWORD_DIALOG_PACKET, ch);
    }
  default:
    break;
  }
}

static void
handle_key_for_chat (STATE *state, int ch)
{
  /* TODO: Support scrolling via pgup/pgdn */
  if (ch > 0x7f)
    return;

  switch (ch)
  {
  case '\n':
    waddch (state->ui->chatrespwin, ch);
    if (state->ui->chatkbufpos > 0)
    {
      send_string_f (state, "%d", C_CHAT_PACKET);
      send_string (state, state->ui->chatkbuf);
      state->ui->chatkbufpos = 0;
      memset (state->ui->chatkbuf, 0, sizeof (state->ui->chatkbuf));
      werase (state->ui->chatrespwin);
      wrefresh (state->ui->chatrespwin);
    }
    break;
  default:
    add_key (state, state->ui->chatrespwin, 0, state->ui->chatkbuf, &state->ui->chatkbufpos, sizeof (state->ui->chatkbuf), FALSE, ch);
  }
}

static void
redraw_msgwin (STATE *state)
{
  int i;

  werase (state->ui->msgwin);
  for (i = 0; i < state->ui->msgpos; i++)
    waddstr (state->ui->msgwin, state->ui->msglin[i]);
  wrefresh (state->ui->msgwin);
}

static void
draw_special_text (STATE *state)
{
  int i;

  if (state->ui->special_text_pos >= state->ui->special_text_len)
  {
    /* We're done. Free the text from memory, and redraw the normal text */
    for (i = 0; i < state->ui->special_text_len; i++)
    {
      free (state->ui->special_text[i]);
      state->ui->special_text[i] = NULL;
    }
    state->ui->special_text_len = state->ui->special_text_pos = 0;
    redraw_msgwin (state);
    ui_present_dialog (state);
    return;
  }

  werase (state->ui->msgwin);
  for (i = 0; i < MSGROWS; i++)
  {
    if (state->ui->special_text_pos + i >= state->ui->special_text_len)
      break;
    waddstr (state->ui->msgwin, state->ui->special_text[state->ui->special_text_pos + i]);
    if (i < MSGROWS - 1)
      waddch (state->ui->msgwin, '\n');
  }
  state->ui->special_text_pos += i;
  wrefresh (state->ui->msgwin);

  werase (state->ui->dlgwin);
  waddstr (state->ui->dlgwin, "--more--");
  wrefresh (state->ui->dlgwin);
}

void
ui_get_key (STATE *state)
{
  int ch;
  int size;
  char buf[32];

  size = read(0, buf, sizeof (buf));
  if (size <= 0)
  {
    ui_teardown (state);
    exit (0);
  }
  ch = getkey (buf, size);

  dlog ("Got char %x\n", ch);

  if (ch == '\t')
  {
    if (!state->ui->chatwin)
      return;
    state->ui->chatmode ^= 1;
    fix_cursor (state);
    return;
  }

  if (state->ui->chatmode)
    handle_key_for_chat (state, ch);
  else if (state->ui->special_text_pos > 0)
    draw_special_text (state);
  else
    handle_key_for_dialog (state, ch);
}

void
ui_teardown (STATE *state)
{
  endwin ();
  free (state->ui);
  state->ui = NULL;
}

void
ui_clear (STATE *state)
{
  int i;

  if (!state->ui->special_text_pos)
  {
    werase (state->ui->msgwin);
    wrefresh (state->ui->msgwin);
  }

  state->ui->msgpos = 0;
  for (i = 0; i < MSGROWS; i++)
  {
    free (state->ui->msglin[i]);
    state->ui->msglin[i] = NULL;
  }
}

static void
draw_stats (STATE *state)
{
  int i;

  if (state->ui->statwin)
    return;
  state->ui->statwin = newwin (8, state->ui->ncols, MSGROWS + 5, 0);

  for (i = 0; i < MAX_PACKET_COUNT; i++)
  {
    if (stats[i].label)
    {
      mvwaddstr (state->ui->statwin, stats[i].row, stats[i].label_col, stats[i].label);
      ui_update_stat (state, i);
    }
  }
}

static void
get_bool (int val, char *buf)
{
  if (val == 0)
    strcpy (buf, "No");
  else if (val == 1)
    strcpy (buf, "Yes");
  else
  {
    strcpy (buf, "???");
    dlog ("ERROR: %s: unexpected value %d\n", __func__, val);
  }
}

void
ui_update_stat (STATE *state, int packet)
{
  char buf[256];
  int i;

  if (packet == NAME_PACKET || packet == LOCATION_PACKET)
  {
    if (state->player.name && state->player.location)
    {
      snprintf (buf, sizeof (buf), "%s is in %s (%d, %d)", state->player.name, state->player.location, state->player.y, state->player.x);
      werase (state->ui->locwin);
      mvwaddstr (state->ui->locwin, 0, 0, buf);
      wrefresh (state->ui->locwin);
    }
    return;
  }

  if (!stats[packet].label)
    return; /* unsupported stat, or no room for it on screen */

  if (!state->ui->statwin)
    draw_stats (state);

  wmove (state->ui->statwin, stats[packet].row, stats[packet].data_col);
  buf[0] = '\0';

  switch (packet)
  {
  case ENERGY_PACKET:
    if (state->player.energy[2] > 0)
      sprintf (buf, "%d (%d) %d", state->player.energy[0], state->player.energy[1], state->player.energy[2]);
    else
      sprintf (buf, "%d (%d)", state->player.energy[0], state->player.energy[1]);
    break;
  case SHIELD_PACKET:
    sprintf (buf, "%d", state->player.shield);
    break;
  case STRENGTH_PACKET:
    sprintf (buf, "%d (%d)", state->player.strength[0], state->player.strength[1]);
    break;
  case SPEED_PACKET:
    sprintf (buf, "%d (%d)", state->player.speed[0], state->player.speed[1]);
    break;
  case SWORD_PACKET:
    sprintf (buf, "%d", state->player.sword);
    break;
  case QUICKSILVER_PACKET:
    sprintf (buf, "%d", state->player.quicksilver);
    break;
  case MANA_PACKET:
#ifdef PHANT5
    sprintf (buf, "%d (%d)", state->player.mana[0], state->player.mana[1]);
#else
    sprintf (buf, "%d", state->player.mana[0]);
#endif
    break;
  case LEVEL_PACKET:
    sprintf (buf, "%d", state->player.level);
    break;
  case GOLD_PACKET:
    sprintf (buf, "%d", state->player.gold);
    break;
  case GEMS_PACKET:
    sprintf (buf, "%d", state->player.gems);
    break;
  case CLOAK_PACKET:
    get_bool (state->player.cloak, buf);
    break;
  case BLESSING_PACKET:
    get_bool (state->player.blessing, buf);
    break;
  case CROWN_PACKET:
    get_bool (state->player.crown, buf);
    break;
  case PALANTIR_PACKET:
    get_bool (state->player.palantir, buf);
    break;
  case RING_PACKET:
    get_bool (state->player.ring, buf);
    break;
  case VIRGIN_PACKET:
    get_bool (state->player.virgin, buf);
    break;
  case AMULETS_PACKET:
    sprintf (buf, "%d", state->player.amulets);
    break;
  case CHARMS_PACKET:
    sprintf (buf, "%d", state->player.charms);
    break;
  case TOKENS_PACKET:
    sprintf (buf, "%d", state->player.tokens);
    break;
  default:
    dlog ("ERROR: %s: unexpected stat %d\n", packet);
    break;
  }
  i = strlen (buf);
  while (i < 24)
    buf[i++] = ' ';
  buf[i] = '\0';
  waddstr (state->ui->statwin, buf);
  wrefresh (state->ui->statwin);
}

void
ui_chat_enable (STATE *state, pbool enable)
{
  int top;

  if (enable)
  {
    if (state->ui->chatwin)
      return;
    top = MSGROWS + 15;
    state->ui->chatwin = newwin (state->ui->nrows - top - 1, state->ui->ncols, top, 0);
    scrollok (state->ui->chatwin, TRUE);
    idlok (state->ui->chatwin, TRUE);
    state->ui->chatrespwin = newwin (1, state->ui->ncols, state->ui->nrows - 1, 0);
  }
  else if (state->ui->chatwin)
  {
    delwin (state->ui->chatwin);
    state->ui->chatwin = NULL;
    delwin (state->ui->chatrespwin);
    state->ui->chatrespwin = NULL;
    if (state->ui->chatmode)
    {
      state->ui->chatmode = 0;
      fix_cursor (state);
    }
  }
}

void
ui_chat_message (STATE *state, const char *message)
{
  if (!state->ui->chatwin)
    return;
  waddstr (state->ui->chatwin, message);
  waddch (state->ui->chatwin, '\n');
  wrefresh (state->ui->chatwin);
  fix_cursor (state);
}

void
ui_timeout (STATE *state)
{
  state->dialog_mode = 0;
}

/* Present special text. Used for, eg, displaying the scoreboard or examining
 * a player.
 * Passing NULL indicates that the caller is done posting special text.
 */
void
ui_post_special_text (STATE *state, const char *buf)
{
  if (!buf && state->ui->special_text_len)
  {
    draw_special_text (state);
    return;
  }

  if (state->ui->special_text_len == state->ui->special_text_size)
  {
    state->ui->special_text_size = (state->ui->special_text_size > 0 ? state->ui->special_text_size * 2 : 64);
    state->ui->special_text = (char **) realloc (state->ui->special_text, state->ui->special_text_size * sizeof (char *));
  }

  /* TODO: THis doesn't handle lines longer than the screen width */
  state->ui->special_text[state->ui->special_text_len++] = strdup (buf);
}
