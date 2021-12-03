import logging
import re
from sys import argv
from os import name, path, access, R_OK
import matplotlib
matplotlib.use('tkagg')
import matplotlib.pyplot as plt
import json
import argparse


def int_to_propagation_mode(value):
    if value == 0:
        return 'i2o'
    elif value == 1:
        return 'o2i with i2o marking'
    elif value == 2:
        return 'o2i'
    return ''


def propagation_mode_to_marker(mode):
    if mode == 'i2o':
        return 'o'
    elif mode == 'o2i with i2o marking':
        return 'v'
    elif mode == 'o2i':
        return '*'
    return ''


class Instance:
    def __init__(self, problem_instance):
        self.problem_instance = problem_instance
        self.runs = []
    
    @property
    def probes_per_second(self):
        if len(self.runs) == 1:
            return self.runs[0]
        return sum(self.runs) / len(self.runs)

    def add_instance(self, instance):
        assert self.problem_instance == instance.problem_instance
        self.runs = self.runs + instance.runs
        return self
    
    def add(self, arg):
        if isinstance(arg, Instance):
            return self.add_instance(arg)
        if isinstance(arg, float):
            self.runs.append(arg)
        return self
    
    def overlaps(self, instance):
        return self.problem_instance == instance.problem_instance

class InstanceCollection:
    def __init__(self, propagation_mode, arguments):
        self.propagation_mode = propagation_mode
        self.arguments = arguments
        self.instances = dict()
    
    @property
    def propagation_modes(self):
        return list(sorted(self.instances.keys()))
    
    def add_instance(self, instance):
        if instance.problem_instance not in self.instances:
            self.instances[instance.problem_instance] = instance
        else:
            self.instances[instance.problem_instance].add_instance(instance)
        return self

    def add_instance_collection(self, instance):
        assert self.propagation_mode == instance.propagation_mode
        
        for instance in instance.instances.values():
            self.add_instance(instance)
        
        return self

    def add(self, obj):
        if isinstance(obj, Instance):
            return self.add_instance(obj)
        if isinstance(obj, InstanceCollection):
            return self.add_instance_collection(obj)
        return self
    
    def results(self):
        problem_instances = list(sorted(self.instances.keys()))
        return problem_instances, [self.instances[problem_instance].probes_per_second for problem_instance in problem_instances]

    def orverlaps(self, instance_collection):
        if self.propagation_mode != instance_collection.propagation_mode:
            return False
        intersection = set(self.instances.keys).intersection(instance_collection.instances.keys())
        
        return any((self.instances[i].overlaps(instance_collection.instances[i]) for i in intersection))


class PropagationModeCollection:
    def __init__(self, identifier):
        self.identifier = identifier
        self.instance_collections = dict()

    @property
    def arguments(self):
        for instance in self.instance_collections.values():
            args = instance.arguments
            if len(args) > 0:
                return args[0:-1]
        return []
    
    @property
    def propagation_modes(self):
        return list(sorted(list(self.instance_collections.keys)))

    def add_instance_collection(self, instance_collection):
        if instance_collection.propagation_mode not in self.instance_collections:
            self.instance_collections[instance_collection.propagation_mode] = instance_collection
        else:
            self.instance_collections[instance_collection.propagation_mode].add_instance_collection(instance_collection)
        
        return self

    def add_propagation_mode_collection(self, instance_collection):
        assert self.identifier == instance_collection.identifier
        
        for instance in instance_collection.instance_collections.values():
            self.add_instance_collection(instance)
        
        return self

    def add(self, obj):
        if isinstance(obj, InstanceCollection):
            return self.add_instance_collection(obj)
        if isinstance(obj, PropagationModeCollection):
            return self.add_propagation_mode_collection(obj)
        return self
    
    def orverlaps(self, propagation_mode_collection):
        if self.identifier != propagation_mode_collection.identifier:
            return False
        
        intersection = set(self.instance_collections.keys).intersection(propagation_mode_collection.instance_collections.keys())
        
        return any((self.instances[pm].overlaps(propagation_mode_collection.instance_collections[pm]) for pm in intersection))

