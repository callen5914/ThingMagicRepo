#include "SparkFun_UHF_RFID_Reader.h"
#include <SoftwareSerial.h>

SoftwareSerial rfSerial(2,3);
SoftwareSerial ptSerial(8,9);

RFID rfid;

void setup()
{
  Serial.begin(115200);
  ptSerial.begin(115200);

  Serial.println("Initializing...");
  ptSerial.println("Initializing...");

  if (setupNano(38400) == false)
  {
    Serial.println("Check Wiring");
    ptSerial.println("Check Wiring");
    while (1);
  }

  rfid.setRegion(REGION_NORTHAMERICA);
  rfid.setReadPower(500); // 5.00 dBm

  Serial.println("Starting Scan...");
  ptSerial.println("Starting Scan");

  rfid.startReading();
}

void loop()
{
  if (rfid.check() == true)
  {
    byte responseType = rfid.parseResponse();

    if (responseType == RESPONSE_IS_KEEPALIVE)
    {
      Serial.println(F("Scanning"));
      //ptSerial.println(F("Scanning"));
    }
    else if (responseType == RESPONSE_IS_TAGFOUND)
    {
      //If we have a full record we can pull out the fun bits
      int rssi = rfid.getTagRSSI(); //Get the RSSI for this tag read
      long freq = rfid.getTagFreq(); //Get the frequency this tag was detected at
      long timeStamp = rfid.getTagTimestamp(); //Get the time this was read, (ms) since last keep-alive message
      byte tagEPCBytes = rfid.getTagEPCBytes(); //Get the number of bytes of EPC from response

      Serial.print(F(" rssi["));
      Serial.print(rssi);
      Serial.print(F("]"));

      Serial.print(F(" freq["));
      Serial.print(freq);
      Serial.print(F("]"));

      Serial.print(F(" time["));
      Serial.print(timeStamp);
      Serial.print(F("]"));

//      ptSerial.print(F("RSSI: "));
//      ptSerial.print(rssi);
//      ptSerial.print(F(", "));
//
//      ptSerial.print(F("Freq: "));
//      ptSerial.print(freq);
//      ptSerial.print(F(", "));

      //Print EPC bytes, this is a subsection of bytes from the response/msg array
      Serial.print(F("Tag: "));
      for (byte x = 0 ; x < tagEPCBytes ; x++)
      {
        if (rfid.msg[31 + x] < 0x10) Serial.print(F("0"));//Pretty print
        Serial.print((char) rfid.msg[31 + x]);
        ptSerial.print((char) rfid.msg[31 + x]);
      }
      Serial.print(F(" "));
      Serial.println();
      ptSerial.println();
    }
    else if (responseType == ERROR_CORRUPT_RESPONSE)
    {
      Serial.println(F("Bad CRC"));
      ptSerial.println(F("Bad CRC"));
    }
    else
    {
      Serial.println(F("Unknown Error"));
      ptSerial.println(F("Unknown Error"));
    }
  }
}

boolean setupNano(long baudRate)
{
  rfid.begin(rfSerial);
  rfSerial.begin(baudRate);

  while (!rfSerial);
  while (rfSerial.available()) rfSerial.read();

  rfid.getVersion();

  if (rfid.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    rfid.stopReading();
    delay(1500);
  }
  else
  {
    rfSerial.begin(115200);
    rfid.setBaud(baudRate);
    rfSerial.begin(baudRate);
  }

  rfid.getVersion();

  if (rfid.msg[0] != ALL_GOOD) return (false);
  rfid.setTagProtocol();
  rfid.setAntennaPort();
  return true;
}
