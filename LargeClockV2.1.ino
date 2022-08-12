/*

   Dual Timezone RGB LED Clock
   by Raina

   This uses code from Michael Marolis and Tom Igoe to pull the current time from an NTP server.

   the rest is all mine.  it basically just then works out the timezone of PST, and GMT, using Timezone.h and displays the current time on two 4 digit 7 segment displays.
   in my case those are oversized 3d prints taken from
   https://nerdiy.de/en/howto-elektronik-sieben-segment-ziffer-pxldigit-aus-ws2812-leds-bauen/

   GPL License applies etc etc etc
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
//#include <RTCZero.h>
#include <FlashStorage.h>
#include <Timezone.h>
#include <Adafruit_NeoPixel.h>
#include "RTClib.h"
//RTCZero rtc;
RTC_DS3231 rtc;
#define CLOCK_INTERRUPT_PIN 5
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(128, 138, 141, 172); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP timestamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];
WiFiUDP Udp;
#define LED_PIN    6
#define SecondRowLED_PIN    7
#define DateLED_PIN    3
#define DayLED_PIN    4
#define Date_Count  106
#define Day_Count  78
#define LED_COUNT 172
const int pResistor = A0;
int checktime = 0;
int Reset = 1;
int clearsettings = 3;
int lightvalue;
int pixels[172];
int pixelsLower[172];
int pixelsDate[106];
int pixelsDOW[78];
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripSecondRow(LED_COUNT, SecondRowLED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripDate(Date_Count, DateLED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripDay(Day_Count, DayLED_PIN, NEO_GRB + NEO_KHZ800);
//int status = WL_IDLE_STATUS;
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "RainbowClockSetup";        // your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key index number (needed only for WEP)
typedef struct {
  boolean valid;
  char wifissid[100];
  char Password[100];
  char brightness[5];
  char temp[3];
  char twelvehr[3];
  char timezone[10];
  char secondtimezone[10];
  char USDateFormat[3];
  char tempUnits[3];
  char dimmer[3];
} Credentials;
int numdigits(int i) {
  char str[20];

  sprintf(str, "%d", i);
  return (strlen(str));
}


int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);
String readString;
int WifiSetup = 1;
FlashStorage(my_flash_store, Credentials);
Credentials owner;
Credentials owner2;
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
TimeChangeRule usMDT = {"MDT", Second, Sun, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, Sun, Nov, 2, -420};
TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};
TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
TimeChangeRule usAKDT = {"AKDT", Second, Sun, Mar, 2, -480};
TimeChangeRule usAKST = {"AKST", First, Sun, Nov, 2, -540};
TimeChangeRule usHST = {"HST", Second, Sun, Mar, 2, -600};
TimeChangeRule AST = {"AST", First, Sun, Nov, 2, -180};
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};  // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};    // Central European Standard Time
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};     // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};      // Standard Time
Timezone UK(BST, GMT);
Timezone CE(CEST, CET);
Timezone usET(usEDT, usEST);
Timezone usPT(usPDT, usPST);
Timezone usCT(usCDT, usCST);
Timezone usMT(usMDT, usMST);
Timezone usAZ(usMST);
Timezone usHI(usHST);
Timezone usAK(usAKDT, usAKST);
Timezone AAST(AST);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  owner = my_flash_store.read();
  digitalWrite(Reset, HIGH);
  delay(200);
  pinMode(Reset, OUTPUT);
  //  pinMode(pResistor, INPUT);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.clear();
  stripDay.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  stripDay.show();            // Turn OFF all pixels ASAP
  stripDay.clear();
  stripSecondRow.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  stripSecondRow.show();            // Turn OFF all pixels ASAP
  stripSecondRow.clear();
  stripDate.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  stripDate.show();            // Turn OFF all pixels ASAP
  stripDate.clear();
  strip.setBrightness(150);
  stripSecondRow.setBrightness(150);
  stripDay.setBrightness(150);
  stripDate.setBrightness(150);
  delay(200);

  if (owner.valid == false) {
    status = WiFi.beginAP(ssid);
    if (status != WL_AP_LISTENING) {
      debugoutput(1, 0, 0, 0, 255, 75, 250);
      // WAP couldn't start.  Top row pinkish
      while (true);
    }
    delay(10000);
    server.begin();
    //
    //    Serial.println("Im here");
    //    Serial.println("--------------------------------------------");
    Serial.print("Stored SSID:");
    Serial.println((String)owner.wifissid);

    printWiFiStatus();
    Serial.println("Access Point Web Server");
    Serial.print("connection status: ");
    Serial.println(server.status());

    //  // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
      Serial.println("Communication with WiFi module failed!");
      debugoutput(1, 0, 0, 0, 255, 0, 0); // no wifi module top row red
      while (true);
    }
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
      Serial.println("Please upgrade the firmware");
    }
    Serial.print("Creating access point named: ");
    Serial.println(ssid);
    // WAP Started - top row yellow
    debugoutput(1, 0, 0, 0, 255, 255, 0);

  } else {
    // we have saved credentials, so we start up normally. middle row green
    debugoutput(0, 1, 0, 0, 0, 150, 0);
    Serial.print("Stored SSID:");
    Serial.println((String)owner.wifissid);
    Serial.println((String)owner.brightness);
    WifiSetup = 0;
    String returnedip = validateSSID(owner);
    if (returnedip == "0.0.0.0") {
      debugoutput(1, 1, 1, 0, 255, 0, 0); //no IP on bootup.  Wifi issue.  Top middle and bottom red
      Serial.println("Wifi Failed");
      status = WiFi.beginAP(ssid);
      printWiFiStatus();
      delay(5000);
    } else {
      // validateSSID(owner);
      delay(5000);
      server.begin();
      if (WiFi.status() == WL_NO_MODULE) {  // module failed.  middle row red
        debugoutput(0, 1, 0, 0, 255, 0, 0);
        Serial.println("Communication with WiFi module failed!");
        while (true)
          ;
      }
      String fv = WiFi.firmwareVersion();
      if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
      }
      while (status != WL_CONNECTED) {  // Wifi is disconnected, so start the connection process
        // lets turn the bottom row green too
        debugoutput(0, 0, 1, 0, 0, 150, 0);
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // wait 10 seconds for connection:
        delay(10000);
      }
      Serial.println("Connected to WiFi");
      strip.setPixelColor(42, strip.Color(0, 255, 0));
      strip.setPixelColor(43, strip.Color(0, 255, 0));
      strip.show();
      printWifiStatus();
      Serial.println("\nStarting connection to server (first run)...");
      Serial.println((String)owner.wifissid);
      Serial.println((String)owner.valid);
      Serial.println((String)owner.timezone);
      Serial.println((String)owner.USDateFormat);

      GetTime();
    }
  }
}
void SetTime() {
  /*
     this is what handles the actual updating of the time.  it's an Alarm interrupt on the RTC.
  */
  Serial.println("Alarm2 Triggered");
  DateTime now = rtc.now();
  int alarmepoch = now.unixtime();
  char bufPST[40];
  char mPST[4];
  char mEST[4];
  char mMST[4];
  char mCST[4];
  char mHST[4];
  char mCET[4];
  char mGMT[4];
  char mAKDT[4];
  char mAZDT[4];
  char mAST[4];

  TimeChangeRule *tcr;
  char PSTHour[3];
  char PSTMin[3];
  char PSTSec[3];
  char PSTDOW[3];
  char PSTYear[3];
  char PSTDay[3];
  char PSTMon[3];
  char OtherHour[3];
  char OtherMin[3];
  char OtherSec[3];

  if ((String)owner.timezone == "PST") {
    // Serial.println("PST Time Zone...");
    time_t tPST = usPT.toLocal(alarmepoch, &tcr);
    strcpy(mPST, monthShortStr(month(tPST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tPST), minute(tPST), second(tPST), dayShortStr(weekday(tPST)), day(tPST), mPST, year(tPST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tPST));
    sprintf(PSTMin, "%.2d", minute(tPST));
    sprintf(PSTSec, "%.2d", second(tPST));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tPST)));
    sprintf(PSTMon, "%d", month(tPST));
    sprintf(PSTYear, "%d", year(tPST));
    sprintf(PSTDay, "%u", day(tPST));

  }
  if ((String)owner.timezone == "EST") {
    Serial.println("EST Time Zone...");
    time_t tEST = usET.toLocal(alarmepoch, &tcr);
    strcpy(mEST, monthShortStr(month(tEST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tEST), minute(tEST), second(tEST), dayShortStr(weekday(tEST)), day(tEST), mEST, year(tEST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tEST));
    sprintf(PSTMin, "%.2d", minute(tEST));
    sprintf(PSTSec, "%.2d", second(tEST));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tEST)));
    sprintf(PSTMon, "%d", month(tEST));
    sprintf(PSTYear, "%d", year(tEST));
    sprintf(PSTDay, "%u", day(tEST));
  }
  if ((String)owner.timezone == "CST") {
    Serial.println("CST Time Zone...");
    time_t tCST = usCT.toLocal(alarmepoch, &tcr);
    strcpy(mCST, monthShortStr(month(tCST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tCST), minute(tCST), second(tCST), dayShortStr(weekday(tCST)), day(tCST), mCST, year(tCST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tCST));
    sprintf(PSTMin, "%.2d", minute(tCST));
    sprintf(PSTSec, "%.2d", second(tCST));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tCST)));
    sprintf(PSTMon, "%d", month(tCST));
    sprintf(PSTYear, "%d", year(tCST));
    sprintf(PSTDay, "%u", day(tCST));
  }
  if ((String)owner.timezone == "MST") {
    Serial.println("MST Time Zone...");
    time_t tMST = usMT.toLocal(alarmepoch, &tcr);
    strcpy(mMST, monthShortStr(month(tMST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tMST), minute(tMST), second(tMST), dayShortStr(weekday(tMST)), day(tMST), mMST, year(tMST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tMST));
    sprintf(PSTMin, "%.2d", minute(tMST));
    sprintf(PSTSec, "%.2d", second(tMST));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tMST)));
    sprintf(PSTMon, "%d", month(tMST));
    sprintf(PSTYear, "%d", year(tMST));
    sprintf(PSTDay, "%u", day(tMST));
  }
  if ((String)owner.timezone == "GMT") {
    Serial.println("GMT Time Zone...");
    time_t tGMT = UK.toLocal(alarmepoch, &tcr);
    strcpy(mGMT, monthShortStr(month(tGMT)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tGMT), minute(tGMT), second(tGMT), dayShortStr(weekday(tGMT)), day(tGMT), mGMT, year(tGMT), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tGMT));
    sprintf(PSTMin, "%.2d", minute(tGMT));
    sprintf(PSTSec, "%.2d", second(tGMT));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tGMT)));
    sprintf(PSTMon, "%d", month(tGMT));
    sprintf(PSTYear, "%d", year(tGMT));
    sprintf(PSTDay, "%u", day(tGMT));
  }
  if ((String)owner.timezone == "CET") {
    Serial.println("CET Time Zone...");
    time_t tCET = CE.toLocal(alarmepoch, &tcr);
    strcpy(mCET, monthShortStr(month(tCET)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tCET), minute(tCET), second(tCET), dayShortStr(weekday(tCET)), day(tCET), mCET, year(tCET), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tCET));
    sprintf(PSTMin, "%.2d", minute(tCET));
    sprintf(PSTSec, "%.2d", second(tCET));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tCET)));
    sprintf(PSTMon, "%d", month(tCET));
    sprintf(PSTYear, "%d", year(tCET));
    sprintf(PSTDay, "%u", day(tCET));
  }
  if ((String)owner.timezone == "AST") {
    Serial.println("AST Time Zone...");
    time_t tAST = AAST.toLocal(alarmepoch, &tcr);
    strcpy(mAST, monthShortStr(month(tAST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tAST), minute(tAST), second(tAST), dayShortStr(weekday(tAST)), day(tAST), mAST, year(tAST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tAST));
    sprintf(PSTMin, "%.2d", minute(tAST));
    sprintf(PSTSec, "%.2d", second(tAST));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tAST)));
    sprintf(PSTMon, "%d", month(tAST));
    sprintf(PSTYear, "%d", year(tAST));
    sprintf(PSTDay, "%u", day(tAST));
  }
  if ((String)owner.timezone == "ZST") {
    Serial.println("AZST Time Zone...");
    time_t tAZST = usAZ.toLocal(alarmepoch, &tcr);
    strcpy(mAZDT, monthShortStr(month(tAZST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tAZST), minute(tAZST), second(tAZST), dayShortStr(weekday(tAZST)), day(tAZST), mAZDT, year(tAZST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tAZST));
    sprintf(PSTMin, "%.2d", minute(tAZST));
    sprintf(PSTSec, "%.2d", second(tAZST));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tAZST)));
    sprintf(PSTMon, "%d", month(tAZST));
    sprintf(PSTYear, "%d", year(tAZST));
    sprintf(PSTDay, "%u", day(tAZST));
  }
  if ((String)owner.timezone == "KST") {
    Serial.println("AKST Time Zone...");
    time_t tAKDT = usAK.toLocal(alarmepoch, &tcr);
    strcpy(mAKDT, monthShortStr(month(tAKDT)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tAKDT), minute(tAKDT), second(tAKDT), dayShortStr(weekday(tAKDT)), day(tAKDT), mAKDT, year(tAKDT), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tAKDT));
    sprintf(PSTMin, "%.2d", minute(tAKDT));
    sprintf(PSTSec, "%.2d", second(tAKDT));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tAKDT)));
    sprintf(PSTMon, "%d", month(tAKDT));
    sprintf(PSTYear, "%d", year(tAKDT));
    sprintf(PSTDay, "%u", day(tAKDT));
  }
  if ((String)owner.timezone == "HST") {
    Serial.println("HST Time Zone...");
    time_t tHST = usHI.toLocal(alarmepoch, &tcr);
    strcpy(mHST, monthShortStr(month(tHST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tHST), minute(tHST), second(tHST), dayShortStr(weekday(tHST)), day(tHST), mHST, year(tHST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(PSTHour, "%.2d", hour(tHST));
    sprintf(PSTMin, "%.2d", minute(tHST));
    sprintf(PSTSec, "%.2d", second(tHST));
    sprintf(PSTDOW, "%s", dayShortStr(weekday(tHST)));
    sprintf(PSTMon, "%d", month(tHST));
    sprintf(PSTYear, "%d", year(tHST));
    sprintf(PSTDay, "%u", day(tHST));
  }

  if ((String)owner.secondtimezone == "PST") {
    Serial.println("PST Other Time Zone...");
    time_t tPST = usPT.toLocal(alarmepoch, &tcr);
    strcpy(mPST, monthShortStr(month(tPST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tPST), minute(tPST), second(tPST), dayShortStr(weekday(tPST)), day(tPST), mPST, year(tPST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tPST));
    sprintf(OtherMin, "%.2d", minute(tPST));
    sprintf(OtherSec, "%.2d", second(tPST));
  }
  if ((String)owner.secondtimezone == "EST") {
    Serial.println("EST  Other Time Zone...");
    time_t tEST = usET.toLocal(alarmepoch, &tcr);
    strcpy(mEST, monthShortStr(month(tEST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tEST), minute(tEST), second(tEST), dayShortStr(weekday(tEST)), day(tEST), mEST, year(tEST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tEST));
    sprintf(OtherMin, "%.2d", minute(tEST));
    sprintf(OtherSec, "%.2d", second(tEST));
  }
  if ((String)owner.secondtimezone == "CST") {
    Serial.println("CST Other Time Zone...");
    time_t tCST = usCT.toLocal(alarmepoch, &tcr);
    strcpy(mCST, monthShortStr(month(tCST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tCST), minute(tCST), second(tCST), dayShortStr(weekday(tCST)), day(tCST), mCST, year(tCST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tCST));
    sprintf(OtherMin, "%.2d", minute(tCST));
    sprintf(OtherSec, "%.2d", second(tCST));
  }
  if ((String)owner.secondtimezone == "MST") {
    Serial.println("MST Other Time Zone...");
    time_t tMST = usMT.toLocal(alarmepoch, &tcr);
    strcpy(mMST, monthShortStr(month(tMST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tMST), minute(tMST), second(tMST), dayShortStr(weekday(tMST)), day(tMST), mMST, year(tMST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tMST));
    sprintf(OtherMin, "%.2d", minute(tMST));
    sprintf(OtherSec, "%.2d", second(tMST));
  }
  if ((String)owner.secondtimezone == "GMT") {
    Serial.println("GMT Other Time Zone...");
    time_t tGMT = UK.toLocal(alarmepoch, &tcr);
    strcpy(mGMT, monthShortStr(month(tGMT)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tGMT), minute(tGMT), second(tGMT), dayShortStr(weekday(tGMT)), day(tGMT), mGMT, year(tGMT), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tGMT));
    sprintf(OtherMin, "%.2d", minute(tGMT));
    sprintf(OtherSec, "%.2d", second(tGMT));
  }
  if ((String)owner.secondtimezone == "CET") {
    Serial.println("CET Other Time Zone...");
    time_t tCET = CE.toLocal(alarmepoch, &tcr);
    strcpy(mCET, monthShortStr(month(tCET)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tCET), minute(tCET), second(tCET), dayShortStr(weekday(tCET)), day(tCET), mCET, year(tCET), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tCET));
    sprintf(OtherMin, "%.2d", minute(tCET));
    sprintf(OtherSec, "%.2d", second(tCET));
  }
  if ((String)owner.secondtimezone == "AST") {
    Serial.println("AST Other Time Zone...");
    time_t tAST = AAST.toLocal(alarmepoch, &tcr);
    strcpy(mAST, monthShortStr(month(tAST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tAST), minute(tAST), second(tAST), dayShortStr(weekday(tAST)), day(tAST), mAST, year(tAST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tAST));
    sprintf(OtherMin, "%.2d", minute(tAST));
    sprintf(OtherSec, "%.2d", second(tAST));
  }
  if ((String)owner.secondtimezone == "ZST") {
    Serial.println("AZST Other Time Zone...");
    time_t tAZST = usAZ.toLocal(alarmepoch, &tcr);
    strcpy(mAZDT, monthShortStr(month(tAZST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tAZST), minute(tAZST), second(tAZST), dayShortStr(weekday(tAZST)), day(tAZST), mAZDT, year(tAZST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tAZST));
    sprintf(OtherMin, "%.2d", minute(tAZST));
    sprintf(OtherSec, "%.2d", second(tAZST));
  }
  if ((String)owner.secondtimezone == "KST") {
    Serial.println("AKST Other Time Zone...");
    time_t tAKDT = usAK.toLocal(alarmepoch, &tcr);
    strcpy(mAKDT, monthShortStr(month(tAKDT)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tAKDT), minute(tAKDT), second(tAKDT), dayShortStr(weekday(tAKDT)), day(tAKDT), mAKDT, year(tAKDT), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tAKDT));
    sprintf(OtherMin, "%.2d", minute(tAKDT));
    sprintf(OtherSec, "%.2d", second(tAKDT));
  }
  if ((String)owner.secondtimezone == "HST") {
    Serial.println("HST Other Time Zone...");
    time_t tHST = usHI.toLocal(alarmepoch, &tcr);
    strcpy(mHST, monthShortStr(month(tHST)));
    sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tHST), minute(tHST), second(tHST), dayShortStr(weekday(tHST)), day(tHST), mHST, year(tHST), tcr->abbrev);
    Serial.println(bufPST);
    sprintf(OtherHour, "%.2d", hour(tHST));
    sprintf(OtherMin, "%.2d", minute(tHST));
    sprintf(OtherSec, "%.2d", second(tHST));
  }


  // There may be a better way to do all this, but I don't know what it is.  but since we're driving the digits in different colours, I figured we needed
  // to drive the digits entirely independently.  so in order to do this, I need to break the time down into separate digits.
  String fooM = (String)PSTMon;
  int barM = fooM.toInt();
  int wwwM = barM;
  if (wwwM < 10) {
    String foo2M = (String)wwwM;
    String prefixM = "0";
    String bar2M = prefixM + foo2M;
    bar2M.toCharArray(PSTMon, 3);
  } else {
    String bar2M = (String)wwwM;
    bar2M.toCharArray(PSTMon, 3);
  }

  String fooD = (String)PSTDay;
  int barD = fooD.toInt();
  int wwwD = barD;
  if (wwwD < 10) {
    String foo2D = (String)wwwD;
    String prefixD = "0";
    String bar2D = prefixD + foo2D;
    bar2D.toCharArray(PSTDay, 3);
  } else {
    String bar2D = (String)wwwD;
    bar2D.toCharArray(PSTDay, 3);
  }








  String PSTHours = String(PSTHour);
  if ((String)owner.twelvehr == "on") {
    //  Serial.println("12 hour Time!!!");
    String fooA = (String)PSTHour;
    String fooB = (String)OtherHour;
    int barA = fooA.toInt();
    int barB = fooB.toInt();
    if ((int)barA > 12) {
      //  Serial.println ("I shouldnt be here for 12 hr time");
      String foo = (String)PSTHour;
      int bar = foo.toInt();
      int www = bar - 12;
      if (www < 10) {
        String foo2 = (String)www;
        String prefix = "0";
        String bar2 = prefix + foo2;
        bar2.toCharArray(PSTHour, 3);
      } else {
        String bar2 = (String)www;
        bar2.toCharArray(PSTHour, 3);
      }
    } else {
      //  Serial.println ("I should be here for 24 hr time");
      String foo = (String)PSTHour;
      int bar = foo.toInt();
      int www = bar;
      if (www < 10) {
        String foo2 = (String)www;
        String prefix = "0";
        String bar2 = prefix + foo2;
        bar2.toCharArray(PSTHour, 3);
      } else {
        String bar2 = (String)www;
        bar2.toCharArray(PSTHour, 3);
      }
    }
    if ((int)barB > 12) {
      //  Serial.println ("I shouldnt be here for 12 hr time");
      String fooX = (String)OtherHour;
      int barX = fooX.toInt();
      int wwwX = barX - 12;
      if (wwwX < 10) {
        String foo2X = (String)wwwX;
        String prefixX = "0";
        String bar2X = prefixX + foo2X;
        bar2X.toCharArray(OtherHour, 3);
      } else {
        String bar2X = (String)wwwX;
        bar2X.toCharArray(OtherHour, 3);
      }
    } else {
      //  Serial.println ("I should be here for 24 hr time");
      String fooX = (String)OtherHour;
      int barX = fooX.toInt();
      int wwwX = barX;
      if (wwwX < 10) {
        String foo2X = (String)wwwX;
        String prefixX = "0";
        String bar2X = prefixX + foo2X;
        bar2X.toCharArray(OtherHour, 3);
      } else {
        String bar2X = (String)wwwX;
        bar2X.toCharArray(OtherHour, 3);
      }
    }


  }
  //  Serial.println((String)PSTHour);
  String PSTMins = String(PSTMin);
  String PSTSeconds = String(PSTSec);
  String PSTMinute1 = String(PSTMin).substring(0, 1);
  String PSTMinute2 = String(PSTMin).substring(1);
  String PSTHour1 = String(PSTHour).substring(0, 1);
  String PSTHour2 = String(PSTHour).substring(1);

  String PSTDOW1 = String(PSTDOW).substring(0, 1);
  String PSTDOW2 = String(PSTDOW).substring(1, 2);
  String PSTDOW3 = String(PSTDOW).substring(2);
  PSTDOW3.toLowerCase();
  PSTDOW2.toLowerCase();
  PSTDOW1.toLowerCase();
  String PSTDay1 = String(PSTDay).substring(0, 1);
  String PSTDay2 = String(PSTDay).substring(1);
  String PSTMon1 = String(PSTMon).substring(0, 1);
  String PSTMon2 = String(PSTMon).substring(1);


  String OtherMins = String(OtherMin);
  String OtherSeconds = String(OtherSec);
  String OtherMinute1 = String(OtherMin).substring(0, 1);
  String OtherMinute2 = String(OtherMin).substring(1);
  String OtherHour1 = String(OtherHour).substring(0, 1);
  String OtherHour2 = String(OtherHour).substring(1);
  //
  //
  //  Serial.println((String)PSTHour1);
  //  Serial.println((String)PSTHour2);
  //  Serial.println((String)OtherHour1);
  //  Serial.println((String)OtherHour2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  // now that the time's been split up, we can output it all to the digits of the clock.
  // I'm doing each digit separately.  again as with before, there's probably way better ways of doing this.

  if ((PSTHour1 == "1") && (PSTHour2 == "2") && (PSTMinute1 == "0") && (PSTMinute2 == "0")) {
    // it's midnight, so re-run the time sync
    checktime = 1;
  }

  outputDigits(PSTHour1.toInt(), PSTHour2.toInt(), PSTMinute1.toInt(), PSTMinute2.toInt());
  outputDigitsRow2 (OtherHour1.toInt(), OtherHour2.toInt(), OtherMinute1.toInt(), OtherMinute2.toInt());

  if ((String)owner.USDateFormat == "of") {
    outputDigitsDate (PSTDay1.toInt(), PSTDay2.toInt(), PSTMon1.toInt(), PSTMon2.toInt());
  } else {
    outputDigitsDate ( PSTMon1.toInt(), PSTMon2.toInt(), PSTDay1.toInt(), PSTDay2.toInt());
  }
  char *c = &PSTDOW1[0];
  char *d = &PSTDOW2[0];
  char *e = &PSTDOW3[0];
  outputDigitsDOW (c[0], d[0], e[0]);
}

void alarm() {
  if (rtc.alarmFired(1)) {
    Serial.println("Alarm1 Triggered");
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    if ((String)owner.temp == "on") {
      String TempUnits = "C";
      int Temperature = 45;
      if ((String)owner.tempUnits == "F") {
        TempUnits = "F";
      } else {
        TempUnits = "C";
      }
      outputDigitsTemp(rtc.getTemperature(), TempUnits);
    }
  } else {
    Serial.println("Alarm1 Triggered2");
    Serial.println((String)owner.timezone);
    Serial.println((String)owner.secondtimezone);
    SetTime();
  }
}

void outputDigitsDOW(char letter1, char letter2, char letter3) {
  int nums[][26] = {
    // 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25
    {  0,  1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, -1, -1, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // m //0
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // o //1
    {  0,  1, -1, -1, -1, -1,  6,  7, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, 20, 21, 22, 23, -1, -1  }, // n //2
    {  0,  1,  2,  3,  4,  5, -1, -1,  8,  9, 10, 11, 12, 13, -1, -1, 16, 17, 18, 19, 20, 21, -1, -1, 24, 25  }, // t //3
    { -1, -1, -1, -1, -1, -1,  6,  7, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // u //4
    { -1, -1,  2,  3,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // e //5
    { -1, -1, -1, -1, -1, -1,  6,  7, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, -1, -1, 24, 25  }, // w //6
    {  0, -1, -1, -1, -1, -1, -1,  7,  8,  9, 10, 11, 12, 13, -1, -1, 16, 17, 18, 19, 20, 22, -1, -1, 24, 25  }, // d //7
    {  0,  1, -1, -1, -1, -1,  6,  6, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // h //8
    {  0,  1,  2,  3, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, -1, -1  }, // r //9
    {  0,  1,  2,  3,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // f //10
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, -1, -1, 16, 17, 18, 19, 20, 21, -1, -1, 24, 25  }, // i //11
    { -1, -1, -1, -1,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  }, // s //12
    {  0,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // a //13
  };
  switch (letter1) {
      break;
    case 'm':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[0][j - 0] ;
      }
      break;
    case 'o':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[1][j - 0] ;
      }
      break;
    case 'n':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[2][j - 0] ;
      }
      break;
    case 't':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[3][j - 0] ;
      }
      break;
    case 'u':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[4][j - 0] ;
      }
      break;
    case 'e':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[5][j - 0];
      }
      break;
    case 'w':
      for (int j = 0; j < 26; j++) {  //
        pixelsDOW[j] = nums[6][j - 0];
      }
      break;
    case 'd':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[7][j - 0] ;
      }
      break;
    case 'h':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[8][j - 0] ;
      }
      break;
    case 'r':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[9][j - 0] ;
      }
      break;
    case 'f':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[10][j - 0] ;
      }
      break;
    case 'i':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[11][j - 0] ;
      }
      break;
    case 's':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[12][j - 0] ;
      }
      break;
    case 'a':
      for (int j = 0; j < 26; j++) {
        pixelsDOW[j] = nums[13][j - 0] ;
      }
      break;
    default:
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = j;
      }
      break;
  }
  switch (letter2) {
      break;
    case 'm':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[0][j - 26] ;
      }
      break;
    case 'o':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[1][j - 26] ;
      }
      break;
    case 'n':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[2][j - 26] ;
      }
      break;
    case 't':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[3][j - 26] ;
      }
      break;
    case 'u':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[4][j - 26] ;
      }
      break;
    case 'e':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[5][j - 26];
      }
      break;
    case 'w':
      for (int j = 26; j < 52; j++) {  //
        pixelsDOW[j] = nums[6][j - 26];
      }
      break;
    case 'd':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[7][j - 26] ;
      }
      break;
    case 'h':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[8][j - 26] ;
      }
      break;
    case 'r':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[9][j - 26] ;
      }
      break;
    case 'f':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[10][j - 26] ;
      }
      break;
    case 'i':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[11][j - 26] ;
      }
      break;
    case 's':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[12][j - 26] ;
      }
      break;
    case 'a':
      for (int j = 26; j < 52; j++) {
        pixelsDOW[j] = nums[13][j - 26] ;
      }
      break;
    default:
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = j - 26;
      }

      break;
  }
  switch (letter3) {
      break;
    case 'm':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[0][j - 52] ;
      }
      break;
    case 'o':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[1][j - 52] ;
      }
      break;
    case 'n':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[2][j - 52] ;
      }
      break;
    case 't':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[3][j - 52] ;
      }
      break;
    case 'u':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[4][j - 52] ;
      }
      break;
    case 'e':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[5][j - 52];
      }
      break;
    case 'w':
      for (int j = 52; j < 78; j++) {  //
        pixelsDOW[j] = nums[6][j - 52];
      }
      break;
    case 'd':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[7][j - 52] ;
      }
      break;
    case 'h':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[8][j - 52] ;
      }
      break;
    case 'r':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[9][j - 52] ;
      }
      break;
    case 'f':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[10][j - 52] ;
      }
      break;
    case 'i':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[11][j - 52] ;
      }
      break;
    case 's':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[12][j - 52] ;
      }
      break;
    case 'a':
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = nums[13][j - 52] ;
      }
      break;
    default:
      for (int j = 52; j < 78; j++) {
        pixelsDOW[j] = j - 52;
      }
      break;
  }
}

