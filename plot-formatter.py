import logging
from sys import argv
from os import name, path, access, R_OK
import matplotlib
matplotlib.use('tkagg')
import matplotlib.pyplot as plt
import json
import argparse
from typing import *


def int_to_propagation_mode(value:int) -> str:
    if value == 0:
        return 'i2o'
    elif value == 1:
        return 'o2i'
    elif value == 2:
        return 'o2i with static marking'
    elif value == 3:
        return 'o2i with i2o marking'
    return ''


def int_to_marker(value:int) -> str:
    if value == 0:
        return 'o'
    elif value == 1:
        return 'v'
    elif value == 2:
        return '*'
    elif value == 3:
         return '+'
    return ''


class Instance:
    problem_instance:int
    runs:List[float]
    settings:Dict[str, Any]

    def __init__(self, problem_instance:int, settings:Dict[str, Any]):
        self.problem_instance:int = problem_instance
        self.runs:List[float] = []
        self.settings:Dict[str, Any] = settings
    
    @property
    def probes_per_second(self) -> float:
        if len(self.runs) == 1:
            return self.runs[0]
        return sum(self.runs) / len(self.runs)

    def add_instance(self, instance):
        assert self.problem_instance == instance.problem_instance
        self.runs = self.runs + instance.runs
        return self
    
    def add_run(self, probes_per_second:float):
        self.runs.append(probes_per_second)
        return self
    
    def add(self, arg:Any):
        if isinstance(arg, Instance):
            return self.add_instance(arg)
        if isinstance(arg, float):
            self.runs.append(arg)
        return self
    
    def overlaps(self, instance) -> bool:
        return self.problem_instance == instance.problem_instance

class InstanceCollection:
    propagation_mode:int
    arguments:List[int]
    instances:Dict[int, Instance]
    settings:Dict[str, Any]
    argument_labels:List[str]

    def __init__(self, propagation_mode:int, arguments:List[int], settings:Dict[str, Any]):
        self.propagation_mode:int = propagation_mode
        self.arguments:List[int] = arguments
        self.instances:Dict[int, Instance] = dict()
        self.settings:Dict[str, Any] = settings
        
        self.argument_labels:List[Tuple[str, int]] = []
        argument_order = settings.get('argument_order', [])
        
        append_prop_mode = "PROPAGATION_MODE" not in argument_order
        for i in range(len(argument_order), len(arguments) - (1 if append_prop_mode else 0)):
            argument_order.append(None)
        
        if append_prop_mode:
            argument_order.append("PROPAGATION_MODE")
        
        for i in range(len(self.arguments)):
            if len(argument_order) <= i:
                self.argument_labels.append((None, self.arguments[i]))
            elif argument_order[i] in {"IGNORE", "PROPAGATION_MODE"}:
                continue
            else:
                self.argument_labels.append((argument_order[i], self.arguments[i]))
    
    @property
    def propagation_modes(self) -> List[Instance]:
        return list(sorted(self.instances.keys()))
    
    def add_instance(self, instance: Instance):
        if instance.problem_instance not in self.instances:
            self.instances[instance.problem_instance] = instance
        else:
            self.instances[instance.problem_instance].add_instance(instance)
        return self
    
    def create_instance(self, problem_instance:int) -> Instance:
        instance = Instance(problem_instance, self.settings)
        
        if problem_instance not in self.instances:
            self.instances[problem_instance] = instance
        else:
            self.instances[problem_instance].add_instance(instance)
                    
        return instance

    def add_instance_collection(self, instance: Instance):
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
    
    def results(self) -> Tuple[List[str], List[float]]: 
        problem_instance_names = list(sorted(self.instances.keys()))
        return problem_instance_names, [self.instances[problem_instance_name].probes_per_second for problem_instance_name in problem_instance_names]

    def orverlaps(self, instance_collection) -> bool:
        if self.propagation_mode != instance_collection.propagation_mode:
            return False
        intersection = set(self.instances.keys).intersection(instance_collection.instances.keys())
        
        return any((self.instances[i].overlaps(instance_collection.instances[i]) for i in intersection))


