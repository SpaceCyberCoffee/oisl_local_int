from astropy import units as u
from datetime import datetime
from calendar import isleap


def propagate_orbit(lines, seconds_elapsed):
    """
    Propagate the orbit using TLE data.
    :param tle_file: Path to the TLE file.
    :param seconds_elapsed: Seconds elasped since simstart.
    :return: ECEF position vector.
    """
    from datetime import datetime, timedelta
    from sgp4.api import Satrec, jday

    s, t = lines[1], lines[2]

    satellite = Satrec.twoline2rv(s, t)
    start_time = datetime(2025, 10, 18, 8, 30, 0)
    current_time = start_time + timedelta(seconds=seconds_elapsed)
    # Get the Julian date and fraction
    jd, fr = jday(current_time.year, current_time.month, current_time.day,
                  current_time.hour, current_time.minute, current_time.second + current_time.microsecond / 1e6)
    e, r, v = satellite.sgp4(jd, fr)

    return r


def date_to_fraction_of_year(year, month, day, hour=0, minute=0, second=0):
    # Create a datetime object for the given date and time
    dt = datetime(year, month, day, hour, minute, second)

    # Create a datetime object for the start of the year
    start_of_year = datetime(year, 1, 1)

    # Determine the number of days in the year
    days_in_year = 366 if isleap(year) else 365

    # Calculate the day of the year
    day_of_year = (dt - start_of_year).days + 1  # +1 because January 1 is day 1

    # Calculate the fraction of the day
    fraction_of_day = (dt.hour * 3600 + dt.minute * 60 + dt.second) / 86400  # 86400 seconds in a day

    # Calculate the fraction of the year
    fraction_of_year = day_of_year + fraction_of_day

    return fraction_of_year


# Example usage
year = 2025
month = 10
day = 18
hour = 8
minute = 30
second = 0

fraction_of_year = date_to_fraction_of_year(year, month, day, hour, minute, second)
print(f"Fraction of the year: {fraction_of_year:.6f}")

l1 = '1 25544U 98067A   25291.35416700  .00022424  00000+0  39310-3 0  9991'
l2 = '2 25544  52.0000 180.0000 0009824 0.008873 0.003093 15.50434953465062'

lines = ['DIOCANE', l1, l2]
r1 = propagate_orbit(lines, 173)
print(r1)

# Given orbital parameters
perigee_alt = 400.0 * u.km  # Perigee altitude
apogee_alt = 400.0 * u.km   # Apogee altitude
inclination = 52.0 * u.deg  # Inclination
raan = 180.0 * u.deg        # Right Ascension of Ascending Node
arg_periapsis = 0.0 * u.deg # Argument of Periapsis
true_anomaly = 0.0 * u.deg # True Anomaly

# Calculate semi-major axis for a circular orbit
earth_radius = 6378.137 * u.km
semi_major_axis = earth_radius + (perigee_alt + apogee_alt) / 2

# Eccentricity for a circular orbit
eccentricity = 0.0 * u.one

