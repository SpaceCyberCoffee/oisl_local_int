import sys
from datetime import datetime, timedelta

try:
    import pymap3d as pm
except ModuleNotFoundError:
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pymap3d"])
    import pymap3d as pm

try:
    import numpy as np
except ModuleNotFoundError:
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "numpy"])
    import numpy as np

def ecef_to_eci(ecef_coords, timestamp):
    """
    Convert ECEF coordinates to ECI coordinates using pymap3d.

    :param ecef_coords: ECEF coordinates (x, y, z) in kilometers
    :param timestamp: The timestamp in UTC format
    :return: ECI coordinates (x, y, z) in kilometers
    """
    # Convert ECEF coordinates to ECI
    eci_coords = pm.ecef2eci(ecef_coords[0], ecef_coords[1], ecef_coords[2], timestamp)

    return eci_coords

if __name__ == "__main__":
    # Read command line arguments
    if len(sys.argv) != 5:
        print("Usage: python3 ECEF2ECI.py <ecef_x> <ecef_y> <ecef_z> <pd>")
        sys.exit(1)

    # ECEF coordinates from command line
    ecef_x = float(sys.argv[1])
    ecef_y = float(sys.argv[2])
    ecef_z = float(sys.argv[3])
    pd = float(sys.argv[4])  # Time offset in seconds

    # Prepare ECEF coordinates
    ecef_coords = np.array([ecef_x, ecef_y, ecef_z])

    # Specify the reference timestamp for the conversion
    timestamp = datetime(2025, 10, 18, 8, 30) + timedelta(seconds=pd)

    # Convert ECEF to ECI
    eci_coords = ecef_to_eci(ecef_coords, timestamp)

    # Print the ECI coordinates
    print(f"ECI propagated Position: [{eci_coords[0][0]}, {eci_coords[1][0]}, {eci_coords[2][0]}]")



