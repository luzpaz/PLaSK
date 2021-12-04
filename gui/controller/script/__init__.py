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
import sys
import weakref
from numpy import inf

from ...qt import QT_API
from ...qt.QtCore import *
from ...qt.QtWidgets import *
from ...qt.QtGui import *
from ...utils.qsignals import BlockQtSignals
from ...utils.qthread import BackgroundTask
from ...utils.help import open_help
from .completer import CompletionsController
from ...model.script.completer import get_docstring, get_definitions
from ..source import SourceEditController
from ...model.script import ScriptModel
from ...utils.config import CONFIG, set_font
from ...utils.widgets import EDITOR_FONT, ComboBox
from ...utils.texteditor import TextEditor, EditorWidget
from ...utils.texteditor.python import PythonTextEditor, PYTHON_SCHEME
from ...utils.texteditor import EditorWidget


LOG_LEVELS = ['Error', 'Warning', 'Important', 'Info', 'Result', 'Data', 'Detail', 'Debug']


class ScriptEditor(PythonTextEditor):
    """Editor with some even more eatures usefult for script editing"""

    def __init__(self, parent=None, controller=None):
        self.controller = weakref.proxy(controller)
        super().__init__(parent)

        self.complete_action = QAction('Show Completer', self)
        CONFIG.set_shortcut(self.complete_action, 'python_completion')
        self.complete_action.triggered.connect(self.show_completer)
        self.addAction(self.complete_action)

        self.completer = CompletionsController(self)

        self._pointer_blocked = False
        self._pointer_definition = None, None

        self.setMouseTracking(True)

    def keyPressEvent(self, event):
        key = event.key()
        modifiers = event.modifiers()

        if self.completer.popup().isVisible():
            if event.key() in (Qt.Key.Key_Enter, Qt.Key.Key_Return, Qt.Key.Key_Escape, Qt.Key.Key_Tab, Qt.Key.Key_Backtab):
                event.ignore()
                return  # let the completer do default behaviors
            elif event.key() == Qt.Key.Key_Backspace:
                self.completer.setCompletionPrefix(self.completer.completionPrefix()[:-1])
            elif event.text():
                last = event.text()[-1]
                if modifiers & ~(Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.ShiftModifier) or \
                        not (last.isalpha() or last.isdigit() or last == '_'):
                    self.completer.popup().hide()
                else:
                    self.completer.setCompletionPrefix(self.completer.completionPrefix() + event.text())
            elif key not in (Qt.Key.Key_Shift, Qt.Key.Key_Control, Qt.Key.Key_Alt, Qt.Key.Key_AltGr,
                             Qt.Key.Key_Meta, Qt.Key.Key_Super_L, Qt.Key.Key_Super_R):
                self.completer.popup().hide()

        super().keyPressEvent(event)

        if key == Qt.Key.Key_Period and not (CONFIG['workarounds/no_jedi'] or CONFIG['workarounds/jedi_no_dot'] or
                                           self.completer.popup().isVisible()):
            self.completer.start_completion()

    def show_completer(self):
        if not (CONFIG['workarounds/no_jedi'] or self.completer.popup().isVisible()):
            self.completer.start_completion()

    def link_definition(self, row, col):
        self._pointer_blocked = False
        self._pointer_definition = row, col
        cursor = QApplication.overrideCursor()
        if not cursor and row is not None:
            QApplication.setOverrideCursor(Qt.CursorShape.PointingHandCursor)
        elif cursor and cursor.shape() == Qt.CursorShape.PointingHandCursor and row is None:
            QApplication.restoreOverrideCursor()

    def _get_mouse_definitions(self, event):
        if event.modifiers() == Qt.KeyboardModifier.ControlModifier:
            if self._pointer_blocked: return
            self._pointer_blocked = True
            cursor = self.cursorForPosition(event.pos())
            row = cursor.blockNumber()
            col = cursor.positionInBlock()
            # task = BackgroundTask(lambda: get_definitions(self.controller.document, self.toPlainText(), row, col),
            #                       self.link_definition)
            # task.start()
            if not CONFIG['workarounds/no_jedi']:
                self.link_definition(*get_definitions(self.controller.document, self.toPlainText(), row, col))
        else:
            cursor = QApplication.overrideCursor()
            if cursor and cursor.shape() == Qt.CursorShape.PointingHandCursor:
                QApplication.restoreOverrideCursor()

    def mouseMoveEvent(self, event):
        super().mouseMoveEvent(event)
        self._get_mouse_definitions(event)

    def mouseReleaseEvent(self, event):
        super().mouseReleaseEvent(event)
        row, col = self._pointer_definition
        if event.modifiers() == Qt.KeyboardModifier.ControlModifier and not self._pointer_blocked and row:
            cursor = QTextCursor(self.document().findBlockByLineNumber(row))
            cursor.movePosition(QTextCursor.MoveOperation.Right, QTextCursor.MoveMode.MoveAnchor, col)
            self.setTextCursor(cursor)


