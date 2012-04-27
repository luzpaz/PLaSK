#!/usr/bin/env python
# -*- coding: utf-8 -*-
import unittest

import sys
if sys.version < "2.7":
    unittest.TestCase.assertIsNone = lambda self, value: self.assertTrue(item is None)
    unittest.TestCase.assertIn = lambda self, item, container: self.assertTrue(item in container)

import plask, plask.materials, plask.geometry



class CalculationSpaces(unittest.TestCase):

    def setUp(self):
        self.axes_backup = plask.config.axes
        plask.config.axes = "xy"

    def tearDown(self):
        plask.config.axes = self.axes_backup


    def testBorders(self):
        r = plask.geometry.Rectangle(2.,1., "Al(0.2)GaN")
        s = plask.Space2DCartesian(r, x_lo="mirror" , right="AlN", top="GaN")
        print s.bbox
        #self.assertEqual( s.borders, {'left': "mirror", 'right': "AlN", 'top': "GaN", 'bottom': None} )
        self.assertEqual( str(s.getMaterial(-1.5, 0.5)), "Al(0.2)GaN" )
        self.assertEqual( str(s.getMaterial(3., 0.5)), "AlN" )
        self.assertEqual( str(s.getMaterial(-3., 0.5)), "AlN" )
        self.assertEqual( str(s.getMaterial(0., 2.)), "GaN" )

        if sys.version >= 2.7:
            with self.assertRaises(RuntimeError):
                plask.Space2DCartesian(r, right="mirror").getMaterial(3., 0.5)

    def testSubspace(self):
        stack = plask.geometry.Stack2D()
        r1 = plask.geometry.Rectangle(2.,2., "GaN")
        r2 = plask.geometry.Rectangle(2.,1., "Al(0.2)GaN")
        stack.append(r1, "l")
        h2 = stack.append(r2, "l")
        space = plask.Space2DCartesian(stack)
        subspace = space.getSubspace(r2)
        v1 = space.getLeafsPositions(h2)
        v2 = subspace.getLeafsPositions(h2)
        self.assertEqual( space.getLeafsPositions(h2)[0], subspace.getLeafsPositions(h2)[0] )
