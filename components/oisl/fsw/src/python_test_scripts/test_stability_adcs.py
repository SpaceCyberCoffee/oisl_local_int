import matplotlib.pyplot as plt
import numpy as np

# Function to read data from file
def read_data(file_path):
    data = []
    with open(file_path, 'r') as file:
        for line in file:
            # Splitting each line by space and converting to float
            row = [float(x) for x in line.split()]
            data.append(row)
    return np.array(data)

def analyze_binary_series(binary_series):
    # Step 1: Initialize variables
    intervals = []
    first_alignment_time = None
    last_index = -1
    
    # Step 2: Iterate through the binary series
    count_1s = 0
    count_0s = 0
    in_first_interval = False

    for i, value in enumerate(binary_series):
        if value == 1:
            count_1s += 1
            if count_1s == 5 and first_alignment_time is None:
                first_alignment_time = i - 5
        else:
            if count_1s >= 5:
                intervals.append(count_1s)
                in_first_interval = True  # Mark that we've found the first interval
            if in_first_interval:
                count_0s += 1  # Count 0s only after the first interval
            count_1s = 0
                
        # Update last_index for the last seen '1'
        if value == 1:
            last_index = i

    # Check if there's an interval at the end of the string
    if count_1s >= 5:
        intervals.append(count_1s)
    
    # Step 3: Calculate average duration
    average_duration = np.mean(intervals) if intervals else 0


    # Step 5: Calculate average reestablishment time
    # Step 5: Calculate average reestablishment time
    reestablish_times = []
    count_0s = 0
    in_interval = False
    
    for i in range(len(binary_series)):
        if binary_series[i] == 1:
            if in_interval:
                # If we were in a 0 interval, record its length
                if count_0s > 0:
                    reestablish_times.append(count_0s)
            # Start a new interval of 1s
            in_interval = True
            count_0s = 0  # Reset 0 count
        else:
            if in_interval:
                count_0s += 1  # Count zeros only if we're in a 1 interval

    # Calculate average reestablishment time
    average_reestablishment_time = np.mean(reestablish_times) if reestablish_times else 0
    
    return average_duration, first_alignment_time, average_reestablishment_time

# Function to plot data
def plot_data(data):
    # Plotting the first column against row numbers
    plt.figure(figsize=(10, 5))
    plt.plot(data[:, 0], label='Alignment')
    plt.xlabel('SIm TIme')
    plt.ylabel('Alignment')
    average_duration, timeOfFirstAlignment, averageReestablishmentTime = analyze_binary_series(data[:, 0])
    print(average_duration, averageReestablishmentTime, timeOfFirstAlignment)
    # Create a legend with the calculated values
    legend_text = (f'Time of First Alignment: {timeOfFirstAlignment}\n'
                   f'Average Duration: {average_duration:.2f}\n'
                   f'Average Reestablishment Time: {averageReestablishmentTime:.2f}')
    
    plt.xlabel('Sim Time')
    plt.ylabel('Alignment')
    plt.title('Plot of Alignment with FOR 3 Degrees, CubeSat')
    plt.legend([legend_text], loc='upper right', prop={'size': 15})  # Add the legend with formatted text
    # plt.show()
    plt.savefig("/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/CubeSat_3deg.png")

    # Plotting the second column against row numbers
    # plt.figure(figsize=(10, 5))
    # plt.plot(data[:, 1], label='Scalar Product')
    # plt.xlabel('SIm TIme')
    # plt.ylabel('Alignment product')
    # plt.title('Plot of Scalar Product SoS')
    # plt.legend()
    # plt.show()
    
    # # Plotting the third column against row numbers
    # plt.figure(figsize=(10, 5))
    # plt.plot(data[:, 2], label='Change of OISL vector')
    # plt.xlabel('Sim TIme')
    # plt.ylabel('Percentage change')
    # plt.title('Plot of Change of OISL vector SoS(previous, current)')
    # plt.legend()
    # # plt.show()
    # plt.savefig("/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/ChangeISLScalar.png")
    
    # Plotting the second column against row numbers WITHOUT OUTLIERS
    # plt.figure(figsize=(10, 5))
    # plt.plot([dat for dat in data[0:, 1] if dat>0.996], label='Alignment Scalar Product')
    # plt.xlabel('Sim Time')
    # plt.ylabel('Scalar Product')
    # plt.axhline(y=0.9998, color='r', linestyle='-', label='1 degree')  # 1 degree
    # plt.axhline(y=0.99863, color='g', linestyle='-', label = '3 degrees') # 3 degree
    # plt.title('Scalar Product between ISL vector and b2')
    # plt.legend()
    #plt.show()
    # plt.savefig("/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/2Vectors_PT3.png")

if __name__ == "__main__":
    file_path = '/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/TEST_CUBESAT_STABILITY_3.txt'  # Path to your data file
    data = read_data(file_path)
    plot_data(data)