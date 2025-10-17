import serial
from robot.api import logger

# Globaali muuttuja portille
ser = None

def open_port(port_name, baudrate=115200, timeout=2):
    """Avaa sarjaportin"""
    global ser
    try:
        ser = serial.Serial(port=port_name, baudrate=int(baudrate), timeout=float(timeout))
        logger.console(f"Sarjaportti {port_name} avattu onnistuneesti.")
    except Exception as e:
        logger.console(f"Virhe avattaessa porttia {port_name}: {e}")
        raise AssertionError("Porttia ei voitu avata!")

def close_port():
    """Sulkee sarjaportin"""
    global ser
    if ser and ser.is_open:
        ser.close()
        logger.console(f"Sarjaportti {ser.name} suljettu.")
    else:
        logger.console("Portti ei ollut auki.")

def write_data(data):
    """Kirjoittaa dataa sarjaporttiin"""
    global ser
    if not ser or not ser.is_open:
        raise AssertionError("Portti ei ole auki!")
    ser.write(data.encode())
    logger.console(f"Lähetetty: {data.strip()}")

def read_until(terminator='\n'):
    """Lukee sarjaportista kunnes tietty merkki saadaan"""
    global ser
    if not ser or not ser.is_open:
        raise AssertionError("Portti ei ole auki!")
    response = ser.read_until(terminator.encode()).decode(errors='ignore').strip()
    logger.console(f"Vastaanotettu raakadata: {response}")
    return response
