#ifndef COMPONENTS_ESP8266_WEB_H_
#define COMPONENTS_ESP8266_WEB_H_

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

ESP8266WebServer server ( 80 );

class ESP8266Web : public ITask {
private:
	boolean inited;
	time_t startTime;
	SensorStore *sensors;
	static ESP8266Web *esp;
public:
	ESP8266Web(SensorStore *sensors_)
    {
		sensors = sensors_;
		inited = false;
		startTime = 0;
		esp = this;
	}

public:
	const char *name() { return "ESP8266Web"; };

	boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload) {
	}

	void updateState(){
		if (year(now()) > 2013) {
			if (!inited) {
				startTime = now();

				inited = true;

				if ( MDNS.begin ( "esp8266" ) ) {
					Serial.println ( "MDNS responder started" );
				}

				server.on ( "/V", &ESP8266Web::handleValues );
				server.on ( "/inline", []() {
					server.send ( 200, "text/plain", "this works as well" );
				} );
				server.onNotFound ( &ESP8266Web::handleNotFound );
				server.begin();
				Serial.println ( "HTTP server started" );
			} else {
				server.handleClient();
			}
		}
	}

	void static timeDateToText(time_t time, char *buffer)
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

#define RSTR(x)	x
#define web_append(xxx) strcat(temp, xxx)

	void static handleValues() {
		char temp[1000];
		temp[0] = 0;

		char timeBuff[30];

		time_t nnn = now();
		web_append(RSTR("{\"time\": \""));
		timeDateToText(nnn, timeBuff);
		web_append(timeBuff);
		web_append(RSTR("\", \"start\": \""));
		timeDateToText(esp->startTime, timeBuff);
		web_append(timeBuff);
		web_append(("\""));

		if (esp->sensors != NULL)
		{
			for (int j = 0; j < esp->sensors->getNodeCount(); ++j)
			{
				NodeData *node = esp->sensors->getNodes()[j];
				for (int i = 0; i < esp->sensors->getSensorCount(); ++i)
				{
					/*
					uint8_t		nodeId;			// the node that collected the data
					char		sensorType;		// A,B,C,D,E,F,G,H,I,J - temperature, X,Y,Z - humidity, 0,1,2,3,4,5,6,7,8,9 - other values
					time_t 		lastUpdate; 	// the date and time of the last sensor message
					uint16_t	value;			// the sensor value
					uint16_t	extra;			// extra data (if value cannot be stored on 16 bit)
					*/


					SensorData * sensor = (esp->sensors->getSensors())[i];
					itoa(i, timeBuff, 10);
					web_append(", \"sensors");
					web_append(timeBuff);
					web_append(RSTR("\": {"));

					web_append(("\"nodeId\": \""));
					itoa(sensor->nodeId, timeBuff, 10);
					web_append(timeBuff);

					web_append(RSTR("\", \"lastUpdate\": \""));
					itoa(nnn - sensor->lastUpdate, timeBuff, 10);
					web_append(timeBuff);

					web_append(RSTR("\", \"sensorType\": \""));
					timeBuff[0] = sensor->sensorType;
					timeBuff[1] = 0;
					web_append(timeBuff);

					web_append(RSTR("\", \"value\": \""));
					itoa(sensor->value, timeBuff, 10);
					web_append(timeBuff);

					web_append(RSTR("\", \"extra\": \""));
					itoa(sensor->extra, timeBuff, 10);
					web_append(timeBuff);

					web_append(RSTR("\"}"));
				}
			}
		}
		web_append(RSTR("}"));

		server.send ( 200, "text/json", temp );
	}

	void static handleNotFound() {
		String message = "File Not Found\n\n";
		message += "URI: ";
		message += server.uri();
		message += "\nMethod: ";
		message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
		message += "\nArguments: ";
		message += server.args();
		message += "\n";

		for ( uint8_t i = 0; i < server.args(); i++ ) {
			message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
		}

		server.send ( 404, "text/plain", message );
	}

};

ESP8266Web *ESP8266Web::esp = NULL;

#endif /* COMPONENTS_ESP8266_WEB_H_ */
