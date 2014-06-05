#!/usr/bin/env python
# -*- coding: utf-8 -*-
import unittest

from numpy import *

from plask import *
from plask import material, geometry, mesh
from optical.slab import FourierReflection3D


@material.simple
class Mat(material.Material):
    @staticmethod
    def nr(): return 2.


class Averaging_Test(unittest.TestCase):

    def setUp(self):
        background = geometry.Cuboid(1.0, 1.0, 0.2, None)
        obj = geometry.Cuboid(0.5, 0.5, 0.2, Mat())
        align = geometry.AlignContainerVert3D(bottom=0.)
        align.add(background, back=0., left=0.)
        align.add(obj, back=0., left=0.)
        geom = geometry.Cartesian3D(align, back='periodic', front='periodic', left='periodic', right='periodic')
        self.solver = FourierReflection3D()
        self.solver.geometry = geom
        self.solver.wavelength = 1000.
        self.solver.size = 11, 11        # number of material coefficients in each direction 4*11+1 = 45
        self.solver.refine = 16, 16

    # 2.5 = ( 1**2 + 2**2 ) / 2
    # 1.6 = 2 / ( 1**(-2) + 2**(-2) )

    def testTran(self):
        msh_tran = mesh.Rectangular3D(mesh.Ordered([0.25]), mesh.Regular(0, 1, 46), mesh.Ordered([0.1]))
        prof_tran = self.solver.get_refractive_index_profile(msh_tran, 'nearest')
        self.assertAlmostEqual(prof_tran.array[0,0,0,0], sqrt(2.5), 5)
        self.assertAlmostEqual(prof_tran.array[0,0,0,1], sqrt(1.6), 5)
        self.assertAlmostEqual(prof_tran.array[0,0,0,2], sqrt(2.5), 5)
        self.assertAlmostEqual(prof_tran.array[0,1,0,0], 2., 5)
        self.assertAlmostEqual(prof_tran.array[0,1,0,1], 2., 5)
        self.assertAlmostEqual(prof_tran.array[0,1,0,2], 2., 5)
        self.assertAlmostEqual(prof_tran.array[0,-2,0,0], 1., 5)
        self.assertAlmostEqual(prof_tran.array[0,-2,0,1], 1., 5)
        self.assertAlmostEqual(prof_tran.array[0,-2,0,2], 1., 5)
        self.assertAlmostEqual(prof_tran.array[0,22,0,0], 2., 5)
        self.assertAlmostEqual(prof_tran.array[0,22,0,1], 2., 5)
        self.assertAlmostEqual(prof_tran.array[0,22,0,2], 2., 5)
        self.assertAlmostEqual(prof_tran.array[0,23,0,0], 1., 5)
        self.assertAlmostEqual(prof_tran.array[0,23,0,1], 1., 5)
        self.assertAlmostEqual(prof_tran.array[0,23,0,2], 1., 5)
        print("{} {} {}".format(*prof_tran.array[0, [0,1,-2], 0, :3].real))
        # figure()
        # plot(msh_tran.axis1, prof_tran.array[0,:,0,0].real, '.')
        # plot(msh_tran.axis1, prof_tran.array[0,:,0,1].real, '.')
        # plot(msh_tran.axis1, prof_tran.array[0,:,0,2].real, '.')
        # xlabel(u'$x$ [µm]')
        # ylabel(u'$n_r$')

    def testLong(self):
        msh_long = mesh.Rectangular3D(mesh.Regular(0, 1, 46), mesh.Ordered([0.25]), mesh.Ordered([0.1]))
        prof_long = self.solver.get_refractive_index_profile(msh_long, 'nearest')
        self.assertAlmostEqual(prof_long.array[0,0,0,0], sqrt(1.6), 5)
        self.assertAlmostEqual(prof_long.array[0,0,0,1], sqrt(2.5), 5)
        self.assertAlmostEqual(prof_long.array[0,0,0,2], sqrt(2.5), 5)
        self.assertAlmostEqual(prof_long.array[1,0,0,0], 2., 5)
        self.assertAlmostEqual(prof_long.array[1,0,0,1], 2., 5)
        self.assertAlmostEqual(prof_long.array[1,0,0,2], 2., 5)
        self.assertAlmostEqual(prof_long.array[-2,0,0,0], 1., 5)
        self.assertAlmostEqual(prof_long.array[-2,0,0,1], 1., 5)
        self.assertAlmostEqual(prof_long.array[-2,0,0,2], 1., 5)
        self.assertAlmostEqual(prof_long.array[22,0,0,0], 2., 5)
        self.assertAlmostEqual(prof_long.array[22,0,0,1], 2., 5)
        self.assertAlmostEqual(prof_long.array[22,0,0,2], 2., 5)
        self.assertAlmostEqual(prof_long.array[23,0,0,0], 1., 5)
        self.assertAlmostEqual(prof_long.array[23,0,0,1], 1., 5)
        self.assertAlmostEqual(prof_long.array[23,0,0,2], 1., 5)
        print("{} {} {}".format(*prof_long.array[[0,1,-2], 0, 0, :3].real))
        # figure()
        # plot(msh_long.axis0, prof_long.array[:,0,0,0].real, '.')
        # plot(msh_long.axis0, prof_long.array[:,0,0,1].real, '.')
        # plot(msh_long.axis0, prof_long.array[:,0,0,2].real, '.')
        # xlabel(u'$z$ [µm]')
        # ylabel(u'$n_r$')
