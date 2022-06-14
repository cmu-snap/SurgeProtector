# SurgeProtector
Artifacts for the "SurgeProtector" paper as it appears in SIGCOMM '22.

## Summary

**SurgeProtector** is a scheduling framework that mitigates the impact of Algorithmic Complexity Attacks (ACAs) -- a potent class of Denial-of-Service (DoS) attacks -- on Network Functions (NFs), using novel insights from adversarial scheduling theory. SurgeProtector interposes a *scheduler* between the vulnerable component and its ingress link, which in turn decides the order in which packets are served by the NF. The key ingredient for ACA mitigation is the scheduling policy underlying the SurgeProtector scheduler: **Packet-Size Weighted Shortest-Job First (WSJF)**. While details are elided for the sake of brevity (please refer to the full research paper), WSJF imposes a theoretical upper-bound on the "harm" an adversary can induce via ACAs. In particular, given perfect job size estimates, it guarantees that *to displace 1 bit-per-second (bps) of innocent traffic, the adversary must inject at least 1 bps of their own bandwidth into the attack*, significantly reducing the potency of ACAs. Further, this bound is independent of the load on the server, the packet and job size distributions for innocent traffic, and the underlying application itself. Thus, SurgeProtector represents a general ACA mitigation strategy that is not tied to any specific NF.

## Contents
This repository contains two artifacts:
1. An **adversarial scheduling simulator**. The event-driven simulator, written in C++, is capable of modeling G/G/1/k queueing systems, supports both trace-driven and synthetic workloads, and exposes a convenient interface for plugging in a wide range of simulated or emulated application backends. We provide implementations of several common distributions (to model packet sizes, job sizes, and inter-arrival times; *e.g.*, constant, exponential, normal, uniform), several scheduling policies (FCFS, FQ, SJF, and WSJF), and example applications (TCP reassembly, and one that samples job sizes i.i.d. from a user-specified distribution). The simulator source also contains scripts to compute the *optimal attack strategy* against a target scheduling policy for a given innocent workload.

2. Software (C++) and hardware (Verilog HDL) implementations of the **Hierarchical Find-First-Set (hFFS) Queue**, the heap design that makes SurgeProtector's WSJF scheduler robust against DoS attacks on the queue itself. The hardware implementation is fully pipelined, and is capable of processing one operation (*INSERT*, *EXTRACT-MIN*, or *EXTRACT-MAX*) every FPGA cycle (~4-5 ns). The software implementation uses builtin BSF/BSR (bit-scan {forward,reverse}) instructions to find the min/max heap element in worst-case constant time.

   * To evaluate this, we provide a DPDK-based benchmarking application that measures the performance of different queue implementations (*e.g.*, FCFS, a drop-tail Fibonacci heap, the hFFS queue) in the presence of adversarial attacks targeting the queue itself.

