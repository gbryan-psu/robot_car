/**************************************************
* CMPEN 473, Spring 2021, Penn State University
* 
* Homework 5 Main Program
* On 3/01/2021
* By Gabien Bryan
* 
***************************************************/

/* Homework 5 Main Program
 * Controlling a small car in C for 
 * Raspberry Pi 4 computer
 * Key hit controls are as follows:
 * 
 * (1) Stop:      ' s '
 * (2) Forward:   ' w '
 * (3) Backward:  ' x '
 * (4) Faster:    ' i '
 * (5) Slower:    ' j '
 * (6) Left:      ' a '
 * (7) Right:     ' d '
 * (8) Quit:      ' q ' to quit all program proper (without cnt’l c, and without an Enter key)
 * 
 * Comments: Could not get backward. I believe the code is close to what is needed,
 * but I could not get it to work successfully. It still goes forward after hitting 'x'. 
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <softPwm.h>
#include <pthread.h>
#include <pigpio.h>
#include <signal.h>
#include "import_registers.h"
#include "gpio.h"
#include "cm.h"
#include "pwm.h"
#include "spi.h"
#include "io_peripherals.h"
#include "enable_pwm_clock.h"


struct thread_parameter
{
  int                             rPin;
  int                             lPin;
  int                             rSpeed;
  int                             lSpeed;
  bool                            run;
}typedef thread_parameter;

//threads global to be accessed in multiple functions
pthread_t                   forward_handle;
pthread_t                   backward_handle;
struct thread_parameter     *forward_parameter;
struct thread_parameter     *backward_parameter;
//global speed variables to keep track of car current overal speed
int glob_rSpeed;
int glob_lSpeed;

bool done;
bool turnLeft = false;
bool turnRight = false;

/* For the steps, start speed is 100, top speed is 255
 * 32 steps: increase/decrease of 5 /this is the steps for increasing/decreasing speed/
 * 16 steps: increase/decrease of 10
*/

int get_pressed_key(void) //get keyboard input
{
  struct termios  original_attributes;
  struct termios  modified_attributes;
  int             ch;

  tcgetattr( STDIN_FILENO, &original_attributes );
  modified_attributes = original_attributes;
  modified_attributes.c_lflag &= ~(ICANON | ECHO);
  modified_attributes.c_cc[VMIN] = 1;
  modified_attributes.c_cc[VTIME] = 0;
  tcsetattr( STDIN_FILENO, TCSANOW, &modified_attributes );

  ch = getchar();

  tcsetattr( STDIN_FILENO, TCSANOW, &original_attributes );

  return ch;
}

void *looopMotors( void * arg)
{
  struct thread_parameter * parameter = (struct thread_parameter *)arg;
  
  while(!done)
  {
    while(parameter->run) // checks if moving forward or backward 
    {
      gpioPWM(parameter->rPin, 255);
      gpioPWM(parameter->lPin, 0); // not sure why this is happening 
      gpioPWM(13, parameter->rSpeed);
      gpioPWM(12, parameter->lSpeed);
    }
  }
  return 0;
}