void outputDigitsDate(int digit1, int digit2, int digit3, int digit4) {
  //this will be for the date specifically.
  int nums[][26] = {
    // 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // 0 //seems ok
    {  0,  1, -1, -1, -1, -1,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },        // 1 //seems ok
    { -1, -1,  2,  3, -1, -1, -1, -1,  8,  9, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // 2 //seems ok
    { -1, -1, -1, -1, -1, -1, -1, -1,  8,  9, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },   // 3
    {  0,  1, -1, -1, -1, -1,  6,  7, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },      // 4 //ok
    { -1, -1, -1, -1,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },   // 5////numbers represent the pixels that are NOT LIT
    { -1, -1, -1, -1,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  },   // 6
    {  0,  1, -1, -1, -1, -1, -1, -1,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },      // 7 //seems ok
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // 8 //seems ok
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  }, // 9 //seems ok
    { -1, -1,  2,  3,  4,  5, -1, -1, -1, -1, 10, 11, -1, -1, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },     // C (for Temp)
    {  0,  1,  2,  3,  4,  5, -1, -1, -1, -1, -1, -1, -1, -1, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },        // F (for Temp)
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },            // all off
  };

  for (int j = 0; j < 26; j++) { //first date digit.  month1
    pixelsDate[j] = nums[digit1][j];
  }

  for (int j = 26; j < 52; j++) {  //second date digit.  month2
    if (nums[digit2][j - 26] == -1) {
      pixelsDate[j] = -1;
    } else {
      pixelsDate[j] = nums[digit2][j - 26] + 26;
    }
  }
  pixelsDate[52] = -1;  // Date Separator1
  //    pixelsDate[53] = -1;  // Date Separator2

  for (int j = 53; j < 79; j++) {  //third date digit.  day1
    if (nums[digit3][j - 53] == -1) {
      pixelsDate[j] = -1;
    } else {
      pixelsDate[j] = nums[digit3][j - 53] + 53;
    }
  }
  for (int j = 79; j < 105; j++) {  //Fourth date digit.  day2
    if (nums[digit4][j - 79] == -1) {
      pixelsDate[j] = -1;
    } else {
      pixelsDate[j] = nums[digit4][j - 79] + 79;
    }
  }
}
void outputDigitsRow2 (int digit1, int digit2, int digit3, int digit4) {
  int numsUK[][21] = {
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 16, 17, -1, -1, -1}, //0
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, -1, -1, -1, 15, 16, 17, -1, -1, -1},//1
    { -1, -1, -1,  3,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, -1, -1, -1, -1, -1, -1}, //2
    { -1, -1, -1,  3,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //3
    {  0,  1,  2, -1, -1, -1,  6,  7,  8,  9, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //4
    { -1, -1, -1, -1, -1, -1,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, 18, 19, 20}, //5
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 18, 19, 20}, //6 ////numbers represent the pixels that are NOT LIT
    { -1, -1, -1,  3,  4,  5,  6,  7,  8,  9,  10, 11, -1, -1, -1, 15, 16, 17, -1, -1, -1}, //7
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //8
    { -1, -1, -1, -1, -1, -1,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //9

  };

  for ( int j = 0; j < 21; j++ ) { //first digit.  a 3
    pixelsLower[j] = numsUK[digit1][j];
  }

  for ( int j = 21; j < 42; j++ ) { //second digit... a 3
    if (numsUK[digit2][j - 21] == -1) {
      pixelsLower[j] = -1;
    } else {
      pixelsLower[j] = numsUK[digit2][j - 21] + 21;
    }
  }
  pixelsLower[42] = -1; //dots
  pixelsLower[43] = -1;// dots

  for ( int j = 44; j < 65; j++ ) { //third digit.. a 7
    if (numsUK[digit3][j - 44] == -1) {
      pixelsLower[j] = -1;
    } else {

      pixelsLower[j] = numsUK[digit3][j - 44] + 44;
    }
  }
  for ( int j = 65; j < 86; j++ ) { //fourth digit... a 1
    if (numsUK[digit4][j - 65] == -1) {
      pixelsLower[j] = -1;
    } else {
      pixelsLower[j] = numsUK[digit4][j - 65] + 65;
    }
  }
}
void outputDigits (int digit1, int digit2, int digit3, int digit4) {


  int nums[][21] = {
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 16, 17, -1, -1, -1}, //0
    {  0,  1,  2, -1, -1, -1, -1, -1, -1,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20},//1
    { -1, -1, -1,  3,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, -1, -1, -1, -1, -1, -1}, //2
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, -1, -1, -1, 18, 19, 20}, //3
    {  0,  1,  2, -1, -1, -1, -1, -1, -1,  9, 10, 11, -1, -1, -1, -1, -1, -1, 18, 19, 20},//4
    { -1, -1, -1, -1, -1, -1,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, 18, 19, 20}, //5
    { -1, -1, -1, -1, -1, -1,  6, -7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //6
    {  0,  1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20}, //7
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //8
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 18, 19, 20}, //9

  };

  //this means I need a 1, 7, 3 and 3
  for ( int j = 0; j < 21; j++ ) { //first digit.  a 3
    pixels[j] = nums[digit1][j];
  }

  for ( int j = 21; j < 42; j++ ) { //second digit... a 3
    if (nums[digit2][j - 21] == -1) {
      pixels[j] = -1;
    } else {
      pixels[j] = nums[digit2][j - 21] + 21;
    }
  }
  pixels[42] = -1; //dots
  pixels[43] = -1;// dots

  for ( int j = 44; j < 65; j++ ) { //third digit.. a 7
    if (nums[digit3][j - 44] == -1) {
      pixels[j] = -1;
    } else {

      pixels[j] = nums[digit3][j - 44] + 44;
    }
  }
  for ( int j = 65; j < 86; j++ ) { //fourth digit... a 1
    if (nums[digit4][j - 65] == -1) {
      pixels[j] = -1;
    } else {
      pixels[j] = nums[digit4][j - 65] + 65;
    }
  }
  //
  //  for ( int j = 86; j < 107; j++ ) { //fifth digit... a 3
  //    if (numsUK[digit5][j - 86] == -1) {
  //      pixels[j] = -1;
  //    } else {
  //
  //      pixels[j] = nums[digit5][j - 86] + 86;
  //    }
  //  }
  //  for ( int j = 107; j < 128; j++ ) { //sixth digit.. a 3
  //    if (numsUK[digit6][j - 107] == -1) {
  //      pixels[j] = -1;
  //    } else {
  //
  //      pixels[j] = nums[digit6][j - 107] + 107;
  //    }
  //  }
  //  pixels[128] = -1; //dots
  //  pixels[129] = -1; //dots
  //
  //  for ( int j = 130; j < 151; j++ ) { //sevenths digit... a 4
  //    if (numsUK[digit7][j - 130] == -1) {
  //      pixels[j] = -1;
  //    } else {
  //
  //      pixels[j] = nums[digit7][j - 130] + 130;
  //    }
  //  }
  //  for ( int j = 151; j < 172; j++ ) { //eighth digit.. a 0
  //    if (numsUK[digit8][j - 151] == -1) {
  //      pixels[j] = -1;
  //    } else {
  //      pixels[j] = nums[digit8][j - 151] + 151;
  //    }
  //  }


}

