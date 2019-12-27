import network
import numpy as np
import matplotlib.pyplot as plt

agent_type = np.dtype([('x', 'f8'), ('v', 'f8'), ('prev', 'i4'), ('next', 'i4'), ('rt_idx', 'i4')])
agent_params_type = np.dtype([('v0', 'f8'), ('a', 'f8'), ('b', 'f8'), ('s0', 'f8'), ('T', 'f8')])
v0_base, a_base, b_base, s0_base, T_base = 15, 1, 2, 1.5, 2
min_dist = s0_base + v0_base*T_base

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

def gen_agent(net, agents, agents_params):
    """Generates a complete definition for of a single agent."""
    agent = np.full(1, -1, dtype=agent_type)
    agent_params = np.zeros(1, dtype=agent_params_type)

    # generate reference x, y coordinates and select closest pair of nodes as the edge
    xy_min, xy_size = net.xy_limits()
    xy_start = xy_min + xy_size*np.random.random_sample(2)
    agent['prev'] = net.closest(xy_start)
    neighs = net.neighbours(agent['prev'][0])
    agent['next'] = neighs[net.closest(xy_start, neighs)]
    agent_route = net.shortest_path(agent['next'][0],
        net.closest(xy_min + xy_size*np.random.random_sample(2)))

    # generate agent parameters and speed
    agr = np.random.normal(1.0, 0.1) # (*) make configurable
    agent_params['v0'] = agr * v0_base
    agent_params['a'] = agr * a_base
    agent_params['b'] = agr * b_base
    agent_params['s0'] = (2-agr) * s0_base
    agent_params['T'] = (2-agr) * T_base
    agent['v'] = np.random.normal(1.0, 0.2)*agent_params['v0']

    # find all existing agents on this edge and generate a suitable position for this agent
    on_edge = agents['x'][(agents['prev'] == agent['prev']) & (agents['next'] == agent['next'])]
    breaks = np.transpose(np.stack((on_edge - min_dist, on_edge + min_dist)))
    agent['x'] = random_disjoint(breaks, net[ agent['prev'][0], agent['next'][0] ]-min_dist)

    return agent, agent_params, agent_route

def gen_agents(net, n):
    agents = np.zeros(n, dtype=agent_type)
    agents_params = np.zeros(n, dtype=agent_params_type)
    mask = np.zeros(n, dtype=bool)
    agents_routes = []

    for i in range(agents.size):
        agents[i], agents_params[i], agent_route = gen_agent(net, agents, agents_params)
        if agents[i]['x'] > 0:
            mask[i] = True
            agents_routes.append(agent_route)

    return agents[mask], agents_params[mask], agents_routes

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

net = network.gen_grid(2, 500, 500)
agents, agents_params, agents_routes = gen_agents(net, 5000)
fig, ax = plt.subplots()
net.plot(ax)
plot_agents(ax, net, agents)
plt.show()