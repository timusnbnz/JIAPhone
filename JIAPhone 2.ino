//Librarys
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <HTTPClient.h>
//PROGMEM vars
PROGMEM String tosversion = "T-OS Alpha v0.1";
PROGMEM const int pin_sda = 12;
PROGMEM const int pin_scl = 14;
PROGMEM byte pin_rows[4] = {13, 15, 2, 0};
PROGMEM byte pin_column[4] = {4, 16, 17, 5};
PROGMEM char keys[4][4] = {{'X', 'X', 'X', 'X'}, {'1', '4', '7', 'Y'}, {'2', '5', '8', '0'}, {'3', '6', '9', 'N'}};
PROGMEM char* menuitems[7] = {"Call", "Messages", "Contacts", "Cloudservices", "Flashlight", "Settings", "Informations"};
PROGMEM char* settingsitems[7] = {"WiFi-Setup", "WiFi-Info" , "Debugging", "Reset", "", "", ""};
PROGMEM String key1 = "1";
PROGMEM String key2 = "2abcABC";
PROGMEM String key3 = "3defDEF";
PROGMEM String key4 = "4ghiGHI";
PROGMEM String key5 = "5jklJKL";
PROGMEM String key6 = "6mnoMNO";
PROGMEM String key7 = "7pqrsPQRS";
PROGMEM String key8 = "8tuvTUV";
PROGMEM String key9 = "9wxyzWXYZ";
PROGMEM String key0 = "0 !?";
PROGMEM String serveradress = "";
//Predefinded vars
boolean debugging;
boolean firstsetup = false;
//Just created vars
int bootcounts;
char key;
int batterypercent;
float batteryvoltage;
unsigned long lastbattery = 0;
boolean batteryupdate;
boolean light = false;
//Setting up devices
Adafruit_SH1106 display(pin_sda, pin_scl);
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, 4, 4);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()   {
  //Initializes Everything
  Serial.begin(115200);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  pinMode(22, OUTPUT);
  pinMode(35, INPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 1);
  digitalWrite(22, HIGH);
  boot();
  mainmenu();
}

void printheading(String headingtext) {
  display.setCursor(3, 1);
  display.setTextSize(1);
  display.drawLine(0, 12, 128, 12, WHITE);
  display.println(headingtext);
  display.display();
}

void printlines(String header, String line1, String line2, String line3, String line4, boolean scrolldown = false) {
  display.clearDisplay();
  printheading(header);
  display.setCursor(3, 16);
  display.println(line1);
  display.setCursor(3, 25);
  display.println(line2);
  display.setCursor(3, 34);
  display.println(line3);
  display.setCursor(3, 43);
  display.println(line4);
  if (scrolldown) {
    display.setCursor(3, 52);
    display.println("v");
  }
  display.display();
}

void comingsoon() {
  printlines("Coming soon...", "This function is in", "progress and cannot", "be used yet.", "Press any button.");
  while (not keypad.getKey()) {
    background();
  }
}

void background() {
  if (digitalRead(35) == HIGH) {
    while (digitalRead(35) == HIGH) {}
    debug("Going to sleep");
    display.clearDisplay();
    display.display();
    esp_deep_sleep_start();
  }
  if (millis() - lastbattery > 10000) {
    lastbattery = millis();
    batteryupdate = true;
    batteryvoltage = analogRead(34) * 2 * 0.000805;
    batterypercent = (batteryvoltage - 3) * 100;
    Serial.println(batteryvoltage);
    if (batterypercent < 0) {
      batterypercent = 0;
    }
    if (batterypercent > 100) {
      batterypercent = 100;
    }
  }
}

void infomenu() {
  printlines("About", "Dev. by Tim U.", tosversion, "Bootcount: " + String(bootcountread()), "IMEI Unknown");
  while (not keypad.getKey()) {
    background();
  }
}

void resetmenu() {
  boolean dobreak = false;
  printlines("Reset", "Do you want to", "reset this device?", "Y - Reset", "N - Cancel");
  while (not dobreak) {
    background();
    key = keypad.getKey();
    if (key == 'Y') {
      EEPROM.write(2, 0);
      EEPROM.commit();
      dobreak = true;
      boot();
    }
    if (key == 'N') {
      dobreak = true;
    }
  }
}

