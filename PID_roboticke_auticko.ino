#include "Shieldbot.h"
#include <aJSON.h>
#include <stdio.h>
#include <math.h>

char buff[255];
char charByte;
int index = 0;
float x = 0.0;
float y = 0.0;
float ax = 0.0;
float ay = 0.0;
float dx = 0.0;
float dy = 0.0;
float ay2 = 0.0;
float ad = 0.0;
float povodna = 20; 
float PI_regulator, prave_koleso, lave_koleso, chyba,cas,predosly_cas,delta_chyby,delta_casu,derivacia,integral;
float predosla_chyba = 0 ;
float P = 0.0;
float I = 0.0;
float D = 0.0;
// konstanty regulatora 
float p_konstanta = 0.5; 
float i_konstanta = 0.015;
float d_konstanta = 0.1;


bool onSerial;
Shieldbot shieldbot = Shieldbot();

// Some utils
#define GET_SIZE(x)  (sizeof(x) / sizeof(x[0]))

void setup() {
  pinMode(13, OUTPUT);
  for(int i=0;i<65;i++)
    {
    digitalWrite(13,HIGH);  
    delay(500);
    digitalWrite(13,LOW);
    delay(500);
    }
  Serial1.begin(115200); // Set the baud.
  shieldbot.setMaxSpeed(255);
  Serial.begin(115200);
  while (!Serial1)
  {
  }
  cas = millis();
}
int prepocet_uhlaAY (float vstupAY,float vstupAX) 
    {
      float vystupAY2;
      if ((vstupAY > 0) && (abs(vstupAX) < 50 )) { vystupAY2 = vstupAY ;}
      else if (abs(vstupAX) >= 50 ) { vystupAY2 = 180 - vstupAY ;} 
      else if ((vstupAY < 0) && (abs(vstupAX) < 50  )) { vystupAY2 = 360 + vstupAY ;}
      return vystupAY2;   
    }

int prepocet_uhlaAD(float DX,float DY,float X,float Y) 
  {
    float vystupAD;
    if ((DX > X) && (DY >= Y)) { vystupAD = atan(abs(DY-Y)/abs(DX-X))*180/M_PI + 270; }
    else if ((DX <= X) && (DY > Y)) { vystupAD = atan(abs(DY-Y)/abs(DX-X))*180/M_PI + 180;}
    else if  ((DX < X) && (DY <= Y)) { vystupAD = atan(abs(DY-Y)/abs(DX-X))*180/M_PI + 90;}
    else if ((DX >= X) && (DY < Y)) { vystupAD = atan(abs(DY-Y)/abs(DX-X))*180/M_PI ; }
    return vystupAD;   
  }
  
void loop()
 {    
index = 0;
onSerial = false;
while(Serial1.available())
  {  
  if(index<GET_SIZE(buff))
    {  
    charByte = Serial1.read();
    buff[index] = charByte;
    index++;
    delayMicroseconds(100);
    }
  onSerial = true;  
  }
  
if(onSerial)
  {  
  aJsonObject* jsonRoot = aJson.parse(buff); 
  if(jsonRoot!=NULL)
    {
    aJsonObject* itemX = aJson.getObjectItem(jsonRoot, "x");
    if(itemX!=NULL) 
      {
      x = itemX->valuefloat;
      }
    aJsonObject* itemY = aJson.getObjectItem(jsonRoot, "y");
    if(itemY!=NULL) 
      {
      y = itemY->valuefloat;
      }  
    aJsonObject* itemAngleX = aJson.getObjectItem(jsonRoot, "ax");
    if(itemAngleX!=NULL) 
      {
      ax = itemAngleX->valuefloat;
      }  
    aJsonObject* itemAngleY = aJson.getObjectItem(jsonRoot, "ay");
    if(itemAngleY!=NULL) 
      {
      ay = itemAngleY->valuefloat;
      }
    aJsonObject* itemDestX = aJson.getObjectItem(jsonRoot, "dx");  
    if(itemDestX!=NULL) 
      {
      dx = itemDestX->valuefloat;    
      }    
    aJsonObject* itemDestY = aJson.getObjectItem(jsonRoot, "dy");  
    if(itemDestY!=NULL) 
      {
      dy = itemDestY->valuefloat;    
      }   
    }

  aJson.deleteItem(jsonRoot);
  memset(buff,0, sizeof(buff));
  
  ay2 = prepocet_uhlaAY(ay,ax);
  ad = prepocet_uhlaAD(dx,dy,x,y);
  
  if ((x < dx+5) && (x > dx-5) && (y < dy+5) && (y > dy-5)){
        shieldbot.drive(0,0);
   }
   else{
      // Delta uhla _ pozadovay uhol / skutocny uhol 
      if (ad < 180){
        if ((ay2 > ad) && (ad+180 > ay2)){
            chyba = ay2 - ad;
          }
        else if((ad+180 < ay2) || (ay2 < ad)){
            if (ay2 > ad){
              chyba = 360 - ay2 + ad;
              }
            else{
              chyba = ad - ay2;
              }
          }
        }
      else if(ad > 180){
          if ((ay2 < ad) && (ad-180 < ay2)){
              chyba = ad - ay2;
            }
          else if((ay2 > ad) || (ad-180 > ay2)){
              if (ay2 < ad){
                  chyba = 360 + ay2 - ad;
                }
              else{
                  chyba = ay2 -ad;
                }
            }
        }
      // Regulator
      // P cast regulatora
      P = p_konstanta * chyba;
      
      // I cast regulatora reaguje iba ked sme velmi blyzko pozadovaneho uhla a chybu dorovna na nulu
    if ((chyba < 5)){
        integral += i_konstanta * (chyba+predosla_chyba)/2;
        I = integral;
       }
    else{
        integral = 0;
        I = 0;
        }
    predosla_chyba = chyba;
       // D cast regulatora
        cas = millis(); 
        delta_casu = (cas - predosly_cas)/1000  ; 
        delta_chyby = chyba - predosla_chyba ;
        derivacia = delta_chyby/delta_casu;
        D = d_konstanta * derivacia;
        predosly_cas = cas;
        
        //regulator P + I + D 
        PI_regulator = P + I ;
      
        // kolesa
      if (ad < 180){
        if ((ay2 > ad) && (ad+180 > ay2)){
          prave_koleso = povodna + PI_regulator;
          lave_koleso = povodna - 0.5*PI_regulator ;
          Serial.println("prave_koleso");
          }
        else if((ad+180 < ay2) || (ay2 < ad)){
          prave_koleso = povodna - 0.5*PI_regulator ;
          lave_koleso = povodna + PI_regulator;
          Serial.println("lave_koleso");
          }
        }
      else if(ad > 180){
          if ((ay2 < ad) && (ad-180 < ay2)){
            prave_koleso = povodna - 0.5*PI_regulator ;
            lave_koleso = povodna + PI_regulator;
            Serial.println("lave_koleso");
            }
          else if((ay2 > ad) || (ad-180 > ay2)){
            prave_koleso = povodna + PI_regulator;
            lave_koleso = povodna - 0.5*PI_regulator ;
            Serial.println("prave_koleso");
            }
        }
      
        shieldbot.drive(prave_koleso,lave_koleso);
   }
    onSerial = false;
    }     
}
