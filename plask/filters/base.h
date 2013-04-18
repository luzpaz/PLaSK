#ifndef PLASK__FILTER__BASE_H
#define PLASK__FILTER__BASE_H

#include <vector>
#include <memory>

#include "../solver.h"
#include "../provider/providerfor.h"

namespace plask {

/// Don't use this directly, use DataSource instead.
template <typename PropertyT, PropertyType propertyType, typename OutputSpaceType, typename VariadicTemplateTypesHolder>
struct DataSourceImpl {
    static_assert(propertyType != SINGLE_VALUE_PROPERTY, "space change filter data sources can't be use with single value properties (it can be use only with fields properties)");
};

template <typename PropertyT, typename OutputSpaceType, typename... ExtraArgs>
class DataSourceImpl<PropertyT, FIELD_PROPERTY, OutputSpaceType, VariadicTemplateTypesHolder<ExtraArgs...>>: public ProviderFor<PropertyT, OutputSpaceType> {

    //shared_ptr<OutputSpaceType> destinationSpace;   //should be stored here? maybe only connection...

public:

    /*shared_ptr<OutputSpaceType> getDestinationSpace() const { return destinationSpace; }

    virtual void setDestinationSpace(shared_ptr<OutputSpaceType>) { this->destinationSpace = destinationSpace; }*/

    /*
     * Check if this source can provide value for given point.
     * @param p point, in outer space coordinates
     * @return @c true only if this can provide data in given point @p p
     */
    //virtual bool canProvide(const Vec<OutputSpaceType::DIMS, double>& p) const = 0;

    /**
     * Check if this source can provide value for given point and eventualy return this value.
     * @param p point (in outer space coordinates)
     * @return value in point @p, set only if this can provide data in given point @p p
     */
    virtual boost::optional<typename PropertyT::ValueType> get(const Vec<OutputSpaceType::DIMS, double>& p, ExtraArgs... extra_args, InterpolationMethod method) const = 0;

   // virtual ValueT get(const Vec<OutputSpaceType::DIMS, double>& p, ExtraArgs... extra_args, InterpolationMethod method) const = 0;

    virtual DataVector<const typename PropertyT::ValueType> operator()(const MeshD<OutputSpaceType::DIMS>& dst_mesh, ExtraArgs... extra_args, InterpolationMethod method) const {
        DataVector<typename PropertyT::ValueType> result(dst_mesh.size());
        for (std::size_t i = 0; i < result.size(); ++i)
            result[i] = *get(dst_mesh[i], std::forward<ExtraArgs>(extra_args)..., method);
        return result;
    }

};

template <typename PropertyT, typename OutputSpaceType>
using DataSource = DataSourceImpl<PropertyT, PropertyT::propertyType, OutputSpaceType, typename PropertyT::ExtraParams>;

template <typename PropertyT, typename OutputSpaceType, typename InputSpaceType = OutputSpaceType, typename OutputGeomObj = OutputSpaceType, typename InputGeomObj = InputSpaceType>
struct DataSourceWithReceiver: public DataSource<PropertyT, OutputSpaceType> {

protected:
    //in, out obj can't be hold by shared_ptr, due to memory leak (circle reference)
    const InputGeomObj* inObj;
    const OutputGeomObj* outObj;
    boost::optional<PathHints> path;
    boost::signals2::connection geomConnectionIn;
    boost::signals2::connection geomConnectionOut;

public:
    ReceiverFor<PropertyT, InputSpaceType> in;

    DataSourceWithReceiver() {
        in.providerValueChanged.connect([&] (typename ReceiverFor<PropertyT, InputSpaceType>::Base&) { this->fireChanged(); });
    }

    ~DataSourceWithReceiver() {
         disconnect();
    }

    void disconnect() {
        geomConnectionIn.disconnect();
        geomConnectionOut.disconnect();
    }

    /**
     * This is called before request for data, but after setup inObj, outObj and path fields.
     * It can calculate trasnaltion and so needed for quick operator() calculation.
     */
    virtual void calcConnectionParameters() = 0;

    void setPath(const PathHints* path) {
        if (path)
            this->path = *path;
        else
            this->path = boost::optional<PathHints>();
    }

    const PathHints* getPath() const {
        return path ? &*path : nullptr;
    }

    void inOrOutWasChanged(GeometryObject::Event& e) {
        if (e.hasFlag(GeometryObject::Event::DELETE)) disconnect(); else
        if (e.hasFlag(GeometryObject::Event::RESIZE)) calcConnectionParameters();
    }

