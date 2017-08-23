# coding: utf8
# Copyright (C) 2014 Photonics Group, Lodz University of Technology
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of GNU General Public License as published by the
# Free Software Foundation; either version 2 of the license, or (at your
# opinion) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import os
from lxml import etree

from ...utils.xml import AttributeReader, print_interior
from . import Solver, SOLVERS, CATEGORIES
from .bconds import BCONDS, SchemaBoundaryConditions

try:
    unicode = unicode
except NameError:
    # 'unicode' is undefined, must be Python 3
    unicode = str
    basestring = (str, bytes)
else:
    # 'unicode' exists, must be Python 2
    bytes = str

try:
    import plask
except ImportError:
    plask = None


class AttrList(list):
    @property
    def flat(self):
        for item in self:
            if isinstance(item, AttrGroup):
                for i in item:
                    yield i
            else:
                yield item


class AttrGroup(list):
    def __init__(self, label):
        self.label = label


class Attr(object):
    def __init__(self, tag, name, label, help, default=None):
        self.tag = tag
        self.name = name
        self.label = label
        self.help = help
        self.default = default
        self.conflicts = set()

class AttrMulti(Attr):
    pass


class AttrChoice(Attr):
    def __init__(self, tag, name, label, help, choices, default=None):
        super(AttrChoice, self).__init__(tag, name, label, help, default)
        self.choices = choices


class AttrBool(AttrChoice):
    def __init__(self, tag, name, label, help, default=None):
        super(AttrBool, self).__init__(tag, name, label, help, ('yes', 'no'), default)


class AttrGeometry(Attr):
    def __init__(self, tag, name, label, help, geometry_type, default=None):
        super(AttrGeometry, self).__init__(tag, name, label, help, default)
        self.type = geometry_type
    pass


class AttrGeometryObject(Attr):
    pass


class AttrGeometryPath(Attr):
    pass


class AttrMesh(AttrChoice):
    pass


def read_attr(tn, attr, xns):
        an = attr.attrib['name']
        al = attr.attrib['label']
        ah = attr.text
        at = attr.attrib.get('type', '')
        au = attr.attrib.get('unit')
        ad = attr.attrib.get('default')
        if au is not None:
            al += u' [{}]'.format(au)
        else:
            au = attr.getparent().attrib.get('unit')
        if au is not None:
            at += u' [{}]'.format(au)
        if at == u'choice':
            ac = tuple(ch.text.strip() for ch in attr.findall(xns+'choice'))
            result = AttrChoice(tn, an, al, ah, ac, ad)
        elif at == u'bool':
            result = AttrBool(tn, an, al, ah, ad)
        elif at == u'geometry object':
            result = AttrGeometryObject(tn, an, al, ah)
        elif at == u'geometry path':
            result = AttrGeometryPath(tn, an, al, ah)
        elif at.startswith(u'geometry '):
            result = AttrGeometry(tn, an, al, ah, at[9:])
        elif at == u'mesh':
            ac = tuple(ch.text.strip() for ch in attr.findall(xns + 'type'))
            result = AttrMesh(tn, an, al, ah, ac)
        else:
            if at:
                ah += u' ({})'.format(at)
            if an.endswith('#'):
                result = AttrMulti(tn, an, al, ah)
            else:
                result = Attr(tn, an, al, ah, ad)
        for conflict in attr.findall(xns+'conflicts'):
            ct = conflict.attrib.get('tag', tn)
            ca = conflict.attrib['attr']
            result.conflicts.add((ct, ca))
        return result


class SchemaTag(object):
    def __init__(self, name, label, attrs):
        self.name = name
        self.label = label
        self.attrs = attrs


class AutoSolver(Solver):
    """Model for solver with its configuration specified in a scheme file solvers.xml
       and automatically generated controller widget"""

    def __init__(self, category, schema, lib=None, solver='', geometry_type=None, mesh_types=None, name='',
                 parent=None, info_cb=None):
        super(AutoSolver, self).__init__(category, solver, name, parent, info_cb)
        self.lib = lib
        self.schema = schema
        self.geometry_type = geometry_type
        self.mesh_types = mesh_types
        self.set_fresh_data()

    def set_fresh_data(self):
        self.data = dict((schema.name,
                          dict((a.name, None) for a in schema.attrs.flat) if isinstance(schema, SchemaTag) else [])
                         for schema in self.schema)

    def get_xml_element(self):
        element = etree.Element(self.category, {'name': self.name, 'solver': self.solver})
        if self.lib is not None:
            element.attrib['lib'] = self.lib
        if self.geometry:
            etree.SubElement(element, 'geometry', {'ref': self.geometry})
        if self.mesh:
            etree.SubElement(element, 'mesh', {'ref': self.mesh})
        for schema in self.schema:
            tag = schema.name
            data = self.data[tag]
            if isinstance(schema, SchemaTag):
                attrs = dict((item for item in data.items() if item[1] and item[0][-1] != '#'))
                if attrs:
                    if '/' in tag:
                        path = tag.split('/')
                        tag = path[-1]
                        el = element
                        for tg in path[:-1]:
                            f = el.find(tg)
                            if f is not None:
                                el = f
                            else:
                                el = etree.SubElement(el, tg)
                        etree.SubElement(el, tag, attrs)
                    else:
                        etree.SubElement(element, tag, attrs)
            elif isinstance(schema, SchemaBoundaryConditions):
                element.append(schema.to_xml(data))
            else:
                if data:
                    if not isinstance(data, str): data = data.encode('utf8')
                    lines = data.split('\n')
                    if not lines[-1]: lines = lines[:-1]
                    lines = '\n    '.join(lines)
                    el = etree.fromstringlist(['<', tag, '>\n    ', lines, '\n  </', tag, '>'])
                    element.append(el)
        return element

    def set_xml_element(self, element):
        self.set_fresh_data()
        super(AutoSolver, self).set_xml_element(element)
        el = element.find('geometry')
        if el is not None:
            self.geometry = el.attrib.get('ref')
            #TODO report missing or mismatching geometry
        if self.mesh_types:
            el = element.find('mesh')
            if el is not None:
                self.mesh = el.attrib.get('ref')
                #TODO report missing or mismatching mesh
        for schema in self.schema:
            el = element.find(schema.name)
            if el is not None:
                if isinstance(schema, SchemaTag):
                    try:
                        data = self.data[schema.name]
                    except KeyError:
                        pass
                    else:
                        with AttributeReader(el) as attrs:
                            for name in attrs:
                                if name+'#' in data:
                                    data[name+'0'] = attrs[name]
                                else:
                                    data[name] = attrs[name]
                elif isinstance(schema, SchemaBoundaryConditions):
                    self.data[schema.name] = schema.from_xml(el)
                else:
                    self.data[schema.name] = print_interior(el)

    def get_controller(self, document):
        from ...controller.solvers.autosolver import AutoSolverController
        return AutoSolverController(document, self)

    def stub(self):
        if self.lib is not None:
            return "import {1}.{2}.{3} as {0}\n{0} = {0}()".format(self.name, self.category, self.lib, self.solver)
        else:
            return "import {1}.{2} as {0}\n{0} = {0}()".format(self.name, self.category, self.solver)


