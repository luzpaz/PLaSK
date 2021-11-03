#!/usr/bin/env python
# -*- coding: utf-8 -*-
import unittest

from numpy import *

from plask import *
from plask import material, geometry, mesh
from electrical.shockley import Shockley2D, ShockleyCyl, ActiveCond2D

eps0 = 8.854187817e-6 # pF/µm

@material.simple()
class Conductor(material.Material):
    def cond(self, T):
        return (1e+12, 1e+12)
    def eps(self, T):
        return 1.


class Cond2D_Test(unittest.TestCase):

    def setUp(self):
        rect = geometry.Rectangle(1000., 300., Conductor())
        junc = geometry.Rectangle(1000., 0.2, 'GaAs')
        junc.role = 'active'
        stack = geometry.Stack2D()
        stack.append(rect)
        stack.append(junc)
        stack.append(rect)
        space = geometry.Cartesian2D(stack, length=1000.)
        self.solver = ActiveCond2D("electrical2d")
        self.solver.geometry = space
        generator = mesh.Rectangular2D.DivideGenerator()
        generator.prediv = 2,1
        self.solver.mesh = generator
        self.solver.cond = lambda j, T: 0.05 + j
        self.solver.maxerr = 1e-5
        self.solver.voltage_boundary.append(self.solver.mesh.Top(), 0.)
        self.solver.voltage_boundary.append(self.solver.mesh.Bottom(), 1.)

    def testComputations(self):
        self.solver.compute()
        correct_current = 500.
        self.assertAlmostEqual(self.solver.get_total_current(), correct_current, 3)


if __name__ == '__main__':
    test = unittest.main(exit=False)
    show()
    sys.exit(not test.result.wasSuccessful())
