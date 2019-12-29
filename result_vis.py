import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
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

def plot_agents(ax, net, agents):
    """Plots agents in 2D space."""
    xy_agents, up_mask, down_mask, left_mask, right_mask = get_agents_points(net, agents)

    up, = ax.plot(xy_agents[up_mask, 0], xy_agents[up_mask, 1], 'r^', alpha=.8)
    down, = ax.plot(xy_agents[down_mask, 0], xy_agents[down_mask, 1], 'rv', alpha=.8)
    left, = ax.plot(xy_agents[left_mask, 0], xy_agents[left_mask, 1], 'r<', alpha=.8)
    right, = ax.plot(xy_agents[right_mask, 0], xy_agents[right_mask, 1], 'r>', alpha=.8)

    return up, down, left, right

def results_fromfile(filename):
    with open(filename, 'rb') as f:
        n_steps = np.fromfile(f, dtype='i4', count=1)[0]
        n_agents = np.fromfile(f, dtype='i4', count=1)[0]
        results = np.fromfile(f, dtype=agent_type, count=n_steps*n_agents)
        results = np.reshape(results, (n_steps, n_agents))
    return results

def update(agents, up, down, left, right):
    xy_agents, up_mask, down_mask, left_mask, right_mask = get_agents_points(net, agents)
    up.set_data(xy_agents[up_mask, 0], xy_agents[up_mask, 1])
    down.set_data(xy_agents[down_mask, 0], xy_agents[down_mask, 1])
    left.set_data(xy_agents[left_mask, 0], xy_agents[left_mask, 1])
    right.set_data(xy_agents[right_mask, 0], xy_agents[right_mask, 1])


net = network.load('net')
res = results_fromfile('results.bin')
fig, ax = plt.subplots()
net.plot(ax)
up, down, left, right = plot_agents(ax, net, res[0])
ani = FuncAnimation(fig, update, frames=res, fargs=(up, down, left, right), interval=20)
plt.show()