//================================================
//  Developed by Benny Saxen, ADCAJO
//================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <math.h> 
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/stat.h>

#define LOW    0
#define HIGH   1
#define INPUT  1
#define OUTPUT 2

#define FORWARD  1
#define BACKWARD 2

#define BYTE   1
#define BIN    2
#define OCT    3
#define DEC    4
#define HEX    5

#define CHANGE  1
#define RISING  2
#define FALLING 3
#define MAX_SERIAL 100
#define MAX_LOG  200
#define MAX_STEP 2000
#define MAX_LOOP 2000
#define MAX_PIN_ANALOG 6
#define MAX_PIN_DIGITAL 14

#define SIZE_ROW 120

#define UNO_H  16
#define UNO_W  61
#define UNO_COLOR 6

#define MSG_H  20
#define MSG_W  61
#define MSG_COLOR 6

#define LOG_H  40
#define LOG_W  40
#define LOG_COLOR 3

#define SER_H  40
#define SER_W  30
#define SER_COLOR 4

#define DP 5
#define AP 11
#define RF 1
#define ER 1
#define SR 20

#define ON     1
#define OFF    0

#define YES    1
#define NO     2

#define RUN    1
#define ADMIN  2

// Configuration
int currentStep = 0;
int currentLoop = 0;
int g_loops=0,g_steps=0;

int g_value = 0;
int s_mode = ADMIN;

void (*interrupt0)();
void (*interrupt1)();

char  simulation[MAX_STEP][SIZE_ROW];
int   loopPos[MAX_LOOP];

#define FREE   0
#define RX     3
#define TX     4

int   s_row,s_col;
int   digPinPos[MAX_PIN_DIGITAL];
int   anaPinPos[MAX_PIN_ANALOG];
char  appName[80];

int   interruptMode[2];
int   digitalMode[MAX_PIN_DIGITAL];
int   paceMaker = 0;
int   baud = 0;
int   error = 0;
int   logging = YES;
char  logBuffer[MAX_LOG][100];
int   logSize = 1;
int   serialSize = 1;
int   serialMode = OFF;
char  serialBuffer[100][100];
int   rememberNewLine;
char  textPinModeIn[MAX_PIN_DIGITAL][80];
char  textPinModeOut[MAX_PIN_DIGITAL][80];
char  textDigitalWriteLow[MAX_PIN_DIGITAL][80];
char  textDigitalWriteHigh[MAX_PIN_DIGITAL][80];
char  textAnalogWrite[MAX_PIN_DIGITAL][80];
char  textAnalogRead[MAX_PIN_ANALOG][80];
char  textDigitalRead[MAX_PIN_DIGITAL][80];
int   scenAnalog    = 0;
int   scenDigital   = 0;
int   scenInterrupt = 0;

// Configuration default values
int   confDelay   = 100;
int   confLogLev  =   1;
int   confLogFile =   0;
char  confSketchFile[200];
char  confServuinoFile[200];

WINDOW *uno,*ser,*slog,*msg;
static struct termios orig, nnew;

#include "simuino.h"
#include "simuino_lib.c"
#include "decode_lib.c"

