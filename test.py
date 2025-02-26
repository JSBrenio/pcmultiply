import subprocess

def run_program(program_path, args):
    """
    Simulates C's fork, execvp, and wait using Python's subprocess module.

    Args:
        program_path (str): The path to the executable program.
        args (list): A list of strings representing the arguments to pass to the program.

    Returns:
        tuple: A tuple containing the return code, standard output, and standard error.
    """
    try:
        # Simulate fork and execvp using subprocess.run
        result = subprocess.run(
            [program_path] + args,  # Combine program path and arguments
            capture_output=True,  # Capture stdout and stderr
            text=True,  # Decode stdout and stderr as text
            check=True  # Raise an exception for non-zero return codes
        )

        # Simulate wait by waiting for subprocess.run to complete

        return result.returncode, result.stdout, result.stderr

    except subprocess.CalledProcessError as e:
        # Handle non-zero return codes
        return e.returncode, e.stdout, e.stderr

    except FileNotFoundError:
        # Handle case where the executable is not found
        return -1, "", "Executable not found"

if __name__ == '__main__':
    program_path = "./pcMatrix"  # Replace with the actual path to your executable

    # Run with default arguments
    for i in range(1):
        print(f"DEFAULT TEST #{i}")
        return_code, stdout, stderr = run_program(program_path, [])
        if return_code == 0: print(f"Stdout:\n{stdout}")
        elif return_code == -1: 
            print(f"Stderr:\n{stderr}")
            break

    # un with custom arguments
    # Numwork, Buffer, Loops, matrix mode
    
    for i in range(100):
        arguments = [str(i), str(200 - i * 2), "1000", "0"]
        if arguments[1] == "0": arguments[1] = "1"
        print(f"NUMWORK = {arguments[0]} BUFFER SIZE = {arguments[1]} LOOPS = {arguments[2]} MODE = {arguments[3]} TEST #{i}")
        return_code, stdout, stderr = run_program(program_path, arguments)
        if return_code == 0: print(f"Stdout:\n{stdout}")
        elif return_code == -1: 
            print(f"Stderr:\n{stderr}")
            break