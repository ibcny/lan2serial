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

#ifndef _LAN2SERIAL_H
#define _LAN2SERIAL_H

#ifndef BOOL
#define BOOL unsigned char
#endif 

#ifndef EOS
#define EOS '\0'
#endif 

#define USAGE printf(" Usage: %s <listening port> <device name>\n\t -c " \
  "[<server host name>] <server port> <device name>\n", argv[0]) 
#define MAX_READ_SIZE 1024
#define DEBUG(str) do { fprintf(stderr, "DEBUG: %s", str); } while (0)

pthread_t t_lan2serial, t_serial2lan;

int lanfd, comfd;

#endif