void outputDigitsTemp(int Temperature, String units) {
  String temp1;
  String temp2;
  String temp3;
  int digit2;
  int digit3;
  int digit1;
  int Temp3;
  String TempUnits;
  //first thing we should do is turn off the dash, and we also need to turn off the day display, so the row doesn't get cluttered

  stripDate.clear();
  stripDay.clear();


  bool ShowThirdDigit = false;
  if (units == "F") {
    Serial.println("CaveMan Temperature");
    Serial.println(numdigits(Temperature));
    Serial.println(Temperature);
    int Temperature2 = (Temperature * 1.8) + 32;
    temp1 = String(Temperature2).substring(0, 1);
    temp2 = String(Temperature2).substring(1, 2);
    if (numdigits(Temperature2) > 2) {
      ShowThirdDigit = true;
      temp3 = String(Temperature2).substring(2, 3);
      digit3 = temp3.toInt();
    } else {
    }
    digit1 = temp1.toInt();
    digit2 = temp2.toInt();
    Serial.println((String)digit1);
    Serial.println((String)digit2);
    TempUnits = "F";
  } else {
    temp1 = String(Temperature).substring(0, 1);
    temp2 = String(Temperature).substring(1, 2);
    if (numdigits(Temperature) > 2) {
      ShowThirdDigit = true;
      temp3 = String(Temperature).substring(2, 3);
      digit3 = temp3.toInt();
    } else {
    }
    digit1 = temp1.toInt();
    digit2 = temp2.toInt();
    TempUnits = "C";
  }
  int nums[][26] = {
    // 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // 0 //seems ok
    {  0,  1, -1, -1, -1, -1,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },        // 1 //seems ok
    { -1, -1,  2,  3, -1, -1, -1, -1,  8,  9, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // 2 //seems ok
    { -1, -1, -1, -1, -1, -1, -1, -1,  8,  9, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },   // 3
    {  0,  1, -1, -1, -1, -1,  6,  7, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },      // 4 //ok
    { -1, -1, -1, -1,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },   // 5////numbers represent the pixels that are NOT LIT
    { -1, -1, -1, -1,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  },   // 6
    {  0,  1, -1, -1, -1, -1, -1, -1,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },      // 7 //seems ok
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  }, // 8 //seems ok
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  }, // 9 //seems ok
    { -1, -1,  2,  3,  4,  5, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  },     // C (for Temp)
    {  0,  1,  2,  3,  4,  5, -1, -1, -1, -1, -1, -1, 12, 13, 14, 15, 16, 17, -1, -1, 20, 21, 22, 23, 24, 25  },        // F (for Temp)
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },            // all off
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25  },            // negative
  };

  if ((String)temp1 == "-") {
    for (int j = 0; j < 26; j++) { //first date digit.  month1
      pixelsDate[j] = nums[13][j];
    }
  } else {
    for (int j = 0; j < 26; j++) { //first date digit.  month1
      pixelsDate[j] = nums[digit1][j];
    }
  }
  for (int j = 26; j < 52; j++) {  // third digit.. a 7
    if (nums[digit2][j - 26] == -1) {
      pixelsDate[j] = -1;
    } else {
      pixelsDate[j] = nums[digit2][j - 26] + 26;
    }
  }
  pixelsDate[52] = 52;
  if (ShowThirdDigit) {
    for (int j = 53; j < 79; j++) {  //third date digit.  day1
      if (nums[digit3][j - 53] == -1) {
        pixelsDate[j] = -1;
      } else {
        pixelsDate[j] = nums[digit3][j - 53] + 53;
      }
    }
  } else {
    for (int j = 53; j < 79; j++) {  // second digit... a 3
      if (nums[12][j - 53] == -1) {
        pixelsDate[j] = -1;
      } else {
        pixelsDate[j] = nums[12][j - 53] + 53;
      }
    }
  }
  for (int j = 0; j < 78; j++) {
    pixelsDOW[j] = 23;
  }



  if (units == "F") {
    for (int j = 79; j < 105; j++) {  // first digit.  a 3
      if (nums[11][j - 79] == -1) {
        pixelsDate[j] = -1;
      } else {
        pixelsDate[j] = nums[11][j - 79] + 79;
      }
    }
  } else {
    for (int j = 0; j < 14; j++) {  // first digit.  a 3
      for (int j = 79; j < 105; j++) {  // first digit.  a 3
        if (nums[10][j - 79] == -1) {
          pixelsDate[j] = -1;
        } else {
          pixelsDate[j] = nums[10][j - 79] + 79;
        }
      }
    }
  }
}



uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    // return stripSecondRow.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
}
uint32_t WheelLower(byte WheelPos) {
  if (WheelPos < 85) {
    return stripSecondRow.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    // return stripSecondRow.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return stripSecondRow.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
}

uint32_t WheelDate(byte WheelPos) {
  if (WheelPos < 85) {
    return stripDate.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    // return stripSecondRow.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return stripDate.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return stripDate.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
}

uint32_t WheelDOW(byte WheelPos) {
  if (WheelPos < 85) {
    return stripDay.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    // return stripSecondRow.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return stripDay.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return stripDay.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    //  return stripSecondRow.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
}

void loop() {
  if (WifiSetup == 0) {
    // Serial.println("WifiSetup is 0");
    if (owner.valid == false) {
      // Serial.println("For Some reason Wifi Setup is complete, but owner.valid is false");
    } else {
      if (server.status()) {
      } else {
        //        Serial.println("I Should be trying to start the server");
        server.begin();
      }

    }
    //   lightvalue = analogRead(pResistor);
    uint16_t i, j;
    for (j = 0; j < 256; j++) {
      for (i = 0; i < strip.numPixels(); i++) {
        if (pixels[i] == -1) {
          strip.setPixelColor(i, Wheel((i * 21 + j) & 255));
          // stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));

        } else {
          strip.setPixelColor(i, strip.Color(0, 0,  0));
          //  stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));
        }
      }
      for (i = 0; i < stripSecondRow.numPixels(); i++) {
        if (pixelsLower[i] == -1) {
          stripSecondRow.setPixelColor(i, WheelLower((i * 21 + j) & 255));
          // stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));

        } else {
          stripSecondRow.setPixelColor(i, strip.Color(0, 0,  0));
          //  stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));
        }
      }

      for (i = 0; i < stripDay.numPixels(); i++) {
        if (pixelsDOW[i] == -1) {
          stripDay.setPixelColor(i, WheelDOW((i * 21 + j) & 255));
          // stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));

        } else {
          stripDay.setPixelColor(i, strip.Color(0, 0,  0));
          //  stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));
        }
      }

      for (i = 0; i < stripDate.numPixels(); i++) {
        if (pixelsDate[i] == -1) {
          stripDate.setPixelColor(i, WheelDate((i * 21 + j) & 255));
          // stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));

        } else {
          stripDate.setPixelColor(i, strip.Color(0, 0,  0));
          //  stripSecondRow.setPixelColor(i, Wheel((i * 21 + j) & 255));
        }
      }

      strip.show();
      stripSecondRow.show();
      stripDay.show();
      stripDate.show();
      delay(15);
    }
    if (checktime == 1) {
      GetTime();
    }
    if ((String)owner.dimmer == "of") {
      String BrightnessLevel = (String)owner.brightness;
      BrightnessLevel = (String)owner.brightness;
      Serial.println("Setting Brightness to");
      Serial.println((String)BrightnessLevel.toInt());
      strip.setBrightness(BrightnessLevel.toInt());
      strip.show();
    } else {
      //readings in this section are with the photoresistor facing the couch, so light exposure is limited.
      // ok so with new photoresistor, value at 5pm is approx 490
      //all lights off at 9pm 28-40
      //7:30 dining room light only, curtains closed on 170-180
      //8:15am summer curtains open, no lights on >400
      lightvalue = analogRead(pResistor);
      Serial.println(lightvalue);
      if (lightvalue > 450) {
        strip.setBrightness(240);
        strip.show();
        stripSecondRow.setBrightness(240);
        stripSecondRow.show();
        stripDay.setBrightness(125);
        stripDate.setBrightness(125);
        stripDay.show();
        stripDate.show();
      }
      if ((lightvalue > 150) && (lightvalue < 450)) {
        strip.setBrightness(175);
        strip.show();
        stripSecondRow.setBrightness(175);
        stripSecondRow.show();
        stripDay.setBrightness(75);
        stripDate.setBrightness(75);
        stripDay.show();
        stripDate.show();
      }
      if ((lightvalue > 50) && (lightvalue < 150)) {
        strip.setBrightness(75);
        strip.show();
        stripSecondRow.setBrightness(75);
        stripSecondRow.show();
        stripDay.setBrightness(20);
        stripDate.setBrightness(20);
        stripDay.show();
        stripDate.show();
      }
      if (lightvalue < 50) {
        strip.setBrightness(10);
        strip.show();
        stripSecondRow.setBrightness(10);
        stripDay.setBrightness(2);
        stripDate.setBrightness(2);
        stripDay.show();
        stripDate.show();
        stripSecondRow.show();
      }
    }
  }
  WiFiClient client = server.available();
  if (client) {
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        readString += c;

        if (c == '\n' && currentLineIsBlank) {
          String twelvehour = "";
          if ((String)owner.twelvehr == "on") {
            twelvehour = " selected='selected' ";
          }
          String ShowTempUnitsSetting = "";
          if ((String)owner.tempUnits == "F") {
            ShowTempUnitsSetting = " selected ";
          }

          String ShowTempSetting = "";
          if ((String)owner.temp == "on") {
            ShowTempSetting = " selected ";
          }
          String DimmerSetting = "";
          if ((String)owner.dimmer == "of") {
            DimmerSetting = " selected ";
          }
          String dateformat = "";
          if ((String)owner.USDateFormat == "of") {
            dateformat = " selected ";
          }
          Serial.println(DimmerSetting);



          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<HTML>");
          client.println("<HEAD>");
          client.println("<TITLE>Rainbow Clock Setup</TITLE>");
          client.println("<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate' />");
          client.println("<meta http-equiv='Pragma' content='no-cache' />");
          client.println("<meta http-equiv='Expires' content='0' />");
          client.println("</HEAD>");
          client.println("<BODY>");
          client.println("<style type='text/css'>");
          client.println("*,");
          client.println("*::before,");
          client.println("*::after {");
          client.println("box-sizing: border-box;");
          client.println("}");
          client.println(":root {");
          client.println("--select-border: #777;");
          client.println("--select-focus: blue;");
          client.println("--select-arrow: var(--select-border);");
          client.println("}");
          client.print(".loader_website {");
          client.print("position: fixed;");
          client.print("top: 0;");
          client.print("left: 0px;");
          client.print("z-index: 1100;");
          client.print("width: 100%;");
          client.print("height: 100%;");
          client.print("background-color: rgba(0, 0, 0, 0.5);");
          client.print("display: block;");
          client.print("-webkit-transition: ease-in-out 0.1s;");
          client.print("-moz-transition: ease-in-out 0.1s;");
          client.print("-o-transition: ease-in-out 0.1s;");
          client.print("-ms-transition: ease-in-out 0.1s;");
          client.print("transition: ease-in-out 0.1s;");
          client.print("-webkit-box-sizing: border-box;");
          client.print("-moz-box-sizing: border-box;");
          client.print("-o-box-sizing: border-box;");
          client.print("-ms-box-sizing: border-box;");
          client.print("box-sizing: border-box;");
          client.print("}");
          client.print(".loader_website * {");
          client.print("-webkit-box-sizing: border-box;");
          client.print("-moz-box-sizing: border-box;");
          client.print("-o-box-sizing: border-box;");
          client.print("-ms-box-sizing: border-box;");
          client.print("box-sizing: border-box;");
          client.print("}");
          client.print("body.loader .loader_website span {");
          client.print("top: 18%;");
          client.print("}");
          client.print(".loader_website > span {");
          client.print("display: block;");
          client.print("width: 48px;");
          client.print("height: 48px;");
          client.print("padding: 4px;");
          client.print("background-color: #ffffff;");
          client.print("-webkit-border-radius: 100%;");
          client.print("-moz-border-radius: 100%;");
          client.print("-o-border-radius: 100%;");
          client.print("-ms-border-radius: 100%;");
          client.print("border-radius: 100%;");
          client.print("position: absolute;");
          client.print("left: 50%;");
          client.print("margin-left: -24px;");
          client.print("top: -50px;");
          client.print("-webkit-transition: ease-in-out 0.1s;");
          client.print("-moz-transition: ease-in-out 0.1s;");
          client.print("-o-transition: ease-in-out 0.1s;");
          client.print("-ms-transition: ease-in-out 0.1s;");
          client.print("transition: ease-in-out 0.1s;");
          client.print("-webkit-box-shadow: #000 0px 5px 10px -5px;");
          client.print("-moz-box-shadow: #000 0px 5px 10px -5px;");
          client.print("-o-box-shadow: #000 0px 5px 10px -5px;");
          client.print("-ms-box-shadow: #000 0px 5px 10px -5px;");
          client.print("box-shadow: #000 0px 5px 10px -5px;");
          client.print("}");
          client.print(".loader_website > span > svg {");
          client.print("fill: transparent;");
          client.print("stroke: #563d7c;");
          client.print("stroke-width: 5;");
          client.print("animation: loader_dash 2s ease infinite, loader_rotate 2s linear infinite;");
          client.print("}");
          client.print("@keyframes loader_dash {");
          client.print("0% {");
          client.print("stroke-dasharray: 1, 95;");
          client.print("stroke-dashoffset: 0;");
          client.print("}");
          client.print("50% {");
          client.print("stroke-dasharray: 85, 95;");
          client.print("stroke-dashoffset: -25;");
          client.print("}");
          client.print("100% {");
          client.print("stroke-dasharray: 85, 95;");
          client.print("stroke-dashoffset: -93;");
          client.print("}");
          client.print("}");
          client.print("@keyframes loader_rotate {");
          client.print("0% {");
          client.print("transform: rotate(0deg);");
          client.print("}");
          client.print("100% {");
          client.print("transform: rotate(360deg);");
          client.print("}");
          client.print("}");
          client.println("select {");
          client.println("color: black;");
          client.println("-webkit-appearance: none;");
          client.println("-moz-appearance: none;");
          client.println("appearance: none;");
          client.println("background-color: transparent;");
          client.println("border: none;");
          client.println("padding: 0 1em 0 0;");
          client.println("margin: 0;");
          client.println("width: 100%;");
          client.println("font-family: inherit;");
          client.println("font-size: inherit;");
          client.println("cursor: inherit;");
          client.println("line-height: inherit;");
          client.println("z-index: 1;");
          client.println("outline: none;");
          client.println("}");
          client.println("select::-ms-expand {");
          client.println("display: none;");
          client.println("}");
          client.println(".select {");
          client.println("color: black;");
          client.println("display: grid;");
          client.println("grid-template-areas: 'select';");
          client.println("align-items: center;");
          client.println("position: relative;");
          client.println("min-width: 15ch;");
          client.println("max-width: 30ch;");
          client.println("border: 1px solid var(--select-border);");
          client.println("border-radius: 0.25em;");
          client.println("padding: 0.25em 0.5em;");
          client.println("font-size: 1.25rem;");
          client.println("cursor: pointer;");
          client.println("line-height: 1.1;");
          client.println("background-color: #fff;");
          client.println("background-image: linear-gradient(to top, #f9f9f9, #fff 33%);");
          client.println("}");
          client.println(".select select, .select::after {");
          client.println("grid-area: select;");
          client.println("}");
          client.println(".select:not(.select--multiple)::after {");
          client.println("content: '';");
          client.println("justify-self: end;");
          client.println("width: 0.8em;");
          client.println("height: 0.5em;");
          client.println("background-color: var(--select-arrow);");
          client.println("-webkit-clip-path: polygon(100% 0%, 0 0%, 50% 100%);");
          client.println("clip-path: polygon(100% 0%, 0 0%, 50% 100%);");
          client.println("}");
          client.println("select:focus + .focus {");
          client.println("position: absolute;");
          client.println("top: -1px;");
          client.println("left: -1px;");
          client.println("right: -1px;");
          client.println("bottom: -1px;");
          client.println("border: 2px solid var(--select-focus);");
          client.println("border-radius: inherit;");
          client.println("}");
          client.println("select[multiple] {");
          client.println("height: 6rem;");
          client.println("}");
          client.println("select[multiple] option {");
          client.println("white-space: normal;");
          client.println("outline-color: var(--select-focus);");
          client.println("}");
          client.println(".select--disabled {");
          client.println("cursor: not-allowed;");
          client.println("background-color: #eee;");
          client.println("background-image: linear-gradient(to top, #ddd, #eee 33%);");
          client.println("}");
          client.println("label {");
          client.println("font-size: 1.125rem;");
          client.println("font-weight: 500;");
          client.println("}");
          client.println(".select + label {");
          client.println("margin-top: 2rem;");
          client.println("}");
          client.println("body {");
          client.println("min-height: 100vh;");
          client.println("display: grid;");
          client.println("place-content: center;");
          client.println("grid-gap: 0.5rem;");
          client.println("font-family: 'Baloo 2', sans-serif;");
          client.println("/*background-color: #e9f2fd;*/");
          client.println("padding: 1rem;");
          client.println("}");

          client.println(".formcontent label {");
          client.println("/*color: red;*/");
          client.println("display: block;");
          client.println("}");

          client.println(".formcontent {");
          client.println("text-align: center;");
          client.println("width: 30ch;");
          client.println("margin: 0 auto;");
          client.println("}");

          client.println(".formcontent li {");
          client.println("list-style: none;");
          client.println("padding-top: 10px;");
          client.println("width: 30ch;");
          client.println("}");

          client.println(".formcontent input {");
          client.println("color: black;");
          client.println("display: grid;");
          client.println("grid-template-areas: 'select';");
          client.println("align-items: center;");
          client.println("position: relative;");
          client.println("min-width: 15ch;");
          client.println("max-width: 24ch;");
          client.println("border: 1px solid var(--select-border);");
          client.println("border-radius: 0.25em;");
          client.println("padding: 0.25em 0.5em;");
          client.println("font-size: 1.25rem;");
          client.println("cursor: pointer;");
          client.println("line-height: 1.1;");
          client.println("background-color: #fff;");
          client.println("text-align: center;");
          client.println("background-image: linear-gradient(to top, #f9f9f9, #fff 33%);");
          client.println("}");

          client.println(".formcontent button {");
          client.println("color: black;");
          client.println("display: grid;");
          client.println("grid-template-areas: 'select';");
          client.println("align-items: center;");
          client.println("position: relative;");
          client.println("min-width: 15ch;");
          client.println("max-width: 24ch;");
          client.println("border: 1px solid var(--select-border);");
          client.println("border-radius: 0.25em;");
          client.println("padding: 0.25em 0.5em;");
          client.println("font-size: 1.25rem;");
          client.println("cursor: pointer;");
          client.println("line-height: 1.1;");
          client.println("background-color: #fff;");
          client.println("text-align: center;");
          client.println("background-image: linear-gradient(to top, #f9f9f9, #fff 33%);");
          client.println("}");

          client.println("html {");

          client.println("background: linear-gradient(145deg,");
          client.println("rgba(43, 9, 82, 1) 9%,");
          client.println("rgba(2, 2, 255, 1) 18%,");
          client.println("rgba(2, 255, 2, 1) 27%,");
          client.println("rgba(196, 181, 0, 1) 36%,");
          client.println("rgba(255, 165, 0, 1) 45%,");
          client.println("rgba(162, 19, 19, 1) 63%,");
          client.println("rgba(255, 165, 0, 1) 72%,");
          client.println("rgba(196, 181, 0, 1) 81%,");
          client.println("rgba(2, 255, 2, 1) 90%,");
          client.println("rgba(2, 2, 255, 1) 99%,");
          client.println("rgba(43, 9, 82, 1) 100%);");

          client.println("background-size: 400% 400%;");
          client.println("animation: gradient 5s ease infinite;");

          client.println("height: 100vh;");
          client.println("width: 100vw;");
          client.println("align-items: center;");
          client.println("justify-content: center;");
          client.println("}");

          client.println("@keyframes gradient {");
          client.println("0% {");
          client.println("background-position: 0% 50%;");
          client.println("}");
          client.println("50% {");
          client.println("background-position: 100% 50%;");
          client.println("}");
          client.println("100% {");
          client.println("background-position: 0% 50%;");
          client.println("}");
          client.println("}");

          client.println(".loader {");
          client.println("border: 16px solid #f3f3f3; /* Light grey */");
          client.println("border-style: solid;");
          client.println("border-width: 0px;");

          client.println("}");

          client.println("@keyframes spin {");
          client.println("0% {");
          client.println("transform: rotate(0deg);");
          client.println("}");
          client.println("100% {");
          client.println("transform: rotate(360deg);");
          client.println("}");
          client.println("}");

          client.println(".slidecontainer {width: 100%;}");

          client.println(".slider {-webkit-appearance: none;width: 100%;height: 25px;background: #d3d3d3;outline: none;opacity: 0.7;-webkit-transition: .2s;transition: opacity .2s;}");

          client.println(".slider:hover {opacity: 1;}");

          client.println(".slider::-webkit-slider-thumb {-webkit-appearance: none;appearance: none;width: 25px;height: 25px;background: #04AA6D;cursor: pointer;}");

          client.println(".slider::-moz-range-thumb {width: 25px;height: 25px;background: #04AA6D;cursor: pointer;}");
          client.println(".slider:disabled::-webkit-slider-thumb{background: grey;cursor: not-allowed !important;}");
          client.println(".slider:disabled::-moz-range-thumb{background: grey;cursor: not-allowed !important;}");
          client.println(".slider:disabled{cursor: not-allowed !important;}");

          client.println("</style>");

          client.println("<header class='container-col'>");

          client.println("</header>");
          ////  client.print("<form method='get' action=''>");
          //  client.println("<form onsubmit='return false'>");
          client.println("<div class='formcontent'>");
          client.println("<ul>");
          client.println("<Li>");
          client.println("<H3>Enter your SSID and Password below. </h3>");
          client.println("</Li>");
          client.println("<Li>");
          client.println("<label for='ssid'>SSID</label>");
          client.println("<input id='ssid' required type='text' name='ssid'");
          client.print("value='");
          client.print(owner.wifissid);
          client.print("' placeholder='ssid'>");
          client.println("</Li>");
          client.println("<li>");
          client.println("<label for='password'>Password</label>");
          client.println("<input id='password'");
          client.println("       type='password'");
          client.println("       name='password'");
          client.println("      value='");
          client.print(owner.Password);
          client.print("' placeholder='password' required><br>");

          client.println("</li>");
          client.println("<li>");
          client.println("<label for='ShowTemp'>Show Temperature every 1 minute</label>");
          client.println("<div class='select'>");
          client.print("<select name='ShowTemp' id='ShowTemp'>");
          client.print("<option value='off'>off</option>");
          client.print("<option value='on' ");
          client.print(ShowTempSetting);
          client.print(">on</option>");
          client.print("</select>");
          client.println("<span class='focus'></span>");
          client.println("</div>");

          client.println("</li>");

          client.println("<li>");
          client.println("<label for='TempUnits'>Temperature Units</label>");
          client.println("<div class='select'>");
          client.print("<select name='TempUnits' id='TempUnits'>");
          client.print("<option value='C'>C</option>");
          client.print("<option value='F' ");
          client.print(ShowTempUnitsSetting);
          client.print(">F</option>");
          client.print("</select>");
          client.println("<span class='focus'></span>");
          client.println("</div>");

          client.println("</li>");
          client.println("<li>");
          client.println("<label for='12hr'>Show 12hr Time</label>");
          client.println("<div class='select'>");
          client.println("<select name='12hr' id='12hr'>");
          client.print("<option value='off'>off</option>");
          client.print("<option value='on' ");
          client.print(twelvehour);
          client.print(">on</option>");
          client.print("</select>");
          client.println("<span class='focus'></span>");
          client.println("</div>");

          client.println("</li>");
          client.println("<li>");
          client.println("<label for='timezone'>Timezone</label>");
          client.println("<div class='select'>");
          client.println("<select name='timezone' id='timezone'>");
          client.print("<option value='EST' ");
          if ((String)owner.timezone == "EST") {
            client.print(" selected='selected' ");
          }
          client.print(">Eastern</option>");
          client.print("<option value='CST' ");
          if ((String)owner.timezone == "CST") {
            client.print(" selected='selected' ");
          }
          client.print(">Central</option>");
          client.print("<option value='MST' ");
          if ((String)owner.timezone == "MST") {
            client.print(" selected='selected' ");
          }
          client.print(">Mountain</option>");
          client.print("<option value='PST' ");
          if ((String)owner.timezone == "PST") {
            client.print(" selected='selected' ");
          }
          client.print(">Pacific</option>");
          client.print("<option value='KST' ");
          if ((String)owner.timezone == "KST") {
            client.print(" selected='selected' ");
          }
          client.print(">Alaska</option>");
          client.print("<option value='HST' ");
          if ((String)owner.timezone == "HST") {
            client.print(" selected='selected' ");
          }
          client.print(">Hawaii</option>");
          client.print("<option value='ZST' ");
          if ((String)owner.timezone == "ZST") {
            client.print(" selected='selected' ");
          }
          client.print(">Arizona</option>");
          client.print("<option value='GMT' ");
          if ((String)owner.timezone == "GMT") {
            client.print(" selected='selected' ");
          }
          client.print(">GMT</option>");
          client.print("<option value='CET' ");
          if ((String)owner.timezone == "CET") {
            client.print(" selected='selected' ");
          }
          client.print(">Central European Time</option>");
          client.print("<option value='AST' ");
          if ((String)owner.timezone == "AST") {
            client.print(" selected='selected' ");
          }
          client.print(">Argentina Standard Time</option>");
          ////client.print(twelvehour);

          client.print("</select>");

          client.println("<li>");
          client.println("<label for='Secondtimezone'>Second Timezone</label>");
          client.println("<div class='select'>");
          client.println("<select name='Secondtimezone' id='Secondtimezone'>");
          client.print("<option value='EST' ");
          if ((String)owner.secondtimezone == "EST") {
            client.print(" selected='selected' ");
          }
          client.print(">Eastern</option>");
          client.print("<option value='CST' ");
          if ((String)owner.secondtimezone == "CST") {
            client.print(" selected='selected' ");
          }
          client.print(">Central</option>");
          client.print("<option value='MST' ");
          if ((String)owner.secondtimezone == "MST") {
            client.print(" selected='selected' ");
          }
          client.print(">Mountain</option>");
          client.print("<option value='PST' ");
          if ((String)owner.secondtimezone == "PST") {
            client.print(" selected='selected' ");
          }
          client.print(">Pacific</option>");
          client.print("<option value='KST' ");
          if ((String)owner.secondtimezone == "KST") {
            client.print(" selected='selected' ");
          }
          client.print(">Alaska</option>");
          client.print("<option value='HST' ");
          if ((String)owner.secondtimezone == "HST") {
            client.print(" selected='selected' ");
          }
          client.print(">Hawaii</option>");
          client.print("<option value='ZST' ");
          if ((String)owner.secondtimezone == "ZST") {
            client.print(" selected='selected' ");
          }
          client.print(">Arizona</option>");
          client.print("<option value='GMT' ");
          if ((String)owner.secondtimezone == "GMT") {
            client.print(" selected='selected' ");
          }
          client.print(">GMT</option>");
          client.print("<option value='CET' ");
          if ((String)owner.secondtimezone == "CET") {
            client.print(" selected='selected' ");
          }
          client.print(">Central European Time</option>");
          client.print("<option value='AST' ");
          if ((String)owner.secondtimezone == "AST") {
            client.print(" selected='selected' ");
          }
          client.print(">Argentina Standard Time</option>");


          client.print("</select>");
          client.println("<li>");
          client.println("<label for='dateformat'>Date Format</label>");
          client.println("<div class='select'>");
          client.println("<select name='dateformat' id='dateformat'>");
          client.print("<option value='on'>mm-dd</option>");
          client.print("<option value='off' ");
          client.print(dateformat);
          client.print(">dd-mm</option>");
          client.print("</select>");
          client.println("<span class='focus'></span>");
          client.println("</div>");

          client.println("<li>");
          client.println("<label for='dimmer'>Enable Auto Dimming</label>");
          client.println("<div class='select'>");
          client.println("<select name='dimmer' id='dimmer'>");
          client.print("<option value='on'>on</option>");
          client.print("<option value='off' ");
          client.print(DimmerSetting);
          client.print(">off</option>");
          client.print("</select>");
          client.println("<span class='focus'></span>");
          client.println("</div>");

          client.println("</li>");
          client.println("<li>");

          client.println("<label for='myrange'>Display Brightness</label>");

          client.println("<div class='slidecontainer'>");
          client.println("<input type='range' name='range' min='1' max='255' ");

          client.print("value='");

          String Brightnesslevel = (String)owner.brightness;
          Brightnesslevel.trim();
          client.print(Brightnesslevel);

          client.print("' class='slider' id='myRange'>");
          client.println("<input type='hidden' name='rangevalue' ");
          client.print("value='");
          client.print(Brightnesslevel);
          client.print("' id='rangevalue'>");
          // client.println(" <p>Value: <span id='demo'></span></p>");
          client.println(" </div>");

          client.println("</li>");

          client.println("<li>");
          client.println("<button id='Submit' name='Submit' value='Submit' onclick='submit();' >Save</button>");
          client.println("</li>");
          client.println("</ul>");
          client.println("</div>");
          //  client.println("</form>");
          client.println("<script>");

          client.println("if (document.getElementById('dimmer').value === 'off') {");
          client.println("document.getElementById('myRange').removeAttribute('disabled');");
          client.println("} else {");
          client.println("document.getElementById('myRange').setAttribute('disabled', 'disabled');");
          client.println("}");
          client.println("document.getElementById('dimmer').onchange = function () {");
          client.println("document.getElementById('myRange').setAttribute('disabled', 'disabled');");
          client.println("if (this.value == 'off') {");
          client.println("document.getElementById('myRange').removeAttribute('disabled');");
          client.println("}");
          client.println("};");

          client.println("var slider = document.getElementById('myRange');");
          client.println("var output = document.getElementById('rangevalue');");
          client.println("output.value = slider.value;");

          client.println("slider.oninput = function() {");
          client.println("  output.value = this.value;");
          client.println("  slider.value = this.value;");
          client.println("}");
          client.print("var Loader = {");
          client.print("loader: null,");
          client.print("body: null,");
          client.print("html: '<span><svg width=\\'40\\' height=\\'40\\' version=\\'1.1\\' xmlns=\\'http://www.w3.org/2000/svg\\'><circle cx=\\'20\\' cy=\\'20\\' r=\\'15\\'></svg></span>',");
          client.print("cssClass: 'loader',");
          client.print("check: function () {");
          client.print("if (this.body == null) {");
          client.print("this.body = document.getElementsByTagName('body')[0];");
          client.print("}");
          client.print("},");
          client.print("open: function () {");
          client.print("this.check();");
          client.print("if (!this.isOpen()) {");
          client.print("this.loader = document.createElement('div');");
          client.print("this.loader.setAttribute('id', 'loader');");
          client.print("this.loader.classList.add('loader_website');");
          client.print("this.loader.innerHTML = this.html;");
          client.print("this.body.appendChild(this.loader);");
          client.print("setTimeout(function () {");
          client.print("Loader.body.classList.add(Loader.cssClass);");
          client.print("}, 1);");
          client.print("}");
          client.print("return this;");
          client.print("},");
          client.print("close: function () {");
          client.print("this.check();");
          client.print("if (this.isOpen()) {");
          client.print("this.body.classList.remove(this.cssClass);");
          client.print("setTimeout(function () {");
          client.print("Loader.loader.remove();");
          client.print("}, 100);");
          client.print("}");
          client.print("return this;");
          client.print("},");
          client.print("isOpen: function () {");
          client.print("this.check();");
          client.print("return this.body.classList.contains(this.cssClass);");
          client.print("},");
          client.print("ifOpened: function (callback, close) {");
          client.print("this.check();");
          client.print("if (this.isOpen()) {");
          client.print("if (!!close)");
          client.print("this.close();");
          client.print("if (typeof callback === 'function') {");
          client.print("callback();");
          client.print("}");
          client.print("}");
          client.print("return this;");
          client.print("},");
          client.print("ifClosed: function (callback, open) {");
          client.print("this.check();");
          client.print("if (!this.isOpen()) {");
          client.print("if (!!open)");
          client.print("this.open();");
          client.print("if (typeof callback === 'function') {");
          client.print("callback();");
          client.print("}");
          client.print("}");
          client.print("return this;");
          client.print("}");
          client.print("};");
          client.println("function submit() {");
          client.println("Loader.open();");
          client.println("var FormValid = true;");
          client.println("var data = [];");

          client.println(" const container = document.querySelector('div.formcontent');");
          client.println(" container.querySelectorAll('input').forEach(function (e) {");
          client.println("  if (e.validity.valueMissing) {");
          client.println("    FormValid = false;");
          client.println(";  }");

          client.println("  data[e.id] = e.value;");

          client.println("});");
          client.println("container.querySelectorAll('select').forEach(function (e) {");
          client.println("data[e.id] = e.value;");

          client.println("});");

          client.println("encodeDataToURL = (data) => {");
          client.println("return Object");
          client.println(".keys(data)");
          client.println(".map(value => `${value}=${encodeURIComponent(data[value])}`)");
          client.println(".join('&');");
          client.println("}");

          // console.log(encodeDataToURL(data));
          client.println("var mypost = encodeDataToURL(data)");
          client.println("mypost = mypost + '&Submit=Submit'");
          // console.log(mypost);
          client.println("if (FormValid) {");
          client.println("var request = new XMLHttpRequest();");
          client.println("request.open('GET', '/', true);");
          client.println("request.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');");
          client.println("request.setRequestHeader('mydata','/?'+encodeDataToURL(data));");
          client.println("request.onreadystatechange = function () {");
          client.println("if (this.readyState == XMLHttpRequest.DONE && this.status == 200) {");
          client.println("console.log('succeed');");
          client.print("Loader.close()");
          // client.println("el.classList.add('hidden');");
          //   myresponse.value = request.responseText;
          client.println("} else {");
          client.println("console.log('server error');");
          client.print("Loader.close()");

          client.println("}");
          client.println("};");

          client.println("request.onerror = function () {");
          client.println("console.log('something went wrong');");
          client.print("Loader.close()");
          client.println("};");

          client.println("request.send(mypost);");
          client.println("}");
          client.println("}");
          //          client.println("           </script>");

          client.println("</script>");
          client.println("</BODY>");
          client.println("</HTML>");
          break;
        }
        if (c == '\n') {
          Serial.println(readString);
          if (readString.indexOf("ssid") > 0) {
            debugoutput(1, 0, 0, 0, 150, 0, 150);
            // Serial.println(readString);
            char Buf[250];
            readString.toCharArray(Buf, 150);
            char *token = strtok(Buf, "/?");  // Get everything up to the /if(token) // If we got something
            {
              char *name = strtok(NULL, "=");  // Get the first name. Use NULL as first argument to keep parsing same string
              while (name) {
                char *valu = strtok(NULL, "&");
                if (valu) {
                  Serial.println(String(name));
                  Serial.println(String(valu));
                  if (String(name) == "?ssid") {
                    String ssidname = valu;
                    ssidname.toCharArray(owner.wifissid, 100);
                  }
                  if (String(name) == "password") {
                    String pass = valu;
                    pass.toCharArray(owner.Password, 100);
                  }
                  if (String(name) == "timezone") {
                    String pass = valu;
                    pass.toCharArray(owner.timezone, 10);
                  }
                  if (String(name) == "Secondtimezone") {
                    String pass = valu;
                    //                      String pass = "EST";
                    pass.toCharArray(owner.secondtimezone, 10);
                  }
                  if (String(name) == "12hr") {
                    String twelvehr = valu;
                    twelvehr.toCharArray(owner.twelvehr, 3);
                  }
                  if (String(name) == "ShowTemp") {
                    String temp = valu;
                    temp.toCharArray(owner.temp, 3);
                  }
                  if (String(name) == "TempUnits") {
                    String temp = valu;
                    temp.toCharArray(owner.tempUnits, 3);
                  }
                  if (String(name) == "dateformat") {
                    String temp = valu;
                    temp.toCharArray(owner.USDateFormat, 3);
                  }

                  if (String(name) == "myRange") {
                    String brightness = valu;
                    brightness.toCharArray(owner.brightness, 5);
                  }
                  if (String(name) == "dimmer") {
                    String dimmer = valu;
                    dimmer.toCharArray(owner.dimmer, 3);
                  }
                  name = strtok(NULL, "=");  // Get the next name
                }
              }
              //                                    String pass = "EST";
              //                    pass.toCharArray(owner.secondtimezone, 10);
            }
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
            Serial.println("Starting Wifi Check");

            String returnedip = validateSSID(owner);
            if (returnedip == "0.0.0.0") {
              Serial.println("Wifi Failed");
              status = WiFi.beginAP(ssid);
              debugoutput(1, 0, 1, 0, 255, 0, 0);
              printWiFiStatus();
            } else {  // connection successful.  Top and bottow rows blue.
              status = WiFi.begin(owner.wifissid, owner.Password);
              debugoutput(1, 0, 1, 0, 0, 0, 255);
              owner.valid = true;
              Serial.println("Saving Data");
              my_flash_store.write(owner);
              delay(2000);
              digitalWrite(Reset, LOW);
              Serial.println("Restarting Server");
              //              owner2 = my_flash_store.read();
              //              Serial.println((String)owner2.temp);
              //              Serial.println((String)owner2.valid);
              ////digitalWrite(Reset_PIN, LOW);
            }
          }

          readString = "";
          // you're starting a new line

          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          // Serial.println("b");
          currentLineIsBlank = false;
        }
      } else {
        Serial.println("Client Unavailable");
      }
    }

    delay(1);

    // close the connection:

    client.stop();

    Serial.println("client disonnected");
  } else {
    // Serial.println ("Client is false");
  }
}

void debugoutput (int top, int mid, int bottom, int dots, int R, int G, int B) {

  strip.show();            // Turn OFF all pixels ASAP
  strip.clear();
  if (top == 1) {
    strip.setPixelColor(11, strip.Color(R, G, B));
    strip.setPixelColor(10, strip.Color(R, G, B));
    strip.setPixelColor(9, strip.Color(R, G, B));
    strip.setPixelColor(30, strip.Color(R, G, B));
    strip.setPixelColor(31, strip.Color(R, G, B));
    strip.setPixelColor(32, strip.Color(R, G, B));
    strip.setPixelColor(53, strip.Color(R, G, B));
    strip.setPixelColor(54, strip.Color(R, G, B));
    strip.setPixelColor(55, strip.Color(R, G, B));
    strip.setPixelColor(74, strip.Color(R, G, B));
    strip.setPixelColor(75, strip.Color(R, G, B));
    strip.setPixelColor(76, strip.Color(R, G, B));
  }
  if (mid == 1) {
    strip.setPixelColor(82, strip.Color(R, G, B));
    strip.setPixelColor(15, strip.Color(R, G, B));
    strip.setPixelColor(16, strip.Color(R, G, B));
    strip.setPixelColor(17, strip.Color(R, G, B));
    strip.setPixelColor(38, strip.Color(R, G, B));
    strip.setPixelColor(37, strip.Color(R, G, B));
    strip.setPixelColor(36, strip.Color(R, G, B));
    strip.setPixelColor(61, strip.Color(R, G, B));
    strip.setPixelColor(60, strip.Color(R, G, B));
    strip.setPixelColor(59, strip.Color(R, G, B));
    strip.setPixelColor(80, strip.Color(R, G, B));
    strip.setPixelColor(81, strip.Color(R, G, B));
  }
  if (bottom == 1) {
    strip.setPixelColor(0, strip.Color(R, G, B));
    strip.setPixelColor(1, strip.Color(R, G, B));
    strip.setPixelColor(2, strip.Color(R, G, B));
    strip.setPixelColor(21, strip.Color(R, G, B));
    strip.setPixelColor(22, strip.Color(R, G, B));
    strip.setPixelColor(23, strip.Color(R, G, B));
    strip.setPixelColor(44, strip.Color(R, G, B));
    strip.setPixelColor(45, strip.Color(R, G, B));
    strip.setPixelColor(46, strip.Color(R, G, B));
    strip.setPixelColor(65, strip.Color(R, G, B));
    strip.setPixelColor(66, strip.Color(R, G, B));
    strip.setPixelColor(67, strip.Color(R, G, B));
  }
  if (dots == 1) {
    strip.setPixelColor(42, strip.Color(R, G, B));
    strip.setPixelColor(43, strip.Color(R, G, B));
  }
  strip.show();
}
// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress & address) {
  Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Serial.println("5");
  Udp.endPacket();
  Serial.println("6");
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void GetTime() {
  //ok we're going to try and do DNS Lookups
  IPAddress result;
  int err = WiFi.hostByName("0.us.pool.ntp.org", result) ;
  Serial.print("Getting Time from IP : ");
  Serial.println(result);
  if (err == 1) {
    Serial.println("--------------------------------------");
    Serial.println(result);
    Udp.begin(localPort);
    sendNTPpacket(result);  // send an NTP packet to a time server
    delay(3000);
    if (Udp.parsePacket()) {
      Serial.println("packet received");
      debugoutput(0, 0, 0, 1, 0, 0, 255); // turn the dots blue
      // We've received a packet, read the data from it
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer
      delay(1000);
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;
      rtc.begin();  // initialize RTC
      rtc.disable32K();
      rtc.adjust(DateTime(epoch + 4));
      rtc.clearAlarm(1);
      rtc.disable32K();
      pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
      attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), alarm, FALLING);
      rtc.clearAlarm(1);
      rtc.clearAlarm(2);
      rtc.writeSqwPinMode(DS3231_OFF);
      rtc.setAlarm2(rtc.now(), DS3231_A2_PerMinute);
      rtc.setAlarm1(DateTime(0, 0, 0, 0, 0, 55), DS3231_A1_Second);

      if (checktime == 1) {
        checktime = 0;
      } else {
        SetTime();
      }
    } else {
      debugoutput(1, 1, 1, 0, 255, 75, 250);
      //didn't get NTP Packet
      delay(10000);
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(err);
    debugoutput(1, 1, 1, 0, 150, 150, 150); // failed on dns lookups
    delay(10000);
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
String validateSSID(Credentials owner) {
  status = WiFi.begin(owner.wifissid, owner.Password);
  delay(10000);
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("brightness: ");
  Serial.println(owner.brightness);
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  String LocalIP = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3];
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.print("connection status: ");
  Serial.println(server.status());
  return (LocalIP);
}
