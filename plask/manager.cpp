#include <fstream>

#include "manager.h"

#include "utils/stl.h"
#include "geometry/reader.h"
#include "filters/factory.h"

#include "utils/dynlib/manager.h"

#include "utils/system.h"

namespace plask {

void Manager::ExternalSourcesFromFile::operator()(Manager &manager, MaterialsDB& materialsDB, const std::string &url, const std::string &section) {
    boost::filesystem::path url_path(url);
    if (url_path.is_relative()) {
        if (originalFileName.empty())
            throw Exception("Error while reading section \"{0}\": relative path name \"{1}\" is not supported.", section, url);
        url_path = originalFileName;
        url_path.remove_filename();
        url_path /= url;
    }
    if (hasCircularRef(url_path, section))
        throw Exception("Error while reading section \"{0}\": circular reference was detected.", section);
    XMLReader reader(url_path.string().c_str());
    manager.loadSection(reader, section, materialsDB, ExternalSourcesFromFile(url_path, section, this));
}

Manager::SetAxisNames::SetAxisNames(Manager &manager, const AxisNames* names)
    : manager(manager), old(manager.axisNames) {
    manager.axisNames = names;
}

Manager::SetAxisNames::SetAxisNames(Manager &manager, XMLReader& source)
    : manager(manager), old(manager.axisNames) {
    boost::optional<std::string> axis = source.getAttribute(XML_AXES_ATTR);
    if (axis) manager.axisNames = &AxisNames::axisNamesRegister.get(*axis);
}

bool Manager::tryLoadFromExternal(XMLReader& reader, MaterialsDB& materialsDB, const Manager::LoadFunCallbackT& load_from) {
    boost::optional<std::string> from_attr = reader.getAttribute("external");
    if (!from_attr) return false;
    load_from(*this, materialsDB, *from_attr, reader.getNodeName());
    return true;

    /*std::string section_to_load = reader.getNodeName();
    std::pair< XMLReader, std::unique_ptr<LoadFunCallbackT> > new_loader = load_from.get(*from_attr);
    load(new_loader.first, materialsDB, *new_loader.second, [&](const std::string& section_name) -> bool { return section_name == section_to_load; });
    return true;*/
}

PathHints *Manager::getPathHints(const std::string &path_hints_name) {
    auto result_it = pathHints.find(path_hints_name);
    if (result_it == pathHints.end()) return nullptr;
    return &result_it->second;
}

const PathHints *Manager::getPathHints(const std::string &path_hints_name) const {
    auto result_it = pathHints.find(path_hints_name);
    if (result_it == pathHints.end()) return nullptr;
    return &result_it->second;
}

PathHints& Manager::requirePathHints(const std::string& path_hints_name) {
    auto result_it = pathHints.find(path_hints_name);
    if (result_it == pathHints.end()) throw NoSuchPath(path_hints_name);
    return result_it->second;
}

const PathHints& Manager::requirePathHints(const std::string& path_hints_name) const {
    auto result_it = pathHints.find(path_hints_name);
    if (result_it == pathHints.end()) throw NoSuchPath(path_hints_name);
    return result_it->second;
}

shared_ptr<GeometryObject> Manager::getGeometryObject(const std::string &name) const {
    auto result_it = geometrics.find(name);
    if (result_it == geometrics.end()) return shared_ptr<GeometryObject>();
    // auto result = result_it->second.lock();
    // if (!result) const_cast<Manager*>(this)->geometrics.erase(name);
    // return result;
    return result_it->second;
    /*auto result_it = geometrics.find(name);
    return result_it != geometrics.end() ? result_it->second.lock() : shared_ptr<GeometryObject>();*/
}

shared_ptr<GeometryObject> Manager::requireGeometryObject(const std::string &name) const {
    shared_ptr<GeometryObject> result = getGeometryObject(name);
    if (!result) throw NoSuchGeometryObject(name);
    return result;
}

shared_ptr<Geometry> Manager::getGeometry(const std::string& name) const {
    auto result_it = geometrics.find(name);
    return result_it == geometrics.end() ? shared_ptr<Geometry>() : dynamic_pointer_cast<Geometry>(result_it->second);
}

shared_ptr<Mesh> Manager::getMesh(const std::string& name) const {
    auto result_it = meshes.find(name);
    return result_it == meshes.end() ? shared_ptr<Mesh>() : result_it->second;
}

void Manager::loadDefines(XMLReader &reader) {
    writelog(LOG_ERROR, "Loading defines from C++ not implemented. Ignoring XML section <defines>.");
    reader.gotoEndOfCurrentTag();
}

void Manager::loadFromReader(XMLReader &reader, MaterialsDB& materialsDB, const LoadFunCallbackT& load_from_cb) {
    load(reader, materialsDB, load_from_cb);
}

void Manager::loadFromStream(std::unique_ptr<std::istream>&& input, MaterialsDB& materialsDB, const LoadFunCallbackT& load_from_cb) {
    XMLReader reader(std::move(input));
    loadFromReader(reader, materialsDB, load_from_cb);
}

void Manager::loadFromXMLString(const std::string &input_XML_str, MaterialsDB& materialsDB, const LoadFunCallbackT& load_from_cb) {
    loadFromStream(std::unique_ptr<std::istream>(new std::istringstream(input_XML_str)), materialsDB, load_from_cb);
}

void Manager::loadFromFile(const std::string &fileName, MaterialsDB& materialsDB) {
    XMLReader reader(fileName.c_str());
    loadFromReader(reader, materialsDB, ExternalSourcesFromFile(fileName));
}

void Manager::loadFromFILE(FILE* file, MaterialsDB& materialsDB, const LoadFunCallbackT& load_from_cb) {
    XMLReader reader(file);
    loadFromReader(reader, materialsDB, load_from_cb);
}


void Manager::loadGeometry(GeometryReader& greader) {
    if (greader.source.getNodeType() != XMLReader::NODE_ELEMENT || greader.source.getNodeName() != std::string("geometry"))
        throw XMLUnexpectedElementException(greader.source, "<geometry>");
    SetAxisNames read_axis_tag(greader);
    while(greader.source.requireTagOrEnd())
        roots.push_back(greader.readGeometry());
}


void Manager::loadMaterialLib(XMLReader& reader, MaterialsDB& materialsDB) {
    std::string name = reader.requireAttribute("name");
    try {
        if (name != "") materialsDB.loadToDefault(name);
    } catch (Exception& err) {
        throw XMLException(reader, err.what());
    }
    reader.requireTagEnd();
}


void Manager::loadMaterial(XMLReader& reader, MaterialsDB& materialsDB)
{
    writelog(LOG_ERROR, "Loading XML material from C++ not implemented (ignoring material {})", reader.getAttribute<std::string>("name", "unknown"));
    reader.gotoEndOfCurrentTag();
}

void Manager::loadMaterials(XMLReader& reader, MaterialsDB& materialsDB)
{
    while (reader.requireTagOrEnd()) {
        if (reader.getNodeName() == "material")
            loadMaterial(reader, materialsDB);
        else if (reader.getNodeName() == "library")
            loadMaterialLib(reader, materialsDB);
        else
            throw XMLUnexpectedElementException(reader, "<material>");
    }
}

void Manager::loadGrids(XMLReader &reader)
{
    if (reader.getNodeType() != XMLReader::NODE_ELEMENT || reader.getNodeName() != std::string("grids"))
        throw XMLUnexpectedElementException(reader, "<grids>");
    while(reader.requireTagOrEnd()) {
        if (reader.getNodeName() == "mesh") {
            std::string type = reader.requireAttribute("type");
            std::string name = reader.requireAttribute("name");
            BadId::throwIfBad("mesh", name, '-');
            if (meshes.find(name) != meshes.end() || generators.find(name) != generators.end())
                throw NamesConflictException("Mesh or mesh generator", name);
            shared_ptr<Mesh> mesh = RegisterMeshReader::getReader(type)(reader);
            if (reader.getNodeType() != XMLReader::NODE_ELEMENT_END || reader.getNodeName() != "mesh")
                throw Exception("Internal error in {0} mesh reader, after return reader not point to end of mesh tag.", type);
            meshes[name] = mesh;
        } else if (reader.getNodeName() == "generator") {
            std::string type = reader.requireAttribute("type");
            std::string method = reader.requireAttribute("method");
            std::string name = reader.requireAttribute("name");
            BadId::throwIfBad("generator", name, '-');
            std::string key = type + "." + method;
            if (meshes.find(name) != meshes.end() || generators.find(name) != generators.end())
                throw NamesConflictException("Mesh or mesh generator", name);
            shared_ptr<MeshGenerator> generator = RegisterMeshGeneratorReader::getReader(key)(reader, *this);
            if (reader.getNodeType() != XMLReader::NODE_ELEMENT_END || reader.getNodeName() != "generator")
                throw Exception("Internal error in {0} (method: {1}) mesh generator reader, after return reader not point to end of generator tag.", type, method);
            generators[name] = generator;
        } else
            throw XMLUnexpectedElementException(reader, "<mesh...>, <generator...>, or </grids>");
    }
}

shared_ptr<Solver> Manager::loadSolver(const std::string &, const std::string &, const std::string &, const std::string& name) {
    auto found = solvers.find(name);
    if (found == solvers.end())
        throw Exception("In C++ solvers ('{0}' in this case) must be created and added to Manager::solvers manually before reading XML.", name);
    solvers.erase(found); // this is necessary so we don't have name conflicts — the solver will be added back to the map by loadSolvers
    return found->second;
}

void Manager::loadSolvers(XMLReader& reader) {
    if (reader.getNodeType() != XMLReader::NODE_ELEMENT || reader.getNodeName() != std::string("solvers"))
        throw XMLUnexpectedElementException(reader, "<solvers>");
    while (reader.requireTagOrEnd()) {
        const std::string name = reader.requireAttribute("name");
        BadId::throwIfBad("solver", name);
        if (shared_ptr<Solver> filter = FiltersFactory::getDefault().get(reader, *this)) {
            if (!this->solvers.insert(std::make_pair(name, filter)).second)
                throw NamesConflictException("Solver", name);
            continue;
        }
        boost::optional<std::string> lib = reader.getAttribute("lib");
        const std::string solver_name = reader.requireAttribute("solver");
        if (!lib) {
            auto libs = global_solver_names[reader.getNodeName()];
            if (libs.empty()) { // read lib index from file
                boost::filesystem::directory_iterator iter(plaskSolversPath(reader.getNodeName()));
                boost::filesystem::directory_iterator end;
                while (iter != end) {
                    boost::filesystem::path p = iter->path();
                    if (boost::filesystem::is_regular_file(p) && p.extension().string() == ".xml") {
                        XMLReader xml(p.string().c_str());
                        xml.requireTag();
                        while(xml.requireTagOrEnd()) {
                            std::string tag = xml.getTagName();
                            if (tag != "solver" && tag.substr(max(tag.length(),size_t(7))-7) != " solver")
                                continue;
                            auto cls = xml.requireAttribute("name");
                            auto lib = xml.getAttribute("lib");
                            if (!lib) lib.reset(); //TODO should we do this?
                            libs[cls] = *lib;
                            xml.ignoreAllAttributes();
                            xml.gotoEndOfCurrentTag();
                        }
                    }
                    ++iter;
                }
            }
            lib.reset(libs[solver_name]);
        }
        if (lib->empty())
            throw XMLException(reader, format("Cannot determine library for {0}.{1} solver", reader.getNodeName(), solver_name));
        shared_ptr<Solver> solver = loadSolver(reader.getNodeName(), *lib, solver_name, name);
        solver->loadConfiguration(reader, *this);
        if (!this->solvers.insert(std::make_pair(name, solver)).second)
            throw NamesConflictException("Solver", name);
    }
}

void Manager::loadConnects(XMLReader& reader)
{
    writelog(LOG_ERROR, "Loading interconnects only possible from Python interface. Ignoring XML section <connects>.");
    reader.gotoEndOfCurrentTag();
}

void Manager::loadScript(XMLReader& reader)
{
    if (reader.getNodeType() != XMLReader::NODE_ELEMENT || reader.getNodeName() != "script")
        throw XMLUnexpectedElementException(reader, "<script>");
    scriptline = reader.getLineNr();
    std::string scr = reader.requireTextInCurrentTag();
    size_t start;
    for (start = 0; scr[start] != '\n' && start < scr.length(); ++start) {
        if (!std::isspace(scr[start]))
            throw XMLException(reader, "Script must begin from new line after <script>");
    }
    if (start != scr.length()) script = scr.substr(start+1);
}


/*static inline MaterialsDB& getMaterialsDBfromSource(const Manager::MaterialsDB& materialsDB) {
    const GeometryReader::MaterialsDBSource* src = materialsDB.target<const GeometryReader::MaterialsDBSource>();
    return src ? const_cast<MaterialsDB&>(src->materialsDB) : MaterialsDB::getDefault();
}*/

void Manager::load(XMLReader& reader, MaterialsDB& materialsDB,
                   const LoadFunCallbackT& load_from,
                   const std::function<bool(const std::string& section_name)>& section_filter)
{
    try {
        reader.requireTag(TAG_NAME_ROOT);
        reader.removeAlienNamespaceAttr();  // possible schema decl. will be removed
        auto logattr = reader.getAttribute("loglevel");
        if (logattr && !forcedLoglevel) {
            try {
                maxLoglevel = LogLevel(boost::lexical_cast<unsigned>(*logattr));
            } catch (boost::bad_lexical_cast) {
                maxLoglevel = reader.enumAttribute<LogLevel>("loglevel")
                    .value("critical-error", LOG_CRITICAL_ERROR)
                    .value("critical", LOG_CRITICAL_ERROR)
                    .value("error", LOG_ERROR)
                    .value("error-detail", LOG_ERROR_DETAIL)
                    .value("warning", LOG_WARNING)
                    .value("info", LOG_INFO)
                    .value("result", LOG_RESULT)
                    .value("data", LOG_DATA)
                    .value("detail", LOG_DETAIL)
                    .value("debug", LOG_DEBUG)
                    .get(maxLoglevel);
            }
        }

        size_t next = 0;

        reader.requireTag();

        if (reader.getNodeName() == TAG_NAME_DEFINES) {
            next = 1;
            if (section_filter(TAG_NAME_DEFINES)) {
                if (!tryLoadFromExternal(reader, materialsDB, load_from)) loadDefines(reader);
            } else
                reader.gotoEndOfCurrentTag();
            if (!reader.requireTagOrEnd()) return;
        }

        if (reader.getNodeName() == TAG_NAME_MATERIALS) {
            next = 2;
            if (section_filter(TAG_NAME_MATERIALS)) {
                if (!tryLoadFromExternal(reader, materialsDB, load_from)) loadMaterials(reader, materialsDB);
            } else
                reader.gotoEndOfCurrentTag();
            if (!reader.requireTagOrEnd()) return;
        }

        if (reader.getNodeName() == TAG_NAME_GEOMETRY) {
            next = 3;
            if (section_filter(TAG_NAME_GEOMETRY)) {
                if (!tryLoadFromExternal(reader, materialsDB, load_from)) {
                    GeometryReader greader(*this, reader, materialsDB);
                    loadGeometry(greader);
                }
            } else
                reader.gotoEndOfCurrentTag();
            if (!reader.requireTagOrEnd()) return;
        }

        if (reader.getNodeName() == TAG_NAME_GRIDS) {
            next = 4;
            if (section_filter(TAG_NAME_GRIDS)) {
                if (!tryLoadFromExternal(reader, materialsDB, load_from)) loadGrids(reader);
            } else
                reader.gotoEndOfCurrentTag();
            if (!reader.requireTagOrEnd()) return;
        }

        if (reader.getNodeName() == TAG_NAME_SOLVERS) {
            next = 5;
            if (section_filter(TAG_NAME_SOLVERS)) {
                if (!tryLoadFromExternal(reader, materialsDB, load_from)) loadSolvers(reader);
            } else
                reader.gotoEndOfCurrentTag();
            if (!reader.requireTagOrEnd()) return;
        }

        if (reader.getNodeName() == TAG_NAME_CONNECTS) {
            next = 6;
            if (section_filter(TAG_NAME_CONNECTS)) {
                if (!tryLoadFromExternal(reader, materialsDB, load_from)) loadConnects(reader);
            } else
                reader.gotoEndOfCurrentTag();
            if (!reader.requireTagOrEnd()) return;
        }

        if (reader.getNodeName() == TAG_NAME_SCRIPT) {
            next = 7;
            if (section_filter(TAG_NAME_SCRIPT)) {
                if (!tryLoadFromExternal(reader, materialsDB, load_from)) loadScript(reader);
            } else
                reader.gotoEndOfCurrentTag();
            if (!reader.requireTagOrEnd()) return;
        }

        const char* tags[] = { TAG_NAME_DEFINES, TAG_NAME_MATERIALS, TAG_NAME_GEOMETRY, TAG_NAME_GRIDS,
                               TAG_NAME_SOLVERS, TAG_NAME_CONNECTS, TAG_NAME_SCRIPT };
        std::string msg;
        for (; next != 7; ++next) {
            msg += "<"; msg += tags[next]; msg += ">, ";
        }
        if (msg != "") msg += "or ";
        msg += "</plask>";

        throw XMLUnexpectedElementException(reader, msg);

    } catch (const XMLException&) {
        throw;
    } catch (const std::exception& err) {
        throw XMLException(reader, err.what());
    // } catch (...) {
    //     throw XMLException(reader, "Unrecognized exception");
    }
}

/*
Validate positions:

Notatki:
1. obiekt istotny w geometrii to obiekt mający nazwę, taki że na ścieżce od korzenia (geometrii) do tego obiektu nie ma innego obiektu mającego nazwe
    mozna je znaleźć przechodząc drzew w głąb aż do napotkania obiektu mającego nazwę (wtedy przestajemy schodzić w głąb)
2. geometrie są porównywane parami, każda para zwiera:
     geometrie tych samych typów, takie że jedna nie jest zawarta w drugiej
3. sprawdzane są tylko obiekty należące do sumy zbiorów obiektów istotnych obu geometrii
    (uwaga: obiekt który nie jest istotny w geometrii nadal może być w niej obiektem nazwanym)
4. ustalane i porównywane są pozycje każdego takiego obiektu w obu geometriach i:
    - jeśli obiekt nie występuje w jednej z geometrii ostrzeżenie NIE jest drukowane
    - jeśli zbiory pozycji w geometriach są równej wielkości i nie są takie same ostrzeżenie JEST drukowane
    - jeśli zbiory pozycji w geometriach mają elementy wspólne ostrzeżenie NIE jest drukowane
    - w pozostałych przypadkach ostrzeżenie JEST drukowane
*/

struct PositionValidator {

