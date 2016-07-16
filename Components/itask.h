/*
 * task.h
 *
 *  Created on: 2015.10.20.
 *      Author: koverg
 */

#ifndef ITASK_H_
#define ITASK_H_

class ITask
{
public:
    virtual ~ITask() {}
    virtual const char *name(void) {return "unknown";};
    virtual boolean begin(void) {return true;};
    virtual boolean receiveMessage(RF24NetworkHeader header, uint8_t *payload) = 0;
    virtual void updateState() = 0;

    static char *timeToText(time_t time, char * buffer)
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

    	return buffer;
    }
};

#endif /* ITASK_H_ */
