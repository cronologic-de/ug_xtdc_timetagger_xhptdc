import matplotlib.pyplot as plt
from matplotlib.axes import Axes
import matplotlib.ticker as mticker
import numpy as np
import mplutils as mplu

FNAME = "cable_delay_hist"
CHANNEL_PAIRS = [f"A and {c}" for c in "BCDEFGH"]


def normalize_to_100(data: np.ndarray) -> np.ndarray:
    sums = np.sum(data[1:], axis=1)
    for d, s in zip(data[1:], sums):
        d /= s / 100.0
    return data


def plot():

    data = normalize_to_100(np.loadtxt(f"{FNAME}.dat").T)
    ymax = data[1:].max()

    plt.style.use("cronostyle.mplstyle")
    plt.rcParams["axes.spines.left"] = True
    plt.rcParams["axes.spines.bottom"] = True
    plt.rcParams["axes.spines.top"] = True
    plt.rcParams["axes.spines.right"] = True

    layout_engine = mplu.FixedLayoutEngine(
        col_pads_ignore_labels=True,
        col_pads_pts=3.0,
        max_figwidth=190 / mplu.constants.MM_PER_INCH,
    )
    fig, axs = plt.subplots(1, 7, layout=layout_engine)

    for ax, y, l in zip(axs, data[1:], CHANNEL_PAIRS):
        ax: Axes
        ax.plot(data[0], y, drawstyle="steps-mid")
        ax.set_ylim(-1, ymax * 1.05)
        ax.set_xlim(-4.5, 4.5)
        ax.text(0.0, 1.01, l, transform=ax.transAxes)
        ax.xaxis.set_major_locator(mticker.MultipleLocator(2))
        mplu.set_axes_size(0.9, 3, ax=ax)

    for ax in axs[1:]:
        ax.set_yticklabels([])

    axs[0].set_ylabel("Intensity (%)")
    axs[3].set_xlabel("Relative delay (bins)")
    axs[0].set_title("xHPTDC8 cable delay histograms", pad=18)

    fig.savefig(f"{FNAME}.pdf")


if __name__ == "__main__":
    plot()
