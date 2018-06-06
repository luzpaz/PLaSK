#include "rectilinear3d.h"

#include "regular1d.h"
#include "ordered1d.h"

namespace plask {

#define RECTILINEAR_MESH_3D_DECLARE_ITERATION_ORDER(first, second, third) \
    static std::size_t index_##first##second##third(const RectilinearMesh3D* mesh, std::size_t index0, std::size_t index1, std::size_t index2) { \
        return index##third + mesh->axis[third]->size() * (index##second + mesh->axis[second]->size() * index##first); \
    } \
    static std::size_t index##first##_##first##second##third(const RectilinearMesh3D* mesh, std::size_t mesh_index) { \
        return mesh_index / mesh->axis[third]->size() / mesh->axis[second]->size(); \
    } \
    static std::size_t index##second##_##first##second##third(const RectilinearMesh3D* mesh, std::size_t mesh_index) { \
        return (mesh_index / mesh->axis[third]->size()) % mesh->axis[second]->size(); \
    } \
    static std::size_t index##third##_##first##second##third(const RectilinearMesh3D* mesh, std::size_t mesh_index) { \
        return mesh_index % mesh->axis[third]->size(); \
    }

RECTILINEAR_MESH_3D_DECLARE_ITERATION_ORDER(0,1,2)
RECTILINEAR_MESH_3D_DECLARE_ITERATION_ORDER(0,2,1)
RECTILINEAR_MESH_3D_DECLARE_ITERATION_ORDER(1,0,2)
RECTILINEAR_MESH_3D_DECLARE_ITERATION_ORDER(1,2,0)
RECTILINEAR_MESH_3D_DECLARE_ITERATION_ORDER(2,0,1)
RECTILINEAR_MESH_3D_DECLARE_ITERATION_ORDER(2,1,0)


void RectilinearMesh3D::setIterationOrder(IterationOrder iterationOrder) {
#   define RECTILINEAR_MESH_3D_CASE_ITERATION_ORDER(o1,o2,o3) \
        case ORDER_##o1##o2##o3: \
            index_f = index_##o1##o2##o3; index0_f = index0_##o1##o2##o3; \
            index1_f = index1_##o1##o2##o3; index2_f = index2_##o1##o2##o3; \
            major_axis = &axis[o1]; medium_axis = &axis[o2]; minor_axis = &axis[o3]; \
            break;
    switch (iterationOrder) {
        RECTILINEAR_MESH_3D_CASE_ITERATION_ORDER(0,1,2)
        RECTILINEAR_MESH_3D_CASE_ITERATION_ORDER(0,2,1)
        RECTILINEAR_MESH_3D_CASE_ITERATION_ORDER(1,0,2)
        RECTILINEAR_MESH_3D_CASE_ITERATION_ORDER(1,2,0)
        RECTILINEAR_MESH_3D_CASE_ITERATION_ORDER(2,0,1)
        RECTILINEAR_MESH_3D_CASE_ITERATION_ORDER(2,1,0)
        default:
            index_f = index_210; index0_f = index0_210;  index1_f = index1_210; index2_f = index2_210;
            major_axis = &axis[2]; medium_axis = &axis[1]; minor_axis = &axis[0];
    }
    fireChanged();
}


typename RectilinearMesh3D::IterationOrder RectilinearMesh3D::getIterationOrder() const {
    return this->index_f == decltype(this->index_f)(index_012) ? ORDER_012 :
           this->index_f == decltype(this->index_f)(index_021) ? ORDER_021 :
           this->index_f == decltype(this->index_f)(index_102) ? ORDER_102 :
           this->index_f == decltype(this->index_f)(index_120) ? ORDER_120 :
           this->index_f == decltype(this->index_f)(index_201) ? ORDER_201 :
                                                                 ORDER_210;
}

void RectilinearMesh3D::setOptimalIterationOrder() {
#   define RECTILINEAR_MESH_3D_DETERMINE_ITERATION_ORDER(first, second, third) \
        if (this->axis[third]->size() <= this->axis[second]->size() && this->axis[second]->size() <= this->axis[first]->size()) { \
            setIterationOrder(ORDER_##first##second##third); return; \
        }
    RECTILINEAR_MESH_3D_DETERMINE_ITERATION_ORDER(0,1,2)
    RECTILINEAR_MESH_3D_DETERMINE_ITERATION_ORDER(0,2,1)
    RECTILINEAR_MESH_3D_DETERMINE_ITERATION_ORDER(1,0,2)
    RECTILINEAR_MESH_3D_DETERMINE_ITERATION_ORDER(1,2,0)
    RECTILINEAR_MESH_3D_DETERMINE_ITERATION_ORDER(2,0,1)
    RECTILINEAR_MESH_3D_DETERMINE_ITERATION_ORDER(2,1,0)
}

void RectilinearMesh3D::setAxis(const shared_ptr<MeshAxis> &axis, shared_ptr<MeshAxis> new_val) {
    if (axis == new_val) return;
    unsetChangeSignal(axis);
    const_cast<shared_ptr<MeshAxis>&>(axis) = new_val;
    setChangeSignal(axis);
    fireResized();
}

void RectilinearMesh3D::onAxisChanged(Mesh::Event &e) {
    assert(!e.isDelete());
    fireChanged(e.flags());
}

RectilinearMesh3D::RectilinearMesh3D(IterationOrder iterationOrder)
    : axis{plask::make_shared<OrderedAxis>(), plask::make_shared<OrderedAxis>(), plask::make_shared<OrderedAxis>()} {
    setIterationOrder(iterationOrder);
    setChangeSignal(this->axis[0]);
    setChangeSignal(this->axis[1]);
    setChangeSignal(this->axis[2]);
}

RectilinearMesh3D::RectilinearMesh3D(shared_ptr<MeshAxis> mesh0, shared_ptr<MeshAxis> mesh1, shared_ptr<MeshAxis> mesh2, IterationOrder iterationOrder)
    : axis{std::move(mesh0), std::move(mesh1), std::move(mesh2)}
{
    setIterationOrder(iterationOrder);
    setChangeSignal(this->axis[0]);
    setChangeSignal(this->axis[1]);
    setChangeSignal(this->axis[2]);
}

RectilinearMesh3D::RectilinearMesh3D(const RectilinearMesh3D &src, bool clone_axes)
: RectangularMeshBase3D(src),
  axis{clone_axes ? src.axis[0]->clone() : src.axis[0],
       clone_axes ? src.axis[1]->clone() : src.axis[1],
       clone_axes ? src.axis[2]->clone() : src.axis[2]}
{
    setIterationOrder(src.getIterationOrder());
    setChangeSignal(this->axis[0]);
    setChangeSignal(this->axis[1]);
    setChangeSignal(this->axis[2]);
}

RectilinearMesh3D::~RectilinearMesh3D() {
    unsetChangeSignal(this->axis[0]);
    unsetChangeSignal(this->axis[1]);
    unsetChangeSignal(this->axis[2]);
}

} // namespace plask




