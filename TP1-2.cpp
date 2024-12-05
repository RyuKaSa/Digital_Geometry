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
#include <DGtal/geometry/curves/GreedySegmentation.h>

#include <iostream>
#include <vector>
#include <filesystem>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm> // For std::
#include <numeric>
#include <cmath>
#include <map> // For std::map

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
            // std::cerr << "Invalid direction in chain code: " << direction << std::endl;
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
            // std::cerr << "Cannot close chain with diagonal moves: deltaX = " << deltaX << ", deltaY = " << deltaY << std::endl;
            break;
        }
    }
    return closureChain;
}

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

// Function to analyze perimeter distributions across files
void analyzePerimeterDistributions(const std::map<std::string, std::vector<double>> &perimeters_polygon_by_file)
{
    std::cout << "\n=============================" << std::endl;
    std::cout << "Analyzing perimeter distributions across files:" << std::endl;
    for (const auto &entry : perimeters_polygon_by_file)
    {
        const std::string &fileName = entry.first;
        const std::vector<double> &perimeters = entry.second;

        if (!perimeters.empty())
        {
            double sum = std::accumulate(perimeters.begin(), perimeters.end(), 0.0);
            double average = sum / perimeters.size();
            double median = computeMedian(perimeters);
            double minVal = *std::min_element(perimeters.begin(), perimeters.end());
            double maxVal = *std::max_element(perimeters.begin(), perimeters.end());

            // Compute standard deviation
            double variance = 0.0;
            for (double val : perimeters)
            {
                variance += (val - average) * (val - average);
            }
            variance /= perimeters.size();
            double stddev = std::sqrt(variance);

            std::cout << "-----------------------------" << std::endl;
            std::cout << "File: " << fileName << std::endl;
            std::cout << "Number of grains: " << perimeters.size() << std::endl;
            std::cout << "Perimeter statistics (Polygon Perimeter):" << std::endl;
            std::cout << "Average: " << average << std::endl;
            std::cout << "Median: " << median << std::endl;
            std::cout << "Minimum: " << minVal << std::endl;
            std::cout << "Maximum: " << maxVal << std::endl;
            std::cout << "Standard Deviation: " << stddev << std::endl;
        }
        else
        {
            std::cout << "No perimeters found for file: " << fileName << std::endl;
        }
    }
    std::cout << "=============================" << std::endl;
}

