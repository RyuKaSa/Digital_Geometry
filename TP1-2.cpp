#include <DGtal/base/Common.h>
#include <DGtal/helpers/StdDefs.h>
#include <DGtal/images/ImageSelector.h>
#include "DGtal/io/readers/PGMReader.h"
#include "DGtal/io/writers/GenericWriter.h"
#include <DGtal/images/imagesSetsUtils/SetFromImage.h>
#include <DGtal/io/boards/Board2D.h>
#include <DGtal/io/Color.h>
#include <DGtal/io/colormaps/ColorBrightnessColorMap.h>
#include <DGtal/topology/SurfelAdjacency.h>
#include <DGtal/topology/helpers/Surfaces.h>
#include "DGtal/io/Color.h"
#include "DGtal/geometry/curves/GreedySegmentation.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm> // For std::sort

using namespace std;
using namespace DGtal;
using namespace Z2i;

namespace fs = std::filesystem;

// Define direction mappings for 4-connected Freeman chain codes
const std::vector<std::pair<int, int>> directionOffsets = {
    {1, 0},  // 0: East
    {0, 1},  // 1: North
    {-1, 0}, // 2: West
    {0, -1}  // 3: South
};

// Function to compute the endpoint based on chain code
std::pair<int, int> computeEndpoint(int startX, int startY, const std::string &chainCode)
{
    int x = startX;
    int y = startY;
    for (char c : chainCode)
    {
        if (!isdigit(c))
            continue; // Ignore non-digit characters
        int direction = c - '0';
        if (direction < 0 || direction >= directionOffsets.size())
        {
            std::cerr << "Invalid direction in chain code: " << direction << std::endl;
            continue;
        }
        x += directionOffsets[direction].first;
        y += directionOffsets[direction].second;
    }
    return {x, y};
}

// Function to generate chain code to close the contour (simple approach)
std::string generateClosureChain(int deltaX, int deltaY)
{
    std::string closureChain = "";
    while (deltaX != 0 || deltaY != 0)
    {
        if (deltaX > 0 && deltaY == 0)
        {
            closureChain += "0"; // Move East
            deltaX -= 1;
        }
        else if (deltaX < 0 && deltaY == 0)
        {
            closureChain += "2"; // Move West
            deltaX += 1;
        }
        else if (deltaY > 0 && deltaX == 0)
        {
            closureChain += "1"; // Move North
            deltaY -= 1;
        }
        else if (deltaY < 0 && deltaX == 0)
        {
            closureChain += "3"; // Move South
            deltaY += 1;
        }
        else
        {
            // For 4-connected, diagonal moves should not occur
            std::cerr << "Cannot close chain with diagonal moves: deltaX = " << deltaX << ", deltaY = " << deltaY << std::endl;
            break;
        }
    }
    return closureChain;
}

// === Added: Helper Functions ===

// Function to compute polygon area using Shoelace formula
double computePolygonArea(const std::vector<Point> &vertices)
{
    if (vertices.size() < 3)
        return 0.0; // Not a polygon

    double area = 0.0;
    size_t n = vertices.size();

    for (size_t i = 0; i < n; ++i)
    {
        const Point &current = vertices[i];
        const Point &next = vertices[(i + 1) % n];
        area += (current[0] * next[1]) - (next[0] * current[1]);
    }

    return std::abs(area) / 2.0;
}

