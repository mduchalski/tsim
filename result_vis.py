import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button, TextBox
import network

agent_type = np.dtype([
    ('x', 'f8'),
    ('v', 'f8'),
    ('prev', 'i4'),
    ('next', 'i4'),
    ('route_pos', 'i4'),
    ('_params', 'u8')])

def get_agents_points(net, agents):
    # calculate X-Y agents' positions
    mask = agents['next'] != -1
    prev, next = agents['prev'][mask] , agents['next'][mask]
    vecs = net.xy[next]-net.xy[prev]
    xy_agents = net.xy[prev] + (agents['x'][mask] / net[prev, next])[:, np.newaxis]*vecs
    if np.any(net[prev, next] == 0):
        print('wtf')

    # separate agents by direction of movement (approximated by four directions)
    angles = np.arctan2(vecs[:, 1], vecs[:, 0]) 
    up_mask = (angles <= 3*np.pi/4) & (angles > np.pi/4)
    down_mask = (angles <= -np.pi/4) & (angles > -3*np.pi/4)
    left_mask = (angles < -3*np.pi/4) | (angles >= 3*np.pi/4)
    right_mask = (angles <= np.pi/4) & (angles > -np.pi/4)

    return xy_agents, up_mask, down_mask, left_mask, right_mask

def plot_agents(ax, net, agents, **plot_kwargs):
    """Plots agents in 2D space."""
    xy_agents, up_mask, down_mask, left_mask, right_mask = get_agents_points(net, agents)

    up, = ax.plot(xy_agents[up_mask, 0], xy_agents[up_mask, 1], '^', **plot_kwargs)
    down, = ax.plot(xy_agents[down_mask, 0], xy_agents[down_mask, 1], 'v', **plot_kwargs)
    left, = ax.plot(xy_agents[left_mask, 0], xy_agents[left_mask, 1], '<', **plot_kwargs)
    right, = ax.plot(xy_agents[right_mask, 0], xy_agents[right_mask, 1], '>', **plot_kwargs)

    return up, down, left, right

def results_fromfile(filename):
    with open(filename, 'rb') as f:
        n_steps = np.fromfile(f, dtype='i4', count=1)[0]
        n_agents = np.fromfile(f, dtype='i4', count=1)[0]
        results = np.fromfile(f, dtype=agent_type, count=n_steps*n_agents)
        results = np.reshape(results, (n_steps, n_agents))
    return results

class ResultIterator:
    def __init__(self, result, bplay_pause, bstop, bstep_prev, bstep_next, tstep):
        self.result = result
        self.step = 0
        self.paused = True

        bplay_pause.on_clicked(self.play_pause)
        bstop.on_clicked(self.stop)
        bstep_prev.on_clicked(self.step_prev)
        bstep_next.on_clicked(self.step_next)
        self.tstep = tstep

    def __next__(self):
        result = self.result[self.step]
        if not self.paused and self.step < len(self.result)-1:
            self.step += 1
        return result

    def play_pause(self, _):
        self.paused = not self.paused
        if self.paused:
            self.tstep.set_val(str(self.step))
        else:
            self.tstep.set_val('...')

    def stop(self, _):
        self.step = 0
        self.paused = True
        self.tstep.set_val('0')

    def step_prev(self, _):
        if self.paused and self.step - 1 >= 0:
            self.step -= 1
            self.tstep.set_val(str(self.step))

    def step_next(self, _):
        if self.paused and self.step + 1 < len(self.result):
            self.step += 1
            self.tstep.set_val(str(self.step))

class ResultWrapper:
    def __init__(self, result, *iter_init_params):
        self.result = result
        self.iter_init_params = iter_init_params

    def __iter__(self):
        return ResultIterator(self.result, *self.iter_init_params)

    def __getitem__(self, key):
        return self.result[key]

def update(agents, up, down, left, right):
    xy_agents, up_mask, down_mask, left_mask, right_mask = get_agents_points(net, agents)
    up.set_data(xy_agents[up_mask, 0], xy_agents[up_mask, 1])
    down.set_data(xy_agents[down_mask, 0], xy_agents[down_mask, 1])
    left.set_data(xy_agents[left_mask, 0], xy_agents[left_mask, 1])
    right.set_data(xy_agents[right_mask, 0], xy_agents[right_mask, 1])
    return up, down, left, right

net = network.load('net')
result = results_fromfile('results.bin')

# UI setup
fig, ax = plt.subplots()
plt.subplots_adjust(.1, .25, .9, .95)
bplay_pause = Button(plt.axes([.1, .125, .15, .05]), 'Play/Pause')
bstop       = Button(plt.axes([.1, .05, .15, .05]), 'Stop')
bstep_prev  = Button(plt.axes([.275, .125, .1, .05]), 'Previous')
bstep_next  = Button(plt.axes([.275, .05, .1, .05]), 'Next')
tstep = TextBox(plt.axes([.45, .125, .1, .05]), 'Step', initial='0', label_pad=.1)
tstep.set_active(False)

# plotting network and agents, animation setup
net.plot(ax)
up, down, left, right = plot_agents(ax, net, result[0], alpha=0.8, color='r')
result = ResultWrapper(result, bplay_pause, bstop, bstep_prev, bstep_next, tstep)
ani = FuncAnimation(fig, update, frames=result, fargs=(up, down, left, right), interval=20, blit=True)
plt.show()