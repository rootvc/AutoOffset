/* Automate carbon offset using Particle and OBDII
 * made by Kane
 * lol you're using my code godspeed
 *
 * Accumulates miles in a counter stored in EEPROM.
 * When carbon offset interval is reached, use wren.co to automatically purchase offsets.
 * 
 * Based on https://github.com/carloop/app-reminder
 * Hardware required: Carloop Basic and Particle Photon
 */

#include "application.h"
#include "carloop/carloop.h"
#include <math.h>

// TODO: FILL THESE OUT!
const double mpg = 0.25;	// mpg of your vehicle; 2004 Jeep Wrangler gets 14 mpg; using 0.25 for testing
const double fractionTon = 10.0;	// fraction of ton you want offsets to run, eg. 10 -> every 1/10 ton (i think the wren min)

// Don't block the main program while connecting to WiFi/cellular.
// This way the main program runs on the Carloop even outside of WiFi range.
SYSTEM_THREAD(ENABLED);

// Tell the program which revision of Carloop you are using.
Carloop<CarloopRevision2> carloop;

// Function declarations
void setupCloud();
int resetIntervalCounter(String);
int changeIntervalLimit(String arg);
void updateSpeed();
void requestVehicleSpeed();
void waitForVehicleSpeedResponse();
void updateMileage(uint8_t newVehicleSpeedKmh);
double computeDeltaMileage(uint8_t newVehicleSpeedKmh);
void checkIntervalLimit();
void storeMileage();
void loadFromStorage();
void saveToStorage();
void test();

// Helpers for converting doubles to strings
void ftoa(float n, char *res, int afterpoint);
int intToStr(int x, char str[], int d);
void reverse(char *str, int len);

// Structures

/*
 * This struct must not be re-ordered since it is the EEPROM layout.
 * Elements must not be deleted.
 * To remove an element, replace the name with _unused1/2/3.
 */
struct Data
{
	uint16_t appId;	// Used to make sure the EEPROM was properly initialized for this app
	uint16_t version;	// Increment in case more fields are added in a later version

	double intervalCounter;	// The count of miles
	double intervalLimit;	// The upper limit of miles to trigger a reminder
	uint8_t intervalReached;	// (this isn't used anymore) Whether a reminder must be triggered next time the Carloop is online
	double tonsOffset;	// total tons offset
};

// 100 gallons = 1 ton CO2
// https://www.epa.gov/energy/greenhouse-gases-equivalencies-calculator-calculations-and-references
const double gallonsPerTonCarbon = 100.0;	// gallons per ton of carbon
const double milesPerTonCarbon = gallonsPerTonCarbon * mpg;	// miles per fractional ton carbon
const double milesPerFractionTonCarbon = milesPerTonCarbon / fractionTon;

// The default values for the EEPROM on first run
const Data DEFAULT_DATA = {
	/*appId */
	0x4352,	// Letters CR = Carloop Reminder
	/*version */
	1,
	/*intervalCounter */
	0.0,
	/*intervalLimit */
	0.0,
	/*intervalReached */
	0,
	/*tonsOffset */
	0.0,
};

// The data that is stored and loaded in permanent storage (EEPROM)
Data data;

// Only store to EEPROM every so often
const auto STORAGE_PERIOD = 60 * 1000; /*every minute */
uint32_t lastStorageTime = 0;

// OBD constants for CAN
// reference: https://x-engineer.org/on-board-diagnostics-obd-modes-operation-diagnostic-services/

// CAN IDs for OBD messages
const auto OBD_CAN_REQUEST_ID = 0x7E0;
const auto OBD_CAN_REPLY_ID = 0x7E8;

// Modes (aka services) for OBD
const auto OBD_MODE_CURRENT_DATA = 0x01;

// OBD signals (aka PID) that can be requested
const auto OBD_PID_VEHICLE_SPEED = 0x0d;

// Time to wait for a reply for an OBD request
const auto OBD_TIMEOUT_MS = 20;

int obdResponseCount = 0;

uint8_t vehicleSpeedKmh = 0;
uint32_t lastVehicleSpeedUpdateTime = 0;

/*
 * Called at boot
 * Sets up the CAN bus and cloud functions
 */
void setup()
{
	setupCloud();

	// set up variables; write interval limit to EEPROM
	vehicleSpeedKmh = 0;
	lastVehicleSpeedUpdateTime = 0;
	data.intervalLimit = milesPerFractionTonCarbon;
	saveToStorage();

	// Configure the CAN bus speed for 500 kbps, the standard speed for the OBD-II port.
	// Other common speeds are 250 kbps and 1 Mbps.
	carloop.setCANSpeed(500000);

	// Connect to the CAN bus
	carloop.begin();
}

