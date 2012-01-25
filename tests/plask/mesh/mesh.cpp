#include <boost/test/unit_test.hpp>

#include <plask/mesh/mesh.h>

BOOST_AUTO_TEST_SUITE(mesh) // MUST be the same as the file name

BOOST_AUTO_TEST_CASE(Mesh) {

    struct OnePoint3dMesh: public plask::Mesh<plask::space::Cartesian3d> {
    
        //Held point:
        plask::Vec<3, double> point;
        
        OnePoint3dMesh(const plask::Vec<3, double>& point)
        : point(point) {}

        //Iterator:
        struct IteratorImpl: public Mesh<plask::space::Cartesian3d>::IteratorImpl {

            //point to mesh or is equal to nullptr for end iterator
            const OnePoint3dMesh* mesh_ptr;
            
            //mesh == nullptr for end iterator
            IteratorImpl(const OnePoint3dMesh* mesh)
            : mesh_ptr(mesh) {}

            virtual const plask::Vec<3, double> dereference() const {
                return mesh_ptr->point;
            }

            virtual void increment() {
                mesh_ptr = nullptr; //we iterate only over one point, so next state is end
            }

            virtual bool equal(const typename Mesh<plask::space::Cartesian3d>::IteratorImpl& other) const {
                return mesh_ptr == static_cast<const IteratorImpl&>(other).mesh_ptr;
            }
            
            virtual IteratorImpl* clone() const {
                return new IteratorImpl(mesh_ptr);
            }

        }; 
        
        //plask::Mesh<plask::space::Cartesian3d> methods implementation:
        
        virtual std::size_t size() const {
            return 1;
        }

        virtual typename Mesh<plask::space::Cartesian3d>::Iterator begin() {
            return Mesh<plask::space::Cartesian3d>::Iterator(new IteratorImpl(this));
        }

        virtual typename Mesh<plask::space::Cartesian3d>::Iterator end() {
            return Mesh<plask::space::Cartesian3d>::Iterator(new IteratorImpl(nullptr));
        }
        
    };
    
    OnePoint3dMesh mesh(plask::vec(1.0, 2.0, 3.0));
    OnePoint3dMesh::Iterator it = mesh.begin();
    BOOST_CHECK(it != mesh.end());
    BOOST_CHECK_EQUAL(*it, plask::vec(1.0, 2.0, 3.0));
    ++it;
    BOOST_CHECK(it == mesh.end());
}

BOOST_AUTO_TEST_CASE(SimpleMeshAdapter) {
    //Create 3d mesh which use std::vector of 3d points as internal representation:
    plask::SimpleMeshAdapter< std::vector< plask::Vec<3, double> >, plask::space::Cartesian3d > mesh;
    mesh.internal.push_back(plask::vec(1.0, 1.2, 3.0));
    mesh->push_back(plask::vec(3.0, 4.0, 0.0));
    BOOST_CHECK_EQUAL(mesh.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()