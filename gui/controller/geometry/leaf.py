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

from .object import GNObjectController
from ...utils.str import empty_to_none, none_to_empty
from ...qt import QtGui
from ...utils.qsignals import BlockQtSignals


class GNLeafController(GNObjectController):

    def construct_form(self):
        material_form = self.construct_group('Material')

        self.material_selection_type = QtGui.QComboBox()
        self.material_selection_type.addItems(['Solid', 'Gradual'])
        self.material_selection_type.currentIndexChanged.connect(self._save_material_in_model_undoable)

        self.material_solid = self.construct_material_combo_box(items=[''], change_cb=self._save_material_in_model_undoable)

        material_tb_hbox, material_tb_group = self._construct_hbox()
        self.material_bottom = self.construct_material_combo_box(items=[''], change_cb=self._save_material_in_model_undoable)
        self.material_top = self.construct_material_combo_box(items=[''], change_cb=self._save_material_in_model_undoable)
        material_tb_hbox.addWidget(self.material_bottom)
        material_tb_hbox.addWidget(self.material_top)

        self.material_group = QtGui.QStackedWidget()
        self.material_group.addWidget(self.material_solid)
        self.material_group.addWidget(material_tb_group)
        self.material_selection_type.currentIndexChanged.connect(self.material_group.setCurrentIndex)

        material_form.addRow(self.material_selection_type, self.material_group)

        super(GNLeafController, self).construct_form()

        self.construct_group('Meshing Settings')
        self.step_num = self.construct_line_edit('Maximum steps number:', node_property_name='step_num',
                                                 display_property_name='maximum steps number')
        self.step_num.setToolTip(u'&lt;{} <b>steps-num</b>="" steps-dist="" ...&gt;<br/>'
                                 u'Maximum number of the mesh steps in each direction the object is divided into '
                                 u'if it is non-uniform. (integer)'
                                 .format(self.node.tag_name(False)))
        self.step_dist = self.construct_line_edit('Minimum step size:', node_property_name='step_dist',
                                                  display_property_name='minimum step size')
        self.step_dist.setToolTip(u'&lt;{} steps-num="" <b>steps-dist</b>="" ...&gt;<br/>'
                                  u'Minimum step size if the object is non-uniform.'
                                  .format(self.node.tag_name(False)))

    def _save_material_in_model_undoable(self):
        def setter(n, v):
            n.material_bottom = v[0]
            n.material_top = v[1]

        if self.material_selection_type.currentIndex() == 0:
            m = empty_to_none(self.material_solid.currentText())
            new_material = (m, m)
        else:
            new_material = ( empty_to_none(self.material_bottom.currentText()), empty_to_none(self.material_top.currentText()) )

        self._set_node_by_setter_undoable(
            setter, new_material, (self.node.material_bottom, self.node.material_top), 'change material'
        )

        #if self.material_selection_type.currentIndex() == 0:
        #    self.node.set_material(empty_to_none(self.material_solid.currentText()))
        #else:
        #    self.node.material_bottom = empty_to_none(self.material_bottom.currentText())
        #    self.node.material_top = empty_to_none(self.material_top.currentText())

    def fill_form(self):
        super(GNLeafController, self).fill_form()
        with BlockQtSignals(self.material_selection_type, self.material_bottom, self.material_top, self.material_solid):
            index = 0 if self.node.is_solid() else 1
            self.material_selection_type.setCurrentIndex(index)
            self.material_group.setCurrentIndex(index)
            if self.node.is_solid():
                self.material_solid.setEditText(none_to_empty(self.node.material_top))
            else:
                self.material_bottom.setEditText(none_to_empty(self.node.material_bottom))
                self.material_top.setEditText(none_to_empty(self.node.material_top))
        self.step_num.setText(none_to_empty(self.node.step_num))
        self.step_dist.setText(none_to_empty(self.node.step_dist))


class GNBlockController(GNLeafController):

    def construct_form(self):
        self.construct_group('{} Settings'.format('Rectangle' if self.node.dim == 2 else 'Cuboid'))
        def setter(n, v): n.size = v
        self.size = self.construct_point_controllers(row_name='Size:', change_cb=lambda point:
            self._set_node_by_setter_undoable(setter, list(point), self.node.size, 'change block size')
        )
        super(GNBlockController, self).construct_form()

    def fill_form(self):
        super(GNBlockController, self).fill_form()
        for i in range(0, self.node.dim):
            self.size[i].setText(none_to_empty(self.node.size[i]))


class GNTriangleController(GNLeafController):

    def _on_point_set(self, index, value):
        def setter(n, v): n.points = n.points[0:index] + (v,) + n.points[index+1:]
        self._set_node_by_setter_undoable(setter, value, self.node.points[index],
            'change {} triangle point'.format('first' if index == 0 else 'second')
        )

    def construct_form(self):
        self.construct_group('Vertexes coordinates (other than: {}):'
                             .format(', '.join('0' for _ in range(0, self.node.dim))))
        self.p0 = self.construct_point_controllers(row_name='first', change_cb=lambda point: self._on_point_set(0, point))
        self.p1 = self.construct_point_controllers(row_name='second', change_cb=lambda point: self._on_point_set(1, point))
        super(GNTriangleController, self).construct_form()

    def fill_form(self):
        super(GNTriangleController, self).fill_form()
        for i in range(0, self.node.dim):
            self.p0[i].setText(none_to_empty(self.node.points[0][i]))
            self.p1[i].setText(none_to_empty(self.node.points[1][i]))


class GNCircleController(GNLeafController):

    def construct_form(self):
        self.construct_group('{} Size:'.format('Circle' if self.node.dim == 2 else 'Sphere'))
        self.radius = self.construct_line_edit('Radius:', unit=u'µm', node_property_name='radius')
        self.radius.setToolTip(u'&lt;{} <b>radius</b>="" ...&gt;<br/>'
                               u'Radius. (float [µm], required)'.format(self.node.tag_name(False)))
        super(GNCircleController, self).construct_form()

    def fill_form(self):
        super(GNCircleController, self).fill_form()
        self.radius.setText(none_to_empty(self.node.radius))


class GNCylinderController(GNLeafController):

    def construct_form(self):
        self.construct_group('Cylinder Size')
        self.radius = self.construct_line_edit('Radius:', unit=u'µm', node_property_name='radius', display_property_name='radius of the cylinder base')
        self.radius.setToolTip(u'&lt;cylinder <b>radius</b>="" height="" ...&gt;<br/>'
                               u'Radius of the cylinder base. (float [µm], required)')
        self.height = self.construct_line_edit('Height:', unit=u'µm', node_property_name='height', display_property_name='height of the cylinder')
        self.radius.setToolTip(u'&lt;cylinder radius="" <b>height</b>="" ...&gt;<br/>'
                               u'Height of the cylinder. (float [µm], required)')
        super(GNCylinderController, self).construct_form()

    def fill_form(self):
        super(GNCylinderController, self).fill_form()
        self.radius.setText(none_to_empty(self.node.radius))
        self.height.setText(none_to_empty(self.node.height))