/*
 * Allow interacting with the Carloop remotely
 */
void setupCloud()
{
	Particle.function("reset", resetIntervalCounter);
	Particle.function("limit", changeIntervalLimit);
	Particle.variable("count", data.intervalCounter);
	Particle.variable("msg", obdResponseCount);
}

/*
 * Reset the interval counter and store the zero value in EEPROM
 */
int resetIntervalCounter(String = String())
{
	data.intervalCounter = 0;
	saveToStorage();
	return 0;
}

/*
 * Set the interval upper limit and make sure the current value is below
 * that. Store the values value in EEPROM
 */	
int changeIntervalLimit(String arg)
{
	long newLimit = arg.toInt();
	if (newLimit <= 0)
	{
		return -1;
	}

	data.intervalLimit = newLimit;
	checkIntervalLimit();
	saveToStorage();
	return 0;
}

/*
 * Called over and over
 *
 * Process new CAN messages here to update the vehicle speed, update
 * the mileage and update the interval counter
 */
void loop()
{
	updateSpeed();
	storeMileage();
	// test();
	checkIntervalLimit();
	delay(100);
}

void test()
{
	// convert to string bc publish() takes string
	char str[16];
	ftoa(data.intervalLimit / milesPerTonCarbon, str, 2);

	Particle.publish("TEST Offset Carbon", str, PRIVATE);
	delay(2000);
}

/*
 * Request the vehicle speed through OBD and wait for the response
 */
void updateSpeed()
{
	requestVehicleSpeed();
	waitForVehicleSpeedResponse();
}

/*
 * Send a PID request for the vehicle speed
 */
void requestVehicleSpeed()
{
	CANMessage message;

	// A CAN message to request the vehicle speed
	message.id = OBD_CAN_REQUEST_ID;
	message.len = 8;

	// Data is an OBD request: get current value of the vehicle speed PID
	message.data[0] = 2;	// 2 byte request
	message.data[1] = OBD_MODE_CURRENT_DATA;
	message.data[2] = OBD_PID_VEHICLE_SPEED;

	// Send the message on the bus!
	carloop.can().transmit(message);
}

/*
 * Wait for the PID response with a timeout and update mileage to new
 * value if response received — otherwise, update mileage to zero
 */
void waitForVehicleSpeedResponse()
{
	uint32_t start = millis();
	while ((millis() - start) < OBD_TIMEOUT_MS)
	{
		CANMessage message;
		if (carloop.can().receive(message))
		{
			if (message.id == OBD_CAN_REPLY_ID &&
				message.data[2] == OBD_PID_VEHICLE_SPEED)
			{
				uint8_t newVehicleSpeedKmh = message.data[3];
				updateMileage(newVehicleSpeedKmh);
				obdResponseCount++;
				return;
			}
		}
	}

	// A timeout occurred
	updateMileage(0);
}

/*
 * Update the interval counter based on the new speed and check if the
 * interval limit has been reached
 */
void updateMileage(uint8_t newVehicleSpeedKmh)
{
	double deltaMileage = computeDeltaMileage(newVehicleSpeedKmh);
	data.intervalCounter += deltaMileage;
	// TODO-dev [2022-02-21] Does this need to be saved to storage?
}

/*
 * Calculate the increase in mileage given the old and new speed
 */
double computeDeltaMileage(uint8_t newVehicleSpeedKmh)
{
	uint32_t now = millis();
	double deltaMileage = 0.0;
	int PLAUSIBLE_DIFF_MS = 1000;
	// TODO-dev: [2022-02-21] How was this determined?

	// If the speed was previously 0 or newly 0, or timed out because the
	// car was off, just save the new speed value
	// TODO-dev: [2022-02-21] Is the comment above still correct?
	if (vehicleSpeedKmh > 0 && newVehicleSpeedKmh > 0)
	{
		// The car was previously driving and is still driving

		// Figure out the distance driven using the trapezoidal rule
		uint32_t msDiff = now - lastVehicleSpeedUpdateTime;
		// Calculate only if the difference is plausible
		if (msDiff < PLAUSIBLE_DIFF_MS)
		{
			// distance in km/h * ms
			uint32_t deltaDistance = msDiff * (vehicleSpeedKmh + newVehicleSpeedKmh) / 2;

			// Convert to miles
			// (1 kilometer per hour * ms) * 1.72603109 × 10^-7  = miles
			// (km/h * ms) *(1 h / 3600 s) * (0.621371 mi / 1 km) * (1 s / 1000 ms)
			deltaMileage = deltaDistance * 1.72603109e-7;
		}
	}

	// Update logged speed and time with latest values
	vehicleSpeedKmh = newVehicleSpeedKmh;
	lastVehicleSpeedUpdateTime = now;
	return deltaMileage;
}