//====================================
void runStep(int dir)
//====================================
{
  char row[SIZE_ROW],event[SIZE_ROW],temp[SIZE_ROW],mode[5],*p,temp2[SIZE_ROW],temp3[SIZE_ROW];
  int res  = 0,value = 0;
  int step = 0,pin,x,y;

  if(dir == FORWARD)currentStep++;
  if(dir == BACKWARD)currentStep--;

  if(currentStep > g_steps)return; // Not possible to step beyond simulation length

  if(currentStep > loopPos[currentLoop+1] && loopPos[currentLoop+1] !=0)
    {
      currentLoop++;
    }

  res = readEvent(event,currentStep);
  if(res != 0)
    {
      if(p=strstr(event,"pinMode"))
	{
	  sscanf(event,"%d %s %s %d",&step,temp,mode,&pin);
	  if(strstr(mode,"IN"))pinMode(pin,INPUT);
	  if(strstr(mode,"OUT"))pinMode(pin,OUTPUT);
	}
      else if (p=strstr(event,"digitalRead"))
	{
	  sscanf(event,"%d %s %d %d",&step,temp,&pin,&value);
	  g_value = value;
	  digitalRead(pin);
	}
      else if (p=strstr(event,"digitalWrite"))
	{
	  sscanf(event,"%d %s %s %d",&step,temp,mode,&pin);
	  if(strstr(mode,"LOW"))digitalWrite(pin,LOW);
	  if(strstr(mode,"HIGH"))digitalWrite(pin,HIGH);
	}
      else if (p=strstr(event,"analogRead"))
	{
	  sscanf(event,"%d %s %d %d",&step,temp,&pin,&value);
	  g_value = value;
	  analogRead(pin);
	}
      else if (p=strstr(event,"analogWrite"))
	{
	  sscanf(event,"%d %s %d %d",&step,temp,&pin,&value);
	  analogWrite(pin,value);
	}
      else if (p=strstr(event,"Serial:begin"))
	{
	  sscanf(event,"%d %s %d",&step,temp,&x);
	  Serial.begin(x);
	}
      else if (p=strstr(event,"Serial:print(int)"))
	{
	  sscanf(event,"%d %s %d",&step,temp,&x);
	  Serial.print(x);
	}
      else if (p=strstr(event,"Serial:print(int,int)"))
	{
	  sscanf(event,"%d %s %d",&step,temp,&x,&y);
	  Serial.print(x,y);
	}
      else if (p=strstr(event,"Serial:print(char)"))
	{
	  //sscanf(event,"%d %s %s",&step,temp,temp2);
	  getString(event,temp3);
	  Serial.print(temp3);
	}
      else if (p=strstr(event,"Serial:println(int)"))
	{
	  sscanf(event,"%d %s %d",&step,temp,&x);
	  Serial.println(x);
	}
      else if (p=strstr(event,"Serial:println(char)"))
	{
	  //sscanf(event,"%d %s %s",&step,temp,temp2);
	  getString(event,temp3);
	  Serial.println(temp3);
	}
      else if (p=strstr(event,"delay"))
	{
	  sscanf(event,"%d %s %d",&step,temp,&x);
	  delay(x);
	}
      else if (p=strstr(event,"attachInterrupt"))
	{
	  sscanf(event,"%d %s %d",&step,temp,&x);
	  attachInterrupt(x,0,0);
	}
      else if (p=strstr(event,"detachInterrupt"))
	{
	  sscanf(event,"%d %s %d",&step,temp,&x);
	  detachInterrupt(x);
	}
      else
	unimplemented(temp);

      show(slog);
    }
  else
    showError("Unable to step ",currentStep);

  unoInfo();

  return;
}    


