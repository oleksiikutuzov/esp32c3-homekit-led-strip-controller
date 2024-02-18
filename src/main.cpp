/*********************************************************************************
 *
 *  MIT License
 *
 *  Copyright (c) 2023 Oleksii Kutuzov
 *
 ********************************************************************************/

#define REQUIRED VERSION(1, 7, 0) // Required HomeSpan version
#define FW_VERSION "1.1.0"

#include "HomeSpan.h"
#include "extras/Pixel.h"
#include <ElegantOTA.h>
#include <WebServer.h>
#include <WiFiClient.h>

#if defined(CONFIG_IDF_TARGET_ESP32)

#define NEOPIXEL_PIN 16

#elif defined(CONFIG_IDF_TARGET_ESP32S2)

#define NEOPIXEL_PIN 38

#elif defined(CONFIG_IDF_TARGET_ESP32C3)

#define NEOPIXEL_PIN 4
#define MOSFET_PIN 7
#define BUTTON_PIN 3
#define STATUS_PIN 6
#define MAX_LEDS 150
#define DEFAULT_LEDS 90

#endif

// clang-format off
CUSTOM_CHAR(RainbowEnabled, 00000001-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
CUSTOM_CHAR(RGBWEnabled, 00000002-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
CUSTOM_CHAR_STRING(IPAddress, 00000003-0001-0001-0001-46637266EA00, PR + EV, "");
CUSTOM_CHAR(RainbowSpeed, 00000004-0001-0001-0001-46637266EA00, PR + PW + EV, UINT8, 1, 1, 10, false);
CUSTOM_CHAR(NumLeds, 00000005-0001-0001-0001-46637266EA00, PR + PW + EV, UINT8, DEFAULT_LEDS, 1, MAX_LEDS, false);
// clang-format on

WebServer server(80);
float angle = 0;

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

struct Pixel_Strand
	: Service::LightBulb
{ // Addressable RGBW Pixel Strand of nPixel Pixels

	struct SpecialEffect
	{
		Pixel_Strand *px;
		const char *name;

		SpecialEffect(Pixel_Strand *px, const char *name)
		{
			this->px = px;
			this->name = name;
			Serial.printf("Adding Effect %d: %s\n", px->Effects.size() + 1, name);
		}

		virtual void init() {}
		virtual uint32_t update() { return (60000); }
		virtual int requiredBuffer() { return (0); }
	};

	Characteristic::On power{0, true};
	Characteristic::Hue H{0, true};
	Characteristic::Saturation S{100, true};
	Characteristic::Brightness V{100, true};
	Characteristic::RainbowEnabled rainbow{false, true};
	Characteristic::RainbowSpeed rainbow_speed{1, true};
	Characteristic::RGBWEnabled rgbw{false, true};
	Characteristic::IPAddress ip_address{"0.0.0.0"};
	Characteristic::NumLeds num_leds{DEFAULT_LEDS, true};

	vector<SpecialEffect *> Effects;

	Pixel *pixel;
	int nPixels = num_leds.getVal();
	Pixel::Color *colors;
	uint32_t alarmTime;

	Pixel_Strand(int pin) : Service::LightBulb()
	{

		pixel = new Pixel(pin, rgbw.getVal()); // creates RGB/RGBW pixel LED on specified pin using default
											   // timing parameters suitable for most SK68xx LEDs

		Effects.push_back(new ManualControl(this));
		Effects.push_back(new Rainbow(this));

		rainbow.setUnit("");
		rainbow.setDescription("Rainbow Animation");

		rainbow_speed.setUnit("");
		rainbow_speed.setDescription("Rainbow Speed");
		rainbow_speed.setRange(1, 10, 1);

		rgbw.setUnit("");
		rgbw.setDescription("Enable RGBW Strip");

		ip_address.setDescription("IP Address");

		num_leds.setUnit(""); // configures custom "Selector" characteristic for use with Eve HomeKit
		num_leds.setDescription("Number of LEDs");
		num_leds.setRange(1, MAX_LEDS, 1);

		V.setRange(5, 100, 1); // sets the range of the Brightness to be from a min
							   // of 5%, to a max of 100%, in steps of 1%

		new SpanButton(BUTTON_PIN);

		int bufSize = 0;

		for (int i = 0; i < Effects.size(); i++)
			bufSize = Effects[i]->requiredBuffer() > bufSize
						  ? Effects[i]->requiredBuffer()
						  : bufSize;

		colors = (Pixel::Color *)calloc(
			bufSize, sizeof(Pixel::Color)); // storage for dynamic pixel pattern

		Serial.printf("\nConfigured Pixel_Strand on pin %d with %d pixels and %d "
					  "effects.  Color buffer = %d pixels\n\n",
					  pin, nPixels, Effects.size(), bufSize);

		update();
	}

	boolean update() override
	{

		if (!power.getNewVal())
		{
			pixel->set(Pixel::Color().RGB(0, 0, 0, 0), this->nPixels);
		}
		else if (!rainbow.getNewVal())
		{
			Effects[0]->init();
			if (rainbow.updated())
				Serial.printf("Effect changed to: %s\n", Effects[0]->name);
		}
		else
		{
			Effects[1]->init();
			alarmTime = millis() + Effects[1]->update();
			if (rainbow.updated())
				Serial.printf("Effect changed to: %s\n", Effects[1]->name);
		}
		return (true);
	}

	void loop() override
	{

		if (millis() > alarmTime && power.getVal())
			if (rainbow.getVal())
			{
				alarmTime = millis() + Effects[1]->update();
			}
	}

	void button(int pin, int pressType) override
	{

		LOG1("Found button press on pin: ");
		LOG1(pin);
		LOG1("  type: ");
		LOG1(pressType == SpanButton::LONG		 ? "LONG"
			 : (pressType == SpanButton::SINGLE) ? "SINGLE"
												 : "DOUBLE");
		LOG1("\n");

		int newLevel;

		if (pressType == SpanButton::SINGLE)
		{
			power.setVal(1 - power.getVal()); // ...toggle the value of the power Characteristic
			update();
		}
	}

	//////////////

	struct ManualControl : SpecialEffect
	{

		ManualControl(Pixel_Strand *px) : SpecialEffect{px, "Manual Control"} {}

		void init() override
		{

			float h = px->H.getNewVal();
			float s = px->S.getNewVal();
			float v = px->V.getNewVal();

			if (px->rgbw.getVal())
			{
				if (h == 30)
				{
					px->pixel->set(Pixel::Color().HSV(h, s, 0, v), px->nPixels);
				}
				else
				{
					px->pixel->set(Pixel::Color().HSV(h, s, v), px->nPixels);
				}
			}
			else
			{
				px->pixel->set(Pixel::Color().HSV(h, s, v), px->nPixels);
			}
		}
	};

	//////////////
	struct Rainbow : SpecialEffect
	{

		int8_t *dir;

		Rainbow(Pixel_Strand *px) : SpecialEffect{px, "Rainbow"}
		{
			dir = (int8_t *)calloc(px->nPixels, sizeof(int8_t));
		}

		void init() override
		{
			for (int i = 0; i < px->nPixels; i++)
			{
				px->colors[i].RGB(0, 0, 0, 0);
				dir[i] = 0;
			}
		}

		uint32_t update() override
		{
			float value = px->V.getNewVal<float>();
			for (int i = 0; i < px->nPixels; i++)
			{
				px->colors[i] = Pixel::Color().HSV(angle, 100, value);
			}
			px->pixel->set(px->colors, px->nPixels);
			angle = angle + 0.1;
			if (angle == 360)
				angle = 0;
			return (50 / px->rainbow_speed.getVal());
		}

		int requiredBuffer() override { return (px->nPixels); }
	};
};

struct DEV_Switch : Service::Switch
{
	int ledPin; // relay pin
	SpanCharacteristic *power;

	// Constructor
	DEV_Switch(int ledPin) : Service::Switch()
	{
		power = new Characteristic::On(0, true);
		this->ledPin = ledPin;
		pinMode(ledPin, OUTPUT);

		digitalWrite(ledPin, power->getVal());
	}

	// Override update method
	boolean update()
	{
		digitalWrite(ledPin, power->getNewVal());

		return (true);
	}
};

///////////////////////////////

Pixel_Strand *STRIP;

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

	homeSpan.setSketchVersion(FW_VERSION);							// set sketch version							// set the status pin to GPIO32
	homeSpan.setLogLevel(0);										// set log level to 0 (no logs)
	homeSpan.setStatusPin(STATUS_PIN);								// set the status pin to GPIO32
	homeSpan.setStatusAutoOff(10);									// disable led after 10 seconds
	homeSpan.setWifiCallback(setupWeb);								// Set the callback function for wifi events
	homeSpan.reserveSocketConnections(5);							// reserve 5 socket connections for Web Server
	homeSpan.setControlPin(BUTTON_PIN);								// set the control pin to GPIO0
	homeSpan.setPortNum(88);										// set the port number to 81
	homeSpan.enableAutoStartAP();									// enable auto start of AP
	homeSpan.enableWebLog(10, "pool.ntp.org", "UTC-2:00", "myLog"); // enable Web Log
	homeSpan.setSketchVersion(fw_ver);

	homeSpan.begin(Category::Lighting, "LED Strip Controller");

	new SpanAccessory(1);
	new Service::AccessoryInformation();
	new Characteristic::Name("Controller");
	new Characteristic::Manufacturer("HomeSpan");
	new Characteristic::SerialNumber(sNumber);
	new Characteristic::Model("NeoPixel LEDs");
	new Characteristic::FirmwareRevision(temp.c_str());
	new Characteristic::Identify();

	new Service::HAPProtocolInformation();
	new Characteristic::Version("1.1.0");

	STRIP = new Pixel_Strand(NEOPIXEL_PIN);
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
	LOG1("HTTP server started\n");

	if (homeSpan.updateDatabase())
		Serial.printf("Accessories Database updated.  New configuration number broadcasted...\n\n");
	else
		Serial.printf("Nothing to update - no changes were made!\n\n");
} // setupWeb
