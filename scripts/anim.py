import matplotlib.pyplot as plt
import matplotlib.animation as animation

from dataclasses import dataclass, field
import json
import os

@dataclass
class Data:
    index: int
    time: float

@dataclass
class DataFile:
    path: str
    colour: str
    style: str | tuple[int, tuple[int, ...]] # (offset, (on_off_sequence))
    data: list[Data] = field(
        default_factory=lambda: [],
        init=False
    )

    def __post_init__(self):
        with open(self.path) as data:
            for line in data:
                if line.startswith('#'):
                    continue
                if ':' in line:
                    # old-fashioned data file
                    index, _, time = map(str.strip, line.split('::'))
                else:
                    # new data file
                    index, time, _ = map(str.strip, line.split('|'))
                    time = time[:-1] # remove trailing 's'

                index = int(index)
                time = float(time)
                self.data.append(Data(index, time))

        while self.data[-2].time > 1.:
            self.data.pop()

    @classmethod
    def fromdict(cls, dc: dict[str, str], *, root: str = None):
        return cls(
            path=dc["path"] if root is None else os.path.join(root, dc["path"]),
            colour=dc["colour"],
            style=dc.get("style", "solid"),
        )

def load_data(fname: str, *, level: int = None) -> dict[str, DataFile]:
    with open(fname) as file:
        config: dict[str, dict] = json.load(file)

    assert isinstance(config, dict)
    return {
        key: DataFile.fromdict(dc, root=os.path.dirname(fname))
        for key, dc in config.items()
        if level is None or dc.get("level", 0) <= level
    }

def generate_plot_anim(stats: dict[str, DataFile], *, save_to: str = None):

    def cutoff(data: list[Data], max_idx: int):
        if max_idx > data[-1].index:
            return len(data)
        lo = 0
        hi = len(data)
        while hi - lo > 1:
            mid = (lo + hi) >> 1
            if data[mid].index > max_idx:
                hi = mid
            else:
                lo = mid
        return hi + (hi < len(data))

    def update(step: int, ax: plt.Axes, stats: dict[str, DataFile], algs: list[str],
               lines: list[plt.Line2D], lengths: list[int]):
        max_idx = lengths[step]
        xmax = 1
        for i, alg in enumerate(algs):
            data = stats[alg].data
            bound = cutoff(data, max_idx)
            xs = [data[i].index for i in range(bound)]
            ys = [data[i].time for i in range(bound)]
            lines[i].set_data(xs, ys)
            xmax = max(0, xmax, *xs)

        ax.set_xlim(0, xmax*1.1)
        return lines

    fig, ax = plt.subplots()
    plt.subplots_adjust(right=0.6)
    fig.set_size_inches(10, 6)

    lines = list[plt.Line2D]()
    lengths = list[int]()

    prev = 0
    delta = 1
    for top_index in sorted(map(lambda alg: alg.data[-1].index, stats.values())):
        delta = max(delta, (top_index - prev) >> 8)
        lengths.extend(range(prev, top_index+delta-1, delta))
        prev = top_index
    lengths.append(top_index)

    algs = sorted(stats.keys(), key=lambda alg: stats[alg].data[-1].index)
    for alg in algs:
        line, = ax.plot([], [], label=f"{alg}", color=stats[alg].colour, linestyle=stats[alg].style)
        lines.append(line)
        print(f"{alg:>{max(map(len, algs))}}: {stats[alg].data[-2].index}")

    ax.legend(loc="center left", bbox_to_anchor=(1.05, 0.5))
    ax.set_xlabel("Fibonacci index")
    ax.set_ylabel("Runtime (s)")
    ax.set_ylim(0, 1)

    anim = animation.FuncAnimation(fig, update, len(lengths),
                                   fargs=[ax, stats, algs, lines, lengths],
                                   interval=2, repeat=False)

    if save_to is not None:
        # anim.save(f"{save_to}.gif", writer=animation.PillowWriter(
        #     fps=30, metadata=dict(artist="GSheaf"), bitrate=1800
        # ))
        anim.save(f"{save_to}.mp4", writer=animation.FFMpegWriter(fps=60, bitrate=4000, metadata=dict(artist="GSheaf")))
        fig.savefig(f"{save_to}.png", dpi=600)
        with open(f"{save_to}.md", 'w') as file:
            file.write("| Algorithm | Fibonacci index |\n")
            file.write("|:--------- | ---------------:|\n")
            for alg in algs:
                file.write("| {alg} | {stats} |\n".format(
                    alg = alg,
                    stats =f"{stats[alg].data[-1].index:,d}".replace(',', '\'')
                ))
    else:
        plt.show()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()

    parser.add_argument("json", help="JSON file containing all data.")
    parser.add_argument("-o", "--output", help="Output filename (without extension).")
    parser.add_argument("-l", "--level", type=int, help="Max level to plot.")

    args = parser.parse_args()

    plt.style.use("dark_background")

    stats = load_data(args.json, level=args.level)
    generate_plot_anim(stats, save_to=args.output)
