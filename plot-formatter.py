from fileinput import filename
import logging
from os import path, access, R_OK
from re import X
import matplotlib
import matplotlib.pyplot as plt
import json
import argparse
from typing import *
from math import ceil
from pprint import pprint
matplotlib.use('tkagg')


def int_to_propagation_mode(value: int) -> str:
    if value == 0:
        return 'i2o'
    elif value == 1:
        return 'o2i'
    elif value == 2:
        return 'o2i - static'
    elif value == 3:
        return 'o2i - ad-hoc'
    return ''


def iteration_to_line_style(iteration: int) -> str:
    if iteration == 0:
        return '-'
    elif iteration == 1:
        return '--'
    elif iteration == 2:
        return ':'
    elif iteration == 3:
        return '-.'


def int_to_marker(value: int) -> str:
    if value == 0:
        return 'o'
    elif value == 1:
        return 'v'
    elif value == 2:
        return '*'
    elif value == 3:
        return '+'
    return ''


class Plot:
    x_vals: List[int]
    y_vals: List[float]
    label: str
    marker: str
    linestyle: str

    def __init__(self, x_vals: List[int], y_vals: List[float], label: str,
                 marker: str, linestyle: str):
        self.x_vals: List[int] = x_vals
        self.y_vals: List[float] = y_vals
        self.label: str = label
        self.marker: str = marker
        self.linestyle: str = linestyle


def get_xticks(plots: Iterable[Plot]) -> List[str]:
    xticks = set()
    for p in plots:
        xticks.update(p.x_vals)
    return list(sorted(xticks))


class Figure:
    title: str
    xlabel: str
    ylabel: str
    yscale: str
    identifier: str
    show_y_values: bool
    plots: List[Plot]
    xticks: List[str]

    def __init__(self, title: str, xlabel: str, ylabel: str, yscale: str,
                 identifier: str, show_y_values: bool,
                 plots: List[Plot]) -> None:
        self.title: str = title
        self.xlabel: str = xlabel
        self.ylabel: str = ylabel
        self.yscale: str = yscale
        self.identifier: str = identifier
        self.show_y_values: bool = show_y_values
        self.plots: List[Plot] = plots
        self.xticks: List[int] = get_xticks(plots)

    def __iter__(self) -> Iterable[Plot]:
        return iter(self.plots)

    @property
    def pretty_xticks(self):
        if len(self.xticks) == 0:
            return []
        first = int(self.xticks[0])
        last = int(self.xticks[-1])
        res = [first]
        min_dist = (last - first) / 10
        for xt in self.xticks[1:]:
            if int(xt) - res[-1] >= min_dist:
                res.append(xt)
        return res


class Instance:
    problem_instance: int
    runs: List[float]
    settings: Dict[str, Any]

    def __init__(self, problem_instance: int, settings: Dict[str, Any]):
        self.problem_instance: int = problem_instance
        self.runs: List[float] = []
        self.settings: Dict[str, Any] = settings

    def __iter__(self) -> Iterable[float]:
        return iter(self.runs)

    @property
    def probes_per_second(self) -> float:
        if len(self.runs) == 1:
            return self.runs[0]
        return sum(self.runs) / len(self.runs)

    def add_instance(self, instance) -> None:
        assert self.problem_instance == instance.problem_instance
        self.runs = self.runs + instance.runs
        return self

    def add_run(self, probes_per_second: float) -> 'Instance':
        self.runs.append(probes_per_second)
        return self

    def add(self, arg: Any) -> 'Instance':
        if isinstance(arg, Instance):
            return self.add_instance(arg)
        if isinstance(arg, float):
            self.runs.append(arg)
        return self

    def overlaps(self, instance) -> bool:
        return self.problem_instance == instance.problem_instance


