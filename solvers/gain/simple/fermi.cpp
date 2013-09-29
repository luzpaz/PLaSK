#include "fermi.h"

namespace plask { namespace solvers { namespace fermi {

template <typename GeometryType>
FermiGainSolver<GeometryType>::FermiGainSolver(const std::string& name): SolverWithMesh<GeometryType,RectilinearMesh1D>(name),
    outGain(this, &FermiGainSolver<GeometryType>::getGain),
    outGainOverCarriersConcentration(this, &FermiGainSolver<GeometryType>::getdGdn)// getDelegated will be called whether provider value is requested
{
    inTemperature = 300.; // temperature receiver has some sensible value
    mLifeTime = 0.1; // [ps]
    mMatrixElem = 10.0;
    cond_waveguide_depth = 0.26; // [eV]
    vale_waveguide_depth = 0.13; // [eV]
    differenceQuotient = 0.01;  // [%]
    inTemperature.changedConnectMethod(this, &FermiGainSolver<GeometryType>::onInputChange);
    inCarriersConcentration.changedConnectMethod(this, &FermiGainSolver<GeometryType>::onInputChange);
}

template <typename GeometryType>
FermiGainSolver<GeometryType>::~FermiGainSolver() {
    inTemperature.changedDisconnectMethod(this, &FermiGainSolver<GeometryType>::onInputChange);
    inCarriersConcentration.changedDisconnectMethod(this, &FermiGainSolver<GeometryType>::onInputChange);
    if (extern_levels) {
        delete[] extern_levels->el;
        delete[] extern_levels->hh;
        delete[] extern_levels->lh;
    }
}


template <typename GeometryType>
void FermiGainSolver<GeometryType>::loadConfiguration(XMLReader& reader, Manager& manager)
{
    // Load a configuration parameter from XML.
    while (reader.requireTagOrEnd())
    {
        std::string param = reader.getNodeName();
        if (param == "config") {
            mLifeTime = reader.getAttribute<double>("lifetime", mLifeTime);
            mMatrixElem = reader.getAttribute<double>("matrix_elem", mMatrixElem);
            reader.requireTagEnd();
        } else
            this->parseStandardConfiguration(reader, manager, "<geometry>, <mesh>, or <config>");
    }
}


template <typename GeometryType>
void FermiGainSolver<GeometryType>::onInitialize() // In this function check if geometry and mesh are set
{
    if (!this->geometry) throw NoGeometryException(this->getId());

    detectActiveRegions();

    //TODO

    outGain.fireChanged();
}


template <typename GeometryType>
void FermiGainSolver<GeometryType>::onInvalidate() // This will be called when e.g. geometry or mesh changes and your results become outdated
{
    //TODO (if needed)
}

//template <typename GeometryType>
//void FermiGainSolver<GeometryType>::compute()
//{
//    this->initCalculation(); // This must be called before any calculation!
//}




template <typename GeometryType>
void FermiGainSolver<GeometryType>::detectActiveRegions()
{
    shared_ptr<RectilinearMesh2D> mesh = makeGeometryGrid(this->geometry->getChild());
    shared_ptr<RectilinearMesh2D> points = mesh->getMidpointsMesh();

    size_t ileft = 0, iright = points->axis0.size();
    bool in_active = false;

    for (size_t r = 0; r < points->axis1.size(); ++r)
    {
        bool had_active = false; // indicates if we had active region in this layer
        shared_ptr<Material> layer_material;
        bool layer_QW = false;

        for (size_t c = 0; c < points->axis0.size(); ++c)
        { // In the (possible) active region
            auto point = points->at(c,r);
            auto tags = this->geometry->getRolesAt(point);
            bool active = tags.find("active") != tags.end();
            bool QW = tags.find("QW") != tags.end()/* || tags.find("QD") != tags.end()*/;

            if (QW && !active)
                throw Exception("%1%: All marked quantum wells must belong to marked active region.", this->getId());

            if (c < ileft)
            {
                if (active)
                    throw Exception("%1%: Left edge of the active region not aligned.", this->getId());
            }
            else if (c >= iright)
            {
                if (active)
                    throw Exception("%1%: Right edge of the active region not aligned.", this->getId());
            }
            else
            {
                // Here we are inside potential active region
                if (active)
                {
                    if (!had_active)
                    {
                        if (!in_active)
                        { // active region is starting set-up new region info
                            regions.emplace_back(mesh->at(c,r));
                            ileft = c;
                        }
                        layer_material = this->geometry->getMaterial(point);
                        layer_QW = QW;
                    }
                    else
                    {
                        if (*layer_material != *this->geometry->getMaterial(point))
                            throw Exception("%1%: Non-uniform active region layer.", this->getId());
                        if (layer_QW != QW)
                            throw Exception("%1%: Quantum-well role of the active region layer not consistent.", this->getId());
                    }
                }
                else if (had_active)
                {
                    if (!in_active)
                    {
                        iright = c;
                        if (layer_QW)
                        { // quantum well is at the egde of the active region, add one row below it
                            if (r == 0)
                                throw Exception("%1%: Quantum-well at the edge of the structure.", this->getId());
                            auto bottom_material = this->geometry->getMaterial(points->at(ileft,r-1));
                            for (size_t cc = ileft; cc < iright; ++cc)
                                if (*this->geometry->getMaterial(points->at(cc,r-1)) != *bottom_material)
                                    throw Exception("%1%: Material below quantum well not uniform.", this->getId());
                            auto& region = regions.back();
                            double w = mesh->axis0[iright] - mesh->axis0[ileft];
                            double h = mesh->axis1[r] - mesh->axis1[r-1];
                            region.origin += Vec<2>(0., -h);
                            region.layers->push_back(make_shared<Block<2>>(Vec<2>(w, h), bottom_material));
                        }
                    }
                    else
                        throw Exception("%1%: Right edge of the active region not aligned.", this->getId());
                }
                had_active |= active;
            }
        }
        in_active = had_active;

        // Now fill-in the layer info
        ActiveRegionInfo* region = regions.empty()? nullptr : &regions.back();
        if (region)
        {
            double h = mesh->axis1[r+1] - mesh->axis1[r];
            double w = mesh->axis0[iright] - mesh->axis0[ileft];
            if (in_active)
            {
                size_t n = region->layers->getChildrenCount();
                shared_ptr<Block<2>> last;
                if (n > 0) last = static_pointer_cast<Block<2>>(static_pointer_cast<Translation<2>>(region->layers->getChildNo(n-1))->getChild());
                assert(!last || last->size.c0 == w);
                if (last && layer_material == last->getRepresentativeMaterial() && layer_QW == region->isQW(region->size()-1))  //TODO check if usage of getRepresentativeMaterial is fine here (was material)
                {
                    last->setSize(w, last->size.c1 + h);
                }
                else
                {
                    auto layer = make_shared<Block<2>>(Vec<2>(w,h), layer_material);
                    if (layer_QW) layer->addRole("QW");
                    region->layers->push_back(layer);
                }
            }
            else
            {
                if (region->isQW(region->size()-1))
                { // top layer of the active region is quantum well, add the next layer
                    auto top_material = this->geometry->getMaterial(points->at(ileft,r));
                    for (size_t cc = ileft; cc < iright; ++cc)
                        if (*this->geometry->getMaterial(points->at(cc,r)) != *top_material)
                            throw Exception("%1%: Material above quantum well not uniform.", this->getId());
                    region->layers->push_back(make_shared<Block<2>>(Vec<2>(w,h), top_material));
                }
                ileft = 0;
                iright = points->axis0.size();
            }
        }
    }
    if (!regions.empty() && regions.back().isQW(regions.back().size()-1))
        throw Exception("%1%: Quantum-well at the edge of the structure.", this->getId());
    for (auto& region: regions) region.summarize(this);
}


template <typename GeometryType>
QW::gain FermiGainSolver<GeometryType>::getGainModule(double wavelength, double T, double n, const ActiveRegionInfo& region)
{
    QW::gain gainModule;

    gainModule.Set_temperature(T);
    gainModule.Set_koncentr(n);

    Tensor2<double> qme, qmhh, qmlh, bme, bmhh, bmlh;

    #pragma omp critical // necessary as the material may be defined in Python
    {
        qme = region.materialQW->Me(T); qmhh = region.materialQW->Mhh(T); qmlh = region.materialQW->Mlh(T);
        bme = region.materialBarrier->Me(T); bmhh = region.materialBarrier->Mhh(T); bmlh = region.materialBarrier->Mlh(T);
        gainModule.Set_refr_index(region.materialQW->nr(wavelength, T));
        gainModule.Set_split_off(region.materialQW->Dso(T));
        gainModule.Set_bandgap(region.materialQW->Eg(T));

        gainModule.Set_conduction_depth(region.materialBarrier->CBO(T) - region.materialQW->CBO(T));
        gainModule.Set_valence_depth(region.materialQW->VBO(T) - region.materialBarrier->VBO(T));
    }

    gainModule.Set_electron_mass_in_plain(qme.c00);
    gainModule.Set_electron_mass_transverse(qme.c11);
    gainModule.Set_heavy_hole_mass_in_plain(qmhh.c00);
    gainModule.Set_heavy_hole_mass_transverse(qmhh.c11);
    gainModule.Set_light_hole_mass_in_plain(qmlh.c00);
    gainModule.Set_light_hole_mass_transverse(qmlh.c11);

    gainModule.Set_electron_mass_in_barrier(bme.c00);
    gainModule.Set_heavy_hole_mass_in_barrier(bmhh.c00);
    gainModule.Set_light_hole_mass_in_barrier(bmlh.c00);

    gainModule.Set_well_width(region.qwlen);
    gainModule.Set_waveguide_width(region.totallen);

    gainModule.Set_cond_waveguide_depth(cond_waveguide_depth);
    gainModule.Set_vale_waveguide_depth(vale_waveguide_depth);

    gainModule.Set_lifetime(mLifeTime);
    gainModule.Set_momentum_matrix_element(mMatrixElem);

    // Now compute levels
    if (extern_levels) {
        gainModule.przygobl_n(*extern_levels, gainModule.przel_dlug_z_angstr(region.qwtotallen));
    } else {
        gainModule.przygobl_n(gainModule.przel_dlug_z_angstr(region.qwtotallen));
    }

    return gainModule;
}

//  TODO: it should return computed levels
template <typename GeometryType>
/// Function computing energy levels
std::deque<std::tuple<std::vector<double>,std::vector<double>,std::vector<double>,double,double>>
FermiGainSolver<GeometryType>::determineLevels(double T, double n)
{
    this->initCalculation(); // This must be called before any calculation!

    std::deque<std::tuple<std::vector<double>,std::vector<double>,std::vector<double>,double,double>> result;

    if (regions.size() == 1)
        this->writelog(LOG_DETAIL, "Found 1 active region");
    else
        this->writelog(LOG_DETAIL, "Found %1% active regions", regions.size());

    for (int act=0; act<regions.size(); act++)
    {
        double qFlc, qFlv;
        std::vector<double> el, hh, lh;

        this->writelog(LOG_DETAIL, "Evaluating energy levels for active region nr %1%:", act+1);

        QW::gain gainModule = getGainModule(0.0, T, n, regions[act]); //wavelength=0.0 - no refractive index needs to be calculated (any will do)

        writelog(LOG_RESULT, "Conduction band quasi-Fermi level (from the band edge) = %1% eV", qFlc = gainModule.Get_qFlc());
        writelog(LOG_RESULT, "Valence band quasi-Fermi level (from the band edge) = %1% eV", qFlv = gainModule.Get_qFlv());

        int j = 0;
        double level;

        std::string levelsstr = "Electron energy levels (from the conduction band edge) [eV]: ";
        do
        {
            level = gainModule.Get_electron_level_depth(j);
            if (level > 0) {
                el.push_back(level);
                levelsstr += format("%1%, ", level);
            }
            j++;
        }
        while(level>0);
        writelog(LOG_RESULT, levelsstr.substr(0, levelsstr.length()-2));

        levelsstr = "Heavy hole energy levels (from the valence band edge) [eV]: ";
        j=0;
        do
        {
            level = gainModule.Get_heavy_hole_level_depth(j);
            if (level > 0) {
                hh.push_back(level);
                levelsstr += format("%1%, ", level);
            }
            j++;
        }
        while(level>0);
        writelog(LOG_RESULT, levelsstr.substr(0, levelsstr.length()-2));

        levelsstr = "Light hole energy levels (from the valence band edge) [eV]: ";
        j=0;
        do
        {
            level = gainModule.Get_light_hole_level_depth(j);
            if (level > 0) {
                lh.push_back(level);
                levelsstr += format("%1%, ", level);
            }
            j++;
        }
        while(level>0);
        writelog(LOG_RESULT, levelsstr.substr(0, levelsstr.length()-2));

        result.push_back(std::make_tuple(el, hh, lh, qFlc, qFlv));
    }
    return result;
}

template <typename GeometryType>
const DataVector<double> FermiGainSolver<GeometryType>::getGain(const MeshD<2>& dst_mesh, double wavelength, InterpolationMethod)
{
    this->writelog(LOG_INFO, "Calculating gain");
    this->initCalculation(); // This must be called before any calculation!

    RectilinearMesh2D mesh2;
    if (this->mesh) {
        RectilinearAxis verts;
        for (auto p: dst_mesh) verts.addPoint(p.vert());
        mesh2.axis0 = this->mesh->axis; mesh2.axis1 = verts;
    }
    const MeshD<2>& src_mesh = (this->mesh)? (const MeshD<2>&)mesh2 : dst_mesh;

    WrappedMesh<2> geo_mesh(src_mesh, this->geometry);

    DataVector<const double> nOnMesh = inCarriersConcentration(src_mesh); // carriers concentration on the mesh
    DataVector<const double> TOnMesh = inTemperature(src_mesh); // temperature on the mesh
    DataVector<double> gainOnMesh(src_mesh.size(), 0.);

    if (regions.size() == 1)
        this->writelog(LOG_DETAIL, "Found 1 active region");
    else
        this->writelog(LOG_DETAIL, "Found %1% active regions", regions.size());

    #pragma omp parallel for
    for (int i = 0; i < geo_mesh.size(); i++)
    {
        for (const ActiveRegionInfo& region: regions)
        {
            if (region.contains(geo_mesh[i]) && nOnMesh[i] > 0.)
            {
                QW::gain gainModule = getGainModule(wavelength, TOnMesh[i], nOnMesh[i], region);
                gainOnMesh[i] = gainModule.Get_gain_at_n(nm_to_eV(wavelength), region.qwtotallen);
            }
        }
    }

    if (this->mesh) {
        return interpolate(mesh2, gainOnMesh, dst_mesh, INTERPOLATION_SPLINE);
    } else {
        return gainOnMesh;
    }
}


template <typename GeometryType>
const DataVector<double> FermiGainSolver<GeometryType>::getdGdn(const MeshD<2>& dst_mesh, double wavelength, InterpolationMethod)
{
    this->writelog(LOG_INFO, "Calculating gain over carriers concentration first derivative");
    this->initCalculation(); // This must be called before any calculation!

    RectilinearMesh2D mesh2;
    if (this->mesh) {
        RectilinearAxis verts;
        for (auto p: dst_mesh) verts.addPoint(p.vert());
        mesh2.axis0 = this->mesh->axis; mesh2.axis1 = verts;
    }
    const MeshD<2>& src_mesh = (this->mesh)? (const MeshD<2>&)mesh2 : dst_mesh;

    WrappedMesh<2> geo_mesh(src_mesh, this->geometry);

    DataVector<const double> nOnMesh = inCarriersConcentration(src_mesh); // carriers concentration on the mesh
    DataVector<const double> TOnMesh = inTemperature(src_mesh); // temperature on the mesh
    DataVector<double> dGdn(src_mesh.size(), 0.);

    if (regions.size() == 1)
        this->writelog(LOG_DETAIL, "Found 1 active region");
    else
        this->writelog(LOG_DETAIL, "Found %1% active regions", regions.size());

    #pragma omp parallel for
    for (int i = 0; i < geo_mesh.size(); i++)
    {
        for (const ActiveRegionInfo& region: regions)
        {
            if (region.contains(geo_mesh[i]) && nOnMesh[i] > 0.)
            {
                double gainOnMesh1 = getGainModule(wavelength, TOnMesh[i], nOnMesh[i], region)
                    .Get_gain_at_n(nm_to_eV(wavelength), region.qwtotallen);
                double gainOnMesh2 = getGainModule(wavelength, TOnMesh[i] * (1.+differenceQuotient), nOnMesh[i], region)
                    .Get_gain_at_n(nm_to_eV(wavelength), region.qwtotallen);
                dGdn[i] = (gainOnMesh2 - gainOnMesh1) / (differenceQuotient*nOnMesh[i]);

            }
        }
    }

    if (this->mesh) {
        return interpolate(mesh2, dGdn, dst_mesh, INTERPOLATION_SPLINE);
    } else {
        return dGdn;
    }
}


template <typename GeometryType>
GainSpectrum<GeometryType> FermiGainSolver<GeometryType>::getGainSpectrum(const Vec<2>& point)
{
    this->initCalculation();
    return GainSpectrum<GeometryType>(this, point);
}


template <> std::string FermiGainSolver<Geometry2DCartesian>::getClassName() const { return "gain.Fermi2D"; }
template <> std::string FermiGainSolver<Geometry2DCylindrical>::getClassName() const { return "gain.FermiCyl"; }

template struct FermiGainSolver<Geometry2DCartesian>;
template struct FermiGainSolver<Geometry2DCylindrical>;

}}} // namespace
