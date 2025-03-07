import matplotlib.pyplot as plt

# Data based on observation
file_sizes_kb = [13/1024,2, 438.9, 1024]  # in bytes
num_packets = [24,24, 73, 125]  

plt.figure(figsize=(8, 5))
plt.plot(file_sizes_kb, num_packets, marker='o', linestyle='-', color='b',label="Number of Packets")

# Labels and title
plt.xlabel("File Size (KB)")
plt.ylabel("Number of Packets")
plt.title("File Size vs. Number of Packets")
plt.grid(True)
plt.legend()
plt.show()
