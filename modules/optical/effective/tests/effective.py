#!/usr/bin/env python
# -*- coding: utf-8 -*-
import unittest

from numpy import *

from plask import *
from plask import material, geometry, mesh
from plask.optical.effective import EffectiveIndex2D

@material.simple
class LowContrastMaterial(material.Material):
    def Nr(self, wl, T): return 1.3

class EffectiveIndex2D_Test(unittest.TestCase):

    def setUp(self):
        self.module = EffectiveIndex2D()
        rect = geometry.Rectangle(1.0, 0.5, LowContrastMaterial())
        space = geometry.Space2DCartesian(rect, left="mirror")
        self.module.geometry = space

    def testExceptions(self):
        self.module.inWavelength.disconnect()
        self.module.invalidate()
        with self.assertRaisesRegexp(TypeError, r"^No wavelength set nor its provider connected\.$"):
            self.module.inWavelength()
        with self.assertRaisesRegexp(ValueError, r"^Effective index cannot be provided now\.$"):
            self.module.outNeff()
        with self.assertRaisesRegexp(ValueError, r"^Light intensity cannot be provided now\.$"):
            self.module.outIntensity(mesh.Rectilinear2D([1,2],[3,4]))

    def testSymmetry(self):
        self.assertIsNone( self.module.symmetry )
        self.module.symmetry = "-"
        self.assertEqual( self.module.symmetry, "negative" )

    def testReceivers(self):
        self.module.inWavelength = 850.
        self.assertEqual( self.module.inWavelength(), 850. )
