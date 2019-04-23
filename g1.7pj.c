#include <16F886.h>
#device ADC=10 *=16

#include <math.h>

#FUSES NOWDT //No Watch Dog Timer
#FUSES PUT //Power Up Timer
#FUSES NOMCLR //Master Clear pin not enabled
#FUSES NOPROTECT //Code not protected from reading
#FUSES NOCPD //No EE protection
#FUSES BROWNOUT //Brownout reset
#FUSES IESO //Internal External Switch Over mode enabled
#FUSES FCMEN //Fail-safe clock monitor enabled
#FUSES NOLVP //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NODEBUG //No Debug mode for ICD
#FUSES NOWRT //Program memory not write protected
#FUSES BORV40 //Brodddddwnout reset at 4.0V
#FUSES RESERVED //Used to set the reserved FUSE bits
#FUSES INTRC_IO 

#use delay(clock=8M)
//define LCD Display 
////////////////////////////////////////////////////////////
#define SLAVE_ADDRESS  0xB0
#define DISPLAY_ADDRESS 0xB4
#define REGISTER_SIZE  8   
#define DISPLAY_CMD_SEND_VALUE 2
#define DISPLAY_CMD_SEND_LONG_TEXT 5
#define DISPLAY_CMD_CLS 6
#define DISPLAY_CMD_SETPOS 8
////////////////////////////////////////////////////////////
//define LCD Display 
////////////////////////////////////////////////////////////

// setup the I2C port
#use i2c(MASTER, I2C1, FORCE_HW)
#use rs232(baud=9600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8)
#define Trig PIN_C0
#define Echo PIN_C1
float setDistanceFromUltrasonic(){
   int16 time = 0;
   float distance;
   output_low(Trig);
   delay_ms(2);
   output_high(Trig);
   delay_ms(20);
   output_low(Trig);
   while(input(Echo) == 0)
   {
     time = 0;
   }
   while(input(Echo) == 1)
   {    
     time++;
     delay_us(1);
   }
   distance = (time * 50/48.0);
   return (distance*0.1)*1.875;
}


unsigned int32 count=0;
int counter1=0;
int counter2=0;
unsigned int16 counter16 =0;
signed int duty=0,duty2=0;
int1 dob =0;
int1 flag =0;
float distance;
int16 counter1s =0;
int1 fs =0;

#INT_TIMER1
void timer1_isr() {
   
   set_timer1(65486);
   count++;
   counter1++;
   counter2++;
   counter16++;
   counter1s++;
   if(counter1s == 10000) {counter1s = 0; fs =1;}
   if (counter1 == 100) { counter1 = 0;} 
   if(counter2 == 100) {counter2 =0;}
   if(counter16 == 100) {counter16=0; dob =1;}
   if (counter1 == 0) { output_high(PIN_B5);}  // beginning of a new period
   if(counter2 == 0) { output_high(PIN_B6);}
   if (counter1 ==duty) {output_low(PIN_B5);}   
   if (counter2 == duty2) {output_low(PIN_B6);}
}

void displayLongText(char* text);
void displayValue(int16 value[]);
void clearDisplay();
void setDisplayPos(int pos);
void digitalMag(int16 sensor);
void control(float distance, float);
void setupfirst();



