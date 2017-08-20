FineProtocol
==============

This is the protocol used for communication between a microcontroller gathering air quality (or anything) data and another device (like PC or a mobile phone) for displaying, parsing, sending further etc. over serial - whether it's a Chinese Bluetooth module that works as a serial emulator or just a wired RS232-to-USB adapter.
Supports up to 16 different sensors and is query-based.

## The protocol itself ##
It's a simple no-nonsense 5-byte protocol, including header, command, two bytes of data and checksum. Implementation should be easy for the simplest devices and is constant regardless of connected sensors.
| Byte # | Name   | Description 
|--------|--------|----------------------------------------------------------|
| 0      |Header  | Always 0xF2 for master device (queries), 0xF1 for answers.
| 1      |Command | Described thoroughly later; copied from query to answer.  
| 2      |Data 0  | Higher byte of data.
| 3      |Data 1  | Lower byte of data.
| 4      |Checksum| Header+Command+Data0+Data1.
|-----------------------------------------------------------------------------

### Header ###
Value of ``0xF2`` ("Fine?") indicate queries. These are sent by the master device which ask for data from the microcontroller or another device getting values from its sensors.

Value of ``0xF1`` ("F1ne") is what is sent back, along with the requested data.

### Command ###
These are a little more complex, and functionalities may get added in the future. In its basic version it should suffice for getting and sending data though. Command byte is always copied from the query to the answer.

| Command  | Description
|----------|--------------
| ``0x01`` | Capabilities query. Slave sends which sensors are connected.
| ``0xX2`` | ``X`` is sensor ID (see further down). Query for data. Slave should send requested data back.
| ``0x04`` | Diagnostics. Slave sends back build date (firmware version).
| ``0x05`` | Continuous mode. Normally the slave replies only after a query, but with this it could iterate over devices and send the data every ``Data`` seconds. If ``Data`` is 0, continuous mode is disabled.
| ``0xF1`` | Diagnostics/Handshake. Slave device adds ``0xF1`` to both data bytes and send it back.
|-----------------------------


### Data and devices ###
In response to ``0x01`` the slave should indicate sensors it supports, according to this spec.
Every ``1`` in the response data means that this sensor is supported; ``0`` means otherwise. Sensor numbers are also described below.
| Byte            |           Data 0        ||||| |        |           | Data 1 ||||||||||
|-------------------------------------------------------------
| **Bit**         |7 (MSB)| 6 | 5 | 4 | 3 | 2 | 1 |0 (LSB)|7 (MSB)| 6 | 5 | 4 | 3 | 2 | 1 |0 (LSB)|
|**Sensor#**|``0``|``1``|``2``|``3``|``4``|``5``|``6``|``7``|``8``|``9``|``A``|``B``|``C``|``D``|``E``|``F``|

Devices supported list requires not only the name of the device, but also measurement unit. This does not have to concern the slave device, but rather should be saved in the master. If the sensor you're using uses a different unit, you should either convert it or create your own category.

| Sensor# | Device name | Unit of measurement
|------------------------------------------------------
| 0 | Temperature | °C
| 1 | Humidity    | %RH
| 2 | PM2.5	  | 0.1μg/m^3 (div by 10)
| 3 | PM10	  | 0.1μg/m^3 (div by 10)
| 4 | CO	  | mV (raw ADC data, requires per-unit calibration)
| 5-F | Reserved  | Can be added later, if anything comes up.
|-------------------------------------------------------

### Checksum ###
Simply add all the previous bytes together and you will get the checksum.


## Examples of communication ##
To be added later.

### Implementation ###
Should be easy enough in any language.
