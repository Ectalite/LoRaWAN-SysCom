#include <LoRaWan.h>

void LoraInit(void);
void sendLoraPacket(String);

char buffer[256];

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);
  SerialUSB.begin(115200);

  LoraInit();
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(3000);              // wait for a second
  sendLoraPacket("Hello World!");
  SerialUSB.println("Hello Xavier!");
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
}

void LoraInit()
{
  lora.init();

  memset(buffer, 0, 256);
  lora.getVersion(buffer, 256, 1);
  SerialUSB.print(buffer); 
  
  memset(buffer, 0, 256);
  lora.getId(buffer, 256, 1);
  SerialUSB.print(buffer);
  
  lora.setKey("79E25FD06BFD693C6002B0D0FAC2855C", "79E25FD06BFD693C6002B0D0FAC2855C", "79E25FD06BFD693C6002B0D0FAC2855C");
  
  lora.setDeciveMode(LWOTAA);
  lora.setDataRate(DR5, EU868);
  
  lora.setAdaptiveDataRate(true); 

  lora.setChannel(0, 868.1);
  lora.setChannel(1, 868.3);
  lora.setChannel(2, 868.5);
  lora.setChannel(3, 867.1);
  lora.setChannel(4, 867.3);
  lora.setChannel(5, 867.5);
  lora.setChannel(6, 867.7);
  
  lora.setReceiceWindowFirst(1);
  lora.setReceiceWindowSecond(869.5, DR3);
  
  lora.setDutyCycle(false);
  lora.setJoinDutyCycle(false);
  
  lora.setPower(14);
  while(!lora.setOTAAJoin(JOIN));
}

void sendLoraPacket(String text)
{
  text.toCharArray(buffer, 256);
  bool result = lora.transferPacket(buffer, 10);
  
  if(result)
  {
      short length;
      short rssi;
      
      memset(buffer, 0, 256);
      length = lora.receivePacket(buffer, 256, &rssi);
      
      if(length)
      {
          SerialUSB.print("Length is: ");
          SerialUSB.println(length);
          SerialUSB.print("RSSI is: ");
          SerialUSB.println(rssi);
          SerialUSB.print("Data is: ");
          for(unsigned char i = 0; i < length; i ++)
          {
              SerialUSB.print("0x");
              SerialUSB.print(buffer[i], HEX);
              SerialUSB.print(" ");
          }
          SerialUSB.println();
      }
  }
}