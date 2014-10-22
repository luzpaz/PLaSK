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

from ...model.geometry import GeometryModel
from ...model.geometry.constructor import construct_by_name
from ...model.geometry.types import geometry_types_geometries_core
from ...qt import QtGui, QtCore

from .. import Controller


class GeometryController(Controller):

    def fill_add_menu(self):
        self.add_menu.clear()
        for n in geometry_types_geometries_core.keys():
            a = QtGui.QAction(n, self.add_menu)
            a.triggered[()].connect(lambda n=n: self.append_geometry_node(n))
            self.add_menu.addAction(a)


    def update_actions(self):
        self.remove_action.setEnabled(not self.tree.selectionModel().selection().isEmpty())
        self.fill_add_menu()

        #hasCurrent = self.tree.selectionModel().currentIndex().isValid()
        #self.insertRowAction.setEnabled(hasCurrent)
        #self.insertColumnAction.setEnabled(hasCurrent)

    def append_geometry_node(self, type_name):
        self.tree.model().append_geometry(type_name)

    def remove_node(self):
        index = self.tree.selectionModel().currentIndex()
        model = self.tree.model()
        if (model.removeRow(index.row(), index.parent())):
            self.update_actions()

    def _construct_toolbar(self):
        toolbar = QtGui.QToolBar()

        self.add_menu = QtGui.QMenu()

        addButton = QtGui.QToolButton()
        addButton.setText('Add')
        addButton.setIcon(QtGui.QIcon.fromTheme('list-add', QtGui.QIcon(':/list-add.png')))
        addButton.setToolTip('Add new geometry object to the tree')
        addButton.setShortcut(QtCore.Qt.CTRL + QtCore.Qt.Key_Plus)
        addButton.setMenu(self.add_menu)
        addButton.setPopupMode(QtGui.QToolButton.InstantPopup)
        toolbar.addWidget(addButton)

        self.remove_action = QtGui.QAction(QtGui.QIcon.fromTheme('list-remove', QtGui.QIcon(':/list-remove.png')), '&Remove', toolbar)
        self.remove_action.setStatusTip('Remove selected node from the tree')
        # self.remove_action.setShortcut(QtCore.Qt.CTRL + QtCore.Qt.Key_Minus)
        self.remove_action.triggered.connect(self.remove_node)
        toolbar.addAction(self.remove_action)

        return toolbar

    def _construct_tree(self, model):
        self.tree = QtGui.QTreeView()
        self.tree.setModel(model)
        return self.tree

    def __init__(self, document, model=None):
        if model is None: model = GeometryModel()
        Controller.__init__(self, document, model)

        external = QtGui.QGroupBox()
        vbox = QtGui.QVBoxLayout()
        external.setLayout(vbox)

        vbox.addWidget(self._construct_toolbar())
        vbox.addWidget(self._construct_tree(model))
        self.tree.selectionModel().selectionChanged.connect(self.update_actions)
        self.update_actions()

        self.splitter = QtGui.QSplitter()
        self.splitter.addWidget(external)

    def get_widget(self):
        return self.splitter