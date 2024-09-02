import subprocess
import pytest

# CppLox Path
LOX_PATH = "./build/CppLox"

# TEST_PROGRAM_LOCATION
LOX_HELLO_WORLD = "./test/src/hello.lox"

def run_program(file_name):
    result = subprocess.run([LOX_PATH, file_name], capture_output=True, text=True)
    return result.stdout
    

def test_hello_world():
    assert run_program(LOX_HELLO_WORLD) == "Hello World!\n"
    