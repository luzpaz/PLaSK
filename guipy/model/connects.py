from PyQt4 import QtCore
from lxml import etree as ElementTree
from model.table import TableModel
from model.info import Info
#from guis import DefinesEditor

class ConnectsModel(TableModel):
    
    class Entry:
        def __init__(self, output, input, comment = None):
            self.output = output
            self.input = input
            self.comment = comment
    
    def __init__(self, parent=None, info_cb = None, *args):
        TableModel.__init__(self, 'connects', parent, info_cb, *args)
        
    def set_XML_element(self, element):
        self.layoutAboutToBeChanged.emit()
        if element is not None:
            self.entries = [ConnectsModel.Entry(c.attrib.get("out", ""), c.attrib.get("in", "")) for c in element.iter("connect")]
        else:
            self.entries = []
        self.layoutChanged.emit()
        self.fire_changed()
    
    # XML element that represents whole section
    def get_XML_element(self):
        res = ElementTree.Element(self.name)
        for e in self.entries:
            ElementTree.SubElement(res, "connect", { "out": e.output, "in": e.input }).tail = '\n'
        return res
    
    def get(self, col, row):
        if col == 0: return self.entries[row].output
        if col == 1: return self.entries[row].input
        if col == 2: return self.entries[row].comment
        raise IndexError('column number for ConnectsModel should be 0, 1, or 2, but is %d' % col)
    
    def set(self, col, row, value):
        if col == 0: self.entries[row].output = value
        elif col == 1: self.entries[row].input = value
        elif col == 2: self.entries[row].comment = value
        else: raise IndexError('column number for ConnectsModel should be 0, 1, or 2, but is %d' % col)       
        
    def createDefaultEntry(self):
        return ConnectsModel.Entry("out", "in")
    
    # QAbstractListModel implementation
    
    def columnCount(self, parent = QtCore.QModelIndex()): 
        return 2    # 3 if comment supported
            
    def headerData(self, col, orientation, role):
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            if col == 0: return 'output provider'
            if col == 1: return 'input receiver'
            if col == 2: return 'comment'
        return None
    
    def create_info(self):
        res = super(ConnectsModel, self).create_info()
        for i, d in enumerate(self.entries):
            if not d.output: res.append(Info('Connection output is required [row: %d]' % i, Info.ERROR, rows = [i], cols = [0]))
            if not d.input: res.append(Info('Connection input is required [row: %d]' % i, Info.ERROR, rows = [i], cols = [1]))
        return res