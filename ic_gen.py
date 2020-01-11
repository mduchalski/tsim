import sys
import json
import argparse
import numpy as np
import matplotlib.pyplot as plt
from enum import Enum

import network

v0, s0, T, a, b = 15, 1.5, 3, 1, 2
spacing = s0 + v0 * T

agent_type = np.dtype([
    ('x', 'f8'),
    ('v', 'f8'),
    ('prev', 'i4'),
    ('next', 'i4'),
    ('route_pos', 'i4'),
    ('_params', 'u8')])

agent_params_type = np.dtype([
    ('uid', 'i4'),
    ('v0', 'f8'),
    ('s0', 'f8'),
    ('T', 'f8'),
    ('a', 'f8'),
    ('b', 'f8'),
    ('route_len', 'i4'),
    ('_route', 'u8')])

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

def gen_agent(net, agents):
    """Generates a complete definition for of a single agent."""
    agent = np.empty(1, dtype=agent_type)
    
    # generate reference x, y coordinates and select closest pair of nodes as the edge
    xy_min, xy_size = net.xy_limits()
    xy_start = xy_min + xy_size*np.random.random_sample(2)
    agent['prev'] = net.closest(xy_start)
    neighs = net.neighbours(agent['prev'][0])
    agent['next'] = neighs[net.closest(xy_start, neighs)] 

    # find all existing agents on this edge and generate a suitable position for this agent
    on_edge = agents['x'][(agents['prev'] == agent['prev']) & (agents['next'] == agent['next'])]
    breaks = np.transpose(np.stack((on_edge - spacing, on_edge + spacing)))
    agent['x'] = random_disjoint(breaks, net[ agent['prev'][0], agent['next'][0] ] - spacing)

    valid = agent['x'] > 0
    agent_route = net.shortest_path(agent['next'][0], net.closest(xy_min +
        xy_size*np.random.random_sample(2)))[1:] if valid else None
    agent['route_pos'] = 0

    return agent, agent_route, valid

def gen_agents(net, config):
    """Generates full definitions (state, parameters and routes) for n-random agents."""
    # generate agents' positions and routes sequentially
    agents = np.empty(config['attempts'], dtype=agent_type)
    agents_routes = np.empty_like(agents, dtype=object)
    mask = np.zeros_like(agents, dtype=bool)
    for i in range(agents.size):
        agents[i], agents_routes[i], mask[i] = gen_agent(net, agents)
    agents, agents_routes = agents[mask], agents_routes[mask]

    # generate agents' other parameters all at once
    agents_params = np.empty(agents.size, dtype=agent_params_type)
    agents_params['route_len'] = np.array([len(route) for route in agents_routes])
    agents_params['uid'] = np.arange(agents.size)

    agr = np.random.normal(1.0, config['agressive_stdev'], agents.size)
    mean = config['idm_mean_params']
    agents_params['v0'] = mean['v0'] * agr
    agents_params['s0'] = mean['s0'] * (2-agr)
    agents_params['T']  = mean['T']  * (2-agr)
    agents_params['a']  = mean['a']  * agr
    agents_params['b']  = mean['b']  * agr
    agents['v'] = agents_params['v0']
    agents['v'] *= np.random.normal(1.0, config['speed_from_v0_stdev'], agents.size)
    return agents, agents_params, agents_routes

def ic_tofile(net, agents, agents_params, agents_routes, filename):
    """Outputs initial conditions to a binary file."""
    with open(filename, 'wb') as f:
        np.array([len(net)], dtype='i4').tofile(f)
        net.weights.tofile(f)
        np.array([len(agents)], dtype='i4').tofile(f)
        agents.tofile(f)
        agents_params.tofile(f)
        for agent_route in agents_routes:
            np.array(agent_route, dtype='i4').tofile(f)

# load generation parameters from JSON file
with open('params.json') as config_file:
    config_text = config_file.read()
config = json.loads(config_text)['config']

net = network.gen_grid(config['network']['size'], 
    config['network']['unit_width'],
    config['network']['unit_height'])
agents, agents_params, agents_routes = gen_agents(net, config['agents'])
ic_tofile(net, agents, agents_params, agents_routes, 'ic.bin')
net.save('net')