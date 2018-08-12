/*
 * master.h
 *
 *  Created on: 2015.10.17.
 *      Author: koverg
 */

#ifndef MASTER_H_
#define MASTER_H_

/**
 * This structure is used by the master to store several node values
 */
typedef struct
{
	uint8_t		nodeId;			// the node that collected the data
	char		sensorType;		// A,B,C,D,E,F,G,H,I,J - temperature, X,Y,Z - humidity, 0,1,2,3,4,5,6,7,8,9 - other values
	time_t 		lastUpdate; 	// the date and time of the last sensor message
	uint16_t	value;			// the sensor value
	uint16_t	extra;			// extra data (if value cannot be stored on 16 bit)
}  SensorData;

typedef struct
{
	uint8_t 	nodeId;
	uint16_t	nodeAddress;
	uint8_t 	sensorCount;

	// TODO: registeredTime, errorCount
} NodeData;

class SensorStore
{
private:
	int maxSensors;
	int sensorCount;
	SensorData** sensors;
	int maxNodes;
	int nodeCount;
	NodeData** nodes;
public:
	SensorStore(int _maxSensors, int _maxNodes) {
		nodeCount = 0;
		sensorCount = 0;
		maxSensors = _maxSensors;
		sensors = new SensorData*[maxSensors];
		for (int i = 0; i < maxSensors; ++i) {
			sensors[i] = NULL;
		}
		maxNodes = _maxNodes;
		nodes = new NodeData*[maxNodes];
		updateNodes();
	}

	void updateNodes(void) {
		// clear
		for (int i = 0; i < nodeCount; ++i) {
			delete nodes[i];
			nodes[i] = NULL;
		}
		nodeCount = 0;

		// reinsert
		for (int j = 0; j < sensorCount; ++j) {
			SensorData *sd = sensors[j];
			for (int i = 0; i < nodeCount; ++i) {
				if (nodes[i]->nodeId == sd->nodeId) {
					nodes[i]->sensorCount++;
					return;
				}
			}
			if (nodeCount < maxNodes) {
				NodeData *nd = new NodeData();
				nd->nodeId = sd->nodeId;
				nd->nodeAddress = mesh.getAddress(nd->nodeId);
				nd->sensorCount = 1;
				nodes[nodeCount++] = nd;
			}
		}
	}

	void updateNode(uint8_t nodeId, boolean newSensor) {
		boolean foundNode = false;
		for (int i = 0; i < nodeCount; ++i) {
			if (nodes[i]->nodeId == nodeId) {
				foundNode = true;
				nodes[i]->nodeAddress = mesh.getAddress(nodeId);
				if (newSensor) {
					++(nodes[i]->sensorCount);
				}
			}
		}
		if (!foundNode) {
			NodeData *nd = new NodeData();
			nd->nodeId = nodeId;
			nd->nodeAddress = mesh.getAddress(nodeId);
			nd->sensorCount = 1;
			nodes[nodeCount++] = nd;
		}
	}

	SensorData *storeData(RF24NetworkHeader header, uint8_t *payload) {
		SensorData *sd = new SensorData();
		uint16_t nodeId = mesh.getNodeID(header.from_node);
		if ((int)nodeId > 0) {
			sd->nodeId = nodeId;
			sd->sensorType = (char)payload[0];
			sd->value = payload[2]*256+payload[1];		// TODO: platform independent solution, this not ok on xtensa *((uint16_t *)(payload + 1));
			sd->extra = 0;
			sd->lastUpdate = now();
			//char buffer[30];
			//ITask::timeToText(now(), buffer);
			//printf_P(PSTR("Time: %s Node:%d sensor:%c value:%d\r\n"), buffer, sd->nodeId, sd->sensorType, sd->value);
			int i;

			boolean sensorStored = false; // already exist
			boolean newSensor = false;
			for (i = 0; i < sensorCount; ++i) {
				if (sensors[i]->nodeId == sd->nodeId && sensors[i]->sensorType == sd->sensorType) {
					if (sensors[i] != NULL) {
						delete sensors[i];
					}
					sensors[i] = sd;
					sensorStored = true;
				}
			}
			if (! sensorStored) {
				if (sensorCount < maxSensors) {
					sensors[sensorCount++] = sd;
					sensorStored = true;
					newSensor = true;
				}
				// new sensor detected, we should update node list
				// TODO: ez mikor is kell??? updateNodes();
			}

			updateNode(sd->nodeId, newSensor);

			if (sensorStored) {
				return sd;
			} else {
				return NULL;
			}
		} else {
			return NULL;
		}
	}

