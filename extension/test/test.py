import sqlite3
import os

# Use absolute path to your DLL
dll_path = os.path.abspath("extension.dll")

# Connect to database
conn = sqlite3.connect(":memory:")

try:
    # Enable extension loading
    conn.enable_load_extension(True)
    
    # Load your DLL
    conn.load_extension(dll_path)
    
    # Test your extension function
    # if you want to expose one
    cursor = conn.cursor()
    result = cursor.execute("SELECT rot13('test')").fetchone()
    print(f"Success: {result}")
    
except Exception as e:
    print(f"Error: {e}")
    
finally:
    conn.close()