void debuggingsettings() {
  boolean dobreak = false;
  printlines("Seria debugging", "Change active after", "rebooting.", "Y - Enable", "N - Disable");
  while (not dobreak) {
    background();
    key = keypad.getKey();
    if (key == 'Y') {
      EEPROM.write(3, 1);
      EEPROM.commit();
      dobreak = true;
    }
    if (key == 'N') {
      EEPROM.write(3, 1);
      EEPROM.commit();
      dobreak = true;
    }
  }
}

void wifiinfo() {
  if (WiFi.status() ==  WL_CONNECTED) {
    printlines(firstChars(WiFi.SSID(), 18), "Connected!", "Strength: " + String(WiFi.RSSI()), iptostring(WiFi.localIP()), "");
  } else {
    printlines("Not connected!", "Go to Wifi-Setup", "to connect to an", "WiFi Network. ", "");
  }
  while (not keypad.getKey()) {
    background();
  }
}

void settingsmenu() {
  boolean doupdate = true;
  boolean scrolldown = false;
  boolean dobreak = false;
  int offset = 0;
  int listsize = 4;
  while (not dobreak) {
    background();
    char key = keypad.getKey();
    if (key == 'N') {
      dobreak  = true;
    }
    if (key == '8') {
      if (offset != listsize - 1) {
        offset++;
        doupdate = true;
      }
    }
    if (key == '2') {
      if (offset != 0) {
        offset--;
        doupdate = true;
      } else {
        dobreak = true;
      }
    }
    if (key == 'Y') {
      if (offset == 0) {
        wifisetup();
      }
      if (offset == 1) {
        wifiinfo();
      }
      if (offset == 2) {
        debuggingsettings();
      }
      if (offset == 3) {
        resetmenu();
      }
      display.clearDisplay();
      display.display();
      doupdate = true;
    }
    if (doupdate) {
      if (offset + 5 > listsize) {
        scrolldown = false;
      } else {
        scrolldown = true;
      }
      printlines("Settings", "> " + String(settingsitems[offset]), "  " + String(settingsitems[1 + offset]), "  " + String(settingsitems[2 + offset]), "  " + String(settingsitems[3 + offset]), scrolldown);
      doupdate = false;
    }
  }
}

void cloudservices() {
  boolean dobreak = false;
  boolean doupdate = true;
  boolean tryconnect = false;
  HTTPClient http;
  while (not dobreak) {
    background();
    key = keypad.getKey();
    if (doupdate) {
      doupdate = false;
      printlines("Cloudservices", "Trying to connect", "to the server...", "", "");
    }
    if (tryconnect) {
      tryconnect = false;
      String serverPath = serveradress + "?battery=" + String(batterypercent);
      http.begin(serverPath.c_str());
      int httpResponseCode = http.GET();
      if (httpResponseCode > 0) {
        debug("HTTP Response code: " + String(httpResponseCode));
        String payload = http.getString();
        debug(payload);
      }
      else {
        debug("HTTP Error code: " + String(httpResponseCode));
      }
      http.end();
    }
    if (key == 'N') {
      dobreak = true;
    }
  }
}

void mainmenu() {
  boolean doupdate = true;
  boolean scrolldown = false;
  boolean lastwifistatus = false;
  int listsize = 7;
  int offset = 0;
  String wifistatus = "    ";
  unsigned long lasttried = millis();
  WiFi.reconnect();
  while (true) {
    background();
    char key = keypad.getKey();
    if (key == '8') {
      if (offset != listsize - 1) {
        offset++;
        doupdate = true;
      }
    }
    if (key == '2') {
      if (offset != 0) {
        offset--;
        doupdate = true;
      }
    }
    if (key == 'Y') {
      if (offset == 0) {
        comingsoon();
      }
      if (offset == 1) {
        comingsoon();
      }
      if (offset == 2) {
        comingsoon();
      }
      if (offset == 3) {
        cloudservices();
      }
      if (offset == 4) {
        if (light == true) {
          digitalWrite(22, LOW);
          light = false;
        } else {
          digitalWrite(22, HIGH);
          light = true;
        }
      }
      if (offset == 5) {
        settingsmenu();
      }
      if (offset == 6) {
        infomenu();
      }
      display.clearDisplay();
      display.display();
      doupdate = true;
    }
    if (doupdate) {
      if (offset + 5 > listsize) {
        scrolldown = false;
      } else {
        scrolldown = true;
      }
      printlines(wifistatus + "            " + String(batterypercent) + "%", "> " + String(menuitems[offset]), "  " + String(menuitems[1 + offset]), "  " + String(menuitems[2 + offset]), "  " + String(menuitems[3 + offset]), scrolldown);
      doupdate = false;
    }
    if (WiFi.status() == WL_CONNECTED) {
      wifistatus = "WiFi";
      if (lastwifistatus == false) {
        lastwifistatus = true;
        doupdate = true;
      }
    } else {
      wifistatus = "    ";
      if (lastwifistatus == true) {
        lastwifistatus = false;
        doupdate = true;
      }
      if ((millis() - lasttried) > 15000) {
        debug("Trying to reconnect...");
        WiFi.reconnect();
      }
    }
    if (batteryupdate) {
      batteryupdate = false;
      doupdate = true;
    }
  }
}