class ScriptController(SourceEditController):

    def __init__(self, document, model=None):
        if model is None: model = ScriptModel()
        SourceEditController.__init__(self, document, model)
        self.document.window.config_changed.connect(self.reconfig)

    def create_source_widget(self, parent):
        window = QMainWindow(parent)
        window.setWindowFlags(Qt.WindowType.Widget)

        source = EditorWidget(parent, ScriptEditor, self)

        source.editor.setReadOnly(self.model.is_read_only())
        window.editor = source.editor

        self.model.editor = source.editor
        source.editor.cursorPositionChanged.connect(self.model.refresh_info)

        source.toolbar.addSeparator()
        unindent_action = QAction(QIcon.fromTheme('format-indent-less'), 'Unin&dent', source)
        unindent_action.triggered.connect(lambda: unindent(source.editor))
        source.toolbar.addAction(unindent_action)
        indent_action = QAction(QIcon.fromTheme('format-indent-more'), '&Indent', source)
        indent_action.triggered.connect(lambda: indent(source.editor))
        source.toolbar.addAction(indent_action)
        button = QToolButton()
        button.setIcon(QIcon.fromTheme('document-properties'))
        menu = QMenu(button)
        menu.addAction(source.editor.comment_action)
        menu.addAction(source.editor.uncomment_action)
        menu.addAction(source.editor.toggle_comment_action)
        button.setMenu(menu)
        button.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        source.toolbar.addWidget(button)
        if self.model.is_read_only():
            unindent_action.setEnabled(False)
            indent_action.setEnabled(False)
            source.editor.comment_action.setEnabled(False)
            source.editor.uncomment_action.setEnabled(False)
            source.editor.toggle_comment_action.setEnabled(False)

        self.help_dock = HelpDock(window)
        parent.config_changed.connect(self.help_dock.reconfig)
        state = CONFIG['session/scriptwindow']
        if state is None or not window.restoreState(state):
            window.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, self.help_dock)
        self.help_dock.hide()

        source.toolbar.addSeparator()

        help_action = QAction(QIcon.fromTheme('help-contents'), 'Open &Help', source)
        CONFIG.set_shortcut(help_action, 'python_help')
        help_action.triggered.connect(self.open_help)
        source.editor.addAction(help_action)
        source.toolbar.addAction(help_action)

        doc_action = QAction(QIcon.fromTheme('help-contextual'), 'Show &Docstring', source)
        CONFIG.set_shortcut(doc_action, 'python_docstring')
        doc_action.triggered.connect(self.show_docstring)
        source.editor.addAction(doc_action)
        hide_doc_action = QAction('Hide Docstring', source)
        CONFIG.set_shortcut(hide_doc_action, 'python_hide_docstring')
        hide_doc_action.triggered.connect(self.help_dock.hide)
        source.editor.addAction(hide_doc_action)
        source.toolbar.addAction(doc_action)

        self.document.window.closed.connect(self.save_state)

        try:
            loglevel = self.document.loglevel
        except AttributeError:
            pass
        else:
            spacer = QWidget()
            spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
            source.toolbar.addWidget(spacer)
            source.toolbar.addWidget(QLabel("Log Level: "))
            self.loglevel = ComboBox()
            self.loglevel.addItems(LOG_LEVELS)
            try:
                self.loglevel.setCurrentIndex(LOG_LEVELS.index(loglevel.title()))
            except ValueError:
                self.loglevel.setCurrentIndex(6)
            self.loglevel.currentIndexChanged.connect(self.document.set_loglevel)
            source.toolbar.addWidget(self.loglevel)

        window.setCentralWidget(source)
        return window

    def _modification_changed(self, changed):
        self.model.undo_stack.cleanChanged.emit(changed)

    def save_state(self):
        try:
            CONFIG['session/scriptwindow'] = self.source_widget.saveState()
        except AttributeError:
            pass
        else:
            CONFIG.sync()

    def on_edit_enter(self):
        super().on_edit_enter()
        self.source_widget.editor.rehighlight(self.document.defines, self.document.solvers)

    def reconfig(self):
        self.source_widget.editor.reconfig()

    def on_edit_exit(self):
        return super().on_edit_exit()

    def save_data_in_model(self):
        self.before_save()
        self.before_save()
        super().save_data_in_model()

    def before_save(self):
        if CONFIG['editor/remove_trailing_spaces']:
            self.load_data_from_model()
            editor = self.get_source_widget().editor
            document = editor.document()
            cursor = editor.textCursor()
            cursor.beginEditBlock()
            regex = QRegularExpression(r'\s+$')
            found = document.find(regex)
            while found and not found.isNull():
                found.removeSelectedText()
                found = document.find(regex, found.position())
            cursor.endEditBlock()
            self.model._code = document.toPlainText()

    def open_help(self):
        open_help('api', self.document.window)

    def show_docstring(self):
        if CONFIG['workarounds/no_jedi']: return
        cursor = self.source_widget.editor.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.EndOfWord)
        row = cursor.blockNumber()
        col = cursor.positionInBlock()
        # QApplication.setOverrideCursor(Qt.CursorShape.BusyCursor)
        if CONFIG['workarounds/blocking_jedi']:
            result = get_docstring(self.document, self.source_widget.editor.toPlainText(), row, col)
            if type(result) is tuple:
                self.help_dock.show_help(*result)
            else:
                self.help_dock.show_help(result)
        else:
            task = BackgroundTask(lambda: get_docstring(self.document, self.source_widget.editor.toPlainText(),
                                                        row, col),
                                  self.help_dock.show_help)
            task.start()


