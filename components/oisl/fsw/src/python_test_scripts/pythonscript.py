import matplotlib.pyplot as plt
import numpy as np
import math

# Function to read and process the files
def process_files(file1, file2): # 42 e prop
    # Initialize lists to store the processed data
    forw_42 = []
    forw_prop = []

    # Process both files simultaneously
    with open(file1, 'r') as f1:
        for i, line1 in enumerate(f1):
            # Skip every other line
            if i % 2 == 0:
                # Extract and convert the numbers
                coords1 = line1.split()
                forw_42.append(float(coords1[0])/1000)
    with open(file2, 'r') as f2:
        for i, line2 in enumerate(f2):
            # Skip every other line 
            coords2 = line2.split()
            forw_prop.append(float(coords2[0]))

    return forw_42, forw_prop

def calculate_orbital_period(altitude_km):
    # Constants
    earth_radius_km = 6371  # Radius of the Earth in kilometers
    gravitational_constant = 6.674 * 10**-11  # Gravitational constant in N*m^2/kg^2
    earth_mass_kg = 5.972 * 10**24  # Mass of the Earth in kilograms

    # Calculate orbital period
    orbital_period_seconds = 2 * math.pi * math.sqrt((((earth_radius_km + altitude_km)*1000)**3) / (gravitational_constant * earth_mass_kg))

    return orbital_period_seconds

# Main execution
if __name__ == "__main__":
    # File names
    file42 = '/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/FORWARD_position_42_2.txt'
    filePROP = '/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/FORWARD_position_PROPAGATED.txt'

    # Read and process the files
    forward_42, forward_PROP = process_files(file42, filePROP)
    print(calculate_orbital_period(400)/24)
    print(len(forward_42), len(forward_PROP), 4%2)

    # Create the plot
    plt.figure(figsize=(10, 6))
    plt.plot(range(len(forward_42)), forward_42, label='42 SAT')
    plt.plot([i + 0 for i in range(len(forward_PROP))], forward_PROP, label='Propagated SAT')

    # Set up the axes
    plt.xlabel('Raw Number')
    plt.ylabel('Coordinate Value')
    plt.title('Comparison of 42 SAT and Prop SAT Coordinates')

    # Add legend and grid
    plt.legend()
    plt.grid(True)

    # Show the plot
    plt.tight_layout()
    plt.show()
    
