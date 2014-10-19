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

from ...qt import QtGui

from .editor import ScriptEditor

from ..source import SourceEditController, SourceWidget
from ...model.script import ScriptModel
from ...utils.config import CONFIG, parse_highlight
from ...utils.widgets import DEFAULT_FONT

from ...external.highlighter import SyntaxHighlighter, load_syntax
if sys.version_info >= (3, 0, 0):
    from ...external.highlighter.python32 import syntax
else:
    from ...external.highlighter.python27 import syntax
from ...external.highlighter.plask import syntax as plask_syntax

syntax['formats'].update(plask_syntax['formats'])
syntax['scanner'][None] = syntax['scanner'][None][:-1] + plask_syntax['scanner'] + [syntax['scanner'][None][-1]]

scheme = {
    'syntax_comment': parse_highlight(CONFIG('syntax/python_comment', 'color=green, italic=true')),
    'syntax_string': parse_highlight(CONFIG('syntax/python_string', 'color=blue')),
    'syntax_builtin': parse_highlight(CONFIG('syntax/python_builtin', 'color=maroon')),
    'syntax_keyword': parse_highlight(CONFIG('syntax/python_keyword', 'color=black, bold=true')),
    'syntax_number': parse_highlight(CONFIG('syntax/python_number', 'color=darkblue')),
    'syntax_member': parse_highlight(CONFIG('syntax/python_member', 'color=#440044')),
    'syntax_plask': parse_highlight(CONFIG('syntax/python_plask', 'color=#0088ff')),
    'syntax_provider': parse_highlight(CONFIG('syntax/python_provider', 'color=#888800')),
    'syntax_receiver': parse_highlight(CONFIG('syntax/python_receiver', 'color=#888800')),
    'syntax_log': parse_highlight(CONFIG('syntax/python_log', 'color=blue')),
    'syntax_solver': parse_highlight(CONFIG('syntax/python_solver', 'color=red')),
    'syntax_loaded': parse_highlight(CONFIG('syntax/python_loaded', 'color=#ff8800')),
    'syntax_pylab': parse_highlight(CONFIG('syntax/python_pylab', 'color=#880044')),
}


class ScriptController(SourceEditController):

    def __init__(self, document, model=None):
        if model is None: model = ScriptModel()
        SourceEditController.__init__(self, document, model)

    def create_source_widget(self, parent):
        source = SourceWidget(parent, ScriptEditor, self)
        source.editor.setReadOnly(self.model.is_read_only())

        source.toolbar.addSeparator()
        menu = QtGui.QMenu()
        menu.addAction(source.editor.comment_action)
        menu.addAction(source.editor.uncomment_action)
        button = QtGui.QToolButton()
        button.setIcon(QtGui.QIcon.fromTheme('code-block', QtGui.QIcon(':/code-block')))
        button.setMenu(menu)
        button.setPopupMode(QtGui.QToolButton.InstantPopup)
        source.toolbar.addWidget(button)
        if self.model.is_read_only():
            source.editor.comment_action.setEnabled(False)
            source.editor.uncomment_action.setEnabled(False)

        self.highlighter = SyntaxHighlighter(source.editor.document(),
                                             *load_syntax(syntax, scheme),
                                             default_font=DEFAULT_FONT)

        return source

    def on_edit_enter(self):
        super(ScriptController, self).on_edit_enter()

    def on_edit_exit(self):
        return super(ScriptController, self).on_edit_exit()
