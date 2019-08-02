#! /usr/bin/env python3
import argparse

# This is the equivalent of MaxTick in gem5, which is MAX_INT of 64-bit
# Here we assume the Tick's are 64-bit integers
# Should be changed if the Tick's are 32-bit integers
# MAX_INT (64-bit) = 2**64 - 1
# MAX_INT (32-bit) = 2**32 - 1
# MAX_TICK is only used for parsing the input
MAX_TICK = 2**64 - 1
# INVALID_TICK is the internal representation for Tick's that are not recorded
INVALID_TICK = -1

bg_color_map = {
    "red": "\033[41m",
    "green": "\033[42m",
    "yellow": "\033[43m",
    "blue": "\033[44m",
    "magenta": "\033[45m",
    "cyan": "\033[46m",
    "white": "\033[47m",
    "bright_black": "\033[100m",
    "bright_red": "\033[101m",
    "bright_green": "\033[102m",
    "bright_yellow": "\033[103m",
    "bright_blue": "\033[104m",
    "bright_magenta": "\033[105m",
    "bright_cyan": "\033[106m",
    "bright_white": "\033[107m",
    "black": "\033[0m"
}

stage_color_map = {
    "f": "blue",
    "d": "yellow",
    "i": "red",
    "e": "cyan",
    "a": "cyan",
    "m": "cyan",
    "z": "green",
    "c": "blue",
    "s": "magenta",
    "\n": "black"
}

# f: creationTick (inst created)
# d: decodeTick (inst decoded)
# i: issueTick (issued)
# e: beginExecuteTick (execute request sent)
# a: effectiveAddressTick (address available)
# m: beginMemoryTick (memory access request sent)
# z: completionTick (execute stage is done)
# c: commitTick (inst committed)
# s: squashTick (inst squashed)

stages = ["f", "d", "i", "e", "a", "m", "z", "c", "s"]

stages_fullname = ["fetch", "decode", "issue", "begin execute", "effAddress",
                   "begin memory", "complete", "commit", "squash"]

bg_color_char_lambda = lambda color, char : bg_color_map[color] + char \
                                            + bg_color_map["black"]

max_tick_to_invalid_tick_lambda = lambda tick: tick if tick != MAX_TICK \
                                                    else INVALID_TICK

def colorize_pipeline(pipeline, start_pos, end_pos, bg_coloring_lambda, wrap):
    curr_color = stage_color_map[pipeline[start_pos]]
    if wrap:
        i = start_pos
        width = len(pipeline)
        end_pos = end_pos % width
        while not i == end_pos:
            if pipeline[i].isalpha():
                curr_color = stage_color_map[pipeline[i]]
            pipeline[i] = bg_coloring_lambda(curr_color, pipeline[i])
            i = (i + 1) % width
    else:
        for i in range(start_pos, end_pos):
            if pipeline[i].isalpha():
                curr_color = stage_color_map[pipeline[i]]
            pipeline[i] = bg_coloring_lambda(curr_color, pipeline[i])
    return pipeline

def no_colorize_pipeline(pipeline, start_pos, end_pos, bg_color_lambda):
    return pipeline

def parse_pipe(line):
    seq_num = int(line[1])
    pc = int(line[2], 16)
    upc = int(line[3], 16)
    inst = line[4].strip()

    creationTick = max_tick_to_invalid_tick_lambda(int(line[5]))
    decodeTick = max_tick_to_invalid_tick_lambda(int(line[6]))
    issueTick = max_tick_to_invalid_tick_lambda(int(line[7]))
    beginExecuteTick = max_tick_to_invalid_tick_lambda(int(line[8]))
    effAddredTick = max_tick_to_invalid_tick_lambda(int(line[9]))
    beginMemoryTick = max_tick_to_invalid_tick_lambda(int(line[10]))
    completionTick = max_tick_to_invalid_tick_lambda(int(line[11]))
    commitTick = max_tick_to_invalid_tick_lambda(int(line[12]))
    squashTick = max_tick_to_invalid_tick_lambda(int(line[13]))

    ticks = (creationTick, decodeTick, issueTick, beginExecuteTick,
             effAddredTick, beginMemoryTick, completionTick, commitTick,
             squashTick)
    return seq_num, pc, upc, inst, ticks

