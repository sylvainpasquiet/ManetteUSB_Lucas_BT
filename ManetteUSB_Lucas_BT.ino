
// Satisfy the IDE, which needs to see the include statment in the ino too.
//#ifdef dobogusinclude
//  #include <spi4teensy3.h>
//#endif

#include <SPI.h>
#include <PS3BT.h>
#include <legopowerfunctions.h>
#include <EEPROM.h>

#define PinIR 8
int SER_Pin = 7;   //pin 14 on the 75HC595
int RCLK_Pin = 5;  //pin 12 on the 75HC595
int SRCLK_Pin = 4; //pin 11 on the 75HC595
int PinLed1 = 4;
int PinLed2 = 5;
int PinLed3 = 6;
int PinLed4 = 7;
char Leds=0;
char Giro=0;

void SerialData(char data)
{
    digitalWrite(RCLK_Pin, LOW);
    unsigned int localdata=data;
    shiftOut(SER_Pin, SRCLK_Pin, MSBFIRST, (localdata>>8));  
    shiftOut(SER_Pin, SRCLK_Pin, MSBFIRST, (localdata & 0xFF));  
    digitalWrite(RCLK_Pin, HIGH);
}

LEGOPowerFunctions lego(PinIR);

USB Usb;
BTD Btd(&Usb); 
//PS3BT PS3(&Btd); 
PS3BT PS3(&Btd, 0x00, 0x15, 0x83, 0x3D, 0x0A, 0x57); // This will also store the bluetooth address - this can be obtained from the dongle when running the sketch



bool Boot;
bool Marche;
char MiseAJour=0;
char Rouge=0;
char Bleu=0;
char NewRouge=0;
char NewBleu=0;
char Canal;
char Led[4];
char Vibration=0;
unsigned long VibrationTime=0;
unsigned long oldtime=0;
unsigned long oldtimeGiro=0;
unsigned long oldtimeCligno=0;
unsigned long oldtimePWM=0;
unsigned long oldtimeStop=0;

void SetVibration()
{
  PS3.setRumbleOn(RumbleHigh);
  Vibration=1;        
  VibrationTime=millis(); 
}


void setup() 
{
  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);
  pinMode(PinLed1, OUTPUT);
  pinMode(PinLed2, OUTPUT);
  pinMode(PinLed3, OUTPUT);
  pinMode(PinLed4, OUTPUT);  
  
  Serial.begin(115200);
  
  if (Usb.Init() == -1) 
  {
    Serial.print(F("\r\nOSC did not start"));
    while (1); //halt
  }
  Serial.print(F("\r\nPS3 Bluetooth Library Started"));
  
  Canal = EEPROM.read(0);
  if ((Canal!=CH1)&&(Canal!=CH2)&&(Canal!=CH3)&&(Canal!=CH4))
  {
    Canal = CH1;
    EEPROM.write(0, Canal);
  }
}

