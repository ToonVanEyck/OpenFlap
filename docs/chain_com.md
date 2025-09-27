# Communication

The communication system between the controller and the modules is a daisy chain system based on UART. The controller sends a message to the first module, the first module processes the message and sends it to the next module. This continues until the last module in the chain has received the message.

## Header

There are 4 action types for message send by the controller to the modules:

| Value | Action                | Description                                                                        |
|-------|-----------------------|------------------------------------------------------------------------------------|
| 0     | read all              | Read a property from all modules.                                                  |
| 1     | write sequential      | Write a different value to different properties of a number of sequential modules. |
| 2     | write all             | Write the same value to the property of all modules.                               |
| 3     | execute / return code | Signal the modules to execute a command and return a return code.                  |

The action type of the message is defined by the 2 most significant bits in the message header byte. The six least significant bits are used to define the property to read or write. This means that there are 64 possible properties that can be read or written. Each property has a read and a write length, this length can be either static or dynamic. In the case of a dynamic length, the length is passed in the message itself.

These are the currently supported properties:

| Value | Property          | Read Size | Write Size | Description                                                                  |
|-------|-------------------|-----------|------------|------------------------------------------------------------------------------|
| 0     | NONE              | 0         | 0          | Nothing                                                                      |
| 1     | Firmware Version  | Dynamic   | 0          | Contains the firmware version                                                |
| 2     | Firmware Update   | 0         | 130        | Used for performing a firmware update.                                       |
| 3     | Command           | 0         | 1          | Write a special command, e.g.: reboot, ...                                   |
| 4     | Module Info       | 1         | 0          | Property containing the module type and column end information.              |
| 5     | Character Set     | Dynamic   | Dynamic    | The possible characters supported by this module.                            |
| 6     | Character Index   | 1         | 1          | The index of the character displayed in the character set.                   |
| 7     | Encoder Offset    | 1         | 1          | The offset between the encoder 'zero' and the first character in the set.    |
| 8     | Color             | 2         | 2          | Flap fore- and background colors.                                            |
| 9     | Motion            | 4         | 4          | Rotational speed control.                                                    |
| 10    | Minimum rotation  | 1         | 1          | The minimum number of flaps that should be rotated when changing characters. |
| 11    | Sensor threshold  | 4         | 4          | Threshold values for the IR encoders.                                        |

## Checksum 

The tree primary message types (read all, write all, write sequential) include some form of single byte checksum. This checksum allows the controller and modules to verify that the message was received correctly. The checksum is calculated by summing all bytes in the message (excluding the checksum byte itself) and taking the least significant byte of the result. The location and number of checksum bytes depends on the message type.

## Execute / Return Code

This is a special action type that is used to signal the modules to execute a command and return a return code. When this action is used, the 6 property bytes are used to communicate the return code. Each module will logically `OR` its return code with the return code in the header. This way the controller can determine if any module in the chain encountered an error.

## Read All 

This action is used to read a property from all modules in the chain.

The controller initiates a read all action by sending a header with the action set to `read all` followed by two counting bytes. Each module will retransmit the header, the counting bytes will be retransmitted as well, but each module will increment the counting bytes by one. Next, the module will transmit the property data, followed by a checksum. Sequential modules will know how much data from previous modules to retransmit based of the counting bytes. The last module will retransmit to the controller, which can use the counting bytes to figure out how many modules are in the chain.

Here is an example of a read all message with a property size of 2 bytes:
![Property Read All Static](images/read_all_static.png)

When a property has a dynamic read size. The module will first transmit the size of the property data (2 bytes), followed by the property data itself. Sequential modules will first read the size of the property data so they know how much data they need to retransmit.

![Property Read All Dynamic](images/read_all_dynamic.png)

Each module append's its own checksum to the end of its data. This checksum includes the header and sum of the two counting bytes of its own index in the chain.

## Write All

This action is used to write the same property value to all modules in the chain. This is useful for applying bulk changes to all modules. E.g: Homing the display or applying a firmware update.

The controller initiates a write all action by sending a header with the action set to `write all` followed by the property data and a checksum byte. The message is terminated with an `execute / return code` header. Each module will retransmit the `write all` header and the property data. Once a module has retransmitted the property data, and determined the checksum to be valid, it will process the data and wait for the `execute / return code` header. Once it has received the header, it will logically `OR` its return code with the header. If the checksum is invalid, the module will not process the data and will return an error code instead.

![Property Write All Static](images/write_all_static.png)

When a property has a dynamic write size. The module will first transmit the size of the property data (2 bytes) before transmitting the property data itself.

![Property Write All Dynamic](images/write_all_dynamic.png)

## Write Sequential

This action is used to write different property values to different modules in the chain.

The controller will send multiple header and data + checksum packets. Typically one for each module. The message is terminated by two sequential `execute / return code` headers. The first `execute / return code` header gets retransmitted by each module imeadiatly and triggers the processing of the received data. The second `execute / return code` header will only be retransmitted after the module has finished processing the data. The combination of these two `execute / return code` headers allow the modules to know when the controller has finished sending data, and it allows the controller to know when the modules have finished processing the data.

The `NONE` property can be used to skip a module in the chain. This can be useful when you want to write a property to a specific module in the chain.

Dynamic and static property sizes can be mixed in together in a single message. But typically the controller will stick to one property at a time.

![Property Write Sequential](images/write_seq.png)

## Examples (Outdated, does not contain CS)

The character property is a good example to show the 3 different action types. The character property has a static read and write size of 1 byte. For this example let's assume the character property id is `5` or `0b000101`.

Our headers would look like this:

| Action    | Action (bin) | Property (bin) | Header (bin) | Header (hex) |
|-----------|--------------|----------------|--------------|--------------| 
| read_all  | 0b01000000   | 0b000101       | 0b01000101   | **0x45**     |
| write_seq | 0b10000000   | 0b000101       | 0b10000101   | **0x85**     |
| write_all | 0b11000000   | 0b000101       | 0b11000101   | **0xC5**     |

The character property will return a number between 0 and 47. This number is the index of the character in the character set. For this example let's assume the character set starts like this `_ABCDEFGHIJKLMNOPQRSTUVWXYZ`. So when the character property is set to `0` the character displayed will be `_`, when the character property is set to `1` the character displayed will be `A`, when the character property is set to `2` the character displayed will be `B`, etc.

**We will start by reading the character property from all modules in the chain:**

![Example Read All](images/example_read_all.png)

We can see the controller transmit the read all header `0x45` followed by the counting bytes `0x00 0x00`.
We receive the header `0x45` followed by the counting bytes `0x05 0x00`, followed by 5 bytes of data. The controller can determine that there are 5 modules in the chain by looking at the counting bytes. The data `0x08 0x05 0x0C 0x0C 0x0F` corresponds to the characters `HELLO` in the character map. 

**Let's finish that message:**

We will write `WORLD` to the modules. This corresponds to the character indexes `0x17 0x0F 0x12 0x0C 0x04`. This will require some sequential write messages. We will use the header `0x85` for this.

![Example Write Seq](images/example_write_seq.png)

We can see 5 header and data packets being transmitted by the controller followed by an ACK byte. On the receiving site we only see the ACK byte being received, this is because every module 'takes' one message and retransmits the remaining messages. By the message return to the controller, only the ACK remains.

**Finally we clear the display:**

We will write the character index `0x00` to all modules in the chain. We will use the header `0xC5` for this.

![Example Write All](images/example_write_all.png)

We see the header, followed by the data, followed by the ACK byte transmitted by the controller. The exact same data passes through all modules and is received by the controller.