    void connect(InputGeomObj& inObj, OutputGeomObj& outObj, const PathHints* path = nullptr) {
        disconnect();
        this->setPath(path);
        this->inObj = &inObj;
        this->outObj = &outObj;
        geomConnectionOut = outObj.changedConnectMethod(this, &DataSourceWithReceiver::inOrOutWasChanged);
        geomConnectionIn = inObj.changedConnectMethod(this, &DataSourceWithReceiver::inOrOutWasChanged);
        calcConnectionParameters();
    }
};

template <typename PropertyT, typename OutputSpaceType, typename InputSpaceType = OutputSpaceType, typename OutputGeomObj = OutputSpaceType, typename InputGeomObj = InputSpaceType>
struct InnerDataSource: public DataSourceWithReceiver<PropertyT, OutputSpaceType, InputSpaceType, OutputGeomObj, InputGeomObj> {

    typedef typename Primitive<OutputSpaceType::DIMS>::Box OutBox;
    typedef Vec<OutputSpaceType::DIMS, double> OutVec;

    struct Region {

        /// Input bouding-box in output geometry.
        OutBox inGeomBB;

        /// Translation to input object (before eventual space reduction).
        OutVec inTranslation;

        Region(const OutBox& inGeomBB, const OutVec& inTranslation)
            : inGeomBB(inGeomBB), inTranslation(inTranslation) {}

    };

    std::vector<Region> regions;

    const Region* findRegion(const OutVec& p) const {
        for (const Region& r: regions)
            if (r.inGeomBB.includes(p)) return &r;
        return nullptr;
    }

    virtual void calcConnectionParameters() {
        regions.clear();
        std::vector<OutVec> pos = this->outObj->getObjectPositions(*this->inObj, this->getPath());
        std::vector<OutBox> bb = this->outObj->getObjectBoundingBoxes(*this->inObj, this->getPath());
        for (std::size_t i = 0; i < pos.size(); ++i)
            regions.emplace_back(bb[i], pos[i]);
    }

};

template <typename PropertyT, typename OutputSpaceType, typename InputSpaceType = OutputSpaceType, typename OutputGeomObj = OutputSpaceType, typename InputGeomObj = InputSpaceType>
struct OuterDataSource: public DataSourceWithReceiver<PropertyT, OutputSpaceType, InputSpaceType, OutputGeomObj, InputGeomObj> {

    typename InputSpaceType::DVec inTranslation;

    virtual void calcConnectionParameters() {
        std::vector<typename OutputSpaceType::DVec> pos = this->outObj->getObjectPositions(*this->inObj, this->getPath());
        if (pos.size() != 1) throw Exception("Inner output geometry object has no unambiguous position in outer input geometry object.");
        inTranslation = pos[0];
    }

};

/// Don't use this directly, use ConstDataSource instead.
template <typename PropertyT, PropertyType propertyType, typename OutputSpaceType, typename VariadicTemplateTypesHolder>
struct ConstDataSourceImpl {
    static_assert(propertyType != SINGLE_VALUE_PROPERTY, "filter data sources can't be use with single value properties (it can be use only with fields properties)");
};

template <typename PropertyT, typename OutputSpaceType, typename... ExtraArgs>
struct ConstDataSourceImpl<PropertyT, FIELD_PROPERTY, OutputSpaceType, VariadicTemplateTypesHolder<ExtraArgs...>>
        : public DataSource<PropertyT, OutputSpaceType> {

public:

    typename PropertyT::ValueType value;

    ConstDataSourceImpl(const typename PropertyT::ValueType& value): value(value) {}

    virtual boost::optional<typename PropertyT::ValueType> get(const Vec<OutputSpaceType::DIMS, double>& p, ExtraArgs... extra_args, InterpolationMethod method) const override {
        return value;
    }

    virtual DataVector<const typename PropertyT::ValueType> operator()(const MeshD<OutputSpaceType::DIMS>& dst_mesh, ExtraArgs... extra_args, InterpolationMethod method) const override {
        return DataVector<typename PropertyT::ValueType>(dst_mesh.size(), value);
    }

};

template <typename PropertyT, typename OutputSpaceType>
using ConstDataSource = ConstDataSourceImpl<PropertyT, PropertyT::propertyType, OutputSpaceType, typename PropertyT::ExtraParams>;

}   // plask

#endif // PLASK__FILTER__BASE_H
