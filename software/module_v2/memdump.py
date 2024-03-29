import pyocd

# Connect to the target
with pyocd.tools.helpers.session_with_chosen_probe(unique_id=None) as session:
    board = session.board
    target = session.target

    # Specify the start address and length of the memory block you want to view
    start_address = 0x08007000  # Example start address
    length = 0x0fff  # Example length in bytes

    # Read the memory block
    memory_contents = target.read_memory_block8(start_address, length)

    # Print the memory contents
    print(memory_contents)