import heapq
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection

class Net:
    """
    Simple network/graph data structure for representing relations in 2D
    space. Please note that all methods, unless otherwise specified,
    take numpy arrays as inputs/outputs.
    """ 
    def __init__(self, adj, xy):
        """Initializes the network for a given adjecancy matrix and X-Y 
        coordinates of nodes."""
        self.adj = adj
        self.xy = xy
        self.widths = np.zeros(adj.shape)
        self._update_widths()

    def __getitem__(self, key):
        """
        Overloads subscript operator for the class as follows:
        - if key is an integer, returns X-Y coordinates of a corresponding node,
        - if key is a 2-tuple, returns edge weight between corresponding nodes.
        This method performs no error checks, ensure that subscripts are correct.
        """
        if type(key) == int:
            return self.xy[key]
        elif type(key) == tuple and len(key) == 2:
            return self.widths[ key[0], key[1] ]

    def _update_widths(self):
        """
        (Re)calculates edge widths based on adjecency matrix and node X-Y
        coordinates. Should be called internally whenever network structure
        changes.
        """
        i, j = np.where(self.adj)
        self.widths[i, j] = np.linalg.norm(self.xy[i]-self.xy[j], axis=1)

    def neighbours(self, node):
        return np.where(self.adj[node])[0]

    def plot(self, ax, marker_style='s', margin=0.05):
        """Plots network's nodes and edges on a 2D plane."""
        # generate edge coordinates
        i, j = np.where(np.tril(self.adj))
        lines = np.zeros((i.size, 2, 2))
        lines[:, 0, :], lines[:, 1, :] = self.xy[i], self.xy[j]
        segments = LineCollection(lines)
        
        # set plot limits, draw generated edges and mark nodes
        margin *= np.max(self.xy, axis=0) - np.min(self.xy, axis=0)
        xy_min, xy_max = np.min(self.xy, axis=0) - margin, np.max(self.xy, axis=0) + margin
        ax.set_xlim(xy_min[0], xy_max[0])
        ax.set_ylim(xy_min[1], xy_max[1])
        ax.add_collection(segments)
        ax.plot(self.xy[:, 0], self.xy[:, 1], marker_style)

    def shortest_path(self, start, goal):
        """Calculates shortest path between two nodes using A* algorithm."""
        # using XY distance as a heuristic
        h = lambda node: np.linalg.norm(self.xy[goal] - self.xy[node])
        frontier = [(0, start)]
        came_from = {start : None}
        cost = {start : 0}

        while frontier:
            _ ,current = heapq.heappop(frontier)
            if current == goal:
                break

            for neigh in self.neighbours(current):
                new_cost = cost[current] + self[current, neigh]
                if neigh not in cost or new_cost < cost[neigh]:
                    cost[neigh] = new_cost
                    heapq.heappush(frontier, (new_cost + h(neigh), neigh))
                    came_from[neigh] = current

        # reconstruct the path
        path = [goal]
        while path[0] in came_from.keys():
            path = [ came_from[path[0]] ] + path
        return path[1:]

def gen_grid(n, hor_len, ver_len):
    """Generates a 2D grid/lattice network with a given number of nodes/side
    and vertical/horizontal edge lengths."""
    adj = np.zeros((n*n, n*n), dtype=bool)
    i = np.arange(n*n)
    # generate all possible connections between grid nodes, take only those
    # that are possible (index of a node exists)
    j = i[:, np.newaxis] + np.array([-1, 1, -n, n])
    j_use = np.logical_and(j >= 0, j < n*n)
    # manually correct for unwanted connections between rightmost and leftmost nodes
    j_use[i[i%n == 0], 0] = False
    j_use[i[i%n == n-1], 1] = False
    # generate suitable indices and fill the adjecancy matrix
    i_rep = np.repeat(i, np.count_nonzero(j_use, axis=1))
    j_rep = j[j_use]
    adj[i_rep, j_rep] = True
    # generate node X-Y coordinates and construct a network
    xy = np.stack((np.tile(hor_len*np.arange(n), n), np.repeat(ver_len*np.arange(n), n)))
    return Net(adj, np.transpose(xy))

# the following code is for debug purposes only!
net = gen_grid(50, 100, 100)
fig, ax = plt.subplots()
net.plot(ax)
plt.show()