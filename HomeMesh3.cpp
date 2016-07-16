#include <stddef.h>
#include <Arduino.h>
#ifdef ARDUINO_ARCH_AVR
#include <avr/wdt.h>
#include "printf.h"
#endif
#include "avr_sleep.h"

#include "RF24Network.h"
#include "RF24.h"
#include "nRF24L01.h"
#include "RF24Mesh.h"
#include "SPI.h"
#include "DHT/dht.h"
#include "TimeLib.h"

//# define PSTR(s) (__extension__({static char __c[] PROGMEM = (s); &__c[0];}))

#define LED_PIN 		2	// the status LED pin
#if NODE_ID != 0
#define SCHEDULE_PIN 	14	// the relay pin
#if NODE_ID >= 10 && NODE_ID < 20
#define DHT22_PIN 		8 	// small sensor node 10..19 (RFTempSens kapcsolás)
#define NRF24_CE_PIN	9
#define NRF24_CS_PIN	10
#else
#define DHT22_PIN 		15 	// define if you want to read DHT sensor
#define NRF24_CE_PIN	9
#define NRF24_CS_PIN	10
#endif
#endif
#if NODE_ID == 0
// Mega 2560
//#define NRF24_CE_PIN	48
//#define NRF24_CS_PIN	SS

// Atmega 328
//#define NRF24_CE_PIN	9
//#define NRF24_CS_PIN	10

// ESP8266
#define NRF24_CE_PIN	5
#define NRF24_CS_PIN	15

#endif

/***** Configure the chosen CE,CS pins *****/
RF24 radio(NRF24_CE_PIN, NRF24_CS_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio,network);

#include "Components/itask.h"			// base interface for tasks
#include "Components/time_sync.h"		// all node use time syncronization

#if NODE_ID == 0
#include "Components/master.h"
#include "Components/esp8266_ntp.h"
#include "Components/esp8266_web.h"
//#include "Components/ethernet.h"
//#include "Components/wifi.h"
#else
#include "Components/sensors.h"
#ifdef SCHEDULE_PIN
#include "Components/schedule.h"
#endif
#endif

uint8_t taskCount = 0;
ITask* tasks[10];

void addTask(ITask *task) {
	if (taskCount < sizeof(tasks))
	{
		tasks[taskCount++] = task;
		printf_P(PSTR("Added task: %s\r\n"), task->name());
	}
}

void setup() {
#ifdef ARDUINO_ARCH_AVR
	sleep_setup();	// if you want to use sleep, or use the optiboot watchdog
#endif
	//wdt_disable();

	Serial.begin(115200);
#ifdef ARDUINO_ARCH_AVR
	printf_begin();
#endif
	printf_P(PSTR("HomeMesh 1.0\r\n"));
	printf_P(PSTR("(c) koverg70 %s %s\r\n"), __DATE__, __TIME__);

	/*
	pinMode(LED_PIN, OUTPUT);
	for (int i=1; i<3; ++i) {
		digitalWrite(LED_PIN, HIGH);
		delay(500);
		digitalWrite(LED_PIN, LOW);
		delay(500);
	}
	*/

	printf_P(PSTR("NodeID: %d, CE pin: %d, CS pin: %d\r\n"), NODE_ID, NRF24_CE_PIN, NRF24_CS_PIN);

  	// a very simple RF24 radio test
	delay(500);

	mesh.setNodeID(NODE_ID);
	printf_P(PSTR("NodeID: %d, CE pin: %d, CS pin: %d\r\n"), NODE_ID, NRF24_CE_PIN, NRF24_CS_PIN);
	mesh.begin();
	printf_P(PSTR("NodeID: %d, CE pin: %d, CS pin: %d\r\n"), NODE_ID, NRF24_CE_PIN, NRF24_CS_PIN);

	// --- now we initialize the tasks the are regularily called with new messages and to process information ----

	addTask(new TimeSync(60000, 0));		// sync frequency and target node to require time from
#if NODE_ID == 0
	Master *master = new Master();
	addTask(master);					// TODO: where to store received data
	addTask(new ESP8266Ntp());
	addTask(new ESP8266Web(master->getSensors()));
//	addTask(new Wifi(master->getSensors()));
//	addTask(new Ethernet(master->getSensors()));
#else
	addTask(new Sensors(0));				// the target node to send sensor data to
#endif
#ifdef SCHEDULE_PIN
	addTask(new Schedule(SCHEDULE_PIN));	// sync frequency and target node to require time from
#endif

	// ------------------------------------------------------------------------------------------------------------

  	for (int i = 0; i < taskCount; ++i) {
  		tasks[i]->begin();
		printf_P(PSTR("Task started: %s\r\n"), tasks[i]->name());
  	}
}

void loop(void) {
	  // Call mesh.update to keep the network updated
	  mesh.update();
#if NODE_ID == 0
	  // In addition, keep the 'DHCP service' running on the master node so addresses will
	  // be assigned to the sensor nodes
	  mesh.DHCP();
#endif

	  // Check for incoming data from the sensors
	  while (network.available()) {
	    RF24NetworkHeader header;
		uint8_t payload[40];
	    //network.peek(header);
	    boolean ok = network.read(header, &payload, sizeof(payload));
	    if (ok) {
	    	for (int i = 0; i < taskCount; ++i) {
	    		ok |= tasks[i]->receiveMessage(header, payload);
	    	}
		    if (!ok) {
		    	printf_P(PSTR("Received unknown message: %s\r\n"), header.toString());
		    }
	    }
	    else {
	    	printf_P(PSTR("Error reading network."));
	    }
	  }

  	for (int i = 0; i < taskCount; ++i) {
  		tasks[i]->updateState();
  	}
}