class HelpDock(QDockWidget):

    def __init__(self, parent):
        super().__init__(parent)
        self.textarea = QTextEdit()
        self.textarea.setReadOnly(True)
        help_font = QFont(EDITOR_FONT)
        set_font(help_font, 'editor/help_font')
        pal = self.textarea.palette()
        pal.setColor(QPalette.ColorRole.Base, QColor(CONFIG['editor/help_background_color']))
        pal.setColor(QPalette.ColorRole.Text, QColor(CONFIG['editor/help_foreground_color']))
        self.textarea.setPalette(pal)
        self.textarea.setFont(help_font)
        font_metrics = self.textarea.fontMetrics()
        self.textarea.setMinimumWidth(86 * font_metrics.horizontalAdvance('a'))
        self.textarea.setMinimumHeight(8 * font_metrics.height())
        self.setWidget(self.textarea)
        self.setObjectName('help')

    def show_help(self, name=None, docstring=None):
        if docstring is not None:
            self.setWindowTitle("Help: " + name)
            self.textarea.setText(docstring)
            self.show()
        # QApplication.restoreOverrideCursor()

    def reconfig(self):
        help_font = self.textarea.font()
        set_font(help_font, 'editor/help_font')
        self.textarea.setFont(help_font)
        font_metrics = self.textarea.fontMetrics()
        self.textarea.setMinimumWidth(86 * font_metrics.horizontalAdvance('a'))
        self.textarea.setMinimumHeight(8 * font_metrics.height())
        self.textarea.setStyleSheet("QTextEdit {{ color: {fg}; background-color: {bg} }}".format(
            fg=(CONFIG['editor/help_foreground_color']),
            bg=(CONFIG['editor/help_background_color'])
        ))
