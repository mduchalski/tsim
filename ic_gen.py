import json
import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Button

import network
from _common import agent_type, plot_agents

v0, s0, T, a, b = 15, 1.5, 3, 1, 2
spacing = s0 + v0 * T

agent_params_type = np.dtype([
    ('v0', 'f8'),
    ('s0', 'f8'),
    ('T', 'f8'),
    ('a', 'f8'),
    ('b', 'f8'),
    ('route_len', 'i4'),
    ('_route', 'u8')])

inter_type = np.dtype([
    ('type_id', 'i4'),
    ('_params', 'u8')
])

inter_type_ids = {
    "always_open" : 0,
    "simple" : 1
}

def random_disjoint(breaks, end):
    """
    Generates a random number from a range of (0, end) given a 2xN vector of
    forbidden ranges defined as (start, end) for each row. Ranges can overlap
    and they don't have to be given in order.
    """
    if breaks.size > 0:
        breaks = breaks[np.argsort(breaks[:, 0])]
        breaks = np.concatenate(([[0, 0]], breaks, [[end, end]]))
    else:
        breaks = np.array([[0, 0], [end, end]])
    breaks[breaks < 0], breaks[breaks > end] = 0, end

    mask = breaks[1:, 0] >= breaks[:-1, 1]
    starts = breaks[1:, 0][mask]
    widths = np.concatenate(([0], np.cumsum(breaks[:-1, 1][mask][1:] - starts[:-1])))
    
    if end == widths[-1]:
        return -1
    x = (end - widths[-1]) * np.random.random_sample()
    return x + widths[np.argmax(x + widths < starts)]

