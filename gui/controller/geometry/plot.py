# -*- coding: utf-8 -*-
# ### plot_geometry ###
import math

import plask
import matplotlib
import matplotlib.colors
import matplotlib.lines
import matplotlib.patches
import matplotlib.artist

from zlib import adler32

_geometry_drawers = {}

class BBoxIntersection(matplotlib.transforms.BboxBase):
    """
    A :class:`Bbox` equals to intersection of 2 other bounding boxes.
    When any of the children box changes, the bounds of this bbox will update accordingly.
    """
    def __init__(self, bbox1, bbox2, **kwargs):
        """
        :param bbox1: a first child :class:`Bbox`
        :param bbox2: a second child :class:`Bbox`
        """
        assert bbox1.is_bbox
        assert bbox2.is_bbox

        matplotlib.transforms.BboxBase.__init__(self, **kwargs)
        self._bbox1 = bbox1
        self._bbox2 = bbox2
        self.set_children(bbox1, bbox2)

    def __repr__(self):
        return "BBoxIntersection(%r, %r)" % (self._bbox, self._bbox)

    def get_points(self):
        if self._invalid:
            box = matplotlib.transforms.Bbox.intersection(self._bbox1, self._bbox2)
            if box is not None:
                self._points = box.get_points()
            else:
                self._points = matplotlib.transforms.Bbox.from_bounds(0, 0, -1, -1).get_points()
            self._invalid = 0
        return self._points
    get_points.__doc__ = matplotlib.transforms.Bbox.get_points.__doc__


def material_to_color(material):
    '''
        Generate color for given material.
        :param plask.Material material: material
        :return (float, float, float): RGB color, 3 floats, each in range [0, 1]
    '''
    i = adler32(str(material))      #maybe crc32?
    return (i & 0xff) / 255.0, ((i >> 8) & 0xff) / 255.0, ((i >> 16) & 0xff) / 255.0

class DrawEnviroment(object):
    '''
        Drawing configuration.
    '''

    def __init__(self, axes, artist_dst, fill = False, color = 'k', lw = 1.0, z_order=3.0):
        '''
        :param axes: plane to draw (important in 3D)
        :param artist_dst: mpl axis where artist should be appended
        :param bool fill: True if artists should be filled
        :param color: edge color (mpl format)
        :param lw: line width
        :param z_order: artists z order
        '''
        super(DrawEnviroment, self).__init__()
        self.patches = artist_dst
        self.fill = fill
        self.color = color
        self.lw = lw
        self.axes = axes
        self.z_order = z_order

    def append(self, artist, clip_box, geometry_object):
        '''
        Configure and append artist to destination axis object.
        :param artist: artist to append
        :param matplotlib.transforms.BboxBase clip_box: clipping box for artist, optional
        :param geometry_object: plask's geometry object which is represented by artist
        '''
        if self.fill:
            artist.set_fill(True)
            artist.set_facecolor(material_to_color(geometry_object.representative_material))
        else:
            artist.set_fill(False)
        artist.set_linewidth(self.lw)
        artist.set_ec(self.color)
        artist.set_zorder(self.z_order)
        self.patches.add_patch(artist)
        if clip_box is not None:
            artist.set_clip_box(BBoxIntersection(clip_box, artist.get_clip_box()))
            #artist.set_clip_box(clip_box)
            #artist.set_clip_on(True)
            #artist.set_clip_path(clip_box)



def _draw_Block(env, geometry_object, transform, clip_box):
    bbox = geometry_object.bbox
    block = matplotlib.patches.Rectangle(
        (bbox.lower[env.axes[0]], bbox.lower[env.axes[1]]),
        bbox.upper[env.axes[0]]-bbox.lower[env.axes[0]], bbox.upper[env.axes[1]]-bbox.lower[env.axes[1]],
        transform = transform
    )
    env.append(block, clip_box, geometry_object)

