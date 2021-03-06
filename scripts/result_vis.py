import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button, TextBox
import numpy as np
import argparse

import network
from _common import agent_type, get_agents_points, plot_agents, results_fromfile

class ResultIterator:
    def __init__(self, result, bplay_pause, bstop, bstep_prev, bstep_next):
        self.result = result
        self.step = 0
        self.paused = True

        bplay_pause.on_clicked(self.play_pause)
        bstop.on_clicked(self.stop)
        bstep_prev.on_clicked(self.step_prev)
        bstep_next.on_clicked(self.step_next)

    def __next__(self):
        result = self.result[self.step]
        if not self.paused and self.step < len(self.result)-1:
            self.step += 1
        return result

    def play_pause(self, _):
        self.paused = not self.paused

    def stop(self, _):
        self.step = 0
        self.paused = True

    def step_prev(self, _):
        if self.paused and self.step - 1 >= 0:
            self.step -= 1

    def step_next(self, _):
        if self.paused and self.step + 1 < len(self.result):
            self.step += 1

class ResultWrapper:
    def __init__(self, result, *iter_init_params):
        self.result = result
        self.iter_init_params = iter_init_params

    def __iter__(self):
        return ResultIterator(self.result, *self.iter_init_params)

    def __getitem__(self, key):
        return self.result[key]

def update(agents, lines, points, up, down, left, right):
    xy_agents, up_mask, down_mask, left_mask, right_mask = get_agents_points(net, agents)
    up.set_data(xy_agents[up_mask, 0], xy_agents[up_mask, 1])
    down.set_data(xy_agents[down_mask, 0], xy_agents[down_mask, 1])
    left.set_data(xy_agents[left_mask, 0], xy_agents[left_mask, 1])
    right.set_data(xy_agents[right_mask, 0], xy_agents[right_mask, 1])
    return lines, points, up, down, left, right


parser = argparse.ArgumentParser(description='Visualize simulation result on a network map.')
parser.add_argument('-ri', metavar='INRESFILE', help='input result file (default: res.bin)', default='res.bin')
parser.add_argument('-ni', metavar='INNETFILE', help='input network file, .npz extension (default: net)', default='net')
args = parser.parse_args()

net = network.load(args.ni)
result, _ = results_fromfile(args.ri)

# UI setup
fig, ax = plt.subplots()
plt.subplots_adjust(.1, .175, .9, .95)
bplay_pause = Button(plt.axes([.1  , .05, .15, .05]), 'Play/Pause')
bstop       = Button(plt.axes([.275, .05, .15, .05]), 'Stop')
bstep_prev  = Button(plt.axes([.45 , .05, .15, .05]), 'Previous')
bstep_next  = Button(plt.axes([.625, .05, .15, .05]), 'Next')

# plotting network and agents, animation setup
lines, points = net.plot(ax)
up, down, left, right = plot_agents(ax, net, result[0], alpha=0.8, color='r')
result = ResultWrapper(result, bplay_pause, bstop, bstep_prev, bstep_next)
ani = FuncAnimation(fig, update, frames=result, fargs=(lines, points, up, down, left, right), interval=20, blit=True)
plt.show()
