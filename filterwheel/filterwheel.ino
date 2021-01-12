#include <EEPROM.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

// Debug output on Serial 1 - Tennsy 2.0 pin 7
#define DEBUGOUT Serial1

// Pins In Use!
#define DriveSTEP 1
#define DriveDIR 0
#define DriveENA 2
#define DriveMS1 19
#define DriveMS2 20
#define DriveMS3 21
#define HOME 5
#define BLINKY 11

// Filter Wheel
#define TOTALSTEPS 14840
#define FILTERSTEPS 2120
#define ANTIBACKLASH 100
#define HOMESEARCHSTEPS (TOTALSTEPS+FILTERSTEPS)
#define HOMEOFFSET 1485

#define FILTERS 7
#define NAMECOUNT 9
#define NAMELEN (8*NAMECOUNT)
#define NAMEOFFSET 0
char filternames[NAMELEN];

#define FILTERID "A"

#define MOVESPEED 400
#define MOVEACCEL 2500
#define SEARCHSPEED 400
#define SLOWSEARCHSPEED 20

// A stepper driver
AccelStepper motor(1, DriveSTEP, DriveDIR);

// Only produce a home return response after a whme, not the initial home
int homeresponse=0;

void setup() {
  int n;

  // hardware serial debug at 115.2k
  DEBUGOUT.begin(115200);
  
  // put your setup code here, to run once:
  motor.setEnablePin(DriveENA);
  motor.setPinsInverted(false,false,true);
  motor.setMaxSpeed(MOVESPEED);
  motor.setAcceleration(MOVEACCEL);
  motor.enableOutputs();
  motor.moveTo(HOMESEARCHSTEPS);

  Serial.begin(9600); // USB is always 12 Mbit/sec

  for (n = 0; n < NAMELEN; n++)
    filternames[n] = EEPROM.read(n + NAMEOFFSET);

  pinMode(DriveMS1, OUTPUT);
  pinMode(DriveMS2, OUTPUT);
  pinMode(DriveMS3, OUTPUT);
  digitalWrite(DriveMS1, LOW); // Low-High-Low means 1/4 step microstepping.
  digitalWrite(DriveMS2, HIGH); 
  digitalWrite(DriveMS3, LOW);

  pinMode(BLINKY, OUTPUT);

  pinMode(HOME,INPUT_PULLUP);

  DEBUGOUT.print("Setup Complete\r\n");
}

// Longest command is WLOAD(64 bytes)
// cmdbuffer to hold the characters themselves
// cmndcount for how many in there at the moment
char cmdbuffer[NAMELEN+8];
int cmdcount = 0;
elapsedMillis cmdtimeout;

// Lets have a nice timing led to show something is happening
elapsedMillis blinker;

// Stored response for when move conpletes.
const char *response=0;
// Stored Target Position to allow anti backlash moves (Final move always +ve
long finaltarget=0;
int finalmoveneeded=0;


// Ok Time for a simple state machine
// 0 Startup, setup for homing move
// 1 Initial move running
// 2 Found home switch, back off slowly 
// 3 Move to filter 1 position.
// 10 Default idle, waiting for commands.
// 11 Initial move to below filter pos.
// 12 Final move in +ve dirn
// 99 ERROR State - something went wrong!
int state=0;
int prevstate=-1; // For debug printing

int currentfilter=-1;

long filterpos()
{
  return currentfilter*FILTERSTEPS+HOMEOFFSET;
}