_geometry_drawers[plask.geometry.Block2D] = _draw_Block
_geometry_drawers[plask.geometry.Block3D] = _draw_Block

def _draw_Triangle(env, geometry_object, transform, clip_box):
    p1 = geometry_object.a
    p2 = geometry_object.b
    env.append(matplotlib.patches.Polygon(((0.0, 0.0), (p1[0], p1[1]), (p2[0], p2[1])), closed=True, transform=transform),
               clip_box, geometry_object
    )

_geometry_drawers[plask.geometry.Triangle] = _draw_Triangle

def _draw_Circle(env, geometry_object, transform, clip_box):
    env.append(matplotlib.patches.Circle((0.0, 0.0), geometry_object.radius, transform=transform),
               clip_box, geometry_object
    )

_geometry_drawers[plask.geometry.Circle] = _draw_Circle
_geometry_drawers[plask.geometry.Sphere] = _draw_Circle

def _draw_Cylinder(env, geometry_object, transform, clip_box):
    if env.axes != (0, 1) and env.axes != (1, 0):
        _draw_Block(env, geometry_object, transform, clip_box)
    else:
        _draw_Circle(env, geometry_object, transform, clip_box)

_geometry_drawers[plask.geometry.Cylinder] = _draw_Cylinder


def _draw_Translation(env, geometry_object, transform, clip_box):
    new_transform = matplotlib.transforms.Affine2D()
    t = geometry_object.translation
    new_transform.translate(t[env.axes[0]], t[env.axes[1]])
    _draw_geometry_object(env, geometry_object.item, new_transform + transform, clip_box)

_geometry_drawers[plask.geometry.Translation2D] = _draw_Translation
_geometry_drawers[plask.geometry.Translation3D] = _draw_Translation

def _draw_Flip(env, geometry_object, transform, clip_box):
    if geometry_object.axis_nr == env.axes[0]:
        _draw_geometry_object(env, geometry_object.item, matplotlib.transforms.Affine2D.from_values(-1.0, 0, 0, 1.0, 0, 0) + transform, clip_box)
    elif geometry_object.axis_nr == env.axes[1]:
        _draw_geometry_object(env, geometry_object.item, matplotlib.transforms.Affine2D.from_values(1.0, 0, 0, -1.0, 0, 0) + transform, clip_box)
    else:
        _draw_geometry_object(env, geometry_object.item, transform, clip_box)

_geometry_drawers[plask.geometry.Flip2D] = _draw_Flip
_geometry_drawers[plask.geometry.Flip3D] = _draw_Flip

def _draw_Mirror(env, geometry_object, transform, clip_box):
    _draw_geometry_object(env, geometry_object.item, transform, clip_box)
    if geometry_object.axis_nr in env.axes: #in 3D this must not be true
        _draw_Flip(env, geometry_object, transform, clip_box)

_geometry_drawers[plask.geometry.Mirror2D] = _draw_Mirror
_geometry_drawers[plask.geometry.Mirror3D] = _draw_Mirror

def _draw_Clip(env, geometry_object, transform, clip_box):
    def _b(bound):
        return math.copysign(1e100, bound) if math.isinf(bound) else bound

    obj_box = geometry_object.clip_box

    new_clipbox = matplotlib.transforms.TransformedBbox(
       #matplotlib.transforms.Bbox([
       #    [obj_box.lower[env.axes[0]], obj_box.lower[env.axes[1]]],
       #    [obj_box.upper[env.axes[0]], obj_box.upper[env.axes[1]]]
       #]),
       matplotlib.transforms.Bbox.from_extents(_b(obj_box.lower[env.axes[0]]), _b(obj_box.lower[env.axes[1]]),
                                               _b(obj_box.upper[env.axes[0]]), _b(obj_box.upper[env.axes[1]])),
       transform
    )

    if clip_box is None:
        clip_box = new_clipbox
    else:
        clip_box = BBoxIntersection(clip_box, new_clipbox)

    x0, y0, x1, y1 = clip_box.extents
    if x0 < x1 and y0 < y1:
        _draw_geometry_object(env, geometry_object.item, transform, clip_box)
    # else, if clip_box is empty now, it will be never non-empty, so all will be clipped-out

