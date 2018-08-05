/*
 * sensors.h
 *
 *  Created on: 2015.10.17.
 *      Author: koverg
 */

#ifndef SENSORS_H_
#define SENSORS_H_

#ifdef ONEWIRE_PIN

class Sensor /* hold info for a DS18 class digital temperature sensor */
{
public:
	byte addr[8];
	boolean parasite;
	float temp;

};

// OneWire commands
#define CONVERT_T       0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy EEPROM
#define READSCRATCH     0xBE  // Read EEPROM
#define WRITESCRATCH    0x4E  // Write to EEPROM
#define RECALLSCRATCH   0xB8  // Reload from last known
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition

// Model IDs
#define DS18S20      0x10
#define DS18B20      0x28
#define DS1822       0x22

byte data[12];
byte addr[8];
OneWire ds(ONEWIRE_PIN); // DS18S20 Temperature chip i/o
Sensor DS[ONEWIRE_MAX]; /* array of digital sensors */

byte sensors = 0;

void initDS(void) {
	byte i;
	byte present = 0;
	// initialize inputs/outputs
	// start serial port
	sensors = 0;
	printf("Searching for sensors...\r\n");
	while (ds.search(addr)) {
		if (OneWire::crc8(addr, 7) != addr[7]) {
			printf("CRC is not valid!\n");
			break;
		}
		delay(1000);
		ds.write(READPOWERSUPPLY);
		boolean parasite = !ds.read_bit();
		present = ds.reset();
		printf("temp%d: ", sensors);
		DS[sensors].parasite = parasite;
		for (i = 0; i < 8; i++) {
			DS[sensors].addr[i] = addr[i];
			printf("%x ", addr[i]);
		}
		if (addr[0] == DS18S20) {
			printf(" DS18S20");
		} else if (addr[0] == DS18B20) {
			printf(" DS18B20");
		} else {
			printf(" unknown");
		}
		if (DS[sensors].parasite) {
			printf(" parasite");
		} else {
			printf(" powered");
		}
		printf("\r\n");
		sensors++;
	}
	printf("%d sensors found\r\n", sensors);
	printf("OneWire pin: %d\r\n", ONEWIRE_PIN);
	for (i = 0; i < sensors; i++) {
		printf("temp%d", i);
		if (i < sensors - 1) {
			printf(",");
		}
	}
	printf("\r\n");
}

void get_ds()
{
	byte i, j;
	boolean ready;
	int dt;
	int HighByte, LowByte, TReading, SignBit, Tc_100;
	byte present = 0;
	for (i = 0; i < sensors; i++) {
		ds.reset();
		ds.select(DS[i].addr);
		ds.write(CONVERT_T, DS[i].parasite); // start conversion, with parasite power off at the end

		if (DS[i].parasite) {
			dt = 75;
			delay(750); /* no way to test if ready, so wait max time */
		} else {
			ready = false;
			dt = 0;
			delay(10);
			while (!ready && dt < 75) { /* wait for ready signal */
				delay(10);
				ready = ds.read_bit();
				dt++;
			}
		}

		present = ds.reset();
		ds.select(DS[i].addr);
		ds.write(READSCRATCH); // Read Scratchpad

		for (j = 0; j < 9; j++) { // we need 9 bytes
			data[j] = ds.read();
		}

		/* check for valid data */
		if ((data[7] == 0x10) || (OneWire::crc8(addr, 8) != addr[8])) {
			LowByte = data[0];
			HighByte = data[1];
			TReading = (HighByte << 8) + LowByte;
			SignBit = TReading & 0x8000; // test most sig bit
			if (SignBit) // negative
			{
				TReading = (TReading ^ 0xffff) + 1; // 2's comp
			}
			if (DS[i].addr[0] == DS18B20) { /* DS18B20 0.0625 deg resolution */
				Tc_100 = (6 * TReading) + TReading / 4; // multiply by (100 * 0.0625) or 6.25
			} else if (DS[i].addr[0] == DS18S20) { /* DS18S20 0.5 deg resolution */
				Tc_100 = (TReading * 100 / 2);
			}

			if (SignBit) {
				DS[i].temp = -(float) Tc_100 / 100;
			} else {
				DS[i].temp = (float) Tc_100 / 100;
			}
		} else { /* invalid data (e.g. disconnected sensor) */
			DS[i].temp = NAN;
		}
	}
}
#endif

