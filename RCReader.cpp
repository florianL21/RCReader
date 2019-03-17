#include "RCReader.h"

//Enum to know to which interrupt a RCReader belongs to not make unnecessary calculations
enum _ISR_Mappings {PCINT0_ISR, PCINT1_ISR, PCINT2_ISR};

//Struct that hold all information for one RCReader instance.
//This cannot be stored in the object itself because the logic of the ISR needs access to these variables.
struct _RCReaderObject
{
    uint8_t attatchedPin;
    bool lastState;
    uint32_t lastMicros;
    uint16_t currentValue;
    _ISR_Mappings assignedISR;
};

// Initialize the array to all NULL pointers which signal the end of the array
// New objects get allocated in memmory at runtime and their address is then added to the end of the array
_RCReaderObject* _AllRCReaders[TOTAL_NUM_OF_PC_INTERRUPTS] = {NULL};

RCReader::RCReader(RCReaderPin PinToAttach, uint16_t timeoutInMilliseconds, uint16_t validMinimumValue, uint16_t validMaximumValue, bool holdLastValueOnFailure)
{
    //checking first if there is still space in the array:
    bool hasSpace = false;
    //Find the last instance of RCReader in the array and append to the end of it:
    for(uint8_t i = 0; i < TOTAL_NUM_OF_PC_INTERRUPTS; i++)
    {
        if(_AllRCReaders[i] == NULL)
        {
            //store the index number for future reference
            _RCReaderIndexNum = i;
            hasSpace = true;
            break;
        }
    }
    if(hasSpace == false)
    {
        // set the index num to 1 higher than maximum to signal that tis RCReader instance is invalid
        _RCReaderIndexNum = TOTAL_NUM_OF_PC_INTERRUPTS + 1; 
        return; //array is already full return without doing anything
    }

    //Cunfiguring pin:
    pinMode(PinToAttach, INPUT);     //Configure pin as input
    digitalWrite(PinToAttach, INPUT_PULLUP); //Enable internal pull up

    /* Configure Pin Change Interrupt and Mask for the current pin
    Pin to register map:
    Register:   From:       To:
    ---------------------------
    PCMSK0      PCINT0      7
    PCMSK1      PCINT8      15
    PCMSK2      PCINT16     23
    */
    
    _ISR_Mappings assignedISR;
    uint8_t interruptNum = _PinToInterruptMap(PinToAttach);
    if(interruptNum >= 0 && interruptNum <= 7)
    {
        PCMSK0 |= (1 << interruptNum);
	    PCICR |= (1 << PCIE0);
        assignedISR = PCINT0_ISR;
    } else if(interruptNum >= 8 && interruptNum <= 15)
    {
        PCMSK1 |= (1 << (interruptNum % 8));
	    PCICR |= (1 << PCIE1);
        assignedISR = PCINT1_ISR;
    }else if(interruptNum >= 16 && interruptNum <= 23)
    {
        PCMSK2 |= (1 << (interruptNum % 8));
	    PCICR |= (1 << PCIE2);
        assignedISR = PCINT2_ISR;
    }

    //allocate memory for the new struct entry and initialize its internal values to a default state:
    _AllRCReaders[_RCReaderIndexNum] = new _RCReaderObject{PinToAttach, LOW, micros(), 0, assignedISR};

    //value validation is disabled by default.
    //disabled state is when min and max are at 0
    _validMinimum = validMinimumValue;
    _validMaximum = validMaximumValue;
    _holdLastValidValue = holdLastValueOnFailure;

    //timeout of 0 means disabled which is the default state
    _timeout = timeoutInMilliseconds;

    interrupts();//enable interrupts
}

