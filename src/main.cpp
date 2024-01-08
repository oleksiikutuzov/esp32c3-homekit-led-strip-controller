/*********************************************************************************
 *
 *  MIT License
 *
 *  Copyright (c) 2023 Oleksii Kutuzov
 *
 ********************************************************************************/

/*
 *                     ESP32-C3-MINI Utilized pins
 *                   ╔═════════════════════════════╗
 *        RST_BUTTON ║ EN                      3V3 ║ +++
 *                   ║                             ║
 *                   ║ IO0                     IO7 ║ MOSFET_PIN
 *                   ║ IO1                     IO8 ║ BOOT_CONFIG
 *       BOOT_CONFIG ║ IO2                     IO9 ║ BOOT_BUTTON
 *        BUTTON_PIN ║ IO3                    IO10 ║
 *      NEOPIXEL_PIN ║ IO4                    IO18 ║ DATA-
 *         EXTRA_LED ║ IO5                    IO19 ║ DATA+
 *        STATUS_LED ║ IO6                         ║
 *                   ║                             ║
 *               TXD ║ TXD                         ║
 *               RXD ║ RXD                     GND ║ +++
 *                   ╚═════════════════════════════╝
 */

#define REQUIRED VERSION(1, 7, 0) // Required HomeSpan version
#define FW_VERSION "1.1.0"

#include "HomeSpan.h"
#include <ElegantOTA.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "defines.h"
#include "DEV_Pixel_Strand.h"
#include "DEV_Switch.h"

WebServer server(80);
bool rgbw_bool = 0;

void setupWeb();

char sNumber[18] = "11:11:11:11:11:11";

unsigned long ota_progress_millis = 0;

void onOTAStart()
{
	// Log when OTA has started
	Serial.println("OTA update started!");
	// <Add your own code here>
}

void onOTAProgress(size_t current, size_t final)
{
	// Log every 1 second
	if (millis() - ota_progress_millis > 1000)
	{
		ota_progress_millis = millis();
		Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
	}
}

void onOTAEnd(bool success)
{
	// Log when OTA has finished
	if (success)
	{
		Serial.println("OTA update finished successfully!");
	}
	else
	{
		Serial.println("There was an error during OTA update!");
	}
	// <Add your own code here>
}

///////////////////////////////

DEV_Pixel_Strand *STRIP;

void setup()
{

	Serial.begin(115200);

	Serial.print("Active firmware version: ");
	Serial.println(FW_VERSION);

	String temp = FW_VERSION;
	const char compile_date[] = __DATE__ " " __TIME__;
	char *fw_ver = new char[temp.length() + 30];
	strcpy(fw_ver, temp.c_str());
	// strcat(fw_ver, mode.c_str());
	strcat(fw_ver, " (");
	strcat(fw_ver, compile_date);
	strcat(fw_ver, ")");

	for (int i = 0; i < 17; ++i) // we will iterate through each character in WiFi.macAddress()
								 // and copy it to the global char sNumber[]
	{
		sNumber[i] = WiFi.macAddress()[i];
	}
	sNumber[17] = '\0'; // the last charater needs to be a null

	homeSpan.setSketchVersion(FW_VERSION);							// set sketch version
	homeSpan.setLogLevel(0);										// set log level to 0 (no logs)
	homeSpan.setStatusPin(STATUS_LED);								// set the status pin to GPIO32
	homeSpan.setStatusAutoOff(10);									// disable led after 10 seconds
	homeSpan.setWifiCallback(setupWeb);								// Set the callback function for wifi events
	homeSpan.reserveSocketConnections(5);							// reserve 5 socket connections for Web Server
	homeSpan.setControlPin(BUTTON_PIN);								// set the control pin to GPIO0
	homeSpan.setPortNum(88);										// set the port number to 81
	homeSpan.enableAutoStartAP();									// enable auto start of AP
	homeSpan.enableWebLog(10, "pool.ntp.org", "UTC-2:00", "myLog"); // enable Web Log
	homeSpan.setSketchVersion(fw_ver);

	homeSpan.begin(Category::Lighting, "LED Strip Controller");

#ifndef SWITCH_ONLY
	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Name("Controller");
	new Characteristic::Manufacturer("HomeSpan");
	new Characteristic::SerialNumber(sNumber);
	new Characteristic::Model("NeoPixel LEDs");
	new Characteristic::FirmwareRevision(temp.c_str());
	new Characteristic::Identify();

	new Service::HAPProtocolInformation();
	new Characteristic::Version("1.1.0");

	STRIP = new DEV_Pixel_Strand(NEOPIXEL_PIN);
#endif

#ifdef SWITCH_ONLY
	LOG0("Adding Accessory: Switch\n");

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Name("Switch");
	new Characteristic::Identify();

	new DEV_Switch(MOSFET_PIN);
#endif
}

///////////////////////////////

void loop()
{
	homeSpan.poll();
	server.handleClient();
	ElegantOTA.loop();
}

///////////////////////////////

void setupWeb()
{
	STRIP->ip_address.setString(WiFi.localIP().toString().c_str());
	ElegantOTA.begin(&server); // Start ElegantOTA
	ElegantOTA.onStart(onOTAStart);
	ElegantOTA.onProgress(onOTAProgress);
	ElegantOTA.onEnd(onOTAEnd);
	server.begin();
	LOG1("HTTP server started");
} // setupWeb
