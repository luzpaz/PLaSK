# -*- coding: utf-8 -*-

from PyQt4 import QtCore
from xml.etree import ElementTree
from model.table import TableModel
from model.info import Info
from collections import OrderedDict
#from guis import DefinesEditor

MATERIALS_PROPERTES = {
    'A': ('Monomolecular recombination coefficient A [1/s]', ['T — temperature [K]']),
    'absb': ('Absorption coefficient α [cm<sup>-1</sup>]', ['wl — wavelength [nm]', 'T — temperature [K]']),
    'ac': ('Hydrostatic deformation potential for the conduction band a<sub>c</sub> [eV]', ['T — temperature [K]']),
    'av': ('Hydrostatic deformation potential for the valence band a<sub>v</sub> [eV]', ['T — temperature [K]']),
    'B': ('Radiative recombination coefficient B [m<sup>3</sup>/s]', ['T — temperature [K]']),
    'b': ('Shear deformation potential b [eV]', ['T — temperature [K]']),
    'C': ('Auger recombination coefficient C [m<sup>6</sup>/s]', ['T — temperature [K]']),
    'c11': ('Elastic constant c<sub>11</sub> [GPa]', ['T — temperature [K]']),
    'c12': ('Elastic constant c<sub>12</sub> [GPa]', ['T — temperature [K]']),
    'CB': ('Conduction band level CB [eV]', ['T — temperature [K]', 'e — lateral strain [-]', 'point — point in the Brillouin zone [-]']),
    'chi': ('Electron affinity χ [eV]', ['T — temperature [K]', 'e — lateral strain [-]', 'point — point in the Brillouin zone [-]']),
    'cond': ('Electrical conductivity sigma in-plane (lateral) and cross-plane (vertical) direction [S/m]', ['T — temperature [K]']),
    'condtype': ('Electrical conductivity type. In semiconductors this indicates what type of carriers Nf refers to.', []),
    'cp': ('Specific heat at constant pressure [J/(kg K)]', ['T — temperature [K]']),
    'D': ('Ambipolar diffusion coefficient D [m<sup>2</sup>/s]', ['T — temperature [K]']),
    'dens': ('Density [kg/m<sup>3</sup>]', ['T — temperature [K]']),
    'Dso': ('Split-off energy D<sub>so</sub> [eV]', ['T — temperature [K]', 'e — lateral strain [-]']),
    'EactA': ('Acceptor ionization energy E<sub>actA</sub> [eV]', ['T — temperature [K]']),
    'EactD': ('Acceptor ionization energy E<sub>actD</sub> [eV]', ['T — temperature [K]']),
    'Eg': ('Energy gap E<sub>g</sub> [eV]', ['T — temperature [K]', 'e — lateral strain [-]', 'point — point in the Brillouin']),
    'eps': ('Donor ionization energy ε<sub>R</sub> [-]', ['T — temperature [K]']),
    'lattC': ('Lattice constant [Å]', ['T — temperature [K]', 'x — lattice parameter [-]']),
    'Me': ('Electron effective mass M<sub>e</sub> in in-plane (lateral) and cross-plane (vertical) direction [m<sub>0</sub>]', ['T — temperature [K]', 'e — lateral strain [-]', 'point — point in the irreducible Brillouin zone [-]']),
    'Mh': ('Hole effective mass M<sub>h</sub> in in-plane (lateral) and cross-plane (vertical) direction [m<sub>0</sub>]', ['T — temperature [K]', 'e — lateral strain [-]']),
    'Mhh': ('Heavy hole effective mass M<sub>hh</sub> in in-plane (lateral) and cross-plane (vertical) direction [m<sub>0</sub>]', ['T — temperature [K]', 'e — lateral strain [-]']),
    'Mlh': ('Light hole effective mass M<sub>lh</sub> in in-plane (lateral) and cross-plane (vertical) direction [m<sub>0</sub>]', ['T — temperature [K]', 'e — lateral strain [-]']),
    'Mso': ('Split-off mass M<sub>so</sub>` [m<sub>0</sub>]', ['T — temperature [K]', 'e — lateral strain [-]']),
}

class MaterialsModel(TableModel):
       
    class Entry:
        def __init__(self, name, kind = None, base = None, properties = {}, comment = None):
            self.name = name
            self.kind = kind
            self.base = base
            self.properties = properties    #TODO what with duplicate properties, should be supported?
            self.comment = comment
            
        @property
        def kind_or_base(self):
            if self.kind: return self.kind
            if self.base: return self.base
            return ""
    
    def __init__(self, parent=None, info_cb = None, *args):
        TableModel.__init__(self, 'materials', parent, info_cb, *args)
        
    def setXMLElement(self, element):
        self.layoutAboutToBeChanged.emit()
        del self.entries[:]
        if isinstance(element, ElementTree.Element):
            for mat in element.iter("material"):
                self.entries.append(
                        MaterialsModel.Entry(mat.attrib.get("name", ""), mat.attrib.get("kind", None), mat.attrib.get("base", None),
                                             { prop.tag: prop.text for prop in mat })
                )
        self.layoutChanged.emit()
        self.fireChanged()
    
    # XML element that represents whole section
    def getXMLElement(self):
        res = ElementTree.Element(self.name)
        for e in self.entries:
            mat = ElementTree.SubElement(res, "material", { "name": e.name })
            mat.tail = '\n'
            if e.kind: mat.attrib['kind'] = e.kind
            if e.base: mat.attrib['base'] = e.base
            if len(e.properties) > 0:
                mat.text = '\n  '
                prev = None
                for n, v in e.properties.items():
                    if prev: prev.tail = '\n  '
                    p = ElementTree.SubElement(mat, n)
                    p.text = v
                    prev = p
                prev.tail = '\n'
        return res
    
    def get(self, col, row): 
        if col == 0: return self.entries[row].name
        if col == 1: return self.entries[row].kind_or_base
        if col == 2: return self.entries[row].comment
        raise IndexError('column number for MaterialsModel should be 0, 1, or 2, but is %d' % col)
    
    def set(self, col, row, value):
        if col == 0: self.entries[row].name = value
        #elif col == 1: self.entries[row].input = value    #TODO??
        elif col == 2: self.entries[row].comment = value
        else: raise IndexError('column number for MaterialsModel should be 0, 1, or 2, but is %d' % col)       
        
    def createDefaultEntry(self):
        return MaterialsModel.Entry("name")
    
    # QAbstractListModel implementation
    
    def columnCount(self, parent = QtCore.QModelIndex()): 
        return 2    # 3 if comment supported
            
    def headerData(self, col, orientation, role):
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            if col == 0: return 'name'
            if col == 1: return 'kind or base'
            if col == 2: return 'comment'
        return None
    
    def createInfo(self):
        res = super(MaterialsModel, self).createInfo()
        
        names = OrderedDict()
        for i, d in enumerate(self.entries):
            if not d.name:
                res.append(Info('Material name is required [row: %d]' % i, Info.ERROR, rows = [i], cols = [0]))
            else:
                names.setdefault(d.name, []).append(i)
            if not d.kind_or_base:
                res.append(Info('Either kind or base is required [row: %d]' % i, Info.ERROR, rows = [i], cols = [1]))
            elif d.kind and d.base:
                res.append(Info('Kind and base are given, but exactly one is allowed [row: %d]' % i, Info.ERROR, rows = [i], cols = [1]))
        for name, indexes in names.items():
            if len(indexes) > 1:
                res.append(Info('Duplicated definition name "%s" [rows: %s]' % (name, ', '.join(map(str, indexes))),
                                Info.ERROR, rows = indexes, cols = [0]
                                )
                          )
        return res