/*
 * If the interval limit is reached, mark it so we can publish an event
 * when we come back to network range
 * TODO-dev: [2022-02-21] Is this description correct? Seems like data
 *     will only be updated if the limit is reached AND we're connected,
 *     and this function won't do anything otherwise? Could probably benefit
 *     from a rename if so since it's doing more than just checking.
 */
void checkIntervalLimit()
{
	int RATE_LIMIT_DELAY_MS = 1000;
	// over limit and connected
	while ((data.intervalCounter >= data.intervalLimit) && Particle.connected())
	{
		// calculate amount to offset
		// this should equal 1/fractionTon
		double tcOffset = data.intervalLimit / milesPerTonCarbon;

		// increment total aggregate offset
		data.tonsOffset += tcOffset;

		// publish amount to offset via wren api
		char str[16];
		ftoa(tcOffset, str, 2);
		Particle.publish("Offset Carbon", str, PRIVATE);
		delay(RATE_LIMIT_DELAY_MS);	// don't trip the rate limiter

		// publish total amount offset
		char str2[16];
		ftoa(data.tonsOffset, str2, 2);
		Particle.publish("Total Tons Carbon Offset", str2, PRIVATE);
		delay(RATE_LIMIT_DELAY_MS);	// don't trip the rate limiter

		// publish total miles offset
		char str3[16];
		ftoa(data.intervalCounter, str3, 2);
		Particle.publish("Total Miles Driven", str3, PRIVATE);
		delay(RATE_LIMIT_DELAY_MS);	// don't trip the rate limiter

		// decrement the counter by the limit we just offset
		data.intervalCounter -= data.intervalLimit;

		// update EEPROM intervalCounter
		saveToStorage();
	}
	// otherwise just keep counting until you get to internet
}

/*
 * Store data to EEPROM every STORAGE_PERIOD_MS ms and
 * update lastStorageTime
 */
void storeMileage()
{
	if (millis() - lastStorageTime > STORAGE_PERIOD_MS)
	{
		saveToStorage();
		lastStorageTime = millis();
	}
}

/*
 * Load the data structure to EEPROM permanent storage
 */
void loadFromStorage()
{
	EEPROM.get(0, data);

	// On first load, set the EEPROM to default values
	if (data.appId != DEFAULT_DATA.appId)
	{
		data = DEFAULT_DATA;
		saveToStorage();
	}
}

/*
 * Save the data structure to EEPROM permanent storage
 */
void saveToStorage()
{
	EEPROM.put(0, data);
}

/*
 * Helper functions for converting doubles to string for publish() call
 * These are only here bc Wren API needs a "0."-prefixed double,
 * but Particle.publish() only gives strings
*/

/*
 * Reverses a string 'str' of length 'len'
 */
void reverse(char *str, int len)
{
	int i = 0, j = len - 1, temp;
	while (i < j)
	{
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++;
		j--;
	}
}

/*
 * Converts a given integer x to string str[]. 
 * d is the number of digits required in the output. 
 * If d is more than the number of digits in x, 
 * then 0s are added at the beginning.
 */
int intToStr(int x, char str[], int d)
{
	int i = 0;

	if (!x)
		str[i++] = '0';	// If the int part is 0, make it explicit for Wren API

	while (x)
	{
		str[i++] = (x % 10) + '0';
		x = x / 10;
	}

	// If number of digits required is more, then add 0s at the beginning
	while (i < d)
	{
		str[i++] = '0';
	}

	reverse(str, i);
	str[i] = '\0';
	return i;
}

/*
 * Converts a floating-point/double number to a string.
 */
void ftoa(float n, char *res, int afterpoint)
{
	// Extract integer part
	int ipart = (int) n;

	// Extract floating part
	float fpart = n - (float) ipart;

	// Convert integer part to string
	int i = intToStr(ipart, res, 0);

	// Check for display option after point
	if (afterpoint != 0)
	{
		res[i] = '.';	// add dot

		// Get the value of fraction part upto given no.
		// of points after dot. The third parameter 
		// is needed to handle cases like 233.007
		fpart = fpart * pow(10, afterpoint);

		intToStr((int) fpart, res + i + 1, afterpoint);
	}
}
