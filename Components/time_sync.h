/*
 * timing.h
 *
 *  Created on: 2015.10.17.
 *      Author: koverg
 *
 *  Minden csomópont kezeli az idõt:
 *  - le lehet kérdezni tõle az aktuális idõt
 */

#ifndef TIME_SYNC_H_
#define TIME_SYNC_H_

class TimeSync : public ITask
{
private:
	unsigned long lastCheck;
	unsigned long lastUpdate;
	uint16_t checkCount;
	uint16_t checkFreq;
	uint16_t timeServerNode;

	uint32_t ledTimer;
	uint8_t ledState;
public:
	TimeSync(uint16_t _checkFreq, uint16_t _timeServerNode)
	{
		checkFreq = _checkFreq;
		lastCheck = 0;
		lastUpdate = 0;
		timeServerNode = _timeServerNode;

		ledTimer = 0;
		ledState = HIGH;
	}

	const char *name() { return "TimeSync"; };

	boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload)
	{
		char buffer[21];
		if (header.type == 'T')
		{
			time_t currentTime = now();

			if (payload[0] == 'S') {
				// set time
				time_t senderTime = *((time_t *)(payload + 1));
				printf_P(PSTR("Time received: %s, from address: %d\r\n"), ITask::timeToText(senderTime, buffer), header.from_node);
				// csak akkor fogadjuk el, ha értelmes évszám van benne
				if (year(senderTime) >= 2013)
				{
					printf_P(PSTR("Time set: %s\r\n"), ITask::timeToText(senderTime, buffer));
					setTime(senderTime);
					lastUpdate = millis();
					checkCount = 0;
				}
			} else if (payload[0] == 'G') {
				if (year(currentTime) > 2013) {
					// csak akkor adjuk vissza az idõt, ha nálunk már be van állítva
					uint8_t buffer[5];
					buffer[0] = 'S';
					memcpy(buffer+1, &currentTime, sizeof(currentTime));
					uint8_t nodeId = mesh.getNodeID(header.from_node);
					delay(50);	// FIXIT: queue?
					boolean ok = mesh.write(buffer, 'T', sizeof(buffer), nodeId);
					printf_P(PSTR("Time sent to %d. Result: %d\r\n"), nodeId, ok);
				}
			}
			return true;
		}
		return false;
	}

	void updateState()
	{
		if (checkFreq > 0)
		{
			// ha még nem volt beállítva az idõ, vagy ideje megújítani
			if (lastUpdate == 0 || millis() - lastUpdate > checkFreq)
			{
				// legfeljebb 2 másodpercenként nézzük, amíg nem kapunk idõt
				if (lastCheck == 0 || millis() - lastCheck > 2000)
				{
// FIXIT: see replacement
//					if (!mesh.checkConnection())
//					{
//					}
					if (timeServerNode != NODE_ID)	// we do not want to send request to ourself
					{
						uint8_t buffer[1];
						buffer[0] = 'G';
						mesh.write(buffer, 'T', sizeof(buffer), timeServerNode);
						printf_P(PSTR("Time request sent to node: %d\r\n"), timeServerNode);
						lastCheck = millis();
						checkCount++;
					}

#if NODE_ID != 0	// a master node nem release-el
					// nem kaptunk idõt az elõzõ 3 lekérdezésre, úgy vesszük, hogy nem él a master
					if (year(now()) > 2013 && lastCheck != 0 && checkCount > 3)
					{
						lastUpdate = 0;
						checkCount = 0;
						// setTime(0); - az idõt ne töröljük, mert felesleges kapcsolgatás lehet belõle
						printf_P(PSTR("Connection lost. Address released.\r\n"));
						mesh.releaseAddress();
					}
#endif
				}
			}
		}

		// LED blinker
		if(millis() - ledTimer > 5000) {
			ledTimer = millis();
			ledState = ledState == HIGH ? LOW : HIGH;
			char buffer[21];
			time_t currentTime = now();
			printf_P(PSTR("Current time is: %s (%ld)\r\n"), timeToText(currentTime, buffer), currentTime);
	    }
		digitalWrite(LED_PIN, ledState);
	}

};

#endif /* TIME_SYNC_H_ */
