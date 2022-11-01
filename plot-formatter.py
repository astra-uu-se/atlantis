from fileinput import filename
import logging
from os import path, access, R_OK
from re import X
import matplotlib
matplotlib.use('tkagg')
import matplotlib.pyplot as plt
import json
import argparse
from typing import *
from math import ceil


class Plot:
    def __init__(self, x_vals, y_vals, label, marker, linestyle):
        self.x_vals = x_vals
        self.y_vals = y_vals
        self.label = label
        self.marker = marker
        self.linestyle = linestyle


class Figure:
    def __init__(self, title, xlabel, ylabel, yscale, xticks, identifier, plots) -> None:
        self.title = title
        self.xlabel = xlabel
        self.ylabel = ylabel
        self.yscale = yscale
        self.xticks = xticks
        self.identifier = identifier
        self.plots:List[Plot] = plots
    
    @property
    def pretty_xticks(self):
        if len(self.xticks) == 0:
            return []
        first = int(self.xticks[0])
        last = int(self.xticks[-1])
        
        res = [first]
        min_dist = (last - first) / 20
        for xt in self.xticks[1:]:
            if int(xt) - res[-1] >= min_dist:
                res.append(xt)
        
        return res

def int_to_propagation_mode(value:int) -> str:
    if value == 0:
        return 'i2o'
    elif value == 1:
        return 'o2i'
    elif value == 2:
        return 'o2i - static'
    elif value == 3:
        return 'o2i - ad-hoc'
    return ''

