import logging
from re import S
from sys import argv
from os import path
import matplotlib
matplotlib.use('tkagg')
import matplotlib.pyplot as plt
import json


def int_to_propagation_mode(value):
    if value == 0:
        return "i2o"
    elif value == 1:
        return "mixed"
    elif value == 2:
        return "o2i"
    return ""


def propagation_mode_to_marker(mode):
    if mode == "i2o":
        return "o"
    elif mode == "mixed":
        return "v"
    elif mode == "o2i":
        return "*"
    return ""


class PlotFormatter:
    def __init__(self, arguments):
        self.arguments = arguments
        self.logger = logging.getLogger('PlotFormatter')
        
        json_data = self.retrieve_json()
        self.benchmarks = self.retrieve_benchmarks(json_data)

    def retrieve_json(self):
        json_file_path = self.arguments.get('input', None)
        if json_file_path is None:
            return dict()
        if not path.isfile(json_file_path):
            return dict()
        with open(json_file_path, 'r') as json_file:
            json_data = json.load(json_file)
        return json_data

    def retrieve_benchmarks(self, json_data):
        benchmarks = json_data.get('benchmarks', None)
        if benchmarks is None:
            return list()
        processed_benchmarks = dict()
        for benchmark in benchmarks:
            probes_per_second = benchmark.get('probes_per_second', None)
            if probes_per_second is None:
                continue
            
            name_parts = benchmark.get('name', benchmark.get('run_name', '')).split('/')

            if len(name_parts) < 2:
                continue
            d = processed_benchmarks
            for part in name_parts[:-2]:
                if part not in d:
                    d[part] = dict()
                d = d[part]
            try:
                propagation_mode = int_to_propagation_mode(int(name_parts[-2]))
                instance = int(name_parts[-1])
            except ValueError:
                continue
            
            if propagation_mode not in d:
                d[propagation_mode] = []
            
            d[propagation_mode].append((instance, probes_per_second))
        
        return processed_benchmarks

    def plot_benchmarks(self):
        
        for problem_name, models in self.benchmarks.items():

            for model_name, modes in models.items():
                plt.title(f'{problem_name}')
                plt.xlabel(f'n')
                plt.ylabel(f'probes/s')
                ticks = set()

                for mode_name, benchmarks in modes.items():
                    x_vals = [int(e[0]) for e in benchmarks]
                    y_vals = [e[1] for e in benchmarks]
                    for x_val in x_vals:
                        ticks.add(x_val)
                    plt.plot(
                        x_vals,
                        y_vals,
                        label=f'{model_name} - {mode_name}',
                        marker=propagation_mode_to_marker(mode_name))
                ticks = list(ticks)
                ticks.sort()
                plt.xticks(ticks)
                plt.legend()
                plt.show()


if __name__ == "__main__":
    logging.basicConfig(level=logging.WARNING)
    flags = ["input"]
    arguments = {f:None for f in flags}
    for f in flags:
        flag = '--' + f +'='
        argument = next((n[len(flag):] for n in argv if n.startswith(flag)), None)
        arguments[f] = argument
    if arguments.get('input', None) is None:
        arguments['input'] = 'tmp.json'
    
    plot_formatter = PlotFormatter(arguments)
    plot_formatter.plot_benchmarks()