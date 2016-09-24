/*
 * esp8266_ntp.h
 *
 *  Created on: 2016. máj. 18.
 *      Author: koverg
 */

#ifndef COMPONENTS_ESP8266_NTP_H_
#define COMPONENTS_ESP8266_NTP_H_

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
extern "C" {
#include "user_interface.h"
#include "daylight.h"
}

#define SYNC_INTERVAL_MS	(10*60)	// check time server every 10 minutes
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

class ESP8266Ntp : public ITask {
private:
	time_t lastUpdate;
	time_t lastCheck;	// 0 = no NTP respons expected, other = expecting NTP and last checked at
	unsigned int localPort = 2390;      // local port to listen for UDP packets
	IPAddress timeServerIP; // time.nist.gov NTP server address
	const char* ntpServerName = "time.nist.gov";
	byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
	// A UDP instance to let us send and receive packets over UDP
	WiFiUDP udp;
public:
	ESP8266Ntp() {
		lastUpdate = now();
		lastCheck = 0;
	}

public:
	const char *name() { return "ESP8266Ntp"; };

	boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload) {
	}

	void updateState(){
		time_t currentTime = now();
		if ((year(currentTime) <= 2013  || currentTime - lastUpdate > SYNC_INTERVAL_MS) && lastCheck == 0) {
			if (WiFi.status() == WL_CONNECTED) {
				printf_P(PSTR("WiFi connected.\r\n"));
				udp.begin(localPort);
				Serial.print("Local port: ");
				Serial.println(udp.localPort());

				WiFi.hostByName(ntpServerName, timeServerIP);
				sendNTPpacket(timeServerIP); // send an NTP packet to a time server
				printf_P(PSTR("NTP time request sent to: %s\r\n"), timeServerIP.toString().c_str());
				lastCheck = currentTime;
				lastUpdate = currentTime;
			} else {
				wl_status_t status;
				//while ((status = WiFi.status()) != WL_CONNECTED) {
					printf_P(PSTR("Connecting to WiFi...(status=%d)\r\n"), (int)status);
					WiFi.begin("korte", "xirtaMdeL");
					  int retry = 40;
					  while (WiFi.status() != WL_CONNECTED) {
						delay(200);
						Serial.print(".");
						if (retry-- == 0) {
							//WiFi.disconnect();
							delay(1000);
							break;
						}
					  }
					  Serial.println("");
				//}

			    if ((status = WiFi.status()) == WL_CONNECTED) {
				  Serial.println("WiFi connected");
				  Serial.println("IP address: ");
				  Serial.println(WiFi.localIP());
				  printf_P(PSTR("CPU frequency: %d\r\n"), system_get_cpu_freq());
			    }
			}
		}
		if (lastCheck != 0 && lastCheck < currentTime - 2) {
			int cb = udp.parsePacket();
			if (cb) {
				lastCheck = 0;
				Serial.print("packet received, length=");
				Serial.println(cb);
				// We've received a packet, read the data from it
				udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

				//the timestamp starts at byte 40 of the received packet and is four bytes,
				// or two words, long. First, esxtract the two words:

				unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
				unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
				// combine the four bytes (two words) into a long integer
				// this is NTP time (seconds since Jan 1 1900):
				unsigned long secsSince1900 = highWord << 16 | lowWord;

				// now convert NTP time into everyday time:
				// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
				const unsigned long seventyYears = 2208988800UL;
				// subtract seventy years:
				unsigned long epoch = secsSince1900 - seventyYears;

				// daylight saving and time zone
				epoch += 60 * 60; // GMT+1
				if (IsDST(year(epoch), month(epoch), day(epoch)))
				{
					epoch += 60 * 60;	// one hour because it's a daylight saving day
				}

				setTime(epoch);

				lastUpdate = currentTime;
				lastCheck = 0;
			} else {
				printf_P(PSTR("No NTP response received yet.\r\n"));
				lastCheck = currentTime;
				if (currentTime - lastUpdate  > 10) {
					lastCheck = 0;
					lastUpdate = lastUpdate - SYNC_INTERVAL_MS;
				}
			}
		}
	}

private:
	// send an NTP request to the time server at the given address
	unsigned long sendNTPpacket(IPAddress& address) {
	  Serial.println("sending NTP packet...");
	  // set all bytes in the buffer to 0
	  memset(packetBuffer, 0, NTP_PACKET_SIZE);
	  // Initialize values needed to form NTP request
	  // (see URL above for details on the packets)
	  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	  packetBuffer[1] = 0;     // Stratum, or type of clock
	  packetBuffer[2] = 6;     // Polling Interval
	  packetBuffer[3] = 0xEC;  // Peer Clock Precision
	  // 8 bytes of zero for Root Delay & Root Dispersion
	  packetBuffer[12]  = 49;
	  packetBuffer[13]  = 0x4E;
	  packetBuffer[14]  = 49;
	  packetBuffer[15]  = 52;

	  // all NTP fields have been given values, now
	  // you can send a packet requesting a timestamp:
	  udp.beginPacket(address, 123); //NTP requests are to port 123
	  udp.write(packetBuffer, NTP_PACKET_SIZE);
	  udp.endPacket();
	}
};

#endif /* COMPONENTS_ESP8266_NTP_H_ */
