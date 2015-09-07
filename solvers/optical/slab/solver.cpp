#include "solver.h"
#include "expansion.h"
#include "meshadapter.h"
#include "muller.h"
#include "broyden.h"
#include "brent.h"
#include "reflection.h"
#include "admittance.h"

namespace plask { namespace solvers { namespace slab {

void SlabBase::initTransfer(Expansion& expansion, bool emitting) {
    switch (transfer_method) {
        case Transfer::METHOD_REFLECTION:
            emitting = true; break;
        case Transfer::METHOD_ADMITTANCE:
            emitting = false; break;
        default:
            break;
    }
    if (emitting) {
        if (!this->transfer || !dynamic_cast<ReflectionTransfer*>(this->transfer.get()) ||
            this->transfer->diagonalizer->source() != &expansion)
        this->transfer.reset(new ReflectionTransfer(this, expansion));
    } else {
        if (!this->transfer || !dynamic_cast<AdmittanceTransfer*>(this->transfer.get()) ||
            this->transfer->diagonalizer->source() != &expansion)
        this->transfer.reset(new AdmittanceTransfer(this, expansion));
    }
}


template <typename BaseT>
SlabSolver<BaseT>::SlabSolver(const std::string& name): BaseT(name),
    outdist(0.1),
    smooth(0.),
    outRefractiveIndex(this, &SlabSolver<BaseT>::getRefractiveIndexProfile),
    outLightMagnitude(this, &SlabSolver<BaseT>::getMagnitude, &SlabSolver<BaseT>::nummodes),
    outElectricField(this, &SlabSolver<BaseT>::getE, &SlabSolver<BaseT>::nummodes),
    outMagneticField(this, &SlabSolver<BaseT>::getH, &SlabSolver<BaseT>::nummodes)
{
    this->inTemperature.changedConnectMethod(this, &SlabSolver<BaseT>::onInputChanged);
    this->inGain.changedConnectMethod(this, &SlabSolver<BaseT>::onInputChanged);
    inTemperature = 300.; // temperature receiver has some sensible value
}

template <typename BaseT>
SlabSolver<BaseT>::~SlabSolver()
{
    this->inTemperature.changedDisconnectMethod(this, &SlabSolver<BaseT>::onInputChanged);
    this->inGain.changedDisconnectMethod(this, &SlabSolver<BaseT>::onInputChanged);
}


std::unique_ptr<RootDigger> SlabBase::getRootDigger(const RootDigger::function_type& func) {
    typedef std::unique_ptr<RootDigger> Res;
    if (root.method == RootDigger::ROOT_MULLER) return Res(new RootMuller(*this, func, detlog, root));
    else if (root.method == RootDigger::ROOT_BROYDEN) return Res(new RootBroyden(*this, func, detlog, root));
    else if (root.method == RootDigger::ROOT_BRENT) return Res(new RootBrent(*this, func, detlog, root));
    throw BadInput(getId(), "Wrong root finding method");
    return Res();
}


template <typename BaseT>
void SlabSolver<BaseT>::setup_vbounds()
{
    if (!this->geometry) throw NoGeometryException(this->getId());
    vbounds = *RectilinearMesh2DSimpleGenerator().get<RectangularMesh<2>>(this->geometry->getChild())->vert();
    //TODO consider geometry objects non-uniform in vertical direction (step approximation)
    if (this->geometry->isSymmetric(Geometry::DIRECTION_VERT)) {
        std::deque<double> zz;
        for (double z: vbounds) zz.push_front(-z);
        vbounds.addOrderedPoints(zz.begin(), zz.end(), zz.size());
    }
}

template <>
void SlabSolver<SolverOver<Geometry3D>>::setup_vbounds()
{
    if (!this->geometry) throw NoGeometryException(this->getId());
    vbounds = *RectilinearMesh3DSimpleGenerator().get<RectangularMesh<3>>(this->geometry->getChild())->vert();
    //TODO consider geometry objects non-uniform in vertical direction (step approximation)
    if (this->geometry->isSymmetric(Geometry::DIRECTION_VERT)) {
        std::deque<double> zz;
        for (double z: vbounds) zz.push_front(-z);
        vbounds.addOrderedPoints(zz.begin(), zz.end(), zz.size());
    }
}

template <typename BaseT>
void SlabSolver<BaseT>::setupLayers()
{
    if (!this->geometry) throw NoGeometryException(this->getId());

    if (vbounds.empty()) setup_vbounds();

    auto points = make_rectilinear_mesh(RectilinearMesh2DSimpleGenerator().get<RectangularMesh<2>>(this->geometry->getChild())->getMidpointsMesh());

    struct LayerItem {
        shared_ptr<Material> material;
        std::set<std::string> roles;
        bool operator==(const LayerItem& other) { return *this->material == *other.material && this->roles == other.roles; }
        bool operator!=(const LayerItem& other) { return !(*this == other); }
    };

    std::vector<std::vector<LayerItem>> layers;

    // Add layers below bottom boundary and above top one
    static_pointer_cast<OrderedAxis>(points->axis1)->addPoint(vbounds[0] - outdist);
    static_pointer_cast<OrderedAxis>(points->axis1)->addPoint(vbounds[vbounds.size()-1] + outdist);

    lverts.clear();
    lgained.clear();
    stack.clear();
    stack.reserve(points->axis1->size());

    for (auto v: *points->axis1) {
        bool gain = false;

        std::vector<LayerItem> layer(points->axis0->size());
        for (size_t i = 0; i != points->axis0->size(); ++i) {
            Vec<2> p(points->axis0->at(i), v);
            layer[i].material = this->geometry->getMaterial(p);
            for (const std::string& role: this->geometry->getRolesAt(p)) {
                if (role.substr(0,3) == "opt") layer[i].roles.insert(role);
                else if (role == "QW" || role == "QD" || role == "gain") { layer[i].roles.insert(role); gain = true; }
            }
        }

        bool unique = true;
        if (group_layers) {
            for (size_t i = 0; i != layers.size(); ++i) {
                unique = false;
                for (size_t j = 0; j != layers[i].size(); ++j) {
                    if (layers[i][j] != layer[j]) {
                        unique = true;
                        break;
                    }
                }
                if (!unique) {
                    lverts[i]->addPoint(v);
                    stack.push_back(i);
                    break;
                }
            }
        }
        if (unique) {
            layers.emplace_back(std::move(layer));
            stack.push_back(lverts.size());
            lverts.emplace_back(make_shared<OrderedAxis,std::initializer_list<double>>({v}));
            lgained.push_back(gain);
        }
    }

    Solver::writelog(LOG_DETAIL, "Detected %1% %2%layers", lverts.size(), group_layers? "distinct " : "");
}

template <>
void SlabSolver<SolverOver<Geometry3D>>::setupLayers()
{
    if (vbounds.empty()) setup_vbounds();

    auto points = make_rectilinear_mesh(RectilinearMesh3DSimpleGenerator().get<RectangularMesh<3>>(this->geometry->getChild())->getMidpointsMesh());

    struct LayerItem {
        shared_ptr<Material> material;
        std::set<std::string> roles;
        bool operator!=(const LayerItem& other) { return *material != *other.material || roles != other.roles; }
    };

    std::vector<std::vector<LayerItem>> layers;

    // Add layers below bottom boundary and above top one
    //static_pointer_cast<OrderedAxis>(points->vert())->addPoint(vbounds[0] - outdist);
    static_pointer_cast<OrderedAxis>(points->vert())->addPoint(vbounds[0] - outdist);
    static_pointer_cast<OrderedAxis>(points->vert())->addPoint(vbounds[vbounds.size()-1] + outdist);

    lverts.clear();
    lgained.clear();
    stack.clear();
    stack.reserve(points->vert()->size());

    for (auto v: *points->vert()) {
        bool gain = false;

        std::vector<LayerItem> layer(points->axis0->size() * points->axis1->size());
        for (size_t i = 0; i != points->axis1->size(); ++i) {
            size_t offs = i * points->axis0->size();
            for (size_t j = 0; j != points->axis0->size(); ++j) {
                Vec<3> p(points->axis0->at(j), points->axis1->at(i), v);
                size_t n = offs + j;
                layer[n].material = this->geometry->getMaterial(p);
                for (const std::string& role: this->geometry->getRolesAt(p)) {
                    if (role.substr(0,3) == "opt") layer[n].roles.insert(role);
                    else if (role == "QW" || role == "QD" || role == "gain") { layer[n].roles.insert(role); gain = true; }
                }
            }
        }

        bool unique = true;
        if (group_layers) {
            for (size_t i = 0; i != layers.size(); ++i) {
                unique = false;
                for (size_t j = 0; j != layers[i].size(); ++j) {
                    if (layers[i][j] != layer[j]) {
                        unique = true;
                        break;
                    }
                }
                if (!unique) {
                    lverts[i]->addPoint(v);
                    stack.push_back(i);
                    break;
                }
            }
        }
        if (unique) {
            layers.emplace_back(std::move(layer));
            stack.push_back(lverts.size());
            lverts.emplace_back(make_shared<OrderedAxis,std::initializer_list<double>>({v}));
            lgained.push_back(gain);
        }
    }

    assert(vbounds.size() == stack.size()-1);

    Solver::writelog(LOG_DETAIL, "Detected %1% %2%layers", lverts.size(), group_layers? "distinct " : "");
}


template <typename BaseT>
DataVector<const Tensor3<dcomplex>> SlabSolver<BaseT>::getRefractiveIndexProfile
                                        (const shared_ptr<const MeshD<BaseT::SpaceType::DIM>>& dst_mesh,
                                        InterpolationMethod interp)
{
    this->initCalculation();
    initTransfer(getExpansion(), false);
    if (recompute_integrals) {
        computeIntegrals();
        recompute_integrals = false;
    }

    //TODO maybe there is a more efficient way to implement this
    DataVector<Tensor3<dcomplex>> result(dst_mesh->size());
    auto levels = makeLevelsAdapter(dst_mesh);

    //std::map<size_t,LazyData<Tensor3<dcomplex>>> cache;
    //while (auto level = levels->yield()) {
    //    double h = level->vpos();
    //    size_t n = getLayerFor(h);
    //    size_t l = stack[n];
    //    LazyData<Tensor3<dcomplex>> data = cache.find(l);
    //    if (data == cache.end()) {
    //        data = transfer->diagonalizer->source()->getMaterialNR(l, level, interp);
    //        cache[l] = data;
    //    }
    //    for (size_t i = 0; i != level->size(); ++i) result[level->index(i)] = data[i];
    //}
    while (auto level = levels->yield()) {
        double h = level->vpos();
        size_t n = getLayerFor(h);
        size_t l = stack[n];
        auto data = transfer->diagonalizer->source()->getMaterialNR(l, level, interp);
        for (size_t i = 0; i != level->size(); ++i) result[level->index(i)] = data[i];
    }

    return result;
}

#ifndef NDEBUG
template <typename BaseT>
void SlabSolver<BaseT>::getMatrices(size_t layer, cmatrix& RE, cmatrix& RH) {
    this->initCalculation();
    if (recompute_integrals) {
        computeIntegrals();
        recompute_integrals = false;
    }
    size_t N = this->getExpansion().matrixSize();
    if (RE.cols() != N || RE.rows() != N) RE = cmatrix(N, N);
    if (RH.cols() != N || RH.rows() != N) RH = cmatrix(N, N);
    this->getExpansion().getMatrices(layer, RE, RH);
}
#endif

template class PLASK_SOLVER_API SlabSolver<SolverOver<Geometry2DCartesian>>;
template class PLASK_SOLVER_API SlabSolver<SolverWithMesh<Geometry2DCylindrical, OrderedAxis>>;
template class PLASK_SOLVER_API SlabSolver<SolverOver<Geometry3D>>;


// FiltersFactory::RegisterStandard<RefractiveIndex> registerRefractiveIndexFilters;

}}} // namespace
