#include <DGtal/base/Common.h>
#include <DGtal/helpers/StdDefs.h>
#include <DGtal/images/ImageSelector.h>
#include "DGtal/io/readers/PGMReader.h"
#include "DGtal/io/writers/GenericWriter.h"
#include <DGtal/images/imagesSetsUtils/SetFromImage.h>
#include <DGtal/io/boards/Board2D.h>
#include <DGtal/io/colormaps/ColorBrightnessColorMap.h>
#include <DGtal/topology/SurfelAdjacency.h>
#include <DGtal/topology/helpers/Surfaces.h>
#include "DGtal/io/Color.h"

using namespace std;
using namespace DGtal;
using namespace Z2i;

template<class T>
Curve getBoundary(T & object)
{
    //Khalimsky space
    KSpace kSpace;
    // we need to add a margine to prevent situations such that an object touch the bourder of the domain
    kSpace.init( object.domain().lowerBound() - Point(1,1),
                   object.domain().upperBound() + Point(1,1), true );

    // 1) Call Surfaces::findABel() to find a cell which belongs to the border
    std::vector<Z2i::Point> boundaryPoints; // boundary points are going to be stored here
    // 2) Call Surfece::track2DBoundaryPoints to extract the boundary of the object
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
    setlocale(LC_NUMERIC, "us_US"); //To prevent French local settings
    typedef ImageSelector<Domain, unsigned char >::Type Image; // type of image
    typedef DigitalSetSelector< Domain, BIG_DS+HIGH_BEL_DS >::Type DigitalSet; // Digital set type
    //typedef Object<DT8_4, DigitalSet> ObjectType; // Digital object type
    typedef Object<DT4_8, DigitalSet> ObjectType; // Digital object type


    // read an image
    Image image = PGMReader<Image>::importPGM ("resources/Rice_japonais_seg_bin.pgm");

    // 1) make a "digital set" of proper size
    // 2) populate a digital set from the image using SetFromImage::append()

    // 3) Create a digital object from the digital set
    std::vector< ObjectType > objects;          // All connected components are going to be stored in it
    std::back_insert_iterator< std::vector< ObjectType > > inserter( objects ); // Iterator used to populated "objects".

    // 4) Set the adjacency pair and obtain the connected components using "writeComponents"
    std::cout << " number of components : " << objects.size() << endl; // Right now size of "objects" is the number of conected components

    // This is an example how to create a pdf file for each object
    Board2D aBoard;                                 // use "Board2D" to save output
    //sendToBoard(aBoard, objects[0], Color::Red);   // send the connected component "objects[0]" to "aBoard"
    //aBoard.saveCairo("out.pdf",Board2D::CairoPDF); // do not forget to change the path!

    return 0;
}
