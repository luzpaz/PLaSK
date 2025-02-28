# This file is part of PLaSK (https://plask.app) by Photonics Group at TUL
# Copyright (c) 2022 Lodz University of Technology
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import sys
import weakref

from ...qt.QtCore import *
from ...qt.QtWidgets import *
from ...qt.QtGui import *
from ...qt import QtSlot, qt_exec
from ...model.geometry import GeometryModel
from ...model.geometry.types import geometry_types_geometries_core, gname
from ...model.geometry.geometry import GNGeometryBase
from ...model.info import Info
from ...model.materials import HandleMaterialsModule
from .. import Controller
from ...utils import get_manager
from ...utils.widgets import HTMLDelegate, VerticalScrollArea, create_undo_actions, set_icon_size, ComboBox
from ...utils.config import CONFIG

try:
    import plask
except ImportError:
    pass  # TODO: make preview optional



try:
    from .plot_widget import PlotWidget
except ImportError:
    PlotWidget = None


class GeometryTreeView(QTreeView):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._current_index = None

    def expand_children(self, index):
        if not index.isValid(): return

        model = index.model()
        for i in range(model.rowCount(index)):
            self.expand_children(model.index(i, 0, parent=index))

        if not self.isExpanded(index):
            self.expand(index)

    @QtSlot()
    def update_current_index(self):
        if self._current_index is not None:
            self.setCurrentIndex(self._current_index)
            self._current_index = None


