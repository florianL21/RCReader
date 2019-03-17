# RCReader
This library is supposed to replace the arduino pulseIn() implementation for reading RC receiver signals in a non blocking way. <br>
Runtime measured with a pintoggle at the beginning of the loop and an oscilloscope.
The loop does not do anything apart from reading in the signal and toggeling the pin.

## 1. Performance measurements:

| Instances (num of pins) |         | PluseIn |         |         | RCReader |         |
|------------------------:|:-------:|:-------:|:-------:|:-------:|:--------:|:-------:|
|                         | **Min** | **Max** | **Avg** | **Min** | **Max**  | **Avg** |
|                **1 pin**|  22ms   |  22ms   |  22ms   | 64us    | 80us     | 65us    |
|               **2 pins**|  44ms   |  44ms   |  44ms   | 120us   | 150us    | 121us   |
|               **3 pins**|  65ms   |  66ms   |  66ms   | 172us   | 197us    | 173us   |
|               **4 pins**|  87ms   |  88ms   |  88ms   | 223us   | 260us    | 226us   |
|               **5 pins**|  86ms   |  108ms  |  92ms   | 276us   | 323us    | 282us   |
|               **6 pins**|  108ms  |  130ms  |  112ms  | 329us   | 370us    | 334us   |

### 1.1 Consistency:
Notice also the value consistency is much better than the pulseIn implementaion.
Both measurements were taken using a serial print and the arduino serial plotter at 115200 baud and a looptime of about 100ms

#### 1.1.1 PulseIn:
![PulseIn](/pictures/Value-output-of-pulseIn.png "Value output of pulseIn")

#### 1.1.2 RCReader:
![RCReader](/pictures/Value-output-of-RCReader.png "Value output of RCReader")

## 2. Usage:
Using the RCReader is a s simple as that:
```cpp
#include <Arduino.h>
#include <RCReader.h>

//Initialize the RCReader for Pin A8
RCReader test(RCR_PIN_A8);

void setup() 
{
  Serial.begin(115200);
}

void loop()
{
  //print the data to the serial monitor
  Serial.println(test.getMicroseconds());
  //add a bit of delay to not flood the serial output
  delay(10);
}
```
If you need error handling use the getMicroseconds function like so:
```cpp
int value;

void loop()
{
  RCRStatus returnStatus = test.getMicroseconds(&value);
  if(returnStatus == RCR_OK)
  {
      Serial.println(test.getMicroseconds());
  } else
  {
      Serial.println("There was an error");
  }
  //add a bit of delay to not flood the serial output
  delay(10);
}
```
For more examples take a look at the example sketches included with the library.

## 3. Detailed function description:
### 3.1 Datatypes:

#### 3.1.1 Pin defines:
Datatype name: `RCReaderPin`

Possible values:

| Port:  |  Pin:  |  Arduino pin:     | RCR pin name:|
|-------:|--------|------------------:|--------------|
B  |     0  |     53                 | RCR_PIN_53 |
B  |     1  |     52                 | RCR_PIN_52 |
B  |     2  |     51                 | RCR_PIN_51 |
B  |     3  |     50                 | RCR_PIN_50 |
B  |     4  |     10                 | RCR_PIN_10 |
B  |     5  |     11                 | RCR_PIN_11 |
B  |     6  |     12                 | RCR_PIN_12 |
B  |     7  |     13                 | RCR_PIN_13 |
E  |     0  |     0                  | RCR_PIN_0 |
J  |     0  |     15                 | RCR_PIN_15 |
J  |     1  |     14                 | RCR_PIN_14 |
J  |     2  |     Not Connected (NC) | Not avaliable (n/a) |
J  |     3  |     NC                 | n/a |
J  |     4  |     NC                 | n/a |
J  |     5  |     NC                 | n/a |
J  |     6  |     NC                 | n/a |
K  |     0  |     A8                 | RCR_PIN_A8  |
K  |     1  |     A9                 | RCR_PIN_A9  |
K  |     2  |     A10                | RCR_PIN_A10 |
K  |     3  |     A11                | RCR_PIN_A11 |
K  |     4  |     A12                | RCR_PIN_A12 |
K  |     5  |     A13                | RCR_PIN_A13 |
K  |     6  |     A14                | RCR_PIN_A14 |
K  |     7  |     A15                | RCR_PIN_A15 |

#### 3.1.2 Status flags:
Datatype name: `RCRStatus`

Possible values: `RCR_OK`, `RCR_InvalidValue`, `RCR_Timeout`, `RCR_InitFailed`

### 3.2 Functions:
#### 3.2.1 Constructor:
##### Description:
This function initializes all registers and internal values of the RCReader class
please use the provided pindefines, as only these pins work with this library. All of them are named like RCR_PIN_<ArduinoPinName>.
Optionally a timeout value can be set at which a Reader is considered inactive. A value of 0 disables the timeout detection.
Also optionally a min and max value that are expected to be measured can be set to validate the measurements.
##### Returns:
- Nothing
##### Parameters:
- `PinToAttach` Default: None <br>
  The pin to read in. Posssible choices can be found looking at the RCReaderPin enum.
