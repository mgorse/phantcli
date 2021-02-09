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
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

int sockconnect (const char *host, int port)
{
  int fd;
  struct sockaddr_in servaddr;

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
  {
    perror ("socket");
    return -1;
  }

  memset (&servaddr, 0, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);

  if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0)
  {
    struct hostent *h;
    h = gethostbyname (host);
    if (!h)
    {
      perror ("gethostbyname");
      return -1;
    }
    memcpy ((char *)&servaddr.sin_addr, h->h_addr, sizeof (servaddr.sin_addr));
  }

  if (connect(fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
  {
    perror("connect");
    return -1;
  }
  return fd;
}

void
dlog (const char *fmt, ...)
{
  FILE *fp;
  va_list args;

  if (!getenv ("PHANTCLI_DEBUG"))
    return;

  fp = fopen ("debug.log", "a");
  va_start (args, fmt);
  vfprintf (fp, fmt, args);
  va_end (args);
  fclose (fp);
}

int
read_socket (STATE *state)
{
  int res;
  int i;
  char *p;

  res = read (state->fd, state->buf + state->bufpos, sizeof (state->buf) - 1 - state->bufpos);
  dlog("data: %s", state->buf + state->bufpos);
  if (res <= 0)
    return -1;
  state->bufpos += res;
  state->buf[state->bufpos] = '\0';

  i = 0;
  while (i < state->bufpos)
  for (;;)
  {
    p = strchr (state->buf + i, '\n');
    if (!p)
    {
      if (i)
        memmove (state->buf, state->buf + i, sizeof (state->buf) - i);
      state->bufpos = strlen (state->buf);
      return 0;
    }
    *p = '\0';
    res = state->sdh (state, state->buf + i);
    if (res == 0)
      state->sdh = handle_packet;
    i = p + 1 - state->buf;
  }

  return 0;
}

static void
mkdirs (char *buf)
{
  int i;
  char *p;

  p = strstr (buf, ".local");

  for (i = 0; i < 3; i++)
  {
    p = strchr (p, '/');
    if (*p != '/')
      return;
    *p = '\0';
    mkdir (buf, 493);	/* FIXME: octal 0755 */
    *p = '/';
    p++;
  }
}

static int
get_cookie ()
{
  char buf[1024];
  int cookie = 0;
  FILE *fp;
  const char *home = getenv ("HOME");

  if (!home)
    return 0;

  snprintf (buf, sizeof (buf), "%s/.local/share/phantcli/cookie", home);
  buf[sizeof(buf) - 1] = '\0';
  mkdirs (buf);

  fp = fopen (buf, "r");
  if (fp)
  {
    fscanf (fp, "%d", &cookie);
    fclose (fp);
  }

  if (!cookie)
  {
    cookie = rand ();
    fp = fopen (buf, "w");
    if (fp)
    {
      fprintf (fp, "%d\n", cookie);
      fclose (fp);
    }
  }

  return cookie;
}

void
do_client (const char *host, int port)
{
  fd_set fds;
  STATE state;
  int result;

  init_handlers ();

  memset (&state, 0, sizeof (state));
  state.sdh = handle_packet;
  state.fd = sockconnect (host, port);
  if (state.fd == -1)
  {
    fprintf(stderr, "Could not connect to %s port %d\n", host, port);
    return;
  }

  state.cookie = get_cookie ();

  ui_init (&state);

  for (;;)
  {
    FD_ZERO (&fds);
    FD_SET (0, &fds);
    FD_SET (state.fd, &fds);
    select (state.fd + 1, &fds, NULL, NULL, NULL);
    if (FD_ISSET (state.fd, &fds))
    {
      result = read_socket (&state);
      if (result < 0)
      {
        ui_teardown (&state);
        exit (1);
      }
    }
    if (FD_ISSET (0, &fds))
      ui_get_key (&state);
  }
}

void
send_string (STATE *state, const char *buf)
{
  write (state->fd, buf, strlen (buf) + 1);
}

void
send_string_fv (STATE *state, const char *fmt, va_list args)
{
  char buf[1024];

  vsnprintf (buf, sizeof (buf), fmt, args);
  buf[sizeof(buf) - 1] = '\0';
  write (state->fd, buf, strlen (buf) + 1);
}

void
send_string_f (STATE *state, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  send_string_fv (state, fmt, args);
  va_end (args);
}

void
respondv (STATE *state, const char *fmt, va_list args)
{
  char buf[1024];

  sprintf (buf, "%d", C_RESPONSE_PACKET);
  vsnprintf (buf + 2, sizeof (buf) - 2, fmt, args);
  dlog ("sending response: %s\n", buf);
  write (state->fd, buf, strlen (buf + 2) + 3);
}

void
respond (STATE *state, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  respondv (state, fmt, args);
  va_end (args);
}

int
main(int argc, char *argv[])
{
  const char *host = "phantasia4.net";
  int port = 43302;
  int done = 0;

  while (!done)
  {
    switch (getopt (argc, argv, "h:p:"))
    {
    case 'h':
      host = strdup (optarg);
      break;
    case 'p':
      port = atoi (optarg);
      break;
    case '?':
      fprintf (stderr, "Usage: %s [-h <host>] [-p <port>]\n", argv[0]);
      exit (0);
    default:
      done = 1;
      break;
    }
  }

  srand (time (NULL));

  do_client (host, port);
}
