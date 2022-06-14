import argparse
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np
import itertools
import os

from common import RateConverter

# PyPlot static parameters
plt.rcParams['axes.grid'] = True
plt.rcParams['grid.alpha'] = 0.25
plt.rcParams['axes.axisbelow'] = True
prop_cycle = plt.rcParams['axes.prop_cycle']
color_list = prop_cycle.by_key()['color'][:4]

class DataPoint:
    """Represents a single output data-point."""
    def __init__(self):
        self.avg_goodput = None # Average goodput
        self.ss_goodput = None # Steady-state goodput
        self.ss_alpha = None # Steady-state displacement factor

def parse_data_point(output_fp: str) -> DataPoint:
    data_point = DataPoint()
    with open(output_fp, 'r') as log:
        for line in log:
            if "Average goodput" in line:
                values = [v.strip() for v in line.split(":")]
                assert len(values) == 2 # Sanity check
                data_point.avg_goodput = float(
                    values[1].split(" ")[0].strip())

            elif "Steady-state goodput" in line:
                values = [v.strip() for v in line.split(":")]
                assert len(values) == 2 # Sanity check
                data_point.ss_goodput = float(
                    values[1].split(" ")[0].strip())

            elif "Steady-state displacement factor" in line:
                values = [v.strip() for v in line.split(":")]
                assert len(values) == 2 # Sanity check
                data_point.ss_alpha = float(values[1])

    return data_point

