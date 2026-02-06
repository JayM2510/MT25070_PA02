# Roll No: MT25070

# System Configuration:
# OS: Ubuntu 24.04 (Dual Boot)
# Architecture: x86-64
# CPU: 12 Logical Cores
# RAM: 16 GB

import matplotlib.pyplot as plt


# Plot style 
plt.rcParams.update({
    "font.size": 12,
    "axes.titlesize": 14,
    "axes.labelsize": 13,
    "legend.fontsize": 11,
    "lines.linewidth": 2,
})

STYLE = {
    "two_copy": {
        "color": "#ffa600",
        "marker": "o",
        "linestyle": "-",
    },
    "one_copy": {
        "color": "#bc5090",
        "marker": "s",
        "linestyle": "--",
    },
    "zero_copy": {
        "color": "#003f5c",
        "marker": "^",
        "linestyle": "-.",
    },
}


# HARD-CODED DATA (from MT25070_Part_C_AllResults.csv)
message_sizes = [64, 256, 1024, 4096]
threads = [1, 2, 4, 8]

throughput = {
    "two_copy":  [5.546936, 17.572738, 55.995350, 115.771619],
    "one_copy":  [3.899806, 14.088616, 13.944205, 32.664450],
    "zero_copy": [3.000519, 10.333172, 35.184303, 89.944395],
}

latency = {
    "two_copy":  [0.40, 0.42, 0.56, 0.72],
    "one_copy":  [1.82, 1.90, 2.25, 2.65],
    "zero_copy": [0.62, 0.69, 0.91, 1.26],
}

cache_misses = {
    "two_copy":  [18258493, 30728335, 164344113, 1492154220],
    "one_copy":  [11545767, 75984937, 4278387, 9600764],
    "zero_copy": [7606435, 48170964, 317583736, 158054953],
}

cycles_per_byte = {
    "two_copy": [
        162238305756 / (64 * 4),
        161249490159 / (256 * 4),
        161148939038 / (1024 * 4),
        156020442234 / (4096 * 4),
    ],
    "one_copy": [
        160043913965 / (64 * 4),
        158895770861 / (256 * 4),
        135095231857 / (1024 * 4),
        133664820069 / (4096 * 4),
    ],
    "zero_copy": [
        156074096500 / (64 * 4),
        157551537201 / (256 * 4),
        154469829502 / (1024 * 4),
        139960448008 / (4096 * 4),
    ],
}


# 1. Throughput vs Message Size
plt.figure(figsize=(7, 5))
for impl in STYLE:
    plt.plot(
        message_sizes,
        throughput[impl],
        label=impl.replace("_", " ").title(),
        **STYLE[impl]
    )

plt.xscale("log", base=2)
plt.xlabel("Message Size (bytes)")
plt.ylabel("Throughput (Gbps)")
plt.title(
    "Throughput vs Message Size (Threads = 4)\n"
    "Ubuntu 24.04 | x86-64 | 12 Cores | 16GB RAM"
)
plt.grid(True, which="both", linestyle="--", alpha=0.6)
plt.legend()
plt.tight_layout()
plt.savefig("MT25070_PartD_throughput_vs_message_size.png")
plt.close()


# 2. Latency vs Thread Count
plt.figure(figsize=(7, 5))
for impl in STYLE:
    plt.plot(
        threads,
        latency[impl],
        label=impl.replace("_", " ").title(),
        **STYLE[impl]
    )

plt.xlabel("Thread Count")
plt.ylabel("Average Latency (µs)")
plt.title(
    "Latency vs Thread Count (Message Size = 1024 bytes)\n"
    "Ubuntu 24.04 | x86-64 | 12 Cores | 16GB RAM"
)
plt.grid(True, linestyle="--", alpha=0.6)
plt.legend()
plt.tight_layout()
plt.savefig("MT25070_PartD_latency_vs_thread_count.png")
plt.close()


# 3. Cache Misses vs Message Size
plt.figure(figsize=(7, 5))
for impl in STYLE:
    plt.plot(
        message_sizes,
        cache_misses[impl],
        label=impl.replace("_", " ").title(),
        **STYLE[impl]
    )

plt.xscale("log", base=2)
plt.xlabel("Message Size (bytes)")
plt.ylabel("Cache Misses")
plt.title(
    "Cache Misses vs Message Size (Threads = 4)\n"
    "Ubuntu 24.04 | x86-64 | 12 Cores | 16GB RAM"
)
plt.grid(True, which="both", linestyle="--", alpha=0.6)
plt.legend()
plt.tight_layout()
plt.savefig("MT25070_PartD_cache_misses_vs_message_size.png")
plt.close()



# 4. CPU Cycles per Byte
plt.figure(figsize=(7, 5))
for impl in STYLE:
    plt.plot(
        message_sizes,
        cycles_per_byte[impl],
        label=impl.replace("_", " ").title(),
        **STYLE[impl]
    )

plt.xscale("log", base=2)
plt.xlabel("Message Size (bytes)")
plt.ylabel("CPU Cycles per Byte")
plt.title(
    "CPU Cycles per Byte vs Message Size (Threads = 4)\n"
    "Ubuntu 24.04 | x86-64 | 12 Cores | 16GB RAM"
)
plt.grid(True, which="both", linestyle="--", alpha=0.6)
plt.legend()
plt.tight_layout()
plt.savefig("MT25070_PartD_cycles_per_byte_vs_message_size.png")
plt.close()


