

// These are the lcd pins used for touch 
#define LCD_RD PA0

#define LCD_RS PA2
#define LCD_CS PA3
#define LCD_D0 PB8
#define LCD_D1 PB9
#define LCD_D7 PB15
#define LCD_D6 PB14

#define LCD_RD PA0
#define LCD_WR PA1

#define YP LCD_WR  // must be an analog pin
#define XM LCD_RS  // must be an analog pin
#define YM LCD_D7   // can be a digital pin
#define XP LCD_D6   // can be a digital pin


TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