class Sensors: public ITask
{
private:
	uint16_t childRefresh;			// frequency of send data to targetNode
	uint8_t nodeId;					// the target node to send sensor values to
	unsigned long lastSendMillis;	// last time the sensor data was send
#if defined(DHT22_PIN) || defined(DHT11_PIN)
	dht dht_sensor;
#endif

public:
	Sensors(uint8_t _nodeId)
	{
		nodeId = _nodeId;
		childRefresh = 5000;
		lastSendMillis = 0;
    	digitalWrite(3, LOW);
    	digitalWrite(4, HIGH);
    	digitalWrite(5, HIGH);
#ifdef ONEWIRE_PIN
    	initDS();
#endif
#ifdef BATTERY_PIN
    	pinMode(BATTERY_PIN, INPUT);
#endif
	}

	const char *name() { return "Sensors"; };

	void updateState() {
		  // Send to the target node every given time
		  if (millis() - lastSendMillis >= childRefresh) {
		    lastSendMillis = millis();
			boolean ok = true;

#ifdef DHT22_PIN
			dht_sensor.read22(DHT22_PIN);
#elif DHT11_PIN
			dht_sensor.read11(DHT11_PIN);
#endif
#if defined(DHT22_PIN) || defined(DHT11_PIN)
			uint8_t payload[3];
			payload[0] = 'A';
			uint16_t temperature = (int16_t)(dht_sensor.temperature * 100);
			*((uint16_t *)(payload + 1)) = temperature;
			ok &= mesh.write(&payload, 'V', sizeof(payload), nodeId);

			delay(100);

			payload[0] = 'X';
			uint16_t humidity = (int16_t)(dht_sensor.humidity * 100);
			*((uint16_t *)(payload + 1)) = humidity;
			ok &= mesh.write(&payload, 'V', sizeof(payload), nodeId);

			delay(100);

			printf("Temperature: %d, Humidity: %d, \r\n", temperature, humidity);
#endif

#ifdef ONEWIRE_PIN
			if (sensors > 0) {
				get_ds();
				for (int i=0; i<sensors; ++i) {
					payload[0] = 'B' + i;
					uint16_t temperature = (int16_t) (DS[i].temp * 100);
					*((uint16_t *)(payload + 1)) = temperature;
					ok &= mesh.write(&payload, 'V', sizeof(payload), nodeId);

					delay(100);

					printf("DS18B20 temperature: %d, \r\n", temperature);
				}
			}
#endif

#ifdef BATTERY_PIN
			float val = 0;
			for (int i = 0; i < 4; ++i) {
				val += analogRead(BATTERY_PIN);
			}
			// average
			val = val / 4;
			// the result should be voltage * 100
			val = (val * VOLTAGE_DIVIDER * MCU_REF_VOLTAGE) / (1023/100);
			payload[0] = '0';
			uint16_t battery = val;
			*((uint16_t *)(payload + 1)) = battery;
			ok &= mesh.write(&payload, 'V', sizeof(payload), nodeId);

			delay(100);

			printf("Battery voltage: %d, \r\n", battery);
#endif

			// FIXIT: a státusz (ok) kezelése
		    if (!ok) {
		      // If a write fails, check connectivity to the mesh network
		      if ( ! mesh.checkConnection() ) {
		        //refresh the network address
		    	digitalWrite(3, HIGH);
		    	digitalWrite(4, LOW);
		    	digitalWrite(5, LOW);
		        printf("Connection broken, renewing Address\r\n");
		        mesh.renewAddress();
		      } else {
		        printf("Send fail, Test OK\r\n");
		      }
		    } else {
		      printf("Send OK.\r\n");
		    	digitalWrite(3, LOW);
		    	digitalWrite(4, HIGH);
		    	digitalWrite(5, LOW);
		    }
		  }

	}

	boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload)
	{
		if (header.type == 'C')
		{
			nodeId = payload[0];
			childRefresh = *((uint16_t *)(payload + 1));
			return true;
		}
		return false;
	}

};

#endif /* SENSORS_H_ */