class Method:
    def __init__(self, name):
        self.name = name
        self.propagation_mode_collection = dict()

    def add_propagation_mode_collection(self, instance_collection):
        if instance_collection.identifier not in self.propagation_mode_collection:
            self.propagation_mode_collection[instance_collection.identifier] = instance_collection
        else:
            self.propagation_mode_collection[instance_collection.identifier].add_propagation_mode_collection(instance_collection)
        
        return self

    def add_method(self, method):
        assert self.name == method.name
        
        for instance_collection in method.propagation_mode_collection.values():
            self.add_propagation_mode_collection(instance_collection)
        
        return self

    def add(self, obj):
        if isinstance(obj, PropagationModeCollection):
            return self.add_propagation_mode_collection(obj)
        if isinstance(obj, Method):
            return self.add_method(obj)
        return self

    def contains(self, identifier):
        return identifier in self.identifiers

    def orverlaps(self, method):
        if self.name != method.name:
            return False
        
        intersection = set(self.propagation_mode_collection.keys).intersection(propagation_mode_collection.instance_collections.keys())        
        return any((self.propagation_mode_collection[i].overlaps(method.propagation_mode_collection[i]) for i in intersection))


class Model:
    def __init__(self, name):
        self.name = name
        self.methods = dict()
    
    def add_method(self, method):
        if method.name not in self.methods:
            self.methods[method.name] = method
        else:
            self.methods[method.name].add_method(method)
        
        return self

    def add_model(self, model):
        assert self.name == model.name
        for method in model.methods.values():
            self.add_method(method)
        return self
    
    def add(self, obj):
        if isinstance(obj, Method):
            return self.add_method(obj)
        if isinstance(obj, Model):
            return self.add_model(obj)
        return self

    def orverlaps(self, model):
        if self.name != model.name:
            return False
        
        intersection = set(self.methods.keys).intersection(model.methods.keys())
        return any((self.methods[n].overlaps(model.methods[n]) for n in intersection))


class ModelCollection:
    def __init__(self):
        self.models = dict()
    
    def add_model(self, model):
        if model.name not in self.models:
            self.models[model.name] = model
        else:
            self.models[model.name].add_model(model)
        return self

    def add_model_collection(self, model_collection):
        for model in model_collection.models.values():
            self.add_model(model)
        return self
    
    def add(self, obj):
        if isinstance(obj, Model):
            return self.add_model(obj)
        if isinstance(obj, ModelCollection):
            return self.add_model_collection(obj)
        return self

    def orverlaps(self, model_collection):
        intersection = set(self.models.keys).intersection(model_collection.models.keys())
        return any((self.models[n].overlaps(model_collection.models[n]) for n in intersection))


class ProblemCollection:
    def __init__(self):
        self.model_collections = dict()
    
    def add_model_collection(self, file_name, model_collection):
        if file_name not in self.model_collections:
            self.model_collections[file_name] = model_collection
        else:
            self.model_collections[file_name].add_model_collection(model_collection)
        return self


