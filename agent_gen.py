import sys
import network
import numpy as np
import matplotlib.pyplot as plt
from collections import namedtuple

agent_type = np.dtype([('x', 'f8'), ('v', 'f8'), ('prev', 'i4'), ('next', 'i4'), ('route_pos', 'i4')])
agent_static_type = namedtuple('agent_static_type', 'v0 s0 T a b route_len route')
base = agent_static_type(15, 1.5, 2, 1, 2, 0, None)
min_dist = base.s0 + base.v0*base.T

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
    agent = np.full(1, -1, dtype=agent_type)
    #agent_params = np.zeros(1, dtype=agent_params_type)

    # generate reference x, y coordinates and select closest pair of nodes as the edge
    xy_min, xy_size = net.xy_limits()
    xy_start = xy_min + xy_size*np.random.random_sample(2)
    agent['prev'] = net.closest(xy_start)
    neighs = net.neighbours(agent['prev'][0])
    agent['next'] = neighs[net.closest(xy_start, neighs)] 

    # generate agent parameters and speed
    agr = np.random.normal(1.0, 0.1) # (*) make configurable
    route = net.shortest_path(agent['next'][0], net.closest(
        xy_min + xy_size*np.random.random_sample(2)))[1:]
    agent_meta = agent_static_type(agr*base.v0, (2-agr)*base.s0, (2-agr)*base.T, 
        agr*base.a, agr*base.b, len(route), route)
    agent['v'] = np.random.normal(1.0, 0.2)*agent_meta.v0

    # find all existing agents on this edge and generate a suitable position for this agent
    on_edge = agents['x'][(agents['prev'] == agent['prev']) & (agents['next'] == agent['next'])]
    breaks = np.transpose(np.stack((on_edge - min_dist, on_edge + min_dist)))
    agent['x'] = random_disjoint(breaks, net[ agent['prev'][0], agent['next'][0] ]-min_dist)

    return agent, agent_meta

def gen_agents(net, n):
    agents = np.empty(n, dtype=agent_type)
    agents_static = np.empty(n, dtype=object)

    for i in range(agents.size):
        agents[i], agents_static[i] = gen_agent(net, agents)

        sys.stdout.write('\r{:.1f}%'.format(100*i/n))
        sys.stdout.flush()

    mask = agents['x'] > 0
    return agents[mask], agents_static[mask]

def plot_agents(ax, net, agents):
    """Plots agents in 2D space given the network they're"""
    # calculate X-Y agents' positions
    prev, next = agents['prev'], agents['next']
    vecs = net.xy[next]-net.xy[prev]
    xy_agents = net.xy[prev] + (agents['x'] / net[prev, next])[:, np.newaxis]*vecs

    # separate agents by direction of movement (approximated by four directions)
    angles = np.arctan2(vecs[:, 1], vecs[:, 0]) 
    up_mask = (angles <= 3*np.pi/4) & (angles > np.pi/4)
    down_mask = (angles <= -np.pi/4) & (angles > -3*np.pi/4)
    right_mask = (angles <= np.pi/4) & (angles > -np.pi/4)
    left_mask = (angles < -3*np.pi/4) | (angles >= 3*np.pi/4)
    
    ax.plot(xy_agents[up_mask, 0], xy_agents[up_mask, 1], 'r^', alpha=.8)
    ax.plot(xy_agents[down_mask, 0], xy_agents[down_mask, 1], 'rv', alpha=.8)
    ax.plot(xy_agents[right_mask, 0], xy_agents[right_mask, 1], 'r>', alpha=.8)
    ax.plot(xy_agents[left_mask, 0], xy_agents[left_mask, 1], 'r<', alpha=.8)

def ic_tofile(net, agents, agents_params, filename):
    with open(filename, 'wb') as f:
        np.array([len(net)], dtype='i4').tofile(f)
        net.weights.tofile(f)
        np.array([len(agents)], dtype='i4').tofile(f)
        agents.tofile(f)
        for agent_params in agents_params:
            np.array(agent_params[:-2], dtype='f8').tofile(f)
            np.array([agent_params.route_len], dtype='i4').tofile(f)
            np.array(agent_params.route, dtype='i4').tofile(f)

net = network.gen_grid(5, 300, 300)
agents, agents_params = gen_agents(net, 500)
ic_tofile(net, agents, agents_params, 'net.bin')
fig, ax = plt.subplots()
net.plot(ax)
plot_agents(ax, net, agents)
plt.show()