def iteration_to_line_style(iteration:int) -> str:
    if iteration == 0:
        return '-'
    elif iteration == 1:
        return '--'
    elif iteration == 2:
        return ':'
    elif iteration == 3:
        return '-.'


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
    def label(self) -> str:
        return self.settings.get('label', self.pretty_name)
    
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
        logging.info(f'Retrieved and loaded {len(json_data)} json file(s)')
        return json_data

    def parse_problem_collection(self, json_data:Dict) -> ProblemCollection:
        problem_collection = ProblemCollection()
        for tuple in zip(self.inputs, json_data):
            problem_collection.add_model_collection(tuple[0], self.parse_json(tuple[1]))
        
        model_collection_count = 0
        model_count = 0
        method_count = 0
        propagation_mode_collection_count = 0
        instance_collection_count = 0
        instance_count = 0
        run_count = 0
        for model_collection in problem_collection.model_collections.values():
            model_collection_count += 1
            for model in model_collection.models.values():
                model_count += 1
                for method in model.methods.values():
                    method_count += 1
                    for propagation_mode_collection in method.propagation_mode_collections.values():
                        propagation_mode_collection_count += 1
                        for instance_collection in propagation_mode_collection.instance_collections.values():
                            instance_collection_count += 1
                            for instance in instance_collection.instances.values():
                                instance_count += 1
                                for run in instance.runs:
                                    run_count += 1
        logging.info('Parsed: ' + str.join('; ',
                                           [f'{c} {l}(s)' for c, l in [
                                             (model_collection_count, 'model collection'),
                                             (model_count, 'model'),
                                             (method_count, 'method'),
                                             (propagation_mode_collection_count,'propagation mode collection'),
                                             (instance_collection_count, 'instance collection'),
                                             (instance_count, 'instance'),
                                             (run_count, 'run')]
                                            ]))
        
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
        
        identifier = '/'.join(name_parts[1:2] + [str(a) for i, a in enumerate(arguments) if i not in {prop_mode_index, problem_instance_index}])

        probes_per_second = benchmark['probes_per_second']

        model = Model(model_name, settings)

        print(f"{model_name}\t{problem_instance}\t{int_to_propagation_mode(propagation_mode)}\t{probes_per_second}")

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
        plt.rcParams["figure.figsize"] = (10, 8)
        #plt.tight_layout(pad=0, h_pad=0, w_pad=0)
        subplots:List[Figure] = []
        for model_collection in sorted(self.problem_collection.model_collections.values()):
            plotted_models = set()
            for model in sorted(model_collection.models.values(), key=lambda a: a.name):
                if model in plotted_models:
                    continue
                models_to_compare = {model}
                for model_to_compare in self.settings.get(model.name, dict()).get('compare', dict()):
                    if model_to_compare in model_collection.models:
                        models_to_compare.add(model_collection.models[model_to_compare])
                if len(models_to_compare) == 1:
                    subplots.append(self.create_plot(model))
                else:
                    figure = self.plot_compare_models(models_to_compare)
                    plt.rcParams["figure.figsize"] = (10, 4)
                    plt.title(figure.title)
                    plt.xlabel(figure.xlabel)
                    plt.ylabel(figure.ylabel)
                    plt.yscale(figure.yscale)
                    for p in figure.plots:
                        plt.plot(
                            p.x_vals,
                            p.y_vals,
                            label=p.label,
                            marker=p.marker,
                            linestyle=p.linestyle
                        )
                    plt.legend(ncol=2)
                    plt.show()
                    
                    
                plotted_models = plotted_models.union(models_to_compare)
        plt.rcParams["figure.figsize"] = (10, 8)
        cols = 2
        rows = int(ceil(len(subplots) / 2))
        
        fig, axes = plt.subplots(rows, cols)
        
        axes = axes.flatten()
        
        lines_labels = set()
        
        for i, fig in enumerate(subplots):

            axes[i].set_title(fig.title)
            axes[i].set_xlabel(fig.xlabel)
            axes[i].set_ylabel(fig.ylabel)
            axes[i].set_yscale(fig.yscale)
            for p in fig.plots:
                axes[i].plot(
                    p.x_vals,
                    p.y_vals,
                    label=p.label,
                    marker=p.marker,
                    linestyle=p.linestyle
                )

            axes[i].set_xticks(fig.pretty_xticks)
        plt.subplots_adjust(
          left=0.06,
          right=0.98,
          bottom=0.06,
          top=0.94,
          wspace=0.15,
          hspace=0.38
        )
        
        seen_labels = set()
        lines = []
        labels = []
        
        for ax in axes:
            li, la = ax.get_legend_handles_labels()
            for i, l in enumerate(la):
                if l not in seen_labels:
                    seen_labels.add(l)
                    lines.append(li[i])
                    labels.append(l)
              
        plt.legend(lines, labels, ncol=2)
        
        plt.show()
        
    
    def plot_compare_models(self, models_to_compare:Set[Model]):
        initial_model = next(iter(models_to_compare))
        logging.info(f"comparing models")
        logging.info(initial_model.methods)
        for method_name, method in initial_model.methods.items():
            logging.info(method.name)
            for identifier, propagation_mode_collection in method.propagation_mode_collections.items():
                logging.info(propagation_mode_collection.identifier)
                argument_labels = propagation_mode_collection.argument_labels
                logging.info(str.join(' - ', [initial_model.name, method.name, str(propagation_mode_collection.identifier)]))
                logging.info(argument_labels)
                title_entries = [a if t is None else f'{t}: {a}' for t, a in argument_labels]
                title = ' - '.join(map(str, [initial_model.pretty_name, method.pretty_name] + title_entries[0:-1]))
                xlabel = argument_labels[-1][0] if len(title_entries) > 0 and argument_labels[-1][0] is not None else 'n'
                ylabel = 'probes/s'
                xticks = set()

                pretty_method_name = method.pretty_name

                plots = []
                
                for i, model in enumerate(models_to_compare):
                    method = next((m for m in model.methods.values() if method_name == m.name or pretty_method_name == m.pretty_name), None)
                    
                    if method is None:
                        continue
                    
                    logging.info(f"identifier: {identifier}")
                    logging.info(f"identifiers: {list(method.propagation_mode_collections.keys())}")
                    
                    for propagation_mode, instance_collection in method.propagation_mode_collections[identifier].instance_collections.items():
                        x_vals, y_vals = instance_collection.results()
                        xticks.update(x_vals)
                        plots.append(Plot(
                            x_vals,
                            y_vals,
                            label=f'{model.label} - {int_to_propagation_mode(propagation_mode)}',
                            marker=int_to_marker(propagation_mode),
                            linestyle=iteration_to_line_style(i)
                        ))
                
                return Figure(title,
                              xlabel,
                              ylabel,
                              initial_model.yscale,
                              list(sorted(xticks)),
                              '-'.join([model.name] + propagation_mode_collection.identifier.split('/')),
                              plots)
    
    def create_plot(self, model:Model) -> Figure:
        logging.info(model.name)
        for method in model.methods.values():
            logging.info(method.name)
            for propagation_mode_collection in method.propagation_mode_collections.values():
                logging.info(propagation_mode_collection.identifier)
                argument_labels = propagation_mode_collection.argument_labels
                logging.info(str.join(' - ', [model.name, method.name, str(propagation_mode_collection.identifier)]))
                logging.info(argument_labels)
                title_entries = [a if t is None else f'{t}: {a}' for t, a in argument_labels]
                title = model.pretty_name #' - '.join(map(str, [model.pretty_name, method.pretty_name] + title_entries[0:-1]))
                xlabel = argument_labels[-1][0] if len(title_entries) > 0 and argument_labels[-1][0] is not None else 'n'
                
                ylabel = 'probes/s'
                
                xticks = set()

                plots = []
                for propagation_mode, instance_collection in propagation_mode_collection.instance_collections.items():
                    x_vals, y_vals = instance_collection.results()
                    xticks.update(x_vals)
                    plots.append(Plot(
                        x_vals,
                        y_vals,
                        int_to_propagation_mode(propagation_mode),
                        int_to_marker(propagation_mode),
                        iteration_to_line_style(0)
                    ))
                return Figure(title,
                              xlabel,
                              ylabel,
                              model.yscale,
                              list(sorted(xticks)),
                              '-'.join([model.name] + propagation_mode_collection.identifier.split('/')),
                              plots)

    def save_plot(self, file_name):
        if not self.save_plots:
            return False
        plot_filename = path.join(
            self.output_dir,
            f'{self.file_prefix}{file_name}{self.file_suffix}.png'
        )
        plt.savefig(plot_filename, bbox_inches=0, transparant=True, pad_inches=0)
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
    parser.add_argument('--settings', dest='settings', default=path.join(path.dirname(path.realpath(__file__)), 'plot-formatter.json'))
    parser.add_argument('-v', '--verbose', dest='verbose', default=True, action="store_true")
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.WARNING)

    plot_formatter = PlotFormatter(
      args.inputs,
      args.save_plots,
      args.output_dir,
      args.settings,
      args.file_prefix,
      args.file_suffix
    )
    plot_formatter.plot_model_collection()