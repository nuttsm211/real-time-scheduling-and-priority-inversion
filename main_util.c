/* cmsis-rtos: rms with timers + signal/wait
   problem is: schedule three periodic tasks using rate monotonic scheduling and show the intended timeline.
   fix: use virtual timers to enforce task periods and signal/wait to hand control between threads deterministically. */

#define osObjectsPublic
#include "osObjects.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <math.h>
#include "LED.h"

/* problem: simulate different compute times to reflect c in (t, c).
   fix: busy-wait delay as stand-in for task execution time. */
static void delay(long k) {
  volatile int count = 0;
  for (long i = 0; i < k; i++) { count++; }
}

/* declare thread entry points. */
void led_Thread1 (void const *argument);
void led_Thread2 (void const *argument);
void led_Thread3 (void const *argument);

/* problem: need fixed-priority tasks mapped to periods (c has different sizes).
   fix: define three threads; priorities chosen to match rms intent. */
osThreadDef(led_Thread1, osPriorityBelowNormal, 1, 0);
osThreadDef(led_Thread2, osPriorityNormal,     1, 0);
osThreadDef(led_Thread3, osPriorityAboveNormal,1, 0);

/* thread ids used for signaling. */
osThreadId T_led_ID1;
osThreadId T_led_ID2;
osThreadId T_led_ID3;

/* problem: enforce task periods 40k, 40k, 20k (lab spec) and trigger work at the right cadence.
   fix: three periodic virtual timers call one callback with an index parameter. */
static void callback(void const *param) {
  switch ((uint32_t)param) {
    case 0: /* thread1 period tick. */
      LED_On(5); LED_Off(6); LED_Off(7);
      osSignalSet(T_led_ID1, 0x01);
      delay(1000);
      LED_Off(5); LED_Off(6); LED_Off(7);
      break;
    case 1: /* thread2 period tick. */
      LED_Off(5); LED_On(6); LED_Off(7);
      osSignalSet(T_led_ID2, 0x02);
      delay(1000);
      LED_Off(5); LED_Off(6); LED_Off(7);
      break;
    case 2: /* thread3 period tick. */
      LED_Off(5); LED_Off(6); LED_On(7);
      osSignalSet(T_led_ID3, 0x03);
      delay(1000);
      LED_Off(5); LED_Off(6); LED_Off(7);
      break;
  }
}

osTimerDef(timer0_handle, callback);
osTimerDef(timer1_handle, callback);
osTimerDef(timer2_handle, callback);

/* problem: each task must run its own compute time and yield until next period.
   fix: each thread waits on its signal, does work proportional to c, then turns off. */

void led_Thread1 (void const *argument) { /* t=40k, larger c. */
  for (;;) {
    osSignalWait(0x01, osWaitForever);
    LED_Off(0); LED_Off(1); LED_Off(2);
    LED_On(0);
    for (int x = 0; x < 260; x++) { delay(300000); }
    LED_Off(0);
  }
}

void led_Thread2 (void const *argument) { /* t=40k, medium c. */
  for (;;) {
    osSignalWait(0x02, osWaitForever);
    LED_Off(0); LED_Off(1); LED_Off(2);
    LED_On(1);
    for (int x = 0; x < 130; x++) { delay(300000); }
    LED_Off(1);
  }
}

void led_Thread3 (void const *argument) { /* t=20k, small c, highest rms priority. */
  for (;;) {
    osSignalWait(0x03, osWaitForever);
    LED_Off(0); LED_Off(1); LED_Off(2);
    LED_On(2);
    for (int x = 0; x < 65; x++) { delay(300000); }
    LED_Off(2);
  }
}

/* problem: start tasks with correct periods and priorities to observe rms.
   fix: start timers at 40k, 40k, and 20k; create threads and hand control to rtos. */
int main (void) {
  osKernelInitialize();

  osTimerId timer_0 = osTimerCreate(osTimer(timer0_handle), osTimerPeriodic, (void *)0);
  osTimerId timer_1 = osTimerCreate(osTimer(timer1_handle), osTimerPeriodic, (void *)1);
  osTimerId timer_2 = osTimerCreate(osTimer(timer2_handle), osTimerPeriodic, (void *)2);

  LED_Init();

  osTimerStart(timer_0, 40000);  /* thread1 period. */
  osTimerStart(timer_1, 40000);  /* thread2 period. */
  osTimerStart(timer_2, 20000);  /* thread3 period (higher rate, higher priority). */

  T_led_ID1 = osThreadCreate(osThread(led_Thread1), NULL);
  T_led_ID2 = osThreadCreate(osThread(led_Thread2), NULL);
  T_led_ID3 = osThreadCreate(osThread(led_Thread3), NULL);

  osKernelStart();
  osDelay(osWaitForever);
  for (;;);
}
