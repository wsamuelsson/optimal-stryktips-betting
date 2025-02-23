import itertools
import struct
# Define the possible outcomes
outcomes = [0, 1, 2]

# Generate all possible combinations of length 13
all_bets = itertools.product(outcomes, repeat=13)




# Define the filename
filename = 'PossibleBets.bin'

# Open the file in write mode
with open(filename, 'wb') as file:
    for bet in all_bets:
        
        # Convert each bet tuple to a string of numbers separated by spaces
        bet_binary = struct.pack("13i", *bet)
        # Write the bet string to the file followed by a newline
        file.write(bet_binary)

print(f"All possible bets have been written to {filename}")
