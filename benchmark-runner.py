import logging
from re import S
from sys import argv
from os import path
import json

class BenchmarkRunner:
    def __init__(self, arguments):
        self.logger = logging.getLogger('BenchmarkRunner')
        self.logger.debug(arguments)
        self.arguments = self.process_arguments(arguments)
        
        json_data = self.retrieve_json()
        self.benchmarks = self.retrieve_benchmarks(json_data)

    def process_arguments(self, arguments):
        if arguments.get('num_runs', None) is None:
            raise Exception("num_runs not set.")
        
        try:
            num_runs = int(arguments['num_runs'])
            if num_runs <= 0:
                raise ValueError("")
        except ValueError:
            raise Exception('num_runs must be a positive integer')

        if arguments.get('output', None) is None:
            raise Exception("No output file given.")
        
        dirname, filename = path.split(arguments['output'])
        if not path.isdir(dirname):
            raise Exception(f"Directory {dirname} does not exist.")

        if len(filename) is None:
            raise Exception(f"Cannot save to a directory.")

        filename, extension = path.splitext(filename)

        arguments['dirname'] = dirname
        arguments['filename'] = filename
        arguments['extension'] = extension

    def run_benchmarks(self):
        run_benchmarks = path.join(
            path.dirname(path.realpath(__file__)),
            'build',
            'runBenchmarks'
        )

        for run in range(1, self.arguments['num_runs'] + 1):
            filename = path.join(
              arguments['dirname'],
              f"{arguments['filename']}-{run}{arguments['extension']}"
            )
        process = run(
            args=[
                '--benchmark_format=json',
                f'--benchmark_out={filename}',
                f'--benchmark_repetitions={num_runs}'
            ],
            returncode=0
        )
            
        

    def retrieve_json(self):
        json_data = []
        for json_file_path in self.arguments.get('input', []):
            with open(json_file_path, 'r') as json_file:
                json_data.append(json.load(json_file))
        
        return json_data

    def retrieve_benchmarks(self, json_data):
        benchmark_instances = dict()
        for tuple in zip(self.arguments.get('input'), json_data):
            benchmark_instances[tuple[0]] = self.parse_json_instance(tuple[1])
        
        return self.merge_benchmark_instances(benchmark_instances)

    def parse_json_instance(self, json_instance):
        benchmarks = json_instance.get('benchmarks', None)
        if benchmarks is None:
            return dict()
        
        processed_benchmarks = dict()
        for benchmark in benchmarks:
            probes_per_second = benchmark.get('probes_per_second', None)
            if probes_per_second is None:
                continue
            
            name_parts = benchmark.get('name', benchmark.get('run_name', '')).split('/')


            if len(name_parts) < 1:
                continue
            
            try:
                propagation_mode = int_to_propagation_mode(int(name_parts[-2]))
            except ValueError:
                propagation_mode = int_to_propagation_mode(0)
                name_parts.insert(len(name_parts) - 1, 0)
            
            try:
                instance = int(name_parts[-1])
            except ValueError:
                continue
            
            d = processed_benchmarks
            for part in name_parts[:-2]:
                if part not in d:
                    d[part] = dict()
                d = d[part]
            
            if propagation_mode not in d:
                d[propagation_mode] = []
            
            d[propagation_mode].append((instance, probes_per_second))

        return processed_benchmarks

    def merge_benchmark_instances(self, benchmark_instances):
        merged_instances = dict()
        for file_name, benchmarks in benchmark_instances.items():
            file_name = path.splitext(file_name)[0]
            for problem_name, models in benchmarks.items():
                if problem_name not in merged_instances:
                    merged_instances[problem_name] = dict()

                for model_name, modes in models.items():
                    if model_name not in merged_instances[problem_name]:
                        merged_instances[problem_name][model_name] = dict()
                    for mode_name, benchmarks in modes.items():
                        if mode_name not in merged_instances[problem_name][model_name]:
                            merged_instances[problem_name][model_name][mode_name] = dict()
                        
                        if file_name not in merged_instances[problem_name][model_name][mode_name]:
                            merged_instances[problem_name][model_name][mode_name][file_name] = benchmarks

        self.logger.debug(merged_instances)
        return merged_instances

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
    logging.basicConfig(level=logging.WARNING)
    flag_prefix = '--'
    flag_splitter = '='

    arguments = {
        # key: default value
        'num_runs': '10',
        'output': 'benchmark-json/tmp.json',
    }
    
    for f in arguments.keys():
        flag = f'{flag_prefix}{f}{flag_splitter}'
        argument = next((n[len(flag):] for n in argv if n.startswith(flag)), None)
        if argument is None:
            continue
        arguments[f] = argument

    benchmark_runner = BenchmarkRunner(arguments)
    benchmark_runner.run_benchmarks()
    benchmark_runner.average()
    benchmark_runner.save()