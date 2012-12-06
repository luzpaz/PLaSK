#!/usr/bin/env python
# -*- coding: utf-8 -*-
import unittest


import plask


class Manager(unittest.TestCase):

    def setUp(self):
        self.manager = plask.Manager()
        self.manager.load('''
        <plask>
            <geometry>
                <cartesian2d name="Space-1" axes="xy">
                    <stack name="Stack-2">
                        <child path="Path-4"><rectangle name="Block-3" dx="5" dy="2" material="GaN" /></child>
                        <ref name="Block-3"/>
                    </stack>
                </cartesian2d>
            </geometry>
            <grids>
                <mesh type="rectilinear2d" name="lin">
                    <axis0>1, 2, 3</axis0>
                    <axis1 start="10" stop="30" num="3"/>
                </mesh>
                <mesh type="regular2d" name="reg">
                    <axis0 start="10" stop="30" num="3"/>
                    <axis1 start="1" stop="3" num="3"/>
                </mesh>
                <generator type="rectilinear2d" method="divide" name="test">
                    <prediv by="4"/>
                    <postdiv by0="2" by1="3"/>
                    <warnings missing="false"/>
                </generator>
                <generator type="rectilinear2d" method="divide" name="refined">
                    <postdiv by0="2"/>
                    <warnings multiple="no"/>
                    <refinements>
                        <vertical object="Block-3" at="1.0"/>
                        <horizontal object="Block-3" path="Path-4" at="1.0"/>
                    </refinements>
                </generator>
            </grids>
        </plask>
        ''')

    def testGeometry(self):
        self.assertEqual( len(self.manager.objects), 2 )
        self.assertEqual( type(self.manager.objects["Block-3"]), plask.geometry.Block2D )
        self.assertEqual( list(self.manager.objects["Stack-2"].get_leafs_bboxes()),
            [plask.geometry.Box2D(0,0,5,2), plask.geometry.Box2D(0,2,5,4)] )
        self.assertEqual( type(self.manager.geometries.Space_1), plask.geometry.Cartesian2D )
        self.assertEqual( len(self.manager.pth), 1 )
        with self.assertRaises(KeyError): self.manager.objects["nonexistent"]

    def testDictionaries(self):
        self.assertEqual( list(self.manager.obj), ["Block-3", "Stack-2"] )

    def testExport(self):
        self.manager.export(globals())
        self.assertIn( "Space-1", GEO )
        self.assertEqual( type(GEO.Space_1), plask.geometry.Cartesian2D )

    def testMesh(self):
        self.assertEqual( len(self.manager.meshes), 2 )
        self.assertEqual( self.manager.msh.lin.axis0 , [1, 2, 3] )
        self.assertEqual( self.manager.msh.lin.axis1 , [10, 20, 30] )
        self.assertEqual( list(self.manager.msh["reg"].axis1) , [1, 2, 3] )
        self.assertEqual( list(self.manager.msh["reg"].axis0) , [10, 20, 30] )

    def testGenerators(self):
        self.assertEqual( self.manager.msg.test.prediv, (4,4) )
        self.assertEqual( self.manager.msg.test.postdiv, (2,3) )
        self.assertEqual( self.manager.msg.test.warn_missing, False )

        mesh = self.manager.msg.refined.generate(self.manager.obj.Stack_2)
        self.assertEqual( mesh.axis1, [0., 2., 3., 4.] )
        self.assertEqual( mesh.axis0, [0.0, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0] )

    def testException(self):
        manager = plask.Manager()
        with self.assertRaises(plask.XMLError):
            manager.load('''
            <plask>
                <geometry>
                    <cartesian2d name="Space-1" axes="xy">
                        <stack name="Stack-2">
                            <child path="Path-4"><rectangle name="Block-3" x="5" y="2" material="GaN" /></child>
                            <ref name="Block-3"/>
                    </cartesian2d>
                </geometry>
            </plask>
            ''')
        with self.assertRaises(plask.XMLError):
            manager.load('''
            <plask>
                <grids>
                    <mesh type="rectilinear2d" name="lin">
                        <axis0>1, 2, 3</axis0>
                        <axis0>10 20 30</axis0>
                    </mesh>
                </grids>
            <plask>
            ''')
        with self.assertRaises(plask.XMLError):
            manager.load('''
            <plask>
                <grids>
                    <generator type="rectilinear2d" method="divide" name="test">
                        <postdiv by="4" by0="2" by1="3"/>
                    </generator>
                </grids>
            </plask>
            ''')
        with self.assertRaises(plask.XMLError):
            manager.load('''
            <plask>
                <grids>
                    <generator type="rectilinear2d" method="divide" name="test">
                        <postdiv bye="4"/>
                    </generator>
                </grids>
            </plask>
            ''')
        with self.assertRaises(plask.XMLError):
            manager.load('''
            <plask>
                <geometry>
                    <cartesian2d name="Space-2" axes="xy">
                        <rectangle x="5" y="2" material="GaN" />
                    </cartesian2d>
            </plask>
            ''')


    def testSolverConnections(self):
        manager = plask.Manager()
        manager.load('''
        <plask>
            <solvers>
                <plasktest lib="solvers" solver="InOut" name="output"/>
                <plasktest lib="solvers" solver="InOut" name="input"/>
            </solvers>
            <connects>
                <connect out="output.outWavelength" in="input.inWavelength"/>
            </connects>
        </plask>
        ''')
        self.assertEqual( manager.solvers.output.inWavelength(), 2 )
        self.assertEqual( manager.solvers.input.inWavelength(), 5 )


    def testMaterials(self):
        manager = plask.Manager()
        manager.load('''
        <plask>
            <materials>
                <material name="XmlMat" kind="dielectric">
                    <nr>1. + 0.001*T + 0.0001*wl</nr>
                    <absp>1.</absp>
                </material>
                <material name="XmlMat:Mg" base="GaN:Mg">
                    <nr>1. + 0.001*T + 0.0001*wl</nr>
                    <absp>T * self.dc</absp>
                </material>
                <material name="XmlMatMg20" base="GaN:Mg=1e20">
                    <nr>1. + 0.001*T + 0.0001*wl</nr>
                    <absp>T * self.dc</absp>
                </material>
                <material name="XmlMatSimple" kind="dielectric">
                    <nr>3.5</nr>
                    <absp>0.</absp>
                </material>
            </materials>
        </plask>
        ''')
        material.update_factories()
        mat = plask.material.XmlMat()
        self.assertAlmostEqual( mat.nr(900, 300), 1.39 )
        self.assertAlmostEqual( mat.nR(900, 300), 1.39-7.16197244e-06j )
        self.assertEqual( plask.material.XmlMatSimple().nR_tensor(900, 300), (3.5, 3.5, 3.5, 0., 0.) )


        mad = plask.material.XmlMat(dp="Mg", dc=1e18)
        self.assertEqual( mad.cond(300), material.GaN(dp="Mg", dc=1e18).cond(300) )
        self.assertEqual( mad.absp(900, 300), 300 * 1e18 )

        mad20 = plask.material.XmlMatMg20()
        self.assertEqual( mad20.cond(300), material.GaN(dp="Mg", dc=1e20).cond(300) )
