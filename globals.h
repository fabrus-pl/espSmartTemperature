#ifndef GLOBALS_H
#define GLOBALS_H
#include "config.h"

#define AdminTimeOut 180                                   // Defines the Time in Seconds, when the Admin-Mode will be disabled
#define LED 14                                      // Device output pin definition, yep I know it's defined as LED, cause that's what I was using to test.
#define ONE_WIRE_BUS 2

ESP8266WebServer server(80);                        // Create web server instance
boolean firstStart = true;                          // On firststart = true, NTP will try to get a valid time
boolean AdminTimeOutCounter = 0;                    // Counter for disabling Admin mode
strDateTime DateTime;                               // Global DateTime structure, will be refreshed every Second
WiFiUDP UDPNTPClient;                               // NTP Client
unsigned long UnixTimestamp = 0;                    // GLOBALTIME  ( Will be set by NTP)
int cNTP_Update = 0;                                // Counter for Updating the time via NTP
Ticker tkSecond;                                    // Second - Timer for Updating Datetime Structure
Ticker tkMeasure;                                   // Ticker for triggering measures
boolean AdminEnabled = false;                        // Enable admin for a certain time
byte Minute_Old = 100;                              // Helpvariable for checking, when a new Minute comes up (for Auto Turn On / Off)
boolean BypassOn = false;                           // Toggle on/off via web interface
int count = 0;                                      // Push Button press time counter
const long connectionCheckInterval = 90000;         // MQTT connection checked every 90 seconds
long previousMillis = 0;
const int NTP_PACKET_SIZE = 48;                     // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];                // Buffer to hold incoming and outgoing packets
int cb;                                             // NTP packet check store
unsigned int localNtpPort = 2390;                   // Set the incomming UDP port to listen to
boolean devStat = false;                            // Timer status condition
WiFiClient wclient;                                 // Create WiFi client instance
PubSubClient client(wclient);            // Partially initialise PubSubClient we'll do the remainder in void setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
String mqttTemperatureTopic;
String mqttFeedbackTopic;
extern void ConnectMQTT(void);




bool ConfigureWifi()
{
	Serial.println("Configuring WiFi "); //Serial.print(config.WiFissid);
	WiFi.mode(WIFI_STA);
    delay (1000);
	WiFi.begin (config.WiFissid, config.WiFipass);
	if (!config.dhcp)
	{
		WiFi.config(IPAddress(config.IP[0],config.IP[1],config.IP[2],config.IP[3] ),  IPAddress(config.Gateway[0],config.Gateway[1],config.Gateway[2],config.Gateway[3] ) , IPAddress(config.Netmask[0],config.Netmask[1],config.Netmask[2],config.Netmask[3] ));
	}
	int retries = 10;
	while ((WiFi.status() != WL_CONNECTED) && retries--)
	{
		delay(5000);
		Serial.print(".");
	}
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("Unable to connect to WiFi network....!");
		WiFi.mode(WIFI_OFF);
        delay (500);
		return false;
	}
	else
	{
		Serial.println("");
		Serial.print("WiFi connected to IP Address: "); Serial.println(WiFi.localIP());
		return true;
	}
}
  



// Main internal clock called every second by Ticker.h

void Second_Tick()
{
  strDateTime tempDateTime;
  //AdminTimeOutCounter++;
  cNTP_Update++;
  UnixTimestamp++;
  ConvertUnixTimeStamp(UnixTimestamp +  (config.timezone *  360) , &tempDateTime);
  if (config.daylight) 
    if (summertime(tempDateTime.year,tempDateTime.month,tempDateTime.day,tempDateTime.hour,0))
      {
      ConvertUnixTimeStamp(UnixTimestamp +  (config.timezone *  360) + 3600, &DateTime);
      }
        else
        {
        DateTime = tempDateTime;
        }
    else
    {
      DateTime = tempDateTime;
    }
}

void initialize(void)
{
	Serial.begin(115200);
	delay(500);
	Serial.println("");
	Serial.println("Starting esp8266");
	Serial.println("");
	delay(10);
}

void handleAdminMode(void)
{
	if (AdminEnabled)
	{
		if (AdminTimeOutCounter > AdminTimeOut)
		{
			AdminEnabled = false;                                   // Start up WiFi and MQTT after Admin timeout expires.
			Serial.println("Admin Mode disabled!");
			Serial.println("Starting WiFi");
			ConfigureWifi();
			delay(2000);
			ConnectMQTT();
			delay(1000);
		}
	}

}

void sensorsMeasure(void)
{
	float temp =0;
	sensors.setResolution(12);
	sensors.requestTemperatures(); // Send the command to get temperatures
	temp = sensors.getTempCByIndex(0);
	if((temp > -20) && (temp <60))
	{
	client.publish(mqttTemperatureTopic.c_str(), String(temp).c_str(),TRUE);
	}
	else
	{
	Serial.print("Bad reading! :");
	Serial.println(temp);
	}
}

void handleRefresh(void)
{
		sensorsMeasure();
}

void startTimers(void)
{
	tkSecond.attach(1,Second_Tick);                 // Start internal timer
	tkMeasure.attach(60,handleRefresh);             // 60 seconds refresh timer
}

void startSensors(void)
{
	  sensors.begin();
}

#endif
