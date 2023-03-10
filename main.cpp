#include <intrinsics.h>
#include <iostm8s003k3.h>

#define RED PB_ODR_ODR0
#define BLUE PB_ODR_ODR1
#define GREEN PB_ODR_ODR2
#define RESET PB_ODR_ODR5
#define HZ PB_ODR_ODR6
#define LANG PB_ODR_ODR7
#define PAL 0
#define NTSC 1
#define JAP 0
#define ENG 1

unsigned int status = 1;

void Delayus(void)   
{    
    asm("nop"); 
    asm("nop");   
    asm("nop");   
    asm("nop");    
} 

void Delayms(unsigned int time)   
{   
    unsigned int i;   
    while(time--)     
    for(i=900;i>0;i--)   
    Delayus();    
} 

void SetSwitches(unsigned int stat)
{
  switch (stat)
  {
    case 1:
      RED = 0;
      GREEN = 1;
      BLUE = 1;
      HZ = 0;
      LANG = 1;
      break;
    case 2:
      RED = 1;
      GREEN = 0;
      BLUE = 1;
      HZ = 1;
      LANG = 1;
      break;
    case 0:
      RED = 1;
      GREEN = 0;
      BLUE = 0;
      HZ = 1;
      LANG = 0;
      break;
  }
}

void ResetMD(){
  RED = 0;
  GREEN = 0;
  BLUE = 0;
  
  RESET = 1;
  Delayms(500);
  RESET = 0;
  
  RED = 1;
  GREEN = 1;
  BLUE = 1;
  Delayms(500);
  SetSwitches(status);
}

unsigned int btnOn = 0;

//
//  Timer 2 Overflow handler.
//
#pragma vector = TIM2_OVR_UIF_vector
__interrupt void TIM2_UPD_OVF_IRQHandler(void)
{ 
  if (PB_IDR_IDR3 == 1 && btnOn > 1){
    SetSwitches(status);
    btnOn = 0;
  } 
  
  if (PB_IDR_IDR3 == 1 && btnOn == 1){
    ResetMD();
    btnOn = 0;
  } 
  
  if (PB_IDR_IDR3 == 0){
    btnOn++;
  }

  if (PB_IDR_IDR3 == 0 && btnOn > 2){
      switch (status)
      {
        case 0:
          RED = 0;
          GREEN = 1;
          BLUE = 1;
          
          status = 1;
          break;
          
         case 1:
          RED = 1;
          GREEN = 0;
          BLUE = 1;
          
          status = 2;
          break;
          
         case 2:
          RED = 1;
          GREEN = 0;
          BLUE = 0;
          
          status = 0;
          break;
      }
    }
    
  TIM2_SR1_UIF = 0;               //  Reset the interrupt otherwise it will fire again straight away.
}

//
//  Process the interrupt generated by the pressing of the button on PD4.
//

#pragma vector = 6
__interrupt void EXTI_PORTB_IRQHandler(void)
{
  btnOn = 1;
}


void InitialiseSystemClock()
{
  CLK_ICKR = 0;                       //  Reset the Internal Clock Register.
  CLK_ICKR_HSIEN = 1;                 //  Enable the HSI.
  CLK_ECKR = 0;                       //  Disable the external clock.
  while (CLK_ICKR_HSIRDY == 0);       //  Wait for the HSI to be ready for use.
  CLK_CKDIVR = 0;                     //  Ensure the clocks are running at full speed.
  CLK_PCKENR1 = 0xff;                 //  Enable all peripheral clocks.
  CLK_PCKENR2 = 0xff;                 //  Ditto.
  CLK_CCOR = 0;                       //  Turn off CCO.
  CLK_HSITRIMR = 0;                   //  Turn off any HSIU trimming.
  CLK_SWIMCCR = 0;                    //  Set SWIM to run at clock / 2.
  CLK_SWR = 0xe1;                     //  Use HSI as the clock source.
  CLK_SWCR = 0;                       //  Reset the clock switch control register.
  CLK_SWCR_SWEN = 1;                  //  Enable switching.
  while (CLK_SWCR_SWBSY != 0);        //  Pause while the clock switch is busy.
}

//
//      Reset Timer 2 to a known state
//
void InitialiseTimer2()
{
    TIM2_CR1 = 0;               // Turn everything TIM2 related off.
    TIM2_IER = 0;
    TIM2_SR2 = 0;
    TIM2_CCER1 = 0;
    TIM2_CCER2 = 0;
    TIM2_CCER1 = 0;
    TIM2_CCER2 = 0;
    TIM2_CCMR1 = 0;
    TIM2_CCMR2 = 0;
    TIM2_CCMR3 = 0;
    TIM2_CNTRH = 0;
    TIM2_CNTRL = 0;
    TIM2_PSCR = 0;
    TIM2_ARRH  = 0;
    TIM2_ARRL  = 0;
    TIM2_CCR1H = 0;
    TIM2_CCR1L = 0;
    TIM2_CCR2H = 0;
    TIM2_CCR2L = 0;
    TIM2_CCR3H = 0;
    TIM2_CCR3L = 0;
    TIM2_SR1 = 0;
}

//
//  Setup Timer 2 to generate a 20 Hz interrupt based upon a 16 MHz timer.
//
void SetupTimer2()
{
    TIM2_PSCR = 0x08;       //  Prescaler = 256.
    TIM2_ARRH = 0x7a;       //  High byte of 50,000.
    TIM2_ARRL = 0x12;       //  Low byte of 50,000.
    TIM2_IER_UIE = 1;       //  Enable the update interrupts.
    TIM2_CR1_CEN = 1;       //  Finally enable the timer.
}

//
//  Main program loop.
//
void main()
{
    //
    //  Initialise the system.
    //
    __disable_interrupt();
    InitialiseSystemClock();
    
    /*
    *     Port B setup
    */
    PB_ODR = 0;         // all pins turned off 
    
    PB_DDR = 0xff;          //  All pins are outputs.
    PB_CR1 = 0xff;          //  Push-Pull outputs.
    PB_CR2 = 0xff;          //  Output speeds up to 10 MHz.
    
    PB_DDR_DDR3 = 0;        //  PB0 is input.
    PB_CR1_C13 = 0;         //  PB0 is floating input.
    //
    //  Set up the interrupt.
    //
    EXTI_CR1_PBIS = 2;      //  Interrupt on falling edge.
    EXTI_CR2_TLIS = 0;      //  Falling edge only.
    
    InitialiseTimer2();
    SetupTimer2();
    
    __enable_interrupt();

    SetSwitches(0);
    
    while (1)
    {
        __wait_for_interrupt();
    }
}
