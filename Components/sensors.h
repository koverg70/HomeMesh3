/*
 * sensors.h
 *
 *  Created on: 2015.10.17.
 *      Author: koverg
 */

#ifndef SENSORS_H_
#define SENSORS_H_


class Sensors: public ITask
{
private:
	uint16_t childRefresh;			// frequency of send data to targetNode
	uint8_t nodeId;					// the target node to send sensor values to
	unsigned long lastSendMillis;	// last time the sensor data was send
#ifdef DHT22_PIN
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
	}

	const char *name() { return "Sensors"; };

	void updateState() {
		  // Send to the target node every given time
		  if (millis() - lastSendMillis >= childRefresh) {
		    lastSendMillis = millis();
			boolean ok = true;

#ifdef DHT22_PIN
			uint8_t payload[3];
			dht_sensor.read22(DHT22_PIN);
			payload[0] = 'A';
			uint16_t temperature = (int16_t)(dht_sensor.temperature * 100);
			*((uint16_t *)(payload + 1)) = temperature;
			ok &= mesh.write(&payload, 'V', sizeof(payload), nodeId);

			payload[0] = 'X';
			uint16_t humidity = (int16_t)(dht_sensor.humidity * 100);
			*((uint16_t *)(payload + 1)) = humidity;
			ok &= mesh.write(&payload, 'V', sizeof(payload), nodeId);

			printf("Temperature: %d, Humidity: %d, ", temperature, humidity);
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
