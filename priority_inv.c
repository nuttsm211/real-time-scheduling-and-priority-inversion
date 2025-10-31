/* cmsis-rtos priority inversion demo with optional priority elevation
   problem: a low-priority worker (p3) is needed by a high-priority task (p1), but a medium task (p2) keeps pre-empting.
   fix: temporarily raise p3 to high priority while it is servicing p1, then restore it. */

#define osObjectsPublic
#include "osObjects.h"
#include "cmsis_os.h"
#include <math.h>
#include <stdio.h>
#include "Board_LED.h"
#include "RTE_Components.h"

/* forward declarations */
void P1 (void const *argument);
void P2 (void const *argument);
void P3 (void const *argument);

/* thread metadata */
osThreadDef(P1, osPriorityHigh,        1, 0);
osThreadDef(P2, osPriorityNormal,      1, 0);
osThreadDef(P3, osPriorityBelowNormal, 1, 0);

osThreadId t_main, t_P1, t_P2, t_P3;

/* tiny busy wait to simulate compute time */
static void spin(void) {
  volatile long k = 0;
  for (long i = 0; i < 100000; i++) { k++; }
}

/* high-priority task: does work, then asks p3 to complete a critical step */
void P1 (void const *argument) {
  for (;;) {
    LED_On(0);
    spin(); /* simulate useful work that precedes the request to p3 */

    /* fix: uncomment next line to elevate p3 while it is blocking p1 */
    // osThreadSetPriority(t_P3, osPriorityHigh);

    osSignalSet(t_P3, 0x01);             /* request service from p3 */
    osSignalWait(0x02, osWaitForever);   /* inversion shows here if p2 keeps running */

    /* fix: uncomment next line to restore p3 after it finishes */
    // osThreadSetPriority(t_P3, osPriorityBelowNormal);

    LED_On(6);
    LED_Off(6);
  }
}

/* medium-priority task: keeps the cpu busy and exposes the inversion */
void P2 (void const *argument) {
  for (;;) {
    LED_On(1);
    LED_Off(1);
    /* no blocking; this pre-empts p3 unless elevation is enabled */
  }
}

/* low-priority worker: performs the critical function for p1 */
void P3 (void const *argument) {
  for (;;) {
    spin();                               /* simulate background work */
    osSignalWait(0x01, osWaitForever);    /* wait until p1 asks for service */
    LED_Off(0);                           /* stand-in for the critical section */
    osSignalSet(t_P1, 0x02);              /* tell p1 the work is done */
  }
}

int main(void) {
  osKernelInitialize();
  LED_Initialize();

  /* make main transient and high priority so it can stage thread creation */
  t_main = osThreadGetId();
  osThreadSetPriority(t_main, osPriorityHigh);

  /* create threads in an order that makes the inversion easy to observe */
  t_P3 = osThreadCreate(osThread(P3), NULL);
  spin();  /* simple staging delay without relying on the scheduler */
  t_P2 = osThreadCreate(osThread(P2), NULL);
  spin();
  t_P1 = osThreadCreate(osThread(P1), NULL);

  osThreadTerminate(t_main);
  osKernelStart();

  for (;;) {}
}