RCReader::~RCReader()
{
    delete _AllRCReaders[_RCReaderIndexNum]; //free the allocated memory of the current object to not create a memory leak.
    _AllRCReaders[_RCReaderIndexNum] = NULL; //set the current address to a NULL pointer to signal an empty space
    //loop throug all elements wich come after the current element and shift them 1 location down the array to keep all objects in one row.
    //this approach is more efficient in the ISR because processing of the array can be stopped once an NULL pointer is encountered.
    for(uint8_t i = _RCReaderIndexNum + 1; i < TOTAL_NUM_OF_PC_INTERRUPTS; i++)
    {
        if(_AllRCReaders[i] == NULL) //If true this is our last element
        {
            _AllRCReaders[i - 1] = NULL;
            break; //break the loop because this was the last element
        }
        _AllRCReaders[i - 1] = _AllRCReaders[i]; // if not hte last element shift them 1 location down
    }
}

void RCReader::setTimeout(uint16_t timeoutInMilliseconds)
{
    _timeout = timeoutInMilliseconds;
}

void RCReader::setValidRange(uint16_t validMinimumValue, uint16_t validMaximumValue, bool holdLastValueOnFailure)
{
    _validMinimum = validMinimumValue;
    _validMaximum = validMaximumValue;
    _holdLastValidValue = holdLastValueOnFailure;
}

int RCReader::getMicroseconds()
{
    //siplified version of the getMicroseconds function if no error processing should be done
    uint16_t value;
    if(getMicroseconds(&value) == RCR_OK || _holdLastValidValue)
    {
        return value;
    } else 
    {
        return -1;
    }
}

RCRStatus RCReader::getMicroseconds(uint16_t* Value)
{
    //Check if init was successful
    if(_RCReaderIndexNum == TOTAL_NUM_OF_PC_INTERRUPTS + 1)
    {
        //return an error if not
        return RCR_InitFailed;
    }
    uint32_t passedTime;
    if(_AllRCReaders[_RCReaderIndexNum]->lastMicros > micros()) //check if an overflow occured
    {
        passedTime = (UINT32_MAX - _AllRCReaders[_RCReaderIndexNum]->lastMicros) + micros(); //correct the overflow
    } else
    {
        passedTime = micros() - _AllRCReaders[_RCReaderIndexNum]->lastMicros; //no overflow, so calculation is done without any corrections
    }
    if(passedTime / 1000 > _timeout && _timeout != 0) //If enabled (not 0) check if the RCReader is still active.
    {
        //the RCReader was inactive for too long, so we have a timeout error. Pass the return value and then return the timeout flag
        *Value = _AllRCReaders[_RCReaderIndexNum]->currentValue;
        return RCR_Timeout;
    }else if((_AllRCReaders[_RCReaderIndexNum]->currentValue >= _validMinimum && _AllRCReaders[_RCReaderIndexNum]->currentValue <= _validMaximum) || 
        (_validMinimum == 0 && _validMaximum == 0)) //check for invalid bounds if activated
    {
        // update the latest valid value and pass the current value to the specified location. Then return the OK flag
        *Value = _AllRCReaders[_RCReaderIndexNum]->currentValue;
        _lastValidValue = _AllRCReaders[_RCReaderIndexNum]->currentValue;
        return RCR_OK;
    } else
    {
        if(_holdLastValidValue) //decide if the current value or the last valid value should be returned depending on the configuration
                                //because there was a out of bounds error
        {
            *Value = _lastValidValue;
        } else
        {
            *Value = _AllRCReaders[_RCReaderIndexNum]->currentValue;
        }
        return RCR_InvalidValue;
    }
}