class InstanceCollection:
    propagation_mode: int
    arguments: List[int]
    instances: Dict[int, Instance]
    settings: Dict[str, Any]
    argument_labels: List[str]

    def __init__(self, propagation_mode: int, arguments: List[int],
                 settings: Dict[str, Any]):
        self.propagation_mode: int = propagation_mode
        self.arguments: List[int] = arguments
        self.instances: Dict[int, Instance] = dict()
        self.settings: Dict[str, Any] = settings

        self.argument_labels: List[Tuple[str, int]] = []
        argument_order = settings.get('argument_order', [])

        append_prop_mode = "PROPAGATION_MODE" not in argument_order
        for i in range(len(argument_order),
                       len(arguments) - (1 if append_prop_mode else 0)):
            argument_order.append(None)

        if append_prop_mode:
            argument_order.append("PROPAGATION_MODE")

        for i in range(len(self.arguments)):
            if len(argument_order) <= i:
                self.argument_labels.append((None, self.arguments[i]))
            elif argument_order[i] in {"IGNORE", "PROPAGATION_MODE"}:
                continue
            else:
                self.argument_labels.append(
                  (argument_order[i], self.arguments[i]))

    def __iter__(self) -> Iterable[Instance]:
        for instance in self.instances.values():
            yield instance

    @property
    def propagation_modes(self) -> List[Instance]:
        return list(sorted(self.instances.keys()))

    def add_instance(self, instance: Instance) -> 'InstanceCollection':
        if instance.problem_instance not in self.instances:
            self.instances[instance.problem_instance] = instance
        else:
            self.instances[instance.problem_instance].add_instance(instance)
        return self

    def create_instance(self, problem_instance: int) -> Instance:
        instance = Instance(problem_instance, self.settings)

        if problem_instance not in self.instances:
            self.instances[problem_instance] = instance
        else:
            self.instances[problem_instance].add_instance(instance)

        return instance

    def add_instance_collection(self,
                                instance: Instance) -> 'InstanceCollection':
        assert self.propagation_mode == instance.propagation_mode

        for instance in instance.instances.values():
            self.add_instance(instance)
        return self

    def add(self, obj) -> 'InstanceCollection':
        if isinstance(obj, Instance):
            return self.add_instance(obj)
        if isinstance(obj, InstanceCollection):
            return self.add_instance_collection(obj)
        return self

    def results(self) -> Tuple[List[str], List[float]]:
        problem_instance_names = list(sorted(self.instances.keys()))
        return (problem_instance_names,
                [self.instances[problem_instance_name].probes_per_second
                 for problem_instance_name in problem_instance_names])

    def orverlaps(self, instance_collection) -> bool:
        if self.propagation_mode != instance_collection.propagation_mode:
            return False
        intersection = set(self.instances.keys).intersection(
          instance_collection.instances.keys())

        return any((self.instances[i].overlaps(
          instance_collection.instances[i]) for i in intersection))


class PropagationModeCollection:
    identifier: str
    instance_collections: Dict[int, InstanceCollection]
    settings: Dict[str, Any]

    def __init__(self, identifier, settings: Dict[str, Any]):
        self.identifier: str = identifier
        self.instance_collections: Dict[int, InstanceCollection] = dict()
        self.settings: Dict[str, Any] = settings

    def __iter__(self) -> Iterable[InstanceCollection]:
        for ic in self.instance_collections.values():
            yield ic

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

    @property
    def xlabel(self) -> str:
        al = self.argument_labels
        if len(al) > 0 and al[-1][0] is not None:
            return self.argument_labels[-1][0]
        return 'n'

    def add_instance_collection(self, ic: InstanceCollection):
        if ic.propagation_mode not in self.instance_collections:
            self.instance_collections[ic.propagation_mode] = ic
        else:
            self.instance_collections[ic.propagation_mode].\
              add_instance_collection(ic)

        return self

    def create_instance_collection(self, propagation_mode: int,
                                   arguments: List[int]) -> InstanceCollection:
        ic = InstanceCollection(propagation_mode, arguments, self.settings)
        self.add_instance_collection(ic)
        return ic

    def add_propagation_mode_collection(self, propagation_mode_collection
                                        ) -> 'PropagationModeCollection':
        assert self.identifier == propagation_mode_collection.identifier

        for ic in propagation_mode_collection.instance_collections.values():
            self.add_instance_collection(ic)
        return self

    def add(self, obj) -> 'PropagationModeCollection':
        if isinstance(obj, InstanceCollection):
            return self.add_instance_collection(obj)
        if isinstance(obj, PropagationModeCollection):
            return self.add_propagation_mode_collection(obj)
        return self

    def orverlaps(self, propagation_mode_collection) -> bool:
        if self.identifier != propagation_mode_collection.identifier:
            return False

        intersection = set(
          self.instance_collections.keys).intersection(
            propagation_mode_collection.instance_collections.keys())

        return any(
          (self.instances[pm].overlaps(
            propagation_mode_collection.instance_collections[pm])
           for pm in intersection))

    def create_plots(self, linestyle: str,
                     label_parts: List[str] = []) -> List[Plot]:
        plots: List[Plot] = []
        for ic in self:
            x_vals, y_vals = ic.results()
            label = ' - '.join(
              label_parts + [int_to_propagation_mode(ic.propagation_mode)])

            plots.append(Plot(
                x_vals,
                y_vals,
                label=label,
                marker=int_to_marker(ic.propagation_mode),
                linestyle=linestyle
            ))
        return plots


