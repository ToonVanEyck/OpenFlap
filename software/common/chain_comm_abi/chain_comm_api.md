# Chain Communication API

The cain communication is the communication interface between a controller and a daisy chained string of OpenFlap modules.

The protocol uses UART as the data link layer. The UART is configured to 115200 baud, 8 data bits, 1 stop bit and no parity.

## Message Format

### Header

Each communication start with a header. The header is only 1 byte and communicates the intent of the message.

The two most significant bits of the header byte indicate the action. The remaining 6 bits indicate the property on which the action will be taken.

#### Actions

| Bits | Action           | Description                                             |
|------|------------------|---------------------------------------------------------|
| 00   | do nothing       | Do nothing.                                             |
| 01   | read all         | Read the property from all modules.                     |
| 10   | write sequential | Write a different value to the property of all modules. |
| 11   | write all        | Write the same value to the property of all modules.    |

#### Properties