void boot() {
  //Print Bootscreen
  if (EEPROM.read(3) == 0) {
    debugging = false;
  } else {
    debugging = true;
  }
  debug("-DEBUG DATA BEGINNS HERE:\n");
  debug("Starting boot process");
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(42, 26);
  display.println("T-OS");
  display.setTextSize(1);
  display.setCursor(46, 42);
  display.println("Booting");
  display.display();
  //EEPROM Setup
  EEPROM.begin(512);
  bootcount();
  //Starting WiFi
  WiFi.mode(WIFI_STA);
  if (debugging == true) {
    display.setCursor(0, 0);
    display.println(String(bootcounts));
    display.display();
  }
  if (EEPROM.read(2) != 1) {
    debug("First setup");
    firstSetup();
  }
}

void firstSetup() {
  background();
  printlines("First Setup", "Welcome to T-OS for", "ESP32-based projects", "Press any button to", "continue.");
  while (not keypad.getKey()) {}
  printlines("WiFi Setup", "Start WiFi Setup?", "Y - Start WiFi Setup", "N - Skip WiFi Setup", "Wifi is recommended.");
  key = keypad.getKey();
  while (not key) {
    key = keypad.getKey();
    if (key == 'Y') {
      wifisetup();
    }
    if (key == 'N') {
      display.clearDisplay();
      display.display();
    }
  }
  EEPROM.write(2, 1);
  EEPROM.write(3, 0);
  EEPROM.commit();
  printlines("Setup completed", "The first setup of", "this device is now", "completed.", "Press any button.");
}

void wifisetup() {
searchwifi:
  int offset = 0;
  printlines("Scanning for WiFi", "", "", "", "");
  int nWiFi = WiFi.scanNetworks();
  boolean updatecontent = true;
  boolean sucess = false;
  while (not sucess) {
    char key = keypad.getKey();
    if (key == '8') {
      if (offset != nWiFi - 1) {
        offset++;
        updatecontent = true;
      }
    }
    if (key == '2') {
      offset--;
      updatecontent = true;
    }
    if (updatecontent == true) {
      printlines("Found " + String(nWiFi) + " networks", "> " + firstChars(WiFi.SSID(0 + offset), 18), "  " + firstChars(WiFi.SSID(1 + offset), 18), "  " + firstChars(WiFi.SSID(2 + offset), 18), "  " + firstChars(WiFi.SSID(3 + offset), 18));
      updatecontent = false;
    }
    if (offset == -1) {
      goto searchwifi;
    }
    if (key == 'Y') {
      if (connectToWifi(offset)) {
        sucess = true;
        Serial.println("Sucess");
      } else {
        updatecontent = true;
      }
    }
    if (key == 'N') {
      sucess = true;
    }
  }
}

boolean connectToWifi(int WiFicount) {
  printlines("WiFi-Info", firstChars(WiFi.SSID(WiFicount), 18), "RSSI:" + String(WiFi.RSSI(WiFicount)), "Y: Connect", "N: Go back");
  boolean dobreak = false;
  boolean sucess = false;
  while (not dobreak) {
    char key = keypad.getKey();
    if (key == 'Y') {
      String wifipassword = getinput("Enter password");
      if (wifipassword == "") {
        dobreak = true;
      }
      if (wifipassword.length() >= 8 and wifipassword != "") {
        printlines("Wifi Network", WiFi.SSID(WiFicount), wifipassword, "Trying to connect", "");
        WiFi.begin(WiFi.SSID(WiFicount).c_str(), wifipassword.c_str());
        int timeoutcounter = 0;
        sucess = true;
        display.setCursor(3, 43);
        while (WiFi.status() != WL_CONNECTED) {
          delay(1000);
          display.print(".");
          display.display();
          timeoutcounter++;
          if (timeoutcounter == 15) {
            sucess = false;
            break;
          }
        }
        if (sucess) {
          printlines("Connected!", WiFi.SSID(WiFicount), iptostring(WiFi.localIP()), "", "WiFi setup completed");
          debug("Connected to " + String(WiFi.SSID(WiFicount)) + " with IP " + iptostring(WiFi.localIP()));
        } else {
          printlines("Could not connect!", "Connection could not", "be established :(", "", "Press any key");
        }
        while (not keypad.getKey()) {}
        dobreak = true;
      } else {
        printlines("Error occured!", "The password has", "to contain at", "least 8 characters", "Y - Try again");
      }
    }
    if (key == 'N') {
      dobreak = true;
    }
  }
  if (sucess) {
    return (true);
  } else {
    return (false);
  }
}

