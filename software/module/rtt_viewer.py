#! /usr/bin/env python3

from pyocd.core.helpers import ConnectHelper
import struct
import time
import re
from collections import deque
# import numpy as np
import json
import argparse

# Bokeh imports for web-based visualization
from bokeh.plotting import figure, curdoc
from bokeh.layouts import column, row
from bokeh.models import (ColumnDataSource, Div, TextInput, ColorPicker, Slider, 
                         CheckboxGroup, Button, PreText, TextAreaInput, Select,
                         HoverTool, CrosshairTool)  # Remove Span, CustomJS imports
from bokeh.server.server import Server
from bokeh.application import Application
from bokeh.application.handlers import FunctionHandler
import threading
import tornado
import asyncio

# Location of the RTT control block (update this!)
RTT_CB_ADDR = 0x20000000  # default, might need adjustment

# Unique marker
RTT_ID = b'SEGGER RTT'

# Generic function to read a null-terminated C string from memory
def read_c_string(target, address, max_length=64):
    """
    Read a null-terminated C string from the target memory.
    
    Args:
        target: PyOCD target object
        address: Memory address to read from
        max_length: Maximum number of bytes to read (default 64)
    
    Returns:
        Decoded string or "Invalid" if reading fails
    """
    if address == 0:
        return "NULL"
    
    try:
        # Read bytes from memory
        string_bytes = target.read_memory_block8(address, max_length)
        # Find null terminator
        null_pos = next((i for i, b in enumerate(string_bytes) if b == 0), len(string_bytes))
        # Decode to string
        return bytes(string_bytes[:null_pos]).decode('utf-8', errors='replace')
    except:
        return "Invalid"

class RTTBuffer:
    """Class representing an RTT buffer (up or down)"""
    
    def __init__(self, target, base_address, buffer_index, direction='up'):
        self.target = target
        self.base_address = base_address
        self.buffer_index = buffer_index
        self.direction = direction  # 'up' or 'down'
        self._refresh_fields()
        self.is_scope_buffer = False
        self.scope_format = []
        self.packet_size = 0
        self._parse_scope_format()

    def _parse_scope_format(self):
        """Parse buffer name to determine if it's a scope buffer with structured data"""
        if not self.name or self.name in ["NULL", "Invalid"]:
            return
        
        # Pattern to match scope format: combination of b, f, u1, u2, u4, i1, i2, i4
        pattern = r'^(b|f|u[124]|i[124])+$'
        
        if not re.match(pattern, self.name):
            return
        
        # Parse individual format specifiers
        format_pattern = r'(b|f|u[124]|i[124])'
        matches = re.findall(format_pattern, self.name)
        
        if not matches:
            return
        
        self.is_scope_buffer = True
        self.scope_format = []
        self.packet_size = 0
        
        for match in matches:
            if match == 'b':
                self.scope_format.append(('bool', 1, 'B'))  # unsigned char for bool
                self.packet_size += 1
            elif match == 'f':
                self.scope_format.append(('float', 4, 'f'))  # float
                self.packet_size += 4
            elif match.startswith('u'):
                size = int(match[1])
                if size == 1:
                    self.scope_format.append(('uint8', 1, 'B'))
                elif size == 2:
                    self.scope_format.append(('uint16', 2, 'H'))
                elif size == 4:
                    self.scope_format.append(('uint32', 4, 'I'))
                self.packet_size += size
            elif match.startswith('i'):
                size = int(match[1])
                if size == 1:
                    self.scope_format.append(('int8', 1, 'b'))
                elif size == 2:
                    self.scope_format.append(('int16', 2, 'h'))
                elif size == 4:
                    self.scope_format.append(('int32', 4, 'i'))
                self.packet_size += size
    
    def _refresh_fields(self):
        """Read current buffer state from memory"""
        self.name_ptr = self.target.read32(self.base_address + 0)
        self.buf_ptr = self.target.read32(self.base_address + 4)
        self.buf_size = self.target.read32(self.base_address + 8)
        self.wr_off = self.target.read32(self.base_address + 12)
        self.rd_off = self.target.read32(self.base_address + 16)
        self.flags = self.target.read32(self.base_address + 20)
        self.name = read_c_string(self.target, self.name_ptr, 32)
    
    def write_data(self, data):
        """Write data to down buffer"""
        if self.direction != 'down':
            raise ValueError("Can only write to down buffers!")
        
        self._refresh_fields()
        
        if isinstance(data, str):
            data = data.encode('utf-8')
        
        data_len = len(data)
        if data_len == 0:
            return 0
        
        # Calculate available space in circular buffer
        # Need to leave at least one byte free to distinguish full from empty
        if self.wr_off >= self.rd_off:
            # Write pointer is ahead of or equal to read pointer
            available = self.buf_size - self.wr_off + self.rd_off - 1
        else:
            # Write pointer has wrapped around and is behind read pointer
            available = self.rd_off - self.wr_off - 1
        
        # Limit data to available space
        write_len = min(data_len, available)
        if write_len <= 0:
            return 0
        
        data_to_write = data[:write_len]
        
        try:
            # Check if we can write without wrapping
            space_to_end = self.buf_size - self.wr_off
            
            if write_len <= space_to_end:
                # No wrap-around needed - all data fits before buffer end
                self.target.write_memory_block8(self.buf_ptr + self.wr_off, data_to_write)
                new_wr_off = (self.wr_off + write_len) % self.buf_size
            else:
                # Wrap-around needed - split the write
                first_part_len = space_to_end
                second_part_len = write_len - first_part_len
                
                # Write first part (to end of buffer)
                self.target.write_memory_block8(self.buf_ptr + self.wr_off, data_to_write[:first_part_len])
                # Write second part (from beginning of buffer)
                self.target.write_memory_block8(self.buf_ptr, data_to_write[first_part_len:])
                
                new_wr_off = second_part_len
            
            # Update write offset
            self.target.write32(self.base_address + 12, new_wr_off)
            self.wr_off = new_wr_off
            
            return write_len
            
        except Exception as e:
            print(f"Error writing to down buffer: {e}")
            return 0

    def read_data(self):
        """Read available data from the buffer and update read offset"""
        self._refresh_fields()

        if(self.direction == 'down'):
            raise NotImplementedError("Reading from a down buffer is not supported!")
        
        if self.wr_off == self.rd_off:
            return b''

        if self.wr_off > self.rd_off:
            size = self.wr_off - self.rd_off
            data = self.target.read_memory_block8(self.buf_ptr + self.rd_off, size)
            new_rd_off = self.wr_off
        else:
            # Wrap-around
            size1 = self.buf_size - self.rd_off
            size2 = self.wr_off
            part1 = self.target.read_memory_block8(self.buf_ptr + self.rd_off, size1)
            part2 = self.target.read_memory_block8(self.buf_ptr, size2)
            data = part1 + part2
            new_rd_off = self.wr_off

        # Write the updated read offset back to the buffer structure
        self.target.write32(self.base_address + 16, new_rd_off)
        self.rd_off = new_rd_off

        return bytes(data)
    
    def read_scope_data(self):
        """Read and parse scope data according to buffer format"""
        if not self.is_scope_buffer:
            raise ValueError(f"Buffer '{self.name}' is not a valid scope buffer. Name must match pattern like 'u4u2b'")
        
        if self.packet_size == 0:
            raise ValueError(f"Invalid scope format for buffer '{self.name}'")
        
        raw_data = self.read_data()
        if not raw_data:
            return []
        
        # Check if we have complete packets
        if len(raw_data) % self.packet_size != 0:
            print(f"Warning: Incomplete packet data. Got {len(raw_data)} bytes, expected multiple of {self.packet_size}")
            # Truncate to complete packets
            raw_data = raw_data[:len(raw_data) - (len(raw_data) % self.packet_size)]
        
        packets = []
        num_packets = len(raw_data) // self.packet_size
        
        for packet_idx in range(num_packets):
            packet_start = packet_idx * self.packet_size
            packet_data = raw_data[packet_start:packet_start + self.packet_size]
            
            parsed_packet = []
            offset = 0
            
            for field_type, field_size, struct_format in self.scope_format:
                field_data = packet_data[offset:offset + field_size]
                
                # Unpack the data according to format (little-endian)
                try:
                    value = struct.unpack('<' + struct_format, field_data)[0]
                    
                    # Convert bool values to actual boolean
                    if field_type == 'bool':
                        value = bool(value)
                    
                    parsed_packet.append({
                        'type': field_type,
                        'value': value
                    })
                except struct.error as e:
                    raise ValueError(f"Failed to parse {field_type} at offset {offset}: {e}")
                
                offset += field_size
            
            packets.append(parsed_packet)
        
        return packets
    
    def is_valid(self):
        """Check if buffer appears to be valid"""
        return (self.buf_ptr != 0 and 
                self.buf_size > 0 and 
                self.buf_size < 0x100000 and  # Reasonable size limit
                self.wr_off < self.buf_size and 
                self.rd_off < self.buf_size)
    
    def print_info(self):
        """Print buffer information"""
        self._refresh_fields()
        print(f"Base address: 0x{self.base_address:08X}")
        print(f"  Name: \"{self.name}\"")
        print(f"  Buffer Ptr:  0x{self.buf_ptr:08X}")
        print(f"  Size:        {self.buf_size}")
        print(f"  Write Off:   {self.wr_off}")
        print(f"  Read Off:    {self.rd_off}")
        print(f"  Flags:       0x{self.flags:08X}")
        print(f"  Name Ptr:    0x{self.name_ptr:08X}")
        if self.is_scope_buffer:
            print(f"  Scope Buffer: YES (packet size: {self.packet_size} bytes)")
            print(f"  Format: {[f'{fmt[0]}({fmt[1]}B)' for fmt in self.scope_format]}")
        if not self.is_valid():
            print(f"  WARNING: Buffer appears invalid")