class Method:
    name: str
    propagation_mode_collections: Dict[str, PropagationModeCollection]
    settings: Dict[str, Any]

    def __init__(self, name: str, settings: Dict[str, Any]):
        self.name: str = name
        self.propagation_mode_collections = dict()
        self.settings: Dict[str, Any] = settings

    def __iter__(self) -> Iterable[PropagationModeCollection]:
        for pmc in self.propagation_mode_collections.values():
            yield pmc

    @property
    def pretty_name(self) -> str:
        return self.settings.get(
          'method_name', dict()).get(self.name, self.name)

    @property
    def argument_labels(self) -> List[str]:
        for pmc in self.propagation_mode_collections.values():
            if len(pmc.argument_labels) > 0:
                return pmc.argument_labels
        return []

    def add_propagation_mode_collection(self, pmc: PropagationModeCollection
                                        ) -> 'Method':
        if pmc.identifier not in self.propagation_mode_collections:
            self.propagation_mode_collections[pmc.identifier] = pmc
        else:
            self.propagation_mode_collections[pmc.identifier].\
              add_propagation_mode_collection(pmc)

        return self

    def create_propagation_mode_collection(self, identifier: str
                                           ) -> PropagationModeCollection:
        pmc = PropagationModeCollection(identifier, self.settings)
        if identifier not in self.propagation_mode_collections:
            self.propagation_mode_collections[identifier] = pmc
        else:
            self.propagation_mode_collections[identifier].\
              add_propagation_mode_collection(pmc)
        return pmc

    def add_method(self, method) -> 'Method':
        assert self.name == method.name

        for pmc in method.propagation_mode_collections.values():
            self.add_propagation_mode_collection(pmc)
        return self

    def add(self, obj) -> 'Method':
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

        intersection = set(
          self.propagation_mode_collections.keys).intersection(
            method.propagation_mode_collections.instance_collections.keys())
        return any(
          (self.propagation_mode_collections[i].overlaps(
            method.propagation_mode_collections[i])
           for i in intersection))

    def find_instance_collections(self, pmc: PropagationModeCollection
                                  ) -> Iterable[InstanceCollection]:
        return iter(
          self.propagation_mode_collections.get(pmc.identifier), list())


