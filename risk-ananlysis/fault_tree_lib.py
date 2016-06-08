#!/usr/bin/env python3
# -*- coding:utf-8 -*-
#
# Copyright (C) 2016, Maximilian Köhl <mail@koehlma.de>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 3 as published by the Free
# Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this
# program. If not, see <http://www.gnu.org/licenses/>.

import collections


class Node:
    shape = 'box'
    label = 'None'
    parameters = {}

    def __init__(self, parent=None, *children, label=None, shape=None, **parameters):
        self.label = label or self.label
        self.shape = shape or self.shape
        self.parent = parent
        self.children = set(children)
        self.name = 'node{}'.format(id(self))
        self.parameters = parameters or dict(self.parameters)

    def graphviz_node(self):
        self.parameters['label'] = '"{}"'.format(self.label)
        self.parameters['shape'] = self.shape
        params = ('{}={}'.format(name, value) for name, value in self.parameters.items())
        return '{}[{}];'.format(self.name, ','.join(params))

    def graphviz_edges(self):
        return '\n'.join('{} -> {};'.format(child.name, self.name)
                         for child in self.children)


class Failure(Node):
    shape = 'box'

    def __init__(self, description=None, probability=None, **parameters):
        super().__init__(label=description, **parameters)
        self.description = description
        self.probability = probability

    def __lshift__(self, other):
        other.parent = self
        self.children.add(other)

    def __rshift__(self, other):
        return other << self

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        return False

    def __or__(self, other):
        if isinstance(other, OR):
            other.children.add(self)
            return other
        return OR(None, self, other)

    def __and__(self, other):
        if isinstance(other, AND):
            other.children.add(self)
            return other
        return AND(None, self, other)

    __ror__ = __or__
    __rand__ = __and__


class Toplevel(Failure):
    parameters = {
        'style': 'bold'
    }


class Primary(Failure):
    shape = 'circle'


class Secondary(Failure):
    shape = 'diamond'


class Gate(Node):
    shape = 'box'

    def __or__(self, other):
        if isinstance(other, Failure):
            self.children.add(other)
            return self
        return OR(self, other)

    def __and__(self, other):
        if isinstance(other, Failure):
            self.children.add(other)
            return self
        return AND(self, other)


class AND(Gate):
    label = 'AND'


class OR(Gate):
    label = 'OR'


def traverse(node):
    queue = collections.deque([node])
    while queue:
        current = queue.popleft()
        yield current
        queue.extend(current.children)


def graphviz(*nodes):
    code = ['digraph faulttree {', 'rankdir=BT;']
    all_nodes = set()
    for root in nodes:
        for node in traverse(root):
            all_nodes.add(node)
    for node in all_nodes:
        code.append(node.graphviz_node())
    for node in all_nodes:
        code.append(node.graphviz_edges())
    code.append('}')
    return '\n'.join(code)