_geometry_drawers[plask.geometry.Clip2D] = _draw_Clip
_geometry_drawers[plask.geometry.Clip3D] = _draw_Clip

#TODO intersection approximated support: clip to bounding box


def _draw_geometry_object(env, geometry_object, transform, clip_box):
    '''
    Draw geometry object.
    :param DrawEnviroment env: drawing configuration
    :param geometry_object: object to draw
    :param transform: transform from a plot coordinates to the geometry_object
    :param matplotlib.transforms.BboxBase clip_box: clipping box in plot coordinates
    '''
    if geometry_object is None: return
    drawer = _geometry_drawers.get(type(geometry_object))
    if drawer is None:
        try:
            for child_index in range(0, geometry_object._children_len()):
                _draw_geometry_object(env, geometry_object._child(child_index), transform, clip_box)
            #for child in geometry_object:
            #    _draw_geometry_object(env, child, transform, clip_box)
        except TypeError:
            pass    #ignore non-iterable object
    else:
        drawer(env, geometry_object, transform, clip_box)

def plot_geometry_object(figure, geometry, color='k', lw=1.0, plane=None, set_limits=False, zorder=3, mirror=False, fill = False):
    '''
        Plot geometry.
        :param figure: destination figure (to which axes with geometry plot should be added)
        :param geometry: geometry object to draw
        :param color: edges color
        :param lw: width of edges
        :param plane: planes to draw (important for 3D geometries)
        :param set_limits:
        :param zorder:
        :param bool mirror:
        :param bool fill: if True, geometry objects will be filled with colors which depends on their material,
            This is not supported when geometry is of type Cartesian3D, and then the fill parameter is ignored.
    '''

    #figure.clear()
    axes = figure.add_subplot(111)

    if type(geometry) == plask.geometry.Cartesian3D:
        fill = False    # we ignore fill parameter in 3D
        dd = 0
        #if plane is None: plane = 'xy'
        ax = _get_2d_axes(plane)
        dirs = tuple((("back", "front"), ("left", "right"), ("top", "bottom"))[i] for i in ax)
    else:
        dd = 1
        ax = (0,1)
        dirs = (("inner", "outer") if type(geometry) == plask.geometry.Cylindrical2D else ("left", "right"),
                ("top", "bottom"))

    env = DrawEnviroment(ax, axes, fill, color, lw, z_order=zorder)

    hmirror = mirror and (geometry.borders[dirs[0][0]] == 'mirror' or geometry.borders[dirs[0][1]] == 'mirror' or dirs[0][0] == "inner")
    vmirror = mirror and (geometry.borders[dirs[1][0]] == 'mirror' or geometry.borders[dirs[1][1]] == 'mirror')

    _draw_geometry_object(env, geometry, axes.transData, None)
    if vmirror:
        _draw_geometry_object(env, geometry, matplotlib.transforms.Affine2D.from_values(-1.0, 0, 0, 1.0, 0, 0) + axes.transData, None)
    if hmirror:
        _draw_geometry_object(env, geometry, matplotlib.transforms.Affine2D.from_values(1.0, 0, 0, -1.0, 0, 0) + axes.transData, None)

    if set_limits:
        box = geometry.bbox
        if hmirror:
            m = max(abs(box.lower[ax[0]]), abs(box.upper[ax[0]]))
            axes.set_xlim(-m, m)
        else:
            axes.set_xlim(box.lower[ax[0]], box.upper[ax[0]])
        if vmirror:
            m = max(abs(box.lower[ax[1]]), abs(box.upper[ax[1]]))
            axes.set_ylim(-m, m)
        else:
            axes.set_ylim(box.lower[ax[1]], box.upper[ax[1]])

    if ax[0] > ax[1] and not axes.yaxis_inverted():
        axes.invert_yaxis()

    #TODO changed 9 XII 2014 in order to avoid segfault in GUI:
    # matplotlib.pyplot.xlabel -> axes.set_xlabel
    # matplotlib.pyplot.ylabel -> axes.set_ylabel
    axes.set_xlabel(u"${}$ [µm]".format(plask.config.axes[dd + ax[0]]))
    axes.set_ylabel(u"${}$ [µm]".format(plask.config.axes[dd + ax[1]]))

    return env.patches






