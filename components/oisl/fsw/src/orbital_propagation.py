import sys

try:
    import sgp4
except ModuleNotFoundError:
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "sgp4"])

def propagate_orbit(tle_file, seconds_elapsed):
    """
    Propagate the orbit using TLE data.
    :param tle_file: Path to the TLE file.
    :param seconds_elapsed: Seconds elasped since simstart.
    :return: ECI position vector.
    """
    from datetime import datetime, timedelta
    from sgp4.api import Satrec, jday
    
    with open(tle_file, 'r') as f:
        lines = f.readlines()
    s, t = lines[1], lines[2]

    satellite = Satrec.twoline2rv(s, t)
    start_time = datetime(2025, 10, 18, 8, 30, 0)
    current_time = start_time + timedelta(seconds=seconds_elapsed)
    # Get the Julian date and fraction
    jd, fr = jday(current_time.year, current_time.month, current_time.day, 
                  current_time.hour, current_time.minute, current_time.second + current_time.microsecond / 1e6)
    e, r, v = satellite.sgp4(jd, fr)

    return r 

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 orbital_propagation.py <tle_file> <pd>")
        sys.exit(1)

    tle_file_path = sys.argv[1]
    seconds_elapsed = float(sys.argv[2])
    

    eci_position = propagate_orbit(tle_file_path, seconds_elapsed)
    print(f"ECI propagated Position: [{eci_position[0]}, {eci_position[1]}, {eci_position[2]}]")