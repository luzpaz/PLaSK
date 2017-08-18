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

# coding utf:8
from __future__ import print_function

import sys
import os
import re
from stat import S_ISDIR

from gui.qt.QtCore import Qt, QThread, QMutex
from gui.qt.QtGui import *
from gui.qt.QtWidgets import *
from gui.launch import LAUNCHERS
from gui.launch.dock import OutputWindow

try:
    import cPickle as pickle
except ImportError:
    import pickle

try:
    import paramiko

except ImportError:
    import webbrowser
    import subprocess
    import platform

    class Launcher(object):
        name = "Remote Process"

        def widget(self, main_window):
            message = QTextBrowser()
            message.setText("Remote launcher cannot be used because Python module "
                            "Paramiko is missing. Either install it manually from its "
                            "<a href=\"http://www.paramiko.org/\">webpage</a> or press "
                            "Ok to try to launch the installation for you.")
            message.setReadOnly(True)
            message.anchorClicked.connect(self._open_link)
            message.setOpenLinks(False)
            pal = message.palette()
            pal.setColor(QPalette.Base, pal.color(QPalette.Window))
            message.setPalette(pal)
            return message

        def _open_link(self, url):
            webbrowser.open(url.toString())

        def launch(self, main_window, args, defs):
            if os.name == 'nt' and ('conda' in sys.version or 'Continuum' in sys.version):
                subprocess.Popen(['conda', 'install', 'paramiko'])
            else:
                dist = platform.dist()[0].lower()
                if dist in ('ubuntu', 'debian', 'mint'):
                    term = 'xterm'
                    cmd = 'apt-get'
                    pkg = 'python3-paramiko' if sys.version_info.major == 3 else 'python-paramiko'
                elif dist in ('redhat', 'centos'):
                    term = 'gnome-terminal'
                    cmd = 'yum'
                    pkg = 'python3{}-paramiko'.format(sys.version_info.minor) if sys.version_info.major == 3 else \
                          'python-paramiko'
                else:
                    return
                subprocess.Popen([term, '-e', 'sudo {} install {}'.format(cmd, pkg)])
                QMessageBox.information(None, "Remote Batch Job Launcher",
                                              "Once you have successfully installed Paramiko, please restart PLaSK "
                                              "to use the remote batch launcher.")


