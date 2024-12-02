#include <cmath>
#include <iostream>
#include <sstream>
#include <limits> // For INT32_MAX and INT32_MIN
#include "DGtal/base/Common.h"
#include "DGtal/io/boards/Board2D.h"
#include "DGtal/io/Color.h"
#include "DGtal/io/colormaps/GradientColorMap.h"
#include "DGtal/shapes/Shapes.h"
#include "DGtal/helpers/StdDefs.h"
#include "DGtal/geometry/curves/ArithmeticalDSSComputer.h"
#include "DGtal/geometry/curves/FreemanChain.h"
#include "DGtal/geometry/curves/GreedySegmentation.h"

using namespace std;
using namespace DGtal;
using namespace Z2i;

int main()
{
    trace.beginBlock("Example dgtalboard-5-greedy-dss");

    typedef FreemanChain<int> Contour4;
    typedef ArithmeticalDSSComputer<Contour4::ConstIterator, int, 4> DSS4;
    typedef GreedySegmentation<DSS4> Decomposition4;

    // A Freeman chain code is a string composed by the coordinates of the first pixel, and the list of elementary displacements.
    std::stringstream ss(stringstream::in | stringstream::out);
    ss << "1398 259 11010111010111010111010111010111110101111111010111111111111111111121212121212222222222222222232322222323232323232223232323232323232323232323332323232323232323232333232323232323232333232323232323232333232323232323332323232323232323332323232323233323232323232333232323233323232323232323232323332323232323233323232323232333232323233323232323232333232323233323232323332323332323333303033303030300000000000101000000010100000001010000000101000101000001010101000101010101010101000101010101010101010101010111010101010101010101011101010101010111010101011101010101110101010101010101010101011101010101010101011101010101010111010101011101" << endl;
    // Construct the Freeman chain
    Contour4 theContour(ss);

    // **New Section: Calculate Average Position and Max Size**

    double sumX = 0.0, sumY = 0.0;
    int count = 0;
    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();

    // Iterate through all points in the Freeman chain
    for(auto it = theContour.begin(); it != theContour.end(); ++it)
    {
        Point p = *it;
        sumX += p[0];
        sumY += p[1];
        count++;

        if(p[0] < minX) minX = p[0];
        if(p[0] > maxX) maxX = p[0];
        if(p[1] < minY) minY = p[1];
        if(p[1] > maxY) maxY = p[1];
    }

    // Calculate average positions
    double avgX = sumX / count;
    double avgY = sumY / count;

    // Calculate width and height
    int width = maxX - minX;
    int height = maxY - minY;

    // Define padding to ensure the Freeman chain isn't touching the borders
    int padding = 10; // You can adjust this value as needed

    // Define the new domain based on the calculations
    Point p1(minX - padding, minY - padding);
    Point p2(maxX + padding, maxY + padding);
    Domain domain(p1, p2);

    // **Optional: Print the calculated values for debugging**
    cout << "Average Position: (" << avgX << ", " << avgY << ")\n";
    cout << "Width: " << width << ", Height: " << height << "\n";
    cout << "Domain: (" << p1 << ") to (" << p2 << ")\n";

    // **End of New Section**

    // Segmentation
    Decomposition4 theDecomposition(theContour.begin(), theContour.end(), DSS4());

    // Draw the domain and the contour
    Board2D aBoard;
    aBoard << SetMode(domain.className(), "Grid")
           << domain
           << SetMode("PointVector", "Grid");

    // Draw each segment
    string styleName = "";
    for (Decomposition4::SegmentComputerIterator
             it = theDecomposition.begin(),
             itEnd = theDecomposition.end();
         it != itEnd; ++it)
    {
        aBoard << SetMode("ArithmeticalDSS", "Points")
               << it->primitive();
        aBoard << SetMode("ArithmeticalDSS", "BoundingBox")
               << CustomStyle("ArithmeticalDSS/BoundingBox",
                              new CustomPenColor(Color::Blue))
               << it->primitive();
    }

    // Save the outputs
    aBoard.saveSVG("greedy-dss-decomposition.svg");
    aBoard.saveEPS("greedy-dss-decomposition.eps");
#ifdef WITH_CAIRO
    aBoard.saveCairo("greedy-dss-decomposition.png");
#endif

    trace.endBlock();

    return 0;
}