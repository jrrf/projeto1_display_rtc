/* Projeto Display RTC
 * Microcontroladores
 * 26/04/18 testado
 * Ultima Alteração: 26/04/18 - 00h22

interrupção no pino D2
*/
#include "Timer.h"
#include "Wire.h"
#include <LiquidCrystal_I2C.h>

#define DS3231_I2C_ADDRESS 0x68
#define DISPLAY_I2C_ADDRESS 0X3F

#define SEC_REG 0x00
#define MIN_REG 0x01
#define HOR_REG 0x02

#define TEM_REG 0x11
#define TEM2_REG 0x12

#define DIA_REG 0x03              //dia da semana 1-7
#define DAT_REG 0x04              //dia do mes 1-31
#define MES_REG 0x05
#define ANO_REG 0x06

#define ALARM1_SEC_REG 0x07
#define ALARM1_MIN_REG 0x08
#define ALARM1_HOR_REG 0x09
#define ALARM1_DAY_REG 0x0A
#define ALARM2_MIN_REG 0x0B
#define ALARM2_HOR_REG 0x0C
#define ALARM2_DAY_REG 0x0D
#define CONTROLE_REG 0x0E
#define STATUS_REG 0x0F

#define BOTAO_PIN A0
#define BUZ 3
#define SQW_PIN 2

//Variaveis
boolean estado_anterior;
byte exibeTemperaturaDataAlarme;   //0 = temperatura, 1 = Data, 2 = alarme;
char buf[14];
LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDRESS,2,1,0,4,5,6,7,3, POSITIVE);
Timer t;
volatile boolean disparouAlarme;
boolean luz;
//  lcd.write((byte)0); //Mostra o icone do relogio formado pelo array

byte sinoIcon[8] = {
  B00000,
  B01110,
  B01010,
  B10001,
  B10001,
  B11111,
  B00100,
  B00000
};

//Funcoes
void setup() {
  
  pinMode(SQW_PIN, INPUT_PULLUP);
  pinMode(BOTAO_PIN, INPUT);
  pinMode(BUZ, OUTPUT);
  attachInterrupt(0,gatilhoAlarme,FALLING);
  
  disparouAlarme = false;
  estado_anterior = false;
  exibeTemperaturaDataAlarme = 0;   //0 = temperatura, 1 = Data, 2 = alarme
  luz = true;
  Serial.begin(9600);
  Serial.flush();
  Wire.begin();
  lcd.begin (16,2);
  
  lcd.setBacklight(luz);       //HIGH acende luz de fundo. LOW apaga
  lcd.setCursor(0,0);
  lcd.createChar(0, sinoIcon);    //Cria o caractere customizado com o simbolo do grau

  t.every(1000, alarmando);
  t.every(1000, exibir);
}

void loop(){
  t.update();
  if(botaoPrecionado()){           //verifica se algum botao foi precionado
    if(disparouAlarme){
      clearAlarme1();
      disparouAlarme = false;
      lcd.setBacklight(HIGH);       //HIGH acende luz de fundo. LOW apaga
    }
    else identificaBotaoPrecionado();              //identifica qual botao foi precionado
  }
  receberValor();
}

//botoes e açoes
boolean botaoPrecionado(){
  int val = analogRead(BOTAO_PIN);
  
  if(val<150) //Se o botao nao estiver precionado, seta pra false
    estado_anterior = false;
    
  else if(!estado_anterior){
    estado_anterior = true; //indica que o botao esta precionado
    return true;
  }
  return false;
}

void identificaBotaoPrecionado(){
  int val = analogRead(BOTAO_PIN);
  Serial.print("Dado botao: ");
  Serial.println(val);
  if(val>500) 
    altExibecaoTemperaturaData();
  else {
    altFormatoDeHora(HOR_REG);
    altFormatoDeHora(ALARM1_HOR_REG);
  }
}

void altExibecaoTemperaturaData(){
  exibeTemperaturaDataAlarme = (++exibeTemperaturaDataAlarme)%3;
}

void altFormatoDeHora(byte regAddress){
  byte val = readDS3231Reg(regAddress);
  boolean formato = lerFormatoHora(regAddress);
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(regAddress);
  if(formato){ //se 12h
    val = de12para24(val)|(val&0b10000000);
  }else {
    val=decToBcd(de24para12(bcdToDec(val)))|(val&0b10000000);
  }
  Wire.write(val);
  Wire.endTransmission();
}