class PropagationModeCollection:
    identifier:str
    instance_collections:Dict[int, InstanceCollection]
    settings:Dict[str, Any]
  
    def __init__(self, identifier, settings:Dict[str, Any]):
        self.identifier:str = identifier
        self.instance_collections:Dict[int, InstanceCollection] = dict()
        self.settings:Dict[str, Any] = settings

    @property
    def arguments(self) -> List[str]:
        for instance in self.instance_collections.values():
            if len(instance.arguments) > 0:
                return instance.arguments[0:-1]
        return []
    
    @property
    def argument_labels(self) -> List[str]:
        for instance in self.instance_collections.values():
            if len(instance.argument_labels) > 0:
                return instance.argument_labels
        return []
    
    @property
    def propagation_modes(self) -> List[str]:
        return list(sorted(list(self.instance_collections.keys)))

    def add_instance_collection(self, instance_collection:InstanceCollection):
        if instance_collection.propagation_mode not in self.instance_collections:
            self.instance_collections[instance_collection.propagation_mode] = instance_collection
        else:
            self.instance_collections[instance_collection.propagation_mode].add_instance_collection(instance_collection)
        
        return self
    
    def create_instance_collection(self, propagation_mode:int, arguments:List[int]) -> InstanceCollection:
        instance_collection = InstanceCollection(propagation_mode, arguments, self.settings)
        self.add_instance_collection(instance_collection)
        return instance_collection

    def add_propagation_mode_collection(self, propagation_mode_collection):
        assert self.identifier == propagation_mode_collection.identifier
        
        for instance_collection in propagation_mode_collection.instance_collections.values():
            self.add_instance_collection(instance_collection)
        return self

    def add(self, obj):
        if isinstance(obj, InstanceCollection):
            return self.add_instance_collection(obj)
        if isinstance(obj, PropagationModeCollection):
            return self.add_propagation_mode_collection(obj)
        return self
    
    def orverlaps(self, propagation_mode_collection) -> bool:
        if self.identifier != propagation_mode_collection.identifier:
            return False
        
        intersection = set(self.instance_collections.keys).intersection(propagation_mode_collection.instance_collections.keys())
        
        return any((self.instances[pm].overlaps(propagation_mode_collection.instance_collections[pm]) for pm in intersection))


class Method:
    name:str
    propagation_mode_collections:Dict[str, PropagationModeCollection]
    settings:Dict[str, Any]

    def __init__(self, name:str, settings:Dict[str, Any]):
        self.name:str = name
        self.propagation_mode_collections:Dict[str, PropagationModeCollection] = dict()
        self.settings:Dict[str, Any] = settings

    @property
    def pretty_name(self) -> str:
        return self.settings.get('method_name', dict()).get(self.name, self.name)

    @property
    def argument_labels(self) -> List[str]:
        for propagation_model_collection in self.propagation_mode_collections.values():
            if len(propagation_model_collection.argument_labels) > 0:
                return propagation_model_collection.argument_labels
        return []

    def add_propagation_mode_collection(self, propagation_mode_collection:PropagationModeCollection):
        if propagation_mode_collection.identifier not in self.propagation_mode_collections:
            self.propagation_mode_collections[propagation_mode_collection.identifier] = propagation_mode_collection
        else:
            self.propagation_mode_collections[propagation_mode_collection.identifier].add_propagation_mode_collection(propagation_mode_collection)
        
        return self

    def create_propagation_mode_collection(self, identifier:str):
        propagation_mode_collection = PropagationModeCollection(identifier, self.settings)
        if identifier not in self.propagation_mode_collections:
            self.propagation_mode_collections[identifier] = propagation_mode_collection
        else:
            self.propagation_mode_collections[identifier].add_propagation_mode_collection(propagation_mode_collection)
        return propagation_mode_collection

    def add_method(self, method):
        assert self.name == method.name
        
        for propagation_mode_collection in method.propagation_mode_collections.values():
            self.add_propagation_mode_collection(propagation_mode_collection)
        return self

    def add(self, obj):
        if isinstance(obj, PropagationModeCollection):
            return self.add_propagation_mode_collection(obj)
        if isinstance(obj, Method):
            return self.add_method(obj)
        return self

    def contains(self, identifier) -> bool:
        return identifier in self.identifiers

    def orverlaps(self, method) -> bool:
        if self.name != method.name:
            return False
        
        intersection = set(self.propagation_mode_collections.keys).intersection(method.propagation_mode_collections.instance_collections.keys())
        return any((self.propagation_mode_collections[i].overlaps(method.propagation_mode_collections[i]) for i in intersection))