    typedef std::map<const GeometryObject*, const char*> GeomToName;

    // always sorted
    typedef std::vector<const GeometryObject*> Set;

    GeomToName& geom_to_name;

    std::map<const Geometry*, Set> cache;

    PositionValidator(GeomToName& geom_to_name): geom_to_name(geom_to_name) {}

    Set& get(const Geometry* geometry) {
        auto it = cache.find(geometry);
        if (it != cache.end())
            return it->second;
        Set& res = cache[geometry];
        fill(geometry->getObject3D().get(), res);
        std::sort(res.begin(), res.end());
        return res;
    }

    void fill(const GeometryObject* obj, Set& s) {
        if (!obj) return;
        if (geom_to_name.count(obj))
            s.push_back(obj);
        else {
            std::size_t c = obj->getRealChildrenCount();
            for (std::size_t i = 0; i < c; ++i)
                fill(obj->getChildNo(i).get(), s);
        }
    }

    /**
     * Compare two vectors of positions.
     * @param v1, v2 vectors to compare
     * @return @c true if there are (probably) no mistakes and @c false in other cases
     */
    // TODO what with NaNs in vectors?
    template <typename VectorType>
    bool compare_vec(std::vector<VectorType> v1, std::vector<VectorType> v2) {
        if (v1.empty() || v2.empty()) return true;
        std::sort(v1.begin(), v1.end());
        std::sort(v2.begin(), v2.end());
        if (v1.size() == v2.size()) {
            return std::equal(v1.begin(), v1.end(), v2.begin(),
                              [](const VectorType& x1, const VectorType& x2) { return isnan(x1) || isnan(x2) || x1.equal(x2, 1e-10); });
        }   // different sizes:
        auto v2_last_match_it = v2.begin(); // != v2.end() since v2 is not empty
        for (VectorType point: v1) {
            while (true) {
                if (point.equal(*v2_last_match_it, 1e-10)) return true; //common point
                if (point < *v2_last_match_it) break;   // point is not included in v2, we are going to check next point from v1
                ++v2_last_match_it;
                if (v2_last_match_it == v2.end()) return false; // no common points found so far
            }
        }
        return false;
    }

