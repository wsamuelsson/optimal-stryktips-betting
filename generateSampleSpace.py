import itertools
import struct
import math 
# Define the possible outcomes
outcomes = [0, 1, 2]
num_games=None 
with open("data/num_games.bin", 'rb') as f:
    num_games = f.read()
    num_games  = struct.unpack("i", num_games)[0]

# Generate all possible combinations of length num_games
all_bets = itertools.product(outcomes, repeat=num_games)
# Define the filename
filename = 'data/PossibleBets.bin'

# Open the file in write mode
with open(filename, 'wb') as file:
    for bet in all_bets:
        # Convert each bet tuple to a string of numbers separated by spaces
        bet_binary = struct.pack(f"{num_games}i", *bet)
        # Write the bet string to the file followed by a newline
        file.write(bet_binary)
print(f"All possible {int(math.pow(3, num_games))} bets have been written to {filename}")
