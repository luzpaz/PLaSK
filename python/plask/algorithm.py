# -*- coding: utf-8 -*-
'''
This module constains the set of high-level PLaSK algorithm typically used
in computations of semiconductor lasers.

TODO doc

'''
import plask


class ThermoElectric(object):
    '''
    Algorithm for thermo-electric calculations without the optical part.

    This algorithm performs under-threshold thermo-electrical computations.
    It computes electric current flow and tempereture distribution in a self-
    consistent loop until desired convergence is reached.

    ThermoElectric(thermal, electrical, tfreq=6)

    Parameters
    ----------
    thermal : thermal solver
        Configured thermal solver. In must have `outTemperature` provider and
        `inHeat` receiver. Temperature computations are done with `compute`
        method, which takes maximum number of iterations as input and returns
        maximum convergence error.
    electrical : electrical solver
        Configured electrical solver. It must have `outHeat` provider and
        `inTemperature` receiver. Temperature computations are done with
        `compute` method, which takes maximum number of iterations as input and
        returns maximum convergence error. Solver specific parameters (e.g.
        `beta`) should already be set before execution of the algorithm.
    tfreq : integer, optional
        Number of electrical iterations per single thermal step. As temperature
        tends to converge faster, it is reasonable to repeat thermal solution
        less frequently.

    Solvers specified on construction of this algorithm are automatically
    connected. Then the computations can be executed using `run` method, after
    which the results may be save to the HDF5 file with `save` or presented
    visually using `plot_`... methods.

    '''

    def __init__(self, thermal, electrical, tfreq=6):
        electrical.inTemperature = thermal.outTemperature
        thermal.inHeat = electrical.outHeat
        self.thermal = thermal
        self.electrical = electrical
        self.tfreq = tfreq

    def run(self):
        '''
        Execute the algorithm.

        In the beginning the solvers are invalidated and next, the thermo-
        electric algorithm is executed until both solvers converge to the
        value specified in their configuration in the `corrlim` property.
        '''
        self.thermal.invalidate()
        self.electrical.invalidate()
        self.electrical.invalidate()

        verr = 2. * self.electrical.corrlim
        terr = 2. * self.thermal.corrlim
        while terr > self.thermal.corrlim or verr > self.electrical.corrlim:
            verr = self.electrical.compute(self.tfreq)
            terr = self.thermal.compute(1)

    def save(self, filename):
        raise NotImplementedError('save')

    def plot_temperature(self, geometry_color='w', mesh_color=None):
        '''
        Plot computed temperature to the current axes.

        Parameters
        ----------
        geometry_color : string or None
            Matplotlib color specification of the geometry. If None, structure
            is not plotted.
        mesh_color : string or None
            Matplotlib color specification of the mesh. If None, the mesh is
            not plotted.

        See Also
        --------
        plask.plot_field : Plot any field obtained from receivers
        '''
        field = self.thermal.outTemperature(self.thermal.mesh)
        plask.plot_field(field)
        cbar = plask.colorbar(use_gridspec=True)
        plask.xlabel(u"$%s$ [\xb5m]" % plask.config.axes[-2])
        plask.ylabel(u"$%s$ [\xb5m]" % plask.config.axes[-1])
        cbar.set_label("Temperature [K]")
        if geometry_color is not None:
            plask.plot_geometry(self.thermal.geometry, color=geometry_color)
        if mesh_color is not None:
            plask.plot_mesh(self.thermal.mesh, color=mesh_color)
        plask.gcf().canvas.set_window_title("Temperature")

    def plot_voltage(self, geometry_color='w', mesh_color=None):
        '''
        Plot computed voltage to the current axes.

        Parameters
        ----------
        geometry_color : string or None
            Matplotlib color specification of the geometry. If None, structure
            is not plotted.
        mesh_color : string or None
            Matplotlib color specification of the mesh. If None, the mesh is
            not plotted.

        See Also
        --------
        plask.plot_field : Plot any field obtained from receivers
        '''
        field = self.electrical.outVoltage(self.electrical.mesh)
        plask.plot_field(field)
        cbar = plask.colorbar(use_gridspec=True)
        plask.xlabel(u"$%s$ [\xb5m]" % plask.config.axes[-2])
        plask.ylabel(u"$%s$ [\xb5m]" % plask.config.axes[-1])
        cbar.set_label("Voltage [V]")
        if geometry_color is not None:
            plask.plot_geometry(self.electrical.geometry, color=geometry_color)
        if mesh_color is not None:
            plask.plot_mesh(self.electrical.mesh, color=mesh_color)
        plask.gcf().canvas.set_window_title("Voltage")

    def plot_junction_current(self, refine=16):
        '''
        Plot current density at the active region.

        Parameters
        ----------
        refine : integer, optional
            Numeber of points in the plot between each two points in
            the computational mesh.
        '''
        # A little magic to get junction position first
        points = self.electrical.mesh.get_midpoints()
        geom = self.electrical.geometry.item
        yy = plask.unique(points.index1(i) for i,p in enumerate(points)
                          if geom.has_role('junction', p)
                          or geom.has_role('active', p))
        yy = [int(y) for y in yy]
        if len(yy) == 0:
            raise ValueError("no junction defined")
        act = []
        start = yy[0]
        axis1 = self.electrical.mesh.axis1
        for i, y in enumerate(yy):
            if y > yy[i-1] + 1:
                act.append(0.5 * (axis1[start] + axis1[yy[i-1] + 1]))
                start = y
        act.append(0.5 * (axis1[start] + axis1[yy[-1] + 1]))

        axis = plask.concatenate([
            plask.linspace(x, self.electrical.mesh.axis0[i+1], refine+1)
            for i,x in enumerate(list(self.electrical.mesh.axis0)[:-1])
        ])

        for i, y in enumerate(act):
            msh = plask.mesh.Rectilinear2D(axis, [y])
            curr = self.electrical.outCurrentDensity(msh, 'spline').array[:,0,1]
            s = sum(curr)
            plask.plot(msh.axis0, curr if s > 0 else -curr,
                       label="Junction %d" % (i + 1))
        if len(act) > 1:
            plask.legend(loc='best')
        plask.xlabel(u"$%s$ [\xb5m]" % plask.config.axes[-2])
        plask.ylabel(u"Current Density [kA/cm\xb2]")
        plask.gcf().canvas.set_window_title("Current Density")