class RTTControlBlock:
    """Class representing the RTT control block"""
    
    def __init__(self, target, base_address):
        self.target = target
        self.base_address = base_address
        self._parse_control_block()
    
    def _parse_control_block(self):
        """Parse the RTT control block structure"""
        self.id = read_c_string(self.target, self.base_address, 16)
        self.max_up = self.target.read32(self.base_address + 16)
        self.max_down = self.target.read32(self.base_address + 20)
        
        # Create buffer objects
        self.up_buffers = []
        self.down_buffers = []
        
        # Up buffers start at offset 24
        for i in range(self.max_up):
            buf_addr = self.base_address + 24 + i * 24
            self.up_buffers.append(RTTBuffer(self.target, buf_addr, i, 'up'))
        
        # Down buffers follow up buffers
        down_start = self.base_address + 24 + self.max_up * 24
        for i in range(self.max_down):
            buf_addr = down_start + i * 24
            self.down_buffers.append(RTTBuffer(self.target, buf_addr, i, 'down'))
    
    def is_valid(self):
        """Check if control block appears valid"""
        return (self.id.startswith("SEGGER RTT") and 
                self.max_up <= 16 and 
                self.max_down <= 16)
    
    def print_info(self, dump_memory=False, num_words=0x30):
        """Print control block information"""
        if dump_memory:
            print(f"Dumping RTT control block at 0x{self.base_address:08X}:")
            for i in range(num_words):
                addr = self.base_address + i * 4
                val = self.target.read32(addr)
                print(f"0x{addr:08X}: 0x{val:08X}")
        
        print(f"\nSEGGER RTT ID: {self.id!r}")
        print(f"MaxNumUpBuffers: {self.max_up}")
        print(f"MaxNumDownBuffers: {self.max_down}")
        
        if not self.is_valid():
            print("WARNING: Control block appears invalid")
            return
        
        print("\nUp Buffers:")
        for i, buffer in enumerate(self.up_buffers):
            print(f"\nUp buffer [{i}]")
            buffer.print_info()
        
        print("\nDown Buffers:")
        for i, buffer in enumerate(self.down_buffers):
            print(f"\nDown buffer [{i}]")
            buffer.print_info()
    
    def get_up_buffer(self, index):
        """Get up buffer by index"""
        if 0 <= index < len(self.up_buffers):
            return self.up_buffers[index]
        return None
    
    def get_down_buffer(self, index):
        """Get down buffer by index"""
        if 0 <= index < len(self.down_buffers):
            return self.down_buffers[index]
        return None
    
    def update_plot(self):
        """Read new data and update plots"""
        try:
            packets = self.scope_buffer.read_scope_data()
            
            if packets:
                for packet in packets:
                    # Add each field value to its corresponding queue
                    for i, field_data in enumerate(packet):
                        if i < len(self.data_queues):
                            self.data_queues[i].append(field_data['value'])
                
                # Update plots
                for i, (line, ax, queue) in enumerate(zip(self.lines, self.axes, self.data_queues)):
                    if queue:
                        x_data = list(range(len(queue)))
                        y_data = list(queue)
                        
                        line.set_data(x_data, y_data)
                        
                        # Auto-scale y-axis
                        if y_data:
                            y_min, y_max = min(y_data), max(y_data)
                            margin = (y_max - y_min) * 0.1 if y_max != y_min else 1
                            ax.set_ylim(y_min - margin, y_max + margin)
                
                # Update x-axis for all plots
                for ax, queue in zip(self.axes, self.data_queues):
                    if queue:
                        ax.set_xlim(0, len(queue))
                
                plt.draw()
                plt.pause(0.001)  # Small pause to update display
                
        except Exception as e:
            print(f"Plot update error: {e}")

