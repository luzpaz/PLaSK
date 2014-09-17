from lxml.etree import ElementTree, SubElement

from ...utils.xml import AttributeReader, OrderedTagReader, require_no_children, UnorderedTagReader
from .grid import Grid


class AxisConf(object):
    """Store axis configuration of rectilinear mesh"""

    def __init__(self, start=None, stop=None, num=None, points=None, type=None):
        self.start = start
        self.stop = stop
        self.num = num
        self.points = points
        self.type = None if type == '' else type

    def fill_XMLElement(self, axisElement):
        for attr in ['start', 'stop', 'num', 'type']:
            a = getattr(self, attr, None)
            if a is not None: axisElement.attrib[attr] = a
        axisElement.text = self.points if self.points else ''

        #if self.start: axisElement.attrib['start'] = self.start
        #if self.stop: axisElement.attrib['stop'] = self.stop
        #if self.num: axisElement.attrib['num'] = self.num
        #if self.points: axisElement.text = ", ".join(self.points)
        #if self.points: axisElement.attrib['points'] = self.points

    def set_from_XML(self, axis_element):
        if axis_element is None:
            for attr in ['start', 'stop', 'num', 'type', 'points']: setattr(self, attr, None)
        else:
            require_no_children(axis_element)
            with AttributeReader(axis_element) as a:
                for attr in ['start', 'stop', 'num', 'type']:
                    setattr(self, attr, a.get(attr, None))
            self.points = axis_element.text
            #self.points = [float(x) for x in axis_element.text.split(',')] #can have {...}


#RectangularMesh1D(Grid)
#if mesh_type in ('ordered', 'regular'):

class RectangularMesh1D(Grid):
    """Model of 1D rectangular mesh (ordered or regular)"""

    @staticmethod
    def from_XML(grids_model, element):
        e = RectangularMesh1D(grids_model, element.attrib['name'], element.attrib['type'])
        e.set_XML_element(element)
        return e

    def __init__(self, grids_model, name, type):
        super(RectangularMesh1D, self).__init__(grids_model, name, type)
        self.axis = AxisConf()

    @property
    def is_regular(self):
        return self.type == 'regular'

    def get_XML_element(self):
        res = super(RectangularMesh1D, self).get_XML_element()
        self.axis.fill_XMLElement(SubElement(res, 'axis'))
        return res

    def set_XML_element(self, element):
        super(RectangularMesh1D, self).set_XML_element(element)
        with OrderedTagReader(element) as r: self.axis.set_from_XML(r.find('axis'))

    def get_controller(self, document):
        from ...controller.grids.mesh_rectilinear import RectangularMesh1DConroller
        return RectangularMesh1DConroller(document=document, model=self)



class RectangularMesh(Grid):
    """Model of 2D and 3D rectangular mesh"""

    @staticmethod
    def from_XML(grids_model, element):
        e = RectangularMesh(grids_model, int(element.attrib['type'][-2]), element.attrib['name'])
        e.set_XML_element(element)
        return e

    def __init__(self, grids_model, dim, name):
        super(RectangularMesh, self).__init__(grids_model, name, 'rectangular{}d'.format(dim))
        self.dim = dim
        self.axis = [AxisConf() for _ in range(0, self.dim)]

    @staticmethod
    def axis_tag_name(nr):
        return "axis{}".format(nr)

    def get_XML_element(self):
        res = super(RectangularMesh, self).get_XML_element()
        for i in range(0, self.dim):
            self.axis[i].fill_XMLElement(SubElement(res, RectangularMesh.axis_tag_name(i)))
        return res

    def set_XML_element(self, element):
        super(RectangularMesh, self).set_XML_element(element)
        with UnorderedTagReader(element) as r:
            for i in range(0, self.dim):
                self.axis[i].set_from_XML(r.find(RectangularMesh.axis_tag_name(i)))

    def get_controller(self, document):
        from ...controller.grids.mesh_rectilinear import RectangularMeshConroller
        return RectangularMeshConroller(document=document, model=self)