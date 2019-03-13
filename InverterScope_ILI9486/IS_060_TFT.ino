#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/TomThumb.h> // super small font


#define PORTRAIT 0
#define LANDSCAPE 1

// Display colours
//#define BEAM1_COLOUR ILI9341_GREEN
//#define BEAM2_COLOUR ILI9341_RED
//#define BEAM_OFF_COLOUR ILI9341_BLACK
//#define CURSOR_COLOUR ILI9341_GREEN
#define BEAM_OFF_COLOUR TFT_BLACK
#define BEAM1_COLOUR TFT_GREEN
#define BEAM2_COLOUR TFT_RED
#define CURSOR_COLOUR TFT_GREEN

#define GRATICULE_COLOUR 0x07FF

//  http://www.barth-dev.de/online/rgb565-color-picker/
#define BLUE1 0x001F
#define WHITE 0xFFFF
#define GREY1 0x6B6D
#define GREY2 0x4A69
#define GREY3 0x2965
#define YELLOW 0xFFE0

//Adafruit_ILI9341_8bit_STM TFT = Adafruit_ILI9341_8bit_STM();
MCUFRIEND_kbv TFT;

uint16_t myHeight,myWidth;

uint8 traceA[FRAME_WIDTH];
uint8 traceB[FRAME_WIDTH];
uint8 *trace;
uint8 *lastTrace = 0; 
uint16 lastVrms;

int16 dataVAC[FRAME_WIDTH];


void drawTrace()
{
  
  static uint8 cnt=0;
  
//  zeroOffset = currentOffset(samples);
//  convert2AC(samples,dataVAC);
//  Period_count =  periodCount(dataVAC);
//  Vrms_12bit = RMS(samples,&Vpp_12bit);

  stats_Sample(samples);
  
  
  if (lastTrace == 0 || lastTrace == traceB)
  {
    trace = traceB;
    lastTrace = traceA;
  }
  else
  {
    trace = traceA;
    lastTrace = traceB;
  }
  convert2Trace(samples,trace);
  TFT.setRotation(LANDSCAPE);
  for (int i=0;i < 320;i++)
  {
    if (! Persist)
    {
      if (lastTrace[i] > 0) TFT.drawPixel(i,lastTrace[i],BEAM_OFF_COLOUR);
    }
    TFT.drawPixel(i,trace[i],BEAM1_COLOUR);
  }
  //showGraticule();
  cnt ++;
  if (cnt > 16 && Vrms != lastVrms)
  {
    cnt=0;
    drawVrms();
  }
}


void TFT_setup()
{
  uint16_t ID = TFT.readID(); //
    Serial.print("ID = 0x");
    Serial.println(ID, HEX);
    if (ID == 0xD3D3) ID = 0x9481; // write-only shield
//    ID = 0x9329;                             // force ID
    TFT.begin(ID);
  
  //TFT.begin();
  clearTFT();
  TFT.setRotation(PORTRAIT);
  myHeight   = TFT.width();
  myWidth  = TFT.height();
  TFT.setTextColor(CURSOR_COLOUR, BEAM_OFF_COLOUR);
  TFT.setRotation(LANDSCAPE);
  showSplash();
  delay(3000);
  clearTFT();
  showGraticule();
  drawVoltRect();
}


void drawVrms()
{
  static uint16 lastJunk=0;
  
  TFT.setFont(&FreeMonoBold9pt7b);
  TFT.setRotation(LANDSCAPE);
  //drawVoltRect();
  TFT.setTextColor(GREY3, GREY3);
  TFT.setCursor(15, 30);
  TFT.print(lastVrms);
  TFT.setFont(&TomThumb);
  TFT.setTextSize(2);
  TFT.print(" Vrms");

  TFT.setFont(&FreeMonoBold9pt7b);
  TFT.setRotation(LANDSCAPE);
  //drawVoltRect();
  TFT.setTextColor(BEAM_OFF_COLOUR, BEAM_OFF_COLOUR);
  TFT.setCursor(135, 30);
  TFT.print(lastJunk);
  

  TFT.setRotation(LANDSCAPE);
  TFT.setFont(&FreeMonoBold9pt7b);
  TFT.setTextColor(YELLOW, GREY3);
  TFT.setCursor(15, 30);
  TFT.print(Vrms);
  TFT.setFont(&TomThumb);
  TFT.setTextSize(2);
  TFT.print(" Vrms");
  lastVrms = Vrms;

  TFT.setRotation(LANDSCAPE);
  TFT.setFont(&FreeMonoBold9pt7b);
  TFT.setTextColor(YELLOW, BEAM_OFF_COLOUR);
  TFT.setCursor(135, 30);
  TFT.print(Frequency);
  lastJunk = (uint16) Frequency;
}