    /**
     * Compare positions of important objects in two geometries of the same type.
     * @param geom1, geom2 geometries
     * @return set of such objects that by mistake have other positions in @p geom1 and @p geom2
     */
    template <typename GeomType>
    Set compare_d(const GeomType* geom1, const GeomType* geom2) {
        auto c1 = geom1->getChildUnsafe();
        auto c2 = geom2->getChildUnsafe();
        if (!c1 || !c2) return {}; //one geom. is empty
        if (geom1->hasInSubtree(*c2) || geom2->hasInSubtree(*c1)) return {};
        Set obj_to_check;
        {
            Set og1 = get(geom1);
            Set og2 = get(geom2);
            std::set_union(og1.begin(), og1.end(), og2.begin(), og2.end(), std::back_inserter(obj_to_check));
        }
        Set result;
        for (auto o: obj_to_check)
            if (!compare_vec(geom1->getObjectPositions(*o), geom2->getObjectPositions(*o))) {
                //found problem, for obj geom_to_name[o]
                result.push_back(o);
            }
        return result;
    }

    Set compare(const Geometry* g1, const Geometry* g2) {
        if (const GeometryD<2>* g1_2d = dynamic_cast<const GeometryD<2>*>(g1))
            return compare_d(g1_2d, static_cast<const GeometryD<2>*>(g2));
        if (const GeometryD<3>* g1_3d = dynamic_cast<const GeometryD<3>*>(g1))
            return compare_d(g1_3d, static_cast<const GeometryD<3>*>(g2));
        return {};
    }
};

void Manager::validatePositions(
        const std::function<void(const Geometry*, const Geometry*, std::vector<const GeometryObject*>&&, const std::map<const GeometryObject*, const char*>&)>& callback
    ) const
{
    // split geometries by types, we will compare each pairs of geometries of the same type:
    typedef std::map<std::type_index, std::set<const Geometry*> > GeomToType;
    GeomToType geometries_by_type;
    for (auto& geom: roots)
        geometries_by_type[std::type_index(typeid(*geom))].insert(geom.get());
    if (std::find_if(geometries_by_type.begin(), geometries_by_type.end(), [] (GeomToType::value_type& v) { return v.second.size() > 1; }) == geometries_by_type.end())
        return; // no 2 geometries of the same type

    PositionValidator::GeomToName geom_to_name;
    for (auto& i: geometrics)
        geom_to_name[i.second.get()] = i.first.c_str();
    PositionValidator important_obj(geom_to_name);

    for (auto& geom_set: geometries_by_type)
        for (auto it = geom_set.second.begin(); it != geom_set.second.end(); ++it) {
            auto it2 = it;
            ++it2;
            for (; it2 != geom_set.second.end(); ++it2) {
                PositionValidator::Set objs = important_obj.compare(*it, *it2);
                if (!objs.empty())
                    callback(*it, *it2, std::move(objs), geom_to_name);
            }
        }
}

static std::string geomName(const Manager& m, const Geometry* g, const std::map<const GeometryObject*, const char*>& names) {
    auto it = names.find(g);
    if (it != names.end()) {
        std::string r = "'";
        r += it->second;
        r += '\'';
        return r;
    } else {
        std::string r = "[";
        r += boost::lexical_cast<std::string>(m.getRootIndex(g));
        r += ']';
        return r;
    }
}

void Manager::validatePositions() const {
    validatePositions(
    [this] (const Geometry* g1, const Geometry* g2, std::vector<const GeometryObject*>&& objs, const std::map<const GeometryObject*, const char*>& names) {
        bool single = objs.size() < 2;
        std::string ons;
        for (auto o: objs) {
            ons += " '"; ons += names.find(o)->second; ons += '\'';
        }
        writelog(plask::LOG_WARNING, "Object{}{} ha{} different position in geometry {} and {}",
                 single?"":"s", ons, single?"s":"ve", geomName(*this, g1, names), geomName(*this, g2, names));
    });
}

std::size_t Manager::getRootIndex(const Geometry *geom) const {
    for (std::size_t result = 0; result < roots.size(); ++result)
        if (roots[result].get() == geom) return result;
    return roots.size();
}

} // namespace plask
