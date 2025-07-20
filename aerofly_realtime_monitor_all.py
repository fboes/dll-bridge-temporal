# aerofly_complete_monitor.py
import mmap
import struct
import time
import os

def read_double(shared_memory, offset):
    try:
        shared_memory.seek(offset)
        data = shared_memory.read(8)
        return struct.unpack('d', data)[0]
    except:
        return 0.0

def read_uint32(shared_memory, offset):
    try:
        shared_memory.seek(offset)
        data = shared_memory.read(4)
        return struct.unpack('I', data)[0]
    except:
        return 0

def read_uint64(shared_memory, offset):
    try:
        shared_memory.seek(offset)
        data = shared_memory.read(8)
        return struct.unpack('Q', data)[0]
    except:
        return 0

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def format_value(name, value):
    """Format values based on variable type"""
    if any(word in name for word in ['Latitude', 'Longitude']):
        return f"{value * 57.2958:8.5f}°"
    elif any(word in name for word in ['Pitch', 'Bank', 'Heading', 'Course']):
        return f"{value * 57.2958:8.2f}°"
    elif 'Altitude' in name or 'Height' in name:
        return f"{value:8.1f} m"
    elif any(word in name for word in ['Speed', 'IAS', 'VS']):
        return f"{value:8.1f} m/s"
    elif 'Frequency' in name or 'COM' in name or 'NAV' in name:
        if value > 1000000:
            return f"{value/1000000:8.3f} MHz"
        else:
            return f"{value:8.0f} Hz"
    elif 'RPM' in name or 'RotationSpeed' in name:
        return f"{value:8.0f} RPM"
    elif any(word in name for word in ['Running', 'Engaged', 'OnGround', 'OnRunway', 'Crashed']):
        return "   YES" if value > 0.5 else "    NO"
    elif 'Mach' in name:
        return f"{value:8.3f}"
    elif 'Code' in name:
        return f"{value:8.0f}"
    else:
        return f"{value:8.3f}"