class RTTBokehPlotter:
    """High-performance real-time web-based plotter with streaming API and terminal"""
    
    def __init__(self, scope_buffer, rtt_cb, max_points=1000, config_file=None):
        self.scope_buffer = scope_buffer
        self.rtt_cb = rtt_cb
        self.max_points = max_points
        self.config_file = config_file
        self.config = {}

        if not scope_buffer.is_scope_buffer:
            raise ValueError("Buffer is not a scope buffer")
        
        # Load configuration if a file is provided
        if self.config_file:
            self.load_config()

        # Terminal buffer references
        self.terminal_buffer = rtt_cb.get_up_buffer(0)  # RTT channel 0 up
        self.terminal_down_buffer = rtt_cb.get_down_buffer(0)  # RTT channel 0 down
        
        # Terminal data storage
        self.terminal_data = []
        self.max_terminal_lines = 1000
        
        # Initialize data streams
        self.streams = []
        self.data_sources = []
        self.line_renderers = []
        self.controls = {}
        self.sample_count = 0
        
        # Performance optimization: batch updates
        self.update_batch_size = 10
        self.pending_updates = []
        
        # Default colors for streams
        self.default_colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', 
                              '#9467bd', '#8c564b', '#e377c2', '#7f7f7f']
        
        # Define scale options for dropdown
        self.scale_options = {
            "x0.001": 0.001,
            "x0.002": 0.002,
            "x0.005": 0.005,
            "x0.01": 0.01, 
            "x0.02": 0.02, 
            "x0.05": 0.05, 
            "x0.1": 0.1, 
            "x0.2": 0.2, 
            "x0.5": 0.5, 
            "x1": 1.0,
            "x2": 2.0,
            "x5": 5.0,
            "x10": 10.0, 
            "x20": 20.0, 
            "x50": 50.0, 
            "x100": 100.0, 
            "x200": 200.0, 
            "x500": 500.0, 
            "x1000": 1000.0, 
            "x2000": 2000.0, 
            "x5000": 5000.0
        }
        
        # Convert numeric scale to string representation for dropdown
        def get_scale_key(value):
            # Find closest matching scale value
            for key, scale in self.scale_options.items():
                if abs(scale - value) < 0.001:
                    return key
            return "x1"  # Default if no match
        
        for i, (field_type, _, _) in enumerate(scope_buffer.scope_format):
            stream_info = {
                'name': f"Field{i}_{field_type}",
                'original_name': f"Field{i}_{field_type}",
                'color': self.default_colors[i % len(self.default_colors)],
                'scale': 1.0,
                'scale_key': "x1",  # Add key for dropdown
                'offset': 0.0,
                'visible': True,
                'raw_data': [],
                'scaled_data': [],
                'stats': {'min': float('inf'), 'max': float('-inf'), 'sum': 0, 'count': 0}
            }

            # Apply settings from configuration if available
            if self.config.get(f"stream_{i}"):
                stream_config = self.config[f"stream_{i}"]
                scale_value = stream_config.get('scale', stream_info['scale'])
                stream_info.update({
                    'name': stream_config.get('name', stream_info['name']),
                    'color': stream_config.get('color', stream_info['color']),
                    'scale': scale_value,
                    'scale_key': get_scale_key(scale_value),
                    'offset': stream_config.get('offset', stream_info['offset']),
                    'visible': stream_config.get('visible', stream_info['visible']),
                })

            self.streams.append(stream_info)
            
            # Create optimized data source with initial capacity
            initial_data = {'x': [], 'y': []}
            source = ColumnDataSource(data=initial_data)
            self.data_sources.append(source)

        # Add pause state
        self.paused = False

    def save_config(self):
        """Save the current stream settings to the configuration file."""
        if not self.config_file:
            return
        
        self.config = {}
        for i, stream in enumerate(self.streams):
            self.config[f"stream_{i}"] = {
                'name': stream['name'],
                'color': stream['color'],
                'scale': stream['scale'],
                'offset': stream['offset'],
                'visible': stream['visible']
            }
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=4)
        except Exception as e:
            print(f"Error saving configuration: {e}")

    def load_config(self):
        """Load stream settings from the configuration file."""
        try:
            with open(self.config_file, 'r') as f:
                self.config = json.load(f)
        except FileNotFoundError:
            print(f"Configuration file '{self.config_file}' not found. Using default settings.")
        except json.JSONDecodeError as e:
            print(f"Error parsing configuration file: {e}. Using default settings.")

    def create_bokeh_app(self):
        """Create high-performance Bokeh application with split layout and terminal"""
        def modify_doc(doc):
            # Create title with live stats
            self.title_div = Div(text=f"<h2>RTT Scope Data: {self.scope_buffer.name} - Samples: 0</h2>", width=800)
            
            # LEFT PANE - Existing scope visualization
            # Create optimized main plot
            self.main_plot = figure(
                title="All Data Streams",
                x_axis_label='Sample',
                y_axis_label='Value',
                width=800,
                height=400,
                tools="pan,wheel_zoom,box_zoom,reset,save",
                # Performance optimizations
                toolbar_location="above",
                output_backend="webgl"  # Use WebGL for better performance
            )
            
            # Configure plot for better performance
            self.main_plot.toolbar.autohide = True
            
            # Create hover tool with tooltips for each line separately
            hover_tools = []
            
            # Add optimized lines for each stream
            for i, (stream, source) in enumerate(zip(self.streams, self.data_sources)):
                line = self.main_plot.line(
                    'x', 'y', source=source, 
                    line_width=1.5,  # Slightly thinner for performance
                    color=stream['color'], 
                    alpha=0.9,
                    nonselection_alpha=0.9,
                    name=stream['name']  # Add name for hover tool
                )
                self.line_renderers.append(line)
                
                # Create a separate hover tool for each line
                hover = HoverTool(
                    tooltips=[
                        ("Sample", "$x{0}"),
                        (f"{stream['name']}", "$y{0.000}")
                    ],
                    mode="mouse",
                    point_policy="follow_mouse",
                    line_policy="nearest",
                    renderers=[line],  # Only apply to this specific line
                    formatters={'$x': 'numeral', '$y': 'numeral'}
                )
                hover_tools.append(hover)
            
            # Add the hover tools to the plot
            for hover in hover_tools:
                self.main_plot.add_tools(hover)
            
            # Add crosshair tool
            crosshair = CrosshairTool(
                line_color='gray',
                line_alpha=0.5,
                line_width=1
            )
            self.main_plot.add_tools(crosshair)
            
            # Create optimized legend
            from bokeh.models import LegendItem, Legend
            legend_items = []
            for i, (stream, renderer) in enumerate(zip(self.streams, self.line_renderers)):
                legend_item = LegendItem(label=stream['name'], renderers=[renderer])
                legend_items.append(legend_item)
            
            self.legend = Legend(items=legend_items, location="top_left", 
                               click_policy="hide", margin=5)
            self.main_plot.add_layout(self.legend, 'right')
            
            # Create performance controls
            perf_title = Div(text="<h3>Performance Controls</h3>", width=800)
            
            # Add pause/resume button
            self.pause_button = Button(
                label="Pause Capture",
                button_type="warning",
                width=100
            )
            self.pause_button.on_click(self.toggle_pause)
            
            # Update rate slider
            self.update_rate_slider = Slider(
                start=20, end=200, value=50, step=10,
                title="Update Rate (ms)", width=150
            )
            self.update_rate_slider.on_change('value', self.update_rate_changed)
            
            # Batch size slider
            self.batch_size_slider = Slider(
                start=1, end=100, value=20, step=1,
                title="Batch Size", width=150
            )
            self.batch_size_slider.on_change('value', self.batch_size_changed)
            
            # Max points slider for performance tuning
            self.max_points_slider = Slider(
                start=100, end=2000, value=self.max_points, step=100,
                title="Max Points", width=150
            )
            self.max_points_slider.on_change('value', self.max_points_changed)
            
            # Performance stats
            self.perf_div = Div(text="Performance: 0 FPS | 0 samples/s", width=300)
            
            perf_controls = row([self.pause_button, self.update_rate_slider, self.batch_size_slider, 
                               self.max_points_slider])
            perf_row = row([perf_controls, self.perf_div])
            
            # Create streamlined control panel
            controls_title = Div(text="<h3>Stream Controls</h3>", width=800)
            
            # Create compact controls for each stream
            control_rows = []
            for i, stream in enumerate(self.streams):
                # Compact row with essential controls
                visibility_checkbox = CheckboxGroup(labels=[""], active=[0] if stream['visible'] else [], 
                                                   width=30, height=31)
                visibility_checkbox.on_change('active', lambda attr, old, new, idx=i: self.update_stream_visibility(idx, len(new) > 0))
                
                stream_label = Div(text=f"<b>{i}:</b>", width=30)
                
                name_input = TextInput(value=stream['name'], width=100, height=31)
                name_input.on_change('value', lambda attr, old, new, idx=i: self.update_stream_name(idx, new))
                
                color_picker = ColorPicker(color=stream['color'], width=40, height=31)
                color_picker.on_change('color', lambda attr, old, new, idx=i: self.update_stream_color(idx, new))
                
                # Replace scale slider with dropdown
                scale_select = Select(
                    value=stream['scale_key'],
                    options=list(self.scale_options.keys()),
                    width=80, height=31
                )
                scale_select.on_change('value', lambda attr, old, new, idx=i: self.update_stream_scale(idx, self.scale_options[new]))
                
                offset_slider = Slider(start=-1000, end=1000, value=stream['offset'], 
                                     step=1, width=80, height=31)
                offset_slider.on_change('value', lambda attr, old, new, idx=i: self.update_stream_offset(idx, new))
                
                # Store controls
                self.controls[i] = {
                    'visibility': visibility_checkbox, 'name': name_input, 
                    'color': color_picker, 'scale': scale_select, 'offset': offset_slider
                }
                
                control_row = row([visibility_checkbox, stream_label, name_input, color_picker, 
                                 scale_select, offset_slider])
                control_rows.append(control_row)
            
            # Create live statistics with better formatting
            stats_title = Div(text="<h3>Live Statistics</h3>", width=800)
            self.stats_div = Div(text="<i>Waiting for data...</i>", width=800)
            
            # LEFT PANE LAYOUT
            left_pane = column([
                self.title_div,
                self.main_plot,
                perf_title,
                perf_row,
                controls_title,
                column(control_rows),
                stats_title,
                self.stats_div
            ], width=850)
            
            # RIGHT PANE - Terminal
            terminal_title = Div(text="<h2>RTT Terminal (Channel 0)</h2>", width=600)
            
            # Terminal display area - Change to Div instead of PreText for HTML support
            self.terminal_display = Div(
                text="<pre style='margin:0'>RTT Terminal Ready...\n</pre>",
                width=600,
                height=400,
                styles={
                    'background-color': '#1e1e1e',
                    'color': '#e0e0e0',
                    'font-family': 'Courier New, monospace',
                    'font-size': '12px',
                    'overflow-y': 'auto',
                    'padding': '4px'
                }
            )
            
            # Terminal input area
            self.terminal_input = TextAreaInput(
                placeholder="Type here and press Enter to send to RTT...",
                width=600,
                height=100,
                styles={
                    'background-color': '#2d2d2d',
                    'color': '#ffffff',
                    'font-family': 'Courier New, monospace',
                    'font-size': '12px'
                }
            )
            self.terminal_input.on_change('value', self.on_terminal_input_change)
            
            # Terminal controls
            clear_button = Button(label="Clear Terminal", button_type="primary", width=100)
            clear_button.on_click(self.clear_terminal)
            
            send_button = Button(label="Send", button_type="success", width=100)
            send_button.on_click(self.send_terminal_input)
            
            terminal_controls = row([clear_button, send_button])
            
            # Terminal status
            self.terminal_status = Div(text="Status: Ready", width=600, 
                                     styles={'color': '#00aa00', 'font-weight': 'bold'})
            
            # RIGHT PANE LAYOUT
            right_pane = column([
                terminal_title,
                self.terminal_display,
                Div(text="<b>Input:</b>", width=600),
                self.terminal_input,
                terminal_controls,
                self.terminal_status
            ], width=650)
            
            # MAIN SPLIT LAYOUT
            main_layout = row([left_pane, right_pane])
            
            doc.add_root(main_layout)
            
            # Start with optimized callback
            self.last_update_time = time.time()
            self.frame_count = 0
            self.samples_processed = 0
            self.last_perf_update = time.time()
            
            # Start with higher performance settings
            initial_rate = 100  # 100ms updates for better FPS
            doc.add_periodic_callback(self.optimized_update_data, initial_rate)
        
        return modify_doc
    
    def ansi_to_html(self, text):
        """Convert ANSI terminal color codes to HTML spans"""
        # Define ANSI color code to CSS mapping
        ansi_colors = {
            '30': 'black', '31': '#e74c3c', '32': '#2ecc71', '33': '#f1c40f',
            '34': '#3498db', '35': '#9b59b6', '36': '#1abc9c', '37': '#e0e0e0',
            '90': '#7f8c8d', '91': '#ff5252', '92': '#00e676', '93': '#ffff00',
            '94': '#448aff', '95': '#e040fb', '96': '#18ffff', '97': 'white',
            '1': 'bold', '4': 'underline'
        }
        
        # Current style state
        current_styles = []
        result = ""
        i = 0
        
        # Strip out most control chars except the color codes we want to handle
        text = re.sub(r'[\x00-\x08\x0B-\x0C\x0E-\x1A\x1C-\x1F\x7F-\x9F]', '', text)
        
        while i < len(text):
            if text[i:i+2] == '\x1b[' or text[i:i+1] == '[':
                # This is the start of an escape sequence
                if text[i:i+2] == '\x1b[':
                    seq_start = i + 2  # Skip the escape character and [
                    i += 2
                else:
                    seq_start = i + 1  # Skip just the [
                    i += 1
                
                # Find the end of the sequence (denoted by 'm')
                seq_end = text.find('m', seq_start)
                if seq_end == -1:
                    # Invalid sequence, just skip it
                    i += 1
                    continue
                
                # Extract the code(s)
                codes = text[seq_start:seq_end].split(';')
                
                # If it's a reset code or empty, close all open spans
                if '0' in codes or not codes or '' in codes:
                    # Close all spans if any are open
                    if current_styles:
                        result += '</span>' * len(current_styles)
                        current_styles = []
                else:
                    # Handle each code
                    css_styles = []
                    for code in codes:
                        if code in ansi_colors:
                            if code in ('1', '4'):  # Bold or underline
                                css_styles.append(f'font-weight: {ansi_colors[code]}' if code == '1' else f'text-decoration: {ansi_colors[code]}')
                            elif int(code) >= 30 and int(code) < 40:  # Foreground color
                                css_styles.append(f'color: {ansi_colors[code]}')
                            elif int(code) >= 40 and int(code) < 50:  # Background color
                                bg_code = str(int(code) - 10)  # Convert bg to fg code
                                if bg_code in ansi_colors:
                                    css_styles.append(f'background-color: {ansi_colors[bg_code]}')
                            elif int(code) >= 90 and int(code) < 100:  # Bright foreground
                                css_styles.append(f'color: {ansi_colors[code]}')
                            elif int(code) >= 100 and int(code) < 110:  # Bright background
                                bg_code = str(int(code) - 10)  # Convert bg to fg code
                                if bg_code in ansi_colors:
                                    css_styles.append(f'background-color: {ansi_colors[bg_code]}')
                    
                    if css_styles:
                        # Close previous span if there's one open
                        if current_styles:
                            result += '</span>'
                            current_styles.pop()
                        
                        # Open a new span with combined styles
                        style_attr = '; '.join(css_styles)
                        result += f'<span style="{style_attr}">'
                        current_styles.append(style_attr)
                
                # Move past this escape sequence
                i = seq_end + 1
            else:
                # Regular character, add it to result
                result += text[i]
                i += 1
        
        # Close any remaining open spans
        if current_styles:
            result += '</span>' * len(current_styles)
        
        return result
    
    def update_terminal_display(self):
        """Update terminal display with new data from RTT channel 0"""
        if self.terminal_buffer and self.terminal_buffer.is_valid():
            try:
                data = self.terminal_buffer.read_data()
                if data:
                    # Decode and process ANSI color codes
                    text = data.decode('utf-8', errors='replace')
                    html_text = self.ansi_to_html(text)
                    
                    self.terminal_data.append(html_text)
                    
                    # Limit terminal history
                    if len(self.terminal_data) > self.max_terminal_lines:
                        self.terminal_data = self.terminal_data[-self.max_terminal_lines:]
                    
                    # Update display with HTML content
                    joined_content = ''.join(self.terminal_data)
                    self.terminal_display.text = f"<pre style='margin:0'>{joined_content}</pre>"
                    
                    # Auto-scroll to bottom - handled by CSS overflow for Div
                    
            except Exception as e:
                print(f"Terminal read error: {e}")
    
    def send_terminal_input(self):
        """Send terminal input to RTT down buffer"""
        if self.terminal_down_buffer and self.terminal_down_buffer.is_valid():
            try:
                text = self.terminal_input.value
                if text.strip():
                    # Add newline if not present
                    if not text.endswith('\n'):
                        text += '\n'
                    
                    # Write to down buffer
                    bytes_written = self.terminal_down_buffer.write_data(text)
                    
                    if bytes_written > 0:
                        # Echo to terminal display with styled prefix
                        echo_text = f"<span style='color: #888888'>&gt;&gt; {text}</span>"
                        self.terminal_data.append(echo_text)
                        joined_content = ''.join(self.terminal_data)
                        self.terminal_display.text = f"<pre style='margin:0'>{joined_content}</pre>"
                        
                        self.terminal_status.text = f"Status: Sent {bytes_written} bytes"
                        self.terminal_status.styles = {'color': '#00aa00', 'font-weight': 'bold'}
                    else:
                        self.terminal_status.text = "Status: Buffer full - try again"
                        self.terminal_status.styles = {'color': '#ff6600', 'font-weight': 'bold'}
                    
                    # Clear input
                    self.terminal_input.value = ""
                    
            except Exception as e:
                self.terminal_status.text = f"Status: Error - {str(e)}"
                self.terminal_status.styles = {'color': '#ff0000', 'font-weight': 'bold'}
                print(f"Terminal write error: {e}")
        else:
            self.terminal_status.text = "Status: No down buffer available"
            self.terminal_status.styles = {'color': '#ff6600', 'font-weight': 'bold'}
    
    def clear_terminal(self):
        """Clear terminal display"""
        self.terminal_data = []
        self.terminal_display.text = "<pre style='margin:0'>RTT Terminal Ready...\n</pre>"
        self.terminal_status.text = "Status: Terminal cleared"
        self.terminal_status.styles = {'color': '#00aa00', 'font-weight': 'bold'}
    
    def update_rate_changed(self, attr, old, new):
        """Update the callback rate"""
        # In production, we'd need to remove old callback and add new one
        # For now, just update batch size based on rate
        self.update_batch_size = max(5, int(new / 5))
    
    def batch_size_changed(self, attr, old, new):
        """Update batch processing size"""
        self.update_batch_size = new
    
    def max_points_changed(self, attr, old, new):
        """Update maximum points displayed"""
        self.max_points = new
        # Clear existing data to immediately apply new limit
        for stream in self.streams:
            if len(stream['raw_data']) > new:
                stream['raw_data'] = stream['raw_data'][-new:]
    
    def toggle_pause(self):
        """Toggle between paused and running states"""
        self.paused = not self.paused
        
        if self.paused:
            self.pause_button.label = "Resume Capture"
            self.pause_button.button_type = "success"
            self.perf_div.text = "Capture PAUSED - Press Resume to continue"
        else:
            self.pause_button.label = "Pause Capture"
            self.pause_button.button_type = "warning"
            # Reset timing stats
            self.last_update_time = time.time()
            self.frame_count = 0
            self.samples_processed = 0
    
    def optimized_update_data(self):
        """Optimized data update with batching and streaming"""
        try:
            # Always update terminal display, even when paused
            self.update_terminal_display()
            
            # Skip data collection when paused
            if self.paused:
                return
                
            # Read data more efficiently - don't read packet by packet
            all_packets = []
            
            # Try to read multiple times to get a good batch
            for _ in range(self.update_batch_size):
                packets = self.scope_buffer.read_scope_data()
                if packets:
                    all_packets.extend(packets)
                    # If we got a lot of data, stop early to avoid blocking too long
                    if len(all_packets) >= 50:
                        break
                else:
                    break
            
            if all_packets:
                # Process batch efficiently
                self.process_batch_data_optimized(all_packets)
                self.samples_processed += len(all_packets)
                
                # Update performance stats less frequently
                self.frame_count += 1
                if self.frame_count % 5 == 0:  # Update every 5 frames
                    self.update_performance_stats()
                
                # Update statistics even less frequently
                if self.frame_count % 20 == 0:  # Update stats every 20 frames
                    self.update_statistics()
                    
        except Exception as e:
            print(f"Optimized update error: {e}")
    
    def process_batch_data_optimized(self, packets_batch):
        """Process incoming data and store raw values while updating the view."""
        if not packets_batch:
            return
        
        num_streams = len(self.streams)
        stream_updates = [{'x': [], 'y': []} for _ in range(num_streams)]
        
        for packet in packets_batch:
            for i, field_data in enumerate(packet):
                if i < num_streams:
                    stream = self.streams[i]
                    raw_value = field_data['value']
                    
                    # Store raw data
                    stream['raw_data'].append(raw_value)
                    
                    # Update statistics
                    stats = stream['stats']
                    stats['min'] = min(stats['min'], raw_value)
                    stats['max'] = max(stats['max'], raw_value)
                    stats['sum'] += raw_value
                    stats['count'] += 1
                    
                    # Apply transformations for display only
                    if stream['visible']:
                        scaled_value = raw_value * stream['scale'] + stream['offset']
                        stream_updates[i]['x'].append(self.sample_count)
                        stream_updates[i]['y'].append(scaled_value)
                    
                    # Maintain window size
                    if len(stream['raw_data']) > self.max_points:
                        stream['raw_data'] = stream['raw_data'][-self.max_points:]
            
            self.sample_count += 1
        
        # Update the view for visible streams
        for i, (stream, update_data) in enumerate(zip(self.streams, stream_updates)):
            if update_data['x'] and stream['visible']:
                try:
                    self.data_sources[i].stream(update_data, rollover=self.max_points)
                except Exception as e:
                    print(f"Stream update error for stream {i}: {e}")

    def update_performance_stats(self):
        """Optimized performance statistics"""
        current_time = time.time()
        
        # Calculate FPS over larger intervals for stability
        if self.frame_count >= 5:
            elapsed = current_time - self.last_update_time
            fps = 5 / elapsed if elapsed > 0 else 0
            
            # Calculate samples per second
            samples_per_sec = self.samples_processed / elapsed if elapsed > 0 else 0
            
            self.perf_div.text = f"Performance: {fps:.1f} FPS | {samples_per_sec:.0f} samples/s | Total: {self.sample_count}"
            self.title_div.text = f"<h2>RTT Scope Data: {self.scope_buffer.name} - Samples: {self.sample_count}</h2>"
            
            # Reset counters
            self.last_update_time = current_time
            self.samples_processed = 0
            self.frame_count = 0
    
    def update_statistics(self):
        """Update statistics display efficiently"""
        stats_html = """
        <table border='1' style='border-collapse:collapse; font-size:12px;'>
        <tr style='background-color:#f0f0f0;'>
            <th>Stream</th><th>Count</th><th>Min</th><th>Max</th><th>Mean</th><th>Last</th>
        </tr>
        """
        
        for i, stream in enumerate(self.streams):
            stats = stream['stats']
            last_val = stream['raw_data'][-1] if stream['raw_data'] else 0
            
            if stats['count'] > 0:
                mean_val = stats['sum'] / stats['count']
                
                stats_html += f"""
                <tr>
                    <td style='color:{stream["color"]};font-weight:bold;'>{stream['name']}</td>
                    <td>{stats['count']}</td>
                    <td>{stats['min']:.3f}</td>
                    <td>{stats['max']:.3f}</td>
                    <td>{mean_val:.3f}</td>
                    <td>{last_val:.3f}</td>
                </tr>
                """
            else:
                stats_html += f"""
                <tr>
                    <td style='color:{stream["color"]};'>{stream['name']}</td>
                    <td colspan='5'><i>No data</i></td>
                </tr>
                """
        
        stats_html += "</table>"
        self.stats_div.text = stats_html
    
    def refresh_stream_display(self, stream_idx):
        """Reapply transformations to all raw data and update the view."""
        if stream_idx >= len(self.streams):
            return
        
        stream = self.streams[stream_idx]
        if not stream['visible'] or not stream['raw_data']:
            return
        
        # Recalculate all display values from raw data
        x_data = list(range(len(stream['raw_data'])))
        y_data = [raw_val * stream['scale'] + stream['offset'] for raw_val in stream['raw_data']]
        
        try:
            # Update the data source with the recalculated values
            self.data_sources[stream_idx].data = {'x': x_data, 'y': y_data}
        except Exception as e:
            print(f"Error refreshing stream {stream_idx}: {e}")
    
    def on_terminal_input_change(self, attr, old, new):
        """Handle terminal input changes (detect Enter key)"""
        if new.endswith('\n'):
            # Send the input when Enter is pressed
            self.send_terminal_input()
    
    # ...existing control methods remain the same...
    def update_stream_name(self, stream_idx, new_name):
        """Update stream name and save to configuration."""
        if stream_idx < len(self.streams):
            self.streams[stream_idx]['name'] = new_name
            self.update_legend()
            self.save_config()

    def update_legend(self):
        """Update the plot legend with current stream names"""
        self.legend.items = []
        from bokeh.models import LegendItem
        legend_items = []
        for i, (stream, renderer) in enumerate(zip(self.streams, self.line_renderers)):
            if stream['visible']:
                legend_item = LegendItem(label=stream['name'], renderers=[renderer])
                legend_items.append(legend_item)
        self.legend.items = legend_items
    
    def update_stream_color(self, stream_idx, new_color):
        """Update stream color and save to configuration."""
        if stream_idx < len(self.streams):
            self.streams[stream_idx]['color'] = new_color
            self.line_renderers[stream_idx].glyph.line_color = new_color
            self.save_config()

    def update_stream_scale(self, stream_idx, new_scale):
        """Update scale, refresh the view, and save to configuration."""
        if stream_idx < len(self.streams):
            self.streams[stream_idx]['scale'] = new_scale
            # Find and update the scale_key for next time we recreate the UI
            for key, scale in self.scale_options.items():
                if abs(scale - new_scale) < 0.001:
                    self.streams[stream_idx]['scale_key'] = key
                    break
            self.refresh_stream_display(stream_idx)
            self.save_config()

    def update_stream_offset(self, stream_idx, new_offset):
        """Update offset, refresh the view, and save to configuration."""
        if stream_idx < len(self.streams):
            self.streams[stream_idx]['offset'] = new_offset
            self.refresh_stream_display(stream_idx)
            self.save_config()

    def update_stream_visibility(self, stream_idx, visible):
        """Update stream visibility, refresh the view, and save to configuration."""
        if stream_idx < len(self.streams):
            self.streams[stream_idx]['visible'] = visible
            self.line_renderers[stream_idx].visible = visible
            self.update_legend()
            self.save_config()

