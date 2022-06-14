import matplotlib.pyplot as plt
import itertools

# PyPlot static parameters
plt.rcParams['axes.grid'] = True
plt.rcParams['grid.alpha'] = 0.25
plt.rcParams['axes.axisbelow'] = True

prop_cycle = plt.rcParams['axes.prop_cycle']
colors_all = prop_cycle.by_key()['color']
colors = itertools.cycle((colors_all[0],
                          colors_all[3],
                          colors_all[2],
                          colors_all[1]))

def autolabel(rects, axis=None):
    """Attach a text label above each bar displaying its height."""
    if axis is None: axis = plt
    for rect in rects:
        height = rect.get_height()
        label = ("{:.2f}".format(height) if height < 10.0
                 else "{:.1f}".format(height))

        axis.text(rect.get_x() + rect.get_width() / 2., height + 0.05,
                  label, ha='center', va='bottom', fontsize=8)

def plot_results():
    """Plots results."""
    goodputs = dict()
    # Update this to plot new results
    attack_rates                            = [0, 0.1, 1, 3]
    goodputs["FCFS"]                        = [9.94, 5.40, 0.74, 0.38]
    goodputs["Fibonacci Heap (drop-tail)"]  = [9.94, 5.87, 0.73, 0.38]
    goodputs["DEPQ (drop-max)"]             = [9.95, 9.95, 5.64, 2.65]
    goodputs["Hierarchical FFS Queue"]      = [9.95, 9.95, 9.94, 9.86]

    figure, axis = plt.subplots()

    width = 0.1
    handles = []
    x_labels = []
    x_centers = []
    start_x = width * 2

    for rate_idx, attack_rate in enumerate(attack_rates):
        for idx, tag in enumerate(["FCFS", "Fibonacci Heap (drop-tail)",
                                   "DEPQ (drop-max)", "Hierarchical FFS Queue"]):
            container = axis.bar([start_x + (idx * width)],
                                 [goodputs[tag][rate_idx]],
                                 width, color=next(colors), label=tag)
            autolabel(container)
            if (rate_idx == 0): handles.append(container)

        x_center = start_x + (width / 2)
        x_centers.append(x_center)
        x_labels.append("{}Gbps".format(attack_rate))
        start_x += width + (4 * width)

    axis.set_xticks(x_centers)
    axis.set_ylim((axis.get_ylim()[0], 12))
    axis.set_xticklabels(x_labels, fontsize="small")
    axis.set_ylabel("Goodput (Gbps)", fontsize="medium")
    axis.set_xlabel("Attack Rate (Gbps)", fontsize="medium")

    # Shrink current axis's height by 10% on the top
    box = axis.get_position()
    axis.set_position([box.x0, box.y0 + box.height * 0.1,
                       box.width, box.height * 0.9])

    # Put a legend above current axis
    legend = axis.legend(handles=handles, loc="upper center", ncol=2,
                         bbox_to_anchor=(0.5, 1.3), fontsize="small")
    axis.add_artist(legend)

    plt.subplots_adjust(top=0.820, bottom=0.170,
                        left=0.100, right=0.995,
                        hspace=0.200, wspace=0.200)
    plt.show()
    plt.close(figure)

if __name__ == "__main__":
    plot_results()
