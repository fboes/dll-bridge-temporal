# aerofly_memory_scanner.py
import mmap
import struct
import time

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

def main():
    print("=== AEROFLY MEMORY SCANNER ===")
    print("Este programa escanea la memoria compartida para encontrar offsets correctos")
    print()
    
    try:
        # Conectar a memoria compartida
        shared_memory = mmap.mmap(-1, 3384, "AeroflyBridgeData")
        print("✅ Conectado a memoria compartida")
        
        # Verificar que los datos sean válidos
        data_valid = read_uint32(shared_memory, 8)
        if data_valid != 1:
            print("❌ Datos no válidos - asegúrate de que Aerofly esté corriendo")
            return
        
        print("✅ Datos válidos detectados")
        print()
        
        print("=== ESCANEANDO ESTRUCTURA FIJA ===")
        # Escanear estructura fija (primeros 672 bytes)
        active_offsets = []
        
        for offset in range(0, 672, 8):  # Cada 8 bytes (double)
            value = read_double(shared_memory, offset)
            if abs(value) > 1e-10:  # No es cero
                active_offsets.append((offset, value))
        
        print(f"Offsets activos en estructura fija:")
        for offset, value in active_offsets:
            print(f"  Offset {offset:3d}: {value:12.6f}")
        
        print()
        print("=== ESCANEANDO ARRAY DE 339 VARIABLES ===")
        
        # Escanear array de variables (672 + index*8)
        base_offset = 672
        active_indices = []
        
        for i in range(339):
            offset = base_offset + i * 8
            value = read_double(shared_memory, offset)
            if abs(value) > 1e-10:  # No es cero
                active_indices.append((i, value))
        
        print(f"Índices activos en array de 339 variables:")
        for index, value in active_indices:
            offset = base_offset + index * 8
            print(f"  Index {index:3d} (offset {offset:4d}): {value:12.6f}")
        
        print()
        print("=== RESUMEN ===")
        print(f"Variables activas en estructura fija: {len(active_offsets)}")
        print(f"Variables activas en array: {len(active_indices)}")
        print(f"Total de variables con datos: {len(active_offsets) + len(active_indices)}")
        
        shared_memory.close()
        
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()