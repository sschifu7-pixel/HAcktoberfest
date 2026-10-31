import sys
import time
import ctypes
from ctypes import wintypes

def parse_hex(hex_path):
    flash = bytearray([0xFF] * 32768)
    max_addr = 0
    with open(hex_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line.startswith(':'):
                continue
            length = int(line[1:3], 16)
            addr = int(line[3:7], 16)
            rectype = int(line[7:9], 16)
            data_hex = line[9:9+length*2]
            if rectype == 0:  # Data record
                data = bytes.fromhex(data_hex)
                flash[addr:addr+length] = data
                if addr + length > max_addr:
                    max_addr = addr + length
            elif rectype == 1:  # EOF
                break
    return flash, max_addr

def flash(port_name, hex_path):
    print(f"Cargando {hex_path} en {port_name}...")
    flash_mem, max_addr = parse_hex(hex_path)
    print(f"Tamaño de flash a escribir: {max_addr} bytes")

    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    h = kernel32.CreateFileW(f'\\\\.\\{port_name}', 0xC0000000, 0, None, 3, 0, None)
    if h == -1:
        raise RuntimeError(f"No se pudo abrir el puerto {port_name}. Error: {ctypes.get_last_error()}")

    class COMMTIMEOUTS(ctypes.Structure):
        _fields_ = [
            ('ReadIntervalTimeout', wintypes.DWORD),
            ('ReadTotalTimeoutMultiplier', wintypes.DWORD),
            ('ReadTotalTimeoutConstant', wintypes.DWORD),
            ('WriteTotalTimeoutMultiplier', wintypes.DWORD),
            ('WriteTotalTimeoutConstant', wintypes.DWORD),
        ]

    timeouts = COMMTIMEOUTS(50, 10, 1000, 10, 1000)
    kernel32.SetCommTimeouts(h, ctypes.byref(timeouts))

    def write_bytes(data):
        written = wintypes.DWORD()
        kernel32.WriteFile(h, data, len(data), ctypes.byref(written), None)
        return written.value

    def read_bytes(count):
        buf = (ctypes.c_char * count)()
        read_n = wintypes.DWORD()
        kernel32.ReadFile(h, buf, count, ctypes.byref(read_n), None)
        return bytes(buf[:read_n.value])

    def send_cmd(cmd, read_count=2):
        write_bytes(cmd)
        resp = read_bytes(read_count)
        return resp

    print("Reiniciando Arduino Nano (DTR)...")
    kernel32.EscapeCommFunction(h, 5) # SETDTR
    time.sleep(0.05)
    kernel32.EscapeCommFunction(h, 6) # CLRDTR
    time.sleep(0.1)
    kernel32.EscapeCommFunction(h, 5) # SETDTR
    time.sleep(0.25)

    PURGE_TXCLEAR = 0x0004
    PURGE_RXCLEAR = 0x0008
    kernel32.PurgeComm(h, PURGE_TXCLEAR | PURGE_RXCLEAR)

    # Sync
    synced = False
    for i in range(10):
        resp = send_cmd(b'\x30\x20', 2)
        if resp == b'\x14\x10':
            synced = True
            break
        time.sleep(0.05)

    if not synced:
        kernel32.CloseHandle(h)
        raise RuntimeError("No se pudo sincronizar con el bootloader STK500.")

    print("Bootloader sincronizado correctamente (STK500 v1).")

    # Enter progmode
    resp = send_cmd(b'\x50\x20', 2)
    if resp != b'\x14\x10':
        print(f"Aviso enter progmode: {resp.hex()}")

    # Page size for ATmega328P is 128 bytes
    page_size = 128
    num_pages = (max_addr + page_size - 1) // page_size

    for page in range(num_pages):
        byte_addr = page * page_size
        word_addr = byte_addr >> 1  # STK500 uses word address

        addr_low = word_addr & 0xFF
        addr_high = (word_addr >> 8) & 0xFF

        # STK_LOAD_ADDRESS (0x55)
        resp = send_cmd(bytes([0x55, addr_low, addr_high, 0x20]), 2)
        if resp != b'\x14\x10':
            kernel32.CloseHandle(h)
            raise RuntimeError(f"Error cargando dirección {byte_addr}: {resp.hex()}")

        # STK_PROG_PAGE (0x64), length high, length low, 'F', data, 0x20
        page_data = flash_mem[byte_addr:byte_addr + page_size]
        prog_cmd = bytes([0x64, 0x00, page_size, ord('F')]) + page_data + b'\x20'
        resp = send_cmd(prog_cmd, 2)
        if resp != b'\x14\x10':
            kernel32.CloseHandle(h)
            raise RuntimeError(f"Error programando página {page}: {resp.hex()}")

        progress = int((page + 1) / num_pages * 100)
        print(f"\rEscribiendo memoria flash: {progress}% ({byte_addr + page_size}/{num_pages * page_size} bytes)", end="", flush=True)

    print("\nEscritura completada exitosamente!")

    # Leave progmode
    send_cmd(b'\x51\x20', 2)
    kernel32.CloseHandle(h)
    print("¡Arduino Nano actualizado y ejecutando el nuevo programa!")

if __name__ == '__main__':
    port = sys.argv[1] if len(sys.argv) > 1 else 'COM10'
    hex_file = sys.argv[2] if len(sys.argv) > 2 else r'c:\Users\alfre\AppData\Local\arduino\sketches\E3302D34345EFA1346D2877B55A9A4D3\TEST_NANO.ino.hex'
    flash(port, hex_file)
