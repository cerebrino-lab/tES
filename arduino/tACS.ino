#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <EEPROM.h>

Adafruit_MCP4725 dac;

const PROGMEM int16_t tabela_seno[500] = {
5, 10, 15, 21, 26, 31, 36, 41, 46, 51, 56, 62, 67, 72, 77, 82, 87, 92, 97, 102, 107, 112, 117, 122, 127, 131, 136, 141, 146, 151, 156, 160, 165, 170, 174, 179, 184, 188, 193, 197, 202, 206, 211, 215, 219, 224, 228, 232, 237, 241, 245, 249, 253, 257, 261, 265, 269, 273, 277, 280, 284, 288, 291, 295, 299, 302, 305, 309, 312, 316, 319, 322, 325, 328, 331, 334, 337, 340, 343, 346, 348, 351, 354, 356, 359, 361, 364, 366, 368, 371, 373, 375, 377, 379, 381, 383, 384, 386, 388, 389, 391, 393, 394, 395, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 406, 407, 407, 408, 408, 409, 409, 409, 409, 409, 410, 409, 409, 409, 409, 409, 408, 408, 407, 407, 406, 406, 405, 404, 403, 402, 401, 400, 399, 398, 397, 395, 394, 393, 391, 389, 388, 386, 384, 383, 381, 379, 377, 375, 373, 371, 368, 366, 364, 361, 359, 356, 354, 351, 348, 346, 343, 340, 337, 334, 331, 328, 325, 322, 319, 316, 312, 309, 305, 302, 299, 295, 291, 288, 284, 280, 277, 273, 269, 265, 261, 257, 253, 249, 245, 241, 237, 232, 228, 224, 219, 215, 211, 206, 202, 197, 193, 188, 184, 179, 174, 170, 165, 160, 156, 151, 146, 141, 136, 131, 127, 122, 117, 112, 107, 102, 97, 92, 87, 82, 77, 72, 67, 62, 56, 51, 46, 41, 36, 31, 26, 21, 15, 10, 5, 0, -5, -10, -15, -21, -26, -31, -36, -41, -46, -51, -56, -62, -67, -72, -77, -82, -87, -92, -97, -102, -107, -112, -117, -122, -127, -131, -136, -141, -146, -151, -156, -160, -165, -170, -174, -179, -184, -188, -193, -197, -202, -206, -211, -215, -219, -224, -228, -232, -237, -241, -245, -249, -253, -257, -261, -265, -269, -273, -277, -280, -284, -288, -291, -295, -299, -302, -305, -309, -312, -316, -319, -322, -325, -328, -331, -334, -337, -340, -343, -346, -348, -351, -354, -356, -359, -361, -364, -366, -368, -371, -373, -375, -377, -379, -381, -383, -384, -386, -388, -389, -391, -393, -394, -395, -397, -398, -399, -400, -401, -402, -403, -404, -405, -406, -406, -407, -407, -408, -408, -409, -409, -409, -409, -409, -410, -409, -409, -409, -409, -409, -408, -408, -407, -407, -406, -406, -405, -404, -403, -402, -401, -400, -399, -398, -397, -395, -394, -393, -391, -389, -388, -386, -384, -383, -381, -379, -377, -375, -373, -371, -368, -366, -364, -361, -359, -356, -354, -351, -348, -346, -343, -340, -337, -334, -331, -328, -325, -322, -319, -316, -312, -309, -305, -302, -299, -295, -291, -288, -284, -280, -277, -273, -269, -265, -261, -257, -253, -249, -245, -241, -237, -232, -228, -224, -219, -215, -211, -206, -202, -197, -193, -188, -184, -179, -174, -170, -165, -160, -156, -151, -146, -141, -136, -131, -127, -122, -117, -112, -107, -102, -97, -92, -87, -82, -77, -72, -67, -62, -56, -51, -46, -41, -36, -31, -26, -21, -15, -10, -5, 0
};

volatile bool timer_flag = false;
uint16_t index_a = 0;
uint16_t index_b = 0;
int16_t sinal_a = 0;
int16_t sinal_b = 0;
int16_t saida = 0;
uint8_t freq_a = EEPROM.read(0);
uint8_t freq_b = EEPROM.read(1);
uint8_t ampl_a = EEPROM.read(2);
uint8_t ampl_b = EEPROM.read(3);
uint8_t modo = EEPROM.read(4);

//interrupção do timer 1
ISR(TIMER1_COMPA_vect) {
  timer_flag = true;
}

