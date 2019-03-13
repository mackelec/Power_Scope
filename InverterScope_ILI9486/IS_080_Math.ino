

/*-----------------------------------------
 * 
 *  Convert the 12bit adc value to 
 *  -   a LCD height (240) 
 *  -   Invert the signal
 *  -   (0,0 is top left)
 *  
 -----------------------------------------*/


void convert2Trace(uint16 _sample[],uint8 _trace[])
{
  int z=0;
  for (int i=0;i < FRAME_WIDTH;i++)
  {
    uint16 y;
    uint32 k;
    int a = (int) _sample[i] - 2048;
    z += a;
    k = (uint32)(( _sample[i]-zeroOffset) * 1000);
    k = (uint32) k / 4096;
    k = (uint32) k * TFT_HEIGHT;
    y = (uint16) ((k + 500) / 1000);
    y = (uint16) TFT_HEIGHT - y;
    if (y < 1) y=1;
    if (y>TFT_HEIGHT-2) y= TFT_HEIGHT - 1;
    _trace[i] = y;
    _sample[i] = 0;
  }
}


void stats_Sample(uint16 _sample[])
{
  gpio_write_bit(GPIOA, 5, HIGH);
  zeroOffset = currentOffset(_sample);
  convert2AC(_sample,dataVAC);
  Period_count =  periodCount(dataVAC);
  Vrms_12bit = RMS(dataVAC,&Vpp_12bit);

  Period_us = (uint32) ((FRAME_PERIOD*100)/FRAME_WIDTH);
  Period_us = (uint32) (Period_us * Period_count + 50);
  Period_us = (uint32) (Period_us / 100);
  int x = Vrms_12bit * 38 + 50;
  Vrms = (uint16)( x / 100);
  Frequency = (uint16) (10000000u / Period_us);
  gpio_write_bit(GPIOA, 5, LOW);
}


/*----------------------------------------
 *  Returns the vertical offset 
 *  
 *  Based on 12bit adc data array.
 *  2048 is the zero value.
 *  Careful to calculate over full cycles ONLY
 -----------------------------------------*/

int currentOffset(uint16 _sample[])
{
  int z=0;
  // 256 is 40mSec of the 50mec window
  // it need whole cycles
  for (int i=0;i < 256;i++)
  {
    int a =  (int) _sample[i] - 2048;
    z += a;
  }
  return (int16) (z/256);
}


/*----------------------------------------------------
 *  Converts the 12bit adc value into +/- 11 bit value
 *  -   Adjust for the Zero Offset.
 *  -   The array AC must be pre-defined.
 *      AC array must be at least the size of _sample
 -----------------------------------------------------*/

void convert2AC(uint16 _sample[],int16 *AC)
{
  for (int i=0;i < FRAME_WIDTH;i++)
  {
    AC[i] = (int) _sample[i] - 2048 ;
    AC[i] -=  zeroOffset;
  }
}


/*----------------------------------------------------
 *  Returns WaveForm Period in terms of sample count
 *  -   Requires an AC data array.
 -----------------------------------------------------*/

uint16 periodCount(int16 AC[])
{
  uint16 upCrosses[10];
  uint16 dnCrosses[10];
  int16 Crosses[20];
  uint8 p=0,up=0,dp=0;
  int8 cross = 0;
  int16 lastCross = 0;
  uint16 totalPeriod=0,upPeriods=0,dnPeriods=0;
  
  for (int i=1;i < FRAME_WIDTH;i++)
  {
    cross=0;
    if (AC[i-1] <= 0 && AC[i] > 0) cross = 1;
    if (AC[i-1] >= 0 && AC[i] < 0) cross = -1;
    if (cross==0) continue;
    if (p != 0 && i - lastCross < (FRAME_WIDTH/20))
    {
      cross = 0;
      continue; 
    }
    Crosses[p] = (int16) (i * cross);
    lastCross = (int16) (i * cross);
    p++;
  }
  
  if (p<2) return 0;
  
  for (int i =1;i<p;i++)
  {
    if (Crosses[i-1]*Crosses[i] >= 0) return 0;
    //if (Crosses[i-2]*Crosses[i] <= 0) return 0;
    uint _period = abs(Crosses[i])-abs(Crosses[i-1]);
    if (Crosses[i]>0)
    {
      dnPeriods += _period;
      dp++;
    }
    else
    {
      upPeriods += _period;
      up++;
    }
//    Serial << "PER:" << i << "," << Crosses[i-1] << "," << Crosses[i] << "=" << _period << endl;
//    delay(20);
  }
  uint32 tdp = (uint32) ((dnPeriods*10u)/dp);
  uint32 tup = (uint32) ((upPeriods*10u)/up);
  uint32 pp = tdp + tup + 5u;
  pp = (uint32) (pp/10);
  return ((uint16) pp);
}






/*----------------------------------------
 *  Returns the RMS Voltage in 12bit scale 
 *  
 *  Based on 12bit AC data array.
 *  Careful to calculate over full cycles ONLY
 -----------------------------------------*/

uint16 RMS(int16 AC[],uint16 *vpeak)
{
  uint32_t z=0;
  uint16 c =0;
  uint16 maxCount;
    // it need whole cycles
  maxCount = (uint16) (FRAME_WIDTH / Period_count);
  maxCount *= Period_count;
  
  for (int i=0;i < maxCount;i++)
  {
    int a = AC[i];
    uint16 k = abs(a);
    if (k > c) c= (uint16) k;
    uint32_t b = (uint32_t) a * a;
    z += (uint32_t) b;
  }
  uint32_t x = (uint32_t) (z/256);
  uint16_t vrms = asqrt(x);
  *vpeak = c;
  return vrms;
}


int RMS_biased(uint16 _sample[],uint16 *vpeak)
{
  uint32_t z=0;
  uint16 c =0;
  // 256 is 40mSec of the 50mec window
  // it need whole cycles
  for (int i=0;i < 256;i++)
  {
    int a =  (int) _sample[i] - 2048 ;
    a -= - zeroOffset;
    uint16 k = abs(a);
    if (k > c) c= (uint16) k;
    uint32_t b = (uint32_t) a * a;
    z += (uint32_t) b;
  }
  uint32_t x = (uint32_t) (z/256);
  uint16_t vrms = asqrt(x);
  *vpeak = c;
  return vrms;
}

/*--------------------------------------------------
 *   Fast Interger Square Root
 *   http://www.stm32duino.com/viewtopic.php?f=18&t=56
 ---------------------------------------------------*/


uint16_t asqrt(uint32_t x) {
  /*      From http://medialab.freaknet.org/martin/src/sqrt/sqrt.c
   *  Logically, these are unsigned. We need the sign bit to test
   *  whether (op - res - one) underflowed.
   */
  int32_t op, res, one;

  op = x;
  res = 0;

  /* "one" starts at the highest power of four <= than the argument. */

  one = 1 << 30;  /* second-to-top bit set */
  while (one > op) one >>= 2;

  while (one != 0) {
    if (op >= res + one) {
      op = op - (res + one);
      res = res +  2 * one;
    }
    res /= 2;
    one /= 4;
  }
  return (uint16_t) (res);
}