# ------------ old plot code: -------------------

_geometry_plotters = {}

def _add_path_Block(patches, trans, box, ax, hmirror, vmirror, color, lw, zorder):
    def add_path(bottom):
        lefts = []
        if hmirror:
            if box.lower[ax[0]] == 0.:
                box.lower[ax[0]] = -box.upper[ax[0]]
            elif box.upper[ax[0]] == 0.:
                box.upper[ax[0]] = -box.lower[ax[0]]
            else:
                lefts.append(-box.upper[ax[0]])
        lefts.append(box.lower[ax[0]])
        for left in lefts:
            patches.append(matplotlib.patches.Rectangle((left, bottom),
                                                        box.upper[ax[0]]-box.lower[ax[0]], box.upper[ax[1]]-box.lower[ax[1]],
                                                        ec=color, lw=lw, fill=False, zorder=zorder))
    if vmirror:
        if box.lower[ax[1]] == 0.:
            box.lower[ax[1]] = -box.upper[ax[1]]
        elif box.upper[ax[1]] == 0.:
            box.upper[ax[1]] = -box.lower[ax[1]]
        else:
            add_path(-box.upper[ax[1]])

    add_path(box.lower[ax[1]])
_geometry_plotters[plask.geometry.Block2D] = _add_path_Block
_geometry_plotters[plask.geometry.Block3D] = _add_path_Block

def _add_path_Triangle(patches, trans, box, ax, hmirror, vmirror, color, lw, zorder):
    p0 = trans.translation
    p1 = p0 + trans.item.a
    p2 = p0 + trans.item.b
    patches.append(matplotlib.patches.Polygon(((p0[0], p0[1]), (p1[0], p1[1]), (p2[0], p2[1])),
                                              closed=True, ec=color, lw=lw, fill=False, zorder=zorder))
    if vmirror:
        patches.append(matplotlib.patches.Polygon(((p0[0], -p0[1]), (p1[0], -p1[1]), (p2[0], p2[1])),
                                                  closed=True, ec=color, lw=lw, fill=False, zorder=zorder))
    if hmirror:
        patches.append(matplotlib.patches.Polygon(((-p0[0], p0[1]), (-p1[0], p1[1]), (-p2[0], p2[1])),
                                                  closed=True, ec=color, lw=lw, fill=False, zorder=zorder))
        if vmirror:
            patches.append(matplotlib.patches.Polygon(((-p0[0], -p0[1]), (-p1[0], -p1[1]), (-p2[0], -p2[1])),
                                                      closed=True, ec=color, lw=lw, fill=False, zorder=zorder))

_geometry_plotters[plask.geometry.Triangle] = _add_path_Triangle

def _add_path_Circle(patches, trans, box, ax, hmirror, vmirror, color, lw, zorder):
    tr = trans.translation
    vecs = [ tr ]
    if hmirror: vecs.append(plask.vec(-tr[0], tr[1]))
    if vmirror: vecs.append(plask.vec(tr[0], -tr[1]))
    if hmirror and vmirror: vecs.append(plask.vec(-tr[0], -tr[1]))
    for vec in vecs:
        patches.append(matplotlib.patches.Circle(vec, trans.item.radius, ec=color, lw=lw, fill=False, zorder=zorder))

_geometry_plotters[plask.geometry.Circle] = _add_path_Circle

