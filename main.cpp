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

#include <iostream>
#include <vector>
#include <filesystem>
#include <string>

using namespace std;
using namespace DGtal;
using namespace Z2i;

namespace fs = std::filesystem;

template<class T>
Curve getBoundary(T & object)
{
    //Khalimsky space
    KSpace kSpace;
    // we need to add a margin to prevent situations where an object touches the boundary of the domain
    kSpace.init(object.domain().lowerBound() - Point(1,1), object.domain().upperBound() + Point(1,1), true);

    // 1) Call Surfaces::findABel() to find a cell belonging to the border
    std::vector<Z2i::Point> boundaryPoints; // boundary points are stored here
    // 2) Call Surfaces::track2DBoundaryPoints to extract the boundary of the object
    Curve boundaryCurve;
    // 3) Create a curve from a vector
    return boundaryCurve;
}

template<class T>
void sendToBoard( Board2D & board, T & p_Object, DGtal::Color p_Color) {
    board << CustomStyle( p_Object.className(), new DGtal::CustomFillColor(p_Color));
    board << p_Object;
}

int main(int argc, char** argv)
{
    setlocale(LC_NUMERIC, "us_US"); // To prevent French locale settings
    typedef ImageSelector<Domain, unsigned char>::Type Image; // Type of image
    typedef DigitalSetSelector<Domain, BIG_DS+HIGH_BEL_DS>::Type DigitalSet; // Digital set type
    typedef Object<DT4_8, DigitalSet> ObjectType; // Digital object type

    std::vector<std::string> fileNames;
    std::string directoryPath = "resources/";

    // Iterate over all files in the directory for .pgm specifically
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            if (filePath.size() >= 12 && filePath.substr(filePath.size() - 12) == "_seg_bin.pgm") {
                fileNames.push_back(filePath);
            }
        }
    }

    std::cout << "Number of files found: " << fileNames.size() << std::endl;
    std::cout << "=============================" << std::endl;

    for (const auto& fileName : fileNames)
    {
        std::cout << "Processing file: " << fileName << std::endl;

        // Read the image from the current filename
        Image image1 = PGMReader<Image>::importPGM(fileName);

        // 1) Create a "digital set" of the same size as the image
        DigitalSet aSet(image1.domain()); // A digital set is created with the same domain as the image

        // 2) Populate the digital set from the image
        SetFromImage<DigitalSet>::append(aSet, image1, 1, 255); // 1 is the background, 255 is the object

        // 3) Create a digital object from the digital set with (4, 8) adjacency
        std::vector<ObjectType> objects; // Store all connected components here
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
        for (const auto& component : objects)
        {
            bool isBoundary = false;
            for (const auto& point : component.pointSet())
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

        // 5) Display the final results for the current file
        std::cout << "Number of components removed: " << boundaryComponents << std::endl;
        std::cout << "Final number of connected components: " << finalComponents.size() << std::endl;
        std::cout << "=============================" << std::endl;

        // save svg of a single component for each file
        Board2D bBoard;

        sendToBoard(bBoard, finalComponents[0], Color::Red);

        // save to file, each file should have a unique name
        bBoard.saveSVG((fileName + "_component.svg").c_str());

        // Step 3: Extract boundary for each final component



    }

    return 0;
}