class GeometryController(Controller):
    # TODO use ControllerWithSubController (?)

    def _add_child(self, type_constructor, parent_index):
        parent = parent_index.internalPointer()
        pos = parent.new_child_pos()
        self.model.insert_node(parent, type_constructor(None, None))
        self.tree.setExpanded(parent_index, True)
        new_index = self.model.index(pos, 0, parent_index)
        self.tree.selectionModel().select(new_index,
                                          QItemSelectionModel.SelectionFlag.Clear | QItemSelectionModel.SelectionFlag.Select |
                                          QItemSelectionModel.SelectionFlag.Rows)
        self.tree.setCurrentIndex(new_index)
        self.update_actions()

    def _make_add_item_menu(self, geometry_node_index, menu, label):
        geometry_node = geometry_node_index.internalPointer()
        if geometry_node is None or not geometry_node.accept_new_child(): return
        first = True
        submenu = menu.addMenu(label)
        weakself = weakref.proxy(self)
        for section in geometry_node.add_child_options():
            if not first:
                submenu.addSeparator()
            first = False
            for type_name, type_constructor in section.items():
                if type_name.endswith('2d') or type_name.endswith('3d'):
                    type_name = type_name[:-2]
                a = QAction(gname(type_name, True), submenu)
                a.triggered.connect(lambda checked=False, type_constructor=type_constructor,
                                           parent_index=geometry_node_index:
                                    weakself._add_child(type_constructor, parent_index))
                submenu.addAction(a)

    def _reparent(self, index, type_constructor):
        parent_index = index.parent()
        if not parent_index.isValid(): return
        new_index = self.model.reparent(index, type_constructor)
        self.tree.setExpanded(new_index, True)
        self.tree.selectionModel().select(new_index,
                                          QItemSelectionModel.SelectionFlag.Clear | QItemSelectionModel.SelectionFlag.Select |
                                          QItemSelectionModel.SelectionFlag.Rows)
        self.tree.setCurrentIndex(new_index)
        self.update_actions()

    def _get_reparent_menu(self, index):
        parent_index = index.parent()
        if not parent_index.isValid(): return
        node = index.internalPointer()
        if node is None: return
        first = True
        result = QMenu(self.reparent_button)
        for section in node.add_parent_options(parent_index.internalPointer()):
            if not first:
                result.addSeparator()
            first = False
            for type_name, type_constructor in section.items():
                if type_name.endswith('2d') or type_name.endswith('3d'):
                    type_name = type_name[:-2]
                a = QAction(gname(type_name, True), result)
                a.triggered.connect(lambda checked=False, type_constructor=type_constructor, index=index:
                                        self._reparent(index, type_constructor))
                result.addAction(a)
        return result

    def __init__(self, document, model=None):
        if model is None: model = GeometryModel()
        super().__init__(document, model)

        weakself = weakref.proxy(self)

        self.manager = None
        self.plotted_object = None

        self.plotted_tree_element = None
        self.model.changed.connect(self.on_model_change)

        self._current_index = None
        self._current_root = None
        self._last_index = None
        self._current_controller = None

        self._lims = None

        tree_with_buttons = QGroupBox()
        vbox = QVBoxLayout()
        tree_with_buttons.setLayout(vbox)

        tree_with_buttons.setTitle("Geometry Tree")
        vbox.setContentsMargins(0, 0, 0, 0)
        vbox.setSpacing(0)

        self._construct_tree(model)
        vbox.addWidget(self._construct_toolbar())
        vbox.addWidget(self.tree)
        tree_selection_model = self.tree.selectionModel()   # workaround of segfault in pySide,
        # see http://stackoverflow.com/questions/19211430/pyside-segfault-when-using-qitemselectionmodel-with-qlistview
        tree_selection_model.selectionChanged.connect(self.object_selected)
        self.update_actions()

        self.checked_plane = '12'

        self.vertical_splitter = QSplitter()
        self.vertical_splitter.setOrientation(Qt.Orientation.Vertical)

        self.vertical_splitter.addWidget(tree_with_buttons)

        self.parent_for_editor_widget = VerticalScrollArea()
        self.vertical_splitter.addWidget(self.parent_for_editor_widget)

        self.main_splitter = QSplitter()
        self.main_splitter.addWidget(self.vertical_splitter)
        self.main_splitter.setHandleWidth(4)

        search_action = QAction(QIcon.fromTheme('edit-find'), '&Search', self.main_splitter)
        CONFIG.set_shortcut(search_action, 'geometry_search')
        search_action.triggered.connect(lambda checked=False: weakself.search_combo.setFocus())
        self.main_splitter.addAction(search_action)

        if PlotWidget is not None:
            self.geometry_view = PlotWidget(self, picker=True)
            self.geometry_view.canvas.mpl_connect('pick_event', self.on_pick_object)
            self.main_splitter.addWidget(self.geometry_view)
        else:
            self.geometry_view = None

        self.document.window.config_changed.connect(self.reconfig)

        self.model.dropped.connect(lambda index: weakself.tree.setCurrentIndex(index))


    def fill_add_menu(self):
        weakself = weakref.proxy(self)
        self.add_menu.clear()
        current_index = self.tree.selectionModel().currentIndex()
        if current_index.isValid():
            self._make_add_item_menu(current_index, self.add_menu, "&Item")
        for n in geometry_types_geometries_core.keys():
            a = QAction(gname(n, True), self.add_menu)
            a.triggered.connect(lambda checked=False, n=n: weakself.append_geometry_node(n))
            self.add_menu.addAction(a)

    def set_reparent_menu(self):
        current_index = self.tree.selectionModel().currentIndex()
        if current_index.isValid():
            reparent_menu = self._get_reparent_menu(current_index)
            if reparent_menu is not None and not reparent_menu.isEmpty():
                self.reparent_button.setMenu(reparent_menu)
                self.reparent_button.setEnabled(True)
                return
        self.reparent_button.setMenu(None)
        self.reparent_button.setEnabled(False)

    def update_actions(self):
        has_selected_object = not self.tree.selectionModel().selection().isEmpty()
        self.remove_action.setEnabled(has_selected_object)
        # self.plot_action.setEnabled(has_selected_object)
        self.fill_add_menu()

        u, d = self.model.can_move_node_up_down(self.tree.selectionModel().currentIndex())
        self.move_up_action.setEnabled(u)
        self.move_down_action.setEnabled(d)

        parent = self.tree.selectionModel().currentIndex().parent()
        self.duplicate_action.setEnabled(parent.isValid() and parent.internalPointer().accept_new_child())

        self.set_reparent_menu()

    def append_geometry_node(self, type_name):
        self.tree.model().append_geometry(type_name)
        new_index = self.model.index(len(self.tree.model().roots)-1, 0)
        self.tree.selectionModel().select(new_index,
                                          QItemSelectionModel.SelectionFlag.Clear | QItemSelectionModel.SelectionFlag.Select |
                                          QItemSelectionModel.SelectionFlag.Rows)
        self.tree.setCurrentIndex(new_index)
        self.update_actions()

    def on_tree_context_menu(self, pos):
        index = self.tree.indexAt(pos)
        if not index.isValid(): return

        weakself = weakref.proxy(self)

        menu = QMenu(self.tree)
        self._make_add_item_menu(index, menu, "&Add item")
        menu.addAction("&Remove", lambda: weakself.remove_node(index))
        u, d = self.model.can_move_node_up_down(index)
        if u: menu.addAction("Move &up", lambda: weakself.move_up(index))
        if d: menu.addAction("Move d&own", lambda: weakself.move_down(index))

        parent = index.parent()
        if parent.isValid():
            parent_node = parent.internalPointer()
            if parent_node.accept_new_child():
                menu.addAction("&Duplicate", lambda: weakself.duplicate(index))

        reparent_menu = self._get_reparent_menu(index)
        if reparent_menu and not reparent_menu.isEmpty():
            if not menu.isEmpty(): menu.addSeparator()
            menu.addMenu(reparent_menu).setText("&Insert into")

        qt_exec(menu, self.tree.mapToGlobal(pos))

    def remove_node(self, index):
        model = self.tree.model()
        if model.removeRow(index.row(), index.parent()):
            self.update_actions()

    def remove_current_node(self):
        self.remove_node(self.tree.selectionModel().currentIndex())

    def _swap_neighbour_nodes(self, parent_index, row1, row2):
        if self.model.is_read_only(): return
        if row2 < row1: row1, row2 = row2, row1
        children = self.model.children_list(parent_index)
        if row1 < 0 or row2 < len(children): return
        self.model.beginMoveRows(parent_index, row2, row2, parent_index, row1)
        children[row1], children[row2] = children[row2], children[row1]
        self.model.endMoveRows()
        self.fire_changed()

    def move_up(self, index):
        self.model.move_node_up(index)
        self.update_actions()

    def move_current_up(self):
        self.move_up(self.tree.selectionModel().currentIndex())

    def move_down(self, index):
        self.model.move_node_down(index)
        self.update_actions()

    def move_current_down(self):
        self.move_down(self.tree.selectionModel().currentIndex())

    def duplicate(self, index):
        new_index = self.model.duplicate(index)
        self.update_actions()
        self.tree.setCurrentIndex(new_index)
        return new_index

    def duplicate_current(self):
        return self.duplicate(self.tree.selectionModel().currentIndex())

    def on_pick_object(self, event):
        # This seems as an ugly hack, but in reality this is the only way to make sure
        # that `setCurrentIndex` is called only once if there are multiple artists in
        # the clicked spot.
        self.tree._current_index = self.model.index_for_node(
            self.plotted_tree_element.get_node_by_real_path(event.artist.plask_real_path))
        QMetaObject.invokeMethod(self.tree, 'update_current_index', Qt.ConnectionType.QueuedConnection)

    def plot_element(self, tree_element, set_limits):
        self.model.clear_info_messages()
        try:
            if self.manager is None:
                manager = get_manager()
            else:
                manager = self.manager
                manager.geo.clear()
                manager.pth.clear()
                manager._roots.clear()
                self.plotted_object = None
            with HandleMaterialsModule(self.document):
                manager.load(self.document.get_contents(sections=('defines', 'materials', 'geometry')))
            self.manager = manager
            try:
                plotted_object = self.model.fake_root.get_corresponding_objects(tree_element, manager)[0]
            except (TypeError, ValueError):
                return False
            if tree_element != self.plotted_tree_element:
                try:
                    self.geometry_view.toolbar._nav_stack.clear()
                except AttributeError:
                    self.geometry_view.toolbar._views.clear()
            self.geometry_view.update_plot(plotted_object, set_limits=set_limits, plane=self.checked_plane)
        except Exception as e:
            self.geometry_view.set_info_message("Could not update geometry view: {}".format(str(e)), Info.ERROR)
            from ... import _DEBUG
            if _DEBUG:
                import traceback
                traceback.print_exc()
                sys.stderr.flush()
            res = False
        else:
            self.geometry_view.set_info_message()
            self.plotted_tree_element = tree_element
            self.plotted_object = plotted_object
            if tree_element.dim == 3:
                self.geometry_view.toolbar.enable_planes(tree_element.get_axes_conf())
            else:
                self.geometry_view.toolbar.disable_planes(tree_element.get_axes_conf())
            self.show_selection()
            res = True
        for err in manager.errors:
            self.model.add_info_message(err, Info.WARNING)
        self.model.refresh_info()
        return res

    def plot(self, tree_element=None):
        if tree_element is None:
            current_index = self.tree.selectionModel().currentIndex()
            if not current_index.isValid(): return
            tree_element = current_index.internalPointer()
        self.plot_element(tree_element, set_limits=True)
        self.geometry_view.toolbar.clear_history()
        self.geometry_view.toolbar.push_current()

    def plot_refresh(self):
        self.plot_element(self.plotted_tree_element, set_limits=False)

    def reconfig(self):
        if self.geometry_view is not None:
            colors = CONFIG['geometry/material_colors'].copy()
            self.geometry_view.get_color = plask.ColorFromDict(colors, self.geometry_view.axes)
            self.geometry_view.axes.set_facecolor(CONFIG['plots/face_color'])
            self.geometry_view.axes.grid(True, color=CONFIG['plots/grid_color'])
            if self.plotted_tree_element is not None and self.get_widget().isVisible():
                self.plot_refresh()
            else:
                self.geometry_view.canvas.draw()

    def on_model_change(self, *args, **kwargs):
        if self.plotted_tree_element is not None:
            self.model.clear_info_messages()
            if self.plot_auto_refresh:
                self.plot_refresh()
            else:
                self.show_update_required()

    def show_update_required(self):
        if self._current_controller is not None and self.geometry_view is not None:
            self.geometry_view.set_info_message("Geometry changed: click here to update the plot", Info.INFO, action=self.plot)

    def _construct_toolbar(self):
        weakself = weakref.proxy(self)

        toolbar = QToolBar()
        toolbar.setStyleSheet("QToolBar { border: 0px }")
        set_icon_size(toolbar)

        create_undo_actions(toolbar, self.model, toolbar)
        toolbar.addSeparator()

        self.add_menu = QMenu()

        add_button = QToolButton()
        add_button.setText('Add Item')
        add_button.setIcon(QIcon.fromTheme('list-add'))
        add_button.setToolTip('Add Item')
        add_button.setStatusTip('Add new geometry object to the tree')
        CONFIG.set_shortcut(add_button, 'entry_add')
        add_button.setMenu(self.add_menu)
        add_button.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        toolbar.addWidget(add_button)

        self.remove_action = QAction(QIcon.fromTheme('list-remove'), '&Remove Item', toolbar)
        self.remove_action.setStatusTip('Remove selected entry from the tree')
        CONFIG.set_shortcut(self.remove_action, 'entry_remove')
        self.remove_action.triggered.connect(self.remove_current_node)
        toolbar.addAction(self.remove_action)

        self.move_up_action = QAction(QIcon.fromTheme('go-up'), 'Move &Up', toolbar)
        self.move_up_action.setStatusTip('Change order of entries: move current entry up')
        CONFIG.set_shortcut(self.move_up_action, 'entry_move_up')
        self.move_up_action.triggered.connect(self.move_current_up)
        toolbar.addAction(self.move_up_action)

        self.move_down_action = QAction(QIcon.fromTheme('go-down'), 'Move D&own', toolbar)
        self.move_down_action.setStatusTip('Change order of entries: move current entry down')
        CONFIG.set_shortcut(self.move_down_action, 'entry_move_down')
        self.move_down_action.triggered.connect(self.move_current_down)
        toolbar.addAction(self.move_down_action)

        self.duplicate_action = QAction(QIcon.fromTheme('edit-copy'), '&Duplicate', toolbar)
        self.duplicate_action.setStatusTip('Duplicate current entry and insert it '
                                           'into default position of the same container')
        CONFIG.set_shortcut(self.duplicate_action, 'entry_duplicate')
        self.duplicate_action.triggered.connect(self.duplicate_current)
        toolbar.addAction(self.duplicate_action)

        toolbar.addSeparator()

        self.reparent_button = QToolButton(toolbar)
        self.reparent_button.setIcon(QIcon.fromTheme('object-group'))
        self.reparent_button.setText("Insert into")
        self.reparent_button.setToolTip("Insert into")
        self.reparent_button.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        CONFIG.set_shortcut(self.reparent_button, 'geometry_reparent')
        toolbar.addWidget(self.reparent_button)

        toolbar.addSeparator()

        props_button = QToolButton(toolbar)
        props_button.setIcon(QIcon.fromTheme('document-properties'))
        props_button.setText("Show Properties")
        props_button.setToolTip("Show Properties")
        props_button.setStatusTip("Show item properties in the tree view")
        props_button.setCheckable(True)
        props_button.setChecked(True)
        CONFIG.set_shortcut(props_button, 'geometry_show_props')
        props_button.toggled.connect(self._show_props)
        toolbar.addWidget(props_button)

        spacer = QWidget()
        spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        toolbar.addWidget(spacer)

        self.search_combo = ComboBox()
        self.search_combo.setEditable(True)
        self.search_combo.setInsertPolicy(QComboBox.InsertPolicy.NoInsert)
        search_box = self.search_combo.lineEdit()
        # search_box.setAlignment(Qt.AlignmentFlag.AlignRight)
        search_box.setPlaceholderText("Name search")
        search_box.returnPressed.connect(self.search)
        self.search_combo.currentIndexChanged.connect(lambda i: weakself.search())
        self.search_combo.setMinimumWidth(search_box.sizeHint().width())
        search_completer = self.search_combo.completer()
        search_completer.setCompletionMode(QCompleter.CompletionMode.PopupCompletion)
        search_completer.setFilterMode(Qt.MatchFlag.MatchContains)
        toolbar.addWidget(self.search_combo)
        find_action = QAction(QIcon.fromTheme('edit-find'), '&Find', toolbar)
        find_action.triggered.connect(self.search)
        toolbar.addAction(find_action)

        toolbar.addSeparator()
        self.expand_all_action = QAction(QIcon.fromTheme('go-down'), '&Expand Current Geometry', toolbar)
        self.expand_all_action.setStatusTip('Expand current geometry')
        CONFIG.set_shortcut(self.expand_all_action, 'geometry_tree_expand_current')
        self.expand_all_action.triggered.connect(self.expand_current_root)
        toolbar.addAction(self.expand_all_action)
        self.expand_all_action = QAction(QIcon.fromTheme('expand-all'), 'E&xpand All', toolbar)
        self.expand_all_action.setStatusTip('Expand whole geometry tree')
        CONFIG.set_shortcut(self.expand_all_action, 'geometry_tree_expand_all')
        self.expand_all_action.triggered.connect(self.tree.expandAll)
        toolbar.addAction(self.expand_all_action)
        self.collapse_all_action = QAction(QIcon.fromTheme('collapse-all'), 'Co&llapse All', toolbar)
        self.collapse_all_action.setStatusTip('Collapse geometry tree')
        CONFIG.set_shortcut(self.collapse_all_action, 'geometry_tree_collapse_all')
        self.collapse_all_action.triggered.connect(self.tree.collapseAll)
        toolbar.addAction(self.collapse_all_action)

        self.model.dataChanged.connect(lambda i,j: weakself._fill_search_combo())

        self.plot_auto_refresh = True
        return toolbar

    def _show_props(self, checked):
        self.model.show_props = checked
        index0 = self.model.index(0, 1)
        index1 = self.model.index(self.model.rowCount()-1, 1)
        self.model.dataChanged.emit(index0, index1)
        self.tree.header().headerDataChanged(Qt.Orientation.Horizontal, 1, 1)

    def _construct_tree(self, model):
        self.tree = GeometryTreeView()
        self.tree.setModel(model)
        self.properties_delegate = HTMLDelegate(self.tree, compact=True)
        self.tree.setItemDelegateForColumn(1, self.properties_delegate)
        self.tree.setColumnWidth(0, 200)

        self.tree.setAutoScroll(True)

        self.tree.dragEnabled()
        self.tree.acceptDrops()
        self.tree.showDropIndicator()
        self.tree.setDragDropMode(QAbstractItemView.DragDropMode.DragDrop)
        self.tree.setDefaultDropAction(Qt.DropAction.MoveAction)

        self.tree.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.tree.customContextMenuRequested.connect(self.on_tree_context_menu)

        self.tree.expanded.connect(self._resize_first_column)

        return self.tree

    def _resize_first_column(self, index=None):
        self.tree.resizeColumnToContents(0)

    def _fill_search_combo(self):
        self.search_combo.clear()
        self.search_combo.addItems([''] + list(self.model.get_names()))

    def show_selection(self):
        if self._current_index is None: return
        node = self._current_index.internalPointer()
        if isinstance(node, GNGeometryBase):
            self.geometry_view.clean_selectors()  # TODO: show geometry edges
            self.geometry_view.canvas.draw()
        else:
            self.geometry_view.clean_selectors()
            selected = self.model.fake_root.get_corresponding_objects(node, self.manager)
            if selected is not None:
                for sel in selected:
                    self.geometry_view.show_selection(self.plotted_object, sel)
            self.geometry_view.canvas.draw()

    def set_current_index(self, new_index):
        """
            Try to change current object.
            :param QModelIndex new_index: index of new current object
            :return: False only when object should restore old selection
        """
        if self._current_index == new_index: return True
        if self._current_controller is not None:
            if not self._current_controller.on_edit_exit():
                return False
        self._current_index = new_index
        if self._current_index is None or not self._current_index.isValid():
            self._current_controller = None
            self.parent_for_editor_widget.setWidget(QWidget())
            self.vertical_splitter.moveSplitter(1, 0)
            current_element = None
            self._current_root = None
        else:
            current_element = self._current_index.internalPointer()
            self._current_controller = current_element.get_controller(self.document, self.model)
            widget = self._current_controller.get_widget()
            self.parent_for_editor_widget.setWidget(widget)
            widget.update()
            str(self.vertical_splitter)  # mysteriously without this the splitter is moved to zero position on reload
            split = max(self.vertical_splitter.height() - widget.height() - 12, 256)
            self.parent_for_editor_widget.resize(QSize(0, 0))  # make sure resizeEvent will be triggered
            self.vertical_splitter.moveSplitter(split, 1)  # it will resize parent_for_editor_widget back
            self._current_controller.on_edit_enter()
            self._current_root = current_element.root
            self.tree.expand_children(self._current_index)
            if self.plotted_tree_element is None or current_element not in self.plotted_tree_element:
                self.plot(self._current_root)
            if self.plotted_object is not None:
                self.show_selection()
        self.update_actions()
        return True

    def expand_current_root(self):
        self.tree.expand_children(self.model.index_for_node(self._current_root))

    def search(self):
        text = self.search_combo.currentText()
        if not text:
            return
        found = self.model.index_for_node(self.model.find_by_name(text))
        if found and found.isValid():
            self.tree.setCurrentIndex(found)
            self.search_combo.setEditText("")
            self.tree.setFocus()
        else:
            red = QPalette()
            red.setColor(QPalette.ColorRole.Text, QColor("#a00"))
            pal = self.search_combo.palette()
            self.search_combo.setPalette(red)
            weakself = weakref.proxy(self)
            QTimer.singleShot(500, lambda: weakself.search_combo.setPalette(pal))

    def current_root(self):
        try:
            current_root = self._current_index.internalPointer().root
        except AttributeError:
            return None
        else:
            return current_root

    def zoom_to_current(self):
        if self.plotted_object is not None:
            objs = self.model.fake_root.get_corresponding_objects(self._current_index.internalPointer(), self.manager)
            bboxes = sum((list(self.plotted_object.get_object_bboxes(obj)) for obj in objs), start=[])
            if not bboxes: return
            self.geometry_view.zoom_bbox(sum(bboxes[1:], start=bboxes[0]))

    def zoom_to_root(self):
        if self.plotted_object is not None:
            box = self.plotted_object.bbox
            self.geometry_view.zoom_bbox(box)

    def object_selected(self, new_selection, old_selection):
        if new_selection.indexes() == old_selection.indexes(): return
        indexes = new_selection.indexes()
        if not self.set_current_index(new_index=(indexes[0] if indexes else None)):
            self.tree.selectionModel().select(old_selection, QItemSelectionModel.SelectionFlag.ClearAndSelect)

    def on_edit_enter(self):
        self.tree.selectionModel().clear()   # model could have been completely changed
        if self.model.dirty:
            self._last_index = None
            self.model.dirty = False
        try:
            if not self._last_index:
                raise IndexError(self._last_index)
            new_index = self._last_index
            self.tree.selectionModel().select(new_index,
                                              QItemSelectionModel.SelectionFlag.Clear | QItemSelectionModel.SelectionFlag.Select |
                                              QItemSelectionModel.SelectionFlag.Rows)
            self.tree.setCurrentIndex(new_index)
            self.plot(self.plotted_tree_element)
            if self._lims is not None:
                self.geometry_view.axes.set_xlim(self._lims[0])
                self.geometry_view.axes.set_ylim(self._lims[1])
            self.show_selection()
        except IndexError:
            new_index = self.model.index(0, 0)
            self.tree.selectionModel().select(new_index,
                                              QItemSelectionModel.SelectionFlag.Clear | QItemSelectionModel.SelectionFlag.Select |
                                              QItemSelectionModel.SelectionFlag.Rows)
            self.tree.setCurrentIndex(new_index)
            self.plot()
        self._fill_search_combo()
        self.update_actions()
        self._resize_first_column()
        self.tree.setFocus()

    def on_edit_exit(self):
        self.manager = None
        if self._current_controller is not None:
            self._last_index = self._current_index
            self.tree.selectionModel().clear()
            self._lims = self.geometry_view.axes.get_xlim(), self.geometry_view.axes.get_ylim()
        return True

    def get_widget(self):
        return self.main_splitter

    def select_info(self, info):
        if hasattr(info, 'action'):
            if isinstance(action, str):
                getattr(self, action)()
            else:
                action()

        if hasattr(info, 'line'):
            self.document.window.goto_line(info.line)

        new_nodes = info.nodes if hasattr(info, 'nodes') else None
        if not new_nodes:
            return # empty??

        if len(new_nodes) == 1:
            self.tree.setCurrentIndex(self.model.index_for_node(new_nodes[0]))
        else:
            try:
                current_node = self.model.node_for_index(self.tree.currentIndex())
            except:
                self.tree.setCurrentIndex(self.model.index_for_node(new_nodes[0]))
            else:
                after = False
                found = False
                for n in new_nodes:
                    if after:
                        self.tree.setCurrentIndex(self.model.index_for_node(n))
                        found = True
                        break
                    if n == current_node: after = True
                if not found:
                    self.tree.setCurrentIndex(self.model.index_for_node(new_nodes[0]))
        try:    # try to set focus on proper widget
            self._current_controller.select_info(info)
        except AttributeError:
            pass