	int getSensorCount() {
		return sensorCount;
	}

	int getNodeCount() {
		return nodeCount;
	}

	SensorData **getSensors() {
		return sensors;
	}

	NodeData **getNodes() {
		return nodes;
	}
};

class Master : public ITask
{
private:
	uint32_t addressTimer;
	SensorStore *sensorStore;
public:
	Master() {
		addressTimer = 0;
		sensorStore = new SensorStore(25, 5);
	}

	boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload) {
		if (header.type == 'V') {
		  printf_P(PSTR("Received sensor from nodeId:%d address:%#o sensor:%c value:%d\r\n"), mesh.getNodeID(header.from_node), header.from_node, (char)payload[0], payload[2]*256+payload[1]);
		  if (sensorStore != NULL) {
			  SensorData *sd = sensorStore->storeData(header, payload);
			  if (sd == NULL) {
				  printf_P(PSTR("!!! ESD !!!"));
			  } else {
				  //printf_P(PSTR("MSG node:%#d s:%c v:%d e:%d\r\n"), sd->nodeId, sd->sensorType, sd->value, sd->extra);
			  }
			  return true;
		  } else {
			  printf_P(PSTR("sensorStore is not initialized\r\n"));
		  }
		}
		return false;
	}

	const char *name() { return "Master"; };

	void updateState() {
		  if(millis() - addressTimer > 3000){
			addressTimer = millis();
			 for(int i=0; i<mesh.addrListTop; i++){
			   printf_P(PSTR("** NodeID: %d RF24Network Address: %o\r\n"), mesh.addrList[i].nodeID, mesh.addrList[i].address);
			 }
//			 printf_P(PSTR("Node count: %d\r\n"), sensorStore->getNodeCount());
//			 printf_P(PSTR("Sensor count: %d\r\n"), sensorStore->getSensorCount());
//			 for (int i = 0; i < sensorStore->getNodeCount(); ++i)
//			 {
//				 NodeData * node = sensorStore->getNodes()[i];
//				 printf_P(PSTR("Node id:%d address:%o, sensorCount:%d\r\n"), node->nodeId, node->nodeAddress, node->sensorCount);
//			 }
//			 for (int i = 0; i < sensorStore->getSensorCount(); ++i)
//			 {
//				 SensorData * sensor = sensorStore->getSensors()[i];
//				 char buffer[30];
//				 ITask::timeToText(sensor->lastUpdate, buffer);
//				 printf_P(PSTR("Sensor node id:%d type:%c, value:%d, extra:%d, lastUpdate:%s\r\n"),
//				   sensor->nodeId, sensor->sensorType, sensor->value, sensor->extra, buffer);
//			 }

			// --- debug get time and set schedule on node 1 (bathroom sensor and ventilating fan)---
			if (year(now()) <= 2013) {
/*				uint8_t buffer[5];
				buffer[0] = 'G';
				mesh.write(buffer, 'T', sizeof(buffer), 1);
				printf_P(PSTR("TRS: 1.\r\n"));
				setTime(23,31,0,1,12,2015);*/
			}
			else {
/*
				uint8_t buffer[13] = { 'S', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff };
				//for (int i = 0; i < 12; ++i) { buffer[1 + i] = 0xff; }
				bool res = mesh.write(buffer, 'S', sizeof(buffer), 1);
				printf_P(PSTR("SMS: %d\r\n"), res);
*/
			}
			// --- debug get time and set schedule ---

		  }
	}

	SensorStore *getSensors() {
		return sensorStore;
	}
};

#endif /* MASTER_H_ */