// Function to compute median of a vector
double computeMedian(std::vector<double> data)
{
    if (data.empty())
        return 0.0;

    std::sort(data.begin(), data.end());
    size_t n = data.size();
    if (n % 2 == 0)
    {
        return (data[n / 2 - 1] + data[n / 2]) / 2.0;
    }
    else
    {
        return data[n / 2];
    }
}
int main(int argc, char **argv)
{
    setlocale(LC_NUMERIC, "us_US"); // To prevent locale issues

    typedef ImageSelector<Domain, unsigned char>::Type Image;                  // Type of image
    typedef DigitalSetSelector<Domain, BIG_DS + HIGH_BEL_DS>::Type DigitalSet; // Digital set type
    typedef Object<DT4_8, DigitalSet> ObjectType;                              // Digital object type

    std::vector<std::string> fileNames;
    std::string directoryPath = "resources/";

    // Iterate over all files in the directory for .pgm specifically
    for (const auto &entry : fs::directory_iterator(directoryPath))
    {
        if (entry.is_regular_file())
        {
            std::string filePath = entry.path().string();
            if (filePath.size() >= 12 && filePath.substr(filePath.size() - 12) == "_seg_bin.pgm")
            {
                fileNames.push_back(filePath);
            }
        }
    }

    std::cout << "*****************************" << std::endl;
    std::cout << "Number of files found: " << fileNames.size() << std::endl;
    std::cout << "*****************************" << std::endl;

    for (const auto &fileName : fileNames)
    {
        std::cout << "\n"
                  << std::endl;
        std::cout << "=============================" << std::endl;
        std::cout << "Processing file: " << fileName << std::endl;
        std::cout << "-----------------------------" << std::endl;

        // Read the image from the current filename
        Image image1 = PGMReader<Image>::importPGM(fileName);

        // 1) Create a "digital set" of the same size as the image
        DigitalSet aSet(image1.domain()); // A digital set is created with the same domain as the image

        // 2) Populate the digital set from the image
        SetFromImage<DigitalSet>::append(aSet, image1, 1, 255); // 1 is the background, 255 is the object

        // 3) Create a digital object from the digital set with (4, 8) adjacency
        std::vector<ObjectType> objects;                                      // Store all connected components here
        std::back_insert_iterator<std::vector<ObjectType>> inserter(objects); // Iterator to populate "objects"
        ObjectType diamond(dt4_8, aSet);
        unsigned int nbc = diamond.writeComponents(inserter);

        std::cout << "Initial number of connected components: " << objects.size() << std::endl;

        // Variables to track components
        int boundaryComponents = 0;
        std::vector<ObjectType> finalComponents;

        // Define image bounds for edge checking
        Point lowerBound = image1.domain().lowerBound();
        Point upperBound = image1.domain().upperBound();

        // 4) Boundary check for each component
        for (const auto &component : objects)
        {
            bool isBoundary = false;
            for (const auto &point : component.pointSet())
            {
                if (point[0] == lowerBound[0] || point[0] == upperBound[0] ||
                    point[1] == lowerBound[1] || point[1] == upperBound[1])
                {
                    isBoundary = true;
                    break;
                }
            }

            // If not on boundary, add to final list; otherwise, count it as removed
            if (isBoundary)
            {
                boundaryComponents++;
            }
            else
            {
                finalComponents.push_back(component);
            }
        }

        // ============================
        // STEP 2
        // ============================

        // 5) Display the final results for the current file
        std::cout << "Number of components removed: " << boundaryComponents << std::endl;
        std::cout << "Final number of connected components: " << finalComponents.size() << std::endl;
        std::cout << "-----------------------------" << std::endl;

        // ============================
        // STEP 3
        // ============================

        // 6) Create a Khalimsky space for the entire digital set
        KSpace kSpace;
        kSpace.init(aSet.domain().lowerBound() - Point(1, 1),
                    aSet.domain().upperBound() + Point(1, 1),
                    true);

        if (!finalComponents.empty())
        {
            // Process only the first valid connected component
            const auto &component = finalComponents[0];

            // Extract boundary for the first component
            SurfelAdjacency<2> adjacency(false); // Use 4-connected adjacency
            SCell bel = Surfaces<KSpace>::findABel(kSpace, component.pointSet(), 10000);

            if (bel == typename KSpace::SCell())
            {
                std::cerr << "Could not find a valid bel for the component!" << std::endl;
                std::cout << "=============================" << std::endl;
                continue;
            }

            std::vector<SCell> boundary;
            Surfaces<KSpace>::track2DBoundary(boundary, kSpace, adjacency, component.pointSet(), bel);

            // ============================
            // STEP 4: POLYGONIZE DIGITAL OBJECT BOUNDARY
            // ============================

            // Type definitions
            typedef FreemanChain<int> Contour4;
            typedef ArithmeticalDSSComputer<Contour4::ConstIterator, int, 4> DSS4;
            typedef GreedySegmentation<DSS4> Decomposition4;

            if (!boundary.empty())
            {
                try
                {
                    // Generate Freeman chain code manually
                    std::stringstream ss;
                    ss << kSpace.sKCoords(boundary[0])[0] << " " << kSpace.sKCoords(boundary[0])[1] << " "; // Starting point

                    for (size_t i = 1; i < boundary.size(); ++i)
                    {
                        auto prev = kSpace.sKCoords(boundary[i - 1]);
                        auto curr = kSpace.sKCoords(boundary[i]);

                        int dx = curr[0] - prev[0];
                        int dy = curr[1] - prev[1];

                        // Handle valid moves
                        if (dx == 1 && dy == 0)
                            ss << "0"; // East
                        else if (dx == 0 && dy == 1)
                            ss << "1"; // North
                        else if (dx == -1 && dy == 0)
                            ss << "2"; // West
                        else if (dx == 0 && dy == -1)
                            ss << "3"; // South
                        else
                        {
                            // Handle invalid moves by breaking them into smaller steps
                            while (dx != 0 || dy != 0)
                            {
                                if (dx > 0)
                                {
                                    ss << "0"; // East
                                    dx -= 1;
                                }
                                else if (dx < 0)
                                {
                                    ss << "2"; // West
                                    dx += 1;
                                }
                                if (dy > 0)
                                {
                                    ss << "1"; // North
                                    dy -= 1;
                                }
                                else if (dy < 0)
                                {
                                    ss << "3"; // South
                                    dy += 1;
                                }
                            }
                        }
                    }

                    // Close the chain
                    auto endpoint = computeEndpoint(kSpace.sKCoords(boundary[0])[0], kSpace.sKCoords(boundary[0])[1], ss.str());
                    int deltaX = kSpace.sKCoords(boundary[0])[0] - endpoint.first;
                    int deltaY = kSpace.sKCoords(boundary[0])[1] - endpoint.second;
                    std::string closureChain = generateClosureChain(deltaX, deltaY);
                    ss << closureChain;
                    ss << "\n"; // Ensure the chain ends with a newline

                    // Construct the FreemanChain object
                    Contour4 theContour(ss);

                    // **Calculate Average Position and Max Size**

                    double sumX = 0.0, sumY = 0.0;
                    int count = 0;
                    int minX = std::numeric_limits<int>::max();
                    int maxX = std::numeric_limits<int>::min();
                    int minY = std::numeric_limits<int>::max();
                    int maxY = std::numeric_limits<int>::min();

                    // Iterate through all points in the Freeman chain
                    for (auto it = theContour.begin(); it != theContour.end(); ++it)
                    {
                        Point p = *it;
                        sumX += p[0];
                        sumY += p[1];
                        count++;

                        if (p[0] < minX)
                            minX = p[0];
                        if (p[0] > maxX)
                            maxX = p[0];
                        if (p[1] < minY)
                            minY = p[1];
                        if (p[1] > maxY)
                            maxY = p[1];
                    }

                    // Calculate average positions
                    double avgX = sumX / count;
                    double avgY = sumY / count;

                    // Calculate width and height
                    int width = maxX - minX;
                    int height = maxY - minY;

                    // Define padding to ensure the Freeman chain isn't touching the borders
                    int padding = 10; // Adjust as needed

                    // Define the new domain based on the calculations
                    Point p1(minX - padding, minY - padding);
                    Point p2(maxX + padding, maxY + padding);
                    Domain domain(p1, p2);

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
                             itSeg = theDecomposition.begin(),
                             itEndSeg = theDecomposition.end();
                         itSeg != itEndSeg; ++itSeg)
                    {
                        aBoard << SetMode("ArithmeticalDSS", "Points")
                               << itSeg->primitive();
                        aBoard << SetMode("ArithmeticalDSS", "BoundingBox")
                               << CustomStyle("ArithmeticalDSS/BoundingBox",
                                              new CustomPenColor(Color::Blue))
                               << itSeg->primitive();
                    }

                    // Save the outputs
                    std::string svgFileName = std::filesystem::path(fileName).stem().string() + "_greedy-dss-decomposition.svg";
                    aBoard.saveSVG(("resources/" + svgFileName).c_str());
                    std::cout << "Saved greedy DSS decomposition to: " << svgFileName << std::endl;

                    // ============================
                    // STEP 5: CALCULATE AREA
                    // ============================

                    // 1. Compute area as number of 2-cells
                    double area_2cells = static_cast<double>(component.pointSet().size());

                    // 2. Compute polygon area using Shoelace formula

                    // Extract ordered list of polygon vertices from the Freeman chain
                    std::vector<Point> polygonVertices;
                    for (auto it = theContour.begin(); it != theContour.end(); ++it)
                    {
                        polygonVertices.push_back(*it);
                    }

                    // Compute polygon area using Shoelace formula
                    double area_polygon_val = computePolygonArea(polygonVertices);

                    // Display the computed areas
                    std::cout << "Area of the component (Number of 2-cells): " << area_2cells << std::endl;
                    std::cout << "Area of the component (Polygon Area): " << area_polygon_val << std::endl;

                    std::cout << "=============================" << std::endl;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Polygonization or area calculation failed: " << e.what() << std::endl;
                    std::cout << "=============================" << std::endl;
                }
            }
            else
            {
                std::cout << "Boundary is empty. Skipping polygonization and area calculation." << std::endl;
                std::cout << "=============================" << std::endl;
            }
        }
        else
        {
            std::cout << "No valid components found. Skipping processing." << std::endl;
            std::cout << "=============================" << std::endl;
        }
    }

    return 0;
}