- `timeoutInMilliseconds` Default: 0 <br>
  This defines the time it takes for a RCReader to be considered as inactive if set to anything else than 0 wich deactivates this feature.
- `validMinimumValue` Default: 0 <br>
  If minimum and maximum are both set to 0 Value checking is disabled. If value checking is enabled the RcReader will consider every value that is not in the 
  configured range as invalid and will follow the specified error behavior.
- `validMaximumValue` Default: 0 <br>
  See description of validMinimumValue parameter for detailed info.
- `holdLastValueOnFailure` Default: false <br>
  If set to true the getMicroseconds function will return the last valid value in case of an error instead of reporting that there was an error.

##### Function prototype:
```cpp
RCReader(RCReaderPin PinToAttach, uint16_t timeoutInMilliseconds = 0, uint16_t validMinimumValue = 0, uint16_t validMaximumValue = 0, bool holdLastValueOnFailure = false)
```

#### 3.2.2 getMicroseconds (With error information):
##### Description:
Passes the last calculated high time of the signal in microseconds via reference.
If a valid maximum and minimum value are configured it returns the `RCR_InvalidValue` flag in case of an error
and holds the last valid value if it was configured to do so using the setValidRange function.
If no valid range has been set it will always pass on the measured value and return the `RCR_OK` flag without doing any checks.
If a timeout error was detected (if a timeout period was set before) the function will still pass on the last measured value
but it will return the `RCR_timeout` flag.

##### Returns:
Datatype: `RCRStatus` <br>
Returns: <br>
The status flag signals the state of the RCReader object. 
- `RCR_OK` will be returned in case everything is fine.
- In case a timeout periode was set and a timeout was detected it returns `RCR_Timeout`
- If the range check was configured it will return `RCR_InvalidValue` in case the value is not inside the set range.
- If the initialization of the RCReader object failed `RCR_InitFailed` will be returned
##### Parameters:
- `Value` Default: None <br>
  This is the variable where the value should be stored in. A '&' sign must be used in order to get the address of the variable instead of its value so it can be modified by the function.
##### Function prototype:
```cpp
RCRStatus getMicroseconds(uint16_t* Value)
```

#### 3.2.3 getMicroseconds (Without error information):
##### Description:
Returns the last calculated high time of the signal in microseconds.
If a valid maximum and minimum value are configured it returns -1 in case of an error
But if it was configured to hold the last valid value using the setValidRange function there will be no error indication.
If no valid range has been set it will always return the measured value without doing any checks.
If a timeout error was detected (if a timeout period was set before) the function will return -1.

##### Returns:
Datatype: `int` <br>
Returns: <br>
- (-1) in case of an error, if any checks were configured
- The current value if no error checks were configured or no errors were detected.
- The last valid value if an error was detected but the cofiguration was set up to hold the last valid value.
##### Parameters:
- None
##### Function prototype:
```cpp
int getMicroseconds()
```

#### 3.2.4 setValidRange:
##### Description:
If not configured on initialization of the RCReader the configuration can be done afterwards by using this function.
This sets the expected values that should get read from the pin. In case the read value is not inside this range
the getMicroseconds method will return the `RCR_InvalidValue` flag  to signal that an invalid value was detected 
but the returned value is not modified. However, if the parameter holdLastValueOnFailure is set to true the function 
getMicroseconds will keep returning the last valid value in case of a failure.
##### Returns:
- Nothing
##### Parameters:
- `validMinimumValue` Default: None <br>
  If minimum and maximum are both set to 0 Value checking is disabled. 
  If value checking is enabled the RcReader will consider every value that is not in the configured range as invalid and will follow the specified error behavior.

- `validMaximumValue` Default: None <br>
  See description of validMinimumValue parameter for detailed info.

- `holdLastValueOnFailure` Default: None <br>
  If set to true the getMicroseconds function will return the last valid value in case of an error instead of reporting that there was an error.

##### Function prototype:
```cpp
void setValidRange(uint16_t validMinimumValue, uint16_t validMaximumValue, bool holdLastValueOnFailure = false)
```

#### 3.2.5 setTimeout:
##### Description:
If not configured on initialization of the RCReader the configuration can be done afterwards by using this function.
This function sets a timeout value  at which a reader is considered inactive.
To disable timeout detections set the parameter to 0.
##### Returns:
- Nothing
##### Parameters:
- `timeoutInMilliseconds` Default: None <br>
  This defines the time it takes for a RCReader to be considered as inactive if set to anything else than 0 wich deactivates this feature.

##### Function prototype:
```cpp
void setValidRange(uint16_t validMinimumValue, uint16_t validMaximumValue, bool holdLastValueOnFailure = false)
```

## 4. Limitations:
This library has a couple limitations compared to the pulseIn function:
* It is only possible to use it with the supported pins
* For now it can only be used on an Arduino Mega 2560 I do not own any other arduino models, so I cannot test it on anying else.
* Heavy usage of interrupts maybe impact the accuracy of the measured values.
