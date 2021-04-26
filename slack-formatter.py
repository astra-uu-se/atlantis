import re
import logging
from sys import argv, stdin
from os import path
from urllib import request

class SlackFormatter:
    def __init__(self, text, arguments):
        self.arguments = arguments
        self.logger = logging.getLogger('SlackFormatter')
        self.lines = self.trim_lines(text.splitlines())
        header, lines = self.retrieve_header(self.lines)
        self.header = header
        self.message_padding = 3
        self.max_message_length = 3850 - self.message_padding
        self.messages = self.split_into_messages(lines)

        self.benchmarks = self.split_into_benchmarks(lines)

    def trim_lines(self, lines):
        return [l.rstrip() for l in lines if len(l.strip()) > 0]

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

    def split_into_messages(self, lines):
        messages = []
        if self.arguments.get('header', None) is not None:
            cur_message = self.arguments['header'] + "\n```" + '\n'.join(self.header)
        else:
            cur_message = '```'

        for line in lines:

            if len(cur_message) + len(line) >= self.max_message_length:
                messages.append(cur_message + "```")
                cur_message = "```" + line
            else:
                cur_message += '\n' + line
            
        
        if len(lines) > 0:
            messages.append(cur_message + '```')
        
        return messages

    def send_messages_to_slack(self):
        for message in self.messages:
            json = '{"text":"' + message + '"}'
            self.logger.debug(message)
            data = json.encode('utf-8')
            if self.arguments.get('webhook', None) is None:
                continue
            req = request.Request(
                url=self.arguments.get('webhook'),
                headers={'content-type': 'application/json'},
                data=data
            )
            response = request.urlopen(req)
            self.logger.info(response.read().decode('utf-8'))

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

    def send_to_slack(self):
        for i, benchmark in enumerate(self.benchmarks):
            if self.arguments.get('header', None) is not None and i == 0:
                benchmark = (
                    self.arguments['header'] + "\n```" + '\n'.join(self.header) + "\n" + benchmark + "```"
                )
            else:
                benchmark = "```" + benchmark + "```"
            json = '{"text":"' + benchmark + '"}'
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
    flags = ["header", "webhook"]
    if not stdin.isatty():
        text = stdin.read()
    else:
        exit(0)
    
    arguments = {f:None for f in flags}
    for f in flags:
        flag = '--' + f +'='
        argument = next((n[len(flag):] for n in argv if n.startswith(flag)), None)
        arguments[f] = argument
    slack_formatter = SlackFormatter(text, arguments)
    slack_formatter.send_messages_to_slack()