class RTTWebServer:
    """Web server for RTT data visualization"""
    
    def __init__(self, rtt_cb, port=5006):
        self.rtt_cb = rtt_cb
        self.port = port
        self.server = None
        self.apps = []
        
    def setup_applications(self, config_file=None):
        """Setup Bokeh applications for each scope buffer"""
        for i, buffer in enumerate(self.rtt_cb.up_buffers):
            if buffer.is_valid() and buffer.is_scope_buffer:
                try:
                    # Create application with a factory function to avoid model reuse
                    app_name = f"/scope_{i}_{buffer.name}"
                    
                    def create_app_handler(buf=buffer, cfg=config_file):
                        def app_factory(doc):
                            # Create fresh plotter instance for each session with RTT CB access
                            plotter = RTTBokehPlotter(buf, self.rtt_cb, max_points=500, config_file=cfg)
                            return plotter.create_bokeh_app()(doc)
                        return app_factory
                    
                    handler = FunctionHandler(create_app_handler())
                    app = Application(handler)
                    
                    self.apps.append((app_name, app))
                    print(f"Created scope app: http://localhost:{self.port}{app_name}")
                    
                except Exception as e:
                    print(f"Failed to create app for buffer {i}: {e}")
    
    def start_server(self):
        """Start the Bokeh server"""
        if not self.apps:
            print("No scope buffers found for visualization")
            return False
        
        try:
            # Create server with applications
            apps = {name: app for name, app in self.apps}
            
            self.server = Server(
                apps,
                port=self.port,
                allow_websocket_origin=[f"localhost:{self.port}"],
                show=False  # Don't automatically open browser
            )
            
            # Start server in a separate thread
            def run_server():
                self.server.start()
                self.server.io_loop.start()
            
            server_thread = threading.Thread(target=run_server, daemon=True)
            server_thread.start()
            
            print(f"\nBokeh server started on http://localhost:{self.port}")
            print("Available applications:")
            for name, _ in self.apps:
                print(f"  - http://localhost:{self.port}{name}")
            
            return True
            
        except Exception as e:
            print(f"Failed to start Bokeh server: {e}")
            return False
    
    def stop_server(self):
        """Stop the Bokeh server"""
        if self.server:
            self.server.stop()

