/* testYMZ284 --- simple sketch to drive the YMZ284 sound chip      2017-01-28 */
/* Copyright (c) 2017 John Honniball                                           */

#define CLK_PIN   (3)
#define BDIR_PIN  (4)
#define BC1_PIN   (5)
#define D0_PIN    (6)

#define D2  38

#define E2  40

#define C3  48

#define F3  53
#define F3s 54
#define G3  55
#define G3s 56
#define A3  57
#define A3s 58
#define B3  59
#define C4  60
#define C4s 61
#define D4  62
#define D4s 63
#define E4  64

#define G4  67
#define B4  71

int tp[] = {//Frequencies related to MIDI note numbers
  15289, 14431, 13621, 12856, 12135, 11454, 10811, 10204,//0-o7
  9631, 9091, 8581, 8099, 7645, 7215, 6810, 6428,//8-15
  6067, 5727, 5405, 5102, 4816, 4545, 4290, 4050,//16-23
  3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551,//24-31
  2408, 2273, 2145, 2025, 1911, 1804, 1703, 1607,//32-39
  1517, 1432, 1351, 1276, 1204, 1136, 1073, 1012,//40-47
  956, 902, 851, 804, 758, 716, 676, 638,//48-55
  602, 568, 536, 506, 478, 451, 426, 402,//56-63
  379, 358, 338, 319, 301, 284, 268, 253,//64-71
  239, 225, 213, 201, 190, 179, 169, 159,//72-79
  150, 142, 134, 127, 119, 113, 106, 100,//80-87
  95, 89, 84, 80, 75, 71, 67, 63,//88-95
  60, 56, 53, 50, 47, 45, 42, 40,//96-103
  38, 36, 34, 32, 30, 28, 27, 25,//104-111
  24, 22, 21, 20, 19, 18, 17, 16,//112-119
  15, 14, 13, 13, 12, 11, 11, 10,//120-127
  0//off
};

void setup(void)
{
  int i;

  Serial.begin(9600);

  // Generate 2MHz clock on Pin 3
  pinMode(CLK_PIN, OUTPUT);
  TCCR2A = 0x23;
  TCCR2B = 0x09;
  OCR2A = 7;
  OCR2B = 1;

  delay(50);

  // BDIR pin
  pinMode(BDIR_PIN, OUTPUT);
  digitalWrite(BDIR_PIN, LOW);

  // BC1 pin
  pinMode(BC1_PIN, OUTPUT);
  digitalWrite(BC1_PIN, LOW);

  // D0-D7 pins
  for (i = D0_PIN; i < (D0_PIN + 8); i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  //for (i = 0; i <= 0x0d; i++) {
  //  if (i == 7)
  //    aywrite(i, 0x3f);
  //  else
  //    aywrite(i, 0);
  //}

  aywrite(15, 0);
  aywrite(0, 142);  // (2000000 / 32) / frequency
  aywrite(1, 0);
  //aywrite(2, 255);
  //aywrite(0, 64);
  aywrite(6, 15);   // Noise
  aywrite(7, 0x3e); // Channel enables
  aywrite(8, 16);   // Channel amplitudes
  aywrite(9, 0);  
  aywrite(10, 0);
  aywrite(11, 0);   // Envelope period
  aywrite(12, 4);
  aywrite(13, 10);
}

void loop(void)
{
  int ana;

  ana = analogRead(0);

  aywrite(11, ana & 0xff);
  aywrite(12, ana >> 8);

  ana = analogRead(1);
  
  aywrite(0, ana & 0xff);
  aywrite(1, ana >> 8);

  delay(20);
}

void setnote(const int chan, const int midi)
{
  aywrite((chan * 2),     tp[midi] & 0xff);
  aywrite((chan * 2) + 1, tp[midi] >> 8);
}

void setenvelope(const int env)
{
  aywrite(11, env & 0xff);
  aywrite(12, env >> 8);
}

void aywrite(int reg, int val)
{
  ymzwrite(LOW, reg);
  ymzwrite(HIGH, val);
}

void ymzwrite(int a0, int val)
{
  int i;

#ifdef SLOW
  for (i = 0; i < 8; i++) {
    if (val & (1 << i))
      digitalWrite(D0_PIN + i, HIGH);
    else
      digitalWrite(D0_PIN + i, LOW);
  }

  if (a0)
    digitalWrite(BC1_PIN, LOW);  // BC1 LOW wite PSG
  else
    digitalWrite(BC1_PIN, HIGH); // BC1 HIGH latch address

  // BDIR HIGH for either type of cycle
  digitalWrite(BDIR_PIN, HIGH);
  
  // BDIR LOW
  digitalWrite(BDIR_PIN, LOW);
  digitalWrite(BC1_PIN, LOW);  // BC1 LOW
#else
  // D0-D1 on Port D bits 6 and 7; D2-D7 on Port B bits 0-5
  PORTD = (PORTD & 0x3f) | ((val & 0x03) << 6);
  PORTB = val >> 2;

  // BC1 on Arduino Pin 5, Port D bit 5
  if (a0)
    PORTD &= ~(1 << 5);
  else
    PORTD |= (1 << 5);

  // BDIR on Arduino Pin 4, Port D bit 4
  PORTD |= (1 << 4);

  __asm("nop");
  
  // BDIR on Arduino Pin 4, Port D bit 4
  // BC1 on Arduino Pin 5, Port D bit 5
  // Both go LOW
  PORTD &= ~((1 << 4) | (1 << 5));
#endif
}