boolean lerFormatoHora(byte regAddress){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(regAddress); // set DS3231 register pointer to regAddress
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 1); //requests 1 Byte from DS3231 register identified by regAddress
  return Wire.read()&0x40; //0 = 24h, 1 = am/pm
}

//alarme
boolean verificaAlarmeAtivo(){
  return readDS3231Reg(CONTROLE_REG)&0x01;
}

void alarmando(){
  if(disparouAlarme){
    lcd.setBacklight(luz);
    tone(BUZ, 200, 500); //(pino, tom, duracao)
    luz=!luz;
  }
}

void ativarAlarme(){
  byte val = readDS3231Reg(CONTROLE_REG);
  if(verificaAlarmeAtivo())
    writeDS3231Reg(CONTROLE_REG, bcdToDec(val&0b11111110));
  else
    writeDS3231Reg(CONTROLE_REG, bcdToDec(val|0b00000001));
  clearAlarme1();
}

void gatilhoAlarme(){  
  disparouAlarme = true;
  Serial.println("Disparou alarme");
}

void clearAlarme1(void){
    byte val;
    val = readDS3231Reg(STATUS_REG);
    val &= 0b11111110;
    writeDS3231Reg(STATUS_REG, bcdToDec(val));
}

void periodoDoAlarme(){
  //hr,min,seg
  writeDS3231Reg(ALARM1_SEC_REG, bcdToDec(readDS3231Reg(ALARM1_SEC_REG)&0b01111111));
  writeDS3231Reg(ALARM1_MIN_REG, bcdToDec(readDS3231Reg(ALARM1_MIN_REG)&0b01111111));
  writeDS3231Reg(ALARM1_HOR_REG, bcdToDec(readDS3231Reg(ALARM1_HOR_REG)&0b01111111));
  writeDS3231Reg(ALARM1_DAY_REG, bcdToDec(readDS3231Reg(ALARM1_DAY_REG)|0b10000000)); 
  
  writeDS3231Reg(ALARM2_MIN_REG, bcdToDec(readDS3231Reg(ALARM2_MIN_REG)&0b01111111));
  writeDS3231Reg(ALARM2_HOR_REG, bcdToDec(readDS3231Reg(ALARM2_HOR_REG)&0b01111111));
  writeDS3231Reg(ALARM2_DAY_REG, bcdToDec(readDS3231Reg(ALARM2_DAY_REG)&0b01111111)); 
}

//entrada de dados
void receberValor(){
  if(Serial.available()){   //verifica se tem entrada de dados
    int index=0;
    delay(20);
    int qntChar = Serial.available();
    if(qntChar>12)
      qntChar=12;
    while(qntChar--)
      buf[index++]=Serial.read();
    dividirString(buf);
  }
}

void dividirString(char* dado){
  Serial.print("Dado recebido: ");
  Serial.print(dado);
  char* parameter;
  parameter = strtok(dado," ,");
  while(parameter !=NULL){        //Se NULL indica que chegou ao final da string
    setTempo(parameter);
    parameter=strtok(NULL," ,");  //NULL indica pra continuar de onde parou
  }
  //limpa o texto e o buffer seriais
  for(int x=0;x<14;x++)
    buf[x]='\0';
}

