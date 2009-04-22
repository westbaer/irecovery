/**
  * iRecovery - Utility for DFU 2.0, WTF and Recovery Mode
  * Copyright (C) 2008 - 2009 westbaer
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
  **/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <usb.h>
#include <readline/readline.h>

#include "constants.h"

struct usb_dev_handle *devPhone;

void irecv_hexdump(char *a, int c) {
	int b;
	
	for(b=0;b<c;b++) {
		if(b%16==0&&b!=0)
			printf("\n");
		printf("%2.2X ",a[b]);
	}
	printf("\n");
}

struct usb_dev_handle *irecv_init(int devid) {
	struct usb_device *dev;
	struct usb_bus *bus;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for(bus = usb_get_busses(); bus; bus = bus->next) {
		for(dev = bus->devices; dev; dev = dev->next) {
			if(dev->descriptor.idVendor == 0x5AC && dev->descriptor.idProduct == devid) {
				devPhone = usb_open(dev);
			}
		}
	}

	return devPhone;
}

void irecv_close() {
	printf("Closing USB connection...\n");
	usb_close(devPhone);
}

void irecv_sendfile(char *filename) {
	FILE *file;
	int packets, len, last, i, a, c, sl;
	char *fbuf, buf[6];
	
	if(!filename)
		return;

	file = fopen(filename, "rb");
	if(file == NULL) {
		printf("File %s not found.\n", filename);
		exit(EXIT_FAILURE);
	}
	fseek(file, 0, 0);
	fclose(file);

	irecv_init(WTF_MODE);
	if(!devPhone) {
		devPhone = irecv_init(RECV_MODE);
		if(devPhone) {
			printf("Found iPhone/iPod in Recovery mode\n");
		}
	} else {
		printf("Found iPhone/iPod in DFU/WTF mode\n");
	}

	if(!devPhone) {
		printf("No iPhone/iPod found.\n");
		exit(EXIT_FAILURE);
	}

	if(usb_set_configuration(devPhone, 1)) {
		printf("Error setting configuration\n");
	}

	printf("\n");

	file = fopen(filename, "rb");
	fseek(file, 0, SEEK_END);
	len = ftell(file);
	fseek(file, 0, 0);

	packets = len / 0x800;
	if(len % 0x800)
		packets++;
	last = len % 0x800;

	printf("Loaded image file (len: 0x%x, packets: %d, last: 0x%x).\n", len, packets, last);

	fbuf = malloc(packets * 0x800);
	if(!last) {
		last = 0x800;
	}
	
	fread(fbuf, 1, len, file);
	fclose(file);

	printf("Sending 0x%x bytes\n", len);

	for(i=0, a=0, c=0; i<packets; i++, a+=0x800, c++) {
		sl = 0x800;

		if(i == packets-1) {
			sl = last;
		}

		printf("Sending 0x%x bytes in packet %d... ", sl, c);

		if(usb_control_msg(devPhone, 0x21, 1, c, 0, &fbuf[a], sl, 1000)) {
			printf(" OK\n");
		} else{
			printf(" x\n");
		}

		if(usb_control_msg(devPhone, 0xA1, 3, 0, 0, buf, 6, 1000) != 6){
			printf("Error receiving status!\n");
		} else {
			irecv_hexdump(buf, 6);
			if(buf[4]!=5) {
				printf("Status error!\n");
			}
		}
	}

	printf("Successfully uploaded file!\nExecuting it...\n");	

	usb_control_msg(devPhone, 0x21, 1, c, 0, fbuf, 0, 1000);

	for(i=6; i<=8; i++) {
		if(usb_control_msg(devPhone, 0xA1, 3, 0, 0, buf, 6, 1000) != 6){
			printf("Error receiving status!\n");
		} else {
			irecv_hexdump(buf, 6);
			if(buf[4]!=i) {
				printf("Status error!\n");
			}
		}
	}

	free(fbuf);
	irecv_close(devPhone);
}

void irecv_console() {
		int ret, length, skip_recv, firstresp;
		char *buf, *sendbuf, *cmd, *response;
		
		irecv_init(RECV_MODE);

		if(devPhone == 0) {
			printf("No iPhone/iPod found.\n");
			exit(EXIT_FAILURE);
		}

		if((ret = usb_set_configuration(devPhone, 1)) < 0) {
			printf("Error %d when setting configuration\n", ret);
			irecv_close();
			exit(EXIT_FAILURE);
		}

		if((ret = usb_claim_interface(devPhone, 1)) < 0) {
			printf("Error %d when claiming interface\n", ret);
			irecv_close();
			exit(EXIT_FAILURE);
		}

		if((ret = usb_set_altinterface(devPhone, 1)) < 0) {
			printf("Error %d when setting altinterface\n", ret);
			irecv_close();
			exit(EXIT_FAILURE);
		}
		
		buf = malloc(0x10001);
		sendbuf = malloc(160);
		cmd = malloc(160);

		skip_recv = 0;
		firstresp = 0;
		

		do {
			if(!skip_recv) {
				ret = usb_bulk_read(devPhone, 0x81, buf, 0x10000, 1000);
				if(ret > 0) {
					response = buf;
					while(response < buf + ret)
					{
						printf("%s", response);
						response += strlen(response) + 1;
					}
				}
			} else {
				skip_recv = 0;
			}

			if(firstresp == 1) {
				printf("] ", response);
			}
			cmd = readline(NULL);
			if(cmd && *cmd) {
				add_history(cmd);
			}

			if(firstresp == 0) {
				firstresp = 1;
			}

			length = (int)(((strlen(cmd)-1)/0x10)+1)*0x10;
			memset(sendbuf, 0, length);
			memcpy(sendbuf, cmd, strlen(cmd));
			if(!usb_control_msg(devPhone, 0x40, 0, 0, 0, sendbuf, length, 1000)) {
				printf("[!] %s", usb_strerror());
			}
		} while(strcmp("/exit", cmd) != 0);
		usb_release_interface(devPhone, 0);
		free(buf);
		free(sendbuf);
		free(cmd);
		irecv_close(devPhone);
}

int irecv_usage(void) {
		printf("./iRecovery [args]\n");
		printf("\t-f <file>\t\tupload file.\n");
		printf("\t-r\t\t\treset usb.\n");
		printf("\t-s\t\t\tstarts a shell.\n\n");
}

int main(int argc, char *argv[]) {
	printf("iRecovery - Recovery Utility\n");
	printf("by westbaer\nThanks to pod2g, tom3q, planetbeing and geohot.\n\n");
	if(argc < 2) {
		irecv_usage();
		exit(EXIT_FAILURE);
	}

	if(argv[1][0] != '-') {
		irecv_usage();
		exit(EXIT_FAILURE);
	}
	
	if(strcmp(argv[1], "-f") == 0) {
		if(argc < 3) {
			printf("No valid file set.\n");
			exit(EXIT_FAILURE);
		} 

		irecv_sendfile(argv[2]);
	} else if(strcmp(argv[1], "-s") == 0) {
		irecv_console();
	} else if(strcmp(argv[1], "-r") == 0) {
		usb_reset(devPhone);
	}
	
	return 0;
}
