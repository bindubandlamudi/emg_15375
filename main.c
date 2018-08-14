/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.65.2
        Device            :  PIC16F15375
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/
/**
  Section: Included Files
*/
#include "mcc_generated_files/mcc.h"
#include "peak_filter.h"
#include "moving_avg_filter.h"
#include "signal_buffer.h"
#include "signal_processing.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
/**
  Section: Global Variable Declaration
*/
// Flag that starts data acquisition
uint8_t start_flag = 0;

// Index for looping elements
uint8_t i;
uint8_t threshold = 18;

// Flags to indicate the character sent over UART to the Slave PIC
uint8_t sent_1 =0;
uint8_t sent_0 =0;

// Custom Interrupt Handler to acquire data from ADC
// Refer to files tmr6.c and tmr6.h for Timer Interrupt working
void TMR2_EMG_InterruptHandler(void)
{
    if (start_flag == 1) {
        
        //Uncomment the line below if you need to test Analog Channel with a Potentiometer
        //ADCC_StartConversion(POT_RA0); 
        
        //ADC Channel selected to be the EMG Click
        //Make sure to insert the EMG Click on Curiosity HPC MikroBus Slot #2
        //***ADCC_StartConversion(EMG_RA2);
        
        ADC_SelectChannel(EMG_RA2);
        //ADC_SelectChannel(POT_RA0);
        ADC_StartConversion();
        
        adc_result_t adval = ADC_GetConversionResult();
        //***adc_result_t adval = ADCC_GetConversionResult();
        
        // Every new sample point is added to the "Signal Circular Buffer"
        // Data is "buffered" (until the limit of 50 data points) so the main loop can get the first inserted element (FIFO)
        // whenever it is free and process it.
       
        sbuf_insert(adval/100);
    }
}

/*
    This application takes muscle-flex data from an EMG Sensor and sends commands to a Slave microcontroller to control a motor 
    connected to a prosthetic arm prototype
 */
void main(void)
{
    // Initialize the Device
    SYSTEM_Initialize();
    // ^-^ 

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    ADC_SelectChannel(POT_RA0);
    
    ADC_StartConversion();
    
    adc_result_t pot_value = ADC_GetConversionResult();
    
    if (pot_value >= 0 | pot_value < 250)
        threshold = 10;
    else if (pot_value >= 250 | pot_value < 500)
        threshold = 18;
    else if (pot_value >= 500 | pot_value < 750)
        threshold = 25;
    else 
        threshold = 32;
    
    
    
    
    // Initialize custom Interrupt Handler which acquires data from ADC
    TMR2_SetInterruptHandler(TMR2_EMG_InterruptHandler);
    TMR2_Start();
    
    /**
    Section: Local Variable Declaration
    */
    int count = 0;                               //Counter for number of data points in signal buffer
    uint16_t datapoint;
    uint16_t neutral_datapoint, result;
    uint8_t mode = 0;                            //Mode of Action of Prosthetic prototype
    double time_elapsed;    
    uint8_t flex_flag, motor_started;            //Flags set after events
    char send_data[10];
    int num_bytes; 
    int j = 0;
    
    //Initialize flags
    flex_flag = 0;
    motor_started = 0;

    // Mode 0 -- Flex and Release for Motor ON and Motor OFF (LED_RA7 OFF)
    // Mode 1 -- Flex and Flex for Motor ON and Motor OFF (LED_RA7 ON)
    LED_RA7_SetLow(); //Initialize Mode and Mode-indicator LED to Default Mode 0
    
    //Select Mode-Switch1 and Start Process-Switch2
    while (1)
    {
        // Press Switch-1 to toggle the Mode (Reset Microcontroller after every Mode change)
        if (MODE_RB4_GetValue() == 0)  
        {
            mode ^= 1;
            LED_RA7_Toggle();
            __delay_ms(700);
        }
         
		// When the flag is cleared and Switch-2 is pressed, the process starts
        if (START_RC5_GetValue() == 0 && start_flag == 0) 
        {
            //Uncomment the line below for sending data to MikroPlot
            //***printf("START\r\n");
            
            num_bytes = sprintf(send_data, "START\r\n");
            j=0;
            while (j<num_bytes)
            {
                EUSART2_Write(send_data[j]);
                j++; 
            }
            start_flag = 1;
            __delay_ms(700);
            break;
        }
    }
    
    //Signal Conditioning 
    while (1)
    {
      //  NOP();
        if(start_flag == 1)
        {
           // NOP();
          //  NOP();
            // Count the number of data points present in signal buffer
            for (i = sb_front; i != sb_rear; i = (i + 1) % SB_DATA_WINDOW) {
                count++;
            }

            if(count>0)
            {
                // Get the first inserted data point (in FIFO order)
                datapoint = sbuf_peek();
                
                // Calculates the neutral peak by passing the data point to the Peak Filter
                neutral_datapoint = get_neutral_peaktopeak(datapoint);
                
                // Uses the neutral point to subtract from the current data point
                // Signal is rectified by taking absolute difference between the two points
                // Moving Average filter is applied to the obtained peak values 
                result = get_moving_average(abs(datapoint - neutral_datapoint));
                
                //printf("%u\r\n", result);
                
                memset(send_data, 0, 10);
                
                num_bytes = sprintf(send_data, "%d\r\n", result);
                
                j=0;
                                             
                while(j < num_bytes)
                {
                    EUSART2_Write(send_data[j]);
                    j++;
                }
                
                if(mode == 0)
                {
                    // Turn motor at muscle flex & Turn back motor at muscle release
                    if(result>= threshold && sent_1 == 0)
                    {
                        //Send Character '1' over UART to SLAVE MCU to turn motor Clockwise
                        printf("#1 P1000 #2 P1000 #3 P1000 #4 P1000 #5 P1000 T800\r");
                        //printf("#1 P2000 #2 P2000 #3 P2000 #4 P2000 #5 P2000 T800\r");
                        
                        //Set and Reset flags 
                        sent_1 = 1;
                        sent_0 = 0;
                    }
                    else if(result<threshold && sent_0 == 0)
                    {
                        //Send Character '0' over UART to SLAVE MCU to turn motor Counter-Clockwise
                        printf("#1 P2000 #2 P2000 #3 P2000 #4 P2000 #5 P2000 T800\r");
                        //printf("#1 P1000 #2 P1000 #3 P1000 #4 P1000 #5 P1000 T800\r");
                        
                        //Set and Reset flags
                        sent_0 = 1;
                        sent_1 = 0;
                    }
                }
                
                else //Mode 1
                {
                    // Turn motor at Muscle flex & Turn back motor at next muscle flex
                    if(result>= threshold && flex_flag == 0)
                    {
                        flex_flag = 1;
                        
						if(motor_started == 1)
                        {
                            motor_started =0;
                           // printf("0");
                            printf("#1 P1000 #2 P1000 #3 P1000 #4 P1000 #5 P1000 T800\r");
                        }
                        else
                        {
                            motor_started = 1;
                           // printf("1");
                           printf("#1 P2000 #2 P2000 #3 P2000 #4 P2000 #5 P2000 T800\r");
                        }
                        
                    }
                    
                    else if(result< threshold && flex_flag == 1)
                    {
                        flex_flag = 0;
                    }
                }
                                
                // Remove the first element (in FIFO order) since it has already been processed
                sbuf_remove();
                
                time_elapsed += 5.0;
            }

            count = 0;
        }
    }
}
/**
 End of File
*/