/*
  h = hora
  m = minuto
  s = segundo
  ds = dia da semana
  d = dia
  me = mes
  a = ano
  ha = hora alarme
  ma = minuto alarme
  sa = segundo alarme
*/
void setTempo(char* dado){
  if(dado[0]=='1'){ //1 = ativa/desativa alarme
     ativarAlarme();
     
  }
  
  else if(dado[0]=='2'){ //1 = ativa/desativa INTCN
    
     if(readDS3231Reg(CONTROLE_REG)&0b00000100) writeDS3231Reg(CONTROLE_REG, bcdToDec(readDS3231Reg(CONTROLE_REG)&0b11111011));
     else writeDS3231Reg(CONTROLE_REG, bcdToDec(readDS3231Reg(CONTROLE_REG)|0b00000100));
  }

//alarme1
  else if(dado[0]=='h' && dado[1]=='a'){ //ha = hora alarme
    //Converte a string para inteiro
    byte Ans = strtol(dado+2,NULL,10); //Recebe String para converter, 
                                       //ponteiro caractere depois do inteiro, 
                                       //a base
    Serial.print("Hor alarme: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 23); //restringe para o valor ir de 0 a 23  
    Ans = converte(Ans, ALARM1_HOR_REG);
    Ans = bcdToDec(decToBcd(Ans)|(readDS3231Reg(ALARM1_HOR_REG)&0b10000000));
    writeDS3231Reg(ALARM1_HOR_REG,Ans);
  }
  
  else if(dado[0]=='m' && dado[1]=='a'){ //ma = minuto alarme
    
    //Converte a string para inteiro
    int Ans = strtol(dado+2,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  Min alarme: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 59); //restringe para o valor ir de 0 a 59
    Ans = bcdToDec(decToBcd(Ans)|(readDS3231Reg(ALARM1_HOR_REG)&0b10000000));
    writeDS3231Reg(ALARM1_MIN_REG,Ans);
  }
  
  else if(dado[0]=='s' && dado[1]=='a'){ //sa = segundo alarme
    //Converte a string para inteiro
    int Ans = strtol(dado+2,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  Seg alarme: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 59); //restringe para o valor ir de 0 a 59 
    Ans = bcdToDec(decToBcd(Ans)|(readDS3231Reg(ALARM1_HOR_REG)&0b10000000));
    writeDS3231Reg(ALARM1_SEC_REG,Ans);
  }

//data
  else if(dado[0]=='d' && dado[1]=='s'){ //ds = dia da semana
    //Converte a string para inteiro
    int Ans = strtol(dado+2,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  dia da semana alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 1, 7); //restringe para o valor ir de 1 a 7 
    writeDS3231Reg(DIA_REG,Ans);
  }
  
  else if(dado[0]=='d'){ //d = dia
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  dia alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 1, 31); //restringe para o valor ir de 0 a 31 
    writeDS3231Reg(DAT_REG,Ans);
  }
  
  else if(dado[0]=='m' && dado[1]=='e'){ //me = mes
    //Converte a string para inteiro
    int Ans = strtol(dado+2,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  mes alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 1, 12); //restringe para o valor ir de 0 a 12 
    writeDS3231Reg(MES_REG,Ans);
  }
  
  else if(dado[0]=='a'){ //a = ano
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  ano alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 99); //restringe para o valor ir de 0 a 99 
    writeDS3231Reg(ANO_REG,Ans);
  }

//hora
  else if(dado[0]=='h'){ //h = hora
    //Converte a string para inteiro
    byte Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                       //ponteiro caractere depois do inteiro, 
                                       //a base
    Serial.print("Hor alterada para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 23); //restringe para o valor ir de 0 a 23   
    writeDS3231Reg(HOR_REG,converte(Ans, HOR_REG));
  }
  
  else if(dado[0]=='m'){ //m = minuto
    
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  Min alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 59); //restringe para o valor ir de 0 a 59
    writeDS3231Reg(MIN_REG,Ans);
  }
  
  else if(dado[0]=='s'){ //s = segundo
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  Seg alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 59); //restringe para o valor ir de 0 a 59 
    writeDS3231Reg(SEC_REG,Ans);
  }


}

byte converte(byte Ans, byte regAddress){
  if(lerFormatoHora(regAddress))
    Ans = de24para12(Ans);
  return Ans;
}

//leitura e escrita nos registradores
byte readDS3231Reg(byte regAddress){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(regAddress); // set DS3231 register pointer to regAddress
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 1); //requests 1 Byte from DS3231 register identified by regAddress
  if(regAddress==TEM2_REG){
    byte lsb = Wire.read();
    return ((lsb>> 6)*25);
  }
  return Wire.read();
}

void writeDS3231Reg(byte regAddress, byte value){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(regAddress); // set DS3231 register pointer to regAddress
  Wire.write(decToBcd(value));
  Wire.endTransmission();
}

//gets
String getTemperatura(){
  byte val = readDS3231Reg(TEM_REG);
  String st="";
  st+=val;
  st+=",";
  val = readDS3231Reg(TEM2_REG);
  if(val<10)
    st+="0";
  st+=val;
  return st;
}

String getTempo(){
  byte val;
  if(lerFormatoHora(HOR_REG))
    val = bcdToDec(readDS3231Reg(HOR_REG)&0x1F);
  else val = bcdToDec(readDS3231Reg(HOR_REG)&0x3F);
  String st="";
  if(val<10)
    st+="0";
  st+=val;
  st+=":";
  val = bcdToDec(readDS3231Reg(MIN_REG));
  if(val<10)
    st+="0";
  st+=val;
  st+=":";
  val = bcdToDec(readDS3231Reg(SEC_REG));
  if(val<10)
    st+="0";
  st+=val;
  
  if(lerFormatoHora(HOR_REG)){
    if(readDS3231Reg(HOR_REG)&0x20)
      st+=" PM";
  else st+=" AM";
  }
  return st;
}

