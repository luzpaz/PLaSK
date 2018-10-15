#include "equilateral3d.h"
#include "equilateral3d.h"

namespace plask {

EquilateralMesh3D::EquilateralMesh3D(IterationOrder iterationOrder, Vec<3> vec0, Vec<3> vec1, Vec<3> vec2):
    RectilinearMesh3D(iterationOrder), trans{vec0.c0, vec1.c0, vec2.c0, vec0.c1, vec1.c1, vec2.c1, vec0.c2, vec1.c2, vec2.c2} {
        findInverse();
    }

EquilateralMesh3D::EquilateralMesh3D(shared_ptr<MeshAxis> mesh0, shared_ptr<MeshAxis> mesh1, shared_ptr<MeshAxis> mesh2, IterationOrder iterationOrder,
                                     Vec<3> vec0, Vec<3> vec1, Vec<3> vec2):
    RectilinearMesh3D(std::move(mesh0), std::move(mesh1), std::move(mesh2), iterationOrder),
    trans{vec0.c0, vec1.c0, vec2.c0, vec0.c1, vec1.c1, vec2.c1, vec0.c2, vec1.c2, vec2.c2} {
        findInverse();
    }

shared_ptr<EquilateralMesh3D::ElementMesh> EquilateralMesh3D::getElementMesh() const {
    return plask::make_shared<EquilateralMesh3D::ElementMesh>(this, axis[0]->getMidpointAxis(), axis[1]->getMidpointAxis(), axis[2]->getMidpointAxis(),
                                                              getIterationOrder(), getVec0(), getVec1(), getVec2());
}

} // namespace plask
