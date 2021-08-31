import logging
import re
from sys import argv
from os import path
import matplotlib
matplotlib.use('tkagg')
import matplotlib.pyplot as plt
import json


def int_to_propagation_mode(value):
    if value == 0:
        return 'i2o'
    elif value == 1:
        return 'mixed'
    elif value == 2:
        return 'o2i'
    return ''


def propagation_mode_to_marker(mode):
    if mode == 'i2o':
        return 'o'
    elif mode == 'mixed':
        return 'v'
    elif mode == 'o2i':
        return '*'
    return ''


class PlotFormatter:
    def __init__(self, arguments):
        self.logger = logging.getLogger('PlotFormatter')
        self.arguments = self.process_arguments(arguments)
        
        json_data = self.retrieve_json()
        self.benchmarks = self.retrieve_benchmarks(json_data)

    def process_arguments(self, arguments):
        if len(arguments.get('input', [])) == 0:
            raise Exception(f"No input file given.")

        for json_file_path in arguments.get('input', []):
            if json_file_path is None or not path.isfile(json_file_path):
                raise Exception(f"file {json_file_path} does not exists")

        if arguments.get('save_plots') != True:
            return arguments

        if arguments.get('output_dir', None) is None or not path.isdir(arguments['output_dir']):
            raise Exception("Output directory does not exist")
        
        if len(arguments.get('file_prefix', '')) > 0:
            arguments['file_prefix'] = arguments['file_prefix'] + '-'
        else:
            arguments['file_prefix'] = ''

        if len(arguments.get('file_suffix', '')) > 0:
            arguments['file_suffix'] = '-' + arguments['file_suffix']
        else:
            arguments['file_suffix'] = ''
        
        return arguments

    def retrieve_json(self):
        json_data = []
        for json_file_path in self.arguments.get('input', []):
            with open(json_file_path, 'r') as json_file:
                json_data.append(json.load(json_file))
        
        return json_data

    def retrieve_benchmarks(self, json_data):
        benchmark_instances = dict()
        for tuple in zip(self.arguments.get('input'), json_data):
            benchmark_instances[tuple[0]] = self.benchmark_avarage(
                self.parse_json_instance(tuple[1])
            )
        
        return self.merge_benchmark_instances(benchmark_instances)

    def parse_json_instance(self, json_instance):
        benchmarks = json_instance.get('benchmarks', None)
        if benchmarks is None:
            return dict()
        
        processed_benchmarks = dict()
        for benchmark in benchmarks:
            if 'name' not in benchmark:
                continue
            
            name = benchmark['name']

            if "aggregate_name" in name:
                if name["aggregate_name"] != 'mean':
                    continue
            elif any((b.get("aggregate_name", "") == 'mean' for b in benchmarks if b.get('name', "") == name)):
                continue

            probes_per_second = benchmark.get('probes_per_second', None)
            if probes_per_second is None:
                continue
            
            name_parts = name.split('/')

            if len(name_parts) < 1:
                continue

            propagation_mode_name_index = -1

            for i, name in enumerate(name_parts):
                if bool(re.search(r'^\d+$', name)) and len(int_to_propagation_mode(int(name))) > 0:
                    propagation_mode_name_index = i
                    propagation_mode_name = int_to_propagation_mode(int(name))
                    break
            
            if propagation_mode_name_index < 0:
                continue

            try:
                instance = int(name_parts[-1])
            except ValueError:
                continue
            
            model_name = name_parts[0]
            method_name = "/".join(
                name_parts[1:propagation_mode_name_index] + name_parts[propagation_mode_name_index+1:-1]
            )
            
            if model_name not in processed_benchmarks:
                processed_benchmarks[model_name] = dict()
            if method_name not in processed_benchmarks[model_name]:
                processed_benchmarks[model_name][method_name] = dict()
            if propagation_mode_name not in processed_benchmarks[model_name][method_name]:
                processed_benchmarks[model_name][method_name][propagation_mode_name] = []
            
            processed_benchmarks[model_name][method_name][propagation_mode_name].append((instance, probes_per_second))
            
        return processed_benchmarks

    def benchmark_avarage(self, benchmarks):
        for methods in benchmarks.values():
            for propagation_modes in methods.values():
                for propagation_mode, instances in propagation_modes.items():
                    averages = []
                    for instance in sorted({instance for instance, _ in instances}):
                        probes_per_second = list((pbs for i, pbs in instances if i == instance))

                        if len(probes_per_second) == 1:
                            averages.append((instance, probes_per_second[0]))
                            continue

                        average_instance = (instance, sum(probes_per_second) / len(probes_per_second))
                        
                        averages.append(average_instance)
                    propagation_modes[propagation_mode] = averages
        return benchmarks

    def merge_benchmark_instances(self, benchmark_instances):
        merged_benchmarks = dict()
        for file_name, benchmarks in benchmark_instances.items():
            file_name = path.splitext(file_name)[0]
            for problem_name, models in benchmarks.items():
                if problem_name not in merged_benchmarks:
                    merged_benchmarks[problem_name] = dict()

                for model, modes in models.items():
                    if model not in merged_benchmarks[problem_name]:
                        merged_benchmarks[problem_name][model] = dict()
                    for propagation_mode, benchmarks in modes.items():
                        if propagation_mode not in merged_benchmarks[problem_name][model]:
                            merged_benchmarks[problem_name][model][propagation_mode] = dict()
                        
                        if file_name not in merged_benchmarks[problem_name][model][propagation_mode]:
                            merged_benchmarks[problem_name][model][propagation_mode][file_name] = benchmarks

        return merged_benchmarks

    def plot_benchmarks(self):
        for models in self.benchmarks.values():
            for modes in models.values():
                if max((len(files) for files in modes.values()), default=1) > 1:
                    self.plot_benchmarks_compare()
                    return
        
        self.plot_benchmarks_no_compare()
    
    def plot_benchmarks_compare(self):
        for problem_name, models in self.benchmarks.items():

            for model_name, modes in models.items():

                for mode_name, files in modes.items():
                    if len(files) <= 1:
                      continue

                    plt.xlabel(f'n')
                    plt.ylabel(f'probes/s')
                    plt.title(f'{problem_name} - {model_name} - {mode_name}')
                    ticks = set()

                    for file_name, benchmarks in files.items():
                        x_vals = [int(e[0]) for e in benchmarks]
                        y_vals = [e[1] for e in benchmarks]
                        for x_val in x_vals:
                            ticks.add(x_val)
                        plt.plot(
                            x_vals,
                            y_vals,
                            label=f'{file_name}'
                        )
                    
                    ticks = list(ticks)
                    ticks.sort()
                    plt.xticks(ticks)
                    plt.legend()

                    if self.save_plot(f'{problem_name}-{model_name}-{mode_name}'):
                        plt.clf()
                    else:
                        plt.show()
    
    def plot_benchmarks_no_compare(self):
        for problem_name, models in self.benchmarks.items():
            for model_name, modes in models.items():
                
                plt.title(f'{problem_name}')
                ticks = set()

                for mode_name, files in modes.items():
                    plt.xlabel(f'n')
                    plt.ylabel(f'probes/s')
                    
                    for benchmarks in files.values():
                        x_vals = [int(e[0]) for e in benchmarks]
                        y_vals = [e[1] for e in benchmarks]
                        for x_val in x_vals:
                            ticks.add(x_val)
                        plt.plot(
                            x_vals,
                            y_vals,
                            label=f'{model_name} - {mode_name}',
                            marker=propagation_mode_to_marker(mode_name)
                        )
                
                ticks = list(ticks)
                ticks.sort()
                plt.xticks(ticks)
                plt.legend()
                
                if self.save_plot(f'{problem_name}-{model_name}'):
                    plt.clf()
                else:
                    plt.show()

    def save_plot(self, file_name):
        if self.arguments.get('save_plots', False) != True:
            return False
        plot_filename = path.join(
            self.arguments['output_dir'],
            f'{self.arguments["file_prefix"]}{file_name}{self.arguments["file_suffix"]}.png'
        )
        plt.savefig(plot_filename)
        return True


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    flag_prefix = '--'
    flag_splitter = '='

    arguments = {
        # key: default value
        'output_dir': 'plots',
        'file_prefix': '',
        'file_suffix': ''
    }
    bool_flags = ['save_plots']
    multiple = ['input', ['tmp.json']]

    for f in arguments.keys():
        flag = f'{flag_prefix}{f}{flag_splitter}'
        argument = next((n[len(flag):] for n in argv if n.startswith(flag)), None)
        if argument is None:
            continue
        arguments[f] = argument
    
    for f in bool_flags:
        flag = f'{flag_prefix}{f}'
        arguments[f] = any((n.startswith(flag) for n in argv))
    
    for f in multiple:
        flag = f'{flag_prefix}{f}{flag_splitter}'
        index, argument = next(
            ((i, n[len(flag):]) for i, n in enumerate(argv) if n.startswith(flag)),
            (-1, None)
        )
        if argument is None:
            continue
        arguments[f] = [argument]
        for i in range(index + 1, len(argv)):
            if argv[i].startswith(flag_prefix):
                break
            arguments[f].append(argv[i])

    plot_formatter = PlotFormatter(arguments)
    plot_formatter.plot_benchmarks()