#include "femT.h"

namespace plask { namespace solvers { namespace thermal {

template<typename Geometry2DType> FiniteElementMethodThermal2DSolver<Geometry2DType>::FiniteElementMethodThermal2DSolver(const std::string& name) :
    SolverWithMesh<Geometry2DType, RectilinearMesh2D>(name),
    corrlim(0.05),
    inittemp(300.),
    loopno(0),
    corrtype(CORRECTION_ABSOLUTE),
    outTemperature(this, &FiniteElementMethodThermal2DSolver<Geometry2DType>::getTemperatures),
    outHeatFlux(this, &FiniteElementMethodThermal2DSolver<Geometry2DType>::getHeatFluxes),
    algorithm(ALGORITHM_CHOLESKY),
    itererr(1e-8),
    iterlim(10000),
    logfreq(500)
{
    mTemperatures.reset();
    mHeatFluxes.reset();
    inHeatDensity = 0.;
}


template<typename Geometry2DType> FiniteElementMethodThermal2DSolver<Geometry2DType>::~FiniteElementMethodThermal2DSolver() {
}


template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::loadConfiguration(XMLReader &source, Manager &manager)
{
    while (source.requireTagOrEnd())
    {
        std::string param = source.getNodeName();

        if (param == "temperature")
            this->readBoundaryConditions(manager, source, temperature_boundary);

        else if (param == "heatflux")
            this->readBoundaryConditions(manager, source, heatflux_boundary);

        else if (param == "convection")
            this->readBoundaryConditions(manager, source, convection_boundary);

        else if (param == "radiation")
            this->readBoundaryConditions(manager, source, radiation_boundary);

        else if (param == "loop") {
            inittemp = source.getAttribute<double>("inittemp", inittemp);
            corrlim = source.getAttribute<double>("corrlim", corrlim);
            corrtype = source.enumAttribute<CorrectionType>("corrtype")
                .value("absolute", CORRECTION_ABSOLUTE, 3)
                .value("relative", CORRECTION_RELATIVE, 3)
                .get(corrtype);
            source.requireTagEnd();
        }

        else if (param == "matrix") {
            algorithm = source.enumAttribute<Algorithm>("algorithm")
                .value("cholesky", ALGORITHM_CHOLESKY)
                .value("gauss", ALGORITHM_GAUSS)
                .value("iterative", ALGORITHM_ITERATIVE)
                .get(algorithm);
            itererr = source.getAttribute<double>("itererr", itererr);
            iterlim = source.getAttribute<size_t>("iterlim", iterlim);
            logfreq = source.getAttribute<size_t>("logfreq", logfreq);
            source.requireTagEnd();
        } else
            this->parseStandardConfiguration(source, manager);
    }
}


template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::onInitialize() {
    if (!this->geometry) throw NoGeometryException(this->getId());
    if (!this->mesh) throw NoMeshException(this->getId());
    loopno = 0;
    size = this->mesh->size();
    mTemperatures.reset(size, inittemp);
}


template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::onInvalidate() {
    mTemperatures.reset();
    mHeatFluxes.reset();
}

enum BoundarySide { LEFT, RIGHT, TOP, BOTTOM };

/**
    * Helper function for applying boundary conditions of element edges to stiffness matrix.
    * Boundary conditions must be set for both nodes at the element edge.
    * \param boundary_conditions boundary conditions holder
    * \param i1, i2, i3, i4 indices of the lower left, lower right, upper right, and upper left node
    * \param width width of the element
    * \param height height of the element
    * \param[out] F1, F2, F3, F4 references to the load vector components
    * \param[out] K11, K22, K33, K44, K12, K14, K24, K34 references to the stiffness matrix components
    * \param F_function function returning load vector component
    * \param Kmm_function function returning stiffness matrix diagonal component
    * \param Kmn_function function returning stiffness matrix off-diagonal component
    */
template <typename ConditionT>
static void setBoundaries(const BoundaryConditionsWithMesh<RectilinearMesh2D,ConditionT>& boundary_conditions,
                          size_t i1, size_t i2, size_t i3, size_t i4, double width, double height,
                          double& F1, double& F2, double& F3, double& F4,
                          double& K11, double& K22, double& K33, double& K44,
                          double& K12, double& K23, double& K34, double& K41,
                          const std::function<double(double,ConditionT,ConditionT,size_t,size_t,BoundarySide)>& F_function,
                          const std::function<double(double,ConditionT,ConditionT,size_t,size_t,BoundarySide)>& Kmm_function,
                          const std::function<double(double,ConditionT,ConditionT,size_t,size_t,BoundarySide)>& Kmn_function
                         )
{
    auto val1 = boundary_conditions.getValue(i1);
    auto val2 = boundary_conditions.getValue(i2);
    auto val3 = boundary_conditions.getValue(i3);
    auto val4 = boundary_conditions.getValue(i4);
    if (val1 && val2) { // bottom
        F1 += F_function(width, *val1, *val2, i1, i2, BOTTOM); F2 += F_function(width, *val2, *val1, i2, i1, BOTTOM);
        K11 += Kmm_function(width, *val1, *val2, i1, i2, BOTTOM); K22 += Kmm_function(width, *val2, *val1, i2, i1, BOTTOM);
        K12 += Kmn_function(width, *val1, *val2, i1, i2, BOTTOM);
    }
    if (val2 && val3) { // right
        F2 += F_function(height, *val2, *val3, i2, i3, RIGHT); F3 += F_function(height, *val3, *val2, i3, i2, RIGHT);
        K22 += Kmm_function(height, *val2, *val3, i2, i3, RIGHT); K33 += Kmm_function(height, *val3, *val2, i3, i2, RIGHT);
        K23 += Kmn_function(height, *val2, *val3, i2, i3, RIGHT);
    }
    if (val3 && val4) { // top
        F3 += F_function(width, *val3, *val4, i3, i4, TOP); F4 += F_function(width, *val4, *val3, i4, i3, TOP);
        K33 += Kmm_function(width, *val3, *val4, i3, i4, TOP); K44 += Kmm_function(width, *val4, *val3, i4, i3, TOP);
        K34 += Kmn_function(width, *val3, *val4, i3, i4, TOP);
    }
    if (val4 && val1) { // left
        F1 += F_function(height, *val1, *val4, i1, i4, LEFT); F4 += F_function(height, *val4, *val1, i4, i1, LEFT);
        K11 += Kmm_function(height, *val1, *val4, i1, i4, LEFT); K44 += Kmm_function(height, *val4, *val1, i4, i1, LEFT);
        K41 += Kmn_function(height, *val1, *val4, i1, i4, LEFT);
    }
}


template<> template<typename MatrixT>
void FiniteElementMethodThermal2DSolver<Geometry2DCartesian>::setMatrix(MatrixT& A, DataVector<double>& B,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,double>& btemperature,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,double>& bheatflux,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,Convection>& bconvection,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,Radiation>& bradiation
                  )
{
    this->writelog(LOG_DETAIL, "Setting up matrix system (size=%1%, bands=%2%{%3%})", A.size, A.kd+1, A.ld+1);

    auto iMesh = (this->mesh)->getMidpointsMesh();
    auto heatdensities = inHeatDensity(iMesh);

    std::fill_n(A.data, A.size*(A.ld+1), 0.); // zero the matrix
    B.fill(0.);

    std::vector<Box2D> vecbox = this->geometry->getLeafsBoundingBoxes();

    // Set stiffness matrix and load vector
    for (auto e: this->mesh->elements)
    {
        // nodes numbers for the current element
        size_t loleftno = e.getLoLoIndex();
        size_t lorghtno = e.getUpLoIndex();
        size_t upleftno = e.getLoUpIndex();
        size_t uprghtno = e.getUpUpIndex();

        // element size
        double elemwidth = e.getUpper0() - e.getLower0();
        double elemheight = e.getUpper1() - e.getLower1();

        // point and material in the middle of the element
        Vec<2,double> midpoint = e.getMidpoint();
        auto material = this->geometry->getMaterial(midpoint);

        // average temperature on the element
        double temp = 0.25 * (mTemperatures[loleftno] + mTemperatures[lorghtno] + mTemperatures[upleftno] + mTemperatures[uprghtno]);

        // thermal conductivity
        double kx, ky;
        auto leaf = dynamic_pointer_cast<const GeometryObjectD<2>>(this->geometry->getMatchingAt(midpoint, &GeometryObject::PredicateIsLeaf));
        if (leaf)
            std::tie(kx,ky) = std::tuple<double,double>(material->thermk(temp, leaf->getBoundingBox().height()));
        else
            std::tie(kx,ky) = std::tuple<double,double>(material->thermk(temp));

        kx *= elemheight; kx /= elemwidth;
        ky *= elemwidth; ky /= elemheight;

        // load vector: heat densities
        double f = 0.25e-12 * elemwidth * elemheight * heatdensities[e.getIndex()]; // 1e-12 -> to transform µm² into m²

        // set symmetric matrix components
        double k44, k33, k22, k11, k43, k21, k42, k31, k32, k41;

        k44 = k33 = k22 = k11 = (kx + ky) / 3.;
        k43 = k21 = (-2. * kx + ky) / 6.;
        k42 = k31 = - (kx + ky) / 6.;
        k32 = k41 = (kx - 2. * ky) / 6.;

        double f1 = f, f2 = f, f3 = f, f4 = f;

        // boundary conditions: heat flux
        setBoundaries<double>(bheatflux, loleftno, lorghtno, uprghtno, upleftno, elemwidth, elemheight,
                      f1, f2, f3, f4, k11, k22, k33, k44, k21, k32, k43, k41,
                      [](double len, double val, double, size_t, size_t, BoundarySide) { // F
                          return - 0.5e-6 * len * val;
                      },
                      [](double, double, double, size_t, size_t, BoundarySide){return 0.;}, // K diagonal
                      [](double, double, double, size_t, size_t, BoundarySide){return 0.;}  // K off-diagonal
                     );

        // boundary conditions: convection
        setBoundaries<Convection>(bconvection, loleftno, lorghtno, uprghtno, upleftno, elemwidth, elemheight,
                      f1, f2, f3, f4, k11, k22, k33, k44, k21, k32, k43, k41,
                      [](double len, Convection val, Convection, size_t, size_t, BoundarySide) { // F
                          return 0.5e-6 * len * val.coeff * val.ambient;
                      },
                      [](double len, Convection val1, Convection val2, size_t, size_t, BoundarySide) { // K diagonal
                          return (val1.coeff + val2.coeff) * len / 6.;
                      },
                      [](double len, Convection val1, Convection val2, size_t, size_t, BoundarySide) { // K off-diagonal
                          return (val1.coeff + val2.coeff) * len / 12.;
                      }
                     );

        // boundary conditions: radiation
        setBoundaries<Radiation>(bradiation, loleftno, lorghtno, uprghtno, upleftno, elemwidth, elemheight,
                      f1, f2, f3, f4, k11, k22, k33, k44, k21, k32, k43, k41,
                      [this](double len, Radiation val, Radiation, size_t i, size_t, BoundarySide) -> double { // F
                          double a = val.ambient; a = a*a;
                          double T = this->mTemperatures[i]; T = T*T;
                          return - 0.5e-6 * len * val.emissivity * phys::SB * (T*T - a*a);},
                      [](double, Radiation, Radiation, size_t, size_t, BoundarySide){return 0.;}, // K diagonal
                      [](double, Radiation, Radiation, size_t, size_t, BoundarySide){return 0.;}  // K off-diagonal
                     );

        // set stiffness matrix
        A(loleftno, loleftno) += k11;
        A(lorghtno, lorghtno) += k22;
        A(uprghtno, uprghtno) += k33;
        A(upleftno, upleftno) += k44;

        A(lorghtno, loleftno) += k21;
        A(uprghtno, loleftno) += k31;
        A(upleftno, loleftno) += k41;
        A(uprghtno, lorghtno) += k32;
        A(upleftno, lorghtno) += k42;
        A(upleftno, uprghtno) += k43;

        // set load vector
        B[loleftno] += f1;
        B[lorghtno] += f2;
        B[uprghtno] += f3;
        B[upleftno] += f4;
    }

    // boundary conditions of the first kind
    applyBC(A, B, btemperature);

#ifndef NDEBUG
    double* aend = A.data + A.size * A.kd;
    for (double* pa = A.data; pa != aend; ++pa) {
        if (isnan(*pa) || isinf(*pa))
            throw ComputationError(this->getId(), "Error in stiffness matrix at position %1%", pa-A.data);
    }
#endif

}


template<> template<typename MatrixT>
void FiniteElementMethodThermal2DSolver<Geometry2DCylindrical>::setMatrix(MatrixT& A, DataVector<double>& B,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,double>& btemperature,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,double>& bheatflux,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,Convection>& bconvection,
                   const BoundaryConditionsWithMesh<RectilinearMesh2D,Radiation>& bradiation
                  )
{
    this->writelog(LOG_DETAIL, "Setting up matrix system (size=%1%, bands=%2%{%3%})", A.size, A.kd+1, A.ld+1);

    auto iMesh = (this->mesh)->getMidpointsMesh();
    auto heatdensities = inHeatDensity(iMesh);

    std::fill_n(A.data, A.size*(A.ld+1), 0.); // zero the matrix
    B.fill(0.);

    std::vector<Box2D> vecbox = geometry->getLeafsBoundingBoxes();

    // Set stiffness matrix and load vector
    for (auto e: this->mesh->elements)
    {
        // nodes numbers for the current element
        size_t loleftno = e.getLoLoIndex();
        size_t lorghtno = e.getUpLoIndex();
        size_t upleftno = e.getLoUpIndex();
        size_t uprghtno = e.getUpUpIndex();

        // element size
        double elemwidth = e.getUpper0() - e.getLower0();
        double elemheight = e.getUpper1() - e.getLower1();

        // point and material in the middle of the element
        Vec<2,double> midpoint = e.getMidpoint();
        auto material = geometry->getMaterial(midpoint);
        double r = midpoint.rad_r();

        // average temperature on the element
        double temp = 0.25 * (mTemperatures[loleftno] + mTemperatures[lorghtno] + mTemperatures[upleftno] + mTemperatures[uprghtno]);

        // thermal conductivity
        double kx, ky;
        auto leaf = dynamic_pointer_cast<const GeometryObjectD<2>>(geometry->getMatchingAt(midpoint, &GeometryObject::PredicateIsLeaf));
        if (leaf)
            std::tie(kx,ky) = std::tuple<double,double>(material->thermk(temp, leaf->getBoundingBox().height()));
        else
            std::tie(kx,ky) = std::tuple<double,double>(material->thermk(temp));

        kx = kx * elemheight / elemwidth;
        ky = ky * elemwidth / elemheight;

        // load vector: heat densities
        double f = 0.25e-12 * elemwidth * elemheight * heatdensities[e.getIndex()]; // 1e-12 -> to transform um*um into m*m

        // set symmetric matrix components
        double k44, k33, k22, k11, k43, k21, k42, k31, k32, k41;

        k44 = k33 = k22 = k11 = (kx + ky) / 3.;
        k43 = k21 = (-2. * kx + ky) / 6.;
        k42 = k31 = - (kx + ky) / 6.;
        k32 = k41 = (kx - 2. * ky) / 6.;

        double f1 = f, f2 = f, f3 = f, f4 = f;

        // boundary conditions: heat flux
        setBoundaries<double>(bheatflux, loleftno, lorghtno, uprghtno, upleftno, elemwidth, elemheight,
                      f1, f2, f3, f4, k11, k22, k33, k44, k21, k32, k43, k41,
                      [&](double len, double val, double, size_t i1, size_t i2, BoundarySide side) -> double { // F
                            if (side == LEFT) return - 0.5e-6 * len * val * e.getLower0();
                            else if (side == RIGHT) return - 0.5e-6 * len * val * e.getUpper0();
                            else return - 0.5e-6 * len * val * (r + (i1<i2? -len/6. : len/6.));
                      },
                      [](double, double, double, size_t, size_t, BoundarySide){return 0.;}, // K diagonal
                      [](double, double, double, size_t, size_t, BoundarySide){return 0.;}  // K off-diagonal
                     );

        // boundary conditions: convection
        setBoundaries<Convection>(bconvection, loleftno, lorghtno, uprghtno, upleftno, elemwidth, elemheight,
                      f1, f2, f3, f4, k11, k22, k33, k44, k21, k32, k43, k41,
                      [&](double len, Convection val1, Convection val2, size_t i1, size_t i2, BoundarySide side) -> double { // F
                          double a = 0.125e-6 * len * (val1.coeff + val2.coeff) * (val1.ambient + val2.ambient);
                            if (side == LEFT) return a * e.getLower0();
                            else if (side == RIGHT) return a * e.getUpper0();
                            else return a * (r + (i1<i2? -len/6. : len/6.));

                      },
                      [&](double len, Convection val1, Convection val2, size_t i1, size_t i2, BoundarySide side) -> double { // K diagonal
                            double a = (val1.coeff + val2.coeff) * len / 6.;
                            if (side == LEFT) return a * e.getLower0();
                            else if (side == RIGHT) return a * e.getUpper0();
                            else return a * (r + (i1<i2? -len/6. : len/6.));
                      },
                      [&](double len, Convection val1, Convection val2, size_t, size_t, BoundarySide side) -> double { // K off-diagonal
                            double a = (val1.coeff + val2.coeff) * len / 12.;
                            if (side == LEFT) return a * e.getLower0();
                            else if (side == RIGHT) return a * e.getUpper0();
                            else return a * r;
                      }
                     );

        // boundary conditions: radiation
        setBoundaries<Radiation>(bradiation, loleftno, lorghtno, uprghtno, upleftno, elemwidth, elemheight,
                      f1, f2, f3, f4, k11, k22, k33, k44, k21, k32, k43, k41,
                      [&,this](double len, Radiation val, Radiation, size_t i1,  size_t i2, BoundarySide side) -> double { // F
                            double amb = val.ambient; amb = amb*amb;
                            double T = this->mTemperatures[i1]; T = T*T;
                            double a = - 0.5e-6 * len * val.emissivity * phys::SB * (T*T - amb*amb);
                            if (side == LEFT) return a * e.getLower0();
                            else if (side == RIGHT) return a * e.getUpper0();
                            else return a * (r + (i1<i2? -len/6. : len/6.));
                      },
                      [](double, Radiation, Radiation, size_t, size_t, BoundarySide){return 0.;}, // K diagonal
                      [](double, Radiation, Radiation, size_t, size_t, BoundarySide){return 0.;}  // K off-diagonal
                     );

        double kr = ky * elemwidth / 12.;

        // set stiffness matrix
        A(loleftno, loleftno) += r * k11 - kr;
        A(lorghtno, lorghtno) += r * k22 + kr;
        A(uprghtno, uprghtno) += r * k33 + kr;
        A(upleftno, upleftno) += r * k44 - kr;

        A(lorghtno, loleftno) += r * k21;
        A(uprghtno, loleftno) += r * k31;
        A(upleftno, loleftno) += r * k41 + kr;
        A(uprghtno, lorghtno) += r * k32 - kr;
        A(upleftno, lorghtno) += r * k42;
        A(upleftno, uprghtno) += r * k43;

        // set load vector
        B[loleftno] += f1;
        B[lorghtno] += f2;
        B[uprghtno] += f3;
        B[upleftno] += f4;
    }

    // boundary conditions of the first kind
    applyBC(A, B, btemperature);

#ifndef NDEBUG
    double* aend = A.data + A.size * A.kd;
    for (double* pa = A.data; pa != aend; ++pa) {
        if (isnan(*pa) || isinf(*pa))
            throw ComputationError(this->getId(), "Error in stiffness matrix at position %1%", pa-A.data);
    }
#endif

}


template<typename Geometry2DType> double FiniteElementMethodThermal2DSolver<Geometry2DType>::compute(int loops) {
    switch (algorithm) {
        case ALGORITHM_CHOLESKY: return doCompute<DpbMatrix>(loops);
        case ALGORITHM_GAUSS: return doCompute<DgbMatrix>(loops);
        case ALGORITHM_ITERATIVE: return doCompute<SparseBandMatrix>(loops);
    }
    return 0.;
}


template<typename Geometry2DType> template<typename MatrixT> double FiniteElementMethodThermal2DSolver<Geometry2DType>::doCompute(int loops)
{
    this->initCalculation();

    mHeatFluxes.reset();

    // store boundary conditions for current mesh
    auto btemperature = temperature_boundary(this->mesh);
    auto bheatflux = heatflux_boundary(this->mesh);
    auto bconvection = convection_boundary(this->mesh);
    auto bradiation = radiation_boundary(this->mesh);

    this->writelog(LOG_INFO, "Running thermal calculations");

    int loop = 0;
    MatrixT A(size, this->mesh->minorAxis().size());

    double max_abscorr = 0.,
           max_relcorr = 0.;

#   ifndef NDEBUG
        if (!mTemperatures.unique()) this->writelog(LOG_DEBUG, "Temperature data held by something else...");
#   endif
    mTemperatures = mTemperatures.claim();
    DataVector<double> T(size);

    do {
        setMatrix(A, T, btemperature, bheatflux, bconvection, bradiation);

        solveMatrix(A, T);

        saveTemperatures(T);

        if (abscorr > max_abscorr) max_abscorr = abscorr;
        if (relcorr > max_relcorr) max_relcorr = relcorr;

        ++loopno;
        ++loop;

        // show max correction
        this->writelog(LOG_RESULT, "Loop %d(%d): max(T)=%.3fK, update=%.3fK(%.3f%%)", loop, loopno, maxT, abscorr, relcorr);

    } while (((corrtype == CORRECTION_ABSOLUTE)? (abscorr > corrlim) : (relcorr > corrlim)) && (loops == 0 || loop < loops));

    outTemperature.fireChanged();
    outHeatFlux.fireChanged();

    // Make sure we store the maximum encountered values, not just the last ones
    // (so, this will indicate if the results changed since the last run, not since the last loop iteration)
    abscorr = max_abscorr;
    relcorr = max_relcorr;

    if (corrtype == CORRECTION_RELATIVE) return relcorr;
    else return abscorr;
}


template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::solveMatrix(DpbMatrix& A, DataVector<double>& B)
{
    int info = 0;

    this->writelog(LOG_DETAIL, "Solving matrix system");

    // Factorize matrix
    dpbtrf(UPLO, A.size, A.kd, A.data, A.ld+1, info);
    if (info < 0)
        throw CriticalException("%1%: Argument %2% of dpbtrf has illegal value", this->getId(), -info);
    else if (info > 0)
        throw ComputationError(this->getId(), "Leading minor of order %1% of the stiffness matrix is not positive-definite", info);

    // Find solutions
    dpbtrs(UPLO, A.size, A.kd, 1, A.data, A.ld+1, B.data(), B.size(), info);
    if (info < 0) throw CriticalException("%1%: Argument %2% of dpbtrs has illegal value", this->getId(), -info);

    // now A contains factorized matrix and B the solutions
}

template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::solveMatrix(DgbMatrix& A, DataVector<double>& B)
{
    int info = 0;
    this->writelog(LOG_DETAIL, "Solving matrix system");
    int* ipiv = aligned_malloc<int>(A.size);

    A.mirror();

    // Factorize matrix
    dgbtrf(A.size, A.size, A.kd, A.kd, A.data, A.ld+1, ipiv, info);
    if (info < 0) {
        aligned_free(ipiv);
        throw CriticalException("%1%: Argument %2% of dgbtrf has illegal value", this->getId(), -info);
    } else if (info > 0) {
        aligned_free(ipiv);
        throw ComputationError(this->getId(), "Matrix is singlar (at %1%)", info);
    }

    // Find solutions
    dgbtrs('N', A.size, A.kd, A.kd, 1, A.data, A.ld+1, ipiv, B.data(), B.size(), info);
    aligned_free(ipiv);
    if (info < 0) throw CriticalException("%1%: Argument %2% of dgbtrs has illegal value", this->getId(), -info);

    // now A contains factorized matrix and B the solutions
}

template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::solveMatrix(SparseBandMatrix& ioA, DataVector<double>& B)
{
    this->writelog(LOG_DETAIL, "Solving matrix system");

    PrecondJacobi precond(ioA);

    DataVector<double> x = mTemperatures.copy(); // We use previous potentials as initial solution
    double err;
    try {
        int iter = solveDCG(ioA, precond, x.data(), B.data(), err, iterlim, itererr, logfreq, this->getId());
        this->writelog(LOG_DETAIL, "Conjugate gradient converged after %1% iterations.", iter);
    } catch (DCGError exc) {
        throw ComputationError(this->getId(), "Conjugate gradient failed:, %1%", exc.what());
    }

    B = x;

    // now A contains factorized matrix and B the solutions
}


template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::saveTemperatures(DataVector<double>& T)
{
    abscorr = 0.;
    relcorr = 0.;

    maxT = 0.;

    for (auto temp = mTemperatures.begin(), t = T.begin(); t != T.end(); ++temp, ++t)
    {
        double acorr = std::abs(*t - *temp); // for boundary with constant temperature this will be zero anyway
        double rcorr = acorr / *t;
        if (acorr > abscorr) abscorr = acorr;
        if (rcorr > relcorr) relcorr = rcorr;
        if (*t > maxT) maxT = *t;
    }
    relcorr *= 100.; // %
    if (loopno == 0) relcorr = 100.;
    std::swap(mTemperatures, T);
}


template<typename Geometry2DType> void FiniteElementMethodThermal2DSolver<Geometry2DType>::saveHeatFluxes()
{
    this->writelog(LOG_DETAIL, "Computing heat fluxes");

    mHeatFluxes.reset(this->mesh->elements.size());

    for (auto e: this->mesh->elements)
    {
        Vec<2,double> midpoint = e.getMidpoint();
        auto material = this->geometry->getMaterial(midpoint);

        size_t loleftno = e.getLoLoIndex();
        size_t lorghtno = e.getUpLoIndex();
        size_t upleftno = e.getLoUpIndex();
        size_t uprghtno = e.getUpUpIndex();

        double temp = 0.25 * (mTemperatures[loleftno] + mTemperatures[lorghtno] +
                               mTemperatures[upleftno] + mTemperatures[uprghtno]);

        double kx, ky;
        auto leaf = dynamic_pointer_cast<const GeometryObjectD<2>>(
                        this->geometry->getMatchingAt(midpoint, &GeometryObject::PredicateIsLeaf)
                     );
        if (leaf)
            std::tie(kx,ky) = std::tuple<double,double>(material->thermk(temp, leaf->getBoundingBox().height()));
        else
            std::tie(kx,ky) = std::tuple<double,double>(material->thermk(temp));


        mHeatFluxes[e.getIndex()] = vec(
            - 0.5e6 * kx * (- mTemperatures[loleftno] + mTemperatures[lorghtno]
                             - mTemperatures[upleftno] + mTemperatures[uprghtno]) / (e.getUpper0() - e.getLower0()), // 1e6 - from um to m
            - 0.5e6 * ky * (- mTemperatures[loleftno] - mTemperatures[lorghtno]
                             + mTemperatures[upleftno] + mTemperatures[uprghtno]) / (e.getUpper1() - e.getLower1())); // 1e6 - from um to m
    }
}


template<typename Geometry2DType> DataVector<const double> FiniteElementMethodThermal2DSolver<Geometry2DType>::getTemperatures(const MeshD<2>& dst_mesh, InterpolationMethod method) const {
    this->writelog(LOG_DETAIL, "Getting temperatures");
    if (!mTemperatures) return DataVector<const double>(dst_mesh.size(), inittemp); // in case the receiver is connected and no temperature calculated yet
    if (method == INTERPOLATION_DEFAULT) method = INTERPOLATION_LINEAR;
    return interpolate(*(this->mesh), mTemperatures, WrappedMesh<2>(dst_mesh, this->geometry), method);
}


template<typename Geometry2DType> DataVector<const Vec<2> > FiniteElementMethodThermal2DSolver<Geometry2DType>::getHeatFluxes(const MeshD<2>& dst_mesh, InterpolationMethod method) {
    this->writelog(LOG_DETAIL, "Getting heat fluxes");
    if (!mTemperatures) return DataVector<const Vec<2>>(dst_mesh.size(), Vec<2>(0.,0.)); // in case the receiver is connected and no fluxes calculated yet
    if (!mHeatFluxes) saveHeatFluxes(); // we will compute fluxes only if they are needed
    if (method == INTERPOLATION_DEFAULT) method = INTERPOLATION_LINEAR;
    return interpolate(*((this->mesh)->getMidpointsMesh()), mHeatFluxes, WrappedMesh<2>(dst_mesh, this->geometry), method);
}


template<> std::string FiniteElementMethodThermal2DSolver<Geometry2DCartesian>::getClassName() const { return "thermal.Static2D"; }
template<> std::string FiniteElementMethodThermal2DSolver<Geometry2DCylindrical>::getClassName() const { return "thermal.StaticCyl"; }

template struct FiniteElementMethodThermal2DSolver<Geometry2DCartesian>;
template struct FiniteElementMethodThermal2DSolver<Geometry2DCylindrical>;

}}} // namespace plask::solvers::thermal
