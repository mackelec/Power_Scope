/*-------------------------------------------------------------------------------------------------------
 * 
 *   (c) Andrew McKinnon - 2019
 *    
 *    InverterScope - aka Power-Scope   released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 * 
 * 
 --------------------------------------------------------------------------------------------------------*/
#include <SPI.h>  
#include <STM32ADC.h>
#include <Streaming.h>
//#include <Adafruit_GFX.h>
#include "Adafruit_GFX.h"
#include <gfxfont.h>
#include <TouchScreen_STM.h>
//#include "Adafruit_ILI9341_8bit_STM.h" //modified lib for 8-bit parallel displays
#include <MCUFRIEND_kbv.h>

#define LCD_RD PA0
#define LCD_WR PA1
#define LCD_CD PA2
#define LCD_CS PA3
#define LCD_RESET PA8


HardwareTimer adcTimer(4);
STM32ADC adcV(ADC1);
STM32ADC adcI(ADC2);


/*----------------------------
 *    Size of LCD in Landscape
 -----------------------------*/
#define TFT_WIDTH  320
#define TFT_HEIGHT 240

/*----------------------------
 *    Size of Scope (graphing)
 *    -   Must be same / smaller
 *        than TFT_ parameters
 -----------------------------*/

#define FRAME_HEIGHT TFT_HEIGHT
#define FRAME_WIDTH TFT_WIDTH

/*----------------------------
 *  Frame_Period : 
 *  - time in microseconds
  ----------------------------*/

#define FRAME_PERIOD 50000

/*-----------------------------------------
 *    Persist, if True
 *    - last trace will not be erased on LCD
 -----------------------------------------*/
bool Persist = true;

#define testPin1 PA4
#define testPin2 PA5
#define testPin3 PA6

#define LED      PC13

uint8 adcPinV = PB0;
uint8 adcPinI = PB1;

bool testpinFlag;
//bool triggerMode = true;
volatile bool readyForTrigger = false;
volatile bool triggered = false;



volatile uint16 adcValueV=0;
volatile uint16 adcValueI=0;
volatile bool adcV_dataPresent;
volatile bool adcI_dataPresent;

uint16 adcPtr;
uint16 samplesA[FRAME_WIDTH];
uint16 samplesB[FRAME_WIDTH];
uint16 IsamplesA[FRAME_WIDTH];
uint16 IsamplesB[FRAME_WIDTH];
uint16 *samples;
uint16 *adcSamples;

uint16 zeroOffset = 0;
uint16 Vrms = 0;
uint16 Vrms_12bit = 0;
uint16 Vpp_12bit = 0;


uint16 Period_count = 0;   
uint32 Period_us = 0;          //  microseconds
uint16 Frequency = 0;


void setup() {
  //Serial.begin(115200);
  Serial.begin(250000);
  pinMode(testPin1,OUTPUT);
  pinMode(testPin2,OUTPUT);
  pinMode(testPin3,OUTPUT);
  pinMode(LED,OUTPUT);
  digitalWrite(LED,LOW);
  TFT_setup();
  samples = samplesA;
  adcSamples = samples;

  //*******  ADC setup

  adcV.calibrate();
  adcV.setSampleRate(ADC_SMPR_239_5);
  adcV.attachInterrupt(adcConvertedInt, ADC_EOC);
  adcV.setPins(&adcPinV, 1);

  adcI.calibrate();
  adcI.setSampleRate(ADC_SMPR_239_5);
  //adcI.attachInterrupt(int_ADCI,, ADC2_EOC);
  adcI.setPins(&adcPinI, 1);

    // -- sets up the adc control registers
    //    for Dual ADC, simultaneous sample
  ADC1->regs->CR1 |= 0x60000;
  ADC1->regs->CR2 |= 0x100;

  //*******  Timer setup  (156.25 microseconds, 6400Hz)
  //---
  //---  works out perfect for 50Hz
  //---  Frame = 50mS, 2.5 full cycles
  //---  Sample rate = 50ms / 320 (lcd width or frame)
  //---  equals 156.25 uSec - tada
  
  adcTimer.setMode(TIMER_CH4, TIMER_OUTPUT_COMPARE);
  adcTimer.pause();
  adcTimer.setPrescaleFactor(1);
  adcTimer.setCount(0);
  adcTimer.setOverflow(11250);
  adcTimer.setCompare(TIMER_CH4, 1);  
  adcTimer.attachCompare4Interrupt(timer_Handler);
  adcTimer.refresh();

  //********
  
  adcTimer.resume();
  //Serial << "TX buffer = " << SERIAL_TX_BUF_SIZE << endl;
  Serial << "Setup done" << endl;
  Serial.flush();
  digitalWrite(LED,HIGH);
}




void loop() 
{
 
}


void proc_Samples()
{
  if (adcPtr > 319 && adcPtr < 321)
  {
    gpio_write_bit(GPIOA, 6, HIGH);
    triggered = false;
    adcTimer.pause();
    adcPtr = 333;
    readyForTrigger = false;
    drawTrace();
    adcPtr = 0;
    readyForTrigger = true;
    gpio_write_bit(GPIOA, 6, LOW);
    adcTimer.resume();
  }
}

/*---------------------------------------------------------
 * 
 *    ADC callback routine
 *    -  Is called when conversion is finished
 * 
 ---------------------------------------------------------*/

void adcConvertedInt()
{
  static uint16 lastADC = 0;
  gpio_write_bit(GPIOA, 4, LOW);
  //-- read adc 32bit data register
  uint32 data = ADC1->regs->DR;
  adcValueV = data & 0xFFFF;
  adcValueI = data >> 16;

  //------------  If Frame full 
  
  if (adcPtr > 319) 
  {
    if (triggered)
    {
      //gpio_write_bit(GPIOA, 5, LOW);
      if (adcSamples == samplesA)
      {
        samples = samplesA;
        adcSamples = samplesB;
      }
      else
      {
        samples = samplesB;
        adcSamples = samplesA;
      }
      proc_Samples();
    }
    triggered = false;
    
    lastADC = 0;
    return;
  }

  //------------  Saving Frame data
  
  if (triggered)
  {
    //gpio_write_bit(GPIOA, 5, HIGH);
    adcSamples[adcPtr] = adcValueV;
    adcPtr ++;
  }

  //------------  Looking for Trigger
  
  else
  {
    if (lastADC == 0) 
    {
      lastADC = adcValueV;
      return;
    }

    int y1 = (int) (adcValueV - 2048);
    int y2 = (int) (lastADC - 2048);
//    y1 -= (int) zeroOffset;
//    y2 -= (int) zeroOffset;
    lastADC = adcValueV;
    if (y2 < 0 && y1 >= 0)
    {
      triggered = true;
    }
  }
}

/*------------------------------------------------------------
 *    Timer handler - fires every 156.25uSec
 *    -   Starts ADC conversion.
 *        Only needs to Start ADC1 conversion.
 *        As Dual mode is set, ADC2 conversion is started by ADC1.
 *    -   Sets testPin1 HIGH, 
 *        testPin1 is cleared LOW when the ADC callBack Occurs.   
 *        Use a Scope to check timing is good and no over-runs
 -------------------------------------------------------------*/

void timer_Handler()
{
  //----  turn testPin1 high
  gpio_write_bit(GPIOA, 4, HIGH);
  adcV.startConversion();
}
