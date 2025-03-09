## Communication

The communication system between the controller and the modules is a daisy chain system based on UART. The controller sends a message to the first module, the first module processes the message and sends it to the next module. This continues until the last module in the chain has received the message.
