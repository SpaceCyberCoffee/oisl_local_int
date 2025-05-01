import numpy as np

import numpy as np

# Function to read data and skip every other line for the velocity file
def load_velocity_data(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()
        # Skip every other line
        velocity_data = [list(map(float, line.split())) for i, line in enumerate(lines) if i % 2 == 0]
    return np.array(velocity_data)

# Function to read ISL data
def load_isl_data(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()
        isl_data = [list(map(float, line.split())) for line in lines]
    return np.array(isl_data)

# Load the velocity and ISL data
velocity_file = '/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/ECI_velocity.txt'
isl_file = '/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/ECI_forISL.txt'

velocity_data = load_velocity_data(velocity_file)
isl_data = load_isl_data(isl_file)

# Normalize the velocity vectors
velocity_vectors = velocity_data  # All columns are vector components
velocity_magnitudes = np.linalg.norm(velocity_vectors, axis=1, keepdims=True)
normalized_velocity_vectors = velocity_vectors / velocity_magnitudes

# Extract ISL vectors (already normalized)
isl_vectors = isl_data  # All columns are vector components

# Compute cosine similarities
similarities = [np.dot(normalized_velocity_vectors[i], isl_vectors[i]) for i in range(len(isl_vectors))]

# Define a threshold for similarity
threshold = 0.99  # Adjust if necessary
direction_similarity = np.array(similarities) > threshold

# Output the results
print("Cosine Similarities:", similarities)
print("Direction Similarity (True if similar):", direction_similarity)
