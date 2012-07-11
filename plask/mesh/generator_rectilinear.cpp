#include <plask/log/log.h>
#include "generator_rectilinear.h"

namespace plask {

shared_ptr<RectilinearMesh2D> RectilinearMesh2DSimpleGenerator::generate(const shared_ptr<GeometryElementD<2>>& geometry)
{
    auto mesh = make_shared<RectilinearMesh2D>();

    std::vector<Box2D> boxes = geometry->getLeafsBoundingBoxes();

    for (auto& box: boxes) {
        mesh->c0.addPoint(box.lower.c0);
        mesh->c0.addPoint(box.upper.c0);
        mesh->c1.addPoint(box.lower.c1);
        mesh->c1.addPoint(box.upper.c1);
    }

    mesh->setOptimalIterationOrder();
    return mesh;
}

shared_ptr<RectilinearMesh3D> RectilinearMesh3DSimpleGenerator::generate(const shared_ptr<GeometryElementD<3>>& geometry)
{
    auto mesh = make_shared<RectilinearMesh3D>();


    std::vector<Box3D> boxes = geometry->getLeafsBoundingBoxes();

    for (auto& box: boxes) {
        mesh->c0.addPoint(box.lower.c0);
        mesh->c0.addPoint(box.upper.c0);
        mesh->c1.addPoint(box.lower.c1);
        mesh->c1.addPoint(box.upper.c1);
        mesh->c2.addPoint(box.lower.c2);
        mesh->c2.addPoint(box.upper.c2);
    }

    mesh->setOptimalIterationOrder();
    return mesh;
}


RectilinearMesh1D RectilinearMesh2DDividingGenerator::get1DMesh(const RectilinearMesh1D& initial, const shared_ptr<GeometryElementD<2>>& geometry, size_t dir)
{
    // TODO: Użyj algorytmu Roberta, może będzie lepszy

    RectilinearMesh1D result = initial;

    // First add refinement points
    for (auto ref: refinements[dir]) {
        auto boxes = geometry->getLeafsBoundingBoxes(&ref.first);
        auto origins = geometry->getLeafsPositions(&ref.first);
        if (warn_multiple && boxes.size() > 1) writelog(LOG_WARNING, "RectilinearMesh2DDividingGenerator: Single refinement defined for more than one object.");
        if (warn_multiple && boxes.size() == 0) writelog(LOG_WARNING, "RectilinearMesh2DDividingGenerator: Refinement defined for object absent from the geometry.");
        auto box = boxes.begin();
        auto origin = origins.begin();
        for (; box != boxes.end(); ++box, ++origin) {
            for (auto x: ref.second) {
                double zero = (*origin)[dir];
                double lower = box->lower[dir] - zero;
                double upper = box->upper[dir] - zero;
                if (warn_outside && (x < lower || x > upper))
                    writelog(LOG_WARNING, "RectilinearMesh2DDividingGenerator: Refinement at %1% outside of the object (%2% to %3%).",
                                           x, lower, upper);
                result.addPoint(zero + x);
            }
        }
    }

    // First divide each element
    double x = *result.begin();
    std::vector<double> points; points.reserve((divisions[dir]-1)*(result.size()-1));
    for (auto i = result.begin()+1; i!= result.end(); ++i) {
        double w = *i - x;
        for (size_t j = 1; j != divisions[dir]; ++j) points.push_back(x + w*j/divisions[dir]);
        x = *i;
    }
    result.addOrderedPoints(points.begin(), points.end());

    if (result.size() <= 2) return result;

    // Now ensure, that the grids do not change to quickly
    size_t end = result.size()-2;
    double w_prev = INFINITY, w = result[1]-result[0], w_next = result[2]-result[1];
    for (size_t i = 0; i <= end;) {
        if (w > 2.*w_prev) {
            result.addPoint(0.5 * (result[i] + result[i+1])); ++end;
            w = w_next = result[i+1] - result[i];
        } else if (w > 2.*w_next) {
            result.addPoint(0.5 * (result[i] + result[i+1])); ++end;
            w_next = result[i+1] - result[i];
            if (i) {
                --i;
                w = w_prev;
                w_prev = (i == 0)? INFINITY : result[i] - result[i-1];
            } else
                w = w_next;
        } else {
            ++i;
            w_prev = w;
            w = w_next;
            w_next = (i == end)? INFINITY : result[i+2] - result[i+1];
        }
    }
    return result;
}

shared_ptr<RectilinearMesh2D> RectilinearMesh2DDividingGenerator::generate(const shared_ptr<GeometryElementD<2>>& geometry)
{
    RectilinearMesh2D initial;
    std::vector<Box2D> boxes = geometry->getLeafsBoundingBoxes();
    for (auto& box: boxes) {
        initial.c0.addPoint(box.lower.c0);
        initial.c0.addPoint(box.upper.c0);
        initial.c1.addPoint(box.lower.c1);
        initial.c1.addPoint(box.upper.c1);
    }

    auto mesh = make_shared<RectilinearMesh2D>();
    mesh->c0 = get1DMesh(initial.c0, geometry, 0);
    mesh->c1 = get1DMesh(initial.c1, geometry, 1);

    mesh->setOptimalIterationOrder();
    return mesh;
}

} // namespace plask