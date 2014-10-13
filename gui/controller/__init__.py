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

from ..qt import QtGui, QtCore


class Controller(object):
    """
        Base class for controllers.
        Controllers create editor for the fragment of the XPL document (a section or smaller fragment) and make it
        available by get_widget method (subclasses must implement this method).
        They also transfer data from model to editor (typically in on_edit_enter) and in opposite direction
        (typically in save_data_in_model).
    """
    
    def __init__(self, document=None, model=None):
        """Optionally set document and/or model."""
        super(Controller, self).__init__()
        if document is not None:
            self.document = document
        if model is not None:
            self.model = model
        self.notify_changes = False

    def save_data_in_model(self):
        """Called to force save data from editor in model (typically by on_edit_exit or when model is needed while
         editor still is active - for instance when user saves edited document to file)."""
        pass  
        
    def on_edit_enter(self):
        """Called when editor is entered and will be visible."""
        pass

    def on_edit_exit(self):
        """Called when editor is left and will be not visible. Typically and by default it calls save_data_in_model."""
        return self.try_save_data_in_model()

    def update_line_numbers(self, current_line_in_file):
        """If the script has a source editor, update its line numbers offset"""
        self.model.line_in_file = current_line_in_file
        try:
            self.source_widget.editor.line_numbers.offset = current_line_in_file
        except AttributeError:
            pass
        else:
            self.source_widget.editor.repaint()

    def try_save_data_in_model(self, cat=None):
        try:
            self.save_data_in_model()
        except Exception as exc:
            from .. import _DEBUG
            if _DEBUG:
                import traceback as tb
                tb.print_stack()
                tb.print_exc()
                # sys.stderr.write('Traceback (most recent call last):\n')
                # sys.stderr.write(''.join(tb.format_list(tb.extract_stack()[:-1])))
                # sys.stderr.write('{}: {}\n'.format(exc.__class__.__name__, exc))
            return QtGui.QMessageBox().critical(
                None, "Error saving data from editor",
                "An error occured while trying to save data from editor:\n"
                "(caused either by wrong values entered or a by a program error)\n\n"
                "{}: {}\n\n"
                "Do you want to discard your changes in this {}editor and move on?"
                .format(exc.__class__.__name__, str(exc), '' if cat is None else (cat+' ')),
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No) == QtGui.QMessageBox.Yes
        else:
            return True

    def get_widget(self):
        raise NotImplementedError("Method 'get_widget' must be overriden in a subclass!")

    def fire_changed(self, *args, **kwargs):
        if self.notify_changes:
            self.model.fire_changed()


class NoConfController(Controller):
    """Controller for all models that does not require any configuration."""
    def __init__(self, text = 'Configuration is neither required nor available.'):
        super(NoConfController, self).__init__()
        self.label = QtGui.QLabel(text)
        self.label.setAlignment(QtCore.Qt.AlignCenter)

    def get_widget(self):
        return self.label