void main() {
   int16 s[2]; // o indicate magnetic, 1 indicate R
   float rep =0,showrep,countrpm;
   float v_convoyer=0;
   float v_car=0;
   unsigned int32 time1=0;
   unsigned int32 time2=0;
   unsigned int32 dif=0;
   char dist[33];
   char rpm[33];
   setupfirst();   
   int check =0;
   int1 b=0,r=0;
    while (1) {
         distance = setDistanceFromUltrasonic();
         //1 Magnetic
         set_adc_channel( 0 ); // set the ADC chaneel to read
         delay_us(100); // wait for the sensor reading to finish
         
         s[0] = read_adc();
         //2 R
         set_adc_channel( 1 ); // set the ADC chaneel to read
         delay_us(100); // wait for the sensor reading to finish
         s[1] = read_adc();
         if((s[1]) < 390 ){
         s[1] = 390;
         }
         
         
         
         duty = (s[1]-390)/11.2857 ;
         
         
         printf("Mag = %lu R = %lu distance = %.1f duty = %u duty2 =%u\n",s[0],s[1],distance,duty,duty2);
         if(check >= 2) {
            check =0;
            time1=0;
            time2=0;
            dif =0;
         }
         //button
         if(!input(PIN_B7)){
            if(b==0) {
//!         duty2 = ((duty2 + 5)%100) +1;
            check++;
               time1 = count;
               dif=time1-time2;
               time2=time1;      
            b=1;
            printf("dif: %.2f check: %u \n",((float)dif*0.0002),check);
            }
         }else {
            b=0;
         }
         
         
         
         


         //rpm meter
         digitalMag(s[0]);
//!         printf("count %lu time1  %lu time2 %lu dif %lu flag %u\n",count,time1,time2,dif,flag);
         if(flag == 1){
            if(r ==0){
               check++;
               time1 = count;
               dif=time1-time2;
               time2=time1; 
               rep = (5000/(float)dif)*60;
               r=1;
               
            }
         }else {
            r =0;
         }
         
         countrpm = duty*1.85;
         v_convoyer = 2*3.14*(countrpm/60)*2; // v convoyer in cm/s
         v_car = v_convoyer;
         if(fs == 1) {
            showrep = (fabs(countrpm-rep) < 20) ? rep : countrpm;
            fs =0;
         }
         
         if(duty ==0){
         
         showrep =0;
         }
         
//!            printf("RPM : %.2f mag : %lu count : %lu dif : %lu t1 : %lu t2 : %lu\n", rep,s[0],count,dif,time1,time2);
         if(dob == 1){
                     //main
            control(distance,v_convoyer);
            dob = 0;
         }
         
//!         printf("duty2 %u dist %.2f duty %u\n",duty2,distance,duty);
         clearDisplay();
         delay_us(100);
         sprintf(dist , "V = %.1lf cm/s" ,v_convoyer);
         setDisplayPos(1);
         displayLongText(dist);
   
         
         setDisplayPos(17);
         sprintf(rpm, "RPM = %.1f", showrep);
         displayLongText(rpm);
       
       
    }
}

//setup
void setupfirst(){
   enable_interrupts(INT_TIMER1);
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
   enable_interrupts(GLOBAL);
   set_timer1(65486);
   setup_adc_ports(sAN0|sAN1); // setup PIN A0,A1 as analog input  
   setup_adc( ADC_CLOCK_INTERNAL ); 
}

//controller
void control(float distance,float v_car){
   if(distance >= 10 && distance <= 55)
   {
      if(distance > 30 && distance < 40){
         if(distance >=31 ){
            if(duty2+50 >= 100) {
               duty2 =100;
            }else {
               duty2 += 50;
            }
         }else{
            if(v_car*4.88 >= 100){
               duty2 =100;
            }else {
            
            duty2 = v_car*4.88;
            }
         }
      }else if(distance < 30) {
         duty2 =0;
      }else if(distance > 40){
         duty2 = 100;
      }
   }
}

//get digtal mag

void digitalMag(int16 sensor){
   if(sensor > 560){
        flag =1;
   }else if(sensor <= 517){
        flag =0;
   }
}


///////////////////////////////////////////////////////////////////////////
//LCD Display Funtion 
///////////////////////////////////////////////////////////////////////////
void displayLongText(char* text) {
   int i ;
   i2c_start();
   i2c_write(DISPLAY_ADDRESS);
   i2c_write(DISPLAY_CMD_SEND_LONG_TEXT);

   for(i=0;text[i]!='\0';i++)
   {
   i2c_write(text[i]);

   }
   i2c_write('\0');
   i2c_stop();

   //delay_ms(100);

}


void displayValue(int16 value[]) {
   i2c_start();
   i2c_write(DISPLAY_ADDRESS);
   i2c_write(DISPLAY_CMD_SEND_VALUE);
   i2c_write((int)(value>>8)); // high byte
   i2c_write((int)(value & 0xFF)); // low byte
   i2c_write('\0');
   i2c_stop();
   
}

void clearDisplay(){
   i2c_start();
   i2c_write(DISPLAY_ADDRESS);
   i2c_write(DISPLAY_CMD_CLS);
   i2c_stop();
}

void setDisplayPos(int pos){
   i2c_start();
   i2c_write(DISPLAY_ADDRESS);
   i2c_write(DISPLAY_CMD_SETPOS);
   i2c_write(pos);
   i2c_write('\0');
   i2c_stop();
}
///////////////////////////////////////////////////////////////////////////
//LCD Display Funtion 
///////////////////////////////////////////////////////////////////////////