class AutoSolverFactory(object):

    def __init__(self, category, lib, solver, schema, geometry_type, mesh_types, providers, receivers):
        self.category = category
        self.solver = solver
        self.schema = schema
        self.geometry_type = geometry_type
        self.mesh_types = mesh_types
        self.lib = lib
        self.providers = providers
        self.receivers = receivers

    def __call__(self, name='', parent=None, info_cb=None, element=None):
        result = AutoSolver(self.category, self.schema, self.lib, self.solver, self.geometry_type, self.mesh_types,
                            name, parent, info_cb)
        if element is not None:
            result.set_xml_element(element)
        if self.lib is not None and result.lib != self.lib:
            result.lib = self.lib  # force solver lib
        return result


def _iter_tags(parent, xns):
    for tag in parent.findall(xns+'tag'):
        yield tag
        for t in _iter_tags(tag, xns):
            t.attrib['name'] = tag.attrib['name'] + '/' + t.attrib['name']
            yield t


def load_xml(filename, categories=CATEGORIES, solvers=SOLVERS):

    dom = etree.parse(filename)
    root = dom.getroot()

    xns = root.nsmap.get(None, '')
    if xns: xns = '{'+xns+'}'

    if root.tag != xns+'solvers': return

    for solver in root:
        if solver.tag != xns+'solver': continue
        if solver.attrib.get('obsolete'): continue

        name = solver.attrib.get('name')
        if name is None: return

        cat = solver.attrib.get('category')
        if cat is None:
            raise ValueError('Unspecified category')
        lib = solver.attrib.get('lib', os.path.basename(filename)[:-4])

        g = solver.find(xns+'geometry')
        try:
            geometry_type = g.attrib['type']
        except (KeyError, AttributeError):
            geometry_type = None
            # if name.endswith('Cyl'):
            #     geometry_type = "Cylindrical"
            # else:
            #     geometry_type = "Cartesian" + name[-2:]

        mesh_types = set()
        m = solver.find(xns+'mesh')
        if m is not None:
            try:
                mesh_type = m.attrib['type']
            except (KeyError):
                mesh_type = None
            else:
                mesh_types.add(mesh_type)
            for t in m.findall(xns+'type'):
                mesh_types.add(t.text)
        else:
            mesh_type = None

        schema = []

        for tag in _iter_tags(solver, xns):
            tn, tl = tag.attrib['name'], tag.attrib['label']
            attrs = AttrList()
            for attr in tag.iterchildren(xns+'attr', xns+'group'):
                if attr.tag == xns+'attr':
                    attrs.append(read_attr(tn, attr, xns))
                elif attr.tag == xns+'group':
                    gl = attr.attrib['label']
                    gu = attr.attrib.get('unit')
                    if gu is not None:
                        gl += u' [{}]'.format(gu)
                    group = AttrGroup(gl)
                    for attr in attr.findall(xns+'attr'):
                        group.append(read_attr(tn, attr, xns))
                    attrs.append(group)
            schema.append(SchemaTag(tn, tl, attrs))

        for bcond in solver.findall(xns+'bcond'):
            values = bcond.attrib.get('values')
            if values is not None: values = values.split(',')
            BCond = BCONDS[bcond.attrib.get('type', mesh_type)]
            schema.append(BCond(bcond.attrib['name'], bcond.attrib['label'], mesh_type, values))

        flow = solver.find(xns+'flow')
        if flow is not None:
            providers = [(e.attrib['name'], e.attrib.get('for', e.attrib['name'][3:]))
                         for e in flow.findall(xns+'provider')]
            receivers = [(e.attrib['name'], e.attrib.get('for', e.attrib['name'][2:]))
                         for e in flow.findall(xns+'receiver')]
        else:
            providers = []
            receivers = []

        if cat not in categories:
            categories.append(cat)
        solvers[cat,name] = AutoSolverFactory(cat, lib, name, schema, geometry_type, mesh_types, providers, receivers)


# Find XML files with solvers configuration
from os.path import dirname as _d

for _dirname, _, _files in os.walk(os.path.join(_d(_d(_d(_d(__file__)))), 'solvers')):
    if os.path.split(_dirname)[-1] == 'skel': continue
    for _f in _files:
        if _f.endswith('.xml'):
            load_xml(os.path.join(_dirname, _f))