class Model:
    name:str
    methods:Dict[str, Method]
    settings:Dict[str, Any]
    
    def __init__(self, name, settings:Dict[str, Any]):
        self.name:str = name
        self.methods:Dict[str, Method] = dict()
        self.settings:Dict[str, Any] = settings
    
    @property
    def pretty_name(self) -> str:
        return self.settings.get('model_name', self.name)
    
    @property
    def yscale(self) -> str:
        return self.settings.get('yscale', 'linear')
    
    @property
    def argument_labels(self) -> List[str]:
        for method in self.methods.values():
            if len(method.argument_labels) > 0:
                return method.argument_labels
        return []
    
    def add_method(self, method:Method):
        if method.name not in self.methods:
            self.methods[method.name] = method
        else:
            self.methods[method.name].add_method(method)
        
        return self
    
    def create_method(self, method_name:str) -> Method:
        method = Method(method_name, self.settings)
        if method_name not in self.methods:
            self.methods[method_name] = method
        else:
            self.methods[method_name].add_method(method)
        return method

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

    def orverlaps(self, model) -> bool:
        if self.name != model.name:
            return False
        
        intersection = set(self.methods.keys).intersection(model.methods.keys())
        return any((self.methods[n].overlaps(model.methods[n]) for n in intersection))


class ModelCollection:
    models:Dict[str, Model]
    def __init__(self):
        self.models = dict()
    
    def add_model(self, model:Model):
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

    def orverlaps(self, model_collection) -> bool:
        intersection = set(self.models.keys).intersection(model_collection.models.keys())
        return any((self.models[n].overlaps(model_collection.models[n]) for n in intersection))


class ProblemCollection:
    model_collections: Dict[str, ModelCollection]
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

    def parse_problem_collection(self, json_data:Dict) -> ProblemCollection:
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

        settings = self.settings.get(model_name, dict())

        prop_mode_index = next((i for i, val in enumerate(settings.get('agument_order', [])) if val == 'PROPAGATION_MODE'), len(arguments) - 1)
        propagation_mode = arguments[prop_mode_index]
        
        problem_instance_index = max(0, len(arguments) - (2 if prop_mode_index == len(arguments) - 1 else 1))
        problem_instance = arguments[problem_instance_index]
        
        logging.info(f"prop_mode_index: {prop_mode_index}; problem_instance_index: {problem_instance_index}")

        identifier = '/'.join(name_parts[1:2] + [str(a) for i, a in enumerate(arguments) if i not in {prop_mode_index, problem_instance_index}])


        probes_per_second = benchmark['probes_per_second']

        model = Model(model_name, settings)

        model.create_method(method_name)\
             .create_propagation_mode_collection(identifier)\
             .create_instance_collection(propagation_mode, arguments)\
             .create_instance(problem_instance)\
             .add_run(probes_per_second)
        
        return model

    def parse_json(self, json_instance) -> ModelCollection:
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

        for problem_name, model_collection in problem_collection.items():
            for model in model_collection.models.values():
                settings = self.settings.get(model.name, dict()).get('agument_order', [])
                if "PROPAGATION_MODE" not in settings:
                    settings.append("PROPAGATION_MODE")

                for method in model.methods.values():
                    for instance_collection in method.propagation_mode_collections.values():
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
        for model_collections in self.problem_collection.model_collections.values():
            for model in model_collections.models.values():
                logging.info(model.name)
                for method in model.methods.values():
                    logging.info(method.name)
                    for propagation_mode_collection in method.propagation_mode_collections.values():
                        logging.info(propagation_mode_collection.identifier)
                        argument_labels = propagation_mode_collection.argument_labels
                        logging.info(argument_labels)
                        title_entries = [a if t is None else f'{t}: {a}' for t, a in argument_labels]
                        title = ' - '.join(map(str, [model.pretty_name, method.pretty_name] + title_entries[0:-1]))
                        xlabel = argument_labels[-1][0] if len(title_entries) > 0 and argument_labels[-1][0] is not None else 'n'
                        plt.ylabel(f'probes/s')
                        plt.yscale(model.yscale)
                        plt.title(title)
                        plt.xlabel(xlabel)
                        
                        xticks = set()

                        for propagation_mode, instance_collection in propagation_mode_collection.instance_collections.items():
                            x_vals, y_vals = instance_collection.results()
                            xticks.update(x_vals)
                            plt.plot(
                                x_vals,
                                y_vals,
                                label=int_to_propagation_mode(propagation_mode),
                                marker=int_to_marker(propagation_mode)
                            )
                    
                        plt.xticks(list(sorted(xticks)))
                        plt.legend()
                        
                        if self.save_plot('-'.join([model.name] + propagation_mode_collection.identifier.split('/'))):
                            plt.clf()
                        else:
                            plt.show()
            exit(0)

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