/*
 * schedule.h
 *
 *  Created on: 2015.10.17.
 *      Author: koverg
 */

#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#define RELAY_CALC_SEC 2

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

void printTiming(uint8_t *timing)
{
	printf ("Timing: ");
	for (int i = 0; i < 12; ++i)
	{
		printf(BYTETOBINARYPATTERN, BYTETOBINARY(timing[i]));
	}
	printf ("\r\n");
}

class Schedule : public ITask
{
private:
	int heatSeconds;	// ennyit ment a fûtés
	time_t lastMessage;	// ekkor volt küldve az utolsó üzenet
	time_t lastCalculate;
	boolean receive;
	uint8_t timing[12];
	boolean heatOn;
	time_t heatStart;
	char timeBuffer[22];
	uint8_t relayPin;
public:
	Schedule(uint8_t _relayPin)
	{
		relayPin = _relayPin;
		heatSeconds = 0;	// ennyit ment a fûtés
		lastMessage = 0;	// ekkor volt küldve az utolsó üzenet
		lastCalculate = 0;
		receive = false;
		heatOn = false;
		heatStart = 0;

		// clear the timing
		for (int i = 0; i < 12; ++i)
		{
			timing[i] = 0;
		}
	}

    virtual boolean begin(void) {
    	pinMode(SCHEDULE_PIN, OUTPUT);
    }

	uint8_t calcQuarter(time_t currentTime)
	{
		int h = hour(currentTime);
		int m = minute(currentTime);
		return h * 4 + (m / 15);
	}

	uint8_t calculate(time_t currentTime)
	{
		uint8_t currentQuarter = calcQuarter(currentTime);

		int idx = currentQuarter / 8;
		int bit = 7 - (currentQuarter % 8);

		uint8_t ret = (timing[idx] >> bit) & 1;

		return ret;
	}

public:
	const char *name() { return "Schedule"; };

	boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload)
	{
		if (header.type == 'S')
		{
			if (payload[0] == 'S') {
				// update schedule
				printf_P(PSTR("Schedule message received...\r\n"));
				memcpy(&timing, payload + 1, sizeof(timing));
				printTiming(timing);
			} else if (payload[0] == 'G') {
				// get schedule
				uint8_t nodeId = mesh.getNodeID(header.from_node);
				delay(50);	// FIXIT: queue?
				boolean ok = mesh.write(timing, 'S', 12, nodeId);
				printf_P(PSTR("Schedule values sent to %d. Result: %d\r\n"), nodeId, ok);
				printTiming(timing);
			}
			return true;
		}
		return false;
	}

	void updateState()
	{
		time_t currentTime = now();
		if (year(currentTime) <= 2013)
		{
			heatOn = false;
			digitalWrite(relayPin, LOW);	// bad date turn relay off
		}
		else if (currentTime - lastCalculate >= RELAY_CALC_SEC)
		{
			if (day(currentTime) != day(lastCalculate))
			{
				heatSeconds = 0;	// moved to next day, so we clear the heat seconds
			}

			if (calculate(currentTime))
			{
				if (!heatOn)
				{
					printf_P(PSTR("Relay ON.\r\n"));
					heatOn = true;
					heatStart = currentTime;
				}
			}
			else
			{
				if (heatOn)
				{
					printf_P(PSTR("Relay OFF.\r\n"));
					heatOn = false;
					heatSeconds += (currentTime - heatStart);
					heatStart = 0;
					printf_P(PSTR("Heat seconds: %d\r\n"), heatSeconds);
				}
			}

			digitalWrite(relayPin, heatOn ? HIGH : LOW);

			lastCalculate = currentTime;
		}
	}
};

#endif /* SCHEDULE_H_ */
