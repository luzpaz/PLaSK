#include "reader.h"

#include "../manager.h"

namespace plask {

constexpr const char* const GeometryReader::XML_NAME_ATTR;
constexpr const char* const GeometryReader::XML_MATERIAL_ATTR;
constexpr const char* const GeometryReader::XML_MATERIAL_TOP_ATTR;
constexpr const char* const GeometryReader::XML_MATERIAL_BOTTOM_ATTR;


shared_ptr<Material> GeometryReader::getMaterial(const std::string& material_full_name) const {
    try {
        return materialsDB->get(material_full_name);
    } catch (NoSuchMaterial&) {
        if (manager.draft) return make_shared<DummyMaterial>(material_full_name);
        else throw;
    } catch (MaterialParseException&) {
        if (manager.draft) return make_shared<DummyMaterial>(material_full_name);
        else throw;
    }
}

std::map<std::string, GeometryReader::object_read_f*>& GeometryReader::objectReaders() {
    static std::map<std::string, GeometryReader::object_read_f*> result;
    return result;
}

void GeometryReader::registerObjectReader(const std::string &tag_name, object_read_f *reader) {
    objectReaders()[tag_name] = reader;
}

const AxisNames &GeometryReader::getAxisNames() const {
    return *manager.axisNames;
}

std::string GeometryReader::getAxisName(std::size_t axis_index) {
     return manager.getAxisName(axis_index);
}


GeometryReader::SetExpectedSuffix::SetExpectedSuffix(GeometryReader &reader, const char* new_expected_suffix)
    : reader(reader), old(reader.expectedSuffix) {
    reader.expectedSuffix = new_expected_suffix;
}

GeometryReader::SetExpectedSuffix::SetExpectedSuffix(GeometryReader &reader, int dim)
    : reader(reader), old(reader.expectedSuffix)
{
    if (dim == 2) reader.expectedSuffix = PLASK_GEOMETRY_TYPE_NAME_SUFFIX_2D; else
    if (dim == 3) reader.expectedSuffix = PLASK_GEOMETRY_TYPE_NAME_SUFFIX_3D; else
    reader.expectedSuffix = "";
}

GeometryReader::GeometryReader(plask::Manager &manager, plask::XMLReader &source, const MaterialsDB& materialsDB)
    : materialsAreRequired(!manager.draft), expectedSuffix(0), manager(manager), source(source),
      materialsDB(&materialsDB)
{
}

inline bool isAutoName(const std::string& name) { return !name.empty() && name[0] == '#'; }


#define XML_MAX_POINTS_ATTR "steps-num"
#define XML_MIN_PLY_ATTR "steps-dist"

shared_ptr<GeometryObject> GeometryReader::readObject() {
    std::string nodeName = source.getNodeName();

    if (nodeName == "again") {
        shared_ptr<GeometryObject> result = requireObjectWithName(source.requireAttribute("ref"));
        source.requireTagEnd();
        return result;
    }

    boost::optional<std::string> name = source.getAttribute(XML_NAME_ATTR);    // read name
    if (name && !isAutoName(*name))
        BadId::throwIfBad("geometry object", *name, '-');

    boost::optional<std::string> roles = source.getAttribute("role");    // read roles (tags)

    auto max_points = source.getAttribute<unsigned long>(XML_MAX_POINTS_ATTR);
    auto min_ply = source.getAttribute<double>(XML_MIN_PLY_ATTR);

    shared_ptr<GeometryObject> new_object;    // new object will be constructed

    if (nodeName == "copy") {
        shared_ptr<GeometryObject> from = requireObjectWithName(source.requireAttribute("from"));
        GeometryObject::CompositeChanger changers;
        while (source.requireTagOrEnd()) {
            const std::string operation_name = source.getNodeName();
            if (operation_name == "replace") {
                shared_ptr<GeometryObject> op_from = requireObjectWithName(source.requireAttribute("object"));
                shared_ptr<GeometryObject> to;
                boost::optional<std::string> to_name = source.getAttribute("with");
                if (to_name) {
                    to = requireObjectWithName(*to_name);
                } else {
                    source.requireTag();
                    SetExpectedSuffix suffixSetter(*this, op_from->getDimensionsCount());
                    to = readObject();
                }
                //TODO read translation
                changers.append(new GeometryObject::ReplaceChanger(op_from, to, vec(0.0, 0.0, 0.0)));
                source.requireTagEnd();
            } else if (operation_name == "toblock") {
                shared_ptr<GeometryObject> op_from = requireObjectWithName(source.requireAttribute("object"));
                shared_ptr<Material> blockMaterial = getMaterial(source.requireAttribute("material"));
                GeometryObject::ToBlockChanger* changer = new GeometryObject::ToBlockChanger(op_from, blockMaterial);
                changers.append(changer);
                //TODO read and process name and path
                if (boost::optional<std::string> block_roles = source.getAttribute("role")) {  // if have some roles
                    for (const std::string& c: splitEscIterator(*block_roles, ',')) changer->to->addRole(c);
                }
                source.requireTagEnd();
            } else {
                throw Exception("\"%1%\" is not proper name of copy operation and so it is not allowed in <copy> tag.", operation_name);
            }
        }
        new_object = const_pointer_cast<GeometryObject>(from->changedVersion(changers));
    } else {
        Manager::SetAxisNames axis_reader(*this);   //try set up new axis names, store old, and restore old on end of block
        auto reader_it = objectReaders().find(nodeName);
        if (reader_it == objectReaders().end()) {
            if (expectedSuffix == 0)
                throw NoSuchGeometryObjectType(nodeName);
            reader_it = objectReaders().find(nodeName + expectedSuffix);
            if (reader_it == objectReaders().end())
                throw NoSuchGeometryObjectType(nodeName + "[" + expectedSuffix + "]");
        }
        new_object = reader_it->second(*this); //and rest (but while reading this subtree, name is not registred yet)
    }

    registerObjectName(name, new_object);

    if (roles) {  // if have some roles
        new_object->clearRoles();  // in case of copied object: overwrite
        auto roles_it = splitEscIterator(*roles, ',');
        for (const std::string& c: roles_it) {
            //BadId::throwIfBad("path", path, '-');
            new_object->addRole(c);
        }
    }

    if (new_object->isLeaf()) {
        if (max_points) new_object->max_points = *max_points;
        if (min_ply) new_object->min_ply = *min_ply;
    } else {
        if (max_points) throw XMLUnexpectedAttrException(source, XML_MAX_POINTS_ATTR);
        if (min_ply) throw XMLUnexpectedAttrException(source, XML_MIN_PLY_ATTR);
    }

    return new_object;
}

shared_ptr<GeometryObject> GeometryReader::readExactlyOneChild() {
    source.requireTag();
    shared_ptr<GeometryObject> result = readObject();
    source.requireTagEnd();
    return result;
}

shared_ptr<Geometry> GeometryReader::readGeometry() {
    Manager::SetAxisNames axis_reader(*this);   // try set up new axis names, store old, and restore old on end of block
    std::string nodeName = source.getNodeName();
    boost::optional<std::string> name = source.getAttribute(XML_NAME_ATTR);
    if (name) {
        BadId::throwIfBad("geometry", *name, '-');
        if (manager.geometrics.find(*name) != manager.geometrics.end())
            throw XMLDuplicatedElementException(source, "Geometry '"+*name+"'");
    }

    // TODO read subspaces from XML
    shared_ptr<Geometry> result;

    if (nodeName == "cartesian2d") {
        SetExpectedSuffix suffixSetter(*this, PLASK_GEOMETRY_TYPE_NAME_SUFFIX_2D);
        boost::optional<double> l = source.getAttribute<double>("length");
        shared_ptr<Geometry2DCartesian> cartesian2d = make_shared<Geometry2DCartesian>();   // result with original type
        result = cartesian2d;
        result->setBorders([&](const std::string& s) -> boost::optional<std::string> {
                              auto val = source.getAttribute(s); return manager.draft? boost::optional<std::string>() : val;
                           }, getAxisNames(), *materialsDB );
        if (l) {
            cartesian2d->setExtrusion(make_shared<Extrusion>(readExactlyOneChild<GeometryObjectD<2>>(), *l));
        } else {
            auto child = readExactlyOneChild<GeometryObject>();
            auto child_as_extrusion = dynamic_pointer_cast<Extrusion>(child);
            if (child_as_extrusion) {
                cartesian2d->setExtrusion(child_as_extrusion);
            } else {
                auto child_as_2d = dynamic_pointer_cast<GeometryObjectD<2>>(child);
                if (!child_as_2d) throw UnexpectedGeometryObjectTypeException();
                cartesian2d->setExtrusion(make_shared<Extrusion>(child_as_2d, INFINITY));
            }
        }

    } else if (nodeName == "cylindrical" || nodeName == "cylindrical2d") {
        SetExpectedSuffix suffixSetter(*this, PLASK_GEOMETRY_TYPE_NAME_SUFFIX_2D);
        result = make_shared<Geometry2DCylindrical>();
        result->setBorders([&](const std::string& s) -> boost::optional<std::string> {
                              auto val = source.getAttribute(s); return manager.draft? boost::optional<std::string>() : val;
                           }, getAxisNames(), *materialsDB );
        auto child = readExactlyOneChild<GeometryObject>();
        auto child_as_revolution = dynamic_pointer_cast<Revolution>(child);
        if (child_as_revolution) {
            static_pointer_cast<Geometry2DCylindrical>(result)->setRevolution(child_as_revolution);
        } else {
            auto child_as_2d = dynamic_pointer_cast<GeometryObjectD<2>>(child);
            if (!child_as_2d) throw UnexpectedGeometryObjectTypeException();
            static_pointer_cast<Geometry2DCylindrical>(result)->setRevolution(make_shared<Revolution>(child_as_2d));
        }

    } else if (nodeName == "cartesian3d") {
        SetExpectedSuffix suffixSetter(*this, PLASK_GEOMETRY_TYPE_NAME_SUFFIX_3D);
        result = make_shared<Geometry3D>();
        result->setBorders([&](const std::string& s) -> boost::optional<std::string> {
                              auto val = source.getAttribute(s); return manager.draft? boost::optional<std::string>() : val;
                           }, getAxisNames(), *materialsDB );
        static_pointer_cast<Geometry3D>(result)->setChildUnsafe(
            readExactlyOneChild<GeometryObjectD<3>>());

    } else
        throw XMLUnexpectedElementException(source, "geometry tag (<cartesian2d>, <cartesian3d>, or <cylindrical>)");

    result->axisNames = getAxisNames();

    if (name) manager.geometrics[*name] = result;
    return result;
}

shared_ptr<GeometryObject> GeometryReader::requireObjectWithName(const std::string &name) const {
    if (isAutoName(name)) {
        auto it = autoNamedObjects.find(name);
        if (it == autoNamedObjects.end()) throw NoSuchGeometryObject(name);
        return it->second;
    } else
        return manager.requireGeometryObject(name);
}

void GeometryReader::registerObjectName(const std::string &name, shared_ptr<GeometryObject> object) {
    if (isAutoName(name)) {
        if (!autoNamedObjects.insert(std::map<std::string, shared_ptr<GeometryObject> >::value_type(name, object)).second)
            throw NamesConflictException("Auto-named geometry object", name);
    } else {    //normal name
        if (!manager.geometrics.insert(std::map<std::string, shared_ptr<GeometryObject> >::value_type(name, object)).second)
            throw NamesConflictException("Geometry object", name);
    }
}

}   // namespace plask