void _calculateRCReaderCurrentValue(_ISR_Mappings currentISR, uint8_t pinStates)
{
    //loop throug all instances of the RCReader
    for(uint8_t i = 0; i < TOTAL_NUM_OF_PC_INTERRUPTS; i++)
    {
        //stop the loop if the current element is a NULL pointer -> last element
        if(_AllRCReaders[i] == NULL)
        {
            return;
        }
        if(_AllRCReaders[i]->assignedISR == currentISR) //ISR matches, so calculate new values
        {
            _RCReaderObject* currentReader = _AllRCReaders[i];

            bool currentPinState = LOW;
            if(currentReader->attatchedPin == 0)// handle the special case of pin0
            {
                if((pinStates & 0x80) != 0)
                {
                    currentPinState = HIGH;
                }
            }else if((pinStates & digitalPinToBitMask(currentReader->attatchedPin)) != 0) // checking for not 0 because that saves a shifting operation
            {
                currentPinState = HIGH;
            }
            if(currentPinState == HIGH && currentReader->lastState == LOW) //Start measurement when the pin changes from LOW to HIGH
            {
                currentReader->lastMicros = micros();
            } else if(currentPinState == LOW && currentReader->lastState == HIGH) //Stop measurement and calculate result when pin changes from HIGH to LOW
            {
                if(currentReader->lastMicros < micros())// no overflow
                {
                    currentReader->currentValue = micros() - currentReader->lastMicros;
                }
                else  // the variable overflowed so calculate the corrected value
                {
                    currentReader->currentValue = (UINT32_MAX - currentReader->lastMicros) + micros();
                }
            }
            currentReader->lastState = currentPinState; // assinging the last state at every pin change in case the state machine gets messed up.
        }
    }
}

ISR(PCINT2_vect)
{
    #ifdef DISABLE_INTERRUPTS_DURING_CALCULAION
        noInterrupts();
    #endif
    //reading in the state of the whole PORT at once to make sure the values are not changing while the calculation is happening:
    _calculateRCReaderCurrentValue(PCINT2_ISR, PINK);
    #ifdef DISABLE_INTERRUPTS_DURING_CALCULAION
        interrupts();
    #endif
}

ISR(PCINT1_vect)
{
    #ifdef DISABLE_INTERRUPTS_DURING_CALCULAION
        noInterrupts();
    #endif
    //unfortiunatly this is a special case bacause the pin 0 (PE0) is not on the same port as all other pins (PJ0-6) of this interrupt
    uint8_t pinStates = PINJ & (0x7F); 
    pinStates |= (digitalRead(0) << 7);
    _calculateRCReaderCurrentValue(PCINT1_ISR, pinStates);
    #ifdef DISABLE_INTERRUPTS_DURING_CALCULAION
        interrupts();
    #endif
}

ISR(PCINT0_vect)
{
    #ifdef DISABLE_INTERRUPTS_DURING_CALCULAION
        noInterrupts();
    #endif
    _calculateRCReaderCurrentValue(PCINT0_ISR, PINB);
    #ifdef DISABLE_INTERRUPTS_DURING_CALCULAION
        interrupts();
    #endif
}

uint8_t RCReader::_PinToInterruptMap(RCReaderPin pin)
{
    //translating the pin numbers to their PCINT number
    //Doing it with a switch case saves memory because there is a lot of space in between which would be wasted memory
    switch(pin)
    {
        case RCR_PIN_53:
            return 0;
        case RCR_PIN_52:
            return 1;
        case RCR_PIN_51:
            return 2;
        case RCR_PIN_50:
            return 3;
        case RCR_PIN_10:
            return 4;
        case RCR_PIN_11:
            return 5;
        case RCR_PIN_12:
            return 6;
        case RCR_PIN_13:
            return 7;
        case RCR_PIN_0:
            return 8;
        case RCR_PIN_15:
            return 9;
        case RCR_PIN_14:
            return 10;
        case RCR_PIN_A8:
            return 16;
        case RCR_PIN_A9:
            return 17;
        case RCR_PIN_A10:
            return 18;
        case RCR_PIN_A11:
            return 19;
        case RCR_PIN_A12:
            return 20;
        case RCR_PIN_A13:
            return 21;
        case RCR_PIN_A14:
            return 22;
        case RCR_PIN_A15:
            return 23;
        default:
            return 255;
    }
}