def pipe_to_string(line, tick_start, tick_end, coloring_fnct, width,
                   cycle_time, dep):
    ticks_per_line = cycle_time * width
    # decode the instruction info
    seq_num, pc, upc, inst, ticks = parse_pipe(line)
    creationTick = ticks[0]
    squashTick = ticks[-1]
    wrap_around = False

    if creationTick < tick_start or creationTick > tick_end:
        return False, -1, None

    # [...............f............]
    #  ^
    #  start_tick <- the tick at the first position of the pipeline
    start_tick = creationTick - creationTick % ticks_per_line
    end_tick = max(ticks)

    # the lifespan of an instruction can exceed ticks_per_line,
    # so we need multiple lines to represent the lifecycle of
    # the instruction
    n_lines = end_tick // ticks_per_line - start_tick // ticks_per_line + 1
    if n_lines == 2 and \
       end_tick // ticks_per_line - start_tick // ticks_per_line  < width:
        n_lines = 1
        wrap_around = True

    # [...............f......c.....]
    #                 ^      ^
    #           start_pos   end_pos
    start_pos = (creationTick - start_tick) // cycle_time
    end_pos = (end_tick - start_tick) // cycle_time + 1

    # the pipeline looks like:
    #   [=======f====] if the instruction is squashed
    #   [.......f....] if the instruction is committed
    if squashTick == INVALID_TICK:
        pipe = ["."] * (width * n_lines)
    else:
        pipe = ["="] * (width * n_lines)

    for tick, name in zip(ticks, stages):
        if not tick == INVALID_TICK:
            if wrap_around:
                pipe[((tick - start_tick) // cycle_time) % width] = name
            else:
                pipe[((tick - start_tick) // cycle_time)] = name

    # add color to pipeline
    pipe = coloring_fnct(pipe, start_pos, end_pos, bg_color_char_lambda,
                         wrap_around)

    if seq_num in dep:
        dependencies = ", ".join(list(map(str, dep[seq_num])))
    else:
        dependencies = ""

    # split the pipeline to equal chunks of length `width`
    _pipe = []
    for i in range(n_lines):
        _pipe.append("".join(pipe[i*width:(i+1)*width]))
    pipe = _pipe

    # format the output
    if (len(pipe) == 1):
        pipe[0] = "({:10d}) [{}] 0x{:08x}:{:d} {:6d} [{:10}] {}".format(
                  start_tick, pipe[0], pc, upc, seq_num, dependencies, inst)
    else:
        pipe[0] = "({:10d}) [{}  0x{:08x}:{:d} {:6d} [{:10}] {}".format(
                  start_tick, pipe[0], pc, upc, seq_num, dependencies, inst)
        for i in range(1, len(pipe)):
            pipe[i] = "{:12}  {}".format("", pipe[i])
        pipe[-1] = "{}]".format(pipe[-1])

    pipe.append("")
    info = "\n".join(pipe)
    return True, seq_num, info


def parse_dependency(line):
    # inst[seq_num] depends on inst[parent_seq_num]
    seq_num = int(line[1])
    parent_seq_num = int(line[2])
    return seq_num, parent_seq_num

def parser(input_file, output_file, tick_start, tick_end, color, width,
           cycle_time):
    curr_batch_seq_num = 0
    insts = {}
    dep = {}
    max_seq_num = -1

    # choosing the coloring function
    if color:
        coloring_fnct = colorize_pipeline
    else:
        coloring_fnct = no_colorize_pipeline

    # read the debugging output
    with open(input_file, "r") as f:
        seq_num = -1
        for line in f:
            line = line.strip().split(";")
            if line[0] == "pipe":
                in_tick_range, seq_num, info = pipe_to_string(line, tick_start,
                               tick_end, coloring_fnct, width, cycle_time, dep)
                if in_tick_range:
                    insts[seq_num] = info
                max_seq_num = max(seq_num, max_seq_num)
            elif line[0] == "dep":
                seq_num, parent_seq_num = parse_dependency(line)
                if not seq_num in dep:
                    dep[seq_num] = {parent_seq_num}
                else:
                    dep[seq_num].add(parent_seq_num)

    # write the formatted pipeline view to the output file
    with open(output_file, "w") as g:
        # format the legends of the pipeline
        legends = []
        stages_with_color = [bg_color_map[stage_color_map[stage]] + stage +
                             bg_color_map["black"] for stage in stages]
        for name, fullname in zip(stages_fullname, stages_with_color):
            legends.append("{}: {}".format(name, fullname))
        g.write(", ".join(legends))
        g.write("\n")

        # format the title of the output
        headers = "{:^12} {} {:^12} {:6} {:^12} {}\n".format("tick",
                  "pipeline".center(width+2, " "), "pc:upc", "seqnum",
                  "dependencies", "instruction")
        g.write(headers)

        # seq_num starts from 1
        for seq_num in range(1, max_seq_num):
            if seq_num in insts:
                g.write(insts[seq_num])

def main():
    default_options = {
        "input_file"  : "",
        "output_file" : "trace.out",
        "tick_start"  : 0,
        "tick_end"    : MAX_TICK,
        "color"       : True,
        "width"       : 80,
        "cycle_time"  : 1000
    }

    argparser = argparse.ArgumentParser(description="FlexCPU pipeline viewer")
    argparser.add_argument("-i", "--input-file",
                           required=True)
    argparser.add_argument("-o", "--output-file",
                           default=default_options["output_file"])
    argparser.add_argument("-s", "--tick-start",
                           default=default_options["tick_start"],
                           type=int)
    argparser.add_argument("-e", "--tick-end",
                           default=default_options["tick_end"],
                           type=int)
    argparser.add_argument("-l", "--color",
                           action='store_true',
                           default=default_options["color"])
    argparser.add_argument("-w", "--width",
                           type=int,
                           default=default_options["width"])
    argparser.add_argument("-c", "--cycle-time",
                           type=int,
                           default=default_options["cycle_time"])
    args = argparser.parse_args()
    print(vars(args))
    parser(**vars(args))

if __name__ == "__main__":
    main()
