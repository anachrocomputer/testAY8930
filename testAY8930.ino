/* testAY8930 --- simple sketch to drive the AY8930 sound chip      2017-02-09 */
/* Copyright (c) 2017 John Honniball                                           */

#define CLK_PIN   (3)
#define BDIR_PIN  (4)
#define BC1_PIN   (5)
#define D0_PIN    (6)

#define BANKA (0xA0)
#define BANKB (0xB0)

#define DUTYCYCLE50 (4)

uint8_t CurrentBank = 0;
uint8_t CurrentEnvMode = 0;

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

  // First access to any register apart from R13 will switch chip
  // into enhanced mode.
  aywrite(0, 142);  // (2000000 / 32) / frequency
  aywrite(1, 0);
  aywrite(6, 2);   // Noise
  aywrite(7, 0x2e); // Channel enables
  setAmplitude(0, 32);   // Channel amplitudes
  setAmplitude(1, 32);  
  setAmplitude(2, 0);
  setEnvelopePeriod(0, 1024);
  aywrite(13, 10); // Channel A envelope to triangle
  setEnvelopePeriod(1, 2048);
  aywrite(20, 10); // Channel B envelope to triangle
  setDutyCycle(0, DUTYCYCLE50);  // All three tone channels to 50% duty cycle
  setDutyCycle(1, DUTYCYCLE50);
  setDutyCycle(2, DUTYCYCLE50);
  aywrite(25, 0x55);   // Set up noise generator
  aywrite(26, 0xaa);
}

void loop(void)
{
  int ana;

  ana = analogRead(0);

  setEnvelopePeriod(0, ana);

  ana = analogRead(1);
  
  setTonePeriod(0, (ana * 4) + 2048);

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

void setDutyCycle(const int channel, const int dutyCycle)
{
  aywrite(22 + channel, dutyCycle);
}

void setEnvelopePeriod(const int channel, const unsigned int envelope)
{
  switch (channel) {
  case 0:
    aywrite(11, envelope & 0xff);
    aywrite(12, envelope >> 8);
    break;
  case 1:
    aywrite(16, envelope & 0xff);
    aywrite(17, envelope >> 8);
    break;
  case 2:
    aywrite(18, envelope & 0xff);
    aywrite(19, envelope >> 8);
    break;
  }
}


/* aywrite --- write a byte to a given register in the PSG */

void aywrite(int reg, int val)
{
  // Logic in this function handles the mode and bank selection
  // bits in Register 13, so that the chip appears to have 32 registers
  // numbered 0-31. Access to register 13 is a special case.
  // Access to other registers will cause a bank swap via R13 only
  // if required.
  if (reg == 13) {
    CurrentEnvMode = val;
    ay8930write(LOW, 13);  // Latch register number
    ay8930write(HIGH, CurrentEnvMode | CurrentBank);
  }
  else if (reg < 16) {  // Bank A register 0-15
    if (CurrentBank != BANKA) {
      CurrentBank = BANKA;
      ay8930write(LOW, 13);  // Select register 13
      ay8930write(HIGH, CurrentEnvMode | CurrentBank);
    }
    ay8930write(LOW, reg);  // Latch register number
    ay8930write(HIGH, val); // Latch register contents
  }
  else {
    if (CurrentBank != BANKB) {
      CurrentBank = BANKB;
      ay8930write(LOW, 13);  // Select register 13
      ay8930write(HIGH, CurrentEnvMode | CurrentBank);
    }
    ay8930write(LOW, reg - 16);  // Latch register number
    ay8930write(HIGH, val); // Latch register contents
  }
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

