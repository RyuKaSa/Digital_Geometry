#include <DGtal/base/Common.h>
#include <DGtal/helpers/StdDefs.h>
#include <DGtal/images/ImageSelector.h>
#include <DGtal/io/readers/GenericReader.h>
#include <DGtal/io/writers/GenericWriter.h>
#include <DGtal/io/viewers/Viewer3D.h>
#include <DGtal/images/imagesSetsUtils/SetFromImage.h>
#include <DGtal/topology/CubicalComplex.h>

#include <filesystem>


using namespace std;
using namespace DGtal;
using namespace Z3i;

int main(int argc, char** argv)
{
    setlocale(LC_NUMERIC, "us_US"); //To prevent French local settings
    typedef ImageSelector<Z3i::Domain, int>::Type Image3D;
    typedef Object<DT26_6, DigitalSet> ObjectType26_6;
    typedef Object<DT6_26, DigitalSet> ObjectType6_26;
    typedef map<Cell, CubicalCellData>   Map;
    typedef CubicalComplex< KSpace, Map > CC; 
    
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    // read a 3D image
    Image3D image = VolReader<Image3D>::importVol("3D/Torus_Knot-64.vol");

    // make the foreground and background digital sets from the image
    Z3i::DigitalSet set_foreground ( image.domain() );                                 // Create a digital set of proper size
    Z3i::DigitalSet set_background ( image.domain() );
    SetFromImage<Z3i::DigitalSet>::append<Image3D>(set_foreground, image, 0, 255);     //populate a digital set from the input image
    SetFromImage<Z3i::DigitalSet>::append<Image3D>(set_background, image, -1, 1);

    // add your codes here.
    ObjectType26_6 object_foreground(dt26_6, set_foreground);
    ObjectType6_26 object_background(dt6_26, set_background);
    
    /*std::vector< ObjectType26_6 > objects26_6;
    std::back_insert_iterator< std::vector< ObjectType26_6 > > inserter26_6( objects26_6 );

    std::vector< ObjectType6_26 > objects6_26;
    std::back_insert_iterator< std::vector< ObjectType6_26 > > inserter6_26( objects6_26 );

    unsigned int nbc26_6 = object_foreground.writeComponents( inserter26_6 );
    std::cout << " number of components foreground : " << objects26_6.size() << endl; // Right now size of "objects" is the number of conected components

    /*unsigned int nbc6_26 = object_background.writeComponents( inserter6_26 );
    std::cout << " number of components background : " << objects26_6.size() << endl; // Right now size of "objects" is the number of conected components*/

    KSpace K;
    K.init (  set_foreground.domain().lowerBound(), set_foreground.domain().upperBound(), true );
    CC complex ( K );
    complex.construct(set_foreground);
    cout << "0-cells : " << complex.getCells(0).size() << endl;
    cout << "1-cells : " << complex.getCells(1).size() << endl;
    cout << "2-cells : " << complex.getCells(2).size() << endl;
    cout << "3-cells : " << complex.getCells(3).size() << endl;
    int euler = complex.getCells(0).size() - complex.getCells(1).size() + complex.getCells(2).size() - complex.getCells(3).size();
    cout << "euler : " << euler << endl;


    // 3D viewer
    QApplication application(argc,argv);

    typedef Viewer3D<> MyViewer;
    MyViewer viewer;
    viewer.show();
    //viewer << shape;
    viewer << SetMode3D(image.domain().className(),"BoundingBox");
    viewer << set_foreground << image.domain() << MyViewer::updateDisplay;

    return application.exec();
}