/* testAY8930 --- simple sketch to drive the AY8930 sound chip      2017-02-09 */
/* Copyright (c) 2017 John Honniball                                           */

#define CLK_PIN   (3)
#define BDIR_PIN  (4)
#define BC1_PIN   (5)
#define D0_PIN    (6)

// Register numbers 0-15 common to AY-3-8910; 16-31 unique to AY8930
#define TONEPERIODA_REG     (0)
#define TONEPERIODB_REG     (2)
#define TONEPERIODC_REG     (4)
#define NOISEPERIOD_REG     (6)
#define ENABLE_REG          (7)
#define AMPLITUDEA_REG      (8)
#define AMPLITUDEB_REG      (9)
#define AMPLITUDEC_REG      (10)
#define ENVELOPEPERIODA_REG (11)
#define ENVELOPEMODEA_REG   (13)  // Also contains bank selection bits in AY8930
#define IOPORTA_REG         (14)
#define IOPORTB_REG         (15)
#define ENVELOPEPERIODB_REG (16)
#define ENVELOPEPERIODC_REG (18)
#define ENVELOPEMODEB_REG   (20)
#define ENVELOPEMODEC_REG   (21)
#define DUTYCYCLEA_REG      (22)
#define DUTYCYCLEB_REG      (23)
#define DUTYCYCLEC_REG      (24)
#define NOISEANDMASK_REG    (25)
#define NOISEORMASK_REG     (26)

#define BANKA (0xA0)
#define BANKB (0xB0)

#define DUTYCYCLE03 (0)
#define DUTYCYCLE06 (1)
#define DUTYCYCLE12 (2)
#define DUTYCYCLE25 (3)
#define DUTYCYCLE50 (4)
#define DUTYCYCLE75 (5)
#define DUTYCYCLE87 (6)
#define DUTYCYCLE94 (7)
#define DUTYCYCLE97 (8)

#define CHANNELA  (0)
#define CHANNELB  (1)
#define CHANNELC  (2)

#define IOPORTA   (0)
#define IOPORTB   (1)

#define AMPLITUDE_ENV (32)
#define AMPLITUDE_MAX (31)
#define AMPLITUDE_OFF (0)

uint8_t CurrentBank = 0;
uint8_t CurrentEnvMode = 0;
uint8_t EnableReg = 0;
uint8_t Counter = 0;

void setup(void)
{
  Serial.begin(9600);

  initPSG();
  
  setNoiseMasks(0x55, 0xaa);   // Set up noise generator
  setNoisePeriod(2);
  
  setDutyCycle(CHANNELA, DUTYCYCLE50);  // All three tone channels to 50% duty cycle
  setDutyCycle(CHANNELB, DUTYCYCLE50);
  setDutyCycle(CHANNELC, DUTYCYCLE50);
  
  setToneEnable(CHANNELA, true);
  setNoiseEnable(CHANNELB, true);
  setPortDirection(IOPORTB, OUTPUT);
  
  setAmplitude(CHANNELA, AMPLITUDE_ENV);   // Channel amplitudes
  setAmplitude(CHANNELB, AMPLITUDE_ENV);
  setAmplitude(CHANNELC, AMPLITUDE_OFF);
  
  setEnvelopePeriod(CHANNELA, 1024);
  setEnvelopeMode(CHANNELA, 10); // Channel A envelope to triangle
  setEnvelopePeriod(CHANNELB, 2048);
  setEnvelopeMode(CHANNELB, 10); // Channel B envelope to triangle
}

void loop(void)
{
  int ana;

  ana = analogRead(0);

  setEnvelopePeriod(CHANNELA, ana);

  ana = analogRead(1);
  
  setTonePeriod(CHANNELA, (ana * 4) + 2048);

  setPortOutputs(IOPORTB, Counter++);
  
  delay(20);
}


/* initPSG --- initialise chip and disable all noise and tone channels */

void initPSG(void)
{
  int i;

  // Generate 2MHz clock on Pin 3
  pinMode(CLK_PIN, OUTPUT);
  TCCR2A = 0x23;
  TCCR2B = 0x09;
  OCR2A = 7;
  OCR2B = 1;

  delay(10);

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
  
  EnableReg = 0x3f; // All noise and tone channels disabled

  // First access to any register apart from R13 will switch chip
  // into enhanced mode.
  aywrite(ENABLE_REG, EnableReg);
}


/* setToneEnable --- enable or disable tone generation on a given channel */

void setToneEnable(const int channel, const int enable)
{
  const uint8_t mask = 1 << channel;

  if (enable) {
    EnableReg &= ~mask;
  }
  else {
    EnableReg |= mask;
  }

  aywrite(ENABLE_REG, EnableReg);
}


/* setNoiseEnable --- enable or disable noise generation on a given channel */