String getinput(String heading) {
  boolean dobreak = false;
  boolean doupdate = true;
  unsigned long lastpress;
  String possibilitys;
  char lastkey;
  char selection = '+';
  int keycounter = 0;
  boolean resettrigger = false;
  String finalstring;
  while (not dobreak) {
    char key = keypad.getKey();
    if (doupdate) {
      display.clearDisplay();
      doupdate = false;
      printheading(heading);
      display.setCursor(3, 16);
      display.println("> " + finalstring);
      if (possibilitys != "") {
        display.setCursor(3, 43);
        display.println(String(selection) + " - " + possibilitys);
      }
      display.display();
    }
    if (key) {
      lastpress = millis();
      if (key != 'Y') {
        resettrigger = true;
      }
      if (key == lastkey) {
        if (keycounter == (possibilitys.length() - 1)) {
          keycounter = 0;
        } else {
          keycounter++;
        }
      } else {
        keycounter = 0;
        if (selection != '+' and key != 'Y') {
          finalstring += selection;
        }
      }
      lastkey = key;
      doupdate = true;
    }
    if (((millis() - lastpress) > 2500) and resettrigger) {
      possibilitys = "";
      doupdate = true;
      resettrigger = false;
      finalstring += selection;
    }
    if (key == '1') {
      possibilitys = key1;
    }
    if (key == '2') {
      possibilitys = key2;
    }
    if (key == '3') {
      possibilitys = key3;
    }
    if (key == '4') {
      possibilitys = key4;
    }
    if (key == '5') {
      possibilitys = key5;
    }
    if (key == '6') {
      possibilitys = key6;
    }
    if (key == '7') {
      possibilitys = key7;
    }
    if (key == '8') {
      possibilitys = key8;
    }
    if (key == '9') {
      possibilitys = key9;
    }
    if (key == '0') {
      possibilitys = key0;
    }
    if (key == 'N') {
      if (finalstring != "") {
        finalstring = "";
        resettrigger = false;
        possibilitys = "";
        selection = '+';
      }
      else {
        dobreak = true;
      }
    }
    if (key == 'Y' and not resettrigger) {
      dobreak = true;
    }
    if (possibilitys != "") {
      selection = possibilitys[keycounter];
    }
  }
  debug("Input: " + finalstring);
  return (finalstring);
}

///Smaller help functions/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String firstChars(String incomingString, int number) {
  String result = "";
  int counter = 1;
  if (incomingString.length() > number) {
    while (counter != number) {
      result += incomingString[counter - 1];
      counter++;
    }
    return (result);
  } else {
    return (incomingString);
  }
}

String iptostring(IPAddress ipAddress) {
  String strinip = String(ipAddress[0]) + "." + String(ipAddress[1]) + "." + String(ipAddress[2]) + "." + String(ipAddress[3]);
  return (strinip);
}

int bootcountread() {
  int bootbyte1 = EEPROM.read(0);
  int bootbyte2 = EEPROM.read(1);
  bootcounts = bootbyte1 + (bootbyte2 * 256);
}

void bootcount() {
  int bootbyte1 = EEPROM.read(0);
  int bootbyte2 = EEPROM.read(1);
  bootbyte1++;
  if (bootbyte1 == 256) {
    bootbyte1 = 0;
    bootbyte2++;
  }
  EEPROM.write(0, bootbyte1);
  EEPROM.write(1, bootbyte2);
  EEPROM.commit();
  bootcountread();
  debug("Bootcount:" + String(bootcounts));
}

void debug(String debugmessage) {
  if (debugging == true) {
    Serial.println("[i] " + debugmessage);
  }
}

void loop() {
}