else:
    import logging
    logging.raiseExceptions = False

    import paramiko.hostkeys

    import os.path
    from collections import OrderedDict

    try:
        from shlex import quote
    except ImportError:
        from pipes import quote

    from gui.xpldocument import XPLDocument
    from gui.utils.config import CONFIG

    from socket import timeout as TimeoutException

    from gzip import GzipFile
    try:
        from base64 import encodebytes as base64
    except ImportError:
        from base64 import encodestring as base64
    try:
        from StringIO import StringIO as BytesIO
    except ImportError:
        from io import BytesIO

    def hexlify(data):
        if isinstance(data, str):
            return ':'.join('{:02x}'.format(ord(d)) for d in data)
        else:
            return ':'.join('{:02x}'.format(d) for d in data)

    def _parse_bool(value, default):
        try:
            return bool(eval(value))
        except (SyntaxError, ValueError, TypeError):
            return default

    def _parse_int(value, default):
        try:
            return int(value)
        except ValueError:
            return default


    class Account(object):
        """
        Base class for account data.
        """

        def __init__(self, name, userhost=None, port=22, program='', color=None, compress=None, directories=None):
            self.name = name
            self.userhost = userhost
            self.port = _parse_int(port, 22)
            self.program = program
            self.compress = _parse_bool(compress, True)
            self._widget = None
            self.directories = {} if directories is None else directories

        def update(self, source):
            self.userhost = source.userhost
            self.port = source.port
            self.program = source.program
            self.compress = source.compress

        @classmethod
        def load(cls, name, config):
            kwargs = dict(config)
            if 'directories' in kwargs:
                try:
                    kwargs['directories'] = pickle.loads(CONFIG.get('directories', b'N.').encode())
                except (pickle.PickleError, EOFError):
                    del kwargs['directories']
            return cls(name, **kwargs)

        def save(self):
            return dict(userhost=self.userhost, port=self.port, program=self.program, compress=int(self.compress))

        def save_params(self):
            key = 'launcher_remote/accounts/{}/directories'.format(self.name)
            CONFIG[key] = pickle.dumps(self.directories, 0).decode()
            CONFIG.sync()

        class EditDialog(QDialog):
            def __init__(self, account=None, name=None, parent=None):
                super(Account.EditDialog, self).__init__(parent)

                if account is not None and account.userhost:
                    user, host = account.userhost.split('@')
                else:
                    user = host = None

                self.setWindowTitle("Add" if name is None else "Edit" + " Remote Server")

                layout = QFormLayout()
                self.setLayout(layout)

                self.name_edit = QLineEdit()
                self.name_edit.setToolTip("Friendly name of the account.")
                if name is not None:
                    self.name_edit.setText(name)
                    self.autoname = False
                else:
                    self.autoname = True
                self.name_edit.textEdited.connect(self.name_edited)
                layout.addRow("&Name:", self.name_edit)

                self.host_edit = QLineEdit()
                self.host_edit.setToolTip("Hostname to execute the batch job at.")
                if host is not None:
                    self.host_edit.setText(host)
                self.host_edit.textEdited.connect(self.userhost_edited)
                layout.addRow("&Host:", self.host_edit)

                self.port_input = QSpinBox()
                self.port_input.setMinimum(1)
                self.port_input.setMaximum(65536)
                self.port_input.setToolTip("TCP port to connect to (usually 22).")
                if account is not None:
                    self.port_input.setValue(account.port)
                else:
                    self.port_input.setValue(22)
                layout.addRow("&Port:", self.port_input)

                self.user_edit = QLineEdit()
                self.user_edit.setToolTip("Username at the execution host.")
                if user is not None:
                    self.user_edit.setText(user)
                self.user_edit.textEdited.connect(self.userhost_edited)
                layout.addRow("&User:", self.user_edit)

                self._advanced_widgets = []

                self.program_edit = QLineEdit()
                self.program_edit.setToolTip("Path to PLaSK executable. If left blank 'plask' will be used.")
                self.program_edit.setPlaceholderText("plask")
                if account is not None and account.program:
                    self.program_edit.setText(account.program)
                layout.addRow("&Command:", self.program_edit)
                self._advanced_widgets.append(self.program_edit)

                self.compress_checkbox = QCheckBox()
                self.compress_checkbox.setToolTip(
                    "Compress script on sending to the batch system. This can make the scripts\n"
                    "stored in batch system queues smaller, however, it may be harder to track\n"
                    "possible errors.")
                if account is not None:
                    self.compress_checkbox.setChecked(account.compress)
                else:
                    self.compress_checkbox.setChecked(True)
                layout.addRow("Compr&ess Script:", self.compress_checkbox)
                self._advanced_widgets.append(self.compress_checkbox)

                self._set_rows_visibility(self._advanced_widgets, False)

                abutton = QPushButton("Ad&vanced...")
                abutton.setCheckable(True)
                abutton.toggled.connect(self.show_advanced)

                buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
                buttons.addButton(abutton, QDialogButtonBox.ActionRole)
                buttons.accepted.connect(self.accept)
                buttons.rejected.connect(self.reject)
                layout.addRow(buttons)

                self.host_edit.setFocus()

            def _set_rows_visibility(self, widgets, state, layout=None):
                if layout is None:
                    layout = self.layout()
                for widget in widgets:
                    widget.setVisible(state)
                    layout.labelForField(widget).setVisible(state)

            def show_advanced(self, show):
                self._set_rows_visibility(self._advanced_widgets, show)
                self.setFixedHeight(self.sizeHint().height())
                self.adjustSize()

            def name_edited(self):
                self.autoname = False

            def userhost_edited(self):
                if self.autoname:
                    if self.user:
                        self.name_edit.setText("{}@{}".format(self.user, self.host))
                    else:
                        self.name_edit.setText(self.host)

            @property
            def name(self):
                return self.name_edit.text()

            @property
            def userhost(self):
                return "{}@{}".format(self.user, self.host)

            @property
            def host(self):
                return self.host_edit.text()

            @property
            def user(self):
                return self.user_edit.text()

            @property
            def port(self):
                return self.port_input.value()

            @property
            def compress(self):
                return self.compress_checkbox.isChecked()

            @property
            def program(self):
                return self.program_edit.text()


    class RemotePlaskThread(QThread):

        def __init__(self, ssh, account, fname, dock, mutex, main_window, args, defs):
            super(RemotePlaskThread, self).__init__()
            self.main_window = main_window
            document = main_window.document

            command = account.program or 'plask'

            command_line = "{unzip}{cmd} -{ft} -ldebug {defs} -:{fname} {args}".format(
                unzip="base64 -d | gunzip | " if account.compress else "",
                cmd=command,
                fname=fname,
                defs=' '.join(quote(d) for d in defs), args=' '.join(quote(a) for a in args),
                ft='x' if isinstance(main_window.document, XPLDocument) else 'p')

            self.ssh = ssh
            channel = ssh.get_transport().open_session()
            channel.set_combine_stderr(True)
            # channel.get_pty()
            channel.exec_command(command_line)
            stdin = channel.makefile('wb')
            self.stdout = channel.makefile('rb')

            if account.compress:
                gzipped = BytesIO()
                with GzipFile(fileobj=gzipped, filename=fname, mode='wb') as gzip:
                    gzip.write(document.get_content().encode('utf8'))
                stdin.write(base64(gzipped.getvalue()).decode('ascii'))
            else:
                stdin.write(document.get_content())
            stdin.flush()
            stdin.channel.shutdown_write()

            fd, fb = (s.replace(' ', '&nbsp;') for s in os.path.split(fname))
            sep = os.path.sep
            if sep == '\\':
                sep = '\\\\'
                fd = fd.replace('\\', '\\\\')
            self.link = re.compile(
                u'((?:{}{})?{}(?:(?:,|:)(?:&nbsp;XML)?&nbsp;line&nbsp;|:))(\\d+)(.*)'.format(fd, sep, fb))

            self.dock = dock
            self.mutex = mutex
            try:
                self.terminated.connect(self.kill_process)
            except AttributeError:
                self.finished.connect(self.kill_process)
            self.main_window.closed.connect(self.kill_process)

        def __del__(self):
            self.main_window.closed.disconnect(self.kill_process)

        def run(self):
            while not self.stdout.channel.exit_status_ready():
                line = self.stdout.readline().rstrip()
                self.dock.parse_line(line, self.link)
            out = self.stdout.read()
            for line in out.splitlines():
                self.dock.parse_line(line, self.link)

        def kill_process(self):
            try:
                self.stdout.channel.close()
            except paramiko.SSHException:
                pass


    class Launcher(object):
        name = "Remote Process"

        _passwd_cache = {}

        def __init__(self):
            self.current_account = None
            self.load_accounts()

        def widget(self, main_window, parent=None):
            widget = QWidget(parent)
            layout = QVBoxLayout()
            layout.setContentsMargins(0, 0, 0, 0)
            widget.setLayout(layout)

            self.filename = main_window.document.filename

            label = QLabel("E&xecution account:")
            layout.addWidget(label)
            accounts_layout = QHBoxLayout()
            accounts_layout.setContentsMargins(0, 0, 0, 0)
            self.accounts_combo = QComboBox()
            self.accounts_combo.addItems([a.name for a in self.accounts])
            if self.current_account is not None:
                self.accounts_combo.setCurrentIndex(self.current_account)
            else:
                self.current_account = self.accounts_combo.currentIndex()
            self.accounts_combo.currentIndexChanged.connect(self.account_changed)
            self.accounts_combo.setToolTip("Select the remote server and user to send the job to.")
            accounts_layout.addWidget(self.accounts_combo)
            account_add = QToolButton()
            account_add.setIcon(QIcon.fromTheme('list-add'))
            account_add.setToolTip("Add new remote server.")
            account_add.pressed.connect(self.account_add)
            accounts_layout.addWidget(account_add)
            self.account_edit_button = QToolButton()
            self.account_edit_button.setIcon(QIcon.fromTheme('document-edit'))
            self.account_edit_button.setToolTip("Edit the current remote server.")
            self.account_edit_button.pressed.connect(self.account_edit)
            self.account_edit_button.setEnabled(bool(self.accounts))
            accounts_layout.addWidget(self.account_edit_button)
            self.account_remove_button = QToolButton()
            self.account_remove_button.setIcon(QIcon.fromTheme('list-remove'))
            self.account_remove_button.setToolTip("Remove the current remote server.")
            self.account_remove_button.pressed.connect(self.account_remove)
            self.account_remove_button.setEnabled(bool(self.accounts))
            accounts_layout.addWidget(self.account_remove_button)
            layout.addLayout(accounts_layout)
            label.setBuddy(self.accounts_combo)

            label = QLabel("&Working directory:")
            layout.addWidget(label)
            self.workdir = QLineEdit()
            self.workdir.setToolTip("Type a directory at the execution server in which the job will run.\n"
                                    "If the directory starts with / it is consider as an absolute path,\n"
                                    "otherwise it is relative to your home directory. If the directory\n"
                                    "does not exists, it is automatically created.")
            label.setBuddy(self.workdir)
            dirbutton = QPushButton()
            dirbutton.setIcon(QIcon.fromTheme('folder-open'))
            dirbutton.pressed.connect(self.select_workdir)
            dirlayout = QHBoxLayout()
            dirlayout.addWidget(self.workdir)
            dirlayout.addWidget(dirbutton)
            layout.addLayout(dirlayout)

            layout.addWidget(QLabel("Visible Log levels:"))
            self.error = QCheckBox("&Error")
            self.error.setChecked(int(CONFIG.get('launcher_local/show_error', 2)) == 2)
            self.error.stateChanged.connect(lambda state: CONFIG.__setitem__('launcher_local/show_error', state))
            layout.addWidget(self.error)
            self.warning = QCheckBox("&Warning")
            self.warning.setChecked(int(CONFIG.get('launcher_local/show_warning', 2)) == 2)
            layout.addWidget(self.warning)
            self.warning.stateChanged.connect(lambda state: CONFIG.__setitem__('launcher_local/show_warning', state))
            self.info = QCheckBox("&Info")
            self.info.setChecked(int(CONFIG.get('launcher_local/show_info', 2)) == 2)
            layout.addWidget(self.info)
            self.info.stateChanged.connect(lambda state: CONFIG.__setitem__('launcher_local/show_info', state))
            self.result = QCheckBox("&Result")
            self.result.setChecked(int(CONFIG.get('launcher_local/show_result', 2)) == 2)
            layout.addWidget(self.result)
            self.result.stateChanged.connect(lambda state: CONFIG.__setitem__('launcher_local/show_result', state))
            self.data = QCheckBox("&Data")
            self.data.setChecked(int(CONFIG.get('launcher_local/show_data', 2)) == 2)
            layout.addWidget(self.data)
            self.data.stateChanged.connect(lambda state: CONFIG.__setitem__('launcher_local/show_data', state))
            self.detail = QCheckBox("De&tail")
            self.detail.setChecked(int(CONFIG.get('launcher_local/show_detail', 2)) == 2)
            layout.addWidget(self.detail)
            self.detail.stateChanged.connect(lambda state: CONFIG.__setitem__('launcher_local/show_detail', state))
            self.debug = QCheckBox("De&bug")
            self.debug.setChecked(int(CONFIG.get('launcher_local/show_debug', 2)) == 2)
            self.debug.stateChanged.connect(lambda state: CONFIG.__setitem__('launcher_local/show_debug', state))
            layout.addWidget(self.debug)

            layout.setContentsMargins(1, 1, 1, 1)
            widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

            if self.accounts:
                try:
                    self.workdir.setText(self.accounts[self.current_account].directories.get(self.filename, ''))
                except:
                    pass

            self._widget = widget
            return widget

        def exit(self, visible):
            if self.accounts and self.current_account is not None:
                self.accounts[self.current_account].directories[self.filename] = self.workdir.text()
            for account in self.accounts:
                account.save_params()

        def load_accounts(self):
            self.accounts = []
            accounts = CONFIG['launcher_remote/accounts']
            with CONFIG.group('launcher_remote/accounts') as config:
                for name, account in config.groups:
                    self.accounts.append(Account.load(name, account))

        def save_accounts(self):
            del CONFIG['launcher_remote/accounts']
            with CONFIG.group('launcher_remote/accounts') as config:
                for account in self.accounts:
                    with config.group(account.name) as group:
                        for k, v in account.save().items():
                            group[k] = v
            CONFIG.sync()

        def account_add(self):
            dialog = Account.EditDialog()
            if dialog.exec_() == QDialog.Accepted:
                name = dialog.name
                if name not in self.accounts:
                    account = Account(name)
                    account.update(dialog)
                    self.accounts.append(account)
                    self.accounts_combo.addItem(name)
                    index = self.accounts_combo.count() - 1
                    self.accounts_combo.setCurrentIndex(index)
                    self.account_edit_button.setEnabled(True)
                    self.account_remove_button.setEnabled(True)
                    self.account_changed(index)
                else:
                    QMessageBox.critical(None, "Add Error",
                                               "Execution account '{}' already in the list.".format(name))
                self.save_accounts()

        def account_edit(self):
            old = self.accounts_combo.currentText()
            idx = self.accounts_combo.currentIndex()
            dialog = Account.EditDialog(self.accounts[idx], old)
            if dialog.exec_() == QDialog.Accepted:
                new = dialog.name
                if new != old and new in (a.name for a in self.accounts):
                    QMessageBox.critical(None, "Edit Error",
                                               "Execution account '{}' already in the list.".format(new))
                else:
                    if new != old:
                        self.accounts[idx].name = new
                    self.accounts[idx].update(dialog)
                    self.accounts_combo.setItemText(idx, new)
                    self.account_changed(new)
                    self.save_accounts()

        def account_remove(self):
            confirm = QMessageBox.warning(None, "Remove Account?",
                                          "Do you really want to remove the account '{}'?"
                                          .format(self.accounts[self.current_account].name),
                                          QMessageBox.Yes | QMessageBox.No)
            if confirm == QMessageBox.Yes:
                del self.accounts[self.current_account]
                idx = self.current_account
                self.current_account = None
                self.accounts_combo.removeItem(idx)
                self.account_changed(self.accounts_combo.currentIndex())
                self.save_accounts()
                if not self.accounts:
                    self.account_edit_button.setEnabled(False)
                    self.account_remove_button.setEnabled(False)

        def account_changed(self, index):
            if self.accounts and self.current_account is not None:
                self.accounts[self.current_account].directories[self.filename] = self.workdir.text()
            if isinstance(index, int):
                self.current_account = index
            else:
                self.current_account = self.accounts_combo.currentIndex()
            self.workdir.setText(self.accounts[self.current_account].directories.get(self.filename, ''))

        def show_optional(self, field, visible, button=None):
            field.setVisible(visible)
            if button is not None:
                button.setChecked(visible)

        class AbortException(Exception):
            pass

        @staticmethod
        def _save_host_keys(host_keys):
            keylist = []
            for host, keys in host_keys.items():
                for keytype, key in keys.items():
                    keylist.append('{} {} {}'.format(host, keytype, key.get_base64()))
            CONFIG['launcher_remote/ssh_host_keys'] = '\n'.join(keylist)
            CONFIG.sync()

        class AskAddPolicy(paramiko.MissingHostKeyPolicy):
            def missing_host_key(self, client, hostname, key):
                add = QMessageBox.warning(None, "Unknown Host Key",
                                                "The host key for {} is not cached "
                                                "in the registry. You have no guarantee that the "
                                                "server is the computer you think it is.\n\n"
                                                "The server's {} key fingerprint is:\n"
                                                "{}\n\n"
                                                "If you trust this host, hit Yes to add the key "
                                                "to the cache and carry on connecting.\n\n"
                                                "If you want to carry on connecting just once, "
                                                "without adding the key to the cache, hit No.\n\n"
                                                "If you do not trust this host, hit Cancel to "
                                                "abandon the connection."
                                                .format(hostname, key.get_name()[4:], str(hexlify(key.get_fingerprint()))),
                                                 QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel)
                if add == QMessageBox.Cancel:
                    raise Launcher.AbortException(u'Server {} not found in known_hosts'.format(hostname))
                client.get_host_keys().add(hostname, key.get_name(), key)
                if add == QMessageBox.Yes:
                    Launcher._save_host_keys(client.get_host_keys())

        @classmethod
        def connect(cls, host, user, port):
            ssh = paramiko.SSHClient()
            ssh.load_system_host_keys()
            host_keys = ssh.get_host_keys()
            saved_keys = CONFIG['launcher_remote/ssh_host_keys']
            if saved_keys is not None:
                saved_keys = saved_keys.split('\n')
                for key in saved_keys:
                    try:
                        e = paramiko.hostkeys.HostKeyEntry.from_line(key)
                    except paramiko.SSHException:
                        continue
                    if e is not None:
                        for h in e.hostnames:
                            host_keys.add(h, e.key.get_name(), e.key)
            ssh.set_missing_host_key_policy(cls.AskAddPolicy())

            passwd = cls._passwd_cache.get((host, user), '')

            while True:
                try:
                    ssh.connect(host, username=user, password=passwd, port=port, compress=True, timeout=15)
                except Launcher.AbortException:
                    return
                except paramiko.BadHostKeyException as err:
                    add = QMessageBox.warning(None, "Bad Host Key",
                                                    "WARNING - POTENTIAL SECURITY BREACH!\n\n"
                                                    "The host key for {} does not "
                                                    "match the one cached in the registry. This means "
                                                    "that either the server administrator has changed "
                                                    "the host key, or you have actually connected to "
                                                    "another computer pretending to be the server.\n\n"
                                                    "The new server's {} key fingerprint is:\n"
                                                    "{}\n\n"
                                                    "If you trust this host, hit Yes to add the key to "
                                                    "the cache and carry on connecting.\n\n"
                                                    "If you want to carry on connecting just once, "
                                                    "without adding the key to the cache, hit No.\n\n"
                                                    "If you do not trust this host, hit Cancel to "
                                                    "abandon the connection."
                                                    .format(err.hostname, err.key.get_name()[4:],
                                                            str(hexlify(err.key.get_fingerprint()))),
                                                    QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel)
                    if add == QMessageBox.Cancel:
                        return
                    ssh.get_host_keys().add(err.hostname, err.key.get_name(), err.key)
                    if add == QMessageBox.Yes:
                        cls._save_host_keys(ssh.get_host_keys())
                except paramiko.AuthenticationException:
                    dialog = QInputDialog()
                    dialog.setLabelText("Password required for {}@{}. Please enter valid password:"
                                        .format(user, host))
                    dialog.setTextEchoMode(QLineEdit.Password)
                    if dialog.exec_() == QDialog.Accepted:
                        passwd = cls._passwd_cache[host, user] = dialog.textValue()
                    else:
                        return
                except Exception as err:
                    try:
                        msg = err.message
                    except AttributeError:
                        msg = str(err)
                    answer = QMessageBox.critical(None, "Connection Error",
                                                        "Could not connect to {}.\n\n{}\n\nTry again?"
                                                        .format(host, msg),
                                                        QMessageBox.Yes|QMessageBox.No)
                    if answer == QMessageBox.No:
                        return
                else:
                    return ssh

        def launch(self, main_window, args, defs):
            account = self.accounts[self.accounts_combo.currentIndex()]
            user, host = account.userhost.split('@')
            port = account.port
            ssh = self.connect(host, user, port)
            if ssh is None: return

            filename = os.path.basename(main_window.document.filename)

            workdir = self.workdir.text()

            account.directories[self.filename] = workdir
            account.save_params()

            if not workdir:
                _, stdout, _ = ssh.exec_command("pwd")
                workdir = stdout.read().decode('utf8').strip()
            elif not workdir.startswith('/'):
                _, stdout, _ = ssh.exec_command("pwd")
                workdir = '/'.join((stdout.read().decode('utf8').strip(), workdir))
            ssh.exec_command("mkdir -p {}".format(quote(workdir)))
            ssh.exec_command("cd {}".format(quote(workdir)))

            self.mutex = QMutex()

            dock = OutputWindow(self, main_window, "Launch at " + account.name)
            try:
                bottom_docked = [w for w in main_window.findChildren(QDockWidget)
                                 if main_window.dockWidgetArea(w) == (Qt.BottomDockWidgetArea)][-1]
            except IndexError:
                main_window.addDockWidget(Qt.BottomDockWidgetArea, dock)
            else:
                main_window.addDockWidget(Qt.BottomDockWidgetArea, dock)
                main_window.tabifyDockWidget(bottom_docked, dock)
                dock.show()
                dock.raise_()

            dock.thread = RemotePlaskThread(ssh, account, filename, dock, self.mutex, main_window, args, defs)
            dock.thread.finished.connect(dock.thread_finished)
            dock.thread.start()

        def select_workdir(self):
            if self.current_account is None:
                return

            user, host = self.accounts[self.current_account].userhost.split('@')
            port = self.accounts[self.current_account].port
            ssh = self.connect(host, user, port)
            if ssh is None: return

            workdir = self.workdir.text()
            if not workdir.startswith('/'):
                _, stdout, _ = ssh.exec_command("pwd")
                home = stdout.read().decode('utf8').strip()
                if workdir: workdir = '/'.join((home, workdir))
                else: workdir = home

            sftp = ssh.open_sftp()

            dialog = RemoteDirDialog(sftp, host, workdir)
            if dialog.exec_() == QDialog.Accepted:
                self.workdir.setText(dialog.item_path(dialog.tree.currentItem()))


    class RemoteDirDialog(QDialog):

        def __init__(self, sftp, host='/', path=None, parent=None):
            self.folder_icon = QIcon.fromTheme('folder')
            super(RemoteDirDialog, self).__init__(parent)
            self.setWindowTitle("Select Folder")
            self.sftp = sftp
            if path is None: path = ['']
            layout = QVBoxLayout()
            label = QLabel("Please choose a folder on the remote machine.")
            layout.addWidget(label)
            self.tree = QTreeWidget()
            self.tree.setHeaderHidden(True)
            layout.addWidget(self.tree)
            buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
            buttons.accepted.connect(self.accept)
            buttons.rejected.connect(self.reject)
            layout.addWidget(buttons)
            self.setLayout(layout)
            item = QTreeWidgetItem()
            item.setText(0, host)
            item.setIcon(0, QIcon.fromTheme('network-server'))
            item.setChildIndicatorPolicy(QTreeWidgetItem.ShowIndicator)
            self.tree.addTopLevelItem(item)
            self.tree.itemExpanded.connect(self.item_expanded)
            self.resize(540, 720)
            item.setExpanded(True)
            for d in path[1:].split('/'):
                found = False
                for i in range(item.childCount()):
                    child = item.child(i)
                    if child.text(0) == d:
                        item = child
                        found = True
                        break
                if not found: break
                item.setExpanded(True)
            self.tree.scrollToItem(item)
            self.tree.setCurrentItem(item)

        @staticmethod
        def item_path(item):
            path = []
            while item:
                path.insert(0, item.text(0))
                item = item.parent()
            return '/' + '/'.join(path[1:])

        def item_expanded(self, item):
            if item.childIndicatorPolicy() == QTreeWidgetItem.ShowIndicator:
                path = self.item_path(item)
                dirs = []
                try:
                    for f in self.sftp.listdir_attr(path):
                        if S_ISDIR(f.st_mode) and not f.filename.startswith('.'): dirs.append(f)
                except (IOError, SystemError, paramiko.SSHException):
                    pass
                else:
                    dirs.sort(key=lambda d: d.filename)
                    for d in dirs:
                        sub = QTreeWidgetItem()
                        sub.setText(0, d.filename)
                        sub.setIcon(0, self.folder_icon)
                        sub.setChildIndicatorPolicy(QTreeWidgetItem.ShowIndicator)
                        item.addChild(sub)
                item.setChildIndicatorPolicy(QTreeWidgetItem.DontShowIndicatorWhenChildless)


LAUNCHERS.append(Launcher())
