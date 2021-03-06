/*
	Copyright (C) 2013 CurlyMo

	This file is part of the Raspberry Pi 433.92Mhz transceiver.

    Raspberry Pi 433.92Mhz transceiver is free software: you can redistribute
	it and/or modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of the License,
	or (at your option) any later version.

    Raspberry Pi 433.92Mhz transceiver is distributed in the hope that it will
	be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Raspberry Pi 433.92Mhz transceiver. If not, see
	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "log.h"
#include "protocol.h"
#include "binary.h"
#include "arctech_switch.h"

void arctechSwCreateMessage(int id, int unit, int state, int all) {
	arctech_switch.message = json_mkobject();
	json_append_member(arctech_switch.message, "id", json_mknumber(id));
	json_append_member(arctech_switch.message, "unit", json_mknumber(unit));
	if(all == 1)
		json_append_member(arctech_switch.message, "all", json_mknumber(all));
	if(state == 1)
		json_append_member(arctech_switch.message, "state", json_mkstring("on"));
	else
		json_append_member(arctech_switch.message, "state", json_mkstring("off"));
}

void arctechSwParseBinary(void) {
	int unit = binToDecRev(arctech_switch.binary, 28, 31);
	int state = arctech_switch.binary[27];
	int all = arctech_switch.binary[26];
	int id = binToDecRev(arctech_switch.binary, 0, 25);

	arctechSwCreateMessage(id, unit, state, all);
}

void arctechSwCreateLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		arctech_switch.raw[i]=(PULSE_LENGTH);
		arctech_switch.raw[i+1]=(PULSE_LENGTH);
		arctech_switch.raw[i+2]=(PULSE_LENGTH);
		arctech_switch.raw[i+3]=(arctech_switch.pulse*PULSE_LENGTH);
	}
}

void arctechSwCreateHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		arctech_switch.raw[i]=(PULSE_LENGTH);
		arctech_switch.raw[i+1]=(arctech_switch.pulse*PULSE_LENGTH);
		arctech_switch.raw[i+2]=(PULSE_LENGTH);
		arctech_switch.raw[i+3]=(PULSE_LENGTH);
	}
}

void arctechSwClearCode(void) {
	arctechSwCreateLow(2, 132);
}

void arctechSwCreateStart(void) {
	arctech_switch.raw[0]=(PULSE_LENGTH);
	arctech_switch.raw[1]=(arctech_switch.header*PULSE_LENGTH);
}

void arctechSwCreateId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBin(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=((length-i)+1)*4;
			arctechSwCreateHigh(106-x, 106-(x-3));
		}
	}
}

void arctechSwCreateAll(int all) {
	if(all == 1) {
		arctechSwCreateHigh(106, 109);
	}
}

void arctechSwCreateState(int state) {
	if(state == 1) {
		arctechSwCreateHigh(110, 113);
	}
}

void arctechSwCreateUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBin(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=((length-i)+1)*4;
			arctechSwCreateHigh(130-x, 130-(x-3));
		}
	}
}

void arctechSwCreateFooter(void) {
	arctech_switch.raw[131]=(arctech_switch.footer*PULSE_LENGTH);
}

int arctechSwCreateCode(JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	int all = 0;
	char *tmp;

	if(json_find_string(code, "id", &tmp) == 0)
		id=atoi(tmp);
	if(json_find_string(code, "off", &tmp) == 0)
		state=0;
	else if(json_find_string(code, "on", &tmp) == 0)
		state=1;
	if(json_find_string(code, "unit", &tmp) == 0)
		unit = atoi(tmp);
	if(json_find_string(code, "all", &tmp) == 0)
		all = 1;

	if(id == -1 || (unit == -1 && all == 0) || state == -1) {
		logprintf(LOG_ERR, "arctech_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 67108863 || id < 1) {
		logprintf(LOG_ERR, "arctech_switch: invalid id range");
		return EXIT_FAILURE;
	} else if((unit > 16 || unit < 0) && all == 0) {
		logprintf(LOG_ERR, "arctech_switch: invalid unit range");
		return EXIT_FAILURE;
	} else {
		arctechSwCreateMessage(id, unit, state, all);
		arctechSwCreateStart();
		arctechSwClearCode();
		arctechSwCreateId(id);
		arctechSwCreateAll(all);
		arctechSwCreateState(state);
		arctechSwCreateUnit(unit);
		arctechSwCreateFooter();
	}
	return EXIT_SUCCESS;
}

void arctechSwPrintHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -a --all\t\t\tsend command to all devices with this id\n");
}

void arctechSwInit(void) {

	strcpy(arctech_switch.id, "archtech_switches");
	addDevice(&arctech_switch, "kaku_switch", "KlikAanKlikUit Switches");
	addDevice(&arctech_switch, "dio_switch", "D-IO (Chacon) Switches");
	addDevice(&arctech_switch, "nexa_switch", "Nexa Switches");
	addDevice(&arctech_switch, "coco_switch", "CoCo Technologies Switches");
	arctech_switch.type = SWITCH;
	arctech_switch.header = 9;
	arctech_switch.pulse = 5;
	arctech_switch.footer = 35;
	arctech_switch.multiplier = 0.3;
	arctech_switch.rawLength = 132;
	arctech_switch.binaryLength = 33;
	arctech_switch.repeats = 1;
	arctech_switch.message = malloc(sizeof(JsonNode));

	arctech_switch.bit = 0;
	arctech_switch.recording = 0;

	addOption(&arctech_switch.options, 'a', "all", no_value, 0, NULL);
	addOption(&arctech_switch.options, 't', "on", no_value, config_state, NULL);
	addOption(&arctech_switch.options, 'f', "off", no_value, config_state, NULL);
	addOption(&arctech_switch.options, 'u', "unit", has_value, config_id, "[0-9]");
	addOption(&arctech_switch.options, 'i', "id", has_value, config_id, "[0-9]");

	arctech_switch.parseBinary=&arctechSwParseBinary;
	arctech_switch.createCode=&arctechSwCreateCode;
	arctech_switch.printHelp=&arctechSwPrintHelp;

	protocol_register(&arctech_switch);
}
