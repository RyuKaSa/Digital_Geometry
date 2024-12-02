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

using namespace std;
using namespace DGtal;
using namespace Z2i;

namespace fs = std::filesystem;

// Define direction mappings for 4-connected Freeman chain codes
const std::vector<std::pair<int, int>> directionOffsets = {
    {1, 0},   // 0: East
    {0, 1},   // 1: North
    {-1, 0},  // 2: West
    {0, -1}   // 3: South
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
                continue;
            }

            std::vector<SCell> boundary;
            Surfaces<KSpace>::track2DBoundary(boundary, kSpace, adjacency, component.pointSet(), bel);

            // Use your existing logic for Board2D and saving
            Board2D bBoard;

            // Add the boundary cells to the board
            for (const auto &cell : boundary)
            {
                bBoard << cell; // Add each boundary cell to the board
            }

            // Save the visualization to a single SVG file
            std::string boundaryFileName = std::filesystem::path(fileName).stem().string() + "_boundary.svg";
            bBoard.saveSVG(("resources/" + boundaryFileName).c_str());
            std::cout << "Saved boundary visualization to: " << boundaryFileName << std::endl;
            std::cout << "-----------------------------" << std::endl;


            // ============================
            // STEP 4: POLYGONIZE DIGITAL OBJECT BOUNDARY
            // ============================

            // Type definitions aligned with the working example
            typedef FreemanChain<int> Contour4;
            typedef ArithmeticalDSSComputer<Contour4::ConstIterator, int, 4> DSS4;
            typedef GreedySegmentation<DSS4> Decomposition4;

            if (!boundary.empty())
            {
                try
                {
                    // Extract boundary surfels
                    std::vector<SCell> boundary;
                    Surfaces<KSpace>::track2DBoundary(boundary, kSpace, adjacency, component.pointSet(), bel);

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
                                if (dx > 0) {
                                    ss << "0"; // East
                                    dx -= 1;
                                } else if (dx < 0) {
                                    ss << "2"; // West
                                    dx += 1;
                                } else if (dy > 0) {
                                    ss << "1"; // North
                                    dy -= 1;
                                } else if (dy < 0) {
                                    ss << "3"; // South
                                    dy += 1;
                                }
                            }
                        }
                    }

                    // Construct the FreemanChain object
                    Contour4 theContour(ss);

                    // Initialize GreedySegmentation with DSSComputer
                    Decomposition4 segmentation(theContour.begin(), theContour.end(), DSS4());

                    // Visualization
                    Board2D polygonBoard;

                    // Optional: Display the original boundary for comparison
                    polygonBoard << CustomStyle("Boundary", new CustomPenColor(Color::Blue));
                    polygonBoard << theContour; // Draw the original boundary in blue

                    // Define a custom style for the polygonized boundary segments
                    polygonBoard << CustomStyle("PolygonSegments", new CustomPenColor(Color::Red));

                    // Draw each segmented line in red
                    for (auto it = segmentation.begin(); it != segmentation.end(); ++it)
                    {
                        polygonBoard << SetMode("ArithmeticalDSS", "Points")
                                    << it->primitive();
                        polygonBoard << SetMode("ArithmeticalDSS", "BoundingBox")
                                    << CustomStyle("ArithmeticalDSS/BoundingBox",
                                                    new CustomPenColor(Color::Red))
                                    << it->primitive();
                    }

                    // Save the visualization to an SVG file
                    std::string polygonFileName = std::filesystem::path(fileName).stem().string() + "_polygon.svg";
                    polygonBoard.saveSVG(("resources/" + polygonFileName).c_str());

                    std::cout << "Polygonized boundary saved to: " << polygonFileName << std::endl;
                    // print the chain code
                    std::cout << "Chain code: " << ss.str() << std::endl;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Polygonization failed: " << e.what() << std::endl;
                }

                std::cout << "=============================" << std::endl;
            }
            else
            {
                std::cout << "Boundary is empty. Skipping polygonization." << std::endl;
                std::cout << "=============================" << std::endl;
            }

        }
        else
        {
            std::cout << "No valid components found. Skipping Freeman chain processing." << std::endl;
            std::cout << "=============================" << std::endl;
        }
    }

    return 0;
}