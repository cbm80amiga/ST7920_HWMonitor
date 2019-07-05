// Arduino HWMonitor - numeric info and 1 graph
// (C)2019 Pawel A. Hernik

/*
 128x64 ST7920 connections in SPI mode (only 6 wires between LCD and MCU):

 #01 GND  -> GND
 #02 VCC  -> VCC (5V)
 #04 RS   -> SCK/D13
 #05 R/W  -> MOSI/D11
 #06 E    -> CS/D10 or any pin
 #15 PSB  -> GND (for SPI mode)
 #19 BLA  -> VCC or any pin via 300ohm
 #20 BLK  -> GND
*/

/*
 Use PC HardwareSerialMonitor from:
  https://cdn.hackaday.io/files/19018813666112/HardwareSerialMonitor_v1.1.zip
*/

// comment out for regular mode
//#define GRAPH_BAR

// comment out for load graph
#define CLOCK_GRAPH

#define LCD_BACKLIGHT  9
#define LCD_CS         10

#include "ST7920_SPI.h"
#include <SPI.h>
ST7920_SPI lcd(LCD_CS);

// from PropFonts library
#include "c64enh_font.h"

String inputString = "";
char buf[30];
String cpuLoadString;
String cpuTempString;
String cpuClockString;
String ramString;
int cpuLoad=0;
int cpuClock=0;
int inp=0;

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  //analogWrite(LCD_BACKLIGHT,30);
  digitalWrite(LCD_BACKLIGHT, HIGH);
  inputString.reserve(200);
  
  SPI.begin();
  lcd.init();
  lcd.cls();
  lcd.setFont(c64enh);
  lcd.printStr(ALIGN_CENTER, 28, "Connecting ...");
  lcd.drawRect(16,20,127-16*2,63-20*2,1);
  lcd.display(0);
}

int readSerial() 
{
  while (Serial.available()) {
    char ch = (char)Serial.read();
    inputString += ch;

    /// for debug
    //lcd.fillRect(x,y,16,8,0); lcd.printChar(x,y,ch);  lcd.display(0);
    //x+=8; if(x>=128) { x=0;y+=8; }
    //if(y>=64) y=0;
  
    if(ch == '|') {  // full info chunk received
      int st,en;
      st = inputString.indexOf("CHC");  // CPU clock: "CHC1768"
      if(st>=0) {
        en = inputString.indexOf("|", st);
        cpuClockString = inputString.substring(st+3, en);
        cpuClock = cpuClockString.toInt();
        inp=3;
      } else {

        st = inputString.indexOf("R");  // used RAM: "R6.9"
        if(st>=0) {
          en = inputString.indexOf("|", st);
          ramString = inputString.substring(st+1 , en-1);
          st = ramString.indexOf(",");
          if(st>=0) ramString.setCharAt(st,'.');
          inp=2;
        }

        int cpuTempStart = inputString.indexOf("C"); // CPU temperature: "C52"
        int cpuLoadStart = inputString.indexOf("c"); // CPU load: "c18%"
        if(cpuLoadStart>=0 && cpuTempStart>=0) {
          en = inputString.indexOf("|");
          cpuTempString = inputString.substring(cpuTempStart+1, cpuLoadStart);
          cpuLoadString = inputString.substring(cpuLoadStart+1, en-1);
          cpuLoad = cpuLoadString.toInt();
          inp=1;
        }
      }
      inputString = "";
      return 1;
    }
  }
  return 0;
}

#define MIN_CLOCK 400
#define MAX_CLOCK 2900
#define NUM_VAL (32+1)
int valTab[NUM_VAL];
int i,ght=64-36;
int x=0;

void addVal()
{
  for(i=0;i<NUM_VAL-1;i++) valTab[i]=valTab[i+1];
#ifdef CLOCK_GRAPH
  if(cpuClock<400) cpuClock=400;
  if(cpuClock>2900) cpuClock=2900;
  valTab[NUM_VAL-1] = (long)(cpuClock-MIN_CLOCK)*ght/(MAX_CLOCK-MIN_CLOCK);
#else
  valTab[NUM_VAL-1] = cpuLoad*ght/100;
#endif
}

void drawGraph()
{
  lcd.drawRectD(0,36,128,ght,1);
  for(i=0;i<NUM_VAL-1;i++) {
    int dy = valTab[i+1]-valTab[i];
    for(int j=0;j<4;j++) lcd.drawLineVfastD(i*4+j,63-(valTab[i]+dy*j/4),63,1);
    lcd.drawLine(i*4,63-valTab[i],(i+1)*4,63-valTab[i+1],1);
  }
}

void drawGraphBar()
{
  lcd.drawRectD(0,36,128,ght,1);
  for(i=1;i<NUM_VAL;i++) {
    lcd.fillRectD((i-1)*4,63-valTab[i],4,valTab[i],1);
    lcd.drawLineH((i-1)*4,(i+0)*4-1,63-valTab[i],1);
    if(i>1) lcd.drawLineV((i-1)*4,63-valTab[i-1],63-valTab[i],1);
  }
}

void loop() 
{
  if(readSerial()) 
  {
    int xs=68;
    lcd.cls();
    lcd.printStr(0, 0, "CPU Temp: ");
    x=lcd.printStr(xs, 0, (char*)cpuTempString.c_str());
    lcd.printStr(x, 0, "'C");
    lcd.printStr(0, 9*1, "CPU Load: ");
    snprintf(buf,20,"%d %",cpuLoad);
    x=lcd.printStr(xs, 9*1, buf);
    lcd.printStr(x, 9*1, "%");
    lcd.printStr(0, 9*2, "CPU Clock: ");
    x=lcd.printStr(xs, 9*2, (char*)cpuClockString.c_str());
    lcd.printStr(x, 9*2, " MHz");
    lcd.printStr(0, 9*3, "Used RAM: ");
    x=lcd.printStr(xs, 9*3, (char*)ramString.c_str());
    lcd.printStr(x, 9*3, " GB");
#ifdef CLOCK_GRAPH
    i = 2;
#else
    i = 1;
#endif
    lcd.drawLineVfast(127-0,9*i+1,9*i+1+4,1);
    lcd.drawLineVfast(127-1,9*i+2,9*i+2+2,1);
    lcd.drawLineVfast(127-2,9*i+3,9*i+3+0,1);
  
    if(inp==3) addVal();
#ifdef GRAPH_BAR
    drawGraphBar();
#else
    drawGraph();
#endif
    lcd.display(0);
  }
  if(inp>=3) { delay(1000); inp=0; }
}

