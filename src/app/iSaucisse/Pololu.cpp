/************************************************************/
/*    FILE: Pololu.cpp
/*    ORGN: Toutatis AUVs - ENSTA Bretagne
/*    AUTH: Simon Rohou
/*    DATE: 2015
/************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "Pololu.h"

#ifdef _WIN32
  #define O_NOCTTY 0
#else
  #include <termios.h>
#endif

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
  #include <windows.h>
  inline void delay(unsigned long ms)
  {
    Sleep(ms);
  }
#else /* POSIX */
  #include <unistd.h>
  inline void delay(unsigned long ms)
  {
    usleep(ms * 1000);
  }
#endif 

#define HIGH_LEVEL 7000
#define LOW_LEVEL  5000

using namespace std;

Pololu::Pololu(string device_name)
{
  bool init_success = true;
  m_error = false;
  m_error_message = "";

  const char * device = device_name.c_str();

  // Open the Maestro's virtual COM port
  m_pololu = open(device, O_RDWR | O_NOCTTY);
  if(m_pololu == -1)
  {
    perror(device);
    setErrorMessage("error (loading " + string(device) + ")");
    cout << "Pololu: " << m_error_message << endl;
    init_success = false;
  }

  #ifdef _WIN32
    _setmode(m_pololu, _O_BINARY);
  #else
    struct termios options;
    tcgetattr(m_pololu, &options);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~(ONLCR | OCRNL);
    tcsetattr(m_pololu, TCSANOW, &options);
  #endif

  init_success &= setLeftThrusterValue(0.) >= 0;
  init_success &= setRightThrusterValue(0.) >= 0;
  init_success &= setVerticalThrusterValue(0.) >= 0;

  if(!init_success)
    bipError();

  else
    bipOnStartUp();
}

Pololu::~Pololu()
{
  close(m_pololu);
}

bool Pololu::isReady(string &error_message)
{
  error_message = m_error_message;
  return m_error == false;
}

int Pololu::turnOnRelay(int id, bool turned_on)
{
  return setTarget(id, turned_on ? HIGH_LEVEL : LOW_LEVEL);
}

int Pololu::reset(bool all_on)
{
  int success = turnOnRelay(1, all_on);
  success &= turnOnRelay(0, !all_on);

  delay(50);

  success &= turnOnRelay(3, all_on);
  success &= turnOnRelay(2, !all_on);
  
  delay(50);

  success &= turnOnRelay(5, all_on);
  success &= turnOnRelay(4, !all_on);
  
  delay(50);

  success &= turnOnRelay(7, all_on);
  success &= turnOnRelay(6, !all_on);
  
  delay(50);

  success &= turnOnRelay(9, all_on);
  success &= turnOnRelay(8, !all_on);
  
  delay(50);

  success &= turnOnRelay(11, all_on);
  success &= turnOnRelay(10, !all_on);
  
  delay(50);

  success &= turnOnRelay(12, all_on);

  delay(50);
  
  if(success < 0)
    return -1;

  return 0;
}

int Pololu::turnOnBistableRelay(int id_on, int id_off, bool turned_on)
{
  int success_on, success;

  success_on = turnOnRelay(id_on, turned_on);
  success = turnOnRelay(id_off, !turned_on);

  if(success_on < 0 || success < 0)
    return -1;

  delay(50);
  success_on = turnOnRelay(id_on, false);
  success = turnOnRelay(id_off, false);

  if(success_on < 0 || success < 0)
    return -1;
  return 0;
}

int Pololu::setThrusterValue(int id, double value)
{
  // value in [-1.0;1.0]
  double mean = (LOW_LEVEL + HIGH_LEVEL) / 2;
  double radius = (HIGH_LEVEL - LOW_LEVEL) / 2;
  return setTarget(id, mean + radius * value);
}

int Pololu::setLeftThrusterValue(double value)
{
  return setThrusterValue(22, value);
}

int Pololu::setRightThrusterValue(double value)
{
  return setThrusterValue(23, value);
}

int Pololu::setVerticalThrusterValue(double value)
{
  return setThrusterValue(21, value);
}

void Pololu::buzzOn()
{
  setTarget(13, HIGH_LEVEL);
}

void Pololu::buzzOff()
{
  setTarget(13, LOW_LEVEL);
}

void Pololu::bipOnStartUp()
{
  buzzOn();
  delay(80);
  buzzOff();
  delay(50);
  buzzOn();
  delay(80);
  buzzOff();
}

void Pololu::bipError()
{
  for(int i = 0 ; i < 3 ; i++)
  {
    buzzOn();
    delay(2000);
    buzzOff();
    delay(2000);
  }
}

int Pololu::emitBips(int bip_number)
{
  for(int i = 0 ; i < bip_number ; i++)
  {
    buzzOn();
    delay(80);
    buzzOff();
    delay(50);
  }
}

int Pololu::setTarget(unsigned char channel, unsigned short target)
{
  // Sets the target of a Maestro channel.
  // See the "Serial Servo Commands" section of the user's guide.
  // The units of 'target' are quarter-microseconds.
  unsigned char command[] = {0x84, channel, target & 0x7F, target >> 7 & 0x7F};

  if(write(m_pololu, command, sizeof(command)) == -1)
  {
    setErrorMessage("error (writing)");
    perror(m_error_message.c_str());
    return -1;
  }

  return 0;
}

void Pololu::setErrorMessage(string message)
{
  if(m_error_message == "")
    m_error_message = message;
  m_error = true;
}