# Try to locate RTT CB within a memory window
def find_rtt_cb(target, search_start=0x20000000, search_size=0x1000):
    mem = target.read_memory_block8(search_start, search_size)
    for i in range(0, search_size - len(RTT_ID)):
        if bytes(mem[i:i+len(RTT_ID)]) == RTT_ID:
            return search_start + i
    return None

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="RTT Viewer with optional configuration file.")
    parser.add_argument("--config", type=str, help="Path to the JSON configuration file.")
    args = parser.parse_args()

    session = ConnectHelper.session_with_chosen_probe(target_override="py32f003x8")
    session.open()
    target = session.target
    target.resume()

    try:
        rtt_cb_addr = find_rtt_cb(target)
        if not rtt_cb_addr:
            print("RTT control block not found.")
        else:
            print(f"RTT control block found at 0x{rtt_cb_addr:08X}")
            
            # Create RTT control block object
            rtt_cb = RTTControlBlock(target, rtt_cb_addr)
            
            # Print control block info
            rtt_cb.print_info(dump_memory=False)
            
            if not rtt_cb.is_valid():
                print("Invalid RTT control block, exiting...")
            else:
                # Create web server for scope visualization
                web_server = RTTWebServer(rtt_cb, port=5006)
                
                # Pass config file directly to setup_applications
                web_server.setup_applications(config_file=args.config)
                
                if web_server.start_server():
                    print("\nWeb visualization started successfully!")
                    print("Open a web browser and navigate to the URLs above to view live data")
                
                # Get terminal buffer (buffer 0) for text output
                terminal_buffer = rtt_cb.get_up_buffer(0)

                if terminal_buffer and terminal_buffer.is_valid():
                    print(f"\nReading from terminal buffer: {terminal_buffer.name}")
                    print("=" * 50)
                    print("Press Ctrl+C to stop...")
                    
                    try:
                #         while True:
                #             # Read terminal data
                #             data = terminal_buffer.read_data()
                #             if data:
                #                 print(data.decode(errors='replace'), end='', flush=True)

                #             time.sleep(0.05)
                #     except KeyboardInterrupt:
                #         print("\nStopping web server...")
                #         web_server.stop_server()
                # else:
                #     print("No valid terminal buffer found")
                #     # Keep server running even without terminal
                #     try:
                #         print("Press Ctrl+C to stop web server...")
    #                     while True:
    #                         time.sleep(1)
    #                 except KeyboardInterrupt:
    #                     print("\nStopping web server...")
    #                     web_server.stop_server()
    # finally:
    #     session.close()
                        while True:
                            # Read terminal data
                            data = terminal_buffer.read_data()
                            if data:
                                print(data.decode(errors='replace'), end='', flush=True)

                            time.sleep(0.05)
                    except KeyboardInterrupt:
                        print("\nStopping web server...")
                        web_server.stop_server()
                else:
                    print("No valid terminal buffer found")
                    # Keep server running even without terminal
                    try:
                        print("Press Ctrl+C to stop web server...")
                        while True:
                            time.sleep(1)
                    except KeyboardInterrupt:
                        print("\nStopping web server...")
                        web_server.stop_server()
    finally:
        session.close()
