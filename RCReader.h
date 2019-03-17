#ifndef RCREADER_H_
#define RCREADER_H_

#ifndef ARDUINO_AVR_MEGA2560
    #error This is only intended to be used on a arduino Mega 2560 and will not work on any other uC!
#endif

#include <avr/interrupt.h>
#include <Arduino.h>

//maximum number of interrupts in this case is 18 beacuse the ATMega2560 has 23 Pin Change Interrupts, but 5 of them are not connected to the arduino board
#define TOTAL_NUM_OF_PC_INTERRUPTS 18

//Disable the interrupts during the for loop in the ISR to prevent timing errors if another interrupt disrupts calculation.
//Comment this out to leave the interrupts enabled. (Can help with problems related to bus communication which relies heavily on interrupts)
#define DISABLE_INTERRUPTS_DURING_CALCULAION


/*Pin Change Interrupt(PCI) pin mappings:
Available pins for PCI on the ATMega2560:
PORTB: Pins 0-7
PORTJ: Pins 0-6
PORTK: Pins 0-7

PORT:   PIN:    ARDUINO-PIN:            PCI:
----------------------------------------------
B       0       53                      PCINT0
B       1       52                      PCINT1
B       2       51                      PCINT2
B       3       50                      PCINT3
B       4       10                      PCINT4
B       5       11                      PCINT5
B       6       12                      PCINT6
B       7       13                      PCINT7
E       0       0                       PCINT8
J       0       15                      PCINT9
J       1       14                      PCINT10
J       2       Not Connected (NC)      PCINT11
J       3       NC                      PCINT12
J       4       NC                      PCINT13
J       5       NC                      PCINT14
J       6       NC                      PCINT15
K       0       A8                      PCINT16
K       1       A9                      PCINT17
K       2       A10                     PCINT18
K       3       A11                     PCINT19
K       4       A12                     PCINT20
K       5       A13                     PCINT21
K       6       A14                     PCINT22
K       7       A15                     PCINT23
*/
enum RCReaderPin {RCR_PIN_53 = 53, RCR_PIN_52 = 52, RCR_PIN_51 = 51, RCR_PIN_50 = 50, RCR_PIN_10 = 10, RCR_PIN_11 = 11, RCR_PIN_12 = 12, RCR_PIN_13 = 13, //PORT B
                  RCR_PIN_0 = 0,    //PORT E
                  RCR_PIN_14 = 14, RCR_PIN_15 = 15, //PORT J
                  RCR_PIN_A8 = A8, RCR_PIN_A9 = A9, RCR_PIN_A10 = A10, RCR_PIN_A11 = A11, RCR_PIN_A12 = A12, RCR_PIN_A13 = A13, RCR_PIN_A14 = A14, RCR_PIN_A15 = A15};   //PORT K

enum RCRStatus {RCR_OK, RCR_InvalidValue, RCR_Timeout, RCR_InitFailed};

class RCReader
{
public:
     /*
    * This function initializes all registers and internal values of the RCReader class
    * please use the provided pindefines, as only these pins work with this library. All of them are named like RCR_PIN_<ArduinoPinName>.
    * Optionally a timeout value can be set at which a Reader is considered inactive. A value of 0 disables the timeout detection.
    * Also optionally a min and max value that are expected to be measured can be set to validate the measurements.
    * 
    * Parameters:
    *   - PinToAttach:              The pin to read in. Posssible choices can be found looking at the RCReaderPin enum.
    * 
    *   - timeoutInMilliseconds:    Default: 0 
    *                               This defines the time it takes for a RCReader to be considered as inactive if set to anything else 
    *                               than 0 wich deactivates this feature.
    * 
    *   - validMinimumValue:        Default: 0
    *                               If minimum and maximum are both set to 0 Value checking is disabled.
    *                               If value checking is enabled the RcReader will consider every value that is not in the 
    *                               configured range as invalid and will follow the specified error behavior.
    * 
    *   - validMaximumValue:        Default: 0
    *                               See description of validMinimumValue parameter for detailed info.
    * 
    *   - holdLastValueOnFailure:   Default: false
    *                               If set to true the getMicroseconds function will return the last valid value in case of an error 
    *                               instead of reporting that there was an error.
    *                               
    */
    RCReader(RCReaderPin PinToAttach, uint16_t timeoutInMilliseconds = 0, uint16_t validMinimumValue = 0, uint16_t validMaximumValue = 0, bool holdLastValueOnFailure = false);

