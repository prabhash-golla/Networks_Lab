import random
import sys

def generate_arithmetic_tasks(num_tasks=5, output_file=None):
    operations = ['+', '-', '*', '/']
    
    # Open file if specified, otherwise use stdout
    f = open(output_file, 'w') if output_file else sys.stdout
    
    for _ in range(num_tasks):
        # Generate random numbers
        num1 = random.randint(1, 100)
        
        # For division, ensure no division by zero
        op = random.choice(operations)
        if op == '/':
            k = random.randint(1, 20)  # Smaller numbers for 
            num2 = num1 // k
            if num2 == 0 :
                num2 = 1 
            num1 = num2 * k
            op = '/'
        else:
            num2 = random.randint(1, 100)
        
        # Write task to file or stdout
        f.write(f"{num1} {op} {num2}\n")
    
    # Close file if we opened one
    if output_file:
        f.close()
        print(f"Generated {num_tasks} tasks in {output_file}")

if __name__ == "__main__":
    # Parse command line arguments
    import argparse
    parser = argparse.ArgumentParser(description='Generate arithmetic tasks')
    parser.add_argument('-n', '--number', type=int, default=5,
                        help='Number of tasks to generate')
    parser.add_argument('-o', '--output', type=str, default=None,
                        help='Output file name')
    args = parser.parse_args()
    
    generate_arithmetic_tasks(args.number, args.output)