class ThresholdSearch(ThermoElectric):
    '''
    Algorithm for threshold search of semiconductor laser.

    This algorithm performs thermo-electrical computations followed by
    determination ot threshold current and optical analysis in order to
    determine the threshold of a semiconductor laser. The search is
    performed by scipy root finding algorithm in order to determine
    the volatage and electric current ensuring no optical loss in the
    laser cavity.

    ThresholdSearch(thermal, electrical, diffusion, gain, optical,
                    tfreq, approx_mode, quick=False)

    Parameters
    ----------
    thermal : thermal solver
        Configured thermal solver. In must have `outTemperature` provider and
        `inHeat` receiver. Temperature computations are done with `compute`
        method, which takes maximum number of iterations as input and returns
        maximum convergence error.
    electrical : electrical solver
        Configured electrical solver. It must have `outHeat` provider and
        `inTemperature` receiver. Temperature computations are done with
        `compute` method, which takes maximum number of iterations as input and
        returns maximum convergence error. Solver specific parameters (e.g.
        `beta`) should already be set before execution of the algorithm.
    tfreq : integer, optional
        Number of electrical iterations per single thermal step. As temperature
        tends to converge faster, it is reasonable to repeat thermal solution
        less frequently.
    TODO
    approx_mode : float
        Approximation of the optical mode (either the effective index or
        the wavelength) needed for optical computation.
    quick : bool, optional
        If this parameter is True, the algorithm tries to find the threshold
        the easy way i.e. by computing only the optical determinant in each
        iteration.

    Solvers specified on construction of this algorithm are automatically
    connected. Then the computations can be executed using `run` method, after
    which the results may be save to the HDF5 file with `save` or presented
    visually using `plot_`... methods.

    '''
    pass