class Generator:
    def __init__(self, config_filename):
        self.config_filename = config_filename
        self.load_config()
        self.update()

    def load_config(self):
        with open(self.config_filename) as config_file:
            config_text = config_file.read()
        self.config = json.loads(config_text)['config']

    def update(self):
        self.net = network.gen_grid(self.config['network']['size'], 
            self.config['network']['unit_width'],
            self.config['network']['unit_height'])
        self._update_agents()
        self._update_intersections()

    def load_update_plot(self, _):
        self.load_config()
        self.update()
        self.update_plot()

    def save(self, ic_filename, net_filename):
        '''Outputs initial conditions to a binary file.'''
        with open(ic_filename, 'wb') as f:
            np.array([len(self.net)], dtype='i4').tofile(f)
            self.net.weights.tofile(f)
            np.array([len(self.agents)], dtype='i4').tofile(f)
            self.agents.tofile(f)
            self.agents_params.tofile(f)
            for agent_route in self.agents_routes:
                np.array(agent_route, dtype='i4').tofile(f)
            self.inters_types.tofile(f)
            for inter_params in self.inters_params:
                np.array(inter_params, dtype='f8').tofile(f)

        self.net.save(net_filename)

    def attach_plot(self, ax):
        self.ax = ax

    def update_plot(self):
        self.ax.clear()
        plot_agents(self.ax, self.net, self.agents, alpha=0.8, color='r')
        self.net.plot(self.ax)

    def _update_intersections(self):
        config = self.config['intersections']
        choices = []
        probabilities = []
        for type_names, opts in config.items():
            choices.append(inter_type_ids[type_names])
            probabilities.append(opts['probability'])
        self.inters_types = np.empty(len(self.net), dtype=inter_type)
        self.inters_types['type_id'] = np.random.choice(choices, len(self.net), p=probabilities)
        
        self.inters_params = []
        for type_id in self.inters_types['type_id']:
            if inter_type_ids['simple'] == type_id:
                timeout = np.random.normal(config['simple']['timeout_mean'],
                    config['simple']['timeout_stdev'])
                offset = np.abs(np.random.normal(config['simple']['offset_stdev']))
                self.inters_params.append([timeout, offset])
            else:
                self.inters_params.append([])

    def _update_agents(self):
        """Generates full definitions (state, parameters and routes) for n-random agents."""
        # generate agents' positions and routes sequentially
        config = self.config['agents']
        self.agents = np.zeros(config['attempts'], dtype=agent_type)
        self.agents_routes = np.empty_like(self.agents, dtype=object)
        mask = np.zeros_like(self.agents, dtype=bool)

        for i in range(len(self.agents)):
            self.agents[i], self.agents_routes[i], mask[i] = self._gen_agent()
        self.agents, self.agents_routes = self.agents[mask], self.agents_routes[mask]
        self.agents['uid'] = np.arange(len(self.agents))

        # generate agents' other parameters all at once
        self.agents_params = np.empty_like(self.agents, dtype=agent_params_type)
        self.agents_params['route_len'] = np.array([len(route) for route in self.agents_routes])

        agr = np.random.normal(1.0, config['agressive_stdev'], len(self.agents))
        mean = config['idm_mean_params']
        self.agents_params['v0'] = mean['v0'] * agr
        self.agents_params['s0'] = mean['s0'] * (2-agr)
        self.agents_params['T']  = mean['T']  * (2-agr)
        self.agents_params['a']  = mean['a']  * agr
        self.agents_params['b']  = mean['b']  * agr
        self.agents['v'] = self.agents_params['v0']
        self.agents['v'] *= np.random.normal(1.0, config['speed_from_v0_stdev'], len(self.agents))

    def _gen_agent(self):
        """Generates a complete definition for of a single agent."""
        agent = np.empty(1, dtype=agent_type)
        
        # generate reference x, y coordinates and select closest pair of nodes as the edge
        xy_min, xy_size = self.net.xy_limits()
        xy_start = xy_min + xy_size*np.random.random_sample(2)
        agent['prev'] = self.net.closest(xy_start)
        neighs = self.net.neighbours(agent['prev'][0])
        agent['next'] = neighs[self.net.closest(xy_start, neighs)] 

        # find all existing agents on this edge and generate a suitable position for this agent
        on_edge = self.agents['x'][
            (self.agents['prev'] == agent['prev']) & (self.agents['next'] == agent['next'])]
        breaks = np.transpose(np.stack((on_edge - spacing, on_edge + spacing)))
        agent['x'] = random_disjoint(breaks, self.net[agent['prev'][0], agent['next'][0]] - spacing)

        valid = agent['x'] > 0
        agent_route = self.net.shortest_path(agent['next'][0], self.net.closest(xy_min +
            xy_size*np.random.random_sample(2)))[1:] if valid else None
        agent['route_pos'] = 0

        return agent, agent_route, valid

# process commandline arguments and load the configuration file
parser = argparse.ArgumentParser(description='Generate initial conditions for traffic simulation.')
parser.add_argument('-g', action='store_true',
    help='launch in graphical mode (not yet supported)')
parser.add_argument('-c', metavar='CONFIG', default='config.json',
    help='configuration filename (default: config.json)')
parser.add_argument('-oi', metavar='OUTIC', default='ic.bin',
    help='output initial conditions filename (default: ic.bin)')
parser.add_argument('-on', metavar='OUTNET', default='net',
    help='output network filename, saved as an *.npz file (default: net)')
args = parser.parse_args()

gen = Generator(args.c)
if args.g:
    fig, ax = plt.subplots()
    plt.subplots_adjust(.1, .2, .9, .95)
    bloadupd = Button(plt.axes((0.1, 0.05, 0.2, 0.075)), 'Load and update')
    bsave = Button(plt.axes((0.325, 0.05, 0.2, 0.075)), 'Save')
    bloadupd.on_clicked(gen.load_update_plot)
    bsave.on_clicked(lambda _: gen.save(args.oi, args.on))

    gen.attach_plot(ax)
    gen.update_plot()
    plt.show()
else:
    gen.save(args.oi, args.on)