def _add_path_Cylinder(patches, trans, box, ax, hmirror, vmirror, color, lw, zorder):
    if ax != (0,1) and ax != (1,0):
        _add_path_Block(patches, trans, box, ax, hmirror, vmirror, color, lw, zorder)
    else:
        tr = trans.translation
        if ax == (1,0): tr = plask.vec(tr[1], tr[0])
        vecs = [ tr ]
        if hmirror: vecs.append(plask.vec(-tr[0], tr[1]))
        if vmirror: vecs.append(plask.vec(tr[0], -tr[1]))
        if hmirror and vmirror: vecs.append(plask.vec(-tr[0], -tr[1]))
        for vec in vecs:
            patches.append(matplotlib.patches.Circle(vec, trans.item.radius,
                                                     ec=color, lw=lw, fill=False, zorder=zorder))
_geometry_plotters[plask.geometry.Cylinder] = _add_path_Cylinder

def _add_path_Sphere(patches, trans, box, ax, hmirror, vmirror, color, lw, zorder):
    tr = trans.translation[ax[0]], trans.translation[ax[1]]
    vecs = [ tr ]
    if hmirror: vecs.append(plask.vec(-tr[0], tr[1]))
    if vmirror: vecs.append(plask.vec(tr[0], -tr[1]))
    if hmirror and vmirror: vecs.append(plask.vec(-tr[0], -tr[1]))
    for vec in vecs:
        patches.append(matplotlib.patches.Circle(vec, trans.item.radius, ec=color, lw=lw, fill=False, zorder=zorder))

_geometry_plotters[plask.geometry.Sphere] = _add_path_Sphere


def plot_geometry(figure, geometry, color='k', lw=1.0, plane=None, set_limits=False, zorder=3, mirror=False):
    '''Plot geometry.'''
    #TODO documentation

    #figure.clear()
    axes = figure.add_subplot(111)
    patches = []

    if type(geometry) == plask.geometry.Cartesian3D:
        dd = 0
        ax = _get_2d_axes(plane)
        dirs = tuple((("back", "front"), ("left", "right"), ("top", "bottom"))[i] for i in ax)
    else:
        dd = 1
        ax = (0,1)
        dirs = (("inner", "outer") if type(geometry) == plask.geometry.Cylindrical2D else ("left", "right"),
                ("top", "bottom"))

    hmirror = mirror and (geometry.borders[dirs[0][0]] == 'mirror' or geometry.borders[dirs[0][1]] == 'mirror' or dirs[0][0] == "inner")
    vmirror = mirror and (geometry.borders[dirs[1][0]] == 'mirror' or geometry.borders[dirs[1][1]] == 'mirror')

    for trans,box in zip(geometry.get_leafs_translations(), geometry.get_leafs_bboxes()):
        if box:
            _geometry_plotters[type(trans.item)](patches, trans, box, ax, hmirror, vmirror, color, lw, zorder)

    for patch in patches:
        axes.add_patch(patch)

    if set_limits:
        box = geometry.bbox
        if hmirror:
            m = max(abs(box.lower[ax[0]]), abs(box.upper[ax[0]]))
            axes.set_xlim(-m, m)
        else:
            axes.set_xlim(box.lower[ax[0]], box.upper[ax[0]])
        if vmirror:
            m = max(abs(box.lower[ax[1]]), abs(box.upper[ax[1]]))
            axes.set_ylim(-m, m)
        else:
            axes.set_ylim(box.lower[ax[1]], box.upper[ax[1]])

    if ax[0] > ax[1] and not axes.yaxis_inverted():
        axes.invert_yaxis()

    #TODO changed 9 XII 2014 in order to avoid segfault in GUI:
    # matplotlib.pyplot.xlabel -> axes.set_xlabel
    # matplotlib.pyplot.ylabel -> axes.set_ylabel
    axes.set_xlabel(u"${}$ [µm]".format(plask.config.axes[dd + ax[0]]))
    axes.set_ylabel(u"${}$ [µm]".format(plask.config.axes[dd + ax[1]]))

    return patches