void *looopKeyboard( void * arg) // the control for car
{
  
  printf("HW5> ");
  while (!done)
  {
    char key = get_pressed_key();
    if(key == 's') //stop
    {
      printf("%c\n", key);
      forward_parameter->run = false;
      backward_parameter->run = false;
      gpioPWM(12, 0);
      gpioPWM(13, 0);
      usleep(500000);
    }
    if(key == 'w') //forward
    {
      printf("%c\n", key);
      if (!backward_parameter->run)
      {
        forward_parameter->run = true;
      }
      else
      {
        backward_parameter->run = false;
        usleep(1000000);
        forward_parameter->run = true;
      }
    }
    if (key == 'x') //backward
    {
      printf("%c\n", key);
      if (!forward_parameter->run)
      {
        backward_parameter->run = true;
      }
      else
      {
        forward_parameter->run = false;
        usleep(1000000);
        backward_parameter->run = true;
      }
    } 
    if (key == 'i') //faster
    {
      printf("%c\n", key);
      glob_rSpeed += 5;
      glob_lSpeed += 5;
      if(glob_rSpeed > 230 && glob_lSpeed > 255)
      {
        printf("Max Speed");
        glob_rSpeed = 228;
        glob_lSpeed = 255;
      }
        
      if (forward_parameter->run)
      {
        forward_parameter->rSpeed = glob_rSpeed;
        forward_parameter->lSpeed = glob_lSpeed;
      }
      else
      {
        backward_parameter->rSpeed = glob_rSpeed;
        backward_parameter->lSpeed = glob_lSpeed;
      }
    }
    if (key == 'j') //slower
    {
      printf("%c\n", key);
      glob_rSpeed -= 5;
      glob_lSpeed -= 5;
      if(glob_rSpeed < 100 && glob_lSpeed < 100)
      {
        printf("MIN Speed");
        glob_rSpeed = 100;
        glob_lSpeed = 73;
      }
      
      if (forward_parameter->run)
      {
        forward_parameter->rSpeed = glob_rSpeed;
        forward_parameter->lSpeed = glob_lSpeed;
      }
      else
      {
        backward_parameter->rSpeed = glob_rSpeed;
        backward_parameter->lSpeed = glob_lSpeed;
      }
    }
    if (key == 'a') //left
    {
      printf("%c\n", key);
      turnLeft = true;
      if (!turnRight)
      {
        if(forward_parameter->run)
        {
          forward_parameter->rSpeed = 255;
          forward_parameter->lSpeed = 0;
          usleep(190000);
          forward_parameter->rSpeed = glob_rSpeed;
          forward_parameter->lSpeed = glob_lSpeed;
        }
        else
        {
          backward_parameter->rSpeed = 0;
          backward_parameter->lSpeed = 225;
          usleep(200000);
          backward_parameter->rSpeed = glob_rSpeed;
          backward_parameter->lSpeed = glob_lSpeed;
        }
      }
      turnLeft = false;
    }
    if (key == 'd') //right
    {
      printf("%c\n", key);
      turnRight = true;
      if (!turnLeft)
      {
        if(forward_parameter->run)
        {
          forward_parameter->rSpeed = 0;
          forward_parameter->lSpeed = 225;
          usleep(200000);
          forward_parameter->rSpeed = glob_rSpeed;
          forward_parameter->lSpeed = glob_lSpeed;
        }
        else
        {
          backward_parameter->rSpeed = 255;
          backward_parameter->lSpeed = 0;
          usleep(190000);
          backward_parameter->rSpeed = glob_rSpeed;
          backward_parameter->lSpeed = glob_lSpeed;
        }
      }     
      turnRight = false;
    }
    if (key == 'q') //quit
    {
      forward_parameter->run = false;
      backward_parameter->run = false;
      gpioPWM(12, 0);
      gpioPWM(13, 0);
      usleep(500000);
      done = true;
    } 
  } 
  return 0;
}



int main( void )
{
  pthread_t                       KEYthread;

  volatile struct io_peripherals *io;
  io = import_registers();
  
  forward_parameter = (thread_parameter *)malloc(sizeof(thread_parameter));
  backward_parameter = (thread_parameter *)malloc(sizeof(thread_parameter));
  glob_rSpeed = 100;
  glob_lSpeed = 73;
  done = false;
  
  
  if (io != NULL)
  {
    /* print where the I/O memory was actually mapped to */
    printf( "mem at 0x%8.8X\n", (unsigned long)io );
    
    /* pigpio allows for use of 'gpioInitialise' and 'gpioPWM' */
    gpioCfgSetInternals(1<<10); //Kept getting a segfault and this prevents the flag
    gpioInitialise();
    
    printf("Press 'q' to quit program.\n");

#if 0
    //Thread13( (void *)io );
#else
    forward_parameter->rPin = 5;
    forward_parameter->lPin = 22;
    forward_parameter->rSpeed = glob_rSpeed;
    forward_parameter->lSpeed = glob_lSpeed;
    forward_parameter->run = false;
    
    backward_parameter->rPin = 6;
    backward_parameter->lPin = 23;
    backward_parameter->rSpeed = glob_rSpeed;
    backward_parameter->lSpeed = glob_lSpeed;
    backward_parameter->run = false;
    
    pthread_create( &forward_handle, 0, looopMotors, (void *)forward_parameter);
    pthread_create( &backward_handle, 0, looopMotors, (void *)backward_parameter);
    pthread_create( &KEYthread, NULL, looopKeyboard, NULL);
    pthread_join( KEYthread, 0 );
#endif
    printf("'q' pressed... program end \n \n");

  }
  else
  {
    ; /* warning message already issued */
  }

  return 0;
}
