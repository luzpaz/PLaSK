#!/usr/bin/env python
# -*- coding: utf-8 -*-
import unittest

from numpy import *

import plasktest as ptest

import plask.mesh



class Meshes(unittest.TestCase):

    def setUp(self):
        self.mesh2 = plask.mesh.Rectilinear2D([1,3,2,1], array([10,20], float))
        self.mesh3 = plask.mesh.Rectilinear3D([1,2,3], [10,20], [100,200])

    def testOrdering2D(self):
        m = self.mesh2

        self.assertEqual( [list(i) for i in m], [[1,10], [2,10], [3,10], [1,20], [2,20], [3,20]] )

        m.setOrdering("10")
        self.assertEqual( [list(i) for i in m], [[1,10], [1,20], [2,10], [2,20], [3,10], [3,20]] )

        m.setOrdering("01")
        self.assertEqual( [list(i) for i in m], [[1,10], [2,10], [3,10], [1,20], [2,20], [3,20]] )

        m.setOptimalOrdering()
        self.assertEqual( [list(i) for i in m], [[1,10], [1,20], [2,10], [2,20], [3,10], [3,20]] )

        for i in range(len(m)):
            i0 = m.index0(i)
            i1 = m.index1(i)
            self.assertEqual( m.index(i0, i1), i )


    def testOrdering3D(self):
        m = self.mesh3

        m.setOrdering('102')
        self.assertEqual( [list(i) for i in m], [[1,10,100], [1,20,100], [2,10,100], [2,20,100], [3,10,100], [3,20,100],
                                                 [1,10,200], [1,20,200], [2,10,200], [2,20,200], [3,10,200], [3,20,200]] )

        for i in range(len(m)):
            i0 = m.index0(i)
            i1 = m.index1(i)
            i2 = m.index2(i)
            self.assertEqual( m.index(i0, i1, i2), i )

        m.setOptimalOrdering()
        self.assertEqual( [list(i) for i in m], [[1,10,100], [1,20,100], [1,10,200], [1,20,200],
                                                [2,10,100], [2,20,100], [2,10,200], [2,20,200],
                                                [3,10,100], [3,20,100], [3,10,200], [3,20,200]] )
