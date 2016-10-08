/*
 * ethernet.h
 *
 *  Created on: 2015.10.31.
 *      Author: koverg
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

#include "daylight.h"

EtherShield es;
static uint8_t mymac[6] = { 0x54, 0x55, 0x58, 0x10, 0x00, 0x24 };
static uint8_t myip[4] = { 10, 0, 0, 190 };
static uint8_t gwip[4] = { 10, 0, 0, 1 };
static uint8_t ntpip[4] = { 193, 224, 45, 107 };
//static uint8_t ntpip[4] = { 95, 215, 175, 2 };

#define BUFFER_SIZE 3750					// on atmega 328 (2KB RAM) max is 750, on 2560 (8KB RAM) it can safely be 3750
static uint8_t buf[BUFFER_SIZE + 1];
char timeBuff[30];
#define GMT_ZONE 1

static time_t startTime = 0;

typedef struct _schedule {
	uint8_t nodeId;
	time_t receive;
	uint8_t buffer[12];
} Schedule;

class Ethernet : public ITask
{
private:
	SensorStore *sensors;
	uint32_t addressTimer;
	Schedule schedule;
	time_t lastTimeAdjust;
	time_t lastTimeRequest;
	int ntpUpdateMinutes = 5;

public:
	Ethernet(SensorStore *sensors_) {
		sensors = sensors_;
		addressTimer = 0;

		printf_P(PSTR("mac && ip init start\r\n"));
		delay(200);
		es.ES_enc28j60Init(mymac);
		// init the ethernet/ip layer:
		es.ES_init_ip_arp_udp_tcp(mymac, myip, 80);
		es.ES_client_set_gwip(gwip);
		printf_P(PSTR("mac && ip set\r\n"));
		delay(400);
		setTime(10);
		lastTimeAdjust = 0;
		lastTimeRequest = 0;
		es.ES_client_ntp_request(buf, ntpip, 247); // TODO: 25000 was here
	}

	void timeDateToText(time_t time, char *buffer)
	{
		// YEAR
		int val = year(time);
		buffer[0] = '0' + (val / 1000);
		val -= (val / 1000) * 1000;
		buffer[1] = '0' + (val / 100);
		val -= (val / 100) * 100;
		buffer[2] = '0' + (val / 10);
		val -= (val / 10) * 10;
		buffer[3] = '0' + val;
		buffer[4] = '.';

		// MONTH
		val = month(time);
		buffer[5] = '0' + (val / 10);
		val -= (val / 10) * 10;
		buffer[6] = '0' + val;
		buffer[7] = '.';

		// DAY
		val = day(time);
		buffer[8] = '0' + (val / 10);
		val -= (val / 10) * 10;
		buffer[9] = '0' + val;
		buffer[10] = '.';
		buffer[11] = ' ';

		// HOUR
		val = hour(time);
		buffer[12] = '0' + (val / 10);
		val -= (val / 10) * 10;
		buffer[13] = '0' + val;
		buffer[14] = ':';

		// MIN
		val = minute(time);
		buffer[15] = '0' + (val / 10);
		val -= (val / 10) * 10;
		buffer[16] = '0' + val;
		buffer[17] = ':';

		// SEC
		val = second(time);
		buffer[18] = '0' + (val / 10);
		val -= (val / 10) * 10;
		buffer[19] = '0' + val;

		buffer[20] = 0;
	}

	uint16_t http200ok(void)
	{
		return (es.ES_fill_tcp_data_p(buf, 0, PSTR(
				"HTTP/1.0 200 OK\r\n"
				"Content-Type: text/json\r\n"
				"Pragma: no-cache\r\n"
				"Access-Control-Allow-Origin: *\r\n"	// needed to allow request from other sites
				"\r\n")));
	}

	uint16_t print_webpage(uint8_t *buf, uint8_t node_id, const char *stype)
	{
		time_t nnn = now();
		uint16_t plen;
		plen = http200ok();
		plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("{\"time\": \""));
		timeDateToText(nnn, timeBuff);
		plen = es.ES_fill_tcp_data(buf,plen, timeBuff);
		plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", \"start\": \""));
		timeDateToText(startTime, timeBuff);
		plen = es.ES_fill_tcp_data(buf,plen, timeBuff);
		plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", \"sensors\": ["));
		int sensCount = 0;

		if (sensors != NULL)
		{
			for (int j = 0; j < sensors->getNodeCount(); ++j)
			{
				NodeData *node = sensors->getNodes()[j];
				{
					for (int i = 0; i < sensors->getSensorCount(); ++i)
					{
						/*
						uint8_t		nodeId;			// the node that collected the data
						char		sensorType;		// A,B,C,D,E,F,G,H,I,J - temperature, X,Y,Z - humidity, 0,1,2,3,4,5,6,7,8,9 - other values
						time_t 		lastUpdate; 	// the date and time of the last sensor message
						uint16_t	value;			// the sensor value
						uint16_t	extra;			// extra data (if value cannot be stored on 16 bit)
						*/


						SensorData * sensor = (sensors->getSensors())[i];
						if ((stype == NULL || stype[0] == '*' || stype[0] == sensor->sensorType) && (node_id == 0 || node->nodeId == node_id))
						{
							//itoa(i, timeBuff, 10);
							//plen = es.ES_fill_tcp_data_p(buf,plen, PSTR(", \r\ns"));
							//plen = es.ES_fill_tcp_data(buf,plen, timeBuff);
							//plen = es.ES_fill_tcp_data_p(buf,plen, PSTR(": {"));
							if (sensCount++ != 0)
							{
								plen = es.ES_fill_tcp_data_p(buf,plen, PSTR(", "));
							}
							plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\r\n{"));

							plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\"n\": \""));
							itoa(sensor->nodeId, timeBuff, 10);
							plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

							plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", \"lu\": \""));
							itoa(nnn - sensor->lastUpdate, timeBuff, 10);
							plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

							plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", \"t\": \""));
							timeBuff[0] = sensor->sensorType;
							timeBuff[1] = 0;
							plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

							plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", \"v\": \""));
							itoa(sensor->value, timeBuff, 10);
							plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

							plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", \"e\": \""));
							itoa(sensor->extra, timeBuff, 10);
							plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

							plen = es.ES_fill_tcp_data_p(buf, plen, PSTR("\"}"));
						}
					}
				}
			}
		}

		plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\r\n]}"));

