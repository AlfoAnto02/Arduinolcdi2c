#include "wiring_private.h"
//Tick di timer necessari per simulare 15,625sm
#define TM 243
//Tick di timer necessari per simulare 1s
#define TM10CNT 62500

//LCD Pins & Address
#define LCD_ADDRESS 0x27
#define SDA PC4
#define SCL PC5

//Variabili per il calcolo della luce
#define GAMMA 0.7
#define RL10 50 
#define BETA 3950

//Variabile contatore per simulare i 10s
volatile uint8_t second_counter;

//Varaibile accumulatrice per la luce
volatile float lux;

//Variabile accumulatrice per la temperatura
volatile float temperature;

//Variabile accumulatrice per la potenza
volatile float resE;

//Variabile contatore per simulare i 156,25ms
volatile uint8_t count_misuration;

//Variabile utilizzata per selezionare il canale giusto per l'interrupt di lettura analogica
volatile uint8_t channel = 0;

void setup() {
  Serial.begin(9600);

  lux = 0.0;
  resE = 0.0;
  temperature = 0.0;
  //Configurazione Schermo LCD iniziale per il protocollo l2c
  configLCDScreen();

  lcdInitialize();

  //CONFIGURAZIONE ISR PER LETTURA ANALOGICA
  configAnalogRead();

 //CONFIGURAZIONE PRIMO TIMER OGNI s
  config1STimer();

  //CONFIGURAZIONE SECONDO TIMER OGNI 15,625 ms
  config156msTimer();

}

void config1STimer(){
  second_counter = 0;
  count_misuration=0;
  cbi(PORTC,PC0);
  cbi(DDRC,PC0);  
  TCCR1A=0;
  TCCR1B=0;
  TCNT1=0;
  OCR1A=TM10CNT; 
  sbi(TCCR1B,WGM12);
  sbi(TCCR1B,CS12);
  sbi(TIMSK1,OCIE1A);
}

void config156msTimer(){
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  OCR2A = TM;
  sbi(TCCR2A,WGM21);
  sbi(TCCR2B,CS22);
  sbi(TCCR2B,CS21);
  sbi(TCCR2B,CS20);
  sbi(TIMSK2,OCIE2A);
}


void configAnalogRead(){
  sbi(ADMUX,REFS0);
  sbi(ADCSRA,ADEN); 
  sbi(ADCSRA,ADIE); 
  sbi(ADCSRA,ADPS1); 
  sbi(ADCSRA,ADPS2);
  sbi(ADCSRA,ADSC);
}

ISR(TIMER1_COMPA_vect){
  second_counter++;
  if(second_counter==10){
  lcdUpdate();
  second_counter=0;
  lux=0;
  temperature=0;
  resE=0;
  }
}

ISR(TIMER2_COMPA_vect){
    if(count_misuration>=10){
      sbi(ADCSRA,ADSC);
      count_misuration=0;
    }
    count_misuration++;
  }

ISR(ADC_vect){
  if(channel == 0 ){
      int analogValue1 = ADC;
      float voltage = analogValue1 / 1024. * 5;
      float resistance = 2000 * voltage / (1 - voltage / 5);
      float singleLux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
      lux+=singleLux;
      sbi(ADMUX,MUX0);
      channel = 1;
      sbi(ADCSRA,ADSC);
  } else if (channel == 1){
      float analogValue2 = ADC;
      temperature+= 1/(log(1/(1023./analogValue2-1))/BETA+1.0/298.15)-273.15; 
      cbi(ADMUX,MUX0);
      sbi(ADMUX,MUX1);
      channel = 2;
      sbi(ADCSRA,ADSC);
  } else if (channel == 2){
      resE+=ADC;
      cbi(ADMUX,MUX1);
      channel = 0;
  }
}

void configLCDScreen(){
  sbi(DDRC,SDA);
  sbi(DDRC,SCL);
  //Richiamo la funzione stop per far entrare lo schermo nella fase idle
  stopLCDScreen();
}

//Nella fase di stop prima l'SDA va High e poi l'SCL va High
void stopLCDScreen(){
  sbi(PORTC,SCL);
  sbi(PORTC,SDA);
}

//Nella fase di start prima l'SDA va Low e poi l'SCL va Low
void startLCDScreen(){
  cbi(PORTC,SDA);
  cbi(PORTC,SCL);
}

void sendCommand(uint8_t command){
  sendByte(command,LOW);
}

void sendData(uint8_t data) {
  sendByte(data,HIGH);
}

void sendByte(uint8_t value, uint8_t mode){
  sendNibble(value >> 4, mode); //Nibble superiore
  sendNibble(value & 0x0F, mode); //nibble inferiore
}

void sendNibble(uint8_t nibble, uint8_t mode){
  uint8_t data = nibble << 4;
  if (mode == HIGH) data |= 0x01;
  data |= 0x08;
  pulseEnable(data);
}

void pulseEnable(uint8_t data){
  i2cSend(data | 0x04);
  i2cSend(data & ~0x04);
}

void i2cSend(uint8_t data) {
  startLCDScreen();
  lcdWrite(0x4E);
  lcdWrite(data);
  stopLCDScreen();
}

void lcdWrite(uint8_t data){
  for(int i = 0; i<8; i++){
    lcdWriteBit((data & 0x80)!=0);
    data<<=1;
  }
  lcdReadBit();
}

void lcdReadBit(){
  cbi(DDRC,SDA);
  sbi(PORTC,SCL);
  bool data = PINC & (1 <<SDA);
  cbi(PORTC,SCL);
  sbi(DDRC,SDA);
  return data;
}

void lcdWriteBit(bool data){
  if(data == true){
      sbi(PORTC,SDA);
  } else { cbi(PORTC,SDA);}
  sbi(PORTC,SCL);
  cbi(PORTC,SCL);
}

void lcdPrintString( char *data ){
  while (*data){
    sendData(*data++);
  }
}

void lcdPrintNumber(float number){
  char buffer[10];
  dtostrf(number,3,1,buffer);
  lcdPrintString(buffer);
}

void setCursor(uint8_t col, uint8_t row){
  uint8_t address;
  if(row == 0 ) address = 0x80 + col;
  else address = 0xC0 + col;
  sendCommand(address);
}


void lcdInitialize(){
  sendCommand(0x03);
  sendCommand(0x03);
  sendCommand(0x03);
  sendCommand(0x02);  //modalità 4-bit 
  //2-linee, 5x8 , modalità 4-bit 
  sendCommand(0x28);
  // Display On 
  sendCommand(0x0C);
  // Clear display
  sendCommand(0x01);
  // Entry Mode Set
  sendCommand(0x06);
  sendCommand(0x0C);
}

void lcdUpdate(){
  sendCommand(0x01);
  setCursor(0,0);
  lcdPrintNumber(lux/64);
  setCursor(0,1);
  lcdPrintString("Luce");
  setCursor(6,0);
  lcdPrintNumber(temperature/64);
  setCursor(6,1);
  lcdPrintString("Temp");
  setCursor(12,0);
  lcdPrintNumber(resE/64);
  setCursor(12,1);
  lcdPrintString("Res");
}


void loop() {

}

