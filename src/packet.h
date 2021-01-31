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

/* Following copied from phantdefs.h in the phantasia 4.03 sources */

/* client->player packet headers */
#define HANDSHAKE_PACKET	2	/* used when connecting */
#define CLOSE_CONNECTION_PACKET	3	/* last message before close */
#define PING_PACKET		4	/* used for timeouts */
#define ADD_PLAYER_PACKET	5	/* add a player to the list */
#define REMOVE_PLAYER_PACKET	6	/* remove a player from the list */
#define SHUTDOWN_PACKET		7	/* the server is going down */
#define ERROR_PACKET		8	/* server has encountered an error */

#define CLEAR_PACKET		10	/* clears the message screen */
#define WRITE_LINE_PACKET	11	/* write a line on message screen */

#define BUTTONS_PACKET		20	/* use the interfaces buttons */
#define FULL_BUTTONS_PACKET	21	/* use buttons and compass */
#define STRING_DIALOG_PACKET	22	/* request a message response */
#define COORDINATES_DIALOG_PACKET 23	/* request player coordinates */
#define PLAYER_DIALOG_PACKET	24	/* request a player name */
#define PASSWORD_DIALOG_PACKET	25	/* string dialog with hidden text */
#define SCOREBOARD_DIALOG_PACKET 26	/* pull up the scoreboard */
#define DIALOG_PACKET		 27	/* okay dialog with next line */

#define CHAT_PACKET		30	/* chat message */
#define ACTIVATE_CHAT_PACKET	31	/* turn on the chat window */
#define DEACTIVATE_CHAT_PACKET	32	/* turn off the chat window */
#define PLAYER_INFO_PACKET	33	/* display a player's info */
#define CONNECTION_DETAIL_PACKET 34	/* display connection info */

#define NAME_PACKET		40	/* set the player's name */
#define LOCATION_PACKET		41	/* refresh the player's energy */
#define ENERGY_PACKET		42	/* refresh the player's energy */
#define STRENGTH_PACKET		43	/* refresh the player's strength */
#define SPEED_PACKET		44	/* refresh the player's speed */
#define SHIELD_PACKET		45	/* refresh the player's shield */
#define SWORD_PACKET		46	/* refresh the player's sword */
#define QUICKSILVER_PACKET	47	/* refresh the player's quicksilver */
#define MANA_PACKET		48	/* refresh the player's mana */
#define LEVEL_PACKET		49	/* refresh the player's level */
#define GOLD_PACKET		50	/* refresh the player's gold */
#define GEMS_PACKET		51	/* refresh the player's gems */
#define CLOAK_PACKET		52	/* refresh the player's cloak */
#define BLESSING_PACKET		53	/* refresh the player's blessing */
#define CROWN_PACKET		54	/* refresh the player's crowns */
#define PALANTIR_PACKET		55	/* refresh the player's palantir */
#define RING_PACKET		56	/* refresh the player's ring */
#define VIRGIN_PACKET		57	/* refresh the player's virgin */
/* Following were added after the source posted online -MPG */
#define TIMED_PING_PACKET       58
#define AMULETS_PACKET          59
#define CHARMS_PACKET           60
#define TOKENS_PACKET           61
#define STAFF_PACKET            62
#define EXP_PACKET              63

/* player->client packet headers */
#define C_RESPONSE_PACKET	1	/* player feedback for game */
#define C_CANCEL_PACKET		2	/* player canceled question */
#define C_PING_PACKET		3	/* response to a ping */
#define C_CHAT_PACKET		4	/* chat message from player */
#define C_EXAMINE_PACKET	5	/* examine a player */
#define C_ERROR_PACKET		6	/* client is lost */
#define C_SCOREBOARD_PACKET	7	/* show the scoreboard */
/* Following were added after the source posted online -MPG */
#define C_PONG_PACKET           8
#define C_PING_REQUEST_PACKET   9