class PlotFormatter:
    def __init__(self, inputs, save_plots, output_dir, settings, file_prefix='', file_suffix=''):
        self.inputs = inputs
        self.output_dir = output_dir
        self.save_plots = save_plots
        self.settings = self.process_settings(settings)
        self.file_prefix = f'{file_prefix}-' if len(file_prefix) > 0 else ''
        self.file_suffix = f'-{file_suffix}' if len(file_suffix) > 0 else ''        
        json_data = self.retrieve_json()
        self.problem_collection = self.parse_problem_collection(json_data)

    def process_settings(self, settings_path):
        with open(settings_path, 'r') as settings_file:
            return json.load(settings_file)

    def retrieve_json(self):
        json_data = []
        for json_file_path in self.inputs:
            with open(json_file_path, 'r') as json_file:
                json_data.append(json.load(json_file))
        
        return json_data

    def parse_problem_collection(self, json_data):
        problem_collection = ProblemCollection()
        for tuple in zip(self.inputs, json_data):
            problem_collection.add_model_collection(tuple[0], self.parse_json(tuple[1]))
        
        return problem_collection

    def filter_json_benchmarks(self, json_instance):
        benchmarks = json_instance.get('benchmarks', [])
        
        filtered_benchmarks = list()
        
        for benchmark in benchmarks:
            if 'run_name' not in benchmark:
                continue
            
            run_name = benchmark['run_name']

            if benchmark.get('run_type', '') == 'aggregate':
                if benchmark.get('aggregate_name', '') != 'mean':
                    continue
            elif any((b.get('aggregate_name', '') == 'mean' for b in benchmarks if b.get('run_name', '') == run_name)):
                continue

            if benchmark.get('probes_per_second', None) is None:
                continue
            
            if len(run_name.split('/')) < 3:
                continue

            filtered_benchmarks.append(benchmark)
        
        return filtered_benchmarks

    def parse_json_benchmark(self, benchmark):
        name_parts = benchmark['run_name'].split('/')
        model_name = name_parts[0]
        method_name = name_parts[1]

        arguments = [int(a) for a in name_parts[2:]]

        prop_mode_index =  next((i for i, val in enumerate(self.settings.get(model_name, dict()).get('agument_order', [])) if val == 'PROPAGATION_MODE'), 0)
        propagation_mode = int(arguments[prop_mode_index])

        identifier = '/'.join(name_parts[1:prop_mode_index+2] + name_parts[prop_mode_index+3:-1])

        problem_instance = arguments[-1]
        probes_per_second = benchmark['probes_per_second']

        return Model(model_name).add_method(
          Method(method_name).add_propagation_mode_collection(
            PropagationModeCollection(identifier).add_instance_collection(
              InstanceCollection(propagation_mode, arguments).add_instance(
                Instance(problem_instance).add(
                  probes_per_second
                )
              )
            )
          )
        )

    def parse_json(self, json_instance):
        model_collection = ModelCollection()
        for benchmark in self.filter_json_benchmarks(json_instance):
            model_collection.add_model(self.parse_json_benchmark(benchmark))

        return model_collection

    def merge_model_collections(self, model_collections):
        merged_model_collection = None

        for model_collection in model_collections:
            if merged_model_collection is None:
                merged_model_collection = model_collection
            else:
                merged_model_collection.add_model_collection(model_collection)
            
        return merged_model_collection

    def plot_model_collection(self):
        self.plot_model_collection_no_compare()
    
    def plot_model_collection_compare(self):
        problem_collection = list(self.problem_collection.model_collections.items())

        for i, (problem_name, model_collection) in enumerate(problem_collection):
            for model in model_collection.models.values():
                settings = self.settings.get(model.name, dict()).get('agument_order', [])
                if "PROPAGATION_MODE" not in settings:
                    settings.insert(0, "PROPAGATION_MODE")

                for method in model.methods.values():
                    for instance_collection in method.propagation_mode_collection.values():
                        arguments = instance_collection.arguments
                        label_entries = [model.name, method.name]
                        for i in range(max(len(arguments), len(settings))):
                            if len(arguments) <= i:
                                continue
                            elif len(settings) <= i:
                                label_entries.append(arguments[i])
                            elif settings[i] == "IGNORE":
                                continue
                            elif settings[i] == "PROPAGATION_MODE":
                                continue
                            else:
                                label_entries.append(f'{settings[i]}: {arguments[i]}')

                        other_problem_models = [pc for pc in problem_collection[i:] if pc.contains(model.name, method.name, instance_collection.identifier)]

                        if len(settings) <= 1:
                            plt.xlabel(f'n')
                        else:
                            plt.xlabel(settings[-1])
                        plt.ylabel(f'probes/s')
                        
                        plt.title(f'{problem_name} - {model.name} - {int_to_propagation_mode(propagation_mode)}')
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
                        plt.xticks(list(sorted(ticks)))
                        plt.legend()

                        if self.save_plot(f'{problem_name}-{model.name}-{method.name}'):
                            plt.clf()
                        else:
                            plt.show()
    
    def plot_model_collection_no_compare(self):
        for problem_name, model_collections in self.problem_collection.model_collections.items():
            for model in model_collections.models.values():
                settings = self.settings.get(model.name, dict())
                argument_order = settings.get('agument_order', [])

                if "PROPAGATION_MODE" not in argument_order:
                    argument_order.insert(0, "PROPAGATION_MODE")

                for method in model.methods.values():

                    for propagation_mode_collection in method.propagation_mode_collection.values():
                        arguments = propagation_mode_collection.arguments

                        label_entries = [
                            settings.get('model_name', model.name),
                            settings.get('method_name', method.name)
                        ]
                        for i in range(max(len(arguments), len(argument_order)) - 1):
                            if len(arguments) <= i:
                                continue
                            elif len(argument_order) <= i:
                                label_entries.append(arguments[i])
                            elif argument_order[i] == "IGNORE":
                                continue
                            elif argument_order[i] == "PROPAGATION_MODE":
                                continue
                            else:
                                label_entries.append(f'{argument_order[i]}: {arguments[i]}')
                        
                        plt.title(' - '.join(map(str, label_entries)))
                        
                        if len(argument_order) <= 1:
                            plt.xlabel(f'n')
                        else:
                            plt.xlabel(argument_order[-1])

                        plt.ylabel(f'probes/s')
                        plt.yscale(settings.get('yscale', 'linear'))
                        
                        xticks = set()

                        for propagation_mode, instance_collection in propagation_mode_collection.instance_collections.items():
                            x_vals, y_vals = instance_collection.results()
                            xticks.update(x_vals)
                            plt.plot(
                                x_vals,
                                y_vals,
                                label=f'{int_to_propagation_mode(propagation_mode)}',
                                marker=propagation_mode_to_marker(method.name)
                            )
                    
                        plt.xticks(list(sorted(xticks)))
                        plt.legend()

                        if 'figure_text' in settings:
                            figure_text = settings['figure_text']
                            for i in range(len(arguments)):
                                figure_text = figure_text.replace(f'{{{i}}}', str(arguments[i]))
                            plt.figtext(0.5, 0.01, figure_text, wrap=True, horizontalalignment='center', fontsize=10)
                        
                        if self.save_plot('-'.join([model.name] + propagation_mode_collection.identifier.split('/'))):
                            plt.clf()
                        else:
                            plt.show()

    def save_plot(self, file_name):
        if not self.save_plots:
            return False
        plot_filename = path.join(
            self.output_dir,
            f'{self.file_prefix}{file_name}{self.file_suffix}.png'
        )
        plt.savefig(plot_filename)
        return True


if __name__ == "__main__":
    class readable_dir(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
            prospective_dir=values
            if not path.isdir(prospective_dir):
                raise argparse.ArgumentTypeError(f"readable_dir:{prospective_dir} is not a valid path")
            if access(prospective_dir, R_OK):
                setattr(namespace,self.dest,prospective_dir)
            else:
                raise argparse.ArgumentTypeError(f"readable_dir:{prospective_dir} is not a readable dir")

    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir', dest='output_dir', action=readable_dir, default='plots')
    parser.add_argument('--save-plots', dest='save_plots', default=False, action="store_true")
    parser.add_argument('--file-prefix', type=str, dest='file_prefix', default='')
    parser.add_argument('--file-suffix', type=str, dest='file_suffix', default='')
    parser.add_argument('--input', dest='inputs', nargs='+', default=['tmp.json'])
    parser.add_argument('--settings', dest='settings', default='plot-formatter.json')
    parser.add_argument('-v', '--verbose', dest='verbose', default=True, action="store_true")
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.INFO)

    plot_formatter = PlotFormatter(
      args.inputs,
      args.save_plots,
      args.output_dir,
      args.settings,
      args.file_prefix,
      args.file_suffix
    )
    plot_formatter.plot_model_collection()