//====================================
void openCommand()
//====================================
{
  struct stat st;
  int ch,nsteps=1000,x;
  char *p,str[80],fileName[80],temp[80],syscom[120];

  s_mode = ADMIN;

  strcpy(fileName,"help_command.txt");
  readMsg(fileName);

  while(strstr(str,"ex") == NULL)
  {
    wmove(uno,UNO_H-2,1);
    wprintw(uno,"                                                  ");
    mvwprintw(uno,UNO_H-2,1,">>");
    show(uno);
    wmove(uno,UNO_H-2,3);
    wgetstr(uno,str);

    if(p=strstr(str,"run"))
      {
        runMode();
        strcpy(fileName,"help_command.txt");
        readMsg(fileName);
      }
    if(p=strstr(str,"conf"))
      {
	strcpy(fileName,"config.txt");
        readMsg(fileName);
      }
    if(p=strstr(str,"err"))
      {
	strcpy(fileName,"servuino/data.error");
        readMsg(fileName);
      }
    if(p=strstr(str,"help"))
      {
        strcpy(fileName,"help_command.txt");
        readMsg(fileName);
      }
    if(p=strstr(str,"delay"))
      {
       sscanf(p,"%s %d",temp,&confDelay);
       if(confDelay >=0 && confDelay < 1000)
          saveConfig();
      }
    if(p=strstr(str,"log"))
      {
       sscanf(p,"%s %d",temp,&confLogLev);
       if(confLogLev >=0 && confLogLev < 4)
          saveConfig();
      }
    if(p=strstr(str,"record"))
      {
       sscanf(p,"%s %d",temp,&confLogFile);
       if(confLogFile >=0 && confLogFile < 2)
          saveConfig();
      }
    if(p=strstr(str,"loop")) //status
      {
          showLoops();
      }
    if(p=strstr(str,"scen")) // scenario
      {
          showScenario(confSketchFile);
      }
    if(p=strstr(str,"sys"))
      {
	p = p+4;
        wLog(p,-1,-1);
        x=system(p);
      }
    if(p=strstr(str,"clear"))
      {
	sprintf(syscom,"rm servuino/sketch.pde;rm servuino/data.su;");
        x=system(syscom);
      }
    if(p=strstr(str,"load"))
      {
	putMsg("Load Sketch...");
        loadSketch(confSketchFile);
        sscanf(p,"%s %d",temp,&nsteps);
        if(nsteps > 0 && nsteps < MAX_STEP)
        {
          sprintf(syscom,"cd servuino;./servuino %d;",nsteps);
          x=system(syscom);
          iDelay(1000);
          initSim();
          resetSim();
          readSimulation(confServuinoFile);
          readSketchInfo();
	  init();
          unoInfo();
	  strcpy(fileName,"servuino/data.error");
	  readMsg(fileName);
	}
      }
    if(p=strstr(str,"sketch"))
      {
        sscanf(p,"%s %s",temp,confSketchFile);

	if(stat(confSketchFile,&st) == 0)
	  {
	    saveConfig();
	  }
        else
	  putMsg("Sketch not found !");

      }
    if(p=strstr(str,"serv")) // Servuino data file
      {
        sscanf(p,"%s %s",temp,confServuinoFile);
        saveConfig();
      }


  }
  //wmove(uno,UNO_H-2,1);
  //wprintw(uno,"                                                  ");	
  //show(uno);
  //putMsg("Run mode");
}

//====================================
void runMode()
//====================================
{
  int ch;
  char tempName[80];
  strcpy(tempName,"help.txt");

  s_mode = RUN;

  readMsg(tempName);

  wmove(uno,UNO_H-2,1);
  wprintw(uno,"                                                  ");
  mvwprintw(uno,UNO_H-2,1,"**");
  show(uno);
  wmove(uno,UNO_H-2,3);

  while((ch!='q')&&(ch!='x'))  
    {
      ch = getchar();

      if (ch=='c')
        {
          openCommand();
        }
      if (ch=='h')
	{
	  readMsg(tempName);
	}
      if (ch=='g')
	{
	  runAll();
	}
      if (ch=='a')
	{
           resetSim();
           init();
           unoInfo();
           mvwprintw(uno,UNO_H-2,1,"**");
           show(uno);
	}
      if (ch=='r')
	{
	  runLoop();
	}
      if (ch=='s') 
	{
	  runStep(FORWARD);
	}
      if (ch=='b') 
	{
	  runStep(BACKWARD); 
	}
      if (ch=='l') 
	{
	  confLogLev++;
	  if(confLogLev > 3)confLogLev = 0;
	  // Todo save to config.txt
	}
      if (ch=='+') 
	{
	  confDelay = confDelay + 10;
	  // Todo save to config.txt
	}
      if (ch=='-') 
	{
	  confDelay = confDelay - 10;
	  if(confDelay < 0)confDelay = 0;
          if(confDelay > 1000)confDelay = 1000;
	  // Todo save to config.txt
	}
      if (ch=='f') 
	{
	  confLogFile++;
	  if(confLogFile > 1)confLogFile = 0;
	  // Todo save to config.txt
	}
    }
}
//====================================
int main(int argc, char *argv[])
//====================================
{
  int ch;
 
  init();
  initSim();
  resetSim();
  readConfig();
  readSimulation(confServuinoFile);
  readSketchInfo();
  unoInfo();

  if(confLogFile == YES)resetFile("log.txt");
  //readMsg(tempName);

  openCommand();
  
  //tcsetattr(0,TCSANOW, &orig);
  delwin(uno);
  delwin(ser);
  delwin(slog);
  delwin(msg);
  endwin();
}
//====================================
// End of file
//====================================