    /*
    * Disables this RCReader instance from the processing
    */
    ~RCReader();

    /*
    * Passes the last calculated high time of the signal in microseconds via reference.
    * If a valid maximum and minimum value are configured it returns the "RCR_InvalidValue" flag in case of an error
    * and holds the last valid value if it was configured to do so using the setValidRange function.
    * If no valid range has been set it will always pass on the measured value and return the "RCR_OK" flag without doing any checks.
    * If a timeout error was detected (if a timeout period was set before) the function will still pass on the last measured value
    * but it will return the "RCR_timeout" flag.
    * 
    * Parameters:
    *   - Value: This is the variable where the value should be stored in. 
    *            A & sign must be used in order to get the address of the variable instead of its value so it can be modified by the function.
    * 
    * Returns:
    *   - The status flag signals the state of the RCReader object. RCR_OK will be returned in case everything is fine.
    *     In case a timeout periode was set and a timeout was detected it returns RCR_Timeout
    *     If the range check was configured it will return RCR_InvalidValue in case the value is not inside the set range.
    *     If the initialization of the RCReader object failed RCR_InitFailed will be returned
    */
    RCRStatus getMicroseconds(uint16_t* Value);

    /*
    * Returns the last calculated high time of the signal in microseconds.
    * If a valid maximum and minimum value are configured it returns -1 in case of an error
    * But if it was configured to hold the last valid value using the setValidRange function there will be no error indication.
    * If no valid range has been set it will always return the measured value without doing any checks.
    * If a timeout error was detected (if a timeout period was set before) the function will return -1.
    * 
    * Returns:
    *   - (-1) in case of an error, if any checks were configured
    *   - The current value if no error checks were configured or no errors were detected.
    *   - The last valid value if an error was detected but the cofiguration was set up to hold the last valid value.
    */
    int getMicroseconds();

    /*
    * If not configured on initialization of the RCReader the configuration can be done afterwards by using this function.
    * This sets the expected values that should get read from the pin. In case the read value is not inside this range
    * the getMicroseconds method will return the "RCR_invalidValue" flag  to signal that an invalid value was detected 
    * but the returned value is not modified. However, if the parameter holdLastValueOnFailure is set to true the function 
    * getMicroseconds will keep returning the last valid value in case of a failure.
    * 
    * Parameters:
    *   - validMinimumValue:      If minimum and maximum are both set to 0 Value checking is disabled.
    *                             If value checking is enabled the RcReader will consider every value that is not in the 
    *                             configured range as invalid and will follow the specified error behavior.
    * 
    *   - validMaximumValue:      See description of validMinimumValue parameter for detailed info.
    * 
    *   - holdLastValueOnFailure: If set to true the getMicroseconds function will return the last valid value in case of an error 
    *                             instead of reporting that there was an error.
    */
    void setValidRange(uint16_t validMinimumValue, uint16_t validMaximumValue, bool holdLastValueOnFailure = false);

    /*
    * If not configured on initialization of the RCReader the configuration can be done afterwards by using this function.
    * This function sets a timeout value  at which a reader is considered inactive.
    * To disable timeout detections set the parameter to 0.
    * 
    * Parameters:
    *   - timeoutInMilliseconds: This defines the time it takes for a RCReader to be considered as inactive if set to anything else 
    *                            than 0 wich deactivates this feature.
    */
    void setTimeout(uint16_t timeoutInMilliseconds);

private:
    //Internal configuration variables:
    uint16_t _validMinimum;
    uint16_t _validMaximum;
    uint16_t _lastValidValue;
    uint16_t _timeout;
    bool _holdLastValidValue;
    //internal record to know at which array location the object is located at
    uint8_t _RCReaderIndexNum;

    //used to translate a pin number to its PCINT number
    uint8_t _PinToInterruptMap(RCReaderPin pin);
};

#endif