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

#include "serial.h"
#include <sys/io.h>             // ioperm
#include <unistd.h>             // ioperm
#include <termio.h>

void rts_on(int fd) {
	int j = 0;
	/* Get command register status bits */
	if (ioctl(fd, TIOCMGET, &j) < 0) {
		printf("Error getting hardware status bits in serial.c:rts_on()\n");
		return;
	}

	j = j | (TIOCM_RTS);

	/* Send back to port with ioctl */
	ioctl(fd, TIOCMSET, &j);
}

void rts_off(int fd) {
	int j;
	/* Get command register status bits */
	if (ioctl(fd, TIOCMGET, &j) < 0) {
		printf("Error getting hardware status bits in serial.c:rts_off()\n");
	}

	j = j & (~TIOCM_RTS);

	/* Send back to port with ioctl */
	ioctl(fd, TIOCMSET, &j);
}

BOOL cts_check(int fd) {
	printf("check CTS\n");

	int cts;
	// Get command register status bits
	if (ioctl(fd, TIOCMGET, &cts) < 0) {
		printf("Error getting hardware status bits in dts_modem:cts_check()\n");
		return FALSE ;
	}

	cts = cts & TIOCM_CTS;

	if (cts > 0) {
		return TRUE ;
	}

	return FALSE ;
}

/* 
 * Opens the serial port named `dev'
 */
void open_port(int *fd, char *dev) {
	// Initialize the serial port
	//ioperm(0, 0x3ff, 0xffffffff);		// ioperm

	*fd = open(dev, O_RDWR | O_NOCTTY/* | O_NONBLOCK */);       // open device
	if (*fd == -1) {
		perror("Couldn't open serial port");
		exit(-1);
	}

	if (isatty(*fd) != 1) {
		perror("Invalid terminal device");
		exit(-1);
	}

	stty_raw(*fd);		// configure the device
	// rts_off(*fd);
}

void close_port(int fd) {
	close(fd);
}

int write2port(int fd, const char *data, int size_data) {
	int size = 0;
	int maxfd = 0;
	int s = 0;
	fd_set fdset;

	size = size_data;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	maxfd = fd + 1;
	select(maxfd, 0, &fdset, 0, NULL);    // block until the data arrives

	if (FD_ISSET(fd, &fdset) == FALSE)    // data ready?
	{
		printf("Not Ready (W)\n");
		return 0;						  //writen continues trying
	}

	s = write(fd, data, size);            // dump the data to the device
	fsync(fd); 							  // sync the state of the file

	return s;
}

int read_from_port(int fd, char *data, int size_data) {
	int maxfd = 0;
	fd_set fdset;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	maxfd = fd + 1;

	select(maxfd, &fdset, 0, 0, NULL);     // block until the data arrives

	if (FD_ISSET(fd, &fdset) == TRUE)      // data ready?
		/* Just return, value is checked by caller */
		return read(fd, data, size_data);
	else
		return 0;
}

speed_t get_baud_rate_constant() {
	unsigned int baud_rate; /* = BAUD_RATE */

	switch (baud_rate) {
	case 0:
		return B0;
	case 50:
		return B50;
	case 75:
		return B75;
	case 110:
		return B110;
	case 134:
		return B134;
	case 150:
		return B150;
	case 200:
		return B200;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 1800:
		return B1800;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	default:
		return B1200;
	}
}

/*
 * serial port configuration
 */
int stty_raw(int fd) {
	struct termios tty_state;
	int i = 0;

	if (tcgetattr(fd, &tty_state) < 0) {
		exit(-1);
	}

	for (i = 0; i < TTY_STORE; i++) {
		if (sttyfds[i] == -1) {
			orig_tty_state[i] = tty_state;
			sttyfds[i] = fd;
			break;
		}
	}

	// non-canonical mode turns off input character processing
	// echo is off
	tty_state.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
	tty_state.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
	tty_state.c_oflag &= ~OPOST;
	tty_state.c_cflag |= (CS8 | CRTSCTS);
	tty_state.c_cc[VMIN] = 1;
	tty_state.c_cc[VTIME] = 0;

	if (cfsetospeed(&tty_state, B9600) < 0) {
		perror("cfsetospeed");
		exit(-1);
	}

	if (tcsetattr(fd, TCSAFLUSH, &tty_state) < 0) {
		perror("tcsetattr");
		exit(-1);
	}

	return 0;
}