//		plen = es.ES_fill_tcp_data_p(buf, plen, PSTR(", settings: \""));
//		for (int i = 0; i < SETTINGS_SIZE; ++i)
//		{
//			byte b = EEPROM.read(i);
//			//byte b = settings[i];
//			if (b != '0' && b != '1')
//			{
//				b = '0';
//			}
//			timeBuff[0] = b;
//			timeBuff[1] = 0;
//			plen = es.ES_fill_tcp_data(buf,plen, timeBuff);
//		}
//
//		plen = es.ES_fill_tcp_data_p(buf, plen, PSTR("\"}\0"));
		return plen;
	}


	boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload) {
		if (header.type == 'S') {	// schedule receive
			schedule.nodeId = mesh.getNodeID(header.from_node);
			schedule.receive = now();
			memcpy(schedule.buffer, payload, sizeof(schedule.buffer));
		}
		return false;
	}

	const char *name() { return "Ethernet"; };

	/**
	 * Converts the two hexadecimal characters (high digit, low digit) to a byte
	 */
	uint8_t hexToBin(char h1, char h2)
	{
		uint8_t bin = 0;
		if ('0' <= h2 && h2 <= '9')
		{
			bin += h2-'0';
		}
		else if ('a' <= tolower(h2) && tolower(h2) <= 'f')
		{
			bin += tolower(h2)-'a'+10;
		}
		if ('0' <= h1 && h1 <= '9')
		{
			bin += (h1-'0')*16;
		}
		else if ('a' <= tolower(h1) && tolower(h1) <= 'f')
		{
			bin += (tolower(h1)-'a'+10)*16;
		}
		return bin;
	}

	void updateState() {
		// ethernet
		uint16_t plen, dat_p;

		// read packet, handle ping and wait for a tcp packet:
		dat_p = es.ES_packetloop_icmp_tcp(buf,
				es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

		/* dat_p will be unequal to zero if there is a valid
		 * http get */
		if (dat_p != 0) {
			// there is a request

			// process NTP response
			if (buf[IP_PROTO_P] == IP_PROTO_UDP_V &&
				buf[UDP_SRC_PORT_H_P] == 0 &&
				buf[UDP_SRC_PORT_L_P] == 0x7b )
			{
				//Serial.println("Processing NTP response...");
				uint32_t time = 0;
				if (es.ES_client_ntp_process_answer(buf, &time, 247))	// TODO: 25000 was heres
				{
					time_t oldTime = now();
					time_t newTime = time - 2208988800UL + (GMT_ZONE * 60 * 60); // 70 years back plus the GTM shift
					if (IsDST(year(newTime), month(newTime), day(newTime)))
					{
						newTime += 60 * 60;	// one hour because it's a daylight saving day
					}
					if (year() < 2013) {
						startTime = newTime;
					}
					setTime(newTime);
					lastTimeAdjust = newTime;
					timeDateToText(newTime, timeBuff);
					printf_P(PSTR("Time adjusted with NTP time: %s, time diff: %ld\r\n"), timeBuff, (long)(newTime - oldTime));
				}
			}
			else if (strncmp("GET ", (char *) buf + dat_p, 4) == 0)
			{
				// sensor values
				if (strncmp("/V", (char *) buf + dat_p + 4, 2) == 0)
				{
					char sensor[4];
					uint16_t node_id;
					sscanf((char *)buf + dat_p + 6, "%d:%3s", &node_id, sensor);
					dat_p = print_webpage(buf, node_id, sensor);
				}
				else if (strncmp("/S", (char *) buf + dat_p + 4, 2) == 0)
				{
					char hex[30];
					uint16_t node_id;
					sscanf((char *)buf + dat_p + 6, "%d:%25s", &node_id, hex);
					if (hex[0] == 'g' || hex[0] == 'G')
					{
						hex[0] = 'G';
						if(mesh.write(hex, 'S', 1, node_id))
						{
							dat_p = http200ok();
							char buffer[25];
							for (int i = 0; i < 12; ++i)
							{
								sprintf(buffer+2*i, "%02x", *(schedule.buffer + i));
							}
							char result[200];
							timeDateToText(schedule.receive, timeBuff);
							sprintf(result, "{\"node\":\"%d\", \"time\":\"%s\", \"schedule\":\"%s\"}", schedule.nodeId, timeBuff, buffer);
							dat_p = es.ES_fill_tcp_data(buf, dat_p, result);
						}
						else
						{
							dat_p = es.ES_fill_tcp_data_p(buf, dat_p, PSTR("<h1>200 SEND ERROR</h1>"));
						}
					}
					else if (hex[0] == 's' || hex[0] == 'S')
					{
						printf_P(PSTR("\r\nIdõzítések beállítása: node:%d, sched:%s"),  node_id, hex);
						uint8_t buffer[13];
						buffer[0] = 'S';
						for (int i = 0; i < 12; ++i)
						{
							buffer[1+i] = hexToBin(hex[i*2+1], hex[i*2+2]);
						}
						mesh.write(buffer, 'S', sizeof(buffer), node_id);
						printf_P(PSTR("Schedule message sent to node: %d.\r\n"), node_id);
						dat_p = http200ok();
						dat_p = es.ES_fill_tcp_data_p(buf, dat_p, PSTR("<h1>200 OK</h1>"));
					}
					else
					{
						dat_p = es.ES_fill_tcp_data_p(buf, 0,PSTR("HTTP/1.0 400 Bad request\r\nContent-Type: text/html\r\n\r\n<h1>400 Illegal subcommand in schedule.</h1>"));
					}
				}
				else
				{
					dat_p = es.ES_fill_tcp_data_p(buf, 0,PSTR("HTTP/1.0 400 Bad request\r\nContent-Type: text/html\r\n\r\n<h1>400 Illegal command.</h1>"));
				}
			}
			else
			{
				// head, post and other methods:
				dat_p = http200ok();
				dat_p = es.ES_fill_tcp_data_p(buf, dat_p, PSTR("<h1>200 OK</h1>"));
			}
			es.ES_www_server_reply(buf, dat_p); // send web page data
		}

		// NTP idõ lekérése: ha még nincs "rendes" dátum, akkor mos, ha van, akkor 30 percenként
		// viszont arra figyel, hogy request-et ne küldjön csak 3 másodpercenként
		time_t nnn = now();
		if ((year(nnn) < 2013 || nnn - lastTimeAdjust > 60*ntpUpdateMinutes) && nnn - lastTimeRequest > 2)
		{
			lastTimeRequest = nnn;
			es.ES_client_ntp_request(buf, ntpip, 247); // TODO: 25000 was heres
			timeDateToText(nnn, timeBuff);
			printf_P(PSTR("NTP time requested: %s\r\n"), timeBuff);
		}
	}
};

#endif /* ETHERNET_H_ */
