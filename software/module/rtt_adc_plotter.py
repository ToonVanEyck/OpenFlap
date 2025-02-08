#!/usr/bin/env python3
import sys
import re
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Store ADC values over time
x_data = []
y_data = [[], [], [], [], [], []]  # Six ADC channels

def update_plot(frame):
    """Read a new line from stdin and update the plot if it's ADC data."""
    line = sys.stdin.readline().strip()

    if not line:
        return  # Skip empty lines

    # Print all non-ADC output normally
    if not line.startswith("ADC:"):
        print(line)
        return

    # Extract ADC values using regex
    match = re.search(r"ADC:\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)", line)
    if match:
        adc_values = list(map(int, match.groups()))

        # Append data
        if len(x_data) > 50:  # Keep last 50 readings
            x_data.pop(0)
            for i in range(6):
                y_data[i].pop(0)

        x_data.append(len(x_data))  # Time index
        for i in range(6):
            y_data[i].append(adc_values[i])

        # Update plot
        ax.clear()
        for i in range(6):
            ax.plot(x_data, y_data[i], marker='o', linestyle='-', label=f"CH{i+1}")

        ax.legend()
        ax.set_title("ADC Values Over Time")
        ax.set_xlabel("Sample Number")
        ax.set_ylabel("ADC Value")
        ax.set_ylim(0, 1024)  # Assuming 10-bit ADC (0-1023)

# Set up live plotting
fig, ax = plt.subplots()
ani = animation.FuncAnimation(fig, update_plot, interval=500)

# Show the plot
plt.show()
