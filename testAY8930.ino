/* testAY8930 --- simple sketch to drive the AY8930 sound chip      2017-02-09 */
/* Copyright (c) 2017 John Honniball                                           */

#define CLK_PIN   (3)
#define BDIR_PIN  (4)
#define BC1_PIN   (5)
#define D0_PIN    (6)


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

  // BC2 pin on AY8930 is no-connect

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

  aywrite(0, 142);  // (2000000 / 32) / frequency
  aywrite(1, 0);
  aywrite(6, 15);   // Noise
  aywrite(7, 0x3e); // Channel enables
  setAmplitude(0, 16);   // Channel amplitudes
  setAmplitude(1, 0);  
  setAmplitude(2, 0);
  aywrite(11, 0);   // Envelope period
  aywrite(12, 4);
  aywrite(13, 10);
}

void loop(void)
{
  int ana;

  ana = analogRead(0);

  setEnvelopePeriod(ana);

  ana = analogRead(1);
  
  setTonePeriod(0, ana);

  delay(20);
}

void setAmplitude(const int channel, const int amplitude)
{
  aywrite(8 + channel, amplitude);
}

void setTonePeriod(const int channel, const unsigned int period)
{
  aywrite((channel * 2), period & 0xff);
  aywrite((channel * 2) + 1, period >> 8);
}

void setEnvelopePeriod(const unsigned int envelope)
{
  aywrite(11, envelope & 0xff);
  aywrite(12, envelope >> 8);
}


/* aywrite --- write a byte to a given register in the PSG */

void aywrite(int reg, int val)
{
  ay8930write(LOW, reg);  // Latch register number
  ay8930write(HIGH, val); // Latch register contents
}


/* ay8930write --- emulate a bus cycle to write a single byte to the chip */

void ay8930write(int a0, int val)
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