void drawVoltRect()
{
  TFT.setRotation(LANDSCAPE);
  TFT.fillRoundRect(1,1,120,40,10,GREY3);
  TFT.drawRoundRect(1,1,120,40,10,WHITE);
}



/*--------------------------------------------------
 *   Chunks of code borrowed from STM-O-Scope 
 ---------------------------------------------------*/

void clearTFT()
{
  TFT.fillScreen(BEAM_OFF_COLOUR);          // Blank the display
}

void showGraticule()
{
  TFT.setRotation(PORTRAIT);     
  TFT.drawRect(0, 0, TFT_HEIGHT, TFT_WIDTH, GRATICULE_COLOUR);
  // Dot grid - ten distinct divisions (9 dots) in both X and Y axis.
  for (uint16_t TicksX = 1; TicksX < 10; TicksX++)
  {
    for (uint16_t TicksY = 1; TicksY < 10; TicksY++)
    {
      TFT.drawPixel(  TicksX * (myHeight / 10), TicksY * (myWidth / 10), GRATICULE_COLOUR);
    }
  }

  int edge_interrupt_falling = 3;   // show more dots for the falling interrupt edge, which may be used for frequency counting
  for (int TicksY = 1; TicksY < 20; TicksY++)
  {
    TFT.drawPixel(  edge_interrupt_falling * (myHeight / 10), TicksY * (myWidth / 20), GRATICULE_COLOUR);
  }

  // Horizontal and Vertical centre lines 5 ticks per grid square with a longer tick in line with our dots
  for (uint16_t TicksX = 0; TicksX < myWidth; TicksX += (myHeight / 50))
  {
    if (TicksX % (myWidth / 10) > 0 )
    {
      TFT.drawFastHLine(  (myHeight / 2) - 1, TicksX, 3, GRATICULE_COLOUR);
    }
    else
    {
      TFT.drawFastHLine(  (myHeight / 2) - 3, TicksX, 7, GRATICULE_COLOUR);
    }

  }
  for (uint16_t TicksY = 0; TicksY < myHeight; TicksY += (myHeight / 50) )
  {
    if (TicksY % (myHeight / 10) > 0 )
    {
      TFT.drawFastVLine( TicksY,  (myWidth / 2) - 1, 3, GRATICULE_COLOUR);
    }
    else
    {
      TFT.drawFastVLine( TicksY,  (myWidth / 2) - 3, 7, GRATICULE_COLOUR);
    }
  }
}

void showSplash() 
{
    TFT.setTextSize(2);                     // Small 26 char / line
    //TFT.setTextColor(CURSOR_COLOUR, BEAM_OFF_COLOUR) ;
    int YStep = 20;
    int YStart = 20;
    TFT.setCursor(0, YStart);
    TFT.print("     InverterScope");
    TFT.setCursor(0, YStart+=2*YStep);
    TFT.print("5mSec/div  50mSec Frame");
    TFT.setCursor(0, YStart+=YStep);
    TFT.print("320 samples / frame");
    TFT.setCursor(0, YStart+=2*YStep);
    TFT.print("   ");
    TFT.setCursor(0, YStart+=1.5*YStep);
    TFT.print("Max MCU Pin Voltage: 3.3V");
    TFT.setCursor(0, YStart+=YStep);
    TFT.print("");
    TFT.setCursor(0, YStart+=YStep);
    TFT.print("");
    TFT.setTextSize(2);
    TFT.setRotation(PORTRAIT);
}