class Model:
    name: str
    methods: Dict[str, Method]
    settings: Dict[str, Any]

    def __init__(self, name, settings: Dict[str, Any]):
        self.name: str = name
        self.methods: Dict[str, Method] = dict()
        self.settings: Dict[str, Any] = settings

    def __iter__(self) -> Iterable[Method]:
        for method in self.methods.values():
            yield method

    @property
    def pretty_name(self) -> str:
        return self.settings.get('model_name', self.name)

    @property
    def label(self) -> str:
        return self.settings.get('label', self.pretty_name)

    @property
    def compare(self) -> List[str]:
        return self.settings.get('compare', list())

    @property
    def yscale(self) -> str:
        return self.settings.get('yscale', 'linear')

    @property
    def group(self) -> int:
        return self.settings.get('group', 0)

    @property
    def show_y_values(self) -> bool:
        return self.settings.get('show_y_values', False)

    @property
    def argument_labels(self) -> List[str]:
        for method in self.methods.values():
            if len(method.argument_labels) > 0:
                return method.argument_labels
        return []

    def add_method(self, method: Method) -> 'Model':
        if method.name not in self.methods:
            self.methods[method.name] = method
        else:
            self.methods[method.name].add_method(method)

        return self

    def create_method(self, method_name: str) -> Method:
        method = Method(method_name, self.settings)
        if method_name not in self.methods:
            self.methods[method_name] = method
        else:
            self.methods[method_name].add_method(method)
        return method

    def add_model(self, model) -> 'Model':
        assert self.name == model.name
        for method in model.methods.values():
            self.add_method(method)
        return self

    def add(self, obj) -> 'Model':
        if isinstance(obj, Method):
            return self.add_method(obj)
        if isinstance(obj, Model):
            return self.add_model(obj)
        return self

    def orverlaps(self, model) -> bool:
        if self.name != model.name:
            return False

        intersection = set(
          self.methods.keys).intersection(model.methods.keys())
        return any(
          (self.methods[n].overlaps(model.methods[n]) for n in intersection))

    def find_method(self, method: Method) -> Union[Method, None]:
        return next(
          (met for met in self if
           method.name == met.name or method.pretty_name == met.pretty_name),
          None)


class ModelCollection:
    models: Dict[str, Model]

    def __init__(self):
        self.models = dict()

    def __iter__(self) -> Iterable[Model]:
        for model in self.models.values():
            yield model

    def __contain__(self, model_name: str) -> bool:
        return model_name in self.models

    def add_model(self, model: Model):
        if model.name not in self.models:
            self.models[model.name] = model
        else:
            self.models[model.name].add_model(model)
        return self

    def add_model_collection(self, model_collection) -> 'ModelCollection':
        for model in model_collection:
            self.add_model(model)
        return self

    def add(self, obj) -> 'ModelCollection':
        if isinstance(obj, Model):
            return self.add_model(obj)
        if isinstance(obj, ModelCollection):
            return self.add_model_collection(obj)
        return self

    def orverlaps(self, model_collection) -> bool:
        intersection = set(
          self.models.keys).intersection(model_collection.models.keys())
        return any(
          (self.models[n].overlaps(
            model_collection.models[n])
           for n in intersection))

    def models_to_compare(self, model: Model) -> Set[Model]:
        res = {model}
        for m_to_compare in model.compare:
            if m_to_compare in self:
                res.add(
                  self.models[m_to_compare])
        return res


class ProblemCollection:
    model_collections: Dict[str, ModelCollection]

    def __init__(self):
        self.model_collections = dict()

    def __iter__(self) -> Iterable[ModelCollection]:
        for mc in self.model_collections.values():
            yield mc

    def add_model_collection(self, file_name, model_collection
                             ) -> 'ProblemCollection':
        if file_name not in self.model_collections:
            self.model_collections[file_name] = model_collection
        else:
            self.model_collections[file_name].add_model_collection(
              model_collection)
        return self


def get_title(model: Model, method: Method,
              pmc: PropagationModeCollection) -> str:
    title_entries = [a if t is None else f'{t}: {a}'
                     for t, a in pmc.argument_labels[0:-1]]
    return ' - '.join(map(str,
                          [model.pretty_name, method.pretty_name] +
                          title_entries))


