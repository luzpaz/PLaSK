#ifndef PLASK__MESH_MIRROR_H
#define PLASK__MESH_MIRROR_H

#include "mesh.h"
#include "../geometry/space.h"

namespace plask {

/**
 * Mesh adapter for meshes extending outside of the computational regions and wrapping coordinates for
 * mirror symmetry and periodicity.
 *
 * Right now it only handles mirror symmetry in specified directions. It takes other mesh in the constructor
 * and simple delegates the calls to it replacing negative position on specified axis with
 * its absolute value.
 *
 * This is a dumb version designed for use only as temporary local variables. Do not use it for storing any
 * mesh permanently!
 *
 * TODO handle periodicity as well.
 */
template <int dim>
struct WrappedMesh: public MeshD<dim> {

  protected:

      shared_ptr<const MeshD<dim>> original;                   ///< Original mesh
      shared_ptr<const GeometryD<dim>> geometry;    ///< Geometry of the mesh

      bool ignore_symmetry[dim];                    ///< If true, the structure symmetry is ignored

  public:

    /**
     * Construct mirror adapter
     * \param original original mesh
     * \param geometry geometry in which the mesh is defined
     * \param ignore symmetry parameter specifying if the symmetry should be igroned
     */
    WrappedMesh(shared_ptr<const MeshD<dim>> original, const shared_ptr<const GeometryD<dim>>& geometry, const bool ignore_symmetry[dim]):
        original(original), geometry(geometry) {
        for (size_t i = 0; i != dim; ++i) this->ignore_symmetry[i] = ignore_symmetry[i];
    }

    /**
     * Construct mirror adapter
     * \param original original mesh
     * \param geometry geometry in which the mesh is defined
     */
    WrappedMesh(shared_ptr<const MeshD<dim>> original, const shared_ptr<const GeometryD<dim>>& geometry);

    virtual ~WrappedMesh() {}

    virtual std::size_t size() const;

    virtual Vec<dim> at(std::size_t index) const;

    virtual void writeXML(XMLElement& object) const;
};

//extern template struct PLASK_API WrappedMesh<2>;
//extern template struct PLASK_API WrappedMesh<3>;

} // namespace plask

#endif // PLASK__MESH_MIRROR_H
