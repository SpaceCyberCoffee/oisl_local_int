import matplotlib.pyplot as plt
import os

# Function to read data from a file and return columns as lists
def read_data(file_path):
    time, col1, col2, col3, col4 = [], [], [], [], []
    with open(file_path, 'r') as file:
        for line in file:
            values = line.split()
            time.append(float(values[0]))
            col1.append(float(values[1]))
            col2.append(float(values[2]))
            col3.append(float(values[3]))
            col4.append(float(values[4]))
    return time, col1, col2, col3, col4

# Read data from the three files
file1 = "components/sample/fsw/src/backward_test.txt"
file2 = "components/sample/fsw/src/central_test.txt"
file3 = "components/sample/fsw/src/forward_test.txt"

data1 = read_data(file1)
data2 = read_data(file2)
data3 = read_data(file3)

# Plot column data
columns = ["ECI X", "ECI Y", "ECI Z", "Geocentric Latitude"]
for i in range(1, 5):
    plt.figure(figsize=(10, 6))
    plt.plot(data1[0], data1[i], label="Backward Satellite")
    plt.plot(data2[0], data2[i], label="Central Satellite")
    plt.plot(data3[0], data3[i], label="Forward Satellite")
    plt.xlabel('Simulation Time')
    plt.ylabel(columns[i-1])
    plt.title(f'{columns[i-1]} Values from Files')
    plt.legend()
    plt.grid(True)
    plt.show()
    
# Plot column data
columns = ["ECI X", "ECI Y", "ECI Z", "Geocentric Latitude"]
for i in range(1,5):
    plt.figure(figsize=(10, 6))
    plt.plot([t - 231 for t in data1[0]], data1[i], label="Backward Satellite")
    plt.plot(data2[0], data2[i], label="Central Satellite")
    plt.plot([t + 231 for t in data3[0]], data3[i], label="Forward Satellite")
    plt.xlabel('Simulation Time')
    plt.ylabel(columns[i-1])
    plt.title(f'{columns[i-1]} Values from Files normalized to Central Sat time')
    plt.legend()
    plt.grid(True)
    plt.show()