void setup(void) {

  cli();  //disable interrupts
//------------------------------------------------------------------------------
// Configura o timer 1 para 200μs
TCCR1A = 0;
TCCR1B = 0;
TCNT1 = 0;

//https://www.arduinoslovakia.eu/application/timer-calculator
// 5000 Hz (16000000/((49+1)*64))
  OCR1A = 49;
// CTC
  TCCR1B |= (1 << WGM12);
// Prescaler 64
  TCCR1B |= (1 << CS11) | (1 << CS10);
// Output Compare Match A Interrupt Enable
  TIMSK1 |= (1 << OCIE1A);
//-------------------------------------------------------------------------------
  sei();  //enable interrupts

  Serial.begin(9600);
  dac.begin(0x60);
}

void loop(void) {

//--------Recebe parametros pela serial
  if (Serial.available() > 0){
    String entrada = Serial.readStringUntil('\n');
    if (entrada == "c"){
      Serial.print("Modo de configuração\nf1= " + String(freq_a) + "\nf2= " + String(freq_b) + "\na1= " + String(ampl_a) + "\na2= " + String(ampl_b) + "\nmo= " + String(modo) + "\n");
      Serial.println("Digite o parametro:");
      while(Serial.available() == 0){}
      entrada = Serial.readStringUntil('\n');
      String parametro = entrada;
      Serial.println(parametro);
      Serial.println("Digite o valor:");
      if (parametro == "mo"){
        Serial.println("0 - normal\n1 - frequência cruzada positiva\n2 - frequência cruzada total");
      }
      while(Serial.available() == 0){}
      entrada = Serial.readStringUntil('\n');
      uint8_t valor = (uint8_t)entrada.toInt();
      Serial.println(String(valor));
      Serial.println("Gravar " + parametro + ": " + String(valor) + ", s/N?");
      while(Serial.available() == 0){}
      entrada = Serial.readStringUntil('\n');
      if (entrada == "s"){ 
        bool gravado = false;
        if (valor >=0 && valor <= 255){
          if (parametro == "f1"){
            EEPROM.write(0, valor);
            freq_a = EEPROM.read(0);
            gravado = true;
          }
          else if (parametro == "f2"){
            EEPROM.write(1, valor);
            freq_b = EEPROM.read(1);
            gravado = true;
          }
          else if (parametro == "a1"){
            EEPROM.write(2, valor);
            ampl_a = EEPROM.read(2);
            gravado = true;
          }
          else if (parametro == "a2"){
            EEPROM.write(3, valor);
            ampl_b = EEPROM.read(3);
            gravado = true;
          }
          else if (parametro == "mo"){
            EEPROM.write(4, valor);
            modo = EEPROM.read(4);
            gravado = true;
          }
          else{
            Serial.println("parametro incorreto!");
          }
          if(gravado == true){
            Serial.println(parametro + ": " + String(valor) + " - " + "gravado com sucesso!");
          }
        }
        else{
          Serial.println("valor incorreto");
        }
      }
      else
      {
        Serial.println("Saindo do modo de configuração");
      }
    }
  }
//--------Recebe parametros pela serial

//--------incrementa os indices de acordo com as frequencias
  if (timer_flag == true) {
    timer_flag = false;
    index_a = index_a + (uint16_t)freq_a;
    index_b = index_b + (uint16_t)freq_b;
    
    if (index_a > 5000) {
      index_a = index_a - 5000;
    }
    if (index_b > 5000) {
      index_b = index_b - 5000;
    }

//--------lê os valores na tabela_seno    
    
    sinal_a = pgm_read_word(&(tabela_seno[index_a/10])) * (int16_t)(ampl_a / 5);
    sinal_b = pgm_read_word(&(tabela_seno[index_b/10])) * (int16_t)(ampl_b / 5);

//--------calcula os sinais de acordo com o modo

    if (modo == 0){
      saida = sinal_a;
    }
    else if(modo == 1){
      if (sinal_a < 0) {
        saida = (int16_t)((((float)sinal_b + 819) * (float)sinal_a) / 1638);
      } else {
        saida = sinal_a;
      }
    }
    else if(modo == 2){
      if (sinal_a < 0) {
        saida = (int16_t)((((float)sinal_b + 819) * (float)sinal_a) / 1638);
      } else {
        saida = 0;
      }
    }

    
    dac.setVoltage((uint16_t)saida + 2048, false);
//--------calcula ondas

  }
}