String getData(){
  String st="";
  byte val = readDS3231Reg(DIA_REG);
  switch(val){
    case 1:st+="Dom "; break;
    case 2:st+="Seg "; break;
    case 3:st+="Ter "; break;
    case 4:st+="Qua "; break;
    case 5:st+="Qui "; break;
    case 6:st+="Sex "; break;
    case 7:st+="Sab "; 
  }
  val = bcdToDec(readDS3231Reg(DAT_REG));
  if(val<10)
    st+="0";
  st+=val;
  st+="/";
  val = bcdToDec(readDS3231Reg(MES_REG));
  if(val<10)
    st+="0";
  st+=val;
  st+="/";
  val = bcdToDec(readDS3231Reg(ANO_REG));
  if(val<10)
    st+="0";
  st+=val;

  return st;
}

String getAlarme(){
  byte val;
  if(lerFormatoHora(ALARM1_HOR_REG))
    val = bcdToDec(readDS3231Reg(ALARM1_HOR_REG)&0x1F);
  else val = bcdToDec(readDS3231Reg(ALARM1_HOR_REG)&0x3F);
  String st="";
  if(val<10)
    st+="0";
  st+=val;
  st+=":";
  val = bcdToDec(readDS3231Reg(ALARM1_MIN_REG)&0b01111111);
  if(val<10)
    st+="0";
  st+=val;
  st+=":";
  val = bcdToDec(readDS3231Reg(ALARM1_SEC_REG)&0b01111111);
  if(val<10)
    st+="0";
  st+=val;
  
  if(lerFormatoHora(ALARM1_HOR_REG)){
    if(readDS3231Reg(ALARM1_HOR_REG)&0x20)
      st+=" PM";
    else st+=" AM";
  }
  return st;
}

//exibicao
void exibir(){
  //exibirSerial();
  exibirDisplay();
}

void exibirSerial(){
  Serial.print("Tempo: ");
  Serial.print(getTempo());
/*  Serial.print(" Temperatura: ");
  Serial.print(getTemperatura());
  Serial.print(" Data: ");
  Serial.print(getData());
*/  Serial.print(" Alarme1: ");
  Serial.print(getAlarme());
  if(verificaAlarmeAtivo())
    Serial.println(" ON");
  else 
    Serial.println(" OFF");
  
  
}

void exibirDisplay(){
  //Tempo
  lcd.setCursor(0,0); //(C,L)
  if(verificaAlarmeAtivo())
    lcd.write((byte)0); //Mostra o icone do relogio formado pelo array  
  else lcd.print(" ");
  
  if(!lerFormatoHora(HOR_REG))
    lcd.print("   ");
  else lcd.print("  ");
  lcd.print(getTempo());  
  lcd.print("  ");
  
  lcd.setCursor(0,1); //(C,L)
  
  //Temperatura
  switch(exibeTemperaturaDataAlarme){
    case 0: 
      lcd.print("  Temp.");
      lcd.print(getTemperatura());
      lcd.print((char)223); //Mostra o simbolo do grau quadrado
      lcd.print("c       ");
      break;
    case 1: 
      lcd.print("  ");
      lcd.print(getData());
      break;
    case 2: 
      if(!lerFormatoHora(ALARM1_HOR_REG))
        lcd.print("    ");
      else lcd.print("   ");
      lcd.print(getAlarme());  
      lcd.print("  ");
      break;
  }
}

//conversores
byte de24para12(byte Ans){ //recebe a hr atual em padrao decimal
  Ans&=0b01111111;
  byte val=0b01000000; //coloca o bit pra informar que esta no formato am/pm
  if(Ans>=12)
    val= val|0b00100000; //coloca o bit pra informar que esta no periodo da tarde
  if(!(Ans%12)) //Se zero indica que sao 00h ou 12h
    Ans=12; //converte p 12
  else Ans=Ans%12;
  Ans = decToBcd(Ans)|val;
  Ans = bcdToDec(Ans);
  return Ans;
}

byte de12para24(byte Ans){  //recebe a hr atual em padrao BCD
  Ans&=0b01111111;
  if((Ans < 0x72 && Ans >= 0x61)|| Ans==0x52) // entre 1PM e 12AM
    Ans = ((Ans&0b00011111)+0b00010010)%0x24;
  else Ans=Ans&0b00011111;
  return Ans;
}

byte decToBcd(byte valor){
  return((valor/10*16) + (valor%10));
}

byte bcdToDec(byte valor){ 
  return( (valor/16*10) + (valor%16) );
}
