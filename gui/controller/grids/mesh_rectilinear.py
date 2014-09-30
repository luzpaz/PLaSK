# -*- coding: utf-8 -*-
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

from .. import Controller
from ...qt import QtGui
from ...utils.str import empty_to_none


class AxisEdit(QtGui.QGroupBox):

    def __init__(self, title, allow_type_select = False, accept_non_regular = False, axis_model = None):
        super(AxisEdit, self).__init__(title)
        form_layout = QtGui.QFormLayout()
        self.allow_type_select = allow_type_select
        if not allow_type_select: self.accept_non_regular = accept_non_regular
        if self.allow_type_select:
            self.type = QtGui.QComboBox()
            self.type.addItems(['(auto-detected)', 'ordered', 'regular'])
            self.setToolTip('Type of axis. If auto-detected is selected, axis will be regular only if any of the start, stop or num attributes are specified (in other case it will be ordered).')
            form_layout.addRow("type", self.type)
        self.start = QtGui.QLineEdit()
        self.start.setToolTip('Position of the first point on the axis. (float [µm])')
        form_layout.addRow("start", self.start)
        self.stop = QtGui.QLineEdit()
        self.stop.setToolTip('Position of the last point on the axis. (float [µm])')
        form_layout.addRow("stop", self.stop)
        self.num = QtGui.QLineEdit()
        self.num.setToolTip('Number of the equally distributed points along the axis. (integer)')
        form_layout.addRow("num", self.num)
        if allow_type_select or accept_non_regular:
            self.points = QtGui.QTextEdit()
            self.points.setToolTip('Comma-separated list of the mesh points along this axis.')
            #self.points.setWordWrapMode(QtGui.QTextEdit.LineWrapMode)
            form_layout.addRow("points", self.points)
            if allow_type_select:
                self.points.setVisible(self.are_points_editable())
        self.setLayout(form_layout)
        if axis_model is not None: self.from_model(axis_model)

    def are_points_editable(self):
        if self.allow_type_select:
            return self.type.currentText() != 'regular'
        else:
            return self.accept_non_regular

    def to_model(self, axis_model):
        if self.allow_type_select:
            if self.type.currentIndex() == 0:
                axis_model.type = None
            else:
                axis_model.type = self.type.currentText()
        axis_model.start = empty_to_none(self.start.text())
        axis_model.stop = empty_to_none(self.stop.text())
        axis_model.num = empty_to_none(self.num.text())
        if self.are_points_editable():
            #axis_model.type = self.type.get
            axis_model.points = empty_to_none(self.points.toPlainText())
        else:
            axis_model.points = ''

    def from_model(self, axis_model):
        if self.allow_type_select:
            t = getattr(axis_model, 'type')
            if t is None:
                self.type.setCurrentIndex(0)
            else:
                self.type.setEditText(t)
        for attr_name in ('start', 'stop', 'num', 'points'):
            a = getattr(axis_model, attr_name)
            widget = getattr(self, attr_name, False)
            if widget: widget.setText('' if a is None else a)


class RectangularMesh1DConroller(Controller):
    """1D rectangular mesh (ordered or regular) script"""
    def __init__(self, document, model):
        super(RectangularMesh1DConroller, self).__init__(document=document, model=model)
        self.editor = AxisEdit(None, allow_type_select=False, accept_non_regular=not model.is_regular)

    def save_data_in_model(self):
        self.editor.to_model(self.model.axis)
        self.model.fire_changed()

    def on_edit_enter(self):
        self.editor.from_model(self.model.axis)

    def get_editor(self):
        return self.editor


class RectangularMeshConroller(Controller):
    """2D and 3D rectangular mesh script"""

    def __init__(self, document, model):
        super(RectangularMeshConroller, self).__init__(document=document, model=model)

        self.form = QtGui.QGroupBox()

        vbox = QtGui.QVBoxLayout()
        self.axis_edit = []
        for i in range(0, model.dim):
            self.axis_edit.append(AxisEdit(model.axis_tag_name(i), allow_type_select=True))
            vbox.addWidget(self.axis_edit[-1])
        vbox.addStretch()
        self.form.setLayout(vbox)

    def save_data_in_model(self):
        for i in range(0, self.model.dim):
            self.axis_edit[i].to_model(self.model.axis[i])
        self.model.fire_changed()

    def on_edit_enter(self):
        for i in range(0, self.model.dim):
            self.axis_edit[i].from_model(self.model.axis[i])

    def get_editor(self):
        return self.form
