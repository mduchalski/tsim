import argparse
import textwrap
import numpy as np

from _common import results_fromfile

parser = argparse.ArgumentParser(description='Performs sanity checks for a set '
    'of simulation results. For the first given pair of files, basic comparison '
    'is also performed.')
parser.add_argument('files', metavar='INFILE', nargs='+', help='input result files')
args = parser.parse_args()

EPS = 1e-6
def isin_eps(a, b, eps=EPS):
    b_s = np.sort(b)
    i = np.searchsorted(b_s, a)

    valid_mask = (i == len(b))
    i[valid_mask] = len(b)-1
    lval = b_s[i]-a
    lval[valid_mask] *= -1

    invalid_mask = (i == 0)
    j = i-1
    j[invalid_mask] = 0
    rval = a - b_s[j]
    rval[invalid_mask] *= -1
    return np.minimum(lval, rval) <= eps

for filename in args.files:
    res, t_step = results_fromfile(filename)
    x, v = res[:]['x'], res[:]['v']
    summary = textwrap.dedent('''\
        === Summary for {filename} ===
        Samples           | {n_samples} every {t_step} s
        Agents            | {n_agents}
        Speed range       | from {v_min:0.2f} to {v_max:0.2f} m/s
        Position range    | from {x_min:0.2f} to {x_max:0.2f} m''')
    print(summary.format(filename=filename, t_step=t_step,
            n_samples=res.shape[0], n_agents=res.shape[1],
            v_min=np.min(v), v_max=np.max(v),
            x_min=np.min(x), x_max=np.max(x)))

if len(args.files) >= 2:
    res_a, t_step_a = results_fromfile(args.files[0])
    res_b, t_step_b = results_fromfile(args.files[1])
    res_a = np.sort(res_a, axis=1, order='uid')
    res_b = np.sort(res_b, axis=1, order='uid')
    comparison = '=== Comparison for {} and {} ===\n'.format(
        args.files[0], args.files[1])
    if res_a.shape[1] != res_b.shape[1]:
        comparison += 'Results have a different number of agents, no further comparison is possible.'
    else:
        i, j = np.arange(len(res_a)), np.arange(len(res_b))
        overlap_a = isin_eps(t_step_a*i, t_step_b*j)
        overlap_b = isin_eps(t_step_b*j, t_step_a*i)
        i, j = i[overlap_a], j[overlap_b]
        v_a, v_b = res_a[i]['v'], res_b[j]['v']
        x_a, x_b = res_a[i]['x'], res_b[j]['x']
        comparison += textwrap.dedent('''\
            Sample overlap    | {overlap_count} (within {eps} s)
            % of {filename_a:12} | {overlap_a:0.1f}
            % of {filename_b:12} | {overlap_b:0.1f}
            Overlap range     | from {overlap_min:0.1f} to {overlap_max:0.1f} s
            Max speed diff    | {max_v_diff:0.3f} m/s
            Max position diff | {max_x_diff:0.3f} m''')
        comparison = comparison.format(overlap_count=len(i), eps=EPS,
            filename_a=args.files[0], filename_b=args.files[1],
            overlap_a=100*len(i)/len(res_a), overlap_b=100*len(i)/len(res_b),
            overlap_min=t_step_a*np.min(np.argwhere(overlap_a)),
            overlap_max=t_step_a*np.max(np.argwhere(overlap_a)),
            max_v_diff=np.max(np.abs(v_a-v_b)),
            max_x_diff=np.max(np.abs(x_a-x_b)))
    
    print(comparison)