void loop()
{
  // Always flash the LED!
  digitalWrite(BLINKY,(blinker/100)%2);
  //Serial.printf(".");
  if (state!=prevstate)
  {
    DEBUGOUT.printf("State %d\r\n",state);
    prevstate=state;
  }

  if (state==0)
  {
    motor.setSpeed(SEARCHSPEED);
    state=1;
  }
  else if (state==1)
  { // Moving +ve wait till home switch triggers
    if (! digitalRead(HOME))
    {
      motor.setSpeed(-SLOWSEARCHSPEED);
      state=2;
      delay(100); // Bit of a pause for things to stop moving.
    }
    else
      motor.runSpeed();
  }
  else if (state==2)
  { // Moving -ve until home switch opens
    if (digitalRead(HOME))
    { // Found the edge 
      motor.setSpeed(0);
      DEBUGOUT.printf("HOMED %ld\r\n",motor.currentPosition());
      motor.setCurrentPosition(0);
      currentfilter=0;
      motor.moveTo(filterpos());
      state=3;
    }
    else
      motor.runSpeed();
  }  else if (state==3)
  { // Final Move as part of homing
    if (!motor.run())
    {
      DEBUGOUT.print("Home to filter one complete\r\n");
      if (homeresponse)
        Serial.printf("%s\r\n",FILTERID);
      // Flush out and commands sent while homing
      while (Serial.read()!=-1)
        ;
      state=10;
    }
  }
  else if (state==10)
  { // Not moving, waiting for a complete command to happen.
    if (cmdcount>0 && cmdtimeout > 1000)
    {
      DEBUGOUT.printf("Timeout %d %ld %ld %d\r\n", cmdcount, motor.currentPosition(), motor.distanceToGo(),digitalRead(HOME));
      cmdcount = 0;
      cmdtimeout = 0;
    }
    if (Serial.available())
    {
      int n;
      cmdbuffer[cmdcount++] = Serial.read();
      cmdtimeout = 0;
      DEBUGOUT.print("cmdbuffer=\"");
      for (n=0;n<cmdcount;n++)
        if (isprint(cmdbuffer[n]))
          DEBUGOUT.printf("%c",cmdbuffer[n]);
        else
          DEBUGOUT.printf(".");
      DEBUGOUT.print("\"\r\n");
    }
    if (cmdcount!=0 && cmdbuffer[0]!='W') // All Commands starts with 'W'
    {
      DEBUGOUT.print("Not W!\r\n");
      cmdcount=0;
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WSMODE", 6) == 0)
    {
      DEBUGOUT.print("WSMODE\r\n");
      Serial.print("!\n\r");
      cmdcount=0;
    }
    else if (cmdcount == 5 && strncmp(cmdbuffer, "WHOME", 5) == 0)
    {
      DEBUGOUT.print("WHOME\r\n");
      homeresponse=1;
      state=0;
      cmdcount=0;
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WIDENT", 6) == 0)
    {
      DEBUGOUT.print("WIDENT\r\n");
      Serial.printf("%s\r\n",FILTERID);
      cmdcount=0;
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WFILTR", 6) == 0)
    {
      DEBUGOUT.print("WFILTR\r\n");
      Serial.printf("%d\r\n",currentfilter+1);
      cmdcount=0;
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WGOTO", 5) == 0)
    {
      int r=cmdbuffer[5]-'1';
      DEBUGOUT.printf("WGOTO %d\r\n",r);
      if (r<0 || r>=FILTERS)
        Serial.printf("ER=5\n\r"); // Invalid filter
      else
      {
        if (currentfilter!=r)
        {
          currentfilter=r;
          if (filterpos()>motor.currentPosition())
          {
            motor.moveTo(filterpos());
            state=12;
          }
          else
          {
            motor.moveTo(filterpos()-ANTIBACKLASH);
            state=11;
          }
          response="*\n\r";
        }
        else 
          Serial.print("*\n\r");
      }
      cmdcount=0;
    }
    else if (cmdcount == 5 && strncmp(cmdbuffer, "WREAD", 5) == 0)
    {
      int n;
      DEBUGOUT.print("WREAD\r\n");
      for (n=0;n<NAMELEN;n++)
        Serial.printf("%c",filternames[n]);
      Serial.print("\r\n");
      cmdcount=0;
    }
    else if (cmdcount == (NAMELEN+5+2) && strncmp(cmdbuffer, "WLOAD", 5) == 0)
    {
      int n;
      DEBUGOUT.print("WLOAD\r\n");
      for (n = 0; n < NAMELEN; n++)
      {
        EEPROM.write(n+NAMEOFFSET, cmdbuffer[n + 5 + 2]);
        filternames[n] = cmdbuffer[n + 5 + 2];
      }
      Serial.printf("!\r\n",currentfilter+1);
      cmdcount=0;
    }
    else if (cmdcount == 5 && strncmp(cmdbuffer, "WEXIT", 5) == 0)
    {
      DEBUGOUT.print("WEXIT\r\n");
      Serial.print("END\r\n");
      cmdcount=0;
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WDEBUG", 6) == 0)
    {
      DEBUGOUT.print("WDEBUG\r\n");
      Serial.printf("State: %d\n\r",state);
      Serial.printf("Pos: %ld\n\r",motor.currentPosition());
      Serial.printf("Filter: %d\n\r",currentfilter);
      Serial.printf("Filterpos: %ld\n\r",filterpos());
      cmdcount=0;
    }
    else if (cmdcount == 2 && strncmp(cmdbuffer, "W+", 2) == 0)
    {
      DEBUGOUT.print("W+\r\n");
      motor.move(1);
      Serial.printf("%ld %ld\n\r",motor.currentPosition(),motor.targetPosition());
      state=12;
      cmdcount=0;
    }
    else if (cmdcount == 2 && strncmp(cmdbuffer, "W-", 2) == 0)
    {
      DEBUGOUT.print("W-\r\n");
      motor.move(-1); 
      Serial.printf("%ld %ld\n\r",motor.currentPosition(),motor.targetPosition());
      state=12;
      cmdcount=0;
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WFLASH", 6) == 0)
      _reboot_Teensyduino_();
  }
  else if (state==11)
  { // Moving, needs final anti-backlash move once complete.
    if (!motor.run())
    {
      motor.moveTo(filterpos());
      state=12;
    }
  }
  else if (state==12)
  { // Moving to final position.
    if (!motor.run())
    {
      Serial.print("*\n\r");
      state=10;
    }
  }
}
/*
void oldloop() {
  // put your main code here, to run repeatedly:
  motor.run();
  //Serial.printf("%ld\n",motor.currentPosition());
  //motor.stop();

  digitalWrite(BLINKY,(blinker/100)%2);

  if (homing)
  {
    // homing=1 First stage, move until we hit the home switch
    if (homing==1)
    {
      if (motor.distanceToGo())
      {
        if (! digitalRead(HOME))
        {
          motor.stop();
          homing=2;
          motor.move(-100);
        }
      }
      else
        Serial.print("Homing failed");
    }
    // homing=2 Now back off until the home switch disengages.
    if (homing==2)
    {
     if (motor.distanceToGo())
      {
        if (digitalRead(HOME))
        {
          motor.stop();
          homing=0;
          Serial.printf("HOMED %ld\n\r",motor.currentPosition());
          motor.setCurrentPosition(HOMEPOSITION);
          motor.moveTo(0);
        }
      }
      else
        Serial.print("Homing failed");
    }
  }

  if (finalmoveneeded && motor.distanceToGo()==0)
  {
    motor.moveTo(finaltarget);
    finalmoveneeded=0;
    return;
  }

  if (response!=0 && motor.distanceToGo()==0)
  {
    Serial.print(response);
    response=0;
    motor.setMaxSpeed(FASTSPEED);
    motor.setAcceleration(FASTACC);
    return;
  } 
  // Timeout, reset the buffer.
  if (cmdtimeout > 1000)
  {
    //Serial.printf("Timeout %d %ld %ld %d\r\n", cmdcount, motor.currentPosition(), motor.distanceToGo(),digitalRead(HOME));
    cmdcount = 0;
    cmdtimeout = 0;
  }

  if (Serial.available())
  {
    // If we get a serial command in the middle of a move, stop at once!
    //if (motor.distanceToGo())
    //  motor.stop();

    cmdbuffer[cmdcount++] = Serial.read();
    cmdtimeout = 0;

    if (cmdbuffer[0]!='W')
    {
      cmdcount=0;
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WSMODE", 6) == 0)
      Serial.print("!\n\r");
    else if (cmdcount == 5 && strncmp(cmdbuffer, "WHOME", 5) == 0)
    {
      //motor.moveTo(0);
      response="H\n\r"; // I'm filter wheel H!
      motor.moveTo(HOMESEARCHSTEPS);
      homing=1;
      
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WIDENT", 6) == 0)
    {
      Serial.print("F\n\r"); // I'm filter wheel H!
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WFILTR", 6) == 0)
    {
      Serial.printf("%d\n\r", motor.currentPosition()); // Needs translating to filter id
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WGOTO", 5) == 0)
    {
      int r=cmdbuffer[5]-'1';
      if (r<0 || r>=FILTERS)
        Serial.printf("ER=5\n\r"); // Invalid filter
      else
      {
        motor.moveTo(r*FILTERSTEPS-ANTIBACKLASH);
        finaltarget=r*FILTERSTEPS;
        finalmoveneeded=1;
        response="*\n\r";
      }
    }
    else if (cmdcount == 5 && strncmp(cmdbuffer, "WREAD", 5) == 0)
    {
      Serial.write(filternames, 64);
      Serial.print("\n\r");
    }
    // WLOADC*FILTER01FILTER02FILTER03FILTER04FILTER05FILTER06FILTER07FILTER08
    else if (cmdcount == 71 && strncmp(cmdbuffer, "WLOAD", 5) == 0)
    {
      int n;
      for (n = 0; n < NAMELEN; n++)
      {
        EEPROM.write(n, cmdbuffer[n + 5 + 2]);
        filternames[n] = cmdbuffer[n + 5 + 2];
      }
      Serial.printf("!\n\r");
    }
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WEXITS", 6) == 0)
      Serial.print("END\n\r");
    else if (cmdcount == 6 && strncmp(cmdbuffer, "WFLASH", 6) == 0)
      _reboot_Teensyduino_();
  }
}
*/
