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

from lxml import etree as ElementTree

from .controller.script import ScriptController


class PyDocument(object):

    SECTION_NAMES = ["script"]

    def __init__(self, window, filename=None):
        self.window = window
        self.controller = ScriptController(self)
        self.controller.model.changed.connect(self.on_model_change)
        self.controllers = [
            None,   # defines
            None,   # materials
            None,   # geometry
            None,   # grids
            None,   # solvers
            None,   # connects
            self.controller  # script
        ]
        self.filename = None
        self.set_changed(False)
        if filename: self.load_from_file(filename)

    def on_model_change(self, model, *args, **kwargs):
        """Slot called by model 'changed' signals when user edits any section model"""
        self.set_changed(True)

    def set_changed(self, changed=True):
        self.window.set_changed(changed)

    def load_from_file(self, filename):
        data = open(filename, 'r').read()
        self.controller.model.set_text(data)
        self.filename = filename
        self.set_changed(False)

    def save_to_file(self, filename):
        open(filename, 'w').write(self.controller.model.get_text())
        self.filename = filename
        self.set_changed(False)

    def controller_by_index(self, index):
        return self.controller

    def controller_by_name(self, section_name):
        return self.controller

    def get_info(self, level=None):
        """Get messages from all models, on given level (all by default)."""
        return self.controller.model.get_info(level)

    def stubs(self):
        return ""
