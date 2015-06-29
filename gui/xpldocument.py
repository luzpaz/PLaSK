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

import shutil
from lxml import etree

try:
    import plask
except ImportError:
    plask = None

from .model import SectionModelTreeBased
from .controller.source import SourceEditController
from .controller.defines import DefinesController
if plask is not None:
    from .controller.geometry import GeometryController
from .controller.script import ScriptController
from .controller.multi import GUIAndSourceController
from .controller.solvers import SolversController
from .controller.connects import ConnectsController
from .controller import materials
from .controller.grids import GridsController
from .utils.xml import XML_parser, OrderedTagReader

#from .qt import QtGui


class XPLDocument(object):

    SECTION_NAMES = ['defines', 'materials', 'geometry', 'grids', 'solvers', 'connects', 'script']

    def __init__(self, window, filename=None):
        self.window = window
        self.defines = GUIAndSourceController(DefinesController(self))
        self.materials = GUIAndSourceController(materials.MaterialsController(self))
        from . import _DEBUG
        if plask is not None:
            self.geometry = GUIAndSourceController(GeometryController(self))
        else:
            self.geometry = SourceEditController(self, SectionModelTreeBased(XPLDocument.SECTION_NAMES[2]))
        self.grids = GUIAndSourceController(GridsController(self))
        self.script = ScriptController(self)
        #self.solvers = SourceEditController(self, SectionModelTreeBased(XPLDocument.SECTION_NAMES[4]))
        self.solvers = GUIAndSourceController(SolversController(self))
        self.connects = GUIAndSourceController(ConnectsController(self))
        self.loglevel = 'detail'

        self.controllers = (
            self.defines,
            self.materials,
            self.geometry,
            self.grids,
            self.solvers,
            self.connects,
            self.script
        )
        for c in self.controllers:
            c.model.changed.connect(self.on_model_change)
        self.filename = None
        self.set_changed(False)
        if filename: self.load_from_file(filename)

        if _DEBUG and plask is not None:  # very useful!
            from .utils.modeltest import ModelTest
            ModelTest(self.model_by_name('geometry'))
        # self.tree = etree()

        # self.undo_group = QtGui.QUndoGroup()    #does we need this?
        # for c in self.controllers:
        #     self.undo_group.addStack(c.model.undo_stack)

    coding = 'utf-8'

    def on_model_change(self, model, *args, **kwargs):
        """Slot called by model 'changed' signals when user edits any section model"""
        self.set_changed(True)

    def set_changed(self, changed=True):
        self.window.set_changed(changed)
        if not changed:
            for c in self.controllers:
                c.model.undo_stack.setClean()

    def load_from_file(self, filename):
        tree = etree.parse(filename, XML_parser)
        self.loglevel = tree.getroot().attrib.get('loglevel', 'detail')
        with OrderedTagReader(tree.getroot()) as r:
            for i, name in enumerate(XPLDocument.SECTION_NAMES):
                element = r.get(name)
                if element is not None:
                    self.model_by_index(i).set_file_xml_element(element, filename)
                else:
                    self.model_by_index(i).clear()
        self.filename = filename
        self.set_changed(False)

    def get_content(self, sections=None, update_lines=None):
        """
        Get document content in XPL format.
        :param sections: if given, only selected sections will be included in result, list or string (in such case all sections up to given one will be included)
        :param bool update_lines: if True number of lines in model will be updated (True by default only if all sections are stored)
        :return str: document content in XPL format
        """
        if isinstance(sections, basestring):
            sections = XPLDocument.SECTION_NAMES[:XPLDocument.SECTION_NAMES.index(sections)+1]
        if update_lines is None: update_lines = sections is None or sections == self.SECTION_NAMES
        data = '<plask loglevel="{}">\n\n'.format(self.loglevel)
        current_line_in_file = 3
        for c in self.controllers:
            if update_lines: c.update_line_numbers(current_line_in_file)
            element = c.model.get_file_xml_element()
            if len(element) or element.text:
                section_string = etree.tostring(element, encoding="UTF-8", pretty_print=True)
                lines_count = section_string.count('\n') + 1
                current_line_in_file += lines_count
                if sections is None or c.model.name in sections:
                    data += section_string + '\n'
                else:
                    data += '\n' * lines_count
        data += '</plask>\n'
        return data

    def save_to_file(self, filename):
        # safe write: maybe inefficient, but does not destroy the file if the error occurs
        data = self.get_content()
        try:
            shutil.copyfile(filename, filename+'.bak')
        except (IOError, OSError):
            pass
        open(filename, 'w').write(data)
        self.filename = filename
        self.set_changed(False)

    def controller_by_index(self, index):
        return self.controllers[index]

    def controller_by_name(self, section_name):
        return self.controllers[XPLDocument.SECTION_NAMES.index(section_name)]

    def model_by_index(self, index):
        return self.controller_by_index(index).model

    def model_by_name(self, section_name):
        return self.controller_by_name(section_name).model

    def get_info(self, level=None):
        """Get messages from all models, on given level (all by default)."""
        res = []
        for c in self.controllers: res.extend(c.model.get_info(level))
        return res

    def stubs(self):
        return '\n'.join(s for s in (c.model.stubs() for c in self.controllers) if s)

    def set_loglevel(self, loglevel):
        loglevel = loglevel.lower()
        if self.loglevel != loglevel:
            self.loglevel = loglevel
            self.set_changed()