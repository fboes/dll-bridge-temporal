# aerofly_exhaustive_monitor_7columns_EN.py
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

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def main():
    print("=== AEROFLY EXHAUSTIVE VARIABLE MONITOR - ALL 339 VARIABLES (7 COLUMNS) ===")
    print("Displaying ALL variables, including those at zero")
    print("Press Ctrl+C to exit")
    print()
    
    try:
        shared_memory = mmap.mmap(-1, 3384, "AeroflyBridgeData")
        
        while True:
            clear_screen()
            print("=== AEROFLY EXHAUSTIVE VARIABLE MONITOR - ALL 339 VARIABLES (7 COLUMNS) ===")
            print(f"Timestamp: {time.strftime('%H:%M:%S')}")
            
            # Header info
            data_valid = read_uint32(shared_memory, 8)
            update_counter = read_uint32(shared_memory, 12)
            
            print(f"Data Valid: {'✅ YES' if data_valid == 1 else '❌ NO'} | Update Counter: {update_counter}")
            print()
            
            if data_valid != 1:
                print("❌ Waiting for valid data...")
                time.sleep(1)
                continue
            
            # Read ALL 339 variables from array
            base_offset = 672
            active_count = 0
            zero_count = 0
            
            # Header for 7 columns
            header_parts = []
            for i in range(7):
                header_parts.append(f"{'Idx':>3}|{'Offs':>4}|{'Value':>9}")
            
            print(" | ".join(header_parts))
            print("-" * (len(" | ".join(header_parts))))
            
            # Display in rows of 7 columns
            for row in range(0, 339, 7):
                line_parts = []
                
                for col in range(7):
                    index = row + col
                    if index < 339:
                        offset = base_offset + index * 8
                        value = read_double(shared_memory, offset)
                        
                        if abs(value) > 1e-10:
                            active_count += 1
                            if abs(value) > 1000000:  # Large frequencies
                                formatted_value = f"{value:9.0f}"
                            elif abs(value) > 1000:   # Large values
                                formatted_value = f"{value:9.2f}"
                            elif abs(value) > 1:      # Medium values
                                formatted_value = f"{value:9.4f}"
                            else:                     # Small values
                                formatted_value = f"{value:9.6f}"
                        else:
                            zero_count += 1
                            formatted_value = " 0.000000"
                        
                        # Build each part in correct order
                        part = f"{index:3d}|{offset:4d}|{formatted_value}"
                        line_parts.append(part)
                    else:
                        # Empty column
                        line_parts.append(f"{'':3s}|{'':4s}|{'':9s}")
                
                # Join parts correctly
                print(" | ".join(line_parts))
            
            print()
            print(f"📊 STATISTICS:")
            print(f"   Active variables (≠0): {active_count}")
            print(f"   Zero variables:        {zero_count}")
            print(f"   Total variables:       {active_count + zero_count}/339")
            print(f"   Active percentage:     {(active_count/339)*100:.1f}%")
            print(f"   Rows displayed:        {(339 + 6) // 7} rows")
            print()
            
            # Most important variables
            print(f"🔍 TOP 21 ACTIVE VARIABLES (sorted by absolute value):")
            interesting_vars = []
            for i in range(339):
                offset = base_offset + i * 8
                value = read_double(shared_memory, offset)
                if abs(value) > 1e-10:
                    interesting_vars.append((i, value))
            
            # Sort by absolute value (largest first)
            interesting_vars.sort(key=lambda x: abs(x[1]), reverse=True)
            
            # Display in 7 columns to use space efficiently
            for i, (index, value) in enumerate(interesting_vars[:21]):
                if abs(value) > 1000000:
                    formatted = f"{value:10.0f}"
                elif abs(value) > 1000:
                    formatted = f"{value:10.2f}"
                else:
                    formatted = f"{value:10.5f}"
                
                print(f"[{index:03d}]:{formatted}", end="  ")
                if (i + 1) % 7 == 0:
                    print()
            
            if len(interesting_vars) % 7 != 0:
                print()
            
            print()
            print(f"💡 VARIABLE RANGES:")
            print(f"   Rows 0-6:    Variables [000-048] - Basic Aircraft")
            print(f"   Rows 7-13:   Variables [049-097] - Performance & Config")
            print(f"   Rows 14-20:  Variables [098-146] - Navigation & Comm")
            print(f"   Rows 21-27:  Variables [147-195] - Autopilot & Controls")
            print(f"   Rows 28-34:  Variables [196-244] - Advanced Controls")
            print(f"   Rows 35-41:  Variables [245-293] - Systems & Warnings")
            print(f"   Rows 42-48:  Variables [294-338] - View & Simulation")
            
            time.sleep(1.0)
            
    except KeyboardInterrupt:
        print("\n\n✅ Monitor terminated")
    except Exception as e:
        print(f"\n❌ Error: {e}")
    finally:
        if 'shared_memory' in locals():
            shared_memory.close()

if __name__ == "__main__":
    main()