def plot_data(results_dir: str) -> None:
    """Given a path to the results directory, plots the variation in
    goodput and DF as a function of the innocent and attack rate."""
    outputs_dir = os.path.join(results_dir, "outputs")
    output_subdir_names = set(os.listdir(outputs_dir))

    supported_policies = [
        ("fcfs", "FCFS"), ("fq", "FQ"),
        ("sjf", "SJF"), ("sjf_inorder", "SJF-Inorder"),
        ("wsjf", "WSJF"), ("wsjf_inorder", "WSJF-Inorder")]

    # List of policies simulated
    policies = []
    for policy_tuple in supported_policies:
        if policy_tuple[0] in output_subdir_names:
            policies.append(policy_tuple)

    # Datapoints
    recursive_dict = lambda: defaultdict(recursive_dict)
    datapoints = recursive_dict()

    # Simulated innocent and attack rates. We only pick those
    # rates for which all policies have simulated data-points.
    r_I_set_list, r_A_set_list = [], []
    for policy_tag, _ in policies:
        output_subdir = os.path.join(outputs_dir, policy_tag)
        r_I_set, r_A_set = set(), set()

        for filename in os.listdir(output_subdir):
            output_fp = os.path.join(output_subdir, filename)
            values = os.path.splitext(filename)[0].split('_')

            # Sanity check: Expect I*_A*.out
            assert len(values) == 2
            assert values[0].startswith("I")
            assert values[1].startswith("A")

            # Extract the rates
            r_I_str = values[0][1:]
            r_A_str = values[1][1:]
            r_I = RateConverter.from_str(r_I_str)
            r_A = RateConverter.from_str(r_A_str)

            # Add the rates for this policy
            r_I_set.add(r_I)
            r_A_set.add(r_A)

            # Update the datapoints dict
            datapoints[policy_tag][r_I][r_A] = parse_data_point(output_fp)

        # Append the sets to the set-lists
        r_I_set_list.append(r_I_set)
        r_A_set_list.append(r_A_set)

    # Take the intersection of the sets
    r_I_list = sorted(set.intersection(*r_I_set_list))
    r_A_list = sorted(set.intersection(*r_A_set_list))

    if (len(policies) > 4) or (len(r_I_list) > 3):
        if len(policies) > 4: policies = policies[:4]
        if len(r_I_list) > 3: r_I_list = r_I_list[:3]
        print("Note: For readability, we only support plots with "
              "4 policies and 3 innocent input rates at a time.")

    # Generate the figure
    figure, axes = plt.subplots(nrows=2, ncols=len(r_I_list), sharex=True)

    # Iterators
    colors = itertools.cycle(color_list[:len(policies)])
    markers = itertools.cycle(('v', 'x', '^', '+')[:len(policies)])
    styles = itertools.cycle(('dashed', 'solid', 'solid', 'dashed')[:len(policies)])

    # Normalize Y scale
    ymax_alpha = 0
    ymin_alpha = np.infty

    labels = []
    handles = []
    for col_idx, r_I in enumerate(r_I_list):
        for policy_tag, policy_name in policies:
            # Fetch the list of relevant datapoints
            policy_data = [datapoints[policy_tag][r_I][r_A] for r_A in r_A_list]

            # Fetch the corresponding DFs and goodputs
            alpha_list = [dp.ss_alpha for dp in policy_data]
            goodput_list = [dp.ss_goodput if policy_tag != "fcfs"
                            else dp.avg_goodput for dp in policy_data]
            # Line params
            color = next(colors)
            style = next(styles)
            marker = next(markers)

            # Plot the goodput and the displacement factor
            axes[0, col_idx].plot(
                r_A_list, alpha_list, color=color,
                marker=marker, label=policy_name,
                markersize=8, linewidth=3, linestyle=style)

            handle = axes[1, col_idx].plot(
                r_A_list, goodput_list, color=color,
                marker=marker, label=policy_name,
                markersize=8, linewidth=3, linestyle=style)

            # Handle for legend
            if col_idx == 0:
                handles.append(handle)
                labels.append(policy_name)

            # Configure subgraphs
            axes[0, col_idx].set_xscale("log")
            axes[0, col_idx].set_yscale("log")
            axes[0, col_idx].tick_params(axis='y', labelsize=8,
                                         pad=0, rotation=45)
            axes[1, col_idx].set_xscale("log")
            axes[1, col_idx].set_ylim((
                axes[1, col_idx].get_ylim()[0], (r_I / 10 ** 9) * 1.1))

            axes[1, col_idx].tick_params(axis='y', labelsize=8)
            ylims = axes[0, col_idx].get_ylim()
            ymin_alpha = min(ymin_alpha, ylims[0])
            ymax_alpha = max(ymax_alpha, ylims[1] * 10)

    # Re-label X axis ticks (in terms of bps)
    tick_values = [r_A_list[0]]
    for x in r_A_list:
        if x >= tick_values[-1] * 10:
            tick_values.append(x)

    if r_A_list[-1] > tick_values[-1]:
        tick_values.append(tick_values[-1] * 10)

    tick_labels = ["{}bps".format(
        RateConverter.to_str(x, pretty=True)) for x in tick_values]

    for col_idx in range(len(r_I_list)):
        axes[1, col_idx].set_xticks(tick_values)
        axes[1, col_idx].set_xticklabels(tick_labels, fontsize=8)
        axes[1, col_idx].tick_params(axis='x', rotation=15, pad=0)

    # Rescale Y axis
    for col_idx in range(len(r_I_list)):
        axes[0, col_idx].set_ylim((ymin_alpha, ymax_alpha))

    # Add axes labels
    mid = int(len(r_I_list) / 2)
    axes[1, mid].set_xlabel("Attack Bandwidth (log-scale)")
    axes[1, 0].set_ylabel("Goodput (Gbps)", fontsize=10)
    axes[0, 0].set_ylabel("DF (log-scale)", fontsize=10)

    # Create the legend
    figure.legend(handles, labels=labels,
                  loc="upper center", ncol=4,
                  borderaxespad=0.1)
    plt.show()
    plt.close(figure)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Plots simulation results.")
    parser.add_argument(
        "output_dir", type=str,
        help="Fully-qualified path to output directory containing results"
    )

    # Parse arguments
    args = parser.parse_args()
    output_dir = args.output_dir

    # Plot results
    plot_data(output_dir)
