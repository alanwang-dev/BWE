#!/usr/bin/env python3
import sys
import random
import csv
import os

def main():
    if len(sys.argv) < 2:
        duration = 300
    else:
        duration = int(sys.argv[1])
        
    out_file = "dummy_trace.csv"
    
    print(f"Generating dummy trace data for {duration} seconds...")
    
    with open(out_file, "w", newline="") as f:
        writer = csv.writer(f)
        # Adding actual_bps so that the fit runner knows what the real throughput was
        writer.writerow(["time_s", "rtt_ms", "jitter_ms", "buffer_ms", "actual_bps"])
        
        base_bw = 5_000_000 # 5 Mbps
        base_rtt = 40
        buffer = 1500
        
        for t in range(duration):
            # Add some randomness
            rtt = max(10, base_rtt + random.uniform(-5, 20))
            jitter = max(2, random.uniform(2, 10))
            bps = max(100_000, base_bw + random.uniform(-500_000, 500_000))
            
            # Simulate a network drop
            if 30 <= t <= 45:
                bps *= 0.2
                rtt += 100
                buffer = max(100, buffer - 500)
            else:
                buffer = min(3000, buffer + random.uniform(-100, 200))
                
            writer.writerow([t, round(rtt, 1), round(jitter, 1), round(buffer, 1), int(bps)])
            
    print(f"Wrote generated data to {out_file}")

if __name__ == "__main__":
    main()