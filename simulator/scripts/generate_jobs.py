from __future__ import annotations
import subprocess

import argparse
import io
import libconf
import os
import shutil
import stat

from common import RateConverter
from adversary import analyze_fcfs, analyze_fq,\
                      analyze_sjf, analyze_sjf_inorder,\
                      analyze_wsjf, analyze_wsjf_inorder

def generate_jobs() -> None:
    """Generates simulation jobs."""
    parser = argparse.ArgumentParser(description="Generates simulation jobs.")
    parser.add_argument(
        "simulator_path", type=str,
        help="Fully-qualified path to the simulator binary")

    parser.add_argument(
        "template_cfg", type=str,
        help="Fully-qualified path to a configuration file (*.cfg) to be used as a template"
    )

    parser.add_argument(
        "output_dir", type=str,
        help="Fully-qualified path to target output directory (existing contents will be rm'd)"
    )

    parser.add_argument(
        "--policies", type=str, default="",
        help="Comma-separated list of policies to simulate (e.g., 'fcfs,wsjf')"
    )

    parser.add_argument(
        "--innocent_rates", type=str, default="",
        help="Comma-separated list of innocent input rates to simulate (e.g., " +
             "'1000,1000000' or '1K,1M' will simulate rates of 1Kbps and 1Mbps)"
    )

    parser.add_argument(
        "--attack_rates", type=str, default="",
        help="Comma-separated list of attack input rates to simulate" +
             "(format is identical to that of --innocent_rates)"
    )

    parser.add_argument(
        "--keep_packets", action="store_true",
        help="Whether to save packet logs for each simulation run"
    )

    # Parse arguments
    args = parser.parse_args()
    output_dir = args.output_dir
    keep_packets = args.keep_packets
    template_cfg_fp = args.template_cfg
    simulator_path = args.simulator_path

    policies = []
    for policy in [x.strip() for x in args.policies.split(",")]:
        if policy != '' and (policy not in policies): policies.append(policy)

    innocent_rates = []
    for rate_str in [x.strip() for x in args.innocent_rates.split(",")]:
        if rate_str == '': continue
        rate_str = RateConverter.to_str(rate_str) # Standardize
        if rate_str not in innocent_rates: innocent_rates.append(rate_str)

    attack_rates = []
    for rate_str in [x.strip() for x in args.attack_rates.split(",")]:
        if rate_str == '': continue
        rate_str = RateConverter.to_str(rate_str) # Standardize
        if rate_str not in attack_rates: attack_rates.append(rate_str)

    # If any of the lists is empty, nothing to do
    if len(policies) == 0 or len(innocent_rates) == 0 or len(attack_rates) == 0:
        print("Either no policies or rates were specified, returning early")
        return

    # List of warnings
    warnings = ""

    # Set up the output directory
    if os.path.exists(output_dir):
        warnings += "WARNING: Output path exists, contents will be truncated\n"
        if os.path.isfile(output_dir): os.remove(output_dir)
        elif os.path.isdir(output_dir): shutil.rmtree(output_dir)
        else: raise ValueError("{} is not a file or dir".format(output_dir))

    os.makedirs(output_dir, exist_ok=True)
    shutil.copy(template_cfg_fp, os.path.join(output_dir, "template.cfg"))

    # First, run the simulation in dry-run mode
    log_fp = os.path.join(output_dir, "log.packets")
    subprocess.run(["{}".format(simulator_path),
                    "--config={}".format(template_cfg_fp),
                    "--packets={}".format(log_fp), "--dry"])

    # If the dry-run failed, return immediately
    if not os.path.isfile(log_fp):
        print("ERROR: Simulation dry-run failed")
        return

    # Next, create sub-directories
    for subdir in ["configs", "packets", "outputs"]:
        subdir_path = os.path.join(output_dir, subdir)
        os.mkdir(subdir_path)

        for policy in policies:
            os.mkdir(os.path.join(subdir_path, policy))

    # Print header and display all warnings
    print("==========================================")
    print("              Job Generation              ")
    print("==========================================")
    print(warnings)

    print("Computing optimal adversarial strategies")

    # If the log was successfully created, perform analysis
    # for each specified policy and generate jobs for them.
    jobs = ""
    for policy in policies:
        analyzer = None

        # FCFS
        if policy == analyze_fcfs.FCFSAnalyzer.get_policy_name():
            analyzer = analyze_fcfs.FCFSAnalyzer(template_cfg_fp, log_fp)
        # FQ
        elif policy == analyze_fq.FQAnalyzer.get_policy_name():
            analyzer = analyze_fq.FQAnalyzer(template_cfg_fp, log_fp)
        # SJF
        elif policy == analyze_sjf.SJFAnalyzer.get_policy_name():
            analyzer = analyze_sjf.SJFAnalyzer(template_cfg_fp, log_fp)
        # SJF-Inorder
        elif policy == analyze_sjf_inorder.SJFInorderAnalyzer.get_policy_name():
            analyzer = analyze_sjf_inorder.SJFInorderAnalyzer(template_cfg_fp, log_fp)
        # WSJF
        elif policy == analyze_wsjf.WSJFAnalyzer.get_policy_name():
            analyzer = analyze_wsjf.WSJFAnalyzer(template_cfg_fp, log_fp)
        # WSJF-Inorder
        elif policy == analyze_wsjf_inorder.WSJFInorderAnalyzer.get_policy_name():
            analyzer = analyze_wsjf_inorder.WSJFInorderAnalyzer(template_cfg_fp, log_fp)
        # Unknown policy
        else: raise NotImplementedError

        print("Policy: {}".format(policy))

        # For each innocent and attack rate, create
        # configs using the template configuration.
        for r_I_str in innocent_rates:
            r_I = RateConverter.from_str(r_I_str)
            print("\tFor r_I = {}bps".format(
                RateConverter.to_str(r_I, pretty=True)))

            for r_A_str in attack_rates:
                r_A = RateConverter.from_str(r_A_str)

                # Generate the filename
                filename = "I{}_A{}".format(r_I_str, r_A_str)

                # Output configuration file
                out_config_fp = os.path.join(output_dir,
                    "configs", policy, "{}.cfg".format(filename))

                # Compute the optimal attack strategy
                strategy = analyzer.get_optimal_strategy(
                    r_I / (10 ** 9), r_A / (10 ** 9))

                # TODO(natre): Fix theoretical estimates for FQ
                output = ("For r_A = {}bps: J_A = {:.2f}, P_A = {}".
                    format(RateConverter.to_str(r_A, pretty=True),
                        strategy.estimated_jsize, strategy.psize))

                if policy != "fq":
                    output += (", Expected Goodput = {:.4f} Gbps (Expected DF = {:.4f})".
                        format(strategy.expected_goodput_gbps, strategy.expected_alpha))

                print("\t\t{}".format(output))

                # Open and edit the template configuration
                with io.open(template_cfg_fp) as in_cfg:
                    config = libconf.load(in_cfg)
                    config["policy"] = policy

                    config["innocent_traffic"]["rate_bps"] = r_I
                    config["innocent_traffic"][
                        "average_packet_size_bits"] = analyzer.average_innocent_psize

                    config.setdefault("attack_traffic", {})
                    config["attack_traffic"]["rate_bps"] = r_A
                    config["attack_traffic"]["type"] = "synthetic"
                    config["attack_traffic"]["num_flows"] = 1000000
                    config["attack_traffic"]["packet_size_bits"] = strategy.psize
                    config["attack_traffic"]["job_size_ns"] = strategy.estimated_jsize

                    with open(out_config_fp, 'w') as out_cfg:
                        libconf.dump(config, out_cfg)

                # Output summary
                summary_fp = os.path.join(output_dir, "outputs", policy,
                                          "{}.out".format(filename))
                # Packets file path
                packets_fp = os.path.join(output_dir, "packets", policy,
                                          "{}.packets".format(filename))

                # Create a new simulation job
                jobs += (
                    "{} --config={} ".format(simulator_path, out_config_fp) +
                    ("--packets={} ".format(packets_fp) if keep_packets else "") +
                    "| tee {}".format(summary_fp) +
                    "\n"
                )
            print()
        print()

    # Write the jobs shell script and make it executable
    jobs_fp = os.path.join(output_dir, "jobs.sh")
    with open(jobs_fp, 'w') as jobs_f: jobs_f.write(jobs)
    os.chmod(jobs_fp, os.stat(jobs_fp).st_mode | stat.S_IEXEC)

    print("Generated jobs: {}".format(jobs_fp))

if __name__ == "__main__":
    generate_jobs()
