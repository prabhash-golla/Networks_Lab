n = int(input("Enter the number of lines : "))
with open('22CS30027.txt', 'w') as file:
    for i in range(1,n+1):
        file.write(f"Line Number {i}\n")
    file.close()