def main():
    print("=== AEROFLY COMPLETE VARIABLE MONITOR ===")
    print("Monitor completo de todas las variables activas")
    print("Presiona Ctrl+C para salir")
    print()
    
    # TODAS LAS VARIABLES CONFIRMADAS (basadas en el scanner)
    all_variables = {
        # === ESTRUCTURA FIJA ===
        'Aircraft.Latitude': 16,
        'Aircraft.Longitude': 24,
        'Aircraft.Altitude': 32,
        'Aircraft.Pitch': 40,
        'Aircraft.Bank': 48,
        'Aircraft.TrueHeading': 56,
        'Aircraft.MagneticHeading': 64,
        'Aircraft.IndicatedAirspeed': 72,
        'Aircraft.GroundSpeed': 80,
        'Aircraft.VerticalSpeed': 88,
        'Aircraft.AngleOfAttack': 96,
        'Aircraft.MachNumber': 112,
        'Aircraft.RateOfTurn': 120,
        'Aircraft.Position.X': 128,
        'Aircraft.Position.Y': 136,
        'Aircraft.Position.Z': 144,
        'Aircraft.Velocity.X': 152,
        'Aircraft.Velocity.Y': 160,
        'Aircraft.Velocity.Z': 168,
        'Aircraft.Acceleration.X': 176,
        'Aircraft.Acceleration.Y': 184,
        'Aircraft.Acceleration.Z': 192,
        'Aircraft.AngularVel.X': 200,
        'Aircraft.AngularVel.Y': 208,
        'Aircraft.AngularVel.Z': 216,
        'Aircraft.Wind.X': 224,
        'Aircraft.Wind.Y': 232,
        'Aircraft.Wind.Z': 240,
        'Aircraft.Gravity.X': 248,
        'Aircraft.Gravity.Y': 256,
        'Aircraft.Gravity.Z': 264,
        'Aircraft.OnGround': 272,
        'Aircraft.OnRunway': 280,
        'Aircraft.Flaps': 304,
        'Aircraft.Throttle': 320,
        'Aircraft.EngineThrottle1': 336,
        'Aircraft.EngineThrottle2': 344,
        'Aircraft.EngineRotationSpeed1': 368,
        'Aircraft.EngineRotationSpeed2': 376,
        'Aircraft.EngineRunning1': 400,
        'Aircraft.EngineRunning2': 408,
        'Controls.Pitch.Input': 432,
        'Controls.Roll.Input': 440,
        'Communication.COM1Freq': 456,
        'Communication.COM1Standby': 464,
        'Communication.COM2Freq': 472,
        'Communication.COM2Standby': 480,
        'Navigation.NAV1Freq': 488,
        'Navigation.NAV1Standby': 496,
        'Navigation.NAV1Course': 504,
        'Navigation.NAV2Freq': 512,
        'Navigation.NAV2Standby': 520,
        'Navigation.NAV2Course': 528,
        'Autopilot.Engaged': 536,
        'Autopilot.SelectedHeading': 552,
        'Autopilot.SelectedAltitude': 560,
        'Autopilot.SelectedVS': 568,
        'Performance.VS0': 616,
        'Performance.VS1': 624,
        'Performance.VFE': 632,
        'Performance.VNO': 640,
        'Performance.VNE': 648,
        
        # === VARIABLES DEL ARRAY (672 + index * 8) ===
        'Array[1].Altitude': 672 + (1 * 8),
        'Array[2].VerticalSpeed': 672 + (2 * 8),
        'Array[3].Pitch': 672 + (3 * 8),
        'Array[4].Bank': 672 + (4 * 8),
        'Array[5].IAS': 672 + (5 * 8),
        'Array[6].IASTrend': 672 + (6 * 8),
        'Array[7].GroundSpeed': 672 + (7 * 8),
        'Array[8].MagHeading': 672 + (8 * 8),
        'Array[9].TrueHeading': 672 + (9 * 8),
        'Array[10].Latitude': 672 + (10 * 8),
        'Array[11].Longitude': 672 + (11 * 8),
        'Array[12].Height': 672 + (12 * 8),
        'Array[20].RateOfTurn': 672 + (20 * 8),
        'Array[21].MachNumber': 672 + (21 * 8),
        'Array[22].AngleOfAttack': 672 + (22 * 8),
        'Array[26].Flaps': 672 + (26 * 8),
        'Array[28].Throttle': 672 + (28 * 8),
        'Array[52].OnGround': 672 + (52 * 8),
        'Array[53].OnRunway': 672 + (53 * 8),
        'Array[78].EngineMaster1': 672 + (78 * 8),
        'Array[79].EngineMaster2': 672 + (79 * 8),
        'Array[82].EngineThrottle1': 672 + (82 * 8),
        'Array[83].EngineThrottle2': 672 + (83 * 8),
        'Array[86].EngineRPM1': 672 + (86 * 8),
        'Array[87].EngineRPM2': 672 + (87 * 8),
        'Array[90].EngineRunning1': 672 + (90 * 8),
        'Array[91].EngineRunning2': 672 + (91 * 8),
        'Array[95].VS0': 672 + (95 * 8),
        'Array[96].VS1': 672 + (96 * 8),
        'Array[97].VFE': 672 + (97 * 8),
        'Array[98].VNO': 672 + (98 * 8),
        'Array[99].VNE': 672 + (99 * 8),
        'Array[108].SelectedCourse1': 672 + (108 * 8),
        'Array[109].SelectedCourse2': 672 + (109 * 8),
        'Array[111].NAV1Freq': 672 + (111 * 8),
        'Array[112].NAV1Standby': 672 + (112 * 8),
        'Array[115].NAV2Freq': 672 + (115 * 8),
        'Array[116].NAV2Standby': 672 + (116 * 8),
        'Array[142].COM1Freq': 672 + (142 * 8),
        'Array[143].COM1Standby': 672 + (143 * 8),
        'Array[145].COM2Freq': 672 + (145 * 8),
        'Array[146].COM2Standby': 672 + (146 * 8),
        'Array[159].AP.SelectedHeading': 672 + (159 * 8),
        'Array[160].AP.SelectedAltitude': 672 + (160 * 8),
        'Array[161].AP.SelectedVS': 672 + (161 * 8),
        'Array[172].AP.Engaged': 672 + (172 * 8),
        'Array[201].Controls.Pitch': 672 + (201 * 8),
        'Array[203].Controls.Roll': 672 + (203 * 8),
        'Array[207].Controls.Flaps': 672 + (207 * 8),
    }
    
    try:
        shared_memory = mmap.mmap(-1, 3384, "AeroflyBridgeData")
        
        while True:
            clear_screen()
            print("=== AEROFLY COMPLETE VARIABLE MONITOR ===")
            print(f"Timestamp: {time.strftime('%H:%M:%S')}")
            
            # Header info
            data_valid = read_uint32(shared_memory, 8)
            update_counter = read_uint32(shared_memory, 12)
            timestamp = read_uint64(shared_memory, 0)
            
            print(f"Data Valid: {'✅ YES' if data_valid == 1 else '❌ NO'} | Update Counter: {update_counter} | Timestamp: {timestamp}")
            print()
            
            if data_valid != 1:
                print("❌ Waiting for valid data...")
                time.sleep(1)
                continue
            
            # Display variables in 4 columns
            variables_list = list(all_variables.items())
            
            # Header for columns
            print(f"{'Variable':25s} | {'Value':12s} | {'Variable':25s} | {'Value':12s} | {'Variable':25s} | {'Value':12s} | {'Variable':25s} | {'Value':12s}")
            print("-" * 140)
            
            # Display in rows of 4 columns
            for i in range(0, len(variables_list), 4):
                row_data = []
                
                for j in range(4):
                    if i + j < len(variables_list):
                        name, offset = variables_list[i + j]
                        value = read_double(shared_memory, offset)
                        formatted_value = format_value(name, value)
                        row_data.extend([name[:24], formatted_value])
                    else:
                        row_data.extend(["", ""])
                
                print(f"{row_data[0]:25s} | {row_data[1]:12s} | {row_data[2]:25s} | {row_data[3]:12s} | {row_data[4]:25s} | {row_data[5]:12s} | {row_data[6]:25s} | {row_data[7]:12s}")
            
            print()
            print(f"Total variables monitored: {len(all_variables)}")
            print("Press Ctrl+C to exit")
            
            time.sleep(0.5)  # 2 Hz update rate
            
    except KeyboardInterrupt:
        print("\n\n✅ Monitor terminado")
    except Exception as e:
        print(f"\n❌ Error: {e}")
    finally:
        if 'shared_memory' in locals():
            shared_memory.close()

if __name__ == "__main__":
    main()