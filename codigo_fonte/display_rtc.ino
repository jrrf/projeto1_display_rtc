/* Projeto Display RTC
 * Microcontroladores
 * 10/04/18
 * Ultima Alteração: 10/04/18 - 22h55
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
#define BOTAO_PIN A0

//Variaveis
boolean estado_anterior;
boolean exibeTemperaturaOuData;   //true = temperatura, false = Data;
char buf[14];
LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDRESS,2,1,0,4,5,6,7,3, POSITIVE);
Timer t;

//Funcoes
void setup() {
  estado_anterior = false;
  exibeTemperaturaOuData = true;   //true = temperatura, false = Data;
  Serial.begin(9600);
  Serial.flush();
  Wire.begin();
  lcd.begin (16,2);
  lcd.setBacklight(HIGH);       //HIGH acende luz de fundo. LOW apaga
  lcd.setCursor(0,0);
  t.every(500, exibir);
}

void loop(){
  t.update();
  verificaBotaoPrecionado();              //verifica se algum botao foi precionado
  receberValor();
  //exibir();
}

void verificaBotaoPrecionado(){
  int val = analogRead(BOTAO_PIN);
  if(val<100) //Se o botao nao estiver precionado, seta pra false
    estado_anterior = false;
    
  else if(!estado_anterior){
    
    //identifica qual botao foi precionado
    if(val>500) 
      altExibecaoTemperaturaData();
    else 
      altFormatoDeHora();
    
    estado_anterior = true; //indica que o botao esta precionado
  }
}

void altExibecaoTemperaturaData(){
  exibeTemperaturaOuData = !exibeTemperaturaOuData;
}

void altFormatoDeHora(){
  byte val = readDS3231Register(HOR_REG);
  boolean formato = lerFormatoHora();
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(HOR_REG);
  if(formato){ //se 12h
    val = de12para24(val);
  }else {
    val=decToBcd(de24para12(bcdToDec(val)));
  }
  Wire.write(val);
  Wire.endTransmission();
}

boolean lerFormatoHora(){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(HOR_REG); // set DS3231 register pointer to regAddress
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 1); //requests 1 Byte from DS3231 register identified by regAddress
  return Wire.read()&0x40;
}

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

void setTempo(char* dado){
  if(dado[0]=='h' || dado[0]=='H'){ //h = hora
    //Converte a string para inteiro
    byte Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                       //ponteiro caractere depois do inteiro, 
                                       //a base
    Serial.print("Hor alterada para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 23); //restringe para o valor ir de 0 a 59    
    writeDS3231Register(HOR_REG,converte(Ans));
  }
  if((dado[0]=='m' || dado[0]=='M') && dado[1]!='e'){ //m = minuto
    
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  Min alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 59); //restringe para o valor ir de 0 a 59
    writeDS3231Register(MIN_REG,Ans);
  }
  if(dado[0]=='s' || dado[0]=='S'){ //s = segundo
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  Seg alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 59); //restringe para o valor ir de 0 a 59 
    writeDS3231Register(SEC_REG,Ans);
  }
  if(dado[0]=='d' && dado[1]=='s'){ //ds = dia da semana
    //Converte a string para inteiro
    int Ans = strtol(dado+2,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  dia da semana alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 1, 7); //restringe para o valor ir de 0 a 59 
    writeDS3231Register(DIA_REG,Ans);
  }
  if((dado[0]=='d' || dado[0]=='D') && dado[1]!='s'){ //d = dia
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  dia alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 1, 31); //restringe para o valor ir de 0 a 59 
    writeDS3231Register(DAT_REG,Ans);
  }
  if(dado[0]=='m' && dado[1]=='e'){ //me = mes
    //Converte a string para inteiro
    int Ans = strtol(dado+2,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  mes alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 1, 12); //restringe para o valor ir de 0 a 59 
    writeDS3231Register(MES_REG,Ans);
  }
  if(dado[0]=='a' || dado[0]=='A'){ //a = ano
    //Converte a string para inteiro
    int Ans = strtol(dado+1,NULL,10); //Recebe String para converter, 
                                      //ponteiro caractere depois do inteiro, 
                                      //a base
    Serial.print("  ano alterado para: ");
    Serial.println(Ans);
    Ans = constrain(Ans, 0, 99); //restringe para o valor ir de 0 a 59 
    writeDS3231Register(ANO_REG,Ans);
  }
}

byte converte(byte Ans){
  if(lerFormatoHora())
    Ans = de24para12(Ans);
  return Ans;
}

byte readDS3231Register(byte regAddress){
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

byte writeDS3231Register(byte regAddress, byte value){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(regAddress); // set DS3231 register pointer to regAddress
  Wire.write(decToBcd(value));
  Wire.endTransmission();
}

String temperaturaf(){
  byte val = readDS3231Register(TEM_REG);
  String st="";
  st+=val;
  st+=",";
  val = readDS3231Register(TEM2_REG);
  if(val<10)
    st+="0";
  st+=val;
  return st;
}

String tempof(){
  byte val;
  if(lerFormatoHora())
    val = bcdToDec(readDS3231Register(HOR_REG)&0x1F);
  else val = bcdToDec(readDS3231Register(HOR_REG)&0x3F);
  String st="";
  if(val<10)
    st+="0";
  st+=val;
  st+=":";
  val = bcdToDec(readDS3231Register(MIN_REG));
  if(val<10)
    st+="0";
  st+=val;
  st+=":";
  val = bcdToDec(readDS3231Register(SEC_REG));
  if(val<10)
    st+="0";
  st+=val;
  
  if(lerFormatoHora()){
    if(readDS3231Register(HOR_REG)&0x20)
      st+=" PM";
  else st+=" AM";
  }
  return st;
}

String dataf(){
  String st="";
  byte val = readDS3231Register(DIA_REG);
  switch(val){
    case 1:st+="Dom "; break;
    case 2:st+="Seg "; break;
    case 3:st+="Ter "; break;
    case 4:st+="Qua "; break;
    case 5:st+="Qui "; break;
    case 6:st+="Sex "; break;
    case 7:st+="Sab "; 
  }
  val = bcdToDec(readDS3231Register(DAT_REG));
  if(val<10)
    st+="0";
  st+=val;
  st+="/";
  val = bcdToDec(readDS3231Register(MES_REG));
  if(val<10)
    st+="0";
  st+=val;
  st+="/";
  val = bcdToDec(readDS3231Register(ANO_REG));
  if(val<10)
    st+="0";
  st+=val;

  return st;
}

void exibir(){
  byte v1=readDS3231Register(HOR_REG);
  exibirSerial();
  exibirDisplay();
}

void exibirSerial(){
  Serial.print("Tempo: ");
  Serial.print(tempof());
  Serial.print(" Temperatura: ");
  Serial.print(temperaturaf());
  Serial.print(" Data: ");
  Serial.println(dataf());
}

void exibirDisplay(){
  //Tempo
  lcd.setCursor(0,0); //(C,L)
  if(!lerFormatoHora())
    lcd.print("    ");
  else lcd.print("   ");
  lcd.print(tempof());  
  lcd.print("  ");
  
  lcd.setCursor(0,1); //(C,L)
  
  //Temperatura
  if(exibeTemperaturaOuData){
    lcd.print("  Temp.");
    lcd.print(temperaturaf());
    lcd.print((char)223); //Mostra o simbolo do grau quadrado
    lcd.print("c       ");
  }else{
    lcd.print("  ");
    lcd.print(dataf());
  }
}

byte de24para12(byte Ans){
  byte val=0x40; 
  if(Ans>=12)
    val= val|0x20;
  if(!(Ans%12))
    Ans=12;
  else Ans=Ans%12;
  Ans = decToBcd(Ans)|val;
  Ans = bcdToDec(Ans);
  return Ans;
}

//ok
byte de12para24(byte Ans){  //recebe em padrao BCD
    
  if((Ans < 0x72 && Ans >= 0x61)|| Ans==0x52) // entre 1PM e 12AM
    Ans = ((Ans&0x1F)+0x12)%0x24;
  else Ans=Ans&0x1F;
  return Ans;
}

//ok
byte de12para24v1(byte Ans){ //recebe em padrao BCD
  if(Ans == 0x52){ //12am
    Ans = 0x00;
  } else if(Ans == 0x72){ //12pm
    Ans = 0x12;
  } else if(Ans&0x20){ //1pm a 11pm
    Ans = (Ans&0x1F)+0x12;
  } else { //1am a 11am   
    Ans = Ans&0x1F;
  }
  return Ans;
}

byte decToBcd(byte valor){
  return((valor/10*16) + (valor%10));
}

byte bcdToDec(byte valor){ 
  return( (valor/16*10) + (valor%16) );
}