void setNoiseEnable(const int channel, const int enable)
{
  const uint8_t mask = 1 << (3 + channel);

  if (enable) {
    EnableReg &= ~mask;
  }
  else {
    EnableReg |= mask;
  }

  aywrite(ENABLE_REG, EnableReg);
}


/* setPortDirection --- set I/O port to input or output */

void setPortDirection(const int channel, const int direction)
{
  const uint8_t mask = 1 << (6 + channel);

  if (direction == INPUT) {
    EnableReg &= ~mask;
  }
  else if (direction == OUTPUT) {
    EnableReg |= mask;
  }

  aywrite(ENABLE_REG, EnableReg);
}


/* setPortOutputs --- send a byte to the output port pins */

void setPortOutputs(const int channel, const int val)
{
  aywrite(IOPORTA_REG + channel, val);
}


/* setAmplitude --- set amplitude for a given channel */

void setAmplitude(const int channel, const int amplitude)
{
  aywrite(AMPLITUDEA_REG + channel, amplitude);
}


/* setTonePeriod --- set tone period for a given channel */

void setTonePeriod(const int channel, const unsigned int period)
{
  aywrite(TONEPERIODA_REG + (channel * 2), period & 0xff);
  aywrite(TONEPERIODA_REG + (channel * 2) + 1, period >> 8);
}


/* setDutyCycle --- set tone duty cycle for a given channel */

void setDutyCycle(const int channel, const int dutyCycle)
{
  aywrite(DUTYCYCLEA_REG + channel, dutyCycle);
}


/* setNoisePeriod --- set period of the noise generator */

void setNoisePeriod(const int period)
{
  aywrite(NOISEPERIOD_REG, period);
}


/* setNoiseMasks --- set AND and OR masks of the noise generator */

void setNoiseMasks(const int andMask, const int orMask)
{
  aywrite(NOISEANDMASK_REG, andMask);
  aywrite(NOISEORMASK_REG, orMask);
}


/* setEnvelopePeriod --- set period of envelope generator */

void setEnvelopePeriod(const int channel, const unsigned int envelope)
{
  switch (channel) {
  case CHANNELA:
    aywrite(ENVELOPEPERIODA_REG, envelope & 0xff);
    aywrite(ENVELOPEPERIODA_REG + 1, envelope >> 8);
    break;
  case CHANNELB:
    aywrite(ENVELOPEPERIODB_REG, envelope & 0xff);
    aywrite(ENVELOPEPERIODB_REG + 1, envelope >> 8);
    break;
  case CHANNELC:
    aywrite(ENVELOPEPERIODC_REG, envelope & 0xff);
    aywrite(ENVELOPEPERIODC_REG + 1, envelope >> 8);
    break;
  }
}


/* setEnvelopeMode --- set mode of envelope generator */

void setEnvelopeMode(const int channel, const unsigned int mode)
{
  switch (channel) {
  case CHANNELA:
    aywrite(ENVELOPEMODEA_REG, mode);
    break;
  case CHANNELB:
    aywrite(ENVELOPEMODEB_REG, mode);
    break;
  case CHANNELC:
    aywrite(ENVELOPEMODEC_REG, mode);
    break;
  }
}


/* aywrite --- write a byte to a given register in the PSG */

void aywrite(const int reg, const int val)
{
  // Logic in this function handles the mode and bank selection
  // bits in Register 13, so that the chip appears to have 32 registers
  // numbered 0-31. Access to register 13 is a special case.
  // Access to other registers will cause a bank swap via R13 only
  // if required.
  if (reg == ENVELOPEMODEA_REG) {
    CurrentEnvMode = val;
    ay8930write(LOW, ENVELOPEMODEA_REG);  // Latch register number
    ay8930write(HIGH, CurrentEnvMode | CurrentBank);
  }
  else if (reg < 16) {  // Bank A register 0-15
    if (CurrentBank != BANKA) {
      CurrentBank = BANKA;
      ay8930write(LOW, ENVELOPEMODEA_REG);  // Select register 13
      ay8930write(HIGH, CurrentEnvMode | CurrentBank);
    }
    ay8930write(LOW, reg);  // Latch register number
    ay8930write(HIGH, val); // Latch register contents
  }
  else {
    if (CurrentBank != BANKB) {
      CurrentBank = BANKB;
      ay8930write(LOW, ENVELOPEMODEA_REG);  // Select register 13
      ay8930write(HIGH, CurrentEnvMode | CurrentBank);
    }
    ay8930write(LOW, reg - 16);  // Latch register number
    ay8930write(HIGH, val); // Latch register contents
  }
}


/* ay8930write --- emulate a bus cycle to write a single byte to the chip */

void ay8930write(const int a0, const int val)
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