class PlotFormatter:
    inputs: List[str] = list()
    output_dir: Union[str, None]
    save_plots: bool
    settings: Dict[str, Any]
    file_prefix: str
    file_suffix: str
    problem_collection: ProblemCollection

    def __init__(self, inputs: List[str],
                 save_plots: bool, output_dir: str, settings: str,
                 file_prefix: str = '', file_suffix: str = ''):
        self.inputs: List[str] = inputs
        self.output_dir: Union[str, None] = output_dir
        self.save_plots: bool = save_plots
        self.settings: Dict[str, Any] = self.process_settings(settings)
        self.file_prefix: str = (
          f'{file_prefix}-' if len(file_prefix) > 0 else '')
        self.file_suffix: str = (
          f'-{file_suffix}' if len(file_suffix) > 0 else '')
        json_data = self.retrieve_json()
        self.problem_collection = self.parse_problem_collection(json_data)

    def process_settings(self, settings_path) -> Dict[str, Any]:
        with open(settings_path, 'r') as settings_file:
            return json.load(settings_file)

    def retrieve_json(self) -> Dict[str, Any]:
        json_data = []
        for json_file_path in self.inputs:
            with open(json_file_path, 'r') as json_file:
                json_data.append(json.load(json_file))
        logging.info(f'Retrieved and loaded {len(json_data)} json file(s)')
        return json_data

    def parse_problem_collection(self, json_data: Dict) -> ProblemCollection:
        problem_collection = ProblemCollection()
        for tuple in zip(self.inputs, json_data):
            problem_collection.add_model_collection(tuple[0],
                                                    self.parse_json(tuple[1]))

        model_collection_count = 0
        model_count = 0
        method_count = 0
        propagation_mode_collection_count = 0
        instance_collection_count = 0
        instance_count = 0
        run_count = 0
        for model_collection in problem_collection:
            model_collection_count += 1
            for model in model_collection:
                model_count += 1
                for method in model:
                    method_count += 1
                    for pmc in method:
                        propagation_mode_collection_count += 1
                        for instance_collection in pmc:
                            instance_collection_count += 1
                            for instance in instance_collection:
                                instance_count += 1
                                run_count += len(instance.runs)
        logging.info(
          'Parsed: ' +
          str.join('; ',
                   [f'{c} {l}(s)' for c, l in [
                     (model_collection_count,
                      'model collection'),
                     (model_count,
                      'model'),
                     (method_count,
                      'method'),
                     (propagation_mode_collection_count,
                      'propagation mode collection'),
                     (instance_collection_count,
                      'instance collection'),
                     (instance_count,
                      'instance'),
                     (run_count,
                      'run')]
                    ]))

        return problem_collection

    def filter_json_benchmarks(self, json_instance) -> List[Dict[str, Any]]:
        benchmarks = json_instance.get('benchmarks', [])

        filtered_benchmarks: List[Dict[str, Any]] = list()

        for benchmark in benchmarks:
            if 'run_name' not in benchmark:
                continue

            run_name = benchmark['run_name']

            if benchmark.get('run_type', '') == 'aggregate':
                if benchmark.get('aggregate_name', '') != 'mean':
                    continue
            elif any((b.get('aggregate_name', '') == 'mean'
                      for b in benchmarks if
                      b.get('run_name', '') == run_name)):
                continue

            if benchmark.get('probes_per_second', None) is None:
                continue

            if len(run_name.split('/')) < 3:
                continue

            filtered_benchmarks.append(benchmark)

        return filtered_benchmarks

    def parse_json_benchmark(self, benchmark: Dict[str, Any]) -> Model:
        name_parts: str = benchmark['run_name'].split('/')
        model_name: str = name_parts[0]
        method_name: str = name_parts[1]

        arguments: List[str] = [int(a) for a in name_parts[2:]]

        settings = self.settings.get(model_name, dict())

        if settings.get('ignore', False):
            return None

        prop_mode_index = next(
          (i for i, val in enumerate(settings.get('agument_order', []))
           if val == 'PROPAGATION_MODE'), len(arguments) - 1
        )

        propagation_mode = arguments[prop_mode_index]

        problem_instance_index = max(
          0,
          len(arguments) - (2 if prop_mode_index == len(arguments) - 1 else 1)
        )

        problem_instance = arguments[problem_instance_index]

        identifier = '/'.join(
          name_parts[1:2] + [str(a) for i, a in enumerate(arguments)
                             if i not in {prop_mode_index,
                                          problem_instance_index}])

        probes_per_second = benchmark['probes_per_second']

        model = Model(model_name, settings)

        print('\t'.join([model_name,
                         str(problem_instance),
                         int_to_propagation_mode(propagation_mode),
                         str(probes_per_second)]))

        model.create_method(method_name)\
             .create_propagation_mode_collection(identifier)\
             .create_instance_collection(propagation_mode, arguments)\
             .create_instance(problem_instance)\
             .add_run(probes_per_second)

        return model

    def parse_json(self, json_instance: Dict[str, Any]) -> ModelCollection:
        model_collection = ModelCollection()
        for benchmark in self.filter_json_benchmarks(json_instance):
            model = self.parse_json_benchmark(benchmark)
            if model is not None:
                model_collection.add_model(model)

        return model_collection

    def merge_model_collections(self, model_collections: ModelCollection
                                ) -> ModelCollection:
        merged_model_collection = None

        for model_collection in model_collections:
            if merged_model_collection is None:
                merged_model_collection = model_collection
            else:
                merged_model_collection.add_model_collection(model_collection)

        return merged_model_collection

    def plot_comparing_models(self, models_to_compare: Set[Model]) -> None:
        figure = self.plot_compare_models(models_to_compare)
        plt.rcParams["figure.figsize"] = (10, 4)
        plt.title(figure.title)
        plt.xlabel(figure.xlabel, fontsize=10)
        plt.ylabel(figure.ylabel)
        plt.yscale(figure.yscale)
        for plot in figure:
            plt.plot(
                plot.x_vals,
                plot.y_vals,
                label=plot.label,
                marker=plot.marker,
                linestyle=plot.linestyle
            )
        plt.legend(ncol=2)
        plt.show()

    def plot_subplots(self, subplots: List[Figure]) -> None:
        cols = 1 if len(subplots) < 2 else 2
        rows = int(ceil(len(subplots) / 2))

        fig, axes = plt.subplots(rows, cols, figsize=(10, 8))

        flat = [axes] if len(subplots) == 1 else axes.flat

        for i, figure in enumerate(subplots):
            flat[i].set_title(figure.title)
            flat[i].set_xlabel(figure.xlabel)
            flat[i].set_ylabel(figure.ylabel)
            flat[i].set_yscale(figure.yscale)
            for plot in figure:
                flat[i].plot(
                    plot.x_vals,
                    plot.y_vals,
                    label=plot.label,
                    marker=plot.marker,
                    linestyle=plot.linestyle
                )
                if not figure.show_y_values:
                    continue
                for x, y in zip(plot.x_vals, plot.y_vals):
                    flat[i].text(x, y, str(int(y)), ha='center')
                # flat[i].xaxis.set_tick_params(labelsize=9)
                # flat[i].yaxis.set_tick_params(labelsize=9)

            flat[i].set_xticks(figure.pretty_xticks)

        plt.subplots_adjust(
          left=0.06,
          right=0.999,
          bottom=0.06,
          top=0.92,
          wspace=0.15,
          hspace=0.43)

        seen_labels = set()
        handles_labels = []

        for ax in flat:
            ha, la = ax.get_legend_handles_labels()
            for handle, label in zip(ha, la):
                if label not in seen_labels:
                    handles_labels.append((handle, label))
                    seen_labels.add(label)

        labels = [l for _, l in sorted(handles_labels, key=lambda hl: hl[1])]
        handles = [h for h, _ in sorted(handles_labels, key=lambda hl: hl[1])]

        fig.legend(labels=labels, handles=handles, ncol=len(labels),
                   loc='upper center')

        plt.show()

    def plot_model_collection(self) -> None:
        plt.rcParams["figure.figsize"] = (10, 8)
        # plt.tight_layout(pad=0, h_pad=0, w_pad=0)
        subplots: List[Tuple[Tuple[int, str], Figure]] = []
        for model_collection in self.problem_collection:
            plotted_models = set()
            for model in model_collection:
                if model in plotted_models:
                    continue
                models_to_compare = model_collection.models_to_compare(model)
                if len(models_to_compare) == 1:
                    subplots.append(
                      ((model.group, model.name), self.create_plot(model)))
                else:
                    self.plot_comparing_models(models_to_compare)
                plotted_models = plotted_models.union(models_to_compare)
        subplots = [f for (_, f) in sorted(subplots)]
        self.plot_subplots(subplots)

    def plot_compare_with_other_models(self, initial_model: Model,
                                       models_to_compare: Set[Model],
                                       method: Method,
                                       pmc: PropagationModeCollection
                                       ) -> None:
        logging.info(pmc.identifier)
        logging.info(' - '.join([initial_model.name,
                                 method.name,
                                 str(pmc.identifier)]))
        logging.info(pmc.argument_labels)

        title = get_title(initial_model, method, pmc)
        xlabel = pmc.xlabel
        ylabel = 'probes/s'

        plots: List[Plot] = []

        for i, model in enumerate(models_to_compare):
            c_method = model.find_method(method)

            if c_method is None:
                continue

            logging.info(f"identifier: {pmc.identifier}")
            logging.info(
              "identifiers: " +
              list(c_method.propagation_mode_collections.keys()))

            plots.extend(
              c_method.find_insance_collection(pmc).create_plots(
                iteration_to_line_style(i),
                [model.label]))

        identifier = '-'.join([model.name] + pmc.identifier.split('/'))

        return Figure(title,
                      xlabel,
                      ylabel,
                      initial_model.yscale,
                      identifier,
                      model.show_y_values,
                      plots)

    def plot_compare_models(self, models_to_compare: Set[Model]) -> None:
        if len(models_to_compare) == 0:
            return
        initial_model = next(iter(models_to_compare))
        logging.info(f"comparing models")
        logging.info(initial_model.methods)
        for method in initial_model:
            logging.info(method.name)
            return self.plot_compare_with_other_models(
              initial_model, models_to_compare, method, next(iter(method)))

    def create_plot_for_prop_mode_collection(self, model: Model,
                                             method: Method,
                                             pmc: PropagationModeCollection
                                             ) -> Figure:
        logging.info(pmc.identifier)
        logging.info(str.join(' - ', [method.name, method.name,
                                      pmc.identifier]))
        logging.info(pmc.argument_labels)
        title = model.pretty_name

        xlabel = pmc.xlabel
        ylabel = 'probes/s'

        plots = pmc.create_plots(iteration_to_line_style(0))

        identifier = '-'.join([method.name] + pmc.identifier.split('/'))

        return Figure(title,
                      xlabel,
                      ylabel,
                      model.yscale,
                      identifier,
                      model.show_y_values,
                      plots)

    def create_plot(self, model: Model) -> Figure:
        logging.info(model.name)
        for method in model.methods.values():
            logging.info(method.name)
            return self.create_plot_for_prop_mode_collection(
              model, method, next(iter(method), None))

    def save_plot(self, file_name):
        if not self.save_plots:
            return False
        plot_filename = path.join(
            self.output_dir,
            f'{self.file_prefix}{file_name}{self.file_suffix}.png'
        )
        plt.savefig(plot_filename,
                    bbox_inches=0,
                    transparant=True,
                    pad_inches=0)
        return True


if __name__ == "__main__":
    class readable_dir(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
            prospective_dir = values
            if not path.isdir(prospective_dir):
                raise argparse.ArgumentTypeError(
                  f"readable_dir:{prospective_dir} is not a valid path")
            if access(prospective_dir, R_OK):
                setattr(namespace, self.dest, prospective_dir)
            else:
                raise argparse.ArgumentTypeError(
                  f"readable_dir:{prospective_dir} is not a readable dir")

    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir', dest='output_dir', action=readable_dir,
                        default='plots')
    parser.add_argument('--save-plots', dest='save_plots', default=False,
                        action="store_true")
    parser.add_argument('--file-prefix', type=str, dest='file_prefix',
                        default='')
    parser.add_argument('--file-suffix', type=str, dest='file_suffix',
                        default='')
    parser.add_argument('--input', dest='inputs', nargs='+',
                        default=[
                          'benchmark-json/2022-11-18-16-07-11-103.json'])
    parser.add_argument('--settings', dest='settings',
                        default=path.join(
                          path.dirname(path.realpath(__file__)),
                          'plot-formatter.json'))
    parser.add_argument('-v', '--verbose', dest='verbose', default=True,
                        action="store_true")
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
