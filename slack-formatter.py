import re
import logging
from sys import argv, stdin
from os import path
from urllib import request

class SlackFormatter:
    def __init__(self, text, arguments):
        self.arguments = arguments
        self.logger = logging.getLogger('SlackFormatter')
        self.lines = text.splitlines()
        header, lines = self.retrieve_header(self.lines)
        self.header = header
        self.benchmarks = self.split_into_benchmarks(lines)

    def retrieve_header(self, lines):
        re_devider = re.compile(r'^\s*(-+)\s*$')
        header = []
        start_line = -1
        end_line = -1
        for line_nr, line in enumerate(lines):
            match = re_devider.match(line)
            if match is None:
                if start_line >= 0:
                    header.append(line)
                continue
            
            header.append(match.group(1))
            if start_line < 0:
                start_line = line_nr
                continue
            
            end_line = line_nr
            break
        return header, lines[end_line+1:]

    def split_into_benchmarks(self, lines):
        benchmark_dict = {}
        benchmark_order = []
        re_name = re.compile(r'^(\w+)')
        for line in lines:
            match = re_name.match(line)
            if match is None:
                continue
            line_name = match.group(0)
            if line_name in benchmark_dict:
                benchmark_dict[line_name].append(line)
            else:
                benchmark_dict[line_name] = [line]
                benchmark_order.append(line_name)
        return ['\n'.join(benchmark_dict[n]) for n in benchmark_order]
    
    def combine(self, header, benchmarks):
        return [
            '\n'.join(header) + '\n' + '\n'.join(benchmark)
            for benchmark in benchmarks
        ]

    def send_to_slack(self):
        for i, benchmark in enumerate(self.benchmarks):
            if self.arguments.get('header', None) is not None and i == 0:
                benchmark = (
                    self.arguments['header'] +
                    f"\n```" + '\n'.join(self.header) + f"\n{benchmark}```"
                )
            else:
                benchmark = f"```{benchmark}```"
            json = f'{{"text":"{benchmark}"}}'
            self.logger.debug(benchmark)
            data = json.encode('utf-8')
            req = request.Request(
                url=self.arguments.get('webhook'),
                headers={'content-type': 'application/json'},
                data=data
            )
            response = request.urlopen(req)
            self.logger.info(response.read().decode('utf-8'))

if __name__ == "__main__":
    logging.basicConfig(level=logging.WARNING)
    flags = ["header", "output", "webhook"]
    if not stdin.isatty():
        text = stdin.read()
    else:
        exit(0)
    
    arguments = {f:None for f in flags}
    for f in flags:
        flag = f'--{f}='
        argument = next((n[len(flag):] for n in argv if n.startswith(flag)), None)
        arguments[f] = argument

    if arguments['output'] is not None:
        arguments['output'] = path.realpath(arguments['output'])

    slack_formatter = SlackFormatter(text, arguments)
    slack_formatter.send_to_slack()    
