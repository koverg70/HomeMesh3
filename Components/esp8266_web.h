#ifndef COMPONENTS_ESP8266_WEB_H_
#define COMPONENTS_ESP8266_WEB_H_

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

ESP8266WebServer server ( 80 );
#define RSTR(x)	x
#define web_append(xxx) strcat(temp, xxx)
char temp[2000];

class NodeCommandRequestHandler : public RequestHandler {
public:
	NodeCommandRequestHandler() {}
    bool canHandle(HTTPMethod requestMethod, String requestUri) override  { return (requestMethod == HTTP_GET && requestUri.startsWith("/N")); }

    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) override {
        if (!canHandle(requestMethod, requestUri))
            return false;

        //DEBUGV("NodeCommandRequestHandler::handle: request=%s _uri=%s\r\n", requestUri.c_str(), _uri.c_str());

        String nodeStr = requestUri.substring(2, 5);
        int nodeId = (int) nodeStr.toInt();

        String message = requestUri.substring(5, 6);
        String command = requestUri.substring(6);

        if (message == "S" && command.startsWith("S") && command.length() >= 97) {
        	// convert to hex buffer
            char *buff = new char[14];
            buff[0] = 'S';
        	for (int i = 0; i < 96; ++i) {
        		if (command.charAt(1 + i) == '1') {
        			buff[1 + ( i/8 )] |= 1 << ( i%8 );
        		} else {
        			buff[1 + ( i/8 )] &= ~(1 << ( i%8 ));
        		}
        	}
        	buff[13] = 0;
    		boolean ok = mesh.write(buff, message.charAt(0), 13, nodeId);
    		printf_P(PSTR("Schedule sent to %d. Type: %c, Command: %s, Result: %d\r\n"), nodeId, message.charAt(0), buff, ok);
    		delete buff;
        } else {
            char *buff = new char[command.length()+1];
            command.toCharArray(buff, command.length()+1);

    		boolean ok = mesh.write(buff, message.charAt(0), command.length(), nodeId);
    		printf_P(PSTR("Node command sent to %d. Type: %c, Command: %s, Result: %d\r\n"), nodeId, message.charAt(0), buff, ok);
    		delete buff;
        }


		server.send ( 200, "text/plain", "OK" );
        return true;
    }
};


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
				server.addHandler( new NodeCommandRequestHandler() );
//				server.on ( "/inline", []() {
//					server.send ( 200, "text/plain", "this works as well" );
//				} );
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

	void static handleValues() {
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
					web_append(", \r\n\"s");
					web_append(timeBuff);
					web_append(RSTR("\": {"));

					web_append(("\"n\": \""));
					itoa(sensor->nodeId, timeBuff, 10);
					web_append(timeBuff);

					web_append(RSTR("\", \"lu\": \""));
					itoa(nnn - sensor->lastUpdate, timeBuff, 10);
					web_append(timeBuff);

					web_append(RSTR("\", \"t\": \""));
					timeBuff[0] = sensor->sensorType;
					timeBuff[1] = 0;
					web_append(timeBuff);

					web_append(RSTR("\", \"v\": \""));
					itoa(sensor->value, timeBuff, 10);
					web_append(timeBuff);

					web_append(RSTR("\", \"e\": \""));
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
