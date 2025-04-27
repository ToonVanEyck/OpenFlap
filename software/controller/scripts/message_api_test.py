#! /usr/bin/env python3

import requests
import json
import time

# Constants
display_width = 9  # Maximum characters per line
display_height = 2  # Number of lines

massage_delay_seconds = 2  # Delay between messages

# List of text entries to display
messages = [
    ["OPENFLAP", "#OHS2025"],
    ["MAY 30,31", "EDINBURGH"],
    [" SEE YOU ", "  THERE  "],
    ["", ""],
]


def send_message(message_lines):
    """Convert a multi-line message to JSON format and send via API"""
    # Create the JSON payload
    json_data = []

    # Create a 2D array to represent the display
    display = []
    for line_idx, line in enumerate(message_lines):
        # Pad the line with spaces to fill the width
        padded_line = line.ljust(display_width)
        display.append(padded_line[:display_width])

    # Fill any remaining lines with spaces
    while len(display) < display_height:
        display.append(" " * display_width)

    # Process the display column by column (instead of row by row)
    for col_idx in range(display_width):
        for row_idx in range(display_height):
            module_index = col_idx * display_height + row_idx
            character = display[row_idx][col_idx]
            json_data.append({"module": module_index, "character": character})

    # Send the POST request
    url = "http://openflap.local/api/module"
    headers = {"Content-Type": "application/json"}

    try:
        print(f"Sending message: {message_lines}")
        print(f"JSON payload: {json.dumps(json_data)}")
        response = requests.post(url, headers=headers, data=json.dumps(json_data))
        print(f"Response status code: {response.status_code}")
    except Exception as e:
        print(f"Error sending request: {e}")


def main():
    """Main function to process all messages"""
    # Validate that all messages fit within the display width
    for message in messages:
        for line in message:
            if len(line) > display_width:
                raise ValueError(
                    f"Message line '{line}' exceeds maximum width of {display_width} characters."
                )
    for message in messages:
        send_message(message)
        print(f"Waiting {massage_delay_seconds} seconds before next message...")
        time.sleep(massage_delay_seconds)
    print("All messages sent!")


if __name__ == "__main__":
    main()
