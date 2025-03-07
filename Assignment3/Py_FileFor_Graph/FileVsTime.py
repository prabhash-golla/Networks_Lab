import matplotlib.pyplot as plt

# File sizes in KB
file_sizes_kb = [13 / 1024, 2, 438.9, 1024]  # Converting 13 bytes to KB
times = [0.081960, 0.082225, 0.093528, 0.110849]  # Updated transfer times in seconds

plt.figure(figsize=(8, 5))
plt.plot(file_sizes_kb, times, marker='o', linestyle='-', color='r', label="Transfer Time")

# Labels and title
plt.xlabel("File Size (KB)")
plt.ylabel("Time Taken (seconds)")
plt.title("File Size vs. Transfer Time")
plt.grid(True)
plt.legend()
plt.show()