The [**Pigasus IDS**](https://github.com/crossroadsfpga/pigasus) is an archetypal example of a real-world application that can benefit from this framework, and we are currently integrating the SurgeProtector scheduler into the Pigasus FPGA datapath. The resulting design is fully operational, and will be upstreamed after end-to-end testing. To track the upstreaming effort, please see [this changelist](https://github.com/crossroadsfpga/pigasus/pull/16).

## Minimum Requirements
This code is known to be compatible with `Ubuntu 20.04 LTS`, `GCC 9.4+`, `CMake 3.10+`, `Python 3.8+`, `Boost C++ v1.78`, `DPDK 20.11.3`, and `Libconfig 1.7.3+`. Note that running DPDK applications requires a compatible NIC and Linux kernel version; the simulator does not use DPDK, and can be run without any special hardware.

## Installation (Linux)
Install g++-9, CMake and make sure their dependencies are met:  
```
sudo apt-get update
sudo apt-get install g++-[9+]
sudo apt-get install cmake
```

### 1. Installing Boost
Download Boost C++ (v1.78.0+) [here](https://www.boost.org/users/download/), and extract the tarball to a convenient location. In the top-level directory (*e.g.* `~/libs/boost_1_78_0/`), run the bootstrap and installation scripts as follows (`program_options` is the only separately-compiled library we use):
```
./bootstrap.sh --with-libraries=program_options
sudo ./b2 -j12 toolset=gcc variant=release threading=multi address-model=64 cxxflags=-std=c++17 install
```
Ensure that `program_options` is installed and visible to CMake.

### 2. Installing DPDK
Download DPDK (v20.11+) [here](https://core.dpdk.org/download/), and follow their build guide [here](https://doc.dpdk.org/guides/linux_gsg/build_dpdk.html). To minimize frustration, please check the DPDK Version/NIC Firmware compatibility matrix for your NIC before downloading (i.e., [this](https://doc.dpdk.org/guides/nics/ice.html) one for Intel's E810 NIC).

### 3. Installing Libconfig
The simulator uses Libconfig to read user-specified application and traffic configurations. Download Libconfig (v1.7.3+) [here](https://hyperrealm.github.io/libconfig/), and extract the tarball to a convenient location. In the top-level directory (*e.g.* `~/libs/libconfig-1.7.3/`), run:
```
sh ./configure
make
make check
make install
```

Our Python scripts also require libconfig; to install the relevant Python library, run:
```
pip3 install libconfig
```
### Compiling this code
Clone this repository to a convenient location. In the top-level directory for this project, run the following snippet:
```
mkdir build && cd build
cmake ..
make
```
If compilation was successful, `build/bin/` should contain executables for the simulator (`simulator`), as well as the scheduler benchmarking application (`sched_benchmark_server` and `sched_benchmark_pktgen`).

#### Compiling this code without DPDK
To compile the scheduling simulator without installing DPDK, edit `CMakeLists.txt` in the top-level directory, and comment out the following line: `add_subdirectory(scheduler)`. Now follow the steps in [**Compiling this code**](#compiling-this-code) as usual.

## Using the Simulator
To view the available command-line options, from the run `build` directory, run `./bin/simulator --help`. The simulator takes several arguments, the most important being `config`; this argument points to a ".cfg" file containing a user-specified simulation configuration. It specifies several parameters: the scheduling policy to use, the application to simulate, the innocent traffic workload, and the adversarial traffic workload. For instance, please see `simulator/configs/examples/example_1.cfg`. To simulate this configuration, run:
```
./bin/simulator --config=../simulator/configs/examples/example_1.cfg
```

If simulation was successful, the input configuration and key performance results are displayed on the console, including the *steady-state goodput* and *displacement factor* (for definitions, please refer to the paper). While this is a reasonable starting point, this flow has two limitations: 1) Modifying configuration files is cumbersome, so it's difficult to simulate the effect of different parameters (*e.g.*, policies, input rates, etc.), and 2) While we are simulating *an* adversary, it's not clear what attack strategy (*e.g.*, packet and job sizes) an "optimal" adversary would use.

To address these limitations, we provide an alternate flow: template configurations and a job-generation script. To view the available options for the alternate flow, `cd` into the `simulator/scripts` directory and run `python3 generate_jobs.py --help`. The script requires three arguments: the path to the simulator binary, the path to a "template" configuration file, and a path to a directory to store the results. Additionally, it allows the user to specify a list of policies, innocent input rates, and attack rates to simulate. Given these parameters, the script does the following: performs a "dry-run" to determine the innocent packet and job size distributions for the given template configuration, computes the "optimal" attack strategy for each parameter setting (the relevant code can be found in `simulator/scripts/adversary/`), generates the appropriate configuration files, and outputs an executable shell script to run the simulations.

As an example, from the `simulator/scripts` directory, run:
```
python3 generate_jobs.py \
    "${SURGEPROTECTOR_ROOT_DIR}/build/bin/simulator" \
    "${SURGEPROTECTOR_ROOT_DIR}/simulator/configs/templates/iid_job_sizes.cfg" \
    "${SURGEPROTECTOR_ROOT_DIR}/simulator/data/iid_job_sizes" \
    --policies=fcfs,sjf,wsjf \
    --innocent_rates=100M,500M,900M \
    --attack_rates=10M,30M,70M,100M,300M,700M
```

where ${SURGEPROTECTOR_ROOT_DIR} corresponds to the *absolute path* of the top-level directory for this project. If successful, this should create a directory called `simulator/data/iid_job_sizes`. Next, `cd` into it. `ls` should show the original template configuration specified (`template.cfg`), a packet log resulting from the dry-run (`log.packets`), a directory containing the relevant configuration files (`configs/`), and an executable script (`jobs.sh`) containing commands to run the required set of simulations (one per line). To run these, simply do:
```
./jobs.sh # Can also use parallel < jobs.sh if you have enough cores/memory
```

Once simulations complete, the `outputs` directory should contain a collection of the result logs (simply tee'd from stdout during execution). To plot the results, navigate back to `simulator/scripts`, and run `python3 plot_results.py ${SURGEPROTECTOR_ROOT_DIR}/simulator/data/iid_job_sizes`. The format is as follows: each column of the graph represents a different *innocent input rate* (100Mbps on the left, 500Mbps in the middle, and 900Mbps on the right), and on the X-axis we sweep different *attack input rates* (going from 10Mbps to 700Mbps); finally, we plot the *Displacement Factor (DF)* and *goodput* for each policy in the top and bottom rows, respectively. Observe that WSJF (the policy underlying SurgeProtector) achieves a maximum DF of 1.

The graphs in the paper can be reproduced by repeating this process with the template configuration files for TCP Reassembly (`simulator/configs/templates/tcp_reassembly.cfg`) and the Pigasus Full Matcher (`simulator/configs/templates/full_matcher.cfg`). Please note the the `trace_fp` parameter must be set correctly in both cases! For TCP Reassembly, we use traces from the 2014 CAIDA dataset (must be acquired from their [website](https://www.caida.org/catalog/datasets/passive_dataset_download/)). A portion of the Full Matching trace can be found in `simulator/traces/full_matching.csv`.

### Extending the Simulator
The simulator was designed with extensibility in mind, and we hope it enables developers to quickly implement and evaluate their own applications, distributions, scheduling policies, and traffic generators. We abstract out the basic interface for each of these components into base classes (*e.g.*, `common/distributions/distribution.h` and `simulator/src/queueing/base_queue.h`) that can be inherited from. For a simple example, please refer to the class definition for `FCFSQueue` in `simulator/src/queueing/fcfs_queue.{h,cpp}`.

## Running the Scheduler Benchmark

As described earlier, this repository also contains a DPDK-based application to benchmark the performance of the software hFFS Queue used in SurgeProtector. To run this application, you will need two (2) DPDK-compatible 10Gbps+ NICs set up on two different machines, connected back-to-back. One of these will serve as the *packet generator* (`PKTGEN`) for both innocent and attack traffic, while the other will serve as the *device-under-test* (`DUT`). The experiment works as follows. For each of the several heap designs under considerideration, we will pin a process running a software implementation of the heap to a single core on the `DUT`, where it will consume packets from the Ethernet link via DPDK. The packets (encoding the job size in us) are dispatched to a different core, which emulates ‘running’ the job by sleeping for a period of time corresponding to the job size. A third core is responsible for profiling the application goodput.

On the `PKTGEN` machine, from the `build` directory, run:
```
sudo ./bin/sched_benchmark_pktgen -l 15,16,17 -n 4 -- --rate-innocent=10 --rate-attack=0.1
```
to send 1Gbps of innocent traffic and 100Mbps of attack traffic.

Similarly, on the `DUT` machine, from the `build` directory, run:
```
sudo ./bin/sched_benchmark_server -l 0,1,2 -n 4 -- --policy=X
```
where X can be either "fcfs" (FCFS), "wsjf_drop_tail" (Fibonacci heap), "wsjf_drop_max" (Double-Ended Priority Queue), or "wsjf_hffs" (Hierarchical Find-First Set Queue). If successful, the profiling core on the `DUT` should display the instantaneous goodput every second. The `scheduler/scripts/plot_results.py` script can be used to plot the experiment results.

Notes:
* Both the `PKTGEN` and `DUT` currently support a single NIC port; please ensure that *only one* NIC interface is bound to DPDK
* As usual, please ensure that the lcores used with DPDK (*i.e.*, `-l` argument) are on the same NUMA node as the NIC (for consistent results, you may also isolate these cores from the kernel)
* Depending on how DPDK was built, you may need to manually link against certain DPDK libraries (for instance, `-d /usr/local/lib/x86_64-linux-gnu/librte_mempool_ring.so` may be a required argument)
