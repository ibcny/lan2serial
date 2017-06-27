/*
 * ----------------------------------------------------------------------------
 * lan2serial
 * Utility for the transport of data from serial interfaces to TCP/IP or vice versa.
 * 
 *
 * ----------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#ifndef _SERIAL_H
#define _SERIAL_H

#include <stdlib.h>
#include <stdio.h>              /* perror etc. */
#include <unistd.h>             /* close(fd) etc. */
#include <string.h>

// Socket related headers
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <strings.h>            /* bzero etc. */
#include <termios.h>

// Serial inteface headers
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef TRUE
#define TRUE (BOOL)1
#endif

#ifndef FALSE
#define FALSE (BOOL)0
#endif

#define TTY_STORE       16

struct termios orig_tty_state[TTY_STORE];
int sttyfds[TTY_STORE];

void open_port(int *fd, char *dev);
int write2port(int fd, const char *data, int size_data);
int read_from_port(int fd, char *data, int size_data);
void rts_on(int fd);
void rts_off(int fd);
BOOL cts_check();
int stty_raw(int fd);

#endif
