/*
 * This file is part of PLaSK (https://plask.app) by Photonics Group at TUL
 * Copyright (c) 2022 Lodz University of Technology
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef PLASK__MODULE_THERMAL_TFem3D_H
#define PLASK__MODULE_THERMAL_TFem3D_H

#include <plask/plask.hpp>
#include <plask/common/fem.hpp>

namespace plask { namespace thermal { namespace dynamic {

/**
 * Solver performing calculations in 2D Cartesian or Cylindrical space using finite element method
 */
struct PLASK_SOLVER_API DynamicThermalFem3DSolver: public FemSolverWithMaskedMesh<Geometry3D, RectangularMesh<3>> {

  protected:

    double maxT;        ///< Maximum temperature recorded

    DataVector<double> temperatures;           ///< Computed temperatures

    DataVector<double> thickness;               ///< Thicknesses of the layers

    DataVector<Vec<3,double>> fluxes;      ///< Computed (only when needed) heat fluxes on our own mesh

    /// Set stiffness matrix + load vector
    void setMatrix(FemMatrix& A, FemMatrix& B, DataVector<double>& F,
                   const BoundaryConditionsWithMesh<RectangularMesh<3>::Boundary,double>& btemperature
                  );

    /// Create 3D-vector with calculated heat fluxes
    void saveHeatFluxes(); // [W/m^2]

    /// Initialize the solver
    void onInitialize() override;

    /// Invalidate the data
    void onInvalidate() override;

  public:

    // Boundary conditions
    BoundaryConditions<RectangularMesh<3>::Boundary,double> temperature_boundary;      ///< Boundary condition of constant temperature (K)

    typename ProviderFor<Temperature, Geometry3D>::Delegate outTemperature;

    typename ProviderFor<HeatFlux, Geometry3D>::Delegate outHeatFlux;

    typename ProviderFor<ThermalConductivity, Geometry3D>::Delegate outThermalConductivity;

    ReceiverFor<Heat, Geometry3D> inHeat;

    double inittemp;       ///< Initial temperature
    double methodparam;   ///< Initial parameter determining the calculation method (0.5 - Crank-Nicolson, 0 - explicit, 1 - implicit)
    double timestep;       ///< Time step in nanoseconds
    double elapstime;    ///< Calculations elapsed time
    bool lumping;          ///< Wheter use lumping for matrices?
    size_t rebuildfreq;    ///< Frequency of mass matrix rebuilding
    size_t logfreq;        ///< Frequency of iteration progress reporting

    /**
     * Run temperature calculations
     * \return max correction of temperature against the last call
     **/
    double compute(double time);

    /// Get calculations elapsed time
    double getElapsTime() const { return elapstime; }

    void loadConfiguration(XMLReader& source, Manager& manager) override; // for solver configuration (see: *.xpl file with structures)

    DynamicThermalFem3DSolver(const std::string& name="");

    std::string getClassName() const override { return "thermal.Dynamic3D"; }

    ~DynamicThermalFem3DSolver();

  protected:

    struct ThermalConductivityData: public LazyDataImpl<Tensor2<double>> {
        const DynamicThermalFem3DSolver* solver;
        shared_ptr<const MeshD<3>> dest_mesh;
        InterpolationFlags flags;
        LazyData<double> temps;
        ThermalConductivityData(const DynamicThermalFem3DSolver* solver, const shared_ptr<const MeshD<3>>& dst_mesh);
        Tensor2<double> at(std::size_t i) const override;
        std::size_t size() const override;
    };

    const LazyData<double> getTemperatures(const shared_ptr<const MeshD<3>>& dst_mesh, InterpolationMethod method) const;

    const LazyData<Vec<3>> getHeatFluxes(const shared_ptr<const MeshD<3>>& dst_mesh, InterpolationMethod method);

    const LazyData<Tensor2<double>> getThermalConductivity(const shared_ptr<const MeshD<3>>& dst_mesh, InterpolationMethod method);
};

}} //namespaces

} // namespace plask

#endif
