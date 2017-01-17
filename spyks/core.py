# -*- coding: utf-8 -*-
# -*- mode: python -*-
"""Core functions for model descriptors

In spyks, models are specified by a map with following required keys: name
(string), state (mapping), equations (mapping), and parameters (mapping). Optional
keys include forcing (mapping) and reset (with predicate and state).

The convention in this package is that "mapping" is a list of (name, value)
tuples. Values must be specified with physical units (e.g., '5 mV') to permit
dimensional analysis; if units are not specified the quantity is assumed to be
dimensionless. An array, on the other hand, does not have names or quantities.

"""

import sympy as sp
import pint as pq

# need a global unit registry in order for quantities to be compatible
ureg = pq.UnitRegistry()

def n_params(model):
    """ Number of parameters in model """
    return len(model["parameters"])


def n_state(model):
    """ Number of state variables in model """
    return len(model["equations"])


def n_forcing(model):
    return len(model["forcing"])


def symbols(model):
    lh = set()
    rh = set()
    for n, eq in model["equations"]:
        lh.add(sp.Symbol(n))
        rh.update(eq.free_symbols)
    try:
        reset = model["reset"]
        rh.update(reset["predicate"])
        for n, eq in reset["state"]:
            rh.update(eq.free_symbols)
    except KeyError:
        pass
    return lh, rh


def to_array(mapping):
    """ Converts named vector of quantities into a dimensionless array. """
    from numpy import fromiter
    return fromiter((v.magnitude for k, v in mapping), dtype='d', count=len(mapping))


def to_mapping(array, mapping):
    """ Binds values in array to names and units in mapping. Size of array must match mapping. """
    assert len(array) == len(mapping)
    return [(k, x * getattr(v, 'units', v)) for  (k, v), x in zip(mapping, array) ]


def parse_quantity(x):
    if isinstance(x, (float, int)):
        return x * ureg.dimensionless
    else:
        return ureg.parse_expression(x)

def prune_symbols(model):
    """Removes state variables and parameters not used in the equations or reset functions"""
    pass


def load_model(fname):
    """ Loads a model descriptor file. """
    import ruamel.yaml as yaml
    fp = open(fname, "r")
    model = yaml.load(fp, yaml.RoundTripLoader)  # preserves order
    # parse equations
    model['equations'] = [(n, sp.sympify(s)) for n,s in model['equations'].items()]
    try:
        reset = model["reset"]
        reset["predicate"] = sp.sympify(reset["predicate"])
        reset["state"] = [(n, sp.sympify(s)) for n,s in reset["state"].items()]
    except KeyError:
        pass
    # parse quantities and units
    for k in ('parameters', 'forcing', 'state'):
        if k in model:
            model[k] = [(param, parse_quantity(value)) for param, value in model[k].items()]
    return model


def update_model(model, **kwargs):
    """Updates model with arrays of values.

    Keyword arguments (e.g., parameters, forcing, state) must match existing
    keys, and array size must match size of model vector.

    """
    for key, array in kwargs.items():
        model[key] = to_mapping(array, model[key])