void loop() {
  Usb.Task();
  if (PS3.PS3Connected || PS3.PS3NavigationConnected) 
  {
    if (PS3.getButtonClick(SELECT)) 
    {
      Canal = (Canal+1)%4;
      EEPROM.write(0, Canal);
      Boot=0;
      SetVibration();
    }

    // Si on appui sur un des symboles, on change les LED
    if (Canal!=CH1) 
    {
      if (PS3.getButtonClick(TRIANGLE)) { Leds=(Leds^0x01); SetVibration();}
      if (PS3.getButtonClick(CIRCLE)) { Leds=(Leds^0x02); SetVibration();}
      if (PS3.getButtonClick(CROSS)) { Leds=(Leds^0x04); SetVibration();}
      if (PS3.getButtonClick(SQUARE)) { Leds=(Leds^0x08); SetVibration();}
      if (Marche)
      {
        if (Leds&0x01) digitalWrite(PinLed1, HIGH);
        else digitalWrite(PinLed1, LOW);
        if (Leds&0x02) digitalWrite(PinLed2, HIGH);
        else digitalWrite(PinLed2, LOW);
        if (Leds&0x04) digitalWrite(PinLed3, HIGH);
        else digitalWrite(PinLed3, LOW);
        if (Leds&0x08) digitalWrite(PinLed4, HIGH);
        else digitalWrite(PinLed4, LOW);
      }  
    }
    else
    {
      if (Marche)
      {
        Leds=Leds&0x01;
        if (PS3.getButtonClick(CIRCLE)) { Leds=(Leds^0x01); SetVibration();}
        if (PS3.getButtonClick(TRIANGLE)) { Giro=(Giro^0x01); SetVibration();}
        
        if (Giro)
        {
          if (millis()-oldtimeGiro>250)
          {
            Leds=Leds|0x04;
          }  
          else
          {
            Leds=Leds|0x08;
          }
        }
        else
        {
          oldtimeGiro=millis();
        }
        if ((Rouge==PWM_FWD7)||(Rouge==PWM_FWD6)||(Bleu>8))
        {
          if (millis()-oldtimeCligno>500)
          {
            Leds=Leds|0x10;
          } 
        } 
        if ((Rouge==PWM_REV7)||(Rouge==PWM_REV6)||(Bleu>8))
        {
          if (millis()-oldtimeCligno>500)
          {
            Leds=Leds|0x20;
          }
        }
        if (millis()-oldtimeStop<1000)
        {
          analogWrite(PinLed3, 255);
        } 
        else
        {
          if (Leds&0x01)
          {
            analogWrite(PinLed3, 30); 
          }  
          else
          {
            analogWrite(PinLed3, 0); 
          }                  
        }
      }
      else
      {
        Leds=0x00;
      }
      SerialData(Leds);
    }
    if (millis()-oldtimeGiro>500)
    {
      oldtimeGiro = millis();
    }
    
    if (millis()-oldtimeCligno>1000)
    {
      oldtimeCligno = millis();
    }
    
    if (millis()-oldtimePWM>50)
    {
      oldtimePWM = millis();
    }
    
    if (Vibration==1)
    {
      if (millis()-VibrationTime>300)
      {
        Vibration=0;
        PS3.setRumbleOff();
      }
    }

    // Pour faire le marche ou l'arret
    if (PS3.getButtonClick(START)) 
    {
      Marche=!Marche;
      Boot=0;
    }
    
    // Au demarrage, on eteint toutes les LED et on allume la LED suivant le canal choisi plus haut
    if (!Boot)
    {
      Boot=1; 
      PS3.setLedOff(); 
      if (Marche)
      {
        switch(Canal)
        {
          case CH1: PS3.setLedOn(LED1); break;
          case CH2: PS3.setLedOn(LED2); break;
          case CH3: PS3.setLedOn(LED3); break;
          case CH4: PS3.setLedOn(LED4); break;
        }
      }
    }

    if (Canal==CH1) 
    {
      NewRouge=(255-PS3.getAnalogHat(LeftHatX))/18;
    }
    else          
    {
      NewRouge=(PS3.getAnalogHat(LeftHatY))/18;
    }
    NewBleu=(255-PS3.getAnalogHat(RightHatY))/18;

    switch(NewRouge)
    {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:NewRouge=9+NewRouge;break;
      case 7:NewRouge=PWM_FLT;break;
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:NewRouge=NewRouge-7;break;
    }
    switch(NewBleu)
    {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:NewBleu=9+NewBleu;break;
      case 7:NewBleu=PWM_FLT;break;
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:NewBleu=NewBleu-7;break;
    }

    if (Marche) 
    {     
      //Serial.println("Marche");
      // On gère le bleu
      if (NewBleu!=Bleu) 
      { 
        Bleu=NewBleu; 
        MiseAJour=1;
        if (Bleu==PWM_FLT) oldtimeStop = millis();
      }
      // On gère le rouge
      if (NewRouge!=Rouge) { Rouge=NewRouge; MiseAJour=1;}
    }
    else
     {  
      //Serial.println("Arret");   
      if (Rouge!=PWM_FLT) {Rouge=PWM_FLT; MiseAJour=1;}
      if (Bleu!=PWM_FLT) {Bleu=PWM_FLT; MiseAJour=1;}
    }   
  }

  // on force une nouvelle commande si cela fait trop longtemps sans commande
  if (millis()-oldtime>1000)
  {
    MiseAJour=1;
  }
  // on a demande au dessus de changer quelque chose, alors on envoie la nouvelle commande Power function
  if (MiseAJour==1)
  {  
    oldtime = millis();
    MiseAJour=0;
    lego.ComboPWM(Bleu, Rouge, Canal);
  }   
}