int main(int argc, char **argv)
{
    setlocale(LC_NUMERIC, "us_US"); // To prevent locale issues

    typedef ImageSelector<Domain, unsigned char>::Type Image;                  // Type of image
    typedef DigitalSetSelector<Domain, BIG_DS + HIGH_BEL_DS>::Type DigitalSet; // Digital set type
    typedef Object<DT4_8, DigitalSet> ObjectType;                              // Digital object type

    std::vector<std::string> fileNames;
    std::string directoryPath = "resources/";

    // Map to store perimeters per file
    std::map<std::string, std::vector<double>> perimeters_polygon_by_file;

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
            // Process only the first valid connected component for visualization (Step 4)
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
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Polygonization or visualization failed: " << e.what() << std::endl;
                    std::cout << "=============================" << std::endl;
                }
            }
            else
            {
                std::cout << "Boundary is empty. Skipping polygonization and visualization." << std::endl;
                std::cout << "=============================" << std::endl;
            }

            // ============================
            // STEP 5: CALCULATE AREA AND PERIMETER FOR ALL COMPONENTS
            // ============================

            std::vector<double> areas_2cells;
            std::vector<double> areas_polygon;

            // Step 6: Perimeter calculations
            std::vector<double> perimeters_boundary;
            std::vector<double> perimeters_polygon;

            for (const auto &comp : finalComponents)
            {
                // Compute area as number of 2-cells
                double area_2cells = static_cast<double>(comp.pointSet().size());
                areas_2cells.push_back(area_2cells);

                // Extract boundary for the component
                SurfelAdjacency<2> adjacency(false); // Use 4-connected adjacency
                SCell bel = Surfaces<KSpace>::findABel(kSpace, comp.pointSet(), 10000);

                if (bel == typename KSpace::SCell())
                {
                    std::cerr << "Could not find a valid bel for a component!" << std::endl;
                    continue;
                }

                std::vector<SCell> boundary_comp;
                Surfaces<KSpace>::track2DBoundary(boundary_comp, kSpace, adjacency, comp.pointSet(), bel);

                // Step 6: Perimeter as number of 1-cells
                double perimeter_boundary_val = static_cast<double>(boundary_comp.size());
                perimeters_boundary.push_back(perimeter_boundary_val);

                if (!boundary_comp.empty())
                {
                    try
                    {
                        // Generate Freeman chain code manually
                        std::stringstream ss;
                        ss << kSpace.sKCoords(boundary_comp[0])[0] << " " << kSpace.sKCoords(boundary_comp[0])[1] << " "; // Starting point

                        for (size_t i = 1; i < boundary_comp.size(); ++i)
                        {
                            auto prev = kSpace.sKCoords(boundary_comp[i - 1]);
                            auto curr = kSpace.sKCoords(boundary_comp[i]);

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
                        auto endpoint = computeEndpoint(kSpace.sKCoords(boundary_comp[0])[0], kSpace.sKCoords(boundary_comp[0])[1], ss.str());
                        int deltaX = kSpace.sKCoords(boundary_comp[0])[0] - endpoint.first;
                        int deltaY = kSpace.sKCoords(boundary_comp[0])[1] - endpoint.second;
                        std::string closureChain = generateClosureChain(deltaX, deltaY);
                        ss << closureChain;
                        ss << "\n"; // Ensure the chain ends with a newline

                        // Construct the FreemanChain object
                        typedef FreemanChain<int> Contour4;
                        Contour4 theContour(ss);

                        // Extract ordered list of polygon vertices from the Freeman chain
                        std::vector<Point> polygonVertices;
                        for (auto it = theContour.begin(); it != theContour.end(); ++it)
                        {
                            polygonVertices.push_back(*it);
                        }

                        // Compute polygon area using Shoelace formula
                        double area_polygon_val = computePolygonArea(polygonVertices);
                        areas_polygon.push_back(area_polygon_val);

                        // Step 6: Compute perimeter of the polygon
                        double perimeter_polygon_val = 0.0;
                        size_t numVertices = polygonVertices.size();
                        for (size_t i = 0; i < numVertices; ++i)
                        {
                            const Point &current = polygonVertices[i];
                            const Point &next = polygonVertices[(i + 1) % numVertices];
                            double dx = next[0] - current[0];
                            double dy = next[1] - current[1];
                            perimeter_polygon_val += std::sqrt(dx * dx + dy * dy);
                        }
                        perimeters_polygon.push_back(perimeter_polygon_val);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Area or perimeter calculation failed for a component: " << e.what() << std::endl;
                        continue;
                    }
                }
                else
                {
                    std::cerr << "Boundary is empty for a component. Skipping area and perimeter calculation." << std::endl;
                    continue;
                }
            }

            // Compute statistics for areas_2cells
            if (!areas_2cells.empty())
            {
                double sum_2cells = std::accumulate(areas_2cells.begin(), areas_2cells.end(), 0.0);
                double avg_2cells = sum_2cells / areas_2cells.size();
                double median_2cells = computeMedian(areas_2cells);
                double min_2cells = *std::min_element(areas_2cells.begin(), areas_2cells.end());
                double max_2cells = *std::max_element(areas_2cells.begin(), areas_2cells.end());

                std::cout << "-----------------------------" << std::endl;
                std::cout << "Area statistics (Number of 2-cells):" << std::endl;
                std::cout << "Average: " << avg_2cells << std::endl;
                std::cout << "Median: " << median_2cells << std::endl;
                std::cout << "Minimum: " << min_2cells << std::endl;
                std::cout << "Maximum: " << max_2cells << std::endl;
            }

            // Compute statistics for areas_polygon
            if (!areas_polygon.empty())
            {
                double sum_polygon = std::accumulate(areas_polygon.begin(), areas_polygon.end(), 0.0);
                double avg_polygon = sum_polygon / areas_polygon.size();
                double median_polygon = computeMedian(areas_polygon);
                double min_polygon = *std::min_element(areas_polygon.begin(), areas_polygon.end());
                double max_polygon = *std::max_element(areas_polygon.begin(), areas_polygon.end());

                std::cout << "-----------------------------" << std::endl;
                std::cout << "Area statistics (Polygon Area):" << std::endl;
                std::cout << "Average: " << avg_polygon << std::endl;
                std::cout << "Median: " << median_polygon << std::endl;
                std::cout << "Minimum: " << min_polygon << std::endl;
                std::cout << "Maximum: " << max_polygon << std::endl;
            }

            // Compute statistics for perimeters_boundary
            if (!perimeters_boundary.empty())
            {
                double sum_perimeter_boundary = std::accumulate(perimeters_boundary.begin(), perimeters_boundary.end(), 0.0);
                double avg_perimeter_boundary = sum_perimeter_boundary / perimeters_boundary.size();
                double median_perimeter_boundary = computeMedian(perimeters_boundary);
                double min_perimeter_boundary = *std::min_element(perimeters_boundary.begin(), perimeters_boundary.end());
                double max_perimeter_boundary = *std::max_element(perimeters_boundary.begin(), perimeters_boundary.end());

                std::cout << "-----------------------------" << std::endl;
                std::cout << "Perimeter statistics (Number of 1-cells):" << std::endl;
                std::cout << "Average: " << avg_perimeter_boundary << std::endl;
                std::cout << "Median: " << median_perimeter_boundary << std::endl;
                std::cout << "Minimum: " << min_perimeter_boundary << std::endl;
                std::cout << "Maximum: " << max_perimeter_boundary << std::endl;
            }

            // Compute statistics for perimeters_polygon
            if (!perimeters_polygon.empty())
            {
                double sum_perimeter_polygon = std::accumulate(perimeters_polygon.begin(), perimeters_polygon.end(), 0.0);
                double avg_perimeter_polygon = sum_perimeter_polygon / perimeters_polygon.size();
                double median_perimeter_polygon = computeMedian(perimeters_polygon);
                double min_perimeter_polygon = *std::min_element(perimeters_polygon.begin(), perimeters_polygon.end());
                double max_perimeter_polygon = *std::max_element(perimeters_polygon.begin(), perimeters_polygon.end());

                std::cout << "-----------------------------" << std::endl;
                std::cout << "Perimeter statistics (Polygon Perimeter):" << std::endl;
                std::cout << "Average: " << avg_perimeter_polygon << std::endl;
                std::cout << "Median: " << median_perimeter_polygon << std::endl;
                std::cout << "Minimum: " << min_perimeter_polygon << std::endl;
                std::cout << "Maximum: " << max_perimeter_polygon << std::endl;
            }

            // Store perimeters for analysis
            perimeters_polygon_by_file[fileName] = perimeters_polygon;

            std::cout << "=============================" << std::endl;
        }
        else
        {
            std::cout << "No valid components found. Skipping processing." << std::endl;
            std::cout << "=============================" << std::endl;
        }
    }

    // Call the function to analyze perimeter distributions across files
    analyzePerimeterDistributions(perimeters_polygon_by_file);

    std::cout << "\n"
              << std::endl;
    std::cout << "All files processed successfully